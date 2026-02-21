/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BufferObjectFactory.h"

#include "utils/log.h"

#include <cstdlib>

std::list<CBufferObjectFactory::BufferObjectRegistration> CBufferObjectFactory::m_bufferObjects;

std::unique_ptr<CBufferObject> CBufferObjectFactory::CreateBufferObject(bool needsCreateBySize)
{
  const char* allocatorEnv = getenv("KODI_RETROPLAYER_DMABUF_ALLOCATOR");
  const std::string allocatorOverride = allocatorEnv ? allocatorEnv : "";

  for (const auto& bufferObject : m_bufferObjects)
  {
    if (!allocatorOverride.empty() && allocatorOverride != "auto" && allocatorOverride != bufferObject.name)
      continue;

    auto bo = bufferObject.create();

    if (needsCreateBySize)
    {
      if (!bo->CreateBufferObject(1))
        continue;

      bo->DestroyBufferObject();
    }

    CLog::Log(LOGDEBUG, "CBufferObjectFactory::{} - selected allocator '{}' ({})", __FUNCTION__,
              bufferObject.name, bo->GetName());
    return bo;
  }

  if (!allocatorOverride.empty() && allocatorOverride != "auto")
  {
    CLog::Log(LOGWARNING,
              "CBufferObjectFactory::{} - no allocator registered for override '{}'", __FUNCTION__,
              allocatorOverride);
  }

  return nullptr;
}

void CBufferObjectFactory::RegisterBufferObject(
    const std::string& name, const std::function<std::unique_ptr<CBufferObject>()>& createFunc)
{
  m_bufferObjects.emplace_back(BufferObjectRegistration{name, createFunc});
}

void CBufferObjectFactory::ClearBufferObjects()
{
  m_bufferObjects.clear();
}
