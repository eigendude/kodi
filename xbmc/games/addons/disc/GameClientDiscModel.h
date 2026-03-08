/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{

struct GameClientDiscEntry
{
  std::string path;
  std::string basename;
  std::string cachedLabel;
};

class CGameClientDiscModel
{
public:
  size_t Size() const;
  bool Empty() const;

  void Clear();

  bool AddDisc(const std::string& path, const std::string& cachedLabel = "");
  bool RemoveDiscByPath(const std::string& path);
  bool RemoveDiscByIndex(size_t index);

  const std::vector<GameClientDiscEntry>& GetDiscs() const;

  std::optional<size_t> GetDiscIndexByPath(const std::string& path) const;
  std::optional<size_t> GetDiscIndexByBasename(const std::string& basename) const;

  bool SetMainDiscByPath(const std::string& path);
  bool SetLastDiscByPath(const std::string& path);
  bool SetSelectedDiscByPath(const std::string& path);
  void SetSelectedNoDisc();

  std::string GetMainDiscPath() const;
  std::string GetLastDiscPath() const;
  bool IsSelectedNoDisc() const;
  std::string GetSelectedDiscPath() const;

  bool UpdateCachedLabel(const std::string& path, const std::string& label);

  std::string GetDisplayLabelByIndex(size_t index) const;
  std::vector<std::string> GetSelectorLabels() const;

private:
  static std::string DeriveBasename(const std::string& path);
  std::optional<std::string> GetReplacementPath(size_t removedIndex) const;

  std::vector<GameClientDiscEntry> m_discs;
  std::string m_mainDiscPath;
  std::string m_lastDiscPath;
  std::optional<std::string> m_selectedDiscPath;
};

} // namespace GAME
} // namespace KODI
