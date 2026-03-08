/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscManager.h"

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
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr int CONTROL_DISC_MANAGER_MENU = 3;
constexpr int CONTROL_DISC_MANAGER_DISC_LIST = 108321;

constexpr auto PROPERTY_SHOW_MENU = "GameDiscManager.ShowMenu";
constexpr auto PROPERTY_SHOW_DISC_LIST = "GameDiscManager.ShowDiscList";
} // namespace

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml")
{
  // Initialize CGUIWindow
  m_loadType = KEEP_IN_MEMORY;
}

void CDialogGameDiscManager::OnInitWindow()
{
  // Call ancestor
  CGUIDialog::OnInitWindow();

  // Get active game add-on
  m_gameClient = GetGameClient();
  if (m_gameClient)
  {
    if (m_gameClient->SupportsDiscControl())
    {
      // Refresh discs from live core state
      m_gameClient->Discs().RefreshDiscState();
    }
    else
    {
      CLog::Log(LOGERROR, "Game client does not support disc control. The Disc Manager dialog will "
                          "not function correctly.");
    }
  }
  else
  {
    CLog::Log(LOGERROR,
              "No active game client. The Disc Manager dialog will not function correctly.");
  }

  // Initialize dialog
  InitializeDialog();

  // Show the main menu
  ShowControl(CONTROL_DISC_MANAGER_MENU);

  // Focus first "Select disc" item
  CGUIMessage msgSelectFirst(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_MENU, 0);
  OnMessage(msgSelectFirst);
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  // Reset game add-on
  m_gameClient.reset();

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
      if (GetProperty("GameDiscManager.ShowDiscList").asBoolean() && GetDiscList() != nullptr)
      {
        // Return to the main menu
        ShowControl(CONTROL_DISC_MANAGER_MENU);
        return true;
      }
      break;
    }
    default:
      break;
  }

  // Call ancestor
  return CGUIDialog::OnAction(action);
}

void CDialogGameDiscManager::SelectDiscToInsert(std::optional<size_t> selectedIndex,
                                                std::function<void(std::optional<size_t>)> callback)
{
  m_insertCallback = callback;

  // Reset the disc list
  ResetDiscList();

  // Find the item index to focus/select
  const unsigned int selectedItemIndex = GetSelectedIndex(selectedIndex);

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Select the current disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST,
                            static_cast<int64_t>(selectedItemIndex));
  OnMessage(msgSelectDisc);
}

void CDialogGameDiscManager::SelectDiscToRemove(std::function<void(size_t)> callback)
{
  m_removeCallback = callback;

  // Reset the disc list
  ResetDiscList();

  // Show the disc list
  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);

  // Select the first disc
  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST, 0);
  OnMessage(msgSelectDisc);
}

void CDialogGameDiscManager::OnDiscSelect(size_t discIndex, bool isNoDisc)
{
  if (m_insertCallback)
    m_insertCallback(isNoDisc ? std::nullopt : std::optional<size_t>{discIndex});
  else if (m_removeCallback && !isNoDisc)
    m_removeCallback(discIndex);

  // Return to the main menu
  ShowControl(CONTROL_DISC_MANAGER_MENU);
}

bool CDialogGameDiscManager::AllowSelectNoDisc() const
{
  if (m_insertCallback)
    return true;

  return false;
}

const CGameClientDiscModel& CDialogGameDiscManager::GetDiscModel() const
{
  if (m_gameClient)
    return m_gameClient->Discs().GetDiscs();

  static const CGameClientDiscModel empty;
  return empty;
}

void CDialogGameDiscManager::InitializeDialog()
{
  if (!m_gameClient)
    return;

  //
  // Initialize main menu
  //
  if (CGUIBaseContainer* discMgrMenu = GetMainMenu())
  {
    discMgrMenu->SetListProvider(
        std::make_unique<CDiscManagerMenu>(m_gameClient, *this, CONTROL_DISC_MANAGER_MENU));
  }
  else
  {
    CLog::Log(LOGERROR, "Missing main menu list with control ID {}", CONTROL_DISC_MANAGER_MENU);
  }

  //
  // Initialize disc selection list
  //
  if (CGUIBaseContainer* discMgrDiscList = GetDiscList())
  {
    discMgrDiscList->SetListProvider(std::make_unique<CDiscManagerDiscList>(
        m_gameClient, *this, CONTROL_DISC_MANAGER_DISC_LIST));
  }
  else
  {
    CLog::Log(LOGERROR, "Missing disc list with control ID {}", CONTROL_DISC_MANAGER_DISC_LIST);
  }
}

void CDialogGameDiscManager::ResetDiscList()
{
  CGUIBaseContainer* discMgrDiscList = GetDiscList();
  if (discMgrDiscList != nullptr)
  {
    discMgrDiscList->FreeResources(true);
    discMgrDiscList->AllocResources();
  }
}

unsigned int CDialogGameDiscManager::GetSelectedIndex(std::optional<size_t> selectedIndex)
{
  if (!m_gameClient)
    return 0;

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  unsigned int itemIndex = 0;

  for (size_t i = 0; i < discList.Size(); ++i)
  {
    // Hidden from the visible list, so it does not consume a UI row.
    if (discList.IsRemovedSlotByIndex(i))
      continue;

    if (selectedIndex.has_value() && *selectedIndex == i)
      return itemIndex;

    ++itemIndex;
  }

  // If no real slot matched, select the appended "No disc" row when present.
  if (AllowSelectNoDisc())
    return itemIndex;

  // Fallback for remove flow or invalid selection.
  return 0;
}

void CDialogGameDiscManager::ShowControl(int controlId)
{
  if (controlId == CONTROL_DISC_MANAGER_MENU)
  {
    SetProperty(PROPERTY_SHOW_MENU, true);
    SetProperty(PROPERTY_SHOW_DISC_LIST, false);

    // Give focus to main menu
    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgSetFocus);

    // If we're leaving the disc list, reset the callbacks
    m_insertCallback = {};
    m_removeCallback = {};
  }
  else if (controlId == CONTROL_DISC_MANAGER_DISC_LIST)
  {
    SetProperty(PROPERTY_SHOW_MENU, false);
    SetProperty(PROPERTY_SHOW_DISC_LIST, true);

    // Give focus to disc list
    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgSetFocus);
  }
}

GameClientPtr CDialogGameDiscManager::GetGameClient()
{
  auto gameSettingsHandle = CServiceBroker::GetGameRenderManager().RegisterGameSettingsDialog();
  if (gameSettingsHandle)
  {
    ADDON::AddonPtr addon;
    if (CServiceBroker::GetAddonMgr().GetAddon(gameSettingsHandle->GameClientID(), addon,
                                               ADDON::AddonType::GAMEDLL,
                                               ADDON::OnlyEnabled::CHOICE_YES))
    {
      return std::static_pointer_cast<CGameClient>(addon);
    }
  }

  return {};
}

CGUIBaseContainer* CDialogGameDiscManager::GetMainMenu()
{
  return dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_MENU));
}

CGUIBaseContainer* CDialogGameDiscManager::GetDiscList()
{
  return dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
}
