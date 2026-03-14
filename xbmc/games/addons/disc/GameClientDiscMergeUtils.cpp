/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscMergeUtils.h"

#include <algorithm>
#include <cstddef>

using namespace KODI;
using namespace GAME;

std::vector<GameClientDiscEntry> KODI::GAME::OverlayRemovedTombstonesByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs)
{
  std::vector<GameClientDiscEntry> discs;
  discs.reserve(std::max(coreDiscs.size(), previousDiscs.size()) + coreDiscs.size());

  size_t coreIndex = 0;
  for (const GameClientDiscEntry& previousDisc : previousDiscs)
  {
    if (previousDisc.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
    {
      discs.push_back(previousDisc);
      continue;
    }

    const auto matchIt = std::find_if(coreDiscs.begin() + static_cast<std::ptrdiff_t>(coreIndex),
                                      coreDiscs.end(),
                                      [&previousDisc](const GameClientDiscEntry& coreDisc) {
                                        return coreDisc.slotType ==
                                                   GameClientDiscEntry::DiscSlotType::Disc &&
                                               coreDisc.path == previousDisc.path;
                                      });

    if (matchIt == coreDiscs.end())
      continue;

    const size_t matchIndex = static_cast<size_t>(matchIt - coreDiscs.begin());
    for (; coreIndex < matchIndex; ++coreIndex)
      discs.push_back(coreDiscs[coreIndex]);

    discs.push_back(*matchIt);
    coreIndex = matchIndex + 1;
  }

  for (; coreIndex < coreDiscs.size(); ++coreIndex)
    discs.push_back(coreDiscs[coreIndex]);

  return discs;
}
