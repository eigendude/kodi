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
  enum class Action
  {
    SelectDisc,
    EjectInsert,
    Add,
    Remove,
    ApplyDiscChange,
    ResumeGame,
  };

  struct State
  {
    std::string selectedDiscLabel;
    std::string ejectInsertLabel;
    std::string ejectInsertStatus;
    bool isEjected{false};
    bool ejectSensitiveEnabled{false};
  };

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

  bool Execute(Action action);
  State GetState() const;

private:
  void UpdateItems();

  // Commands
  void OnSelectDisc();
  void OnEjectInsert();
  void OnAdd();
  void OnRemove();
  void OnApplyDiscChange();
  void OnResumeGame();

  // Helper functions
  bool BrowseForDiscImage(const std::string& startingPath, std::string& filePath);
  void ShowInternalError();

  // Construction parameters
  const GameClientPtr m_gameClient;
  CDialogGameDiscManager& m_discManager;

  // GUI parameters
  std::vector<std::shared_ptr<CGUIListItem>> m_items;

  // Game parameters
  bool m_ejected{false};
};
} // namespace GAME
} // namespace KODI
