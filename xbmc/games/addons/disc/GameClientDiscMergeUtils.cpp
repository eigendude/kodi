/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include "games/addons/disc/GameClientDiscModel.h"

#include <algorithm>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
std::optional<std::string> GetSelectedRealDiscPath(const CGameClientDiscModel& discs)
{
  if (discs.IsSelectedNoDisc())
    return std::nullopt;

  const std::optional<size_t> selectedIndex = discs.GetSelectedDiscIndex();
  if (!selectedIndex.has_value() || !discs.IsRealDiscByIndex(*selectedIndex))
    return std::nullopt;

  return discs.GetPathByIndex(*selectedIndex);
}

bool ContainsDiscPath(const CGameClientDiscModel& discs, const std::optional<std::string>& path)
{
  return path.has_value() && discs.GetDiscIndexByPath(*path).has_value();
}

GameClientDiscEntry MakeRemovedEntry()
{
  return {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""};
}

GameClientDiscEntry MakeDiscEntry(const std::string& path, const std::string& label)
{
  return {GameClientDiscEntry::DiscSlotType::Disc, path, CGameClientDiscModel::DeriveBasename(path),
          label};
}

std::vector<GameClientDiscEntry> BuildReconciledEntries(const CGameClientDiscModel& frontendDiscs,
                                                        const CGameClientDiscModel& coreDiscs)
{
  // Start from the frontend slot layout and normalize every slot to a tombstone.
  // This keeps logical slot history (including already-removed slots), while making it
  // explicit that only discs currently reported by the core should become real discs.
  std::vector<GameClientDiscEntry> reconciled(frontendDiscs.Size(), MakeRemovedEntry());

  // Build a path->index table from the frontend model to preserve logical slot positions.
  // Path identity is stable across compacting cores; index identity is not.
  std::unordered_map<std::string, size_t> frontendPathToIndex;
  frontendPathToIndex.reserve(frontendDiscs.Size());
  for (size_t i = 0; i < frontendDiscs.Size(); ++i)
  {
    if (!frontendDiscs.IsRealDiscByIndex(i))
      continue;

    const std::string path = frontendDiscs.GetPathByIndex(i);
    if (!path.empty())
      frontendPathToIndex.emplace(path, i);
  }

  struct CoreDiscInfo
  {
    std::string path;
    std::string label;
  };

  std::vector<CoreDiscInfo> unmatchedCoreDiscs;
  unmatchedCoreDiscs.reserve(coreDiscs.Size());

  std::vector<bool> slotFilledByCore(reconciled.size(), false);

  // Pass 1: place core discs that match a prior frontend disc path back into their original
  // logical slots. This is the key behavior that preserves tombstones when compacting cores
  // shift indices left after removals.
  for (size_t i = 0; i < coreDiscs.Size(); ++i)
  {
    if (!coreDiscs.IsRealDiscByIndex(i))
      continue;

    const std::string path = coreDiscs.GetPathByIndex(i);
    const std::string label = coreDiscs.GetLabelByIndex(i);

    if (path.empty())
      continue;

    const auto it = frontendPathToIndex.find(path);
    if (it != frontendPathToIndex.end())
    {
      const size_t frontendIndex = it->second;
      reconciled[frontendIndex] = MakeDiscEntry(path, label);
      slotFilledByCore[frontendIndex] = true;
    }
    else
    {
      unmatchedCoreDiscs.push_back({path, label});
    }
  }

  // Pass 2: place core discs that did not exist in the frontend model into the earliest
  // remaining tombstone slots. This allows real core discs to replace stale tombstones while
  // keeping the slot map compact and predictable.
  size_t nextReusableSlot = 0;
  auto PlaceInReusableSlot = [&](const CoreDiscInfo& coreDisc) -> bool
  {
    while (nextReusableSlot < reconciled.size())
    {
      if (!slotFilledByCore[nextReusableSlot])
      {
        reconciled[nextReusableSlot] = MakeDiscEntry(coreDisc.path, coreDisc.label);
        slotFilledByCore[nextReusableSlot] = true;
        ++nextReusableSlot;
        return true;
      }

      ++nextReusableSlot;
    }

    return false;
  };

  for (const CoreDiscInfo& coreDisc : unmatchedCoreDiscs)
  {
    if (PlaceInReusableSlot(coreDisc))
      continue;

    // Pass 3: if the core has grown beyond frontend history, append new live discs.
    reconciled.push_back(MakeDiscEntry(coreDisc.path, coreDisc.label));
    slotFilledByCore.push_back(true);
  }

  return reconciled;
}
} // namespace

void CGameClientDiscMergeUtils::ReconcileModels(CGameClientDiscModel& frontendDiscs,
                                                const CGameClientDiscModel& coreDiscs)
{
  // Frontend selection is remembered as path, not index, so compacting cores can shift a disc
  // while still preserving user intent for the same logical disc.
  const std::optional<std::string> previousSelectedPath = GetSelectedRealDiscPath(frontendDiscs);

  // Core-selected path is a fallback when the previously selected frontend disc no longer exists.
  const std::optional<std::string> coreSelectedPath = GetSelectedRealDiscPath(coreDiscs);

  CGameClientDiscModel reconciled;

  // Reconciliation happens in model semantics, not by naive overlap of slot indices:
  // - Frontend model is authoritative for logical slot history (especially tombstones).
  // - Core model is authoritative for which real discs currently exist.
  // This is necessary because cores can either compact after removals or leave zombie slots.
  reconciled.SetDiscs(BuildReconciledEntries(frontendDiscs, coreDiscs));

  // Tray/ejected status is always live core state and should not be persisted frontend intent.
  reconciled.SetEjected(coreDiscs.IsEjected());

  // Selection precedence:
  // 1) Preserve previously selected frontend disc by path, if still present.
  // 2) Otherwise fall back to the core-selected disc path, if present.
  // 3) Otherwise set explicit no-disc selection.
  if (ContainsDiscPath(reconciled, previousSelectedPath))
  {
    reconciled.SetSelectedDiscByPath(*previousSelectedPath);
  }
  else if (ContainsDiscPath(reconciled, coreSelectedPath))
  {
    reconciled.SetSelectedDiscByPath(*coreSelectedPath);
  }
  else
  {
    reconciled.SetSelectedNoDisc();
  }

  frontendDiscs = reconciled;
}
