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

TEST(TestGameClientDiscModel, EmptyModelDefaults)
{
  CGameClientDiscModel model;

  EXPECT_TRUE(model.Empty());
  EXPECT_EQ(model.Size(), 0U);
  EXPECT_EQ(model.GetMainDiscPath(), "");
  EXPECT_EQ(model.GetLastDiscPath(), "");
  EXPECT_FALSE(model.HasSelectedDisc());
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_EQ(model.GetSelectedDiscPath(), "");

  const auto labels = model.GetSelectorLabels();
  ASSERT_EQ(labels.size(), 1U);
  EXPECT_EQ(labels[0], "No disc");
}

TEST(TestGameClientDiscModel, SelectionSemanticsDistinguishEmptyDiscAndExplicitNoDisc)
{
  CGameClientDiscModel model;

  EXPECT_TRUE(model.Empty());
  EXPECT_TRUE(model.IsSelectedNoDisc());

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.SetSelectedDiscByPath("/roms/disc2.chd"));

  EXPECT_FALSE(model.Empty());
  EXPECT_TRUE(model.HasSelectedDisc());
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc2.chd");

  model.SetSelectedNoDisc();

  EXPECT_FALSE(model.Empty());
  EXPECT_FALSE(model.HasSelectedDisc());
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_EQ(model.GetSelectedDiscPath(), "");
}

TEST(TestGameClientDiscModel, AddFirstDiscInitializesMainLastAndSelected)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));

  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc1.chd");
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc1.chd");
  EXPECT_TRUE(model.HasSelectedDisc());
  EXPECT_FALSE(model.IsSelectedNoDisc());
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, AddingSecondDiscPreservesMainDisc)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));

  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, CachedLabelOverridesDisplay)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.UpdateCachedLabel("/roms/disc1.chd", "Disc One"));

  EXPECT_EQ(model.GetDisplayLabelByIndex(0), "Disc One");
}

TEST(TestGameClientDiscModel, SelectorLabelsIncludeNoDiscAtEnd)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd", "Disc One"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd", "Disc Two"));

  const auto labels = model.GetSelectorLabels();
  ASSERT_EQ(labels.size(), 3U);
  EXPECT_EQ(labels[0], "Disc One");
  EXPECT_EQ(labels[1], "Disc Two");
  EXPECT_EQ(labels[2], "No disc");
}

TEST(TestGameClientDiscModel, SetMainLastSelectedByPath)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));

  EXPECT_TRUE(model.SetMainDiscByPath("/roms/disc2.chd"));
  EXPECT_TRUE(model.SetLastDiscByPath("/roms/disc2.chd"));
  EXPECT_TRUE(model.SetSelectedDiscByPath("/roms/disc2.chd"));

  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc2.chd");
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc2.chd");
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc2.chd");

  EXPECT_FALSE(model.SetMainDiscByPath("/roms/missing.chd"));
  EXPECT_FALSE(model.SetLastDiscByPath("/roms/missing.chd"));
  EXPECT_FALSE(model.SetSelectedDiscByPath("/roms/missing.chd"));
}

TEST(TestGameClientDiscModel, RemoveMiddleDiscCompactsOrder)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.RemoveDiscByIndex(1));

  ASSERT_EQ(model.Size(), 2U);
  EXPECT_EQ(model.GetDiscs()[0].path, "/roms/disc1.chd");
  EXPECT_EQ(model.GetDiscs()[1].path, "/roms/disc3.chd");
}

TEST(TestGameClientDiscModel, RemoveSelectedDiscUsesDocumentedReplacementRule)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.SetSelectedDiscByPath("/roms/disc2.chd"));
  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc2.chd"));
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc3.chd");

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc3.chd"));
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc1.chd");

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));
  EXPECT_FALSE(model.HasSelectedDisc());
  EXPECT_TRUE(model.IsSelectedNoDisc());
}

TEST(TestGameClientDiscModel, RemoveLastDiscUsesDocumentedReplacementRule)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));
  ASSERT_TRUE(model.SetLastDiscByPath("/roms/disc2.chd"));

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc2.chd"));
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc3.chd");

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc3.chd"));
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc1.chd");
}

TEST(TestGameClientDiscModel, RemoveMainDiscReassignsFirstRemainingDisc)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));

  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc2.chd");
}

