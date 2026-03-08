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

bool CGameClientDiscs::GetEjectState()
{
  return false;
}

bool CGameClientDiscs::SetEjectState(bool ejected)
{
  return false;
}

unsigned int CGameClientDiscs::GetImageIndex()
{
  return 0;
}

bool CGameClientDiscs::SetImageIndex(unsigned int imageIndex)
{
  return false;
}

unsigned int CGameClientDiscs::GetImageCount()
{
  return 0;
}

bool CGameClientDiscs::AddImageIndex()
{
  return false;
}

bool CGameClientDiscs::ReplaceImageIndex(unsigned int imageIndex, const std::string& filePath)
{
  return false;
}

bool CGameClientDiscs::RemoveImageIndex(unsigned int imageIndex)
{
  return false;
}

bool CGameClientDiscs::SetInitialImage(unsigned int imageIndex, const std::string& filePath)
{
  return false;
}

std::string CGameClientDiscs::GetImagePath(unsigned int imageIndex)
{
  return "";
}

std::string CGameClientDiscs::GetImageLabel(unsigned int imageIndex)
{
  return "";
}
