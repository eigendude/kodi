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

#include <functional>
#include <memory>
#include <string>

class CFileItemList;

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

  // Implementation of CGUIControl via CGUIDialog
  bool OnAction(const CAction& action) override;

  // Disc menu interface
  void SelectDiscToInsert(const std::string& selectedPath,
                          std::function<void(std::string)> callback);
  void SelectDiscToRemove(std::function<void(std::string)> callback);
  void OnDiscSelect(const std::string& filePath);
  bool AllowSelectNoDisc() const;

protected:
  // Implementation of CGUIWindow via CGUIDialog
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;

private:
  void ShowControl(int controlId);

  // Game parameters
  GameClientPtr m_gameClient;

  // Dialog parameters
  std::unique_ptr<CFileItemList> m_discs;
  std::function<void(std::string)> m_insertCallback;
  std::function<void(std::string)> m_removeCallback;
};
} // namespace GAME
} // namespace KODI
