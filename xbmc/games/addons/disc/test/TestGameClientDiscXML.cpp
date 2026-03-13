/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscM3U.h"
#include "games/addons/disc/GameClientDiscMergeUtils.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "games/addons/disc/GameClientDiscXML.h"
#include "utils/URIUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto GAME_PATH = "/roms/my_game.m3u";

void CleanupStateFile()
{
  XFILE::CFile::Delete(CGameClientDiscXML::GetXMLPath(GAME_PATH));
  XFILE::CFile::Delete(CGameClientDiscM3U::GetM3UPath(GAME_PATH));
}

std::string ReadStateXml()
{
  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  if (!file.Open(xmlPath))
    return "";

  std::string xml;
  xml.resize(static_cast<size_t>(file.GetLength()));
  if (!xml.empty())
    file.Read(xml.data(), xml.size());

  file.Close();
  return xml;
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

TEST(TestGameClientDiscXML, SaveLoadRoundtripPreservesSlotTypes)
{
  // Verify roundtripping keeps real and removed slots in their original order.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd", "Disc One"));
  ASSERT_TRUE(savedModel.AddRemovedSlot());
  ASSERT_TRUE(savedModel.AddRemovedSlot());

  ASSERT_TRUE(savedModel.SetSelectedDiscByIndex(0));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  ASSERT_EQ(loadedModel.Size(), 3U);
  EXPECT_TRUE(loadedModel.IsRealDiscByIndex(0));
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(1));
  EXPECT_TRUE(loadedModel.IsRemovedSlotByIndex(2));
  EXPECT_EQ(loadedModel.GetPathByIndex(0), "/roms/disc1.chd");
  EXPECT_EQ(loadedModel.GetLabelByIndex(0), "Disc One");

  ASSERT_TRUE(loadedModel.GetSelectedDiscIndex().has_value());
  EXPECT_EQ(*loadedModel.GetSelectedDiscIndex(), 0U);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveLoadSelectedNonePreserved)
{
  // Verify explicit "No disc" selection survives XML serialization and load.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetSelectedNoDisc();

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_TRUE(loadedModel.IsSelectedNoDisc());
  EXPECT_FALSE(loadedModel.GetSelectedDiscIndex().has_value());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, MissingXmlIsNonErrorAndLeavesEmptyModel)
{
  // Verify loading with no persisted XML is treated as success with an empty model.
  CleanupStateFile();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;

  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());
}

TEST(TestGameClientDiscXML, MalformedXmlFailsAndClearsModel)
{
  // Verify malformed XML is rejected and the output model is reset.
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char malformed[] = "<discstate><slots><slot type=\"disc\"></slots>";
  ASSERT_EQ(file.Write(malformed, sizeof(malformed) - 1), sizeof(malformed) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(loadedModel.AddDisc("/roms/placeholder.chd"));

  EXPECT_FALSE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.Empty());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, MergeAfterLoadPreservesRemovedTombstoneAgainstCoreRemoved)
{
  // Verify tombstone overlays preserve removed slot markers during merge.
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(loadedModel.AddRemovedSlot());
  ASSERT_TRUE(loadedModel.AddDisc("/roms/disc2.chd"));

  CGameClientDiscModel coreModel;
  ASSERT_TRUE(coreModel.AddRemovedSlot());
  ASSERT_TRUE(coreModel.AddDisc("/roms/disc2.chd"));

  const std::vector<GameClientDiscEntry> merged =
      OverlayRemovedTombstonesByIndex(loadedModel.GetDiscs(), coreModel.GetDiscs());

  ASSERT_EQ(merged.size(), 2U);
  EXPECT_EQ(merged[0].slotType, GameClientDiscEntry::DiscSlotType::RemovedSlot);
  EXPECT_EQ(merged[1].slotType, GameClientDiscEntry::DiscSlotType::Disc);
}

TEST(TestGameClientDiscXML, SaveWritesEjectedTrue)
{
  // Verify saving with an ejected tray writes the true tray flag.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"true\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveWritesEjectedFalse)
{
  // Verify saving with an inserted tray writes the false tray flag.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string xml = ReadStateXml();
  EXPECT_NE(xml.find("<tray ejected=\"false\""), std::string::npos);

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedState)
{
  // Verify loading restores a previously persisted ejected tray state.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(true);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_TRUE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadRestoresEjectedFalseState)
{
  // Verify loading restores a previously persisted non-ejected tray state.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  savedModel.SetEjected(false);

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));
  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, LoadMissingEjectedDefaultsToFalse)
{
  // Verify older XML without tray metadata defaults to non-ejected.
  CleanupStateFile();

  const std::string xmlPath = CGameClientDiscXML::GetXMLPath(GAME_PATH);

  XFILE::CFile file;
  ASSERT_TRUE(file.OpenForWrite(xmlPath, true));
  static constexpr char xml[] =
      "<discstate><slots><slot type=\"disc\" path=\"/roms/disc1.chd\"/></slots></discstate>";
  ASSERT_EQ(file.Write(xml, sizeof(xml) - 1), sizeof(xml) - 1);
  file.Close();

  CGameClientDiscXML discXml;
  CGameClientDiscModel loadedModel;
  ASSERT_TRUE(discXml.Load(GAME_PATH, loadedModel));

  EXPECT_FALSE(loadedModel.IsEjected());

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveWritesM3UWithTwoDiscs)
{
  // Verify save writes one M3U line per real disc in frontend model order.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc2.chd"));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc2.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveOmitsRemovedSlotsFromM3U)
{
  // Verify tombstoned slots are excluded from generated M3U output.
  CleanupStateFile();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc1.chd"));
  ASSERT_TRUE(savedModel.AddRemovedSlot());
  ASSERT_TRUE(savedModel.AddDisc("/roms/disc3.chd"));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, "/roms/disc1.chd\n/roms/disc3.chd\n");

  CleanupStateFile();
}

TEST(TestGameClientDiscXML, SaveNormalizesBinToCueInM3UWhenCueExists)
{
  // Verify .bin entries prefer sibling .cue paths when the cue file exists.
  CleanupStateFile();

  const std::string m3uPath = CGameClientDiscM3U::GetM3UPath(GAME_PATH);
  const std::string cuePath = URIUtils::ReplaceExtension(m3uPath, ".cue");
  const std::string binPath = URIUtils::ReplaceExtension(m3uPath, ".bin");

  XFILE::CFile cueFile;
  ASSERT_TRUE(cueFile.OpenForWrite(cuePath, true));
  cueFile.Close();

  CGameClientDiscModel savedModel;
  ASSERT_TRUE(savedModel.AddDisc(binPath));

  CGameClientDiscXML discXml;
  ASSERT_TRUE(discXml.Save(GAME_PATH, savedModel));

  const std::string m3u = ReadStateM3U();
  EXPECT_EQ(m3u, cuePath + "\n");

  XFILE::CFile::Delete(cuePath);
  CleanupStateFile();
}
