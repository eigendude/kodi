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
#include <vector>

class CGUIListItem;

namespace KODI
{
namespace GAME
{
class CDiscManagerActions;

/*!
 * \ingroup games
 */
class CDiscManagerMenu : public IListProvider
{
public:
  explicit CDiscManagerMenu(GameClientPtr gameClient,
                            CDiscManagerActions& discActions,
                            int parentID);
  explicit CDiscManagerMenu(const CDiscManagerMenu& other) = default;
  ~CDiscManagerMenu() override = default;

  // Implementation of IListProvider
  std::unique_ptr<IListProvider> Clone() override;
  bool Update(bool forceRefresh) override;
  void Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items) override;
  bool OnClick(const std::shared_ptr<CGUIListItem>& item) override;
  void OnReplace(IListProvider& previousProvider) override;

private:
  // Dialog interface
  void UpdateItems();
  void UpdateEjectButton(bool ejected);

  // Construction parameters
  const GameClientPtr m_gameClient;
  CDiscManagerActions& m_discActions;

  // GUI parameters
  std::vector<std::shared_ptr<CGUIListItem>> m_items;

  // Game parameters
  bool m_ejected{false};
};
} // namespace GAME
} // namespace KODI
