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

  bool GetEjectState();
  bool SetEjectState(bool ejected);

  unsigned int GetImageIndex();
  bool SetImageIndex(unsigned int imageIndex);
  unsigned int GetImageCount();

  bool AddImageIndex();
  bool ReplaceImageIndex(unsigned int imageIndex, const std::string& filePath);
  bool RemoveImageIndex(unsigned int imageIndex);

  bool SetInitialImage(unsigned int imageIndex, const std::string& filePath);
  std::string GetImagePath(unsigned int imageIndex);
  std::string GetImageLabel(unsigned int imageIndex);

private:
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
};
} // namespace GAME
} // namespace KODI
