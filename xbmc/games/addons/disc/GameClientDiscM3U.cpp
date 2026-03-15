/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscM3U.h"

#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/StringUtils.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace GAME;

std::string CGameClientDiscM3U::GetM3UPath(const std::string& gamePath)
{
  return GetStateFilePath(gamePath, ".m3u");
}

bool CGameClientDiscM3U::Load(const std::string& gamePath, CGameClientDiscModel& model) const
{
  model.Clear();

  if (gamePath.empty())
    return true;

  const std::string m3uPath = GetM3UPath(gamePath);

  if (!CFileUtils::Exists(m3uPath))
  {
    CLog::Log(LOGDEBUG, "Disc state M3U {} does not exist, proceeding with empty disc model",
              CURL::GetRedacted(m3uPath));
    return true;
  }

  CLog::Log(LOGDEBUG, "Loading disc state M3U {}", CURL::GetRedacted(m3uPath));

  std::string m3u;
  if (!CFileUtils::LoadFile(m3uPath, m3u))
  {
    CLog::Log(LOGERROR, "Failed to read disc state M3U {}", CURL::GetRedacted(m3uPath));
    return false;
  }

  std::vector<std::string> lines = StringUtils::Split(m3u, '\n');
  for (std::string& line : lines)
  {
    StringUtils::Trim(line);

    if (line.empty() || StringUtils::StartsWith(line, "#"))
      continue;

    model.AddDisc(line);
  }

  return true;
}

bool CGameClientDiscM3U::Save(const std::string& gamePath, const CGameClientDiscModel& model)
{
  if (gamePath.empty())
    return true;

  const std::string stateDirectory = GetDiscStateDirectory();
  if (!XFILE::CDirectory::Exists(stateDirectory) && !XFILE::CDirectory::Create(stateDirectory))
  {
    CLog::Log(LOGERROR, "Failed to create disc state directory {}",
              CURL::GetRedacted(stateDirectory));
    return false;
  }

  const std::string m3uPath = GetM3UPath(gamePath);
  const std::string m3uDirectory = URIUtils::GetDirectory(m3uPath);
  const std::string m3u = BuildM3U(model);

  CLog::Log(LOGDEBUG, "Saving disc state M3U {}", CURL::GetRedacted(m3uPath));

  if (!XFILE::CDirectory::Exists(m3uDirectory) && !XFILE::CDirectory::Create(m3uDirectory))
  {
    CLog::Log(LOGERROR, "Failed to create disc state subdirectory {} for game {}",
              CURL::GetRedacted(m3uDirectory), CURL::GetRedacted(gamePath));
    return false;
  }

  XFILE::CFile file;
  if (!file.OpenForWrite(m3uPath, true))
  {
    CLog::Log(LOGERROR, "Failed to open disc state M3U {} for writing", CURL::GetRedacted(m3uPath));
    return false;
  }

  const ssize_t bytesWritten = file.Write(m3u.data(), m3u.size());
  file.Close();

  if (bytesWritten != static_cast<ssize_t>(m3u.size()))
  {
    CLog::Log(LOGERROR, "Failed to write disc state M3U {}, only {} of {} bytes written",
              CURL::GetRedacted(m3uPath), bytesWritten, m3u.size());
    return false;
  }

  return true;
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
