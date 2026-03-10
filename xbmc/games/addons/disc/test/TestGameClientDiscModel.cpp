/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscModel, AddDiscAndMarkRemovedKeepsSize)
{
  // Verify removing a disc marks a tombstone without shrinking the model.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.Size(), 2U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
}

TEST(TestGameClientDiscModel, RemovedSlotClearsPathAndLabel)
{
  // Verify removed slots no longer expose path/label information.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso", "Disc One"));
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_EQ(model.GetPathByIndex(0), "");
  EXPECT_EQ(model.GetLabelByIndex(0), "");
}

TEST(TestGameClientDiscModel, LookupIgnoresRemovedSlots)
{
  // Verify removed slots are ignored by path/basename lookup.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso"));
  ASSERT_TRUE(model.MarkRemovedByIndex(0));

  EXPECT_FALSE(model.GetDiscIndexByPath("/roms/game/disc1.iso").has_value());
  EXPECT_FALSE(model.GetDiscIndexByBasename("disc1.iso").has_value());
}

TEST(TestGameClientDiscModel, MultipleRemovedSlotsCanCoexist)
{
  // Verify multiple tombstones can coexist and preserve index slots.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.MarkRemovedByIndex(0));
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.Size(), 3U);
  EXPECT_TRUE(model.IsRemovedSlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(model.IsRemovedSlotByIndex(2));
}

TEST(TestGameClientDiscModel, ReplacementSkipsRemovedSlots)
{
  // Verify selected/last replacement skips removed tombstones.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.SetLastDiscByIndex(1));
  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, EmptyAndRemovedSlotsRemainDistinct)
{
  // Verify zombie empty slots are distinct from frontend removed slots.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddEmptySlot("Zombie Slot"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  EXPECT_TRUE(model.IsEmptySlotByIndex(0));
  EXPECT_FALSE(model.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(model.IsRemovedSlotByIndex(1));
  EXPECT_FALSE(model.IsEmptySlotByIndex(1));
  EXPECT_EQ(model.GetLabelByIndex(0), "Zombie Slot");
  EXPECT_EQ(model.GetLabelByIndex(1), "");
}

TEST(TestGameClientDiscModel, SelectionMainAndLastDoNotPointToRemoved)
{
  // Verify tracked indices are redirected away from removed slots.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.SetMainDiscByIndex(1));
  ASSERT_TRUE(model.SetLastDiscByIndex(1));
  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));

  ASSERT_TRUE(model.MarkRemovedByIndex(1));

  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 0U);
  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc1.chd");
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, MixedSlotModelBehavior)
{
  // Verify mixed Disc/EmptySlot/RemovedSlot behavior for labels and paths.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddEmptySlot("Core Empty"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd", "Disc 3"));
  ASSERT_TRUE(model.MarkRemovedByIndex(2));

  EXPECT_EQ(model.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.chd");
  EXPECT_EQ(model.GetPathByIndex(1), "");
  EXPECT_EQ(model.GetLabelByIndex(1), "Core Empty");
  EXPECT_EQ(model.GetPathByIndex(2), "");
  EXPECT_EQ(model.GetLabelByIndex(2), "");
}

TEST(TestGameClientDiscModel, MergePreservesRemovedTombstoneWhenCoreReportsEmpty)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", "Zombie"},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.discs.size(), 2U);
  EXPECT_EQ(merged.discs[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged.discs[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
}

TEST(TestGameClientDiscModel, MergePreservesGenuineEmptySlotWhenNoRemovedTombstone)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", "Core Empty"}};

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.discs.size(), 2U);
  EXPECT_EQ(merged.discs[1].slotType, GameClientDiscEntry::DiscSlotType::EmptySlot);
  EXPECT_EQ(merged.discs[1].cachedLabel, "Core Empty");
}

TEST(TestGameClientDiscModel, MergeDoesNotDuplicateRemovedTombstone)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", "Zombie"},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(previousDiscs, coreDiscs);

  size_t removedCount = 0;
  size_t discCount = 0;
  size_t emptyCount = 0;

  for (const auto& disc : merged.discs)
  {
    if (disc.slotType == GameClientDiscEntry::DiscSlotType::RemovedSlot)
      ++removedCount;
    else if (disc.slotType == GameClientDiscEntry::DiscSlotType::Disc)
      ++discCount;
    else
      ++emptyCount;
  }

  EXPECT_EQ(removedCount, 1U);
  EXPECT_EQ(discCount, 1U);
  EXPECT_EQ(emptyCount, 0U);
}

TEST(TestGameClientDiscModel, MergeSelectionMappingSkipsRemovedSlot)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", "Zombie"},
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc2.chd", "disc2.chd", ""}};

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(previousDiscs, coreDiscs);

  ASSERT_TRUE(merged.firstSelectable.has_value());
  EXPECT_EQ(*merged.firstSelectable, 1U);
  ASSERT_EQ(merged.coreToMerged.size(), 2U);
  EXPECT_FALSE(merged.coreToMerged[0].has_value());
  ASSERT_TRUE(merged.coreToMerged[1].has_value());
  EXPECT_EQ(*merged.coreToMerged[1], 1U);
}

TEST(TestGameClientDiscModel, MergePreservesTrailingRemovedSlotsWhenCoreShrinks)
{
  std::vector<GameClientDiscEntry> previousDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""},
      {GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""}};

  std::vector<GameClientDiscEntry> coreDiscs{
      {GameClientDiscEntry::DiscSlotType::Disc, "/roms/disc1.chd", "disc1.chd", ""}};

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(previousDiscs, coreDiscs);

  ASSERT_EQ(merged.discs.size(), 3U);
  EXPECT_EQ(merged.discs[0].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged.discs[1].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged.discs[2].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
}

TEST(TestGameClientDiscModel, ClearResetsEjectedState)
{
  CGameClientDiscModel model;

  model.SetEjected(true);
  ASSERT_TRUE(model.IsEjected());

  model.Clear();

  EXPECT_FALSE(model.IsEjected());
}
