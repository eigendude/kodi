/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObjectFactory.h"

#include "utils/log.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

std::list<std::function<std::unique_ptr<CBufferObject>()>> CBufferObjectFactory::m_bufferObjects;

std::unique_ptr<CBufferObject> CBufferObjectFactory::CreateBufferObject(bool needsCreateBySize)
{
  std::vector<std::unique_ptr<CBufferObject>> candidates;

  for (const auto& bufferObject : m_bufferObjects)
  {
    auto bo = bufferObject();

    if (needsCreateBySize)
    {
      if (!bo->CreateBufferObject(1))
        continue;

      bo->DestroyBufferObject();
    }

    candidates.emplace_back(std::move(bo));
  }

  if (candidates.empty())
    return nullptr;

  std::string allocatorMode{"auto"};
  if (const char* envAllocator = std::getenv("KODI_RETROPLAYER_DMABUF_ALLOCATOR"))
    allocatorMode = envAllocator;

  std::string preferredBufferObject;
  if (allocatorMode == "dma_heap" || allocatorMode == "auto")
    preferredBufferObject = "CDMAHeapBufferObject";
  else if (allocatorMode == "udmabuf")
    preferredBufferObject = "CUDMABufferObject";
  else
  {
    CLog::Log(LOGWARNING,
              "CBufferObjectFactory::{} - unknown "
              "KODI_RETROPLAYER_DMABUF_ALLOCATOR='{}', using auto",
              __FUNCTION__, allocatorMode);
    preferredBufferObject = "CDMAHeapBufferObject";
    allocatorMode = "auto";
  }

  auto preferred = std::find_if(candidates.begin(), candidates.end(),
                                [&preferredBufferObject](const auto& candidate) {
                                  return candidate->GetName() == preferredBufferObject;
                                });

  if (preferred != candidates.end())
    return std::move(*preferred);

  if (allocatorMode == "auto")
  {
    auto udma = std::find_if(candidates.begin(), candidates.end(), [](const auto& candidate) {
      return candidate->GetName() == "CUDMABufferObject";
    });

    if (udma != candidates.end())
      return std::move(*udma);
  }

  CLog::Log(LOGDEBUG,
            "CBufferObjectFactory::{} - using fallback allocator {} "
            "(requested mode: {})",
            __FUNCTION__, candidates.front()->GetName(), allocatorMode);

  return std::move(candidates.front());
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
