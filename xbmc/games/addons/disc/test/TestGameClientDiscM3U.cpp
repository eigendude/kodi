/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto GAME_PATH = "/roms/my_game.m3u";

void CleanupStateFile()
{
  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  XFILE::CFile::Delete(m3uPath);

  std::string stateSubdirectory = URIUtils::GetDirectory(m3uPath);
  URIUtils::RemoveSlashAtEnd(stateSubdirectory);
  if (!stateSubdirectory.empty() && XFILE::CDirectory::Exists(stateSubdirectory))
    XFILE::CDirectory::Remove(stateSubdirectory);
}

void EnsureStateSubdirectory()
{
  const std::string m3uDirectory =
      URIUtils::GetDirectory(CGameClientDiscM3U::GetM3UPath(GAME_PATH));
  ASSERT_TRUE(XFILE::CDirectory::Create(m3uDirectory));
}

std::string ReadStateM3U()
{
  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  XFILE::CFile file;
  if (!file.Open(m3uPath))
    return "";

  std::string m3u;
  m3u.resize(static_cast<size_t>(file.GetLength()));
  if (!m3u.empty())
    file.Read(m3u.data(), m3u.size());

  file.Close();
  return m3u;
}

} // namespace

TEST(TestGameClientDiscM3U, SaveWritesM3UWithTwoDiscs)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.AddDisc("/roms/disc2.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc2.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, SaveOmitsRemovedSlotsFromM3U)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");
  savedModel.AddRemovedSlot();
  savedModel.AddDisc("/roms/disc3.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc3.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, SaveNormalizesBinToCueInM3UWhenCueExists)
{
  CleanupStateFile();

  const std::string tempDiscDirectory = "special://temp/test-disc-inputs";
  ASSERT_TRUE(XFILE::CDirectory::Create(tempDiscDirectory));
  const std::string binPath = URIUtils::AddFileToFolder(tempDiscDirectory, "disc1.bin");
  const std::string cuePath = URIUtils::ReplaceExtension(binPath, ".cue");

  XFILE::CFile cueFile;
  ASSERT_TRUE(cueFile.OpenForWrite(cuePath, true));
  cueFile.Close();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc(binPath);

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, cuePath + "\n");

  XFILE::CFile::Delete(cuePath);
  XFILE::CDirectory::Remove(tempDiscDirectory);
  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadRestoresDiscs)
{
  CleanupStateFile();

  EnsureStateSubdirectory();

  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(m3uPath, true));
  static constexpr char m3u[] = "/roms/disc1.chd\n/roms/disc2.chd\n";
  ASSERT_EQ(file.Write(m3u, sizeof(m3u) - 1), sizeof(m3u) - 1);
  file.Close();

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(GAME_PATH, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetPathByIndex(1), "/roms/disc2.chd");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadIgnoresEmptyAndCommentLines)
{
  CleanupStateFile();

  EnsureStateSubdirectory();

  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);
  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(m3uPath, true));
  static constexpr char m3u[] = "\n#EXTM3U\n/roms/disc1.chd\n   \n# comment\n/roms/disc2.chd\n";
  ASSERT_EQ(file.Write(m3u, sizeof(m3u) - 1), sizeof(m3u) - 1);
  file.Close();

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discM3U.Load(GAME_PATH, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 2U);
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetPathByIndex(1), "/roms/disc2.chd");

  CleanupStateFile();
}

TEST(TestGameClientDiscM3U, LoadMissingM3UIsNonErrorAndLeavesEmptyModel)
{
  CleanupStateFile();

  CGameClientDiscM3U discM3U;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discM3U.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscM3U, GetM3UPathUsesPerGameDirectoryAndExtensionlessBaseName)
{
  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);

  EXPECT_EQ(URIUtils::GetFileName(m3uPath), "my_game.m3u");
  EXPECT_EQ(URIUtils::GetExtension(m3uPath), ".m3u");
  EXPECT_EQ(m3uPath.find("my_game.m3u.m3u"), std::string::npos);

  std::string m3uDirectoryName = URIUtils::GetDirectory(m3uPath);
  URIUtils::RemoveSlashAtEnd(m3uDirectoryName);
  m3uDirectoryName = URIUtils::GetFileName(m3uDirectoryName);

  EXPECT_TRUE(StringUtils::StartsWith(m3uDirectoryName, "my_game.m3u_"));
}

TEST(TestGameClientDiscM3U, SaveCreatesPerGameStateFile)
{
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  savedModel.AddDisc("/roms/disc1.chd");

  CGameClientDiscM3U discM3U;
  ASSERT_TRUE(discM3U.Save(GAME_PATH, savedModel));

  EXPECT_TRUE(CFileUtils::Exists(CGameClientDiscM3U::GetM3UPath(GAME_PATH)));

  CleanupStateFile();
}
