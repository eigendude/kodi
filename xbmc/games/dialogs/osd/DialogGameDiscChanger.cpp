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

#include <algorithm>
#include <chrono>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto DISC_CHANGE_DURATION = std::chrono::seconds{2};
} // namespace

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

  SetPercentage(0);
  m_progressStartTime = std::chrono::steady_clock::now();
  m_isProgressRunning = true;
}

void CDialogGameDiscChanger::OnDeinitWindow(int nextWindowID)
{
  SetPercentage(0);
  m_progressStartTime = {};
  m_isProgressRunning = false;

  // Call ancestor
  CGUIDialogProgress::OnDeinitWindow(nextWindowID);
}

void CDialogGameDiscChanger::FrameMove()
{
  if (m_isProgressRunning)
  {
    const auto elapsed = std::chrono::steady_clock::now() - m_progressStartTime;
    const auto percent = std::clamp(
        static_cast<int>((100 * elapsed) / DISC_CHANGE_DURATION), 0, 100);

    SetPercentage(percent);

    if (percent >= 100)
    {
      m_isProgressRunning = false;
      Close();
    }
  }

  // Call ancestor
  CGUIDialogProgress::FrameMove();
}
