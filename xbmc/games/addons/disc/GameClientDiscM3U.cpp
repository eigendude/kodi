/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscM3U.h"

#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/Crc32.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto PROFILE_ROOT = "special://masterprofile";
constexpr auto DISC_STATE_DIRECTORY = "games/discstate";

std::string GetDiscStateDirectory()
{
  return URIUtils::AddFileToFolder(PROFILE_ROOT, DISC_STATE_DIRECTORY);
}
} // namespace

std::string CGameClientDiscM3U::GetM3UPath(const std::string& gamePath)
{
  const std::string fileName = StringUtils::Format("{:08x}.m3u", Crc32::Compute(gamePath));
  return URIUtils::AddFileToFolder(GetDiscStateDirectory(), fileName);
}

bool CGameClientDiscM3U::Save(const std::string& gamePath, const CGameClientDiscModel& model)
{
  if (gamePath.empty())
    return true;

  const std::string m3uPath = GetM3UPath(gamePath);
  const std::string m3u = BuildM3U(model);

  XFILE::CFile file;
  if (!file.OpenForWrite(m3uPath, true))
    return false;

  const ssize_t bytesWritten = file.Write(m3u.data(), m3u.size());
  file.Close();

  return bytesWritten == static_cast<ssize_t>(m3u.size());
}

std::string CGameClientDiscM3U::BuildM3U(const CGameClientDiscModel& model)
{
  std::string m3u;

  for (const GameClientDiscEntry& disc : model.GetDiscs())
    AppendDiscToM3U(m3u, disc);

  return m3u;
}

void CGameClientDiscM3U::AppendDiscToM3U(std::string& m3u, const GameClientDiscEntry& disc)
{
  if (disc.slotType != GameClientDiscEntry::DiscSlotType::Disc)
    return;

  if (disc.path.empty())
    return;

  m3u += NormalizeDiscPath(disc.path);
  m3u += '\n';
}

std::string CGameClientDiscM3U::NormalizeDiscPath(const std::string& discPath)
{
  if (!URIUtils::HasExtension(discPath, ".bin"))
    return discPath;

  const std::string cuePath = URIUtils::ReplaceExtension(discPath, ".cue");
  if (CFileUtils::Exists(cuePath))
    return cuePath;

  return discPath;
}
