/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientDiscXML.h"

#include "URL.h"
#include "filesystem/Directory.h"
#include "games/addons/disc/GameClientDiscModel.h"
#include "utils/Crc32.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <tinyxml2.h>

#include <cstring>
#include <optional>
#include <vector>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto PROFILE_ROOT = "special://masterprofile";
constexpr auto DISC_STATE_DIRECTORY = "games/discstate";
constexpr auto XML_ROOT = "discstate";
constexpr auto XML_SLOTS = "slots";
constexpr auto XML_SLOT = "slot";
constexpr auto XML_SELECTED = "selected";
constexpr auto XML_ATTR_TYPE = "type";
constexpr auto XML_ATTR_PATH = "path";
constexpr auto XML_ATTR_LABEL = "label";
constexpr auto XML_ATTR_INDEX = "index";

constexpr auto TYPE_DISC = "disc";
constexpr auto TYPE_EMPTY = "empty";
constexpr auto TYPE_REMOVED = "removed";
constexpr auto TYPE_NONE = "none";

std::string GetDiscStateDirectory()
{
  return URIUtils::AddFileToFolder(PROFILE_ROOT, DISC_STATE_DIRECTORY);
}

bool IsPersistableDiscPath(const std::string& path)
{
  return !path.empty() && !URIUtils::IsProtocol(path, "disc");
}

std::optional<size_t> GetFirstSelectable(const CGameClientDiscModel& model)
{
  for (size_t i = 0; i < model.Size(); ++i)
  {
    if (model.IsSelectableSlotByIndex(i))
      return i;
  }

  return std::nullopt;
}
} // namespace

std::string CGameClientDiscXML::GetXMLPath(const std::string& gamePath)
{
  const std::string fileName = StringUtils::Format("{:08x}.xml", Crc32::Compute(gamePath));
  return URIUtils::AddFileToFolder(GetDiscStateDirectory(), fileName);
}

