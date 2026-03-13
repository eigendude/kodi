/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameDiscChanger.h"

#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"

using namespace KODI;
using namespace GAME;

CDialogGameDiscChanger::CDialogGameDiscChanger()
{
  // Initialize CGUIWindow via CGUIDialogProgress
  SetID(WINDOW_DIALOG_GAME_DISC_CHANGER);
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameDiscChanger::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SELECT_ITEM:
    case ACTION_MOUSE_LEFT_CLICK:
    case ACTION_MOUSE_RIGHT_CLICK:
    case ACTION_PARENT_DIR:
    case ACTION_PREVIOUS_MENU:
    case ACTION_NAV_BACK:
    {
      Close();
      return true;
    }
    default:
      break;
  }

  // Call ancestor
  return CGUIDialogProgress::OnAction(action);
}

void CDialogGameDiscChanger::OnInitWindow()
{
  // Call ancestor
  CGUIDialogProgress::OnInitWindow();

  //! @todo Start progress advancement
}

void CDialogGameDiscChanger::OnDeinitWindow(int nextWindowID)
{
  //! @todo Reset progress to zero

  // Call ancestor
  CGUIDialogProgress::OnDeinitWindow(nextWindowID);
}

void CDialogGameDiscChanger::FrameMove()
{
  // Call ancestor
  CGUIDialogProgress::FrameMove();

  //! @todo Update progress
}
