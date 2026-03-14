/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/disc/GameClientDiscModel.h"

#include <vector>

namespace KODI
{
namespace GAME
{

/*!
 * \brief Merge frontend removed-slot tombstones onto the current core-reported disc
 * list using index-based overlay semantics.
 *
 * Real discs reported by the core always take precedence over previous
 * tombstones. Previous removed-slot tombstones are preserved only when the
 * corresponding core slot is not a real disc, and trailing previous tombstones
 * are kept when the core shrinks.
 */
std::vector<GameClientDiscEntry> OverlayRemovedTombstonesByIndex(
    const std::vector<GameClientDiscEntry>& previousDiscs,
    const std::vector<GameClientDiscEntry>& coreDiscs);

} // namespace GAME
} // namespace KODI
