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
class CGameClientDiscTransport;

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
  void RefreshDiscState();
  const CGameClientDiscModel& GetDiscs() const { return *m_discModel; }
  bool IsEjected() const { return m_isEjected; }
  bool SetEjected(bool ejected);
  bool QueueAddDisc(const std::string& filePath);
  bool QueueRemoveDisc(const std::string& filePath);
  bool QueueInsertDisc(const std::string& filePath);

private:
  void PopulateModelFromCore(CGameClientDiscModel& model);

  // Add-on parameters
  std::unique_ptr<CGameClientDiscTransport> m_transport;

  // Game parameters
  std::unique_ptr<CGameClientDiscModel> m_discModel;
  std::atomic<bool> m_isEjected{false};
};
} // namespace GAME
} // namespace KODI
