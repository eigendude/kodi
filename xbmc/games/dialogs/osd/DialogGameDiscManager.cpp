/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscManager.h"

#include "DiscManagerMenu.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/osd/DiscManagerDiscList.h"
#include "games/dialogs/osd/DiscManagerMenu.h"
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIMessageIDs.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_DISC_MANAGER_MENU = 3;
constexpr int CONTROL_DISC_MANAGER_DISC_LIST = 108321;
} // namespace

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml"),
    m_discs(std::make_unique<CFileItemList>())
{
}

void CDialogGameDiscManager::OnInitWindow()
{
  // Call ancestor
  CGUIDialog::OnInitWindow();

  // Get active game add-on
  GameClientPtr gameClient;
  {
    auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
    if (gameSettingsHandle)
    {
      ADDON::AddonPtr addon;
      if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                                 ADDON::AddonType::GAMEDLL,
                                                 ADDON::OnlyEnabled::CHOICE_YES))
        gameClient = std::static_pointer_cast<CGameClient>(addon);
    }
  }
  m_gameClient = std::move(gameClient);

  // Refresh discs from live core state
  if (m_gameClient && m_gameClient->SupportsDiscControl())
    m_gameClient->Discs().RefreshDiscState();

  // Initialize dialog
  CGUIBaseContainer* discMgrMenu =
      dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_MENU));
  if (discMgrMenu != nullptr)
  {
    discMgrMenu->SetListProvider(
        std::make_unique<CDiscManagerMenu>(m_gameClient, *this, CONTROL_DISC_MANAGER_MENU));
  }

  CGUIBaseContainer* discMgrDiscList =
      dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
  if (discMgrDiscList != nullptr)
  {
    discMgrDiscList->SetListProvider(std::make_unique<CDiscManagerDiscList>(
        m_gameClient, *this, CONTROL_DISC_MANAGER_DISC_LIST));
  }

  // Select "Insert disc" item
  CGUIMessage msgSelectFirst(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_MENU, 0);
  OnMessage(msgSelectFirst);
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  // Reset game add-on
  m_gameClient.reset();

  // Reset dialog
  m_discs->Clear();

  // Call ancestor
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CDialogGameDiscManager::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    {
      // Check if the disc list is visible
      CGUIBaseContainer* discMgrDiscList =
          dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
      if (discMgrDiscList != nullptr && discMgrDiscList->IsVisible())
      {
        // Return to the main menu
        ShowControl(CONTROL_DISC_MANAGER_MENU);
        return true;
      }
    }
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CDialogGameDiscManager::SelectDiscToInsert(const std::string& selectedPath,
                                                std::function<void(std::string)> callback)
{
  m_insertCallback = callback;

  // Reset the disc list
  CGUIBaseContainer* discMgrDiscList =
      dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
  if (discMgrDiscList != nullptr)
  {
    discMgrDiscList->FreeResources(true);
    discMgrDiscList->AllocResources();
  }

  // Find the item index to focus/select. Empty path means "No disc", which
  // is the last item in the insert list.
  size_t selectedIndex = 0;

  if (m_gameClient)
  {
    const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

    if (selectedPath.empty())
    {
      selectedIndex = static_cast<int>(discList.Size());
    }
    else
    {
      const auto discIndex = discList.GetDiscIndexByPath(selectedPath);
      if (discIndex.has_value())
        selectedIndex = static_cast<int>(*discIndex);
    }
  }

  // Select the current disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST,
                            static_cast<int64_t>(selectedIndex));
  OnMessage(msgSelectDisc);

  // Toggle visibilities
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);
}

void CDialogGameDiscManager::SelectDiscToRemove(std::function<void(std::string)> callback)
{
  m_removeCallback = callback;

  // Reset the disc list
  CGUIBaseContainer* discMgrDiscList =
      dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
  if (discMgrDiscList != nullptr)
  {
    discMgrDiscList->FreeResources(true);
    discMgrDiscList->AllocResources();
  }

  // Select the first disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST, 0);
  OnMessage(msgSelectDisc);

  // Toggle visibilities
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);
}

void CDialogGameDiscManager::OnDiscSelect(const std::string& filePath)
{
  if (m_insertCallback)
  {
    m_insertCallback(filePath);
    m_insertCallback = {};
  }
  else if (m_removeCallback)
  {
    m_removeCallback(filePath);
    m_removeCallback = {};
  }

  // Toggle visibilities
  ShowControl(CONTROL_DISC_MANAGER_MENU);
}

bool CDialogGameDiscManager::AllowSelectNoDisc() const
{
  if (m_insertCallback)
    return true;

  return false;
}

void CDialogGameDiscManager::ShowControl(int controlId)
{
  if (controlId == CONTROL_DISC_MANAGER_MENU)
  {
    // Hide old control
    CGUIMessage msgHideDiscList(GUI_MSG_HIDDEN, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgHideDiscList);

    // Show new control
    CGUIMessage msgShowMenu(GUI_MSG_VISIBLE, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgShowMenu);

    // Give focus to control
    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgSetFocus);
  }
  else if (controlId == CONTROL_DISC_MANAGER_DISC_LIST)
  {
    // Hide old control
    CGUIMessage msgHideMenu(GUI_MSG_HIDDEN, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgHideMenu);

    // Show new control
    CGUIMessage msgShowDiscList(GUI_MSG_VISIBLE, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgShowDiscList);

    // Give focus to control
    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgSetFocus);
  }
}
