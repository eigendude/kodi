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

bool CGameClientDiscs::QueueAddDisc(const std::string& filePath)
{
  if (filePath.empty())
    return false;

  if (m_discModel->GetDiscIndexByPath(filePath).has_value())
    return true;

  if (!m_isEjected)
    return false;

  if (!m_transport->AddImageIndex())
    return false;

  const unsigned int newIndex = m_transport->GetImageCount() - 1;

  if (!m_transport->ReplaceImageIndex(newIndex, filePath))
  {
    m_transport->RemoveImageIndex(newIndex);
    return false;
  }

  RefreshDiscState();
  return true;
}

bool CGameClientDiscs::QueueRemoveDisc(const std::string& filePath)
{
  if (filePath.empty())
    return false;

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return false;

  if (!m_isEjected)
    return false;

  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(*discIndex)))
    return false;

  RefreshDiscState();
  return true;
}

bool CGameClientDiscs::QueueInsertDisc(const std::string& filePath)
{
  if (!m_isEjected)
    return false;

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