bool CGameClientDiscXML::Load(const std::string& gamePath, CGameClientDiscModel& model) const
{
  model.Clear();

  if (gamePath.empty())
    return true;

  const std::string xmlPath = GetXMLPath(gamePath);

  if (!CFileUtils::Exists(xmlPath))
    return true;

  CXBMCTinyXML2 xmlDoc;
  if (!xmlDoc.LoadFile(xmlPath))
  {
    CLog::Log(LOGWARNING, "Failed to load disc state XML {}: {} at line {}",
              CURL::GetRedacted(xmlPath), xmlDoc.ErrorStr(), xmlDoc.ErrorLineNum());
    return false;
  }

  const tinyxml2::XMLElement* rootElement = xmlDoc.RootElement();
  if (rootElement == nullptr || std::strcmp(rootElement->Name(), XML_ROOT) != 0)
  {
    CLog::Log(LOGWARNING, "Failed to parse disc state XML {}, missing root element '{}'",
              CURL::GetRedacted(xmlPath), XML_ROOT);
    model.Clear();
    return false;
  }

  std::vector<GameClientDiscEntry> discs;

  const tinyxml2::XMLElement* slotsElement = rootElement->FirstChildElement(XML_SLOTS);
  const tinyxml2::XMLElement* slotElement = slotsElement != nullptr ? slotsElement->FirstChildElement(XML_SLOT)
                                                                     : nullptr;

  while (slotElement != nullptr)
  {
    const char* type = slotElement->Attribute(XML_ATTR_TYPE);
    const std::string label = slotElement->Attribute(XML_ATTR_LABEL) != nullptr
                                  ? slotElement->Attribute(XML_ATTR_LABEL)
                                  : "";

    if (type != nullptr && std::strcmp(type, TYPE_REMOVED) == 0)
    {
      discs.push_back({GameClientDiscEntry::DiscSlotType::RemovedSlot, "", "", ""});
    }
    else if (type != nullptr && std::strcmp(type, TYPE_EMPTY) == 0)
    {
      discs.push_back({GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", label});
    }
    else
    {
      const char* path = slotElement->Attribute(XML_ATTR_PATH);
      if (path != nullptr && IsPersistableDiscPath(path))
      {
        discs.push_back({GameClientDiscEntry::DiscSlotType::Disc, path,
                         CGameClientDiscModel::DeriveBasename(path), label});
      }
      else
      {
        discs.push_back({GameClientDiscEntry::DiscSlotType::EmptySlot, "", "", label});
      }
    }

    slotElement = slotElement->NextSiblingElement(XML_SLOT);
  }

  model.SetDiscs(discs);

  const std::optional<size_t> firstSelectable = GetFirstSelectable(model);
  if (firstSelectable.has_value())
  {
    model.SetMainDiscByIndex(*firstSelectable);
    model.SetLastDiscByIndex(*firstSelectable);
  }

  const tinyxml2::XMLElement* selectedElement = rootElement->FirstChildElement(XML_SELECTED);
  if (selectedElement != nullptr)
  {
    const char* selectedType = selectedElement->Attribute(XML_ATTR_TYPE);
    if (selectedType != nullptr && std::strcmp(selectedType, TYPE_NONE) == 0)
    {
      model.SetSelectedNoDisc();
    }
    else
    {
      const int selectedIndex = selectedElement->IntAttribute(XML_ATTR_INDEX, -1);
      if (selectedIndex >= 0 && model.IsRealDiscByIndex(static_cast<size_t>(selectedIndex)))
        model.SetSelectedDiscByIndex(static_cast<size_t>(selectedIndex));
      else
        model.SetSelectedNoDisc();
    }
  }
  else
  {
    model.SetSelectedNoDisc();
  }

  return true;
}

bool CGameClientDiscXML::Save(const std::string& gamePath, const CGameClientDiscModel& model) const
{
  if (gamePath.empty())
    return true;

  const std::string directory = GetDiscStateDirectory();
  if (!XFILE::CDirectory::Exists(directory) && !XFILE::CDirectory::Create(directory))
  {
    CLog::Log(LOGWARNING, "Failed to create disc state directory {}", CURL::GetRedacted(directory));
    return false;
  }

  CXBMCTinyXML2 xmlDoc;

  tinyxml2::XMLElement* rootElement = xmlDoc.NewElement(XML_ROOT);
  xmlDoc.InsertEndChild(rootElement);

  tinyxml2::XMLElement* slotsElement = xmlDoc.NewElement(XML_SLOTS);
  rootElement->InsertEndChild(slotsElement);

  for (const GameClientDiscEntry& disc : model.GetDiscs())
  {
    tinyxml2::XMLElement* slotElement = xmlDoc.NewElement(XML_SLOT);

    switch (disc.slotType)
    {
      case GameClientDiscEntry::DiscSlotType::Disc:
      {
        if (IsPersistableDiscPath(disc.path))
        {
          slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_DISC);
          slotElement->SetAttribute(XML_ATTR_PATH, disc.path.c_str());
        }
        else
        {
          slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_EMPTY);
        }

        if (!disc.cachedLabel.empty())
          slotElement->SetAttribute(XML_ATTR_LABEL, disc.cachedLabel.c_str());
        break;
      }
      case GameClientDiscEntry::DiscSlotType::EmptySlot:
      {
        slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_EMPTY);
        if (!disc.cachedLabel.empty())
          slotElement->SetAttribute(XML_ATTR_LABEL, disc.cachedLabel.c_str());
        break;
      }
      case GameClientDiscEntry::DiscSlotType::RemovedSlot:
      {
        slotElement->SetAttribute(XML_ATTR_TYPE, TYPE_REMOVED);
        break;
      }
    }

    slotsElement->InsertEndChild(slotElement);
  }

  tinyxml2::XMLElement* selectedElement = xmlDoc.NewElement(XML_SELECTED);
  rootElement->InsertEndChild(selectedElement);

  const std::optional<size_t> selectedIndex = model.GetSelectedDiscIndex();
  if (selectedIndex.has_value() && model.IsRealDiscByIndex(*selectedIndex) &&
      IsPersistableDiscPath(model.GetPathByIndex(*selectedIndex)))
  {
    selectedElement->SetAttribute(XML_ATTR_TYPE, TYPE_DISC);
    selectedElement->SetAttribute(XML_ATTR_INDEX, static_cast<unsigned int>(*selectedIndex));
  }
  else
  {
    selectedElement->SetAttribute(XML_ATTR_TYPE, TYPE_NONE);
  }

  const std::string xmlPath = GetXMLPath(gamePath);
  if (!xmlDoc.SaveFile(xmlPath))
  {
    CLog::Log(LOGWARNING, "Failed to save disc state XML {}", CURL::GetRedacted(xmlPath));
    return false;
  }

  return true;
}
