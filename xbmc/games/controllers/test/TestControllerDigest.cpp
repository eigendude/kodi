/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

/*!
 * Verify digest of an empty controller hub matches the digest of empty data
 */
TEST(TestControllerDigest, EmptyHub)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  CControllerHub hub;

  // Digest from API
  const std::string hubDigest = hub.GetDigest(type);

  // Manual digest of empty input
  UTILITY::CDigest manual{type};
  const std::string expected = manual.FinalizeRaw();

  EXPECT_EQ(hubDigest, expected);
}

/*!
 * Verify digest calculation for a simple controller tree
 */
TEST(TestControllerDigest, SimpleTree)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  // Build controller
  ADDON::AddonInfoPtr addonInfo = std::make_shared<ADDON::CAddonInfo>(
      "game.controller.test", ADDON::AddonType::GAME_CONTROLLER);
  ControllerPtr controller = std::make_shared<CController>(addonInfo);

  // Controller node containing the controller and an empty hub
  CControllerNode node;
  node.SetController(controller);

  // Port accepting the controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node});

  // Hub with the single port
  CControllerHub hub;
  hub.SetPorts({port});

  // Digests from API
  const std::string nodeDigest = node.GetDigest(type);
  const std::string portDigest = port.GetDigest(type);
  const std::string hubDigest = hub.GetDigest(type);

  // Manually compute node digest
  UTILITY::CDigest emptyHub{type};
  std::string emptyHubDigest = emptyHub.FinalizeRaw();

  UTILITY::CDigest manualNode{type};
  manualNode.Update(controller->ID());
  manualNode.Update(emptyHubDigest);
  const std::string expectedNodeDigest = manualNode.FinalizeRaw();
  EXPECT_EQ(nodeDigest, expectedNodeDigest);

  // Manually compute port digest
  UTILITY::CDigest manualPort{type};
  manualPort.Update(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER));
  manualPort.Update("1");
  manualPort.Update(expectedNodeDigest);
  const std::string expectedPortDigest = manualPort.FinalizeRaw();
  EXPECT_EQ(portDigest, expectedPortDigest);

  // Manually compute hub digest
  UTILITY::CDigest manualHub{type};
  manualHub.Update(expectedPortDigest);
  const std::string expectedHubDigest = manualHub.FinalizeRaw();
  EXPECT_EQ(hubDigest, expectedHubDigest);
}

