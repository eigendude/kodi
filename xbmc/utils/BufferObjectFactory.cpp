/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObjectFactory.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdlib>

std::list<std::function<std::unique_ptr<CBufferObject>()>> CBufferObjectFactory::m_bufferObjects;

std::unique_ptr<CBufferObject> CBufferObjectFactory::CreateBufferObject(bool needsCreateBySize)
{
  for (const auto& bufferObject : m_bufferObjects)
  {
    auto bo = bufferObject();

    if (needsCreateBySize)
    {
      if (!bo->CreateBufferObject(1))
        continue;

      bo->DestroyBufferObject();
    }

    return bo;
  }

  return nullptr;
}

void CBufferObjectFactory::RegisterBufferObject(
    const std::function<std::unique_ptr<CBufferObject>()>& createFunc)
{
  m_bufferObjects.emplace_back(createFunc);
}

void CBufferObjectFactory::ClearBufferObjects()
{
  m_bufferObjects.clear();
}

CBufferObjectFactory::DmabufAllocator CBufferObjectFactory::GetDmabufAllocatorPreference()
{
  static const DmabufAllocator allocator = []() {
    const char* allocatorEnv = std::getenv("KODI_RETROPLAYER_DMABUF_ALLOCATOR");
    if (allocatorEnv == nullptr)
      return DmabufAllocator::AUTO;

    std::string allocatorStr{allocatorEnv};
    StringUtils::ToLower(allocatorStr);

    if (allocatorStr == "auto")
      return DmabufAllocator::AUTO;

    if (allocatorStr == "dma_heap")
      return DmabufAllocator::DMA_HEAP;

    if (allocatorStr == "udmabuf")
      return DmabufAllocator::UDMABUF;

    CLog::Log(LOGWARNING,
              "CBufferObjectFactory::{} - invalid KODI_RETROPLAYER_DMABUF_ALLOCATOR={}"
              " (supported values: auto|dma_heap|udmabuf)",
              __FUNCTION__, allocatorEnv);

    return DmabufAllocator::AUTO;
  }();

  return allocator;
}
