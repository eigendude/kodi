/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

std::vector<GameClientDiscEntry> KODI::GAME::OverlayRemovedTombstonesByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  std::vector<GameClientDiscEntry> discs;
  discs.reserve(std::max(coreDiscs.size(), previousDiscs.size()));

  const size_t overlapCount = std::min(previousDiscs.size(), coreDiscs.size());
  for (size_t i = 0; i < overlapCount; ++i)
  {
    // Never replace a real core disc with a frontend tombstone. This keeps
    // compacting cores from losing meaningful live state during overlay.
    const bool coreHasRealDisc = coreDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::Disc;

    // Preserve a previous tombstone only when the corresponding core slot is
    // not a real disc. This lets frontend tombstones survive over removed or
    // placeholder-like core slots.
    const bool preserveRemoved =
        previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot &&
        !coreHasRealDisc;

    if (preserveRemoved)
      discs.push_back(previousDiscs[i]);
    else
      discs.push_back(coreDiscs[i]);
  }

  // If the core shrank, preserve any trailing frontend tombstones so removed
  // slot history is not lost.
  for (size_t i = overlapCount; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      discs.push_back(previousDiscs[i]);
  }

  // If the core grew, append its trailing slots unchanged.
  for (size_t i = overlapCount; i < coreDiscs.size(); ++i)
    discs.push_back(coreDiscs[i]);

  return discs;
}
