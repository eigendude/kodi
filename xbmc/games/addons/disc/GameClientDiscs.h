/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

struct AddonInstance_Game;

namespace KODI
{
namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientDiscs
{
public:
  CGameClientDiscs(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  // Game API functions

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
