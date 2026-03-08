/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameDiscManager : public CGUIDialog
{
public:
  CDialogGameDiscManager();
  ~CDialogGameDiscManager() override = default;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Game parameters
  GameClientPtr m_gameClient;
};
} // namespace GAME
} // namespace KODI
