/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscManager.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddon.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameRenderManager.h"
#include "cores/RetroPlayer/guibridge/GUIGameSettingsHandle.h"
#include "games/addons/GameClient.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml"),
    m_items(std::make_unique<CFileItemList>())
{
}

void CDialogGameDiscManager::OnInitWindow()
{
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

  m_items->Clear();

  {
    CFileItemPtr item = std::make_shared<CFileItem>("Inserted disc");
    item->SetLabel2("No disc");
    item->SetArt("icon", "DefaultAlbumCover.png");
    m_items->Add(std::move(item));
  }

  {
    CFileItemPtr item = std::make_shared<CFileItem>("Eject");
    item->SetLabel2("Tray closed");
    item->SetArt("icon", "DefaultAddonsUpdates.png");
    m_items->Add(std::move(item));
  }

  {
    CFileItemPtr item = std::make_shared<CFileItem>("Add");
    item->SetArt("icon", "DefaultAddSource.png");
    m_items->Add(std::move(item));
  }

  {
    CFileItemPtr item = std::make_shared<CFileItem>("Remove");
    item->SetArt("icon", "DefaultAddonNone.png");
    m_items->Add(std::move(item));
  }

  {
    CFileItemPtr item = std::make_shared<CFileItem>("Resume game");
    item->SetArt("icon", "osd/fullscreen/buttons/play.png");
    m_items->Add(std::move(item));
  }

  CGUIMessage bindMessage(GUI_MSG_LABEL_BIND, GetID(), 3, 0, 0, m_items.get());
  OnMessage(bindMessage);

  SET_CONTROL_FOCUS(3, 0);

  CGUIMessage selectMessage(GUI_MSG_ITEM_SELECT, GetID(), 3, 0);
  OnMessage(selectMessage);
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  m_gameClient.reset();

  if (m_items)
    m_items->Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}
