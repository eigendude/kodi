/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameServices.h"

#include "controllers/Controller.h"
#include "controllers/ControllerManager.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameSettings.h"
#include "profiles/ProfileManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <filecoin/common/outcome.hpp>
#include <leveldb/options.h>
#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/multi/hash_type.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include <libp2p/multi/multihash.hpp>
#include <filecoin/storage/repository/impl/filesystem_repository.hpp>
#include <filecoin/crypto/blake2/blake2b160.hpp>
#include <memory>
#include <string>

using namespace KODI;
using namespace GAME;

/*!
 * \brief Get the CID of a raw chunk of data
 */
fc::outcome::result<fc::CID> getCidOf(gsl::span<const uint8_t> bytes)
{
  using fc::CID;
  using libp2p::common::Hash256;
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;

  Hash256 hash_raw = libp2p::crypto::sha256(bytes);
  fc::outcome::result<Multihash> hashResult = Multihash::create(HashType::sha256, hash_raw);

  if (hashResult)
    return CID(CID::Version::V1, MulticodecType::SHA2_256, hashResult.value());
  else
    return hashResult.error();
}

void RunFilecoinTest(const std::string& dataStorePath)
{
  using fc::CID;
  using fc::common::Buffer;
  using fc::outcome::result;
  using fc::storage::ipfs::IpfsDatastore;
  using fc::storage::repository::FileSystemRepository;
  using fc::storage::repository::Repository;

  // Ensure data store path exists exists
  if (!XFILE::CDirectory::Exists(dataStorePath))
  {
    CLog::Log(LOGDEBUG, "Creating data store path: {}", dataStorePath);
    XFILE::CDirectory::Create(dataStorePath);
  }

  // Database options
  leveldb::Options leveldbOptions{};
  leveldbOptions.create_if_missing = true;

  // TODO: API options
  std::string apiAddress;

  result<std::shared_ptr<Repository>> repoResult = FileSystemRepository::create(dataStorePath, apiAddress, leveldbOptions);
  if (!repoResult)
  {
    CLog::Log(LOGERROR, "Failed to open data store: {}", repoResult.error().message());
    return;
  }

  std::shared_ptr<Repository> repo = repoResult.value();
  result<unsigned int> versionResult = repo->getVersion();
  if (!versionResult)
  {
    CLog::Log(LOGERROR, "Can't get data store version: {}", versionResult.error().message());
    return;
  }

  CLog::Log(LOGDEBUG, "Opened data store at version: {}", versionResult.value());

  std::shared_ptr<IpfsDatastore> datastore = repo->getIpldStore();

  std::string message = "Hello world!";
  std::vector<uint8_t> buffer(message.begin(), message.end());

  result<CID> cidResult = getCidOf(buffer);
  if (!cidResult)
  {
    CLog::Log(LOGERROR, "Can't calculate CID of data: {}", cidResult.error().message());
    return;
  }

  CID cid = cidResult.value();
  Buffer value(buffer);

  auto setResult = datastore->set(cid, value);
  if (!setResult)
  {
    CLog::Log(LOGERROR, "Failed to set value in data store: {}", setResult.error().message());
    return;
  }

  CLog::Log(LOGDEBUG, "Successfully set value in data store");

  result<Buffer> getResult = datastore->get(cid);
  if (!getResult)
  {
    CLog::Log(LOGERROR, "Failed to get value from data store: {}", getResult.error().message());
    return;
  }

  const std::vector<uint8_t>& resultBuffer = getResult.value().toVector();
  std::string response = std::string(resultBuffer.data(),
                                     resultBuffer.data() + resultBuffer.size());

  CLog::Log(LOGDEBUG, "Successfully retrieved value from data store: {}", response);
}

CGameServices::CGameServices(CControllerManager &controllerManager,
                             RETRO:: CGUIGameRenderManager &renderManager,
                             PERIPHERALS::CPeripherals &peripheralManager,
                             const CProfileManager &profileManager) :
  m_controllerManager(controllerManager),
  m_gameRenderManager(renderManager),
  m_profileManager(profileManager),
  m_gameSettings(new CGameSettings())
{
  // Filecoin test
  const std::string userDataPath = CSpecialProtocol::TranslatePath(profileManager.GetUserDataFolder());
  const std::string dataStorePath = URIUtils::AddFileToFolder(userDataPath, "Datastore");
  RunFilecoinTest(dataStorePath);
}

CGameServices::~CGameServices() = default;

ControllerPtr CGameServices::GetController(const std::string& controllerId)
{
  return m_controllerManager.GetController(controllerId);
}

ControllerPtr CGameServices::GetDefaultController()
{
  return m_controllerManager.GetDefaultController();
}

ControllerPtr CGameServices::GetDefaultKeyboard()
{
  return m_controllerManager.GetDefaultKeyboard();
}

ControllerPtr CGameServices::GetDefaultMouse()
{
  return m_controllerManager.GetDefaultMouse();
}

ControllerVector CGameServices::GetControllers()
{
  return m_controllerManager.GetControllers();
}

std::string CGameServices::GetSavestatesFolder() const
{
  return m_profileManager.GetSavestatesFolder();
}
