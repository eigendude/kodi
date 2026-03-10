/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GAME
{

class CGameClientDiscModel;

class CGameClientDiscXML
{
public:
  bool Load(const std::string& gamePath, CGameClientDiscModel& model) const;
  bool Save(const std::string& gamePath, const CGameClientDiscModel& model) const;

  static std::string GetXMLPath(const std::string& gamePath);
};

} // namespace GAME
} // namespace KODI
