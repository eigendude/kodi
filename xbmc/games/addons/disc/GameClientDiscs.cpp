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
  bool bEjected = true;

  try
  {
    bEjected = m_struct.toAddon->GetEjectState(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetEjectState()");
  }

  return bEjected;
}

bool CGameClientDiscs::SetEjectState(bool ejected)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetEjectState(&m_struct, ejected),
                          "SetEjectState()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetEjectState()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

unsigned int CGameClientDiscs::GetImageIndex()
{
  unsigned int imageIndex = 0;

  try
  {
    imageIndex = m_struct.toAddon->GetImageIndex(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageIndex()");
  }

  return imageIndex;
}

bool CGameClientDiscs::SetImageIndex(unsigned int imageIndex)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->SetImageIndex(&m_struct, imageIndex),
                          "SetImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

unsigned int CGameClientDiscs::GetImageCount()
{
  unsigned int imageCount = 0;

  try
  {
    imageCount = m_struct.toAddon->GetImageCount(&m_struct);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageCount()");
  }

  return imageCount;
}

bool CGameClientDiscs::AddImageIndex()
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->AddImageIndex(&m_struct), "AddImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("AddImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscs::ReplaceImageIndex(unsigned int imageIndex, const std::string& filePath)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->ReplaceImageIndex(&m_struct, imageIndex, filePath.c_str()),
        "ReplaceImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("ReplaceImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscs::RemoveImageIndex(unsigned int imageIndex)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(error = m_struct.toAddon->RemoveImageIndex(&m_struct, imageIndex),
                          "RemoveImageIndex()");
  }
  catch (...)
  {
    m_gameClient.LogException("RemoveImageIndex()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

bool CGameClientDiscs::SetInitialImage(unsigned int imageIndex, const std::string& filePath)
{
  GAME_ERROR error = GAME_ERROR_NO_ERROR;

  try
  {
    m_gameClient.LogError(
        error = m_struct.toAddon->SetInitialImage(&m_struct, imageIndex, filePath.c_str()),
        "SetInitialImage()");
  }
  catch (...)
  {
    m_gameClient.LogException("SetInitialImage()");
  }

  return error == GAME_ERROR_NO_ERROR;
}

std::string CGameClientDiscs::GetImagePath(unsigned int imageIndex)
{
  std::string imagePath;
  char* imagePathRaw = nullptr;

  try
  {
    imagePathRaw = m_struct.toAddon->GetImagePath(&m_struct, imageIndex);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImagePath()");
  }

  if (imagePathRaw)
  {
    imagePath = imagePathRaw;
    m_struct.toAddon->FreeString(&m_struct, imagePathRaw);
  }

  return imagePath;
}

std::string CGameClientDiscs::GetImageLabel(unsigned int imageIndex)
{
  std::string imageLabel;
  char* imageLabelRaw = nullptr;

  try
  {
    imageLabelRaw = m_struct.toAddon->GetImageLabel(&m_struct, imageIndex);
  }
  catch (...)
  {
    m_gameClient.LogException("GetImageLabel()");
  }

  if (imageLabelRaw)
  {
    imageLabel = imageLabelRaw;
    m_struct.toAddon->FreeString(&m_struct, imageLabelRaw);
  }

  return imageLabel;
}
