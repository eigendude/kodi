/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/addons/GameClientSubsystem.h"

#include <atomic>
#include <memory>
#include <string>

struct AddonInstance_Game;
class CCriticalSection;

namespace KODI
{
namespace GAME
{

class CGameClient;
class CGameClientDiscModel;

/*!
 * \ingroup games
 */
class CGameClientDiscs : protected CGameClientSubsystem
{
public:
  CGameClientDiscs(CGameClient& gameClient,
                   AddonInstance_Game& addonStruct,
                   CCriticalSection& clientAccess);
  ~CGameClientDiscs() override;

  // Game client capabilities
  bool SupportsDiskControl() const;

  // Disc interface
  void Initialize();
  void RefreshDiscState(bool force = false);
  const CGameClientDiscModel& GetDiscs() const { return *m_discModel; }
  bool IsEjected() const { return m_isEjected; }
  bool QueueAddDisc(const std::string& filePath);
  bool QueueRemoveDisc(const std::string& filePath);
  bool QueueInsertDisc(const std::string& filePath);

  // Game client functions
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
  void PopulateModelFromCore(CGameClientDiscModel& model);
  bool SyncDiscs();
  bool BuildCoreDiscModel(CGameClientDiscModel& coreModel);

  // Game parameters
  std::unique_ptr<CGameClientDiscModel> m_discModel;
  std::atomic<bool> m_isEjected{false};
  bool m_hasPendingFrontendChanges{false};
};
} // namespace GAME
} // namespace KODI
