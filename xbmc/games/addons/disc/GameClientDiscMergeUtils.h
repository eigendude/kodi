/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"

#include <optional>
#include <vector>

namespace KODI
{
namespace GAME
{

std::vector<GameClientDiscEntry> ReconcileFrontendDiscSlots(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs);

std::optional<size_t> MapCoreSelectedDiscToFrontendIndex(
    const std::vector<GameClientDiscEntry>& coreDiscs,
    const std::vector<GameClientDiscEntry>& frontendDiscs,
    std::optional<size_t> coreSelectedIndex);

} // namespace GAME
} // namespace KODI
