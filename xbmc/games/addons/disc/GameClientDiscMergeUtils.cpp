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
  std::vector<GameClientDiscEntry> merged;
  merged.reserve(std::max(previousDiscs.size(), coreDiscs.size()));

  const size_t overlapCount = std::min(previousDiscs.size(), coreDiscs.size());

  for (size_t i = 0; i < overlapCount; ++i)
  {
    const GameClientDiscEntry& previous = previousDiscs[i];
    const GameClientDiscEntry& core = coreDiscs[i];

    const bool previousIsRemoved =
        previous.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot;
    const bool coreIsRealDisc =
        core.slotType == GameClientDiscEntry::DiscSlotType::Disc;

    // A real disc reported by the core always wins.
    //
    // Otherwise, if the frontend previously marked this slot removed, keep that
    // tombstone so the removed slot identity survives cores that either:
    //   - leave behind zombie/non-disc slots, or
    //   - compact away removed slots and shorten the list.
    if (previousIsRemoved && !coreIsRealDisc)
      merged.emplace_back(previous);
    else
      merged.emplace_back(core);
  }

  // If the previous/frontend model is longer, only preserve trailing removed
  // tombstones. These represent removed slots that still matter to the
  // frontend's slot model after a compacting core shrank the list.
  for (size_t i = overlapCount; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      merged.emplace_back(previousDiscs[i]);
  }

  // If the core is longer, append any newly reported trailing slots verbatim.
  for (size_t i = overlapCount; i < coreDiscs.size(); ++i)
    merged.emplace_back(coreDiscs[i]);

  return merged;
}
