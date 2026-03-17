/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/dialogs/disc/DiscManagerDiscSortUtils.h"

#include <gtest/gtest.h>

using namespace KODI::GAME;

namespace
{
void ExpectParsed(const std::string& label, const std::string& expectedStem, int expectedNumber)
{
  const auto parsed = GetNormalizedStemAndTrailingNumber(label);
  ASSERT_TRUE(parsed.has_value()) << label;
  EXPECT_EQ(parsed->first, expectedStem);
  EXPECT_EQ(parsed->second, expectedNumber);
}
} // namespace

TEST(TestDiscManagerDiscSortUtils, ParsesDiscInParentheses)
{
  // Verifies classic parenthesized "Disc N" labels extract a common normalized stem.
  ExpectParsed("Metal Gear Solid (Europe) (Disc 2)", "metal gear solid (europe)", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesDiscWithoutParentheses)
{
  // Verifies plain trailing "Disc N" labels are still recognized conservatively.
  ExpectParsed("Final Fantasy VII Disc 1", "final fantasy vii", 1);
}

TEST(TestDiscManagerDiscSortUtils, ParsesDiscWithTotal)
{
  // Verifies explicit total counts in "Disc N of M" labels preserve the disc ordinal.
  ExpectParsed("Final Fantasy VII Disc 2 of 3", "final fantasy vii", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesCdPattern)
{
  // Verifies compact CD markers like "CD2" are supported for display-only sorting.
  ExpectParsed("Driver CD2", "driver", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesBracketedDiscOfTotal)
{
  // Verifies bracketed compact forms such as [disc2of2] are parsed with metadata suffixes.
  ExpectParsed("Game Title [disc2of2][SLUS-00776]", "game title", 2);
}

TEST(TestDiscManagerDiscSortUtils, ParsesBracketedDisc)
{
  // Verifies bracketed forms with spacing like [Disc 1] are parsed consistently.
  ExpectParsed("Game Title [Disc 1]", "game title", 1);
}

TEST(TestDiscManagerDiscSortUtils, RejectsMixedAmbiguousLabels)
{
  // Verifies ambiguous/mixed labels remain unsortable by rejecting missing stems and differing series stems.
  const auto plainDisc = GetNormalizedStemAndTrailingNumber("Disc 1");
  const auto gameADisc = GetNormalizedStemAndTrailingNumber("Game A Disc 1");
  const auto gameBDisc = GetNormalizedStemAndTrailingNumber("Game B Disc 2");

  EXPECT_FALSE(plainDisc.has_value());
  ASSERT_TRUE(gameADisc.has_value());
  ASSERT_TRUE(gameBDisc.has_value());
  EXPECT_NE(gameADisc->first, gameBDisc->first);
}