TEST(TestGameClientDiscModel, RemoveFinalDiscClearsState)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));

  EXPECT_TRUE(model.Empty());
  EXPECT_EQ(model.GetMainDiscPath(), "");
  EXPECT_EQ(model.GetLastDiscPath(), "");
  EXPECT_FALSE(model.HasSelectedDisc());
  EXPECT_TRUE(model.IsSelectedNoDisc());
}

TEST(TestGameClientDiscModel, DuplicatePathAddIsRejected)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  EXPECT_FALSE(model.AddDisc("/roms/disc1.chd"));
  EXPECT_EQ(model.Size(), 1U);
}

TEST(TestGameClientDiscModel, BasenameLookupWorks)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso"));
  ASSERT_TRUE(model.AddDisc("/roms/game/disc2.iso"));

  const auto index = model.GetDiscIndexByBasename("disc2.iso");
  ASSERT_TRUE(index.has_value());
  EXPECT_EQ(*index, 1U);
  EXPECT_FALSE(model.GetDiscIndexByBasename("missing.iso").has_value());
}

TEST(TestGameClientDiscModel, DisplayFallbackWithoutCachedLabel)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/game/disc1.iso"));
  EXPECT_EQ(model.GetDisplayLabelByIndex(0), "disc1.iso");

  ASSERT_TRUE(model.AddDisc("disc2.iso"));
  EXPECT_EQ(model.GetDisplayLabelByIndex(1), "disc2.iso");
}

TEST(TestGameClientDiscModel, BasenameDerivationEdgeCases)
{
  CGameClientDiscModel model;

  EXPECT_FALSE(model.AddDisc(""));

  ASSERT_TRUE(model.AddDisc("plain.iso"));
  ASSERT_TRUE(model.AddDisc("/roms/game/unix.iso"));
  ASSERT_TRUE(model.AddDisc("C:\\roms\\game\\windows.iso"));
  ASSERT_TRUE(model.AddDisc("/roms/game/trailing/"));

  EXPECT_EQ(model.GetDiscs()[0].basename, "plain.iso");
  EXPECT_EQ(model.GetDiscs()[1].basename, "unix.iso");
  EXPECT_EQ(model.GetDiscs()[2].basename, "windows.iso");
  EXPECT_EQ(model.GetDiscs()[3].basename, "trailing");
}

TEST(TestGameClientDiscModel, RemovingUnrelatedDiscDoesNotChangeMain)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));
  ASSERT_TRUE(model.SetMainDiscByPath("/roms/disc2.chd"));

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));
  EXPECT_EQ(model.GetMainDiscPath(), "/roms/disc2.chd");
}

TEST(TestGameClientDiscModel, RemovingUnrelatedDiscDoesNotChangeLast)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));
  ASSERT_TRUE(model.SetLastDiscByPath("/roms/disc3.chd"));

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));
  EXPECT_EQ(model.GetLastDiscPath(), "/roms/disc3.chd");
}

TEST(TestGameClientDiscModel, RemovingUnrelatedDiscDoesNotChangeSelected)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd"));
  ASSERT_TRUE(model.AddDisc("/roms/disc3.chd"));
  ASSERT_TRUE(model.SetSelectedDiscByPath("/roms/disc3.chd"));

  ASSERT_TRUE(model.RemoveDiscByPath("/roms/disc1.chd"));
  EXPECT_EQ(model.GetSelectedDiscPath(), "/roms/disc3.chd");
}

TEST(TestGameClientDiscModel, ClearResetsAllState)
{
  CGameClientDiscModel model;

  ASSERT_TRUE(model.AddDisc("/roms/disc1.chd", "Disc One"));
  ASSERT_TRUE(model.AddDisc("/roms/disc2.chd", "Disc Two"));
  ASSERT_TRUE(model.SetMainDiscByPath("/roms/disc2.chd"));
  ASSERT_TRUE(model.SetLastDiscByPath("/roms/disc2.chd"));
  ASSERT_TRUE(model.SetSelectedDiscByPath("/roms/disc2.chd"));

  model.Clear();

  EXPECT_TRUE(model.Empty());
  EXPECT_EQ(model.Size(), 0U);
  EXPECT_EQ(model.GetMainDiscPath(), "");
  EXPECT_EQ(model.GetLastDiscPath(), "");
  EXPECT_FALSE(model.HasSelectedDisc());
  EXPECT_TRUE(model.IsSelectedNoDisc());
  EXPECT_EQ(model.GetSelectedDiscPath(), "");
}
