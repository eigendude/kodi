/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace KODI
{
namespace GAME
{

struct MergedDiscSlots
{
  std::vector<GameClientDiscEntry> discs;
  std::vector<std::optional<size_t>> coreToMerged;
};

MergedDiscSlots MergeCoreSlotsByIndex(const std::vector<GameClientDiscEntry>& previousDiscs,
                                      const std::vector<GameClientDiscEntry>& coreDiscs);

} // namespace GAME
} // namespace KODI
