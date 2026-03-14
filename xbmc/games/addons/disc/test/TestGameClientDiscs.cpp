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

TEST(TestGameClientDiscs, CompactingCoreCasePreservesTombstoneAndSelectedDiscPath)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(frontend.SetSelectedDiscByPath("/roms/disc2.chd"));
  ASSERT_TRUE(frontend.MarkRemovedByIndex(0));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(core.SetSelectedDiscByIndex(0));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(frontend.IsRealDiscByIndex(1));
  EXPECT_EQ(frontend.GetPathByIndex(1), "/roms/disc2.chd");
  ASSERT_TRUE(frontend.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*frontend.GetSelectedDiscIndex(), 1U);
}

TEST(TestGameClientDiscs, ZombieCoreCasePreservesTombstoneWhenCoreSlotIsNonDisc)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddRemovedSlot());
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(0));
  EXPECT_EQ(frontend.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, RealCoreDiscOverridesPreviousTombstone)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 1U);
  EXPECT_TRUE(frontend.IsRealDiscByIndex(0));
  EXPECT_EQ(frontend.GetPathByIndex(0), "/roms/disc1.chd");
}

TEST(TestGameClientDiscs, TrailingFrontendTombstonesSurviveCoreShrink)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddRemovedSlot());
  ASSERT_TRUE(frontend.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 3U);
  EXPECT_TRUE(frontend.IsRealDiscByIndex(0));
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(2));
}

TEST(TestGameClientDiscs, TrailingCoreRealDiscsAreAppended)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_EQ(frontend.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(frontend.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, SelectionFallsBackToCorePathWhenPreviousSelectionDisappears)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(frontend.SetSelectedDiscByPath("/roms/disc1.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(core.SetSelectedDiscByIndex(0));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_TRUE(frontend.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(frontend.GetSelectedDiscPath(), "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, SelectionFallsBackToNoDiscWhenNoCandidatePathExists)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.SetSelectedDiscByIndex(0));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());
  core.SetSelectedNoDisc();

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  EXPECT_TRUE(frontend.IsSelectedNoDisc());
  EXPECT_FALSE(frontend.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscs, ReconcileUsesCoreTrayState)
{
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  frontend.SetEjected(false);

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));
  core.SetEjected(true);

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  EXPECT_TRUE(frontend.IsEjected());
}
