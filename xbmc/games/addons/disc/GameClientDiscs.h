/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct AddonInstance_Game;

namespace KODI
{
namespace GAME
{

class CGameClient;

/*!
 * \ingroup games
 */
class CGameClientDiscs
{
public:
  CGameClientDiscs(CGameClient& gameClient, AddonInstance_Game& addonStruct);

  bool SupportsDiskControl() const;
  bool SupportsDiskControlLabels() const;
  bool SupportsInitialImage() const;

  bool GetEjectState(bool& ejected);
  bool SetEjectState(bool ejected);

  bool GetImageIndex(unsigned int& index);
  bool SetImageIndex(unsigned int index);
  bool GetNumImages(unsigned int& count);

  bool AddImageIndex();
  bool ReplaceImageIndex(unsigned int index, const char* path);
  bool RemoveImageIndex(unsigned int index);

  bool SetInitialImage(unsigned int index, const std::string& path);
  bool GetImagePath(unsigned int index, std::string& path);
  bool GetImageLabel(unsigned int index, std::string& label);

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
