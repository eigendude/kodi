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
#include "games/agents/input/AgentInputMapXML.h"
#include "games/agents/input/AgentTopology.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/ports/types/PortNode.h"

#include <gtest/gtest.h>
#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;

// Suite ensuring agent input maps correctly serialize controller topologies and
// deserialize them back to the same digest and game client data for all
// supported default controllers.

// Ensure a topology containing a keyboard controller can be serialized
// and deserialized back with the same digest and game clients.

TEST(TestAgentInputMap, SerializeDeserialize)
{
  ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Load the default keyboard controller
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_KEYBOARD_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller, nullptr);
  ASSERT_TRUE(controller->LoadLayout());

  // Create a controller tree with one port accepting the keyboard
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode node;
  node.SetController(controller);
  port.SetCompatibleControllers({node});

  CControllerHub hub;
  hub.SetPorts({port});

  // Serialize hub to XML for topology deserialization
  tinyxml2::XMLDocument hubDoc;
  tinyxml2::XMLElement* hubRoot = hubDoc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*hubRoot));
  hubDoc.InsertEndChild(hubRoot);

  CAgentTopology topology;
  ASSERT_TRUE(topology.DeserializeControllerTree(hubDoc));
  topology.SetID(0);
  topology.AddGameClient("game.test");
  topology.UpdateDigest();

  // Create topology map
  CAgentInputMap::TopologyIDMap byId;
  CAgentInputMap::TopologyDigestMap byDigest;
  std::shared_ptr<CAgentTopology> topoPtr = std::make_shared<CAgentTopology>(topology);
  byId[topology.GetID()] = topoPtr;
  byDigest[topology.GetDigest()] = topoPtr;

  // Serialize map to XML
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(CAgentInputMapXML::SerializeTopologies(doc, byId));

  // Deserialize map
  CAgentInputMap::TopologyIDMap outById;
  CAgentInputMap::TopologyDigestMap outByDigest;
  ASSERT_TRUE(CAgentInputMapXML::DeserializeTopologies(doc, outById, outByDigest));

  // Verify round trip
  ASSERT_EQ(outById.size(), 1U);
  auto it = outById.find(0);
  ASSERT_NE(it, outById.end());
  EXPECT_EQ(it->second->GetDigest(), topology.GetDigest());
  EXPECT_EQ(it->second->GetGameClients().count("game.test"), 1U);
}

// Ensure a topology containing the generic default controller can be serialized
// and deserialized back with the same digest and game clients.

TEST(TestAgentInputMap, SerializeDeserializeDefaultController)
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

  // Create a controller tree with one port accepting the controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode node;
  node.SetController(controller);
  port.SetCompatibleControllers({node});

  CControllerHub hub;
  hub.SetPorts({port});

  // Serialize hub to XML for topology deserialization
  tinyxml2::XMLDocument hubDoc;
  tinyxml2::XMLElement* hubRoot = hubDoc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*hubRoot));
  hubDoc.InsertEndChild(hubRoot);

  CAgentTopology topology;
  ASSERT_TRUE(topology.DeserializeControllerTree(hubDoc));
  topology.SetID(0);
  topology.AddGameClient("game.test");
  topology.UpdateDigest();

  // Create topology map
  CAgentInputMap::TopologyIDMap byId;
  CAgentInputMap::TopologyDigestMap byDigest;
  std::shared_ptr<CAgentTopology> topoPtr = std::make_shared<CAgentTopology>(topology);
  byId[topology.GetID()] = topoPtr;
  byDigest[topology.GetDigest()] = topoPtr;

  // Serialize map to XML
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(CAgentInputMapXML::SerializeTopologies(doc, byId));

  // Deserialize map
  CAgentInputMap::TopologyIDMap outById;
  CAgentInputMap::TopologyDigestMap outByDigest;
  ASSERT_TRUE(CAgentInputMapXML::DeserializeTopologies(doc, outById, outByDigest));

  // Verify round trip
  ASSERT_EQ(outById.size(), 1U);
  auto it = outById.find(0);
  ASSERT_NE(it, outById.end());
  EXPECT_EQ(it->second->GetDigest(), topology.GetDigest());
  EXPECT_EQ(it->second->GetGameClients().count("game.test"), 1U);
}

// Ensure a topology containing the default mouse can be serialized and
// deserialized back with the same digest and game clients.

TEST(TestAgentInputMap, SerializeDeserializeMouse)
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

  // Create a controller tree with one port accepting the mouse
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  CControllerNode node;
  node.SetController(controller);
  port.SetCompatibleControllers({node});

  CControllerHub hub;
  hub.SetPorts({port});

  // Serialize hub to XML for topology deserialization
  tinyxml2::XMLDocument hubDoc;
  tinyxml2::XMLElement* hubRoot = hubDoc.NewElement("logicaltopology");
  ASSERT_TRUE(hub.Serialize(*hubRoot));
  hubDoc.InsertEndChild(hubRoot);

  CAgentTopology topology;
  ASSERT_TRUE(topology.DeserializeControllerTree(hubDoc));
  topology.SetID(0);
  topology.AddGameClient("game.test");
  topology.UpdateDigest();

  // Create topology map
  CAgentInputMap::TopologyIDMap byId;
  CAgentInputMap::TopologyDigestMap byDigest;
  std::shared_ptr<CAgentTopology> topoPtr = std::make_shared<CAgentTopology>(topology);
  byId[topology.GetID()] = topoPtr;
  byDigest[topology.GetDigest()] = topoPtr;

  // Serialize map to XML
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(CAgentInputMapXML::SerializeTopologies(doc, byId));

  // Deserialize map
  CAgentInputMap::TopologyIDMap outById;
  CAgentInputMap::TopologyDigestMap outByDigest;
  ASSERT_TRUE(CAgentInputMapXML::DeserializeTopologies(doc, outById, outByDigest));

  // Verify round trip
  ASSERT_EQ(outById.size(), 1U);
  auto it = outById.find(0);
  ASSERT_NE(it, outById.end());
  EXPECT_EQ(it->second->GetDigest(), topology.GetDigest());
  EXPECT_EQ(it->second->GetGameClients().count("game.test"), 1U);
}
