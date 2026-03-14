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

  const size_t overlapSize = std::min(frontendDiscs.Size(), coreDiscs.Size());

  // Overlap range rules:
  // - Core real disc always wins because core is authoritative for current live media.
  // - Otherwise keep frontend tombstones, because those represent explicit user removal intent.
  // - Non-disc on both sides remains absent (no synthetic slot is inserted).
  for (size_t i = 0; i < overlapSize; ++i)
  {
    if (coreDiscs.IsRealDiscByIndex(i))
    {
      reconciled.AddDisc(coreDiscs.GetPathByIndex(i), coreDiscs.GetLabelByIndex(i));
    }
    else if (frontendDiscs.IsRemovedSlotByIndex(i))
    {
      reconciled.AddRemovedSlot();
    }
  }

  // If the core shrank, preserve trailing frontend tombstones so explicit removals survive reloads.
  for (size_t i = overlapSize; i < frontendDiscs.Size(); ++i)
  {
    if (frontendDiscs.IsRemovedSlotByIndex(i))
      reconciled.AddRemovedSlot();
  }

  // If the core grew, append new trailing real discs from the live core state.
  for (size_t i = overlapSize; i < coreDiscs.Size(); ++i)
  {
    if (coreDiscs.IsRealDiscByIndex(i))
      reconciled.AddDisc(coreDiscs.GetPathByIndex(i), coreDiscs.GetLabelByIndex(i));
  }

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
