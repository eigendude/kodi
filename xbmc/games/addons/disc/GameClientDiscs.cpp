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
#include "games/addons/disc/GameClientDiscTransport.h"
#include "utils/StringUtils.h"

#include <mutex>

using namespace KODI;
using namespace GAME;

CGameClientDiscs::CGameClientDiscs(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess),
    m_transport(std::make_unique<CGameClientDiscTransport>(gameClient, addonStruct, clientAccess)),
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
    m_transport->SetInitialImage(startupIndex, startupPath);

  m_isEjected = m_transport->GetEjectState();
}

void CGameClientDiscs::RefreshDiscState(bool force)
{
  if (!force && m_hasPendingFrontendChanges && !m_isEjected)
    return;

  PopulateModelFromCore(*m_discModel);
}

bool CGameClientDiscs::SetEjected(bool ejected)
{
  if (!m_transport->SetEjectState(ejected))
    return false;

  m_isEjected = m_transport->GetEjectState();
  
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
  if (!m_transport->AddImageIndex())
  {
    // Roll back frontend model on failure.
    m_discModel->RemoveDiscByPath(filePath);
    return false;
  }

  // New slot is the last one
  const unsigned int imageIndex = m_transport->GetImageCount() - 1;

  // Populate the new slot
  if (!m_transport->ReplaceImageIndex(imageIndex, filePath))
  {
    // Best effort rollback in the core
    m_transport->RemoveImageIndex(imageIndex);

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
  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(*discIndex)))
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

  unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel

  if (!filePath.empty())
  {
    const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
    if (!discIndex.has_value())
      return false;

    imageIndex = static_cast<unsigned int>(*discIndex);
  }

  if (!m_transport->SetImageIndex(imageIndex))
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
    if (!m_transport->RemoveImageIndex(index))
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

    if (!m_transport->AddImageIndex())
      return false;

    const unsigned int newIndex = m_transport->GetImageCount() - 1;

    if (!m_transport->ReplaceImageIndex(newIndex, frontendDisc.path))
      return false;
  }

  // Rebuild snapshot after additions
  if (!BuildCoreDiscModel(coreModel))
    return false;

  // Apply selected inserted disc, or "No disc"
  unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel

  if (!m_discModel->IsSelectedNoDisc())
  {
    const std::string selectedPath = m_discModel->GetSelectedDiscPath();
    const auto selectedIndex = coreModel.GetDiscIndexByPath(selectedPath);

    if (!selectedIndex.has_value())
      return false;

    imageIndex = static_cast<unsigned int>(*selectedIndex);
  }

  if (!m_transport->SetImageIndex(imageIndex))
    return false;

  m_hasPendingFrontendChanges = false;
  RefreshDiscState(true);
  return true;
}

void CGameClientDiscs::PopulateModelFromCore(CGameClientDiscModel& model)
{
  model.Clear();

  const unsigned int imageCount = m_transport->GetImageCount();

  for (unsigned int i = 0; i < imageCount; ++i)
  {
    std::string imagePath = m_transport->GetImagePath(i);
    std::string imageLabel = m_transport->GetImageLabel(i);

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

  const unsigned int imageIndex = m_transport->GetImageIndex();

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
