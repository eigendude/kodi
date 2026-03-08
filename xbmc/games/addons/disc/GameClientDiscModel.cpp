/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscModel.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

size_t CGameClientDiscModel::Size() const
{
  return m_discs.size();
}

bool CGameClientDiscModel::Empty() const
{
  return m_discs.empty();
}

void CGameClientDiscModel::Clear()
{
  m_discs.clear();
  m_mainDiscPath.clear();
  m_lastDiscPath.clear();
  m_selectedType = DiscSelectionType::NoDisc;
  m_selectedDiscPath.clear();
}

bool CGameClientDiscModel::AddDisc(const std::string& path, const std::string& cachedLabel)
{
  if (path.empty() || GetDiscIndexByPath(path).has_value())
    return false;

  m_discs.push_back({path, DeriveBasename(path), cachedLabel});

  if (m_discs.size() == 1)
  {
    m_mainDiscPath = path;
    m_lastDiscPath = path;
    m_selectedType = DiscSelectionType::Disc;
    m_selectedDiscPath = path;
  }

  return true;
}

bool CGameClientDiscModel::RemoveDiscByPath(const std::string& path)
{
  const auto index = GetDiscIndexByPath(path);
  if (!index.has_value())
    return false;

  return RemoveDiscByIndex(*index);
}

bool CGameClientDiscModel::RemoveDiscByIndex(size_t index)
{
  if (index >= m_discs.size())
    return false;

  const std::string removedPath = m_discs[index].path;
  const bool wasMainDisc = (m_mainDiscPath == removedPath);
  const bool wasLastDisc = (m_lastDiscPath == removedPath);
  const bool wasSelectedDisc =
      (m_selectedType == DiscSelectionType::Disc && m_selectedDiscPath == removedPath);

  m_discs.erase(m_discs.begin() + index);

  if (wasMainDisc)
    m_mainDiscPath = m_discs.empty() ? "" : m_discs.front().path;

  if (wasLastDisc)
  {
    const auto replacement = GetReplacementPath(index);
    m_lastDiscPath = replacement.value_or("");
  }

  if (wasSelectedDisc)
  {
    const auto replacement = GetReplacementPath(index);
    if (replacement.has_value())
    {
      m_selectedType = DiscSelectionType::Disc;
      m_selectedDiscPath = *replacement;
    }
    else
    {
      m_selectedType = DiscSelectionType::NoDisc;
      m_selectedDiscPath.clear();
    }
  }

  return true;
}

const std::vector<GameClientDiscEntry>& CGameClientDiscModel::GetDiscs() const
{
  return m_discs;
}

std::optional<size_t> CGameClientDiscModel::GetDiscIndexByPath(const std::string& path) const
{
  const auto it =
      std::find_if(m_discs.begin(), m_discs.end(),
                   [&path](const GameClientDiscEntry& disc) { return disc.path == path; });
  if (it == m_discs.end())
    return std::nullopt;

  return static_cast<size_t>(it - m_discs.begin());
}

std::optional<size_t> CGameClientDiscModel::GetDiscIndexByBasename(
    const std::string& basename) const
{
  const auto it =
      std::find_if(m_discs.begin(), m_discs.end(), [&basename](const GameClientDiscEntry& disc)
                   { return disc.basename == basename; });
  if (it == m_discs.end())
    return std::nullopt;

  return static_cast<size_t>(it - m_discs.begin());
}

bool CGameClientDiscModel::SetMainDiscByPath(const std::string& path)
{
  if (!HasDiscPath(path))
    return false;

  m_mainDiscPath = path;
  return true;
}

bool CGameClientDiscModel::SetLastDiscByPath(const std::string& path)
{
  if (!HasDiscPath(path))
    return false;

  m_lastDiscPath = path;
  return true;
}

bool CGameClientDiscModel::SetSelectedDiscByPath(const std::string& path)
{
  if (!HasDiscPath(path))
    return false;

  m_selectedType = DiscSelectionType::Disc;
  m_selectedDiscPath = path;
  return true;
}

void CGameClientDiscModel::SetSelectedNoDisc()
{
  m_selectedType = DiscSelectionType::NoDisc;
  m_selectedDiscPath.clear();
}

bool CGameClientDiscModel::HasSelectedDisc() const
{
  return m_selectedType == DiscSelectionType::Disc;
}

std::string CGameClientDiscModel::GetMainDiscPath() const
{
  return m_mainDiscPath;
}

std::string CGameClientDiscModel::GetLastDiscPath() const
{
  return m_lastDiscPath;
}

bool CGameClientDiscModel::IsSelectedNoDisc() const
{
  return m_selectedType == DiscSelectionType::NoDisc;
}

std::string CGameClientDiscModel::GetSelectedDiscPath() const
{
  return HasSelectedDisc() ? m_selectedDiscPath : "";
}

bool CGameClientDiscModel::UpdateCachedLabel(const std::string& path, const std::string& label)
{
  const auto index = GetDiscIndexByPath(path);
  if (!index.has_value())
    return false;

  m_discs[*index].cachedLabel = label;
  return true;
}

std::string CGameClientDiscModel::GetPathByIndex(size_t index) const
{
  if (index >= m_discs.size())
    return "";

  return m_discs[index].path;
}

std::string CGameClientDiscModel::GetLabelByIndex(size_t index) const
{
  if (index >= m_discs.size())
    return "";

  const GameClientDiscEntry& disc = m_discs[index];

  if (!disc.cachedLabel.empty())
    return disc.cachedLabel;

  if (!disc.basename.empty())
    return disc.basename;

  const std::string basename = DeriveBasename(disc.path);
  if (!basename.empty())
    return basename;

  return disc.path;
}

std::string CGameClientDiscModel::DeriveBasename(const std::string& path)
{
  if (path.empty())
    return "";

  const size_t end = path.find_last_not_of("/\\");
  if (end == std::string::npos)
    return "";

  const size_t pos = path.find_last_of("/\\", end);
  if (pos == std::string::npos)
    return path.substr(0, end + 1);

  if (pos == end)
    return "";

  return path.substr(pos + 1, end - pos);
}

std::optional<std::string> CGameClientDiscModel::GetReplacementPath(size_t removedIndex) const
{
  if (m_discs.empty())
    return std::nullopt;

  if (removedIndex < m_discs.size())
    return m_discs[removedIndex].path;

  if (removedIndex > 0)
    return m_discs[removedIndex - 1].path;

  return std::nullopt;
}

bool CGameClientDiscModel::HasDiscPath(const std::string& path) const
{
  return GetDiscIndexByPath(path).has_value();
}
