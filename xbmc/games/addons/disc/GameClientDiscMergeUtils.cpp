/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>
#include <deque>

using namespace KODI;
using namespace GAME;

namespace
{
bool IsRealDisc(const GameClientDiscEntry& entry)
{
  return entry.slotType == GameClientDiscEntry::DiscSlotType::Disc;
}

bool IsRemovedTombstone(const GameClientDiscEntry& entry)
{
  return entry.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot;
}
} // namespace

std::vector<GameClientDiscEntry> KODI::GAME::ReconcileFrontendDiscSlots(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  // Merge strategy:
  // - previous removed slots are frontend tombstones that should remain fixed
  // - fresh core real discs are authoritative and never discarded
  // - compacting cores: place remaining real discs into non-tombstoned slots in order
  // - zombie-slot cores: previous tombstones can continue to occupy those positions
  std::deque<GameClientDiscEntry> coreRealDiscs;
  for (const GameClientDiscEntry& coreDisc : coreDiscs)
  {
    if (IsRealDisc(coreDisc))
      coreRealDiscs.emplace_back(coreDisc);
  }

  std::vector<GameClientDiscEntry> merged;
  merged.reserve(std::max(previousDiscs.size(), coreDiscs.size()));

  for (size_t i = 0; i < previousDiscs.size(); ++i)
  {
    if (IsRemovedTombstone(previousDiscs[i]))
    {
      // Keep frontend tombstones anchored at their previous positions.
      merged.emplace_back(previousDiscs[i]);
      continue;
    }

    if (!coreRealDiscs.empty())
    {
      merged.emplace_back(coreRealDiscs.front());
      coreRealDiscs.pop_front();
      continue;
    }

    // The core shrank and no real disc maps here anymore; keep a tombstone to
    // preserve stable frontend slot identity.
    merged.emplace_back(
        GameClientDiscEntry{GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""});
  }

  // Core grew: append any additional real discs that have no prior frontend slot.
  while (!coreRealDiscs.empty())
  {
    merged.emplace_back(coreRealDiscs.front());
    coreRealDiscs.pop_front();
  }

  return merged;
}

std::optional<size_t> KODI::GAME::MapCoreSelectedDiscToFrontendIndex(
    const std::vector<GameClientDiscEntry>& coreDiscs,
    const std::vector<GameClientDiscEntry>& frontendDiscs,
    std::optional<size_t> coreSelectedIndex)
{
  if (!coreSelectedIndex.has_value() || *coreSelectedIndex >= coreDiscs.size())
    return std::nullopt;

  const GameClientDiscEntry& selectedCoreEntry = coreDiscs[*coreSelectedIndex];
  if (!IsRealDisc(selectedCoreEntry))
    return std::nullopt;

  // Prefer exact path identity to keep selection on the same logical disc.
  for (size_t i = 0; i < frontendDiscs.size(); ++i)
  {
    if (!IsRealDisc(frontendDiscs[i]))
      continue;

    if (frontendDiscs[i].path == selectedCoreEntry.path)
      return i;
  }

  // Fallback: map by real-disc ordinal for cores that don't expose stable paths.
  size_t selectedCoreRealOrdinal = 0;
  for (size_t i = 0; i < *coreSelectedIndex; ++i)
  {
    if (IsRealDisc(coreDiscs[i]))
      ++selectedCoreRealOrdinal;
  }

  size_t frontendRealOrdinal = 0;
  for (size_t i = 0; i < frontendDiscs.size(); ++i)
  {
    if (!IsRealDisc(frontendDiscs[i]))
      continue;

    if (frontendRealOrdinal == selectedCoreRealOrdinal)
      return i;

    ++frontendRealOrdinal;
  }

  return std::nullopt;
}
