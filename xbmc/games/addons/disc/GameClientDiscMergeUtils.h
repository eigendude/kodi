/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{

class CGameClientDiscModel;

class CGameClientDiscMergeUtils
{
public:
  static void ReconcileModels(CGameClientDiscModel& frontendDiscs, const CGameClientDiscModel& coreDiscs);
};

} // namespace GAME
} // namespace KODI
