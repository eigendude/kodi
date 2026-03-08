/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerMenu.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/osd/DiscManagerActions.h"
#include "guilib/GUIListItem.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <assert.h>

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
} // namespace

CDiscManagerMenu::CDiscManagerMenu(GameClientPtr gameClient,
                                   CDiscManagerActions& discActions,
                                   int parentID)
  : IListProvider(parentID),
    m_gameClient(std::move(gameClient)),
    m_discActions(discActions)
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

  if (isEjected != m_ejected || forceRefresh)
  {
    UpdateItems();
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
  // Ensure we have a full menu
  if (m_items.size() < MENU_ITEM_COUNT)
    return false;

  if (item == m_items[INDEX_SELECT_DISC])
  {
    m_discActions.OnSelectDisc();
    return true;
  }
  else if (item == m_items[INDEX_EJECT_INSERT])
  {
    m_discActions.OnEjectInsert();
    return true;
  }
  else if (item == m_items[INDEX_ADD_DISC])
  {
    m_discActions.OnAdd();
    return true;
  }
  else if (item == m_items[INDEX_REMOVE_DISC])
  {
    m_discActions.OnRemove();
    return true;
  }
  else if (item == m_items[INDEX_APPLY_DISC_CHANGE])
  {
    m_discActions.OnApplyDiscChange();
    return true;
  }
  else if (item == m_items[INDEX_RESUME_GAME])
  {
    m_discActions.OnResumeGame();
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
  {
    CLog::Log(LOGERROR, "Disc Manager menu has only {} items. Expected {}.", m_items.size(),
              MENU_ITEM_COUNT);
  }
  else if (m_items.size() > MENU_ITEM_COUNT)
  {
    CLog::Log(LOGINFO, "Disc Manager menu has {} items. Expected {}. Extra items will be ignored.",
              m_items.size(), MENU_ITEM_COUNT);
  }

  UpdateItems();
}

void CDiscManagerMenu::UpdateItems()
{
  while (m_items.size() < MENU_ITEM_COUNT)
    m_items.emplace_back(std::make_shared<CFileItem>());

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  // Set inserted disc label
  std::string insertedDiscLabel;
  if (discList.IsSelectedNoDisc())
  {
    auto& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();
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
