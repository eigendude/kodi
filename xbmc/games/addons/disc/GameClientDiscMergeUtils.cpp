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

namespace
{
bool ShouldPreserveRemovedTombstone(const GameClientDiscEntry& previousDisc,
                                    const GameClientDiscEntry& coreDisc)
{
  return previousDisc.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot &&
         coreDisc.slotType == GameClientDiscEntry::DiscSlotType::EmptySlot;
}
} // namespace

bool KODI::GAME::IsSelectableDiscSlot(const GameClientDiscEntry& discEntry)
{
  return discEntry.slotType != GameClientDiscEntry::DiscSlotType::RemovedSlot;
}

MergedDiscSlots KODI::GAME::MergeCoreSlotsByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  MergedDiscSlots merged;
  merged.coreToMerged.resize(coreDiscs.size());
  merged.discs.reserve(std::max(previousDiscs.size(), coreDiscs.size()));

  const size_t overlapCount = std::min(previousDiscs.size(), coreDiscs.size());
  for (size_t i = 0; i < overlapCount; ++i)
  {
    if (ShouldPreserveRemovedTombstone(previousDiscs[i], coreDiscs[i]))
    {
      merged.discs.push_back(previousDiscs[i]);
      continue;
    }

    merged.discs.push_back(coreDiscs[i]);
    merged.coreToMerged[i] = i;
  }

  for (size_t i = overlapCount; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      merged.discs.push_back(previousDiscs[i]);
  }

  for (size_t i = overlapCount; i < coreDiscs.size(); ++i)
  {
    merged.coreToMerged[i] = merged.discs.size();
    merged.discs.push_back(coreDiscs[i]);
  }

  for (size_t i = 0; i < merged.discs.size(); ++i)
  {
    if (IsSelectableDiscSlot(merged.discs[i]))
    {
      merged.firstSelectable = i;
      break;
    }
  }

  return merged;
}
