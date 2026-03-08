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
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameDiscManager::CDialogGameDiscManager()
  : CGUIDialog(WINDOW_DIALOG_GAME_DISC_MANAGER, "DialogGameControllers.xml")
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
}

void CDialogGameDiscManager::OnDeinitWindow(int nextWindowID)
{
  m_gameClient.reset();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}
