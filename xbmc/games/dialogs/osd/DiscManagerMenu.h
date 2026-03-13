/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include <string>


namespace KODI
{
namespace GAME
{
class CDialogGameDiscManager;

/*!
 * \ingroup games
 */
class CDiscManagerMenu
{
public:
  static constexpr int CONTROL_SELECT_DISC = 108310;
  static constexpr int CONTROL_EJECT_INSERT = 108311;
  static constexpr int CONTROL_ADD_DISC = 108312;
  static constexpr int CONTROL_REMOVE_DISC = 108313;
  static constexpr int CONTROL_APPLY_DISC_CHANGE = 108314;
  static constexpr int CONTROL_RESUME_GAME = 108315;

  explicit CDiscManagerMenu(GameClientPtr gameClient, CDialogGameDiscManager& discManager);

  bool Update();
  bool OnClick(int controlId);

  std::string GetSelectedDiscLabel() const { return m_selectedDiscLabel; }
  std::string GetEjectInsertLabel() const { return m_ejectInsertLabel; }
  std::string GetEjectInsertStatusLabel() const { return m_ejectInsertStatusLabel; }
  bool IsEjected() const { return m_ejected; }

private:
  void UpdateItems();

  // Commands
  void OnSelectDisc();
  void OnEjectInsert();
  void OnAdd();
  void OnRemove();
  void OnResumeGame();

  // Helper functions
  bool BrowseForDiscImage(const std::string& startingPath, std::string& filePath);
  void UpdateEjectButton(bool ejected);
  void ShowInternalError();

  // Construction parameters
  const GameClientPtr m_gameClient;
  CDialogGameDiscManager& m_discManager;

  // Game parameters
  bool m_ejected{false};
  std::string m_selectedDiscLabel;
  std::string m_ejectInsertLabel;
  std::string m_ejectInsertStatusLabel;
};
} // namespace GAME
} // namespace KODI
