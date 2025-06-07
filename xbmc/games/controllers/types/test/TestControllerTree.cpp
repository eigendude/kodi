/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/ports/types/PortNode.h"

#include <gtest/gtest.h>
#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

// Suite verifying that controller hubs can serialize to XML and deserialize
// back without changing their digest or structure. Each test exercises a
// different type of default controller.

// Test that a controller hub containing the keyboard controller can round-trip
// through XML without changing its digest or structure.

TEST(TestControllerTree, SerializeDeserialize)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load the default keyboard controller
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_KEYBOARD_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller, nullptr);
  ASSERT_TRUE(controller->LoadLayout());

  // Create a port accepting the keyboard controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode controllerNode;
  controllerNode.SetController(controller);
  port.SetCompatibleControllers({controllerNode});

  // Build controller hub
  CControllerHub hub;
  hub.SetPorts({port});

  // Digest before serialization
  std::string digestBefore = hub.GetDigest(UTILITY::CDigest::Type::SHA256);

  // Serialize hub to XML
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement* root = doc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*root));
  doc.InsertEndChild(root);

  // Deserialize hub
  CControllerHub hub2;
  ASSERT_TRUE(hub2.Deserialize(*root));

  // Verify digest and structure
  std::string digestAfter = hub2.GetDigest(UTILITY::CDigest::Type::SHA256);
  EXPECT_EQ(digestBefore, digestAfter);
  ASSERT_EQ(hub2.GetPorts().size(), 1U);
  const CPortNode& port2 = hub2.GetPorts()[0];
  EXPECT_EQ(port2.GetPortID(), "1");
  ASSERT_EQ(port2.GetCompatibleControllers().size(), 1U);
  EXPECT_EQ(port2.GetCompatibleControllers()[0].GetController()->ID(), controller->ID());
}

// Test that a controller hub containing the generic default controller can
// round-trip through XML without changing its digest or structure.

TEST(TestControllerTree, SerializeDeserializeDefaultController)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load the default game controller
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_CONTROLLER_ID, addon,
                                    ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller, nullptr);
  ASSERT_TRUE(controller->LoadLayout());

  // Create a port accepting the default controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode controllerNode;
  controllerNode.SetController(controller);
  port.SetCompatibleControllers({controllerNode});

  // Build controller hub
  CControllerHub hub;
  hub.SetPorts({port});

  // Digest before serialization
  std::string digestBefore = hub.GetDigest(UTILITY::CDigest::Type::SHA256);

  // Serialize hub to XML
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement* root = doc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*root));
  doc.InsertEndChild(root);

  // Deserialize hub
  CControllerHub hub2;
  ASSERT_TRUE(hub2.Deserialize(*root));

  // Verify digest and structure
  std::string digestAfter = hub2.GetDigest(UTILITY::CDigest::Type::SHA256);
  EXPECT_EQ(digestBefore, digestAfter);
  ASSERT_EQ(hub2.GetPorts().size(), 1U);
  const CPortNode& port2 = hub2.GetPorts()[0];
  EXPECT_EQ(port2.GetPortID(), "1");
  ASSERT_EQ(port2.GetCompatibleControllers().size(), 1U);
  EXPECT_EQ(port2.GetCompatibleControllers()[0].GetController()->ID(),
            controller->ID());
}

// Test that a controller hub containing the default mouse can round-trip through
// XML without changing its digest or structure.

TEST(TestControllerTree, SerializeDeserializeMouse)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load the default mouse controller
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_MOUSE_ID, addon,
                                    ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller, nullptr);
  ASSERT_TRUE(controller->LoadLayout());

  // Create a port accepting the mouse controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode controllerNode;
  controllerNode.SetController(controller);
  port.SetCompatibleControllers({controllerNode});

  // Build controller hub
  CControllerHub hub;
  hub.SetPorts({port});

  // Digest before serialization
  std::string digestBefore = hub.GetDigest(UTILITY::CDigest::Type::SHA256);

  // Serialize hub to XML
  tinyxml2::XMLDocument doc;
  tinyxml2::XMLElement* root = doc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*root));
  doc.InsertEndChild(root);

  // Deserialize hub
  CControllerHub hub2;
  ASSERT_TRUE(hub2.Deserialize(*root));

  // Verify digest and structure
  std::string digestAfter = hub2.GetDigest(UTILITY::CDigest::Type::SHA256);
  EXPECT_EQ(digestBefore, digestAfter);
  ASSERT_EQ(hub2.GetPorts().size(), 1U);
  const CPortNode& port2 = hub2.GetPorts()[0];
  EXPECT_EQ(port2.GetPortID(), "1");
  ASSERT_EQ(port2.GetCompatibleControllers().size(), 1U);
  EXPECT_EQ(port2.GetCompatibleControllers()[0].GetController()->ID(),
            controller->ID());
}
