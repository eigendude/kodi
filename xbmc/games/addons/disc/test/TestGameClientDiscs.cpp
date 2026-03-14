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

TEST(TestGameClientDiscs, CompactingCoreRemovalPreservesFrontendTombstoneAndDiscPath)
{
  // Compacting core case: disc1 was removed, so the core now reports only disc2 at index 0.
  // Reconciliation must map disc2 back to its prior logical slot and preserve a tombstone at slot 0.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(frontend.IsRealDiscByIndex(1));
  EXPECT_EQ(frontend.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, ZombieCoreNonDiscSlotPreservesFrontendTombstone)
{
  // Zombie-slot core case: first slot remains non-disc after removal.
  // Existing frontend tombstone must survive and disc2 stays in slot 1.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddRemovedSlot());
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(0));
  EXPECT_TRUE(frontend.IsRealDiscByIndex(1));
  EXPECT_EQ(frontend.GetPathByIndex(1), "/roms/disc2.chd");
}

TEST(TestGameClientDiscs, RealCoreDiscOverridesFrontendTombstone)
{
  // Real core media must never be hidden by persisted tombstones.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddRemovedSlot());

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 1U);
  EXPECT_TRUE(frontend.IsRealDiscByIndex(0));
  EXPECT_EQ(frontend.GetPathByIndex(0), "/roms/disc1.chd");
}

TEST(TestGameClientDiscs, MiddleRemovalWithCompactionPreservesHole)
{
  // Middle removal with compacting core: core shifts disc3 left.
  // Reconciliation must preserve the removed middle slot from frontend history.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc3.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(core.AddDisc("/roms/disc3.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 3U);
  EXPECT_EQ(frontend.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(1));
  EXPECT_EQ(frontend.GetPathByIndex(2), "/roms/disc3.chd");
}

TEST(TestGameClientDiscs, TrailingFrontendTombstonesSurviveCoreShrink)
{
  // Frontend removal history beyond current core size must be retained.
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

TEST(TestGameClientDiscs, TrailingCoreRealDiscsAppendWhenCoreGrows)
{
  // New core discs with no prior frontend slot history should append.
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

TEST(TestGameClientDiscs, SelectionPreservesFrontendPathEvenWhenCoreCompacts)
{
  // Selection precedence #1: preserve prior frontend-selected disc by path.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(frontend.SetSelectedDiscByPath("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(core.SetSelectedDiscByIndex(0));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_TRUE(frontend.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(frontend.GetSelectedDiscPath(), "/roms/disc2.chd");
  EXPECT_EQ(*frontend.GetSelectedDiscIndex(), 1U);
}

TEST(TestGameClientDiscs, SelectionFallsBackToCoreSelectedPathWhenFrontendSelectionDisappears)
{
  // Selection precedence #2: use core-selected path if previous frontend-selected disc vanished.
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

TEST(TestGameClientDiscs, SelectionFallsBackToNoDiscWhenNoPathCandidateExists)
{
  // Selection precedence #3: explicit no-disc when neither previous nor core-selected path exists.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.SetSelectedDiscByPath("/roms/disc1.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddRemovedSlot());
  core.SetSelectedNoDisc();

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  EXPECT_TRUE(frontend.IsSelectedNoDisc());
  EXPECT_FALSE(frontend.GetSelectedDiscIndex().has_value());
}

TEST(TestGameClientDiscs, TrayStateAlwaysFollowsCore)
{
  // Ejected/inserted tray state is live core truth and should override persisted frontend state.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  frontend.SetEjected(false);

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));
  core.SetEjected(true);

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  EXPECT_TRUE(frontend.IsEjected());
}

TEST(TestGameClientDiscs, NoDuplicateDiscsIntroducedDuringReconciliation)
{
  // When frontend and core layouts differ, reconciliation should still produce unique real discs
  // and not duplicate paths while reshuffling slots.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/discA.chd"));
  ASSERT_TRUE(frontend.AddRemovedSlot());
  ASSERT_TRUE(frontend.AddDisc("/roms/discB.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/discB.chd"));
  ASSERT_TRUE(core.AddDisc("/roms/discA.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 3U);
  EXPECT_EQ(frontend.GetPathByIndex(0), "/roms/discA.chd");
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(1));
  EXPECT_EQ(frontend.GetPathByIndex(2), "/roms/discB.chd");
  EXPECT_FALSE(frontend.GetDiscIndexByPath("/roms/discMissing.chd").has_value());
}

TEST(TestGameClientDiscs, DisappearedFrontendRealDiscBecomesRemovedSlotToPreserveHistory)
{
  // Old unmatched frontend discs that are no longer in core should become tombstones, not vanish.
  CGameClientDiscModel frontend;
  ASSERT_TRUE(frontend.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(frontend.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel core;
  ASSERT_TRUE(core.AddDisc("/roms/disc1.chd"));

  CGameClientDiscMergeUtils::ReconcileModels(frontend, core);

  ASSERT_EQ(frontend.Size(), 2U);
  EXPECT_TRUE(frontend.IsRealDiscByIndex(0));
  EXPECT_TRUE(frontend.IsRemovedSlotByIndex(1));
}
