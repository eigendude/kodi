/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"
#include "guilib/listproviders/IListProvider.h"

#include <memory>
#include <string>
#include <vector>

class CGUIListItem;

namespace KODI
{
namespace GAME
{
class CDialogGameDiscManager;

/*!
 * \ingroup games
 */
class CDiscManagerMenu : public IListProvider
{
public:
  static constexpr int ACTION_SELECT_DISC = 108323;
  static constexpr int ACTION_EJECT_INSERT = 108324;
  static constexpr int ACTION_ADD_DISC = 108325;
  static constexpr int ACTION_REMOVE_DISC = 108326;
  static constexpr int ACTION_RESUME_GAME = 108327;
  static constexpr int ACTION_APPLY_DISC_CHANGE = 108328;

  explicit CDiscManagerMenu(GameClientPtr gameClient,
                            CDialogGameDiscManager& discManager,
                            int parentID);
  explicit CDiscManagerMenu(const CDiscManagerMenu& other) = default;
  ~CDiscManagerMenu() override = default;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;
  void OnReplace(IListProvider& previousProvider) override;

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

  // GUI parameters
  std::vector<std::shared_ptr<CGUIListItem>> m_items;

  // Game parameters
  bool m_ejected{false};
  std::string m_selectedDiscLabel;
  std::string m_ejectInsertLabel;
  std::string m_ejectInsertStatusLabel;
};
} // namespace GAME
} // namespace KODI
