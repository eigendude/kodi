/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscManagerDiscList.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "games/GameUtils.h"
#include "games/addons/GameClient.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"
#include "games/dialogs/disc/DialogGameDiscManager.h"
#include "guilib/GUIListItem.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <assert.h>
#include <charconv>
#include <optional>
#include <regex>
#include <system_error>

namespace
{
struct DiscListDisplayRow
{
  size_t discIndex{0};
  std::string label;
  std::string path;
  bool selected{false};
  std::optional<std::string> normalizedStem;
  std::optional<int> trailingNumber;
};

std::optional<std::pair<std::string, int>> GetNormalizedStemAndTrailingNumber(
    const std::string& label)
{
  static const std::regex stemWithNumberRegex(R"(^(.+\S)\s+(\d+)$)");
  static const std::regex stemWithParenNumberRegex(R"(^(.+\S)\s+\((\d+)\)$)");

  const std::string trimmedLabel = StringUtils::Trim(label);

  std::smatch matches;
  if (!std::regex_match(trimmedLabel, matches, stemWithNumberRegex) &&
      !std::regex_match(trimmedLabel, matches, stemWithParenNumberRegex))
  {
    return std::nullopt;
  }

  std::string normalizedStem = StringUtils::Trim(matches[1].str());
  StringUtils::ToLower(normalizedStem);

  if (normalizedStem.empty())
    return std::nullopt;

  int trailingNumber = 0;
  const std::string numberString = matches[2].str();
  const auto [ptr, ec] = std::from_chars(numberString.data(),
                                         numberString.data() + numberString.size(), trailingNumber);
  if (ec != std::errc{} || ptr != numberString.data() + numberString.size())
    return std::nullopt;

  return std::make_pair(normalizedStem, trailingNumber);
}

bool CanSafelySortForDisplay(const std::vector<DiscListDisplayRow>& rows)
{
  if (rows.size() < 2)
    return false;

  std::optional<std::string> sharedStem;
  for (const DiscListDisplayRow& row : rows)
  {
    if (!row.normalizedStem.has_value() || !row.trailingNumber.has_value())
      return false;

    if (!sharedStem.has_value())
      sharedStem = row.normalizedStem;
    else if (*sharedStem != *row.normalizedStem)
      return false;
  }

  return true;
}
} // namespace

using namespace KODI;
using namespace GAME;

CDiscManagerDiscList::CDiscManagerDiscList(GameClientPtr gameClient,
                                           CDialogGameDiscManager& discManager,
                                           int parentID)
  : IListProvider(parentID),
    m_gameClient(std::move(gameClient)),
    m_discManager(discManager)
{
  assert(m_gameClient.get() != nullptr);
}

std::unique_ptr<IListProvider> CDiscManagerDiscList::Clone()
{
  return std::make_unique<CDiscManagerDiscList>(*this);
}

bool CDiscManagerDiscList::Update(bool forceRefresh)
{
  if (m_items.empty())
    UpdateItems();

  bool dirty{false};
  std::swap(dirty, m_dirty);
  return dirty;
}

void CDiscManagerDiscList::Fetch(std::vector<std::shared_ptr<CGUIListItem>>& items)
{
  items.clear();
  for (auto item : m_items)
    items.emplace_back(std::static_pointer_cast<CGUIListItem>(item));
}

void CDiscManagerDiscList::Reset()
{
  UpdateItems();
}

bool CDiscManagerDiscList::OnClick(const std::shared_ptr<CGUIListItem>& item)
{
  auto fileItem = std::dynamic_pointer_cast<CFileItem>(item);
  if (fileItem)
  {
    const size_t discIndex = static_cast<size_t>(fileItem->GetProperty("discIndex").asInteger());
    const bool isNoDisc = fileItem->GetProperty("isNoDisc").asBoolean();
    m_discManager.OnDiscSelect(discIndex, isNoDisc);
    return true;
  }

  return false;
}

void CDiscManagerDiscList::UpdateItems()
{
  m_items.clear();

  const CGameClientDiscModel& discList = m_gameClient->Discs().GetDiscs();

  const std::optional<size_t> selectedDiscIndex = discList.GetSelectedDiscIndex();

  std::vector<DiscListDisplayRow> rows;
  rows.reserve(discList.Size());

  for (size_t i = 0; i < discList.Size(); ++i)
  {
    // If disc has been removed from the list, this could be a zombie entry
    if (discList.IsRemovedSlotByIndex(i))
      continue;

    DiscListDisplayRow row;
    row.discIndex = i;
    row.path = discList.GetPathByIndex(i);
    row.label = discList.GetLabelByIndex(i);
    row.selected = selectedDiscIndex.has_value() && *selectedDiscIndex == i;

    if (const auto stemAndNumber = GetNormalizedStemAndTrailingNumber(row.label); stemAndNumber)
    {
      row.normalizedStem = std::move(stemAndNumber->first);
      row.trailingNumber = stemAndNumber->second;
    }

    rows.emplace_back(std::move(row));
  }

  if (CanSafelySortForDisplay(rows))
  {
    std::stable_sort(rows.begin(), rows.end(),
                     [](const DiscListDisplayRow& lhs, const DiscListDisplayRow& rhs)
                     { return *lhs.trailingNumber < *rhs.trailingNumber; });
  }

  for (const DiscListDisplayRow& row : rows)
  {
    auto item = std::make_shared<CFileItem>(row.label);
    item->SetPath(row.path);
    item->Select(row.selected);
    item->SetProperty(std::string{ITEM_PROPERTY_DISC_INDEX}, static_cast<int64_t>(row.discIndex));
    item->SetProperty(std::string{ITEM_PROPERTY_IS_NO_DISC}, false);

    // Avoid automatically selecting the item if its path matches the currently
    // playing game
    item->SetProperty("isplaying", false);

    m_items.emplace_back(std::move(item));
  }

  if (m_discManager.AllowSelectNoDisc() || m_items.empty())
  {
    auto noDiscItem = std::make_shared<CFileItem>("No disc");
    noDiscItem->SetPath("");
    noDiscItem->Select(discList.IsSelectedNoDisc());
    noDiscItem->SetProperty(std::string{ITEM_PROPERTY_DISC_INDEX},
                            static_cast<int64_t>(discList.Size()));
    noDiscItem->SetProperty(std::string{ITEM_PROPERTY_IS_NO_DISC}, true);

    // Avoid automatically selecting the item if its path matches the currently
    // playing game
    noDiscItem->SetProperty("isplaying", false);

    m_items.emplace_back(std::move(noDiscItem));
  }

  m_dirty = true;
}
