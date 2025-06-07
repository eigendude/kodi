/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "games/agents/input/AgentInputMapXML.h"
#include "games/agents/input/AgentTopology.h"
#include "utils/XBMCTinyXML2.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestAgentTopology, UpdateDigest)
{
  //
  // Spec: Digest and creation timestamp are generated when updating an empty topology
  //

  CAgentTopology topology;
  EXPECT_TRUE(topology.GetDigest().empty());
  EXPECT_FALSE(topology.GetDigestCreationUTC().IsValid());

  topology.UpdateDigest();

  EXPECT_FALSE(topology.GetDigest().empty());
  EXPECT_TRUE(topology.GetDigestCreationUTC().IsValid());
}

TEST(TestAgentInputMapXML, SerializeDeserialize)
{
  //
  // Spec: Serializing and deserializing a topology preserves its ID, digest and game clients
  //

  std::shared_ptr<CAgentTopology> topology = std::make_shared<CAgentTopology>();
  topology->SetID(0);
  topology->AddGameClient("test.client");
  topology->UpdateDigest();

  CAgentInputMap::TopologyIDMap topologies;
  topologies[topology->GetID()] = topology;

  CXBMCTinyXML2 xmlDoc;
  ASSERT_TRUE(CAgentInputMapXML::SerializeTopologies(xmlDoc, topologies));

  tinyxml2::XMLPrinter printer;
  xmlDoc.Accept(&printer);
  std::string xml = printer.CStr();

  CXBMCTinyXML2 parsedDoc;
  ASSERT_TRUE(parsedDoc.Parse(xml));

  CAgentInputMap::TopologyIDMap loadedById;
  CAgentInputMap::TopologyDigestMap loadedByDigest;
  ASSERT_TRUE(CAgentInputMapXML::DeserializeTopologies(parsedDoc, loadedById, loadedByDigest));

  ASSERT_EQ(loadedById.size(), 1u);
  ASSERT_EQ(loadedByDigest.size(), 1u);

  const auto& loaded = *loadedById.begin()->second;
  EXPECT_EQ(loaded.GetID(), topology->GetID());
  EXPECT_EQ(loaded.GetDigest(), topology->GetDigest());
  EXPECT_EQ(loaded.GetGameClients().count("test.client"), 1u);
}

