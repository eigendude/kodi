/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscs.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/addons/GameClient.h"

using namespace KODI;
using namespace GAME;

CGameClientDiscs::CGameClientDiscs(CGameClient& gameClient, AddonInstance_Game& addonStruct)
  : m_gameClient(gameClient),
    m_struct(addonStruct)
{
}

bool CGameClientDiscs::SupportsDiskControl() const
{
  return false;
}

bool CGameClientDiscs::SupportsDiskControlLabels() const
{
  return false;
}

bool CGameClientDiscs::SupportsInitialImage() const
{
  return false;
}

bool CGameClientDiscs::GetEjectState(bool& ejected)
{
  ejected = false;
  return false;
}

bool CGameClientDiscs::SetEjectState(bool /*ejected*/)
{
  return false;
}

bool CGameClientDiscs::GetImageIndex(unsigned int& index)
{
  index = 0;
  return false;
}

bool CGameClientDiscs::SetImageIndex(unsigned int /*index*/)
{
  return false;
}

bool CGameClientDiscs::GetNumImages(unsigned int& count)
{
  count = 0;
  return false;
}

bool CGameClientDiscs::AddImageIndex()
{
  return false;
}

bool CGameClientDiscs::ReplaceImageIndex(unsigned int /*index*/, const char* /*path*/)
{
  return false;
}

bool CGameClientDiscs::RemoveImageIndex(unsigned int index)
{
  return ReplaceImageIndex(index, nullptr);
}

bool CGameClientDiscs::SetInitialImage(unsigned int /*index*/, const std::string& /*path*/)
{
  return false;
}

bool CGameClientDiscs::GetImagePath(unsigned int /*index*/, std::string& path)
{
  path.clear();
  return false;
}

bool CGameClientDiscs::GetImageLabel(unsigned int /*index*/, std::string& label)
{
  label.clear();
  return false;
}
