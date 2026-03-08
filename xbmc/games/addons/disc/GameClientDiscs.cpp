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
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/StringUtils.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CGameClientDiscs::CGameClientDiscs(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess),
    m_discModel(std::make_unique<CGameClientDiscModel>())
{
}

CGameClientDiscs::~CGameClientDiscs() = default;

bool CGameClientDiscs::SupportsDiskControl() const
{
  return m_gameClient.SupportsDiscControl();
}

void CGameClientDiscs::Initialize()
{
  //! @todo Initialize the disc model/XML-backed session state
  const unsigned int startupIndex = 0;
  const std::string startupPath;

  // Restore last-used disc from the XML
  if (!startupPath.empty())
    SetInitialImage(startupIndex, startupPath);

  m_isEjected = GetEjectState();
}

void CGameClientDiscs::RefreshDiscState(bool force)
{
  if (!force && m_hasPendingFrontendChanges && !m_isEjected)
    return;

  PopulateModelFromCore(*m_discModel);
}

bool CGameClientDiscs::SetEjected(bool ejected)
{
  return SetEjectState(ejected);
}

bool CGameClientDiscs::QueueAddDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  // Skip duplicates in the frontend model
  if (m_discModel->GetDiscIndexByPath(filePath).has_value())
    return true;

  // Queue in frontend model first
  m_discModel->AddDisc(filePath, "");

  // Libretro only allows changing the inserted image while ejected. If the
  // tray is closed, keep the selection queued in the model and let the caller
  // apply it after opening the tray.
  if (!m_isEjected)
  {
    m_hasPendingFrontendChanges = true;
    return true;
  }

  // Add a new slot in the core
  if (!AddImageIndex())
  {
    // Roll back frontend model on failure.
    m_discModel->RemoveDiscByPath(filePath);
    return false;
  }

  // New slot is the last one
  const unsigned int imageIndex = GetImageCount() - 1;

  // Populate the new slot
  if (!ReplaceImageIndex(imageIndex, filePath))
  {
    // Best effort rollback in the core
    RemoveImageIndex(imageIndex);

    // Roll back frontend model
    m_discModel->RemoveDiscByPath(filePath);

    return false;
  }

  m_hasPendingFrontendChanges = false;

  // Re-sync from live core state
  RefreshDiscState(true);

  return true;
}

bool CGameClientDiscs::QueueRemoveDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return true;

  // Update frontend model first
  m_discModel->RemoveDiscByPath(filePath);

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
  {
    m_hasPendingFrontendChanges = true;
    return true;
  }

  // Remove from the core by current index
  if (!RemoveImageIndex(static_cast<unsigned int>(*discIndex)))
  {
    // Core failed, so rebuild frontend model from live core state
    RefreshDiscState(true);
    return false;
  }

  m_hasPendingFrontendChanges = false;

  // Re-sync from live core state because indexes compact after removal
  RefreshDiscState(true);

  return true;
}

bool CGameClientDiscs::QueueInsertDisc(const std::string& filePath)
{
  // Update frontend selection first
  if (filePath.empty())
    m_discModel->SetSelectedNoDisc();
  else
    m_discModel->SetSelectedDiscByPath(filePath);

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
  {
    m_hasPendingFrontendChanges = true;
    return true;
  }

  unsigned int imageIndex = GetImageCount(); // "No disc" sentinel

  if (!filePath.empty())
  {
    const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
    if (!discIndex.has_value())
      return false;

    imageIndex = static_cast<unsigned int>(*discIndex);
  }

  if (!SetImageIndex(imageIndex))
    return false;

  if (filePath.empty())
  {
    // Keep last disc as-is when temporarily selecting "No disc"
    m_discModel->SetSelectedNoDisc();
  }
  else
  {
    m_discModel->SetLastDiscByPath(filePath);
    m_discModel->SetSelectedDiscByPath(filePath);
  }

  m_hasPendingFrontendChanges = false;

  RefreshDiscState(true);

  return true;
}

