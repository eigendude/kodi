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

#include <optional>
#include <vector>

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
  CGameClientDiscModel coreModel;
  BuildModelFromCore(coreModel);
  MergeCoreModelIntoFrontend(coreModel);
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

  return RemoveDiscByIndex(*discIndex);
}

bool CGameClientDiscs::RemoveDiscByIndex(size_t index)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  // Remove from the core by current index
  if (!m_transport->RemoveImageIndex(static_cast<unsigned int>(index)))
    return false;

  m_discModel->MarkRemovedByIndex(index);
  RefreshDiscState();
  return true;
}

bool CGameClientDiscs::InsertDisc(const std::string& filePath)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (filePath.empty())
  {
    const unsigned int imageIndex = m_transport->GetImageCount(); // "No disc" sentinel
    if (!m_transport->SetImageIndex(imageIndex))
      return false;

    RefreshDiscState();
    return true;
  }

  const auto discIndex = m_discModel->GetDiscIndexByPath(filePath);
  if (!discIndex.has_value())
    return true;

  return InsertDiscByIndex(*discIndex);
}

bool CGameClientDiscs::InsertDiscByIndex(size_t index)
{
  // Libretro only allows mutating the image list while ejected
  if (!m_isEjected)
    return true;

  if (index >= m_discModel->Size())
    return true;

  if (!m_transport->SetImageIndex(static_cast<unsigned int>(index)))
    return false;

  RefreshDiscState();
  return true;
}

void CGameClientDiscs::BuildModelFromCore(CGameClientDiscModel& model) const
{
  model.Clear();

  const unsigned int imageCount = m_transport->GetImageCount();

  for (unsigned int i = 0; i < imageCount; ++i)
  {
    const std::string imagePath = m_transport->GetImagePath(i);
    const std::string imageLabel = m_transport->GetImageLabel(i);

    if (imagePath.empty())
      model.AddEmptySlot(imageLabel);
    else
      model.AddDisc(imagePath, imageLabel);
  }

  if (model.Empty())
    return;

  model.SetMainDiscByIndex(0);

  const unsigned int imageIndex = m_transport->GetImageIndex();

  if (imageIndex < imageCount)
  {
    model.SetLastDiscByIndex(imageIndex);
    model.SetSelectedDiscByIndex(imageIndex);
  }
  else
  {
    model.SetLastDiscByIndex(0);
    model.SetSelectedNoDisc();
  }
}

void CGameClientDiscs::MergeCoreModelIntoFrontend(const CGameClientDiscModel& coreModel)
{
  const std::vector<GameClientDiscEntry>& previousDiscs = m_discModel->GetDiscs();
  const std::vector<GameClientDiscEntry>& coreDiscs = coreModel.GetDiscs();

  std::vector<GameClientDiscEntry> mergedDiscs(
      previousDiscs.size(), {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""});

  std::vector<size_t> coreToMerged(coreDiscs.size());
  size_t corePosition = 0;

  for (size_t i = 0; i < previousDiscs.size(); ++i)
  {
    if (previousDiscs[i].slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      continue;

    if (corePosition >= coreDiscs.size())
      break;

    mergedDiscs[i] = coreDiscs[corePosition];
    coreToMerged[corePosition] = i;
    ++corePosition;
  }

  while (corePosition < coreDiscs.size())
  {
    coreToMerged[corePosition] = mergedDiscs.size();
    mergedDiscs.push_back(coreDiscs[corePosition]);
    ++corePosition;
  }

  m_discModel->SetDiscs(mergedDiscs);

  size_t firstSelectable = m_discModel->Size();
  for (size_t i = 0; i < m_discModel->Size(); ++i)
  {
    if (m_discModel->IsSelectableSlotByIndex(i))
    {
      firstSelectable = i;
      break;
    }
  }

  if (firstSelectable == m_discModel->Size())
    return;

  m_discModel->SetMainDiscByIndex(firstSelectable);

  const std::optional<size_t> selectedCoreIndex = coreModel.GetSelectedDiscIndex();
  if (selectedCoreIndex.has_value() && *selectedCoreIndex < coreToMerged.size())
  {
    const size_t selectedMergedIndex = coreToMerged[*selectedCoreIndex];
    m_discModel->SetLastDiscByIndex(selectedMergedIndex);
    m_discModel->SetSelectedDiscByIndex(selectedMergedIndex);
  }
  else
  {
    m_discModel->SetLastDiscByIndex(firstSelectable);
    m_discModel->SetSelectedNoDisc();
  }
}
