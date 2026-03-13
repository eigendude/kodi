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
#include "guilib/GUIListItem.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <assert.h>
#include <optional>
#include <string_view>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int MENU_ITEM_COUNT = 6;
constexpr std::string_view ACTION_ID_PROPERTY = "GameDiscManager.ActionId";
} // namespace

CDiscManagerMenu::CDiscManagerMenu(GameClientPtr gameClient,
                                   CDialogGameDiscManager& discManager,
                                   int parentID)
  : IListProvider(parentID), m_gameClient(std::move(gameClient)), m_discManager(discManager)
{
  assert(m_gameClient.get() != nullptr);

  m_ejected = m_gameClient->Discs().IsEjected();
}

std::unique_ptr<IListProvider> CDiscManagerMenu::Clone()
{
  return std::make_unique<CDiscManagerMenu>(*this);
}

bool CDiscManagerMenu::Update(bool forceRefresh)
{
  const bool isEjected = m_gameClient->Discs().IsEjected();

  if (forceRefresh || isEjected != m_ejected)
  {
    m_ejected = isEjected;
    UpdateItems();
    return true;
  }

  return false;
}

void CDiscManagerMenu::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items = m_items;
}

bool CDiscManagerMenu::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  if (!item)
    return false;

  const int actionId = static_cast<int>(item->GetProperty(ACTION_ID_PROPERTY).asInteger());

  switch (actionId)
  {
    case ACTION_SELECT_DISC:
      OnSelectDisc();
      return true;
    case ACTION_EJECT_INSERT:
      OnEjectInsert();
      return true;
    case ACTION_ADD_DISC:
      OnAdd();
      return true;
    case ACTION_REMOVE_DISC:
      OnRemove();
      return true;
    case ACTION_RESUME_GAME:
      OnResumeGame();
      return true;
    case ACTION_APPLY_DISC_CHANGE:
      // Skin handles apply behavior.
      return true;
    default:
      break;
  }

  return false;
}

void CDiscManagerMenu::OnReplace(IListProvider& previousProvider)
{
  m_items.clear();
  previousProvider.Fetch(m_items);

  if (m_items.size() != MENU_ITEM_COUNT)
    CLog::Log(LOGERROR, "Disc Manager menu has {} items. Expected {}.", m_items.size(),
              MENU_ITEM_COUNT);

  UpdateItems();
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