bool CGameClientDiscs::SyncDiscsAfterEject()
{
  if (!m_isEjected)
    return true;

  CGameClientDiscModel coreModel;
  if (!BuildCoreDiscModel(coreModel))
    return false;

  // Remove discs missing from frontend model. Do this from highest index to
  // lowest because indices compact.
  std::vector<unsigned int> indicesToRemove;

  for (size_t i = 0; i < coreModel.Size(); ++i)
  {
    const std::string& corePath = coreModel.GetDiscs()[i].path;

    if (!m_discModel->GetDiscIndexByPath(corePath).has_value())
      indicesToRemove.emplace_back(static_cast<unsigned int>(i));
  }

  std::sort(indicesToRemove.rbegin(), indicesToRemove.rend());

  for (unsigned int index : indicesToRemove)
  {
    if (!RemoveImageIndex(index))
      return false;
  }

  // Rebuild snapshot after removals so later indexes are correct
  if (!BuildCoreDiscModel(coreModel))
    return false;

  // Add discs present in frontend model but missing from core model
  for (size_t i = 0; i < m_discModel->Size(); ++i)
  {
    const auto& frontendDisc = m_discModel->GetDiscs()[i];

    if (coreModel.GetDiscIndexByPath(frontendDisc.path).has_value())
      continue;

    if (!AddImageIndex())
      return false;

    const unsigned int newIndex = GetImageCount() - 1;

    if (!ReplaceImageIndex(newIndex, frontendDisc.path))
      return false;
  }

  // Rebuild snapshot after additions
  if (!BuildCoreDiscModel(coreModel))
    return false;

  // Apply selected inserted disc, or "No disc"
  unsigned int imageIndex = GetImageCount(); // "No disc" sentinel

  if (!m_discModel->IsSelectedNoDisc())
  {
    const std::string selectedPath = m_discModel->GetSelectedDiscPath();
    const auto selectedIndex = coreModel.GetDiscIndexByPath(selectedPath);

    if (!selectedIndex.has_value())
      return false;

    imageIndex = static_cast<unsigned int>(*selectedIndex);
  }

  if (!SetImageIndex(imageIndex))
    return false;

  m_hasPendingFrontendChanges = false;
  RefreshDiscState(true);
  return true;
}

void CGameClientDiscs::PopulateModelFromCore(CGameClientDiscModel& model)
{
  model.Clear();

  const unsigned int imageCount = GetImageCount();

  for (unsigned int i = 0; i < imageCount; ++i)
  {
    std::string imagePath = GetImagePath(i);
    std::string imageLabel = GetImageLabel(i);

    if (imagePath.empty())
      imagePath = StringUtils::Format("disc://{}", i);

    model.AddDisc(imagePath, imageLabel);
  }

  if (model.Empty())
  {
    model.AddDisc(m_gameClient.GetGamePath(), "Disc 1");
    return;
  }

  const std::string& firstDiscPath = model.GetDiscs().front().path;
  model.SetMainDiscByPath(firstDiscPath);

  const unsigned int imageIndex = GetImageIndex();

  if (imageIndex < imageCount)
  {
    const std::string insertedDiscPath = model.GetDiscs()[imageIndex].path;
    model.SetLastDiscByPath(insertedDiscPath);
    model.SetSelectedDiscByPath(insertedDiscPath);
  }
  else
  {
    model.SetLastDiscByPath(firstDiscPath);
    model.SetSelectedNoDisc();
  }
}

bool CGameClientDiscs::BuildCoreDiscModel(CGameClientDiscModel& coreModel)
{
  PopulateModelFromCore(coreModel);
  return true;
}

bool CGameClientDiscs::GetEjectState()
{
  bool bEjected = true;

  std::unique_lock lock(m_clientAccess);

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

  {
    std::unique_lock lock(m_clientAccess);

    try
    {
      m_gameClient.LogError(error = m_struct.toAddon->SetEjectState(&m_struct, ejected),
                            "SetEjectState()");
      if (error == GAME_ERROR_NO_ERROR)
        m_isEjected = m_struct.toAddon->GetEjectState(&m_struct);
    }
    catch (...)
    {
      m_gameClient.LogException("SetEjectState()");
    }
  }

  if (error != GAME_ERROR_NO_ERROR)
    return false;

  if (m_isEjected)
  {
    // Tray is now actually open, so queued frontend edits can be pushed
    if (!SyncDiscsAfterEject())
      return false;
  }
  else
  {
    // Refresh after closing too
    RefreshDiscState(true);
  }

  return true;
}

unsigned int CGameClientDiscs::GetImageIndex()
{
  unsigned int imageIndex = 0;

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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

  std::unique_lock lock(m_clientAccess);

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
