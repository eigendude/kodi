/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscs.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscs, PersistedModelSurvivesEmptyCoreMerge)
{
  CGameClientDiscModel persisted;
  ASSERT_TRUE(persisted.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(persisted.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));

  CGameClientDiscModel core;

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(persisted.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.discs.size(), 2U);
  EXPECT_EQ(merged.discs[0].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged.discs[0].path, "/roms/disc1.chd");
  EXPECT_EQ(merged.discs[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
  EXPECT_EQ(merged.discs[1].path, "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, HasUsableStartupDiscUsesPersistedSelectedDisc)
{
  CGameClientDiscModel persisted;
  ASSERT_TRUE(persisted.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(persisted.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));

  std::optional<size_t> selectedIndex;
  std::string startupPath;

  ASSERT_TRUE(HasUsableStartupDisc(persisted, selectedIndex, startupPath));
  ASSERT_TRUE(selectedIndex.has_value());
  EXPECT_EQ(*selectedIndex, 1U);
  EXPECT_EQ(startupPath, "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, HasUsableStartupDiscRejectsNonDiscSelection)
{
  CGameClientDiscModel persisted;
  ASSERT_TRUE(persisted.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(persisted.AddRemovedSlot());
  ASSERT_TRUE(persisted.SetSelectedDiscByIndex(1));

  std::optional<size_t> selectedIndex;
  std::string startupPath;

  EXPECT_FALSE(HasUsableStartupDisc(persisted, selectedIndex, startupPath));
}

TEST(TestGameClientDiscs, EmptyPersistedAndEmptyCoreRemainEmpty)
{
  CGameClientDiscModel persisted;
  CGameClientDiscModel core;

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(persisted.GetDiscs(), core.GetDiscs());

  EXPECT_TRUE(merged.discs.empty());
}

TEST(TestGameClientDiscs, PersistedAndCoreNonEmptyStillMerge)
{
  CGameClientDiscModel persisted;
  ASSERT_TRUE(persisted.AddRemovedSlot());
  ASSERT_TRUE(persisted.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  const MergedDiscSlots merged = MergeCoreSlotsByIndex(persisted.GetDiscs(), core.GetDiscs());

  ASSERT_EQ(merged.discs.size(), 2U);
  EXPECT_EQ(merged.discs[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged.discs[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
}
