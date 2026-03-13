/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogProgress.h"
#include "games/GameTypes.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameDiscChanger : public CGUIDialogProgress
{
public:
  CDialogGameDiscChanger();
  ~CDialogGameDiscChanger() override = default;

  // Implementation of CGUIControl via CGUIDialogProgress
  bool OnAction(const CAction& action) override;

  // Implementation of CGUIWindow via CGUIDialogProgress
  void FrameMove() override;
  void OnDeinitWindow(int nextWindowID) override;

protected:
  // Implementation of CGUIWindow via CGUIDialogProgress
  void OnInitWindow() override;
};
} // namespace GAME
} // namespace KODI
