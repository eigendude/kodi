/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/addons/disc/GameClientDiscModel.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientDiscModel, AddDiscAndEmptySlotSemantics)
{
  // Verify real discs and placeholder empty slots can coexist in insertion order.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddEmptySlot());
  ASSERT_TRUE(model.AddEmptySlot("Corrupt Disc 3"));

  ASSERT_EQ(model.Size(), 3U);
  EXPECT_FALSE(model.IsEmptySlotByIndex(0));
  EXPECT_TRUE(model.IsEmptySlotByIndex(1));
  EXPECT_TRUE(model.IsEmptySlotByIndex(2));
  EXPECT_EQ(model.GetPathByIndex(1), "");
  EXPECT_EQ(model.GetLabelByIndex(1), "");
  EXPECT_EQ(model.GetLabelByIndex(2), "Corrupt Disc 3");
}

TEST(TestGameClientDiscModel, DuplicateDiscPathsRejectedButMultipleEmptySlotsAllowed)
{
  // Verify path uniqueness applies only to real disc entries.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  EXPECT_FALSE(model.AddDisc("/roms/disc1.chd"));

  ASSERT_TRUE(model.AddEmptySlot());
  ASSERT_TRUE(model.AddEmptySlot());

  EXPECT_EQ(model.Size(), 3U);
}

TEST(TestGameClientDiscModel, SelectionDistinguishesDiscEmptySlotAndNoDisc)
{
  // Verify selected placeholder slot is distinct from explicit "no disc" state.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddEmptySlot());

  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));
  EXPECT_TRUE(model.HasSelectedDisc());
  EXPECT_FALSE(model.IsSelectedNoDisc());
  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 1U);
  EXPECT_EQ(model.GetSelectedDiscPath(), "");

  model.SetSelectedNoDisc();
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_FALSE(model.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscModel, RemoveByPathOnlyTargetsRealDiscs)
{
  // Verify placeholder slots are index-addressed and not removable by empty path.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddEmptySlot());

  EXPECT_FALSE(model.RemoveDiscByPath(""));
  EXPECT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));
  EXPECT_EQ(model.Size(), 1U);
  EXPECT_TRUE(model.IsEmptySlotByIndex(0));
}

TEST(TestGameClientDiscModel, LabelFallbackForRealDiscs)
{
  // Verify real disc labels keep basename/cached-label fallback behavior.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso"));
  EXPECT_EQ(model.GetLabelByIndex(0), "disc1.iso");

  ASSERT_TRUE(model.UpdateCachedLabel("/roms/game/disc1.iso", "Disc One"));
  EXPECT_EQ(model.GetLabelByIndex(0), "Disc One");
}

TEST(TestGameClientDiscModel, RemovalReplacementWithMixedSlots)
{
  // Verify replacement chooses neighboring index for selected/last with mixed slot types.
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddEmptySlot());
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.SetLastDiscByIndex(1));
  ASSERT_TRUE(model.SetSelectedDiscByIndex(1));

  ASSERT_TRUE(model.RemoveDiscByIndex(1));

  ASSERT_TRUE(model.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*model.GetSelectedDiscIndex(), 1U);
  EXPECT_EQ(model.GetLabelByIndex(1), "disc3.chd");
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc3.chd");
}
