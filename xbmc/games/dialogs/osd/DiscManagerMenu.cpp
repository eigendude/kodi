/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerMenu.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/osd/DialogGameDiscManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <optional>

using namespace KODI;
using namespace GAME;

CDiscManagerMenu::CDiscManagerMenu(GameClientPtr gameClient, CDialogGameDiscManager& discManager)
  : m_gameClient(std::move(gameClient)), m_discManager(discManager)
{
  m_ejected = m_gameClient->Discs().IsEjected();
  UpdateItems();
}

bool CDiscManagerMenu::Update()
{
  const bool isEjected = m_gameClient->Discs().IsEjected();

  if (isEjected != m_ejected)
  {
    m_ejected = isEjected;
    UpdateItems();
    return true;
  }

  return false;
}

bool CDiscManagerMenu::OnClick(int controlId)
{
  switch (controlId)
  {
    case CONTROL_SELECT_DISC:
      OnSelectDisc();
      return true;
    case CONTROL_EJECT_INSERT:
      OnEjectInsert();
      return true;
    case CONTROL_ADD_DISC:
      OnAdd();
      return true;
    case CONTROL_REMOVE_DISC:
      OnRemove();
      return true;
    case CONTROL_APPLY_DISC_CHANGE:
      // Skin handles apply behavior.
      return true;
    case CONTROL_RESUME_GAME:
      OnResumeGame();
      return true;
    default:
      break;
  }

  return false;
}

void CDiscManagerMenu::UpdateItems()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  m_selectedDiscLabel = strings.Get(35274); // "No disc"
  if (!discList.IsSelectedNoDisc())
  {
    const auto selectedIndex = discList.GetSelectedDiscIndex();
    if (selectedIndex.has_value())
      m_selectedDiscLabel = discList.GetLabelByIndex(*selectedIndex);
  }

  UpdateEjectButton(m_gameClient->Discs().IsEjected());
}

void CDiscManagerMenu::OnSelectDisc()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  if (!discs.IsEjected())
    return;

  const CGameClientDiscModel& discList = discs.GetDiscs();
  const std::optional<size_t> selectedIndex = discList.GetSelectedDiscIndex();

  m_discManager.SelectDiscToInsert(selectedIndex,
                                   [this, selectedIndex](std::optional<size_t> discIndex)
                                   {
                                     if (selectedIndex == discIndex)
                                       return;

                                     if (discIndex.has_value())
                                     {
                                       if (!m_gameClient->Discs().InsertDiscByIndex(*discIndex))
                                         ShowInternalError();
                                     }
                                     else if (!m_gameClient->Discs().InsertDisc(""))
                                     {
                                       ShowInternalError();
                                     }

                                     UpdateItems();
                                     m_discManager.RefreshMenuControls();
                                   });
}

void CDiscManagerMenu::OnEjectInsert()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  CGameClientDiscs& discs = m_gameClient->Discs();

  const bool wasEjected = discs.IsEjected();

  const bool success = discs.SetEjected(!wasEjected);

  if (!success)
  {
    if (wasEjected)
    {
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(35272)}, CVariant{strings.Get(35279)});
    }
    else
    {
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(35272)}, CVariant{strings.Get(35278)});
    }
  }

  const bool isEjected = discs.IsEjected();

  UpdateEjectButton(isEjected);
  m_discManager.RefreshMenuControls();
}

void CDiscManagerMenu::OnAdd()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  if (!discs.IsEjected())
    return;

  const CGameClientDiscModel& discModel = discs.GetDiscs();

  std::string startingPath = discModel.GetSelectedDiscPath();

  if (startingPath.empty())
  {
    for (const GameClientDiscEntry& disc : discModel.GetDiscs())
    {
      if (!disc.path.empty())
      {
        startingPath = disc.path;
        break;
      }
    }
  }

  if (startingPath.empty())
    startingPath = m_gameClient->GetGamePath();

  std::string filePath;
  if (!BrowseForDiscImage(startingPath, filePath) || filePath.empty())
    return;

  if (!discs.AddDisc(filePath))
    ShowInternalError();

  UpdateItems();
  m_discManager.RefreshMenuControls();
}

void CDiscManagerMenu::OnRemove()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  if (!discs.IsEjected())
    return;

  m_discManager.SelectDiscToRemove(
      [this](size_t discIndex)
      {
        if (!m_gameClient->Discs().RemoveDiscByIndex(discIndex))
          ShowInternalError();

        UpdateItems();
        m_discManager.RefreshMenuControls();
      });
}

void CDiscManagerMenu::OnResumeGame()
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                             "PlayerControl(Play)");
}

bool CDiscManagerMenu::BrowseForDiscImage(const std::string& startingPath, std::string& filePath)
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  const std::set<std::string> extensions = CGameUtils::GetGameExtensions();
  const std::string strExtensions = StringUtils::Join(extensions, "|");

  return CGUIDialogFileBrowser::ShowAndGetFile(startingPath, strExtensions,
                                               strings.Get(35280), // "Select disc"
                                               filePath);
}

void CDiscManagerMenu::UpdateEjectButton(bool ejected)
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  if (ejected)
  {
    m_ejectInsertLabel = strings.Get(35276); // "Insert"
    m_ejectInsertStatusLabel = strings.Get(162); // "Tray open"
  }
  else
  {
    m_ejectInsertLabel = strings.Get(35275); // "Eject"
    m_ejectInsertStatusLabel.clear();
  }
}

void CDiscManagerMenu::ShowInternalError()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  MESSAGING::HELPERS::ShowOKDialogText(
      CVariant{strings.Get(35272)},
      CVariant{StringUtils::Format(strings.Get(35213), m_gameClient->Name())});
}
