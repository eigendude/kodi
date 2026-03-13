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
constexpr int CONTROL_DISC_MANAGER_SCROLL_BAR = 108322;
} // namespace

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml")
{
}

bool CDialogGameDiscManager::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    const int actionId = message.GetParam1();
    if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
    {
      if (m_menu && m_menu->OnClick(message.GetSenderId()))
        return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CDialogGameDiscManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

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

  if (!m_gameClient)
    CLog::Log(LOGERROR,
              "No active game client. The Disc Manager dialog will not function correctly.");

  if (m_gameClient)
  {
    if (m_gameClient->SupportsDiscControl())
      m_gameClient->Discs().RefreshDiscState();
    else
      CLog::Log(LOGERROR, "Game client does not support disc control. The Disc Manager dialog will "
                          "not function correctly.");

    InitializeDialog();
    RefreshMenuControls();
  }

  ShowControl(CONTROL_DISC_MANAGER_MENU);

  CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CDiscManagerMenu::CONTROL_SELECT_DISC);
  OnMessage(msgSetFocus);
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  m_menu.reset();
  m_gameClient.reset();

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
      CGUIBaseContainer* discMgrDiscList = GetDiscList();
      if (discMgrDiscList != nullptr && discMgrDiscList->IsVisible())
      {
        ShowControl(CONTROL_DISC_MANAGER_MENU);
        return true;
      }
    }
    default:
      break;
  }

  return CGUIDialog::OnAction(action);
}

void CDialogGameDiscManager::SelectDiscToInsert(std::optional<size_t> selectedIndex,
                                                std::function<void(std::optional<size_t>)> callback)
{
  m_insertCallback = callback;
  ResetDiscList();

  const unsigned int selectedItemIndex = GetSelectedIndex(selectedIndex);

  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST,
                            static_cast<int64_t>(selectedItemIndex));
  OnMessage(msgSelectDisc);

  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);
}

void CDialogGameDiscManager::SelectDiscToRemove(std::function<void(size_t)> callback)
{
  m_removeCallback = callback;
  ResetDiscList();

  CGUIMessage msgSelectDisc(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_DISC_MANAGER_DISC_LIST, 0);
  OnMessage(msgSelectDisc);

  ShowControl(CONTROL_DISC_MANAGER_DISC_LIST);
}

void CDialogGameDiscManager::OnDiscSelect(size_t discIndex, bool isNoDisc)
{
  if (m_insertCallback)
    m_insertCallback(isNoDisc ? std::nullopt : std::optional<size_t>{discIndex});
  else if (m_removeCallback && !isNoDisc)
    m_removeCallback(discIndex);

  ShowControl(CONTROL_DISC_MANAGER_MENU);
}

bool CDialogGameDiscManager::AllowSelectNoDisc() const
{
  if (m_insertCallback)
    return true;

  return false;
}

void CDialogGameDiscManager::RefreshMenuControls()
{
  if (!m_menu)
    return;

  m_menu->Update();

  SetProperty("GameDiscManager.SelectedDisc", m_menu->GetSelectedDiscLabel());
  SetProperty("GameDiscManager.EjectInsertLabel", m_menu->GetEjectInsertLabel());
  SetProperty("GameDiscManager.EjectInsertStatus", m_menu->GetEjectInsertStatusLabel());

  if (m_menu->IsEjected())
    SetProperty("GameDiscManager.IsEjected", "true");
  else
    SetProperty("GameDiscManager.IsEjected", "false");
}

void CDialogGameDiscManager::InitializeDialog()
{
  m_menu = std::make_unique<CDiscManagerMenu>(m_gameClient, *this);

  CGUIBaseContainer* discMgrDiscList = GetDiscList();
  if (discMgrDiscList != nullptr)
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
    if (discList.IsRemovedSlotByIndex(i))
      continue;

    if (selectedIndex.has_value() && *selectedIndex == i)
      return itemIndex;

    ++itemIndex;
  }

  if (AllowSelectNoDisc())
    return itemIndex;

  return 0;
}

void CDialogGameDiscManager::ShowControl(int controlId)
{
  if (controlId == CONTROL_DISC_MANAGER_MENU)
  {
    CGUIMessage msgHideDiscList(GUI_MSG_HIDDEN, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgHideDiscList);

    CGUIMessage msgHideScrollBar(GUI_MSG_HIDDEN, GetID(), CONTROL_DISC_MANAGER_SCROLL_BAR);
    OnMessage(msgHideScrollBar);

    CGUIMessage msgShowMenu(GUI_MSG_VISIBLE, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgShowMenu);

    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CDiscManagerMenu::CONTROL_SELECT_DISC);
    OnMessage(msgSetFocus);

    m_insertCallback = {};
    m_removeCallback = {};
  }
  else if (controlId == CONTROL_DISC_MANAGER_DISC_LIST)
  {
    CGUIMessage msgHideMenu(GUI_MSG_HIDDEN, GetID(), CONTROL_DISC_MANAGER_MENU);
    OnMessage(msgHideMenu);

    CGUIMessage msgShowDiscList(GUI_MSG_VISIBLE, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgShowDiscList);

    CGUIMessage msgShowScrollBar(GUI_MSG_VISIBLE, GetID(), CONTROL_DISC_MANAGER_SCROLL_BAR);
    OnMessage(msgShowScrollBar);

    CGUIMessage msgSetFocus(GUI_MSG_SETFOCUS, GetID(), CONTROL_DISC_MANAGER_DISC_LIST);
    OnMessage(msgSetFocus);
  }
}

CGUIBaseContainer* CDialogGameDiscManager::GetDiscList()
{
  return dynamic_cast<CGUIBaseContainer*>(GetControl(CONTROL_DISC_MANAGER_DISC_LIST));
}
