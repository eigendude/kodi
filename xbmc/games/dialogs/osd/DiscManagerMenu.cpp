/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerMenu.h"

#include "FileItem.h"
#include "FileItemList.h"
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

#include <algorithm>
#include <array>
#include <optional>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int INDEX_SELECT_DISC = 0;
constexpr unsigned int INDEX_EJECT_INSERT = 1;
constexpr unsigned int INDEX_ADD_DISC = 2;
constexpr unsigned int INDEX_REMOVE_DISC = 3;
constexpr unsigned int INDEX_APPLY_DISC_CHANGE = 4;
constexpr unsigned int INDEX_RESUME_GAME = 5;

constexpr unsigned int MENU_ITEM_COUNT = 6;

constexpr auto PROPERTY_ACTION = "GameDiscManager.Action";
constexpr auto PROPERTY_SENSITIVE_ON_EJECT = "GameDiscManager.SensitiveOnEject";

constexpr auto ACTION_SELECT_DISC = "select_disc";
constexpr auto ACTION_EJECT_INSERT = "eject_insert";
constexpr auto ACTION_ADD_DISC = "add_disc";
constexpr auto ACTION_REMOVE_DISC = "remove_disc";
constexpr auto ACTION_APPLY_DISC_CHANGE = "apply_disc_change";
constexpr auto ACTION_RESUME_GAME = "resume_game";
} // namespace

CDiscManagerMenu::CDiscManagerMenu(GameClientPtr gameClient,
                                   CDialogGameDiscManager& discManager,
                                   int parentID)
  : IListProvider(parentID),
    m_gameClient(std::move(gameClient)),
    m_discManager(discManager)
{
  assert(m_gameClient.get() != nullptr);
}

std::unique_ptr<IListProvider> CDiscManagerMenu::Clone()
{
  return std::make_unique<CDiscManagerMenu>(*this);
}

bool CDiscManagerMenu::Update(bool forceRefresh)
{
  // Always update when eject state changes
  const bool isEjected = m_gameClient->Discs().IsEjected();

  if (isEjected != m_ejected)
  {
    m_ejected = isEjected;
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

  const std::string action = GetAction(item);

  if (action == ACTION_SELECT_DISC)
  {
    OnSelectDisc();
    return true;
  }

  if (action == ACTION_EJECT_INSERT)
  {
    OnEjectInsert();
    return true;
  }

  if (action == ACTION_ADD_DISC)
  {
    OnAdd();
    return true;
  }

  if (action == ACTION_REMOVE_DISC)
  {
    OnRemove();
    return true;
  }

  if (action == ACTION_APPLY_DISC_CHANGE)
  {
    OnApplyDiscChange();
    return true;
  }

  if (action == ACTION_RESUME_GAME)
  {
    OnResumeGame();
    return true;
  }

  return false;
}

void CDiscManagerMenu::OnReplace(IListProvider& previousProvider)
{
  m_items.clear();

  previousProvider.Fetch(m_items);

  // Inform the skin developer if we don't have a complete menu
  if (m_items.size() < MENU_ITEM_COUNT)
    CLog::Log(LOGERROR, "Disc Manager menu has only {} items. Expected {}.", m_items.size(),
              MENU_ITEM_COUNT);
  else if (m_items.size() > MENU_ITEM_COUNT)
    CLog::Log(LOGERROR, "Disc Manager menu has {} items. Expected {}. Extra items will be ignored.",
              m_items.size(), MENU_ITEM_COUNT);

  InitializeDefaultItemProperties();
  UpdateItems();
}

void CDiscManagerMenu::UpdateItems()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  while (m_items.size() < MENU_ITEM_COUNT)
    m_items.emplace_back(std::make_shared<CFileItem>());
  m_items.resize(MENU_ITEM_COUNT);

  InitializeDefaultItemProperties();

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  // Set inserted disc label
  std::string insertedDiscLabel;
  if (discList.IsSelectedNoDisc())
  {
    insertedDiscLabel = strings.Get(35274); // "No disc"
  }
  else
  {
    const auto selectedIndex = discList.GetSelectedDiscIndex();
    if (selectedIndex.has_value())
      insertedDiscLabel = discList.GetLabelByIndex(*selectedIndex);
  }
  m_items[INDEX_SELECT_DISC]->SetLabel2(insertedDiscLabel);

  // Set eject/insert item labels
  UpdateEjectButton(m_gameClient->Discs().IsEjected());
}

