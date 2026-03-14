/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscs, OverlayPreservesRemovedTombstoneOverCoreZombie)
{
  // Case A: preserve frontend tombstone when the core slot is also non-disc.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 1U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscs, OverlayPreservesRemovedTombstoneAndAppendsCompactedCoreDisc)
{
  // Case B: keep slot-oriented tombstone history even if the core compacts.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].path, "/roms/disc1.chd");
}


TEST(TestGameClientDiscs, OverlayRemoveEarlierDiscKeepsLaterDiscSlotIdentity)
{
  // Case 1: Disc 2 stays at slot 1 when slot 0 is removed and the core compacts.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(previous.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].path, "/roms/disc2.chd");
}


TEST(TestGameClientDiscs, SelectionFallsBackToPreviousPathWhenCoreIndexHitsTombstone)
{
  // Case 1 selected-index behavior: selection remains attached to surviving Disc 2.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(previous.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(previous.SetSelectedDiscByIndex(1));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(core.SetSelectedDiscByIndex(0));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  CGameClientDiscModel refreshed;
  refreshed.SetDiscs(merged);

  bool selectedSet = false;
  const std::optional<size_t> coreSelectedIndex = core.GetSelectedDiscIndex();
  if (coreSelectedIndex.has_value())
    selectedSet = refreshed.SetSelectedDiscByIndex(*coreSelectedIndex);

  if (!selectedSet)
    selectedSet = refreshed.SetSelectedDiscByPath(previous.GetSelectedDiscPath());

  ASSERT_TRUE(selectedSet);
  ASSERT_TRUE(refreshed.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*refreshed.GetSelectedDiscIndex(), 1U);
}

TEST(TestGameClientDiscs, OverlayAfterTombstonedRemovalAndAddPreservesSurvivingDiscIdentity)
{
  // Case 3: adding after removal keeps surviving disc at its original slot.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddRemovedSlot());
  ASSERT_TRUE(previous.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(core.AddDisc("/roms/disc3.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 3U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].path, "/roms/disc2.chd");
  EXPECT_EQ(merged[2].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[2].path, "/roms/disc3.chd");
}

TEST(TestGameClientDiscs, OverlayUsesCoreZombieWhenPreviousHadRealDisc)
{
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddDisc("/roms/disc1.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 1U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscs, OverlayPreservesTrailingRemovedTombstones)
{
  // Case C: preserve trailing previous tombstones when the core shrinks.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(previous.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscs, OverlayAppendsTrailingCoreSlots)
{
  // Case D: append newly reported trailing core discs when the core grows.
  CGameClientDiscModel previous;
  ASSERT_TRUE(previous.AddDisc("/roms/disc1.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previous.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[1].path, "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, OverlayMixedOverlapPreservesTombstonesAndAppendsUnmatchedCoreSlots)
{
  // Case E: previous tombstones stay in place and compacted core slots are appended.
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""}};
  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""}};

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.size(), 4U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[2].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged[2].path, "/roms/disc1.chd");
  EXPECT_EQ(merged[3].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscs, RemovedIndicesMustBeAppliedDescendingToPreserveOriginalSlots)
{
  const std::vector<int> coreSlots{0, 1, 2, 3, 4};
  std::vector<size_t> removedIndices{1, 3};

  auto removeByIndices = [](std::vector<int> slots, const std::vector<size_t>& indices)
  {
    for (const size_t index : indices)
      slots.erase(slots.begin() + static_cast<std::ptrdiff_t>(index));

    return slots;
  };

  const std::vector<int> ascendingResult = removeByIndices(coreSlots, removedIndices);
  EXPECT_EQ(ascendingResult, (std::vector<int>{0, 2, 3}));

  std::sort(removedIndices.rbegin(), removedIndices.rend());
  const std::vector<int> descendingResult = removeByIndices(coreSlots, removedIndices);
  EXPECT_EQ(descendingResult, (std::vector<int>{0, 2, 4}));
}
