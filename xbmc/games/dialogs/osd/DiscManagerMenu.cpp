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

#include <assert.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr unsigned int INDEX_SELECT_DISC = 0;
constexpr unsigned int INDEX_EJECT_INSERT = 1;
constexpr unsigned int INDEX_ADD_DISC = 2;
constexpr unsigned int INDEX_REMOVE_DISC = 3;
constexpr unsigned int INDEX_RESUME_GAME = 4;

constexpr unsigned int MENU_ITEM_COUNT = 5;
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
    OnSelectDisc();
    return true;
  }
  else if (item == m_items[INDEX_EJECT_INSERT])
  {
    OnEjectInsert();
    return true;
  }
  else if (item == m_items[INDEX_ADD_DISC])
  {
    OnAdd();
    return true;
  }
  else if (item == m_items[INDEX_REMOVE_DISC])
  {
    OnRemove();
    return true;
  }
  else if (item == m_items[INDEX_RESUME_GAME])
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
    insertedDiscLabel = "No disc";
  }
  else
  {
    const std::string selectedPath = discList.GetSelectedDiscPath();
    const auto selectedIndex = discList.GetDiscIndexByPath(selectedPath);
    if (selectedIndex.has_value())
      insertedDiscLabel = discList.GetLabelByIndex(*selectedIndex);
  }
  m_items[INDEX_SELECT_DISC]->SetLabel2(insertedDiscLabel);

  // Set eject/insert item labels
  if (m_gameClient->Discs().IsEjected())
  {
    m_items[INDEX_EJECT_INSERT]->SetLabel("Insert"); // "Insert"
    m_items[INDEX_EJECT_INSERT]->SetLabel2(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(162)); // "Tray open"
  }
  else
  {
    m_items[INDEX_EJECT_INSERT]->SetLabel("Eject"); // "Eject"
    m_items[INDEX_EJECT_INSERT]->SetLabel2("");
  }
}

void CDiscManagerMenu::OnSelectDisc()
{
  // Do nothing if the disc isn't ejected
  CGameClientDiscs& discs = m_gameClient->Discs();

  if (!discs.IsEjected())
    return;

  // Get currently-selected disc
  const CGameClientDiscModel& discList = discs.GetDiscs();
  const std::string selectedPath =
      discList.IsSelectedNoDisc() ? std::string{} : discList.GetSelectedDiscPath();

  m_discManager.SelectDiscToInsert(
      selectedPath,
      [this](const std::string& filePath)
      {
        if (!m_gameClient->Discs().QueueInsertDisc(filePath))
        {
          CLocalizeStrings& strings = CServiceBroker::GetResourcesComponent().GetLocalizeStrings();

          // "Disc swap"
          // "The emulator \"{0:s}\" had an internal error."
          MESSAGING::HELPERS::ShowOKDialogText(
              CVariant{"Disc swap"},
              CVariant{StringUtils::Format(strings.Get(35213), m_gameClient->Name())});
        }

        UpdateItems();
      });
}

void CDiscManagerMenu::OnEjectInsert()
{
  CGameClientDiscs& discs = m_gameClient->Discs();

  const bool wasEjected = discs.IsEjected();

  const bool success = discs.SetEjected(!wasEjected);

  if (!success)
  {
    // TODO: Replace with Kodi-localized notification/dialog text.
    if (wasEjected)
    {
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{"Disc swap"},
                                           CVariant{"The disc can't be inserted right now."});
    }
    else
    {
      MESSAGING::HELPERS::ShowOKDialogText(CVariant{"Disc swap"},
                                           CVariant{"The disc can't be ejected right now."});
    }
  }

  const bool isEjected = discs.IsEjected();

  if (isEjected)
  {
    m_items[INDEX_EJECT_INSERT]->SetLabel("Insert"); // "Insert"
    m_items[INDEX_EJECT_INSERT]->SetLabel2(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(162)); // "Tray open"
  }
  else
  {
    m_items[INDEX_EJECT_INSERT]->SetLabel("Eject"); // "Eject"
    m_items[INDEX_EJECT_INSERT]->SetLabel2("");
  }

  //! @todo Refresh the list control
}

void CDiscManagerMenu::OnAdd()
{
  std::string filePath;
  if (!BrowseForDiscImage(filePath))
    return;

  if (filePath.empty())
    return;

  //! @todo Skip adding if the disc is already in the list

  m_gameClient->Discs().QueueAddDisc(filePath);
}

void CDiscManagerMenu::OnRemove()
{
  m_discManager.SelectDiscToRemove(
      [this](const std::string& filePath)
      {
        if (!filePath.empty())
        {
          m_gameClient->Discs().QueueRemoveDisc(filePath);
        }
      });
}

void CDiscManagerMenu::OnResumeGame()
{
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                             "PlayerControl(Play)");
}

bool CDiscManagerMenu::BrowseForDiscImage(std::string& filePath)
{
  std::set<std::string> extensions = CGameUtils::GetGameExtensions();
  std::string strExtensions = StringUtils::Join(extensions, "|");

  return CGUIDialogFileBrowser::ShowAndGetFile("/Users/garrett/Desktop/ROMs/Frogger (U) [!].smc",
                                               strExtensions, "Select file", filePath);
}
