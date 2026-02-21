/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GBMBufferObject.h"

#include "BufferObjectFactory.h"
#include "utils/log.h"

#include <array>
#include <vector>

#include <dirent.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>

namespace
{

std::vector<std::string> GetRenderNodes()
{
  std::vector<std::string> renderNodes;

  renderNodes.emplace_back("/dev/dri/renderD128");

  DIR* driDir = opendir("/dev/dri");
  if (!driDir)
    return renderNodes;

  dirent* entry;
  while ((entry = readdir(driDir)) != nullptr)
  {
    std::string nodeName{entry->d_name};
    if (nodeName.rfind("renderD", 0) != 0)
      continue;

    std::string nodePath = "/dev/dri/" + nodeName;
    if (nodePath != "/dev/dri/renderD128")
      renderNodes.emplace_back(std::move(nodePath));
  }

  closedir(driDir);

  return renderNodes;
}

} // namespace

std::unique_ptr<CBufferObject> CGBMBufferObject::Create()
{
  return std::make_unique<CGBMBufferObject>();
}

void CGBMBufferObject::Register()
{
  CBufferObjectFactory::RegisterBufferObject(CGBMBufferObject::Create);
}

CGBMBufferObject::CGBMBufferObject()
{
  for (const auto& renderNode : GetRenderNodes())
  {
    int fd = open(renderNode.c_str(), O_RDWR | O_CLOEXEC);
    if (fd < 0)
      continue;

    m_device = gbm_create_device(fd);
    if (!m_device)
    {
      close(fd);
      continue;
    }

    m_renderFd = fd;

    CLog::Log(LOGDEBUG, "CGBMBufferObject::{} - using DRM render node {}", __FUNCTION__, renderNode);
    break;
  }
}

CGBMBufferObject::~CGBMBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();

  if (m_device)
    gbm_device_destroy(m_device);

  m_device = nullptr;

  if (m_renderFd >= 0)
    close(m_renderFd);

  m_renderFd = -1;
}

bool CGBMBufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  if (!m_device)
    return false;

  m_width = width;
  m_height = height;

#if defined(HAS_GBM_MODIFIERS)
  const uint64_t modifiers[] = {DRM_FORMAT_MOD_LINEAR};
#if defined(HAS_GBM_BO_CREATE_WITH_MODIFIERS2)
  m_bo = gbm_bo_create_with_modifiers2(m_device, m_width, m_height, format, modifiers,
                                       std::size(modifiers), GBM_BO_USE_RENDERING);
#else
  m_bo = gbm_bo_create_with_modifiers(m_device, m_width, m_height, format, modifiers,
                                      std::size(modifiers));
#endif
#endif

  if (!m_bo)
  {
    m_bo = gbm_bo_create(m_device, m_width, m_height, format,
                         GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
  }

  if (!m_bo)
  {
    m_bo = gbm_bo_create(m_device, m_width, m_height, format, GBM_BO_USE_RENDERING);
  }

  if (!m_bo)
    return false;

  m_fd = gbm_bo_get_fd(m_bo);
  if (m_fd < 0)
  {
    gbm_bo_destroy(m_bo);
    m_bo = nullptr;
    return false;
  }

  m_stride = gbm_bo_get_stride(m_bo);

  return true;
}

void CGBMBufferObject::DestroyBufferObject()
{
  if (m_fd >= 0)
    close(m_fd);

  m_fd = -1;

  if (m_bo)
    gbm_bo_destroy(m_bo);

  m_bo = nullptr;
  m_stride = 0;
  m_width = 0;
  m_height = 0;
}

uint8_t* CGBMBufferObject::GetMemory()
{
  if (!m_bo)
    return nullptr;

  if (m_map)
    return m_map;

  m_map = static_cast<uint8_t*>(
      gbm_bo_map(m_bo, 0, 0, m_width, m_height, GBM_BO_TRANSFER_WRITE, &m_stride, &m_map_data));
  return m_map;
}

void CGBMBufferObject::ReleaseMemory()
{
  if (m_bo && m_map)
  {
    gbm_bo_unmap(m_bo, m_map_data);
    m_map_data = nullptr;
    m_map = nullptr;
  }
}

uint64_t CGBMBufferObject::GetModifier()
{
#if defined(HAS_GBM_MODIFIERS)
  if (m_bo)
    return gbm_bo_get_modifier(m_bo);
#endif

  return DRM_FORMAT_MOD_INVALID;
}
