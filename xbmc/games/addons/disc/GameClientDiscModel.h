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
  enum class DiscSlotType
  {
    Disc,
    RemovedSlot,
  };

  DiscSlotType slotType{DiscSlotType::Disc};
  std::string path;
  std::string basename;
  std::string cachedLabel;
};

class CGameClientDiscModel
{
public:
  // Selected disc state used by the frontend selector/workflow.
  // "No disc" is explicit and distinct from any real disc entry.
  enum class DiscSelectionType
  {
    Disc,
    NoDisc,
  };

  // Model invariants:
  // - m_mainDiscIndex, m_lastDiscIndex and m_selectedDiscIndex (when selection type is Disc)
  //   always refer to an existing entry in m_discs.
  // - For removal replacement (selected/last): prefer same index if still valid, otherwise
  //   previous valid index, otherwise no replacement.
  // - Main disc replacement is separate: if removed, use first remaining disc or clear.
  size_t Size() const;
  bool Empty() const;

  void Clear();
  void SetDiscs(const std::vector<GameClientDiscEntry>& discs);

  bool AddDisc(const std::string& path, const std::string& cachedLabel = "");
  bool AddRemovedSlot();
  bool RemoveDiscByPath(const std::string& path);
  bool RemoveDiscByIndex(size_t index);
  bool MarkRemovedByIndex(size_t index);

  const std::vector<GameClientDiscEntry>& GetDiscs() const { return m_discs; }

  std::optional<size_t> GetDiscIndexByPath(const std::string& path) const;
  std::optional<size_t> GetDiscIndexByBasename(const std::string& basename) const;

  bool SetMainDiscByPath(const std::string& path);
  bool SetMainDiscByIndex(size_t index);
  bool SetLastDiscByPath(const std::string& path);
  bool SetLastDiscByIndex(size_t index);
  bool SetSelectedDiscByPath(const std::string& path);
  bool SetSelectedDiscByIndex(size_t index);
  void SetSelectedNoDisc();
  bool IsSelectedNoDisc() const { return m_selectedType == DiscSelectionType::NoDisc; }
  bool HasSelectedDisc() const { return m_selectedType == DiscSelectionType::Disc; }
  std::optional<size_t> GetSelectedDiscIndex() const;
  bool IsEjected() const { return m_isEjected; }
  void SetEjected(bool ejected) { m_isEjected = ejected; }

  std::string GetSelectedDiscPath() const;
  std::string GetMainDiscPath() const;
  std::string GetLastDiscPath() const;

  bool UpdateCachedLabel(const std::string& path, const std::string& label);

  std::string GetPathByIndex(size_t index) const;
  std::string GetLabelByIndex(size_t index) const;

  bool IsRemovedSlotByIndex(size_t index) const;
  bool IsSelectableSlotByIndex(size_t index) const;
  bool IsRealDiscByIndex(size_t index) const;
  const GameClientDiscEntry* GetDiscByIndex(size_t index) const;

  static std::string DeriveBasename(const std::string& path);

private:
  std::optional<size_t> GetReplacementIndex(size_t removedIndex) const;

  std::vector<GameClientDiscEntry> m_discs;
  std::optional<size_t> m_mainDiscIndex;
  std::optional<size_t> m_lastDiscIndex;
  DiscSelectionType m_selectedType{DiscSelectionType::NoDisc};
  std::optional<size_t> m_selectedDiscIndex;
  bool m_isEjected{false};
};

} // namespace GAME
} // namespace KODI