void CDiscManagerMenu::InitializeDefaultItemProperties()
{
  static constexpr std::array<const char*, MENU_ITEM_COUNT> defaultActions = {
      ACTION_SELECT_DISC, ACTION_EJECT_INSERT, ACTION_ADD_DISC,
      ACTION_REMOVE_DISC, ACTION_APPLY_DISC_CHANGE, ACTION_RESUME_GAME};

  static constexpr std::array<bool, MENU_ITEM_COUNT> defaultSensitiveOnEject = {
      true, false, true, true, false, false};

  const size_t itemCount = std::min(m_items.size(), static_cast<size_t>(MENU_ITEM_COUNT));
  for (size_t i = 0; i < itemCount; ++i)
  {
    const auto& item = m_items[i];
    if (!item)
      continue;

    if (item->GetProperty(PROPERTY_ACTION).empty())
      item->SetProperty(PROPERTY_ACTION, CVariant{defaultActions[i]});

    if (item->GetProperty(PROPERTY_SENSITIVE_ON_EJECT).empty())
      item->SetProperty(PROPERTY_SENSITIVE_ON_EJECT, CVariant{defaultSensitiveOnEject[i]});
  }
}

bool CDiscManagerMenu::IsEjectSensitive(const std::shared_ptr<CGUIListItem>& item)
{
  if (!item)
    return false;

  return item->GetProperty(PROPERTY_SENSITIVE_ON_EJECT).asBoolean();
}

std::string CDiscManagerMenu::GetAction(const std::shared_ptr<CGUIListItem>& item)
{
  if (!item)
    return {};

  std::string action = item->GetProperty(PROPERTY_ACTION).asString();

  if (!action.empty())
    return action;

  return {};
}

void CDiscManagerMenu::OnSelectDisc()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected() &&
      m_items.size() > INDEX_SELECT_DISC && IsEjectSensitive(m_items[INDEX_SELECT_DISC]))
    return;

  // Get currently-selected disc
  const CGameClientDiscModel& discList = discs.GetDiscs();
  const std::optional<size_t> selectedIndex = discList.GetSelectedDiscIndex();

  m_discManager.SelectDiscToInsert(selectedIndex,
                                   [this, selectedIndex](std::optional<size_t> discIndex)
                                   {
                                     // Do nothing if the selection didn't change
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
      // "Disc menu"
      // "The disc can't be inserted right now."
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(35272)},
                                           CVariant{strings.Get(35279)});
    }
    else
    {
      // "Disc menu"
      // "The disc can't be ejected right now."
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{strings.Get(35272)},
                                           CVariant{strings.Get(35278)});
    }
  }

  const bool isEjected = discs.IsEjected();

  UpdateEjectButton(isEjected);
}

void CDiscManagerMenu::OnAdd()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected() &&
      m_items.size() > INDEX_ADD_DISC && IsEjectSensitive(m_items[INDEX_ADD_DISC]))
    return;

  const CGameClientDiscModel& discModel = discs.GetDiscs();

  std::string startingPath = discModel.GetSelectedDiscPath();

  // Fall back to first valid disc
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

  // Fall back to currently playing game
  if (startingPath.empty())
    startingPath = m_gameClient->GetGamePath();

  std::string filePath;
  if (!BrowseForDiscImage(startingPath, filePath) || filePath.empty())
    return;

  if (!discs.AddDisc(filePath))
    ShowInternalError();

  UpdateItems();
}

void CDiscManagerMenu::OnRemove()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  // Do nothing if the disc isn't ejected
  if (!discs.IsEjected() &&
      m_items.size() > INDEX_REMOVE_DISC && IsEjectSensitive(m_items[INDEX_REMOVE_DISC]))
    return;

  m_discManager.SelectDiscToRemove(
      [this](size_t discIndex)
      {
        if (!m_gameClient->Discs().RemoveDiscByIndex(discIndex))
          ShowInternalError();

        UpdateItems();
      });
}

void CDiscManagerMenu::OnApplyDiscChange()
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                             "PlayerControl(Play,3)");
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
    m_items[INDEX_EJECT_INSERT]->SetLabel(strings.Get(35276)); // "Insert"
    m_items[INDEX_EJECT_INSERT]->SetLabel2(strings.Get(162)); // "Tray open"
  }
  else
  {
    m_items[INDEX_EJECT_INSERT]->SetLabel(strings.Get(35275)); // "Eject"
    m_items[INDEX_EJECT_INSERT]->SetLabel2("");
  }
}

void CDiscManagerMenu::ShowInternalError()
{
  auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

  // "Disc menu"
  // "The emulator \"{0:s}\" had an internal error."
  MESSAGING::HELPERS::ShowOKDialogText(
      CVariant{strings.Get(35272)},
      CVariant{StringUtils::Format(strings.Get(35213), m_gameClient->Name())});
}
