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
  //! @todo Restore XML-backed session state if implemented
  const unsigned int startupIndex = 0;
  const std::string startupPath;

  // Restore last-used disc from the XML
  if (!startupPath.empty())
    m_transport->SetInitialImage(startupIndex, startupPath);

  m_isEjected = m_transport->GetEjectState();
  RefreshDiscState();
}

void CGameClientDiscs::RefreshDiscState()
{
  PopulateModelFromCore(*m_discModel);
}

bool CGameClientDiscs::SetEjected(bool ejected)
{
  if (!m_transport->SetEjectState(ejected))
    return false;

  m_isEjected = m_transport->GetEjectState();
  RefreshDiscState();

  return true;
}

bool CGameClientDiscs::AddDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  // Skip duplicates in the frontend model
  if (m_discModel->GetDiscIndexByPath(filePath).has_value())
    return true;

  // Libretro only allows changing the inserted image while ejected
  if (!m_isEjected)
    return true;

  // Add a new slot in the core
  if (!m_transport->AddImageIndex())
    return false;

  // New slot is the last one
  const unsigned int newIndex = m_transport->GetImageCount() - 1;

  // Populate the new slot
  if (!m_transport->ReplaceImageIndex(newIndex, filePath))
  {
    // Best effort rollback in the core
    m_transport->RemoveImageIndex(newIndex);
    return false;
  }

  RefreshDiscState();
  return true;
}

bool CGameClientDiscs::RemoveDisc(const std::string& filePath)
{
  if (filePath.empty())
    return true;

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return true;

  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  // Remove from the core by current index
  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(*discIndex)))
    return false;

  RefreshDiscState();
  return true;
}

bool CGameClientDiscs::InsertDisc(const std::string& filePath)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel

  if (!filePath.empty())
  {
    const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
    if (!discIndex.has_value())
      return true;

    imageIndex = static_cast<unsigned int>(*discIndex);
  }

  if (!m_transport->SetImageIndex(imageIndex))
    return false;

  RefreshDiscState();
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

    //! @todo Better handling of bad paths
    if (imagePath.empty())
      imagePath = StringUtils::Format("disc://{}", i);

    model.AddDisc(imagePath, imageLabel);
  }

  if (model.Empty())
    return;

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
