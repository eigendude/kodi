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

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class CGUIBaseContainer;

namespace KODI
{
namespace GAME
{
class CGameClientDiscModel;

/*!
 * \ingroup games
 *
 * \brief Dialog for managing multi-disc game media during gameplay.
 *
 * This dialog provides the in-game UI for disc-related operations such as
 * viewing the selected disc, ejecting or inserting the virtual tray, choosing
 * a different disc, adding or removing discs, and opening the disc-selection
 * submenu when needed.
 *
 * It coordinates between the GUI and the active game client’s disc model,
 * while keeping the main menu and disc-selection list in sync with the
 * emulator’s current disc state.
 */
class CDialogGameDiscManager : public CGUIDialog
{
public:
  CDialogGameDiscManager();
  ~CDialogGameDiscManager() override = default;

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;

  // Disc menu interface
  void SelectDiscToInsert(std::optional<size_t> selectedIndex,
                          std::function<void(std::optional<size_t>)> callback);
  void SelectDiscToRemove(std::function<void(size_t)> callback);
  void OnDiscSelect(size_t discIndex, bool isNoDisc);
  bool AllowSelectNoDisc() const;

  // Game interface
  const CGameClientDiscModel& GetDiscModel() const;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  // Helper functions
  void InitializeDialog();
  void ResetDiscList();
  unsigned int GetSelectedIndex(std::optional<size_t> selectedIndex);
  void ShowControl(int controlId);

  GameClientPtr GetGameClient();

  CGUIBaseContainer* GetMainMenu();
  CGUIBaseContainer* GetDiscList();

  // Game parameters
  GameClientPtr m_gameClient;

  // Dialog parameters
  std::function<void(std::optional<size_t>)> m_insertCallback;
  std::function<void(size_t)> m_removeCallback;
};
} // namespace GAME
} // namespace KODI
