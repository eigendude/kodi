/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GBMBufferObject.h"

#include "BufferObjectFactory.h"
#include "utils/DRMHelpers.h"
#include "utils/log.h"

#include <array>
#include <filesystem>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <dlfcn.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>

namespace
{
uint32_t MapRetroPlayerFormat(uint32_t format)
{
  switch (format)
  {
    case DRM_FORMAT_ARGB8888:
      return DRM_FORMAT_ARGB8888;
    case DRM_FORMAT_XRGB8888:
      return DRM_FORMAT_XRGB8888;
    case DRM_FORMAT_ARGB1555:
      return DRM_FORMAT_ARGB1555;
    case DRM_FORMAT_RGB565:
      return DRM_FORMAT_RGB565;
    default:
      throw std::runtime_error("CGBMBufferObject: pixel format not implemented");
  }
}

int OpenRenderNode()
{
  constexpr auto renderNode = "/dev/dri/renderD128";
  int fd = open(renderNode, O_RDWR | O_CLOEXEC);
  if (fd >= 0)
    return fd;

  for (const auto& entry : std::filesystem::directory_iterator("/dev/dri"))
  {
    if (!entry.is_character_file())
      continue;

    const auto filename = entry.path().filename().string();
    if (filename.rfind("renderD", 0) != 0)
      continue;

    fd = open(entry.path().c_str(), O_RDWR | O_CLOEXEC);
    if (fd >= 0)
      return fd;
  }

  return -1;
}
} // namespace

std::unique_ptr<CBufferObject> CGBMBufferObject::Create()
{
  return std::make_unique<CGBMBufferObject>();
}

void CGBMBufferObject::Register()
{
  int fd = OpenRenderNode();
  if (fd < 0)
  {
    CLog::Log(LOGDEBUG, "CGBMBufferObject::{} - unable to open DRM render node: {}", __FUNCTION__,
              strerror(errno));
    return;
  }

  gbm_device* device = gbm_create_device(fd);
  if (!device)
  {
    CLog::Log(LOGDEBUG, "CGBMBufferObject::{} - unable to create GBM device", __FUNCTION__);
    close(fd);
    return;
  }

  gbm_device_destroy(device);
  close(fd);

  CBufferObjectFactory::RegisterBufferObject("gbm", CGBMBufferObject::Create);
}

CGBMBufferObject::~CGBMBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();
}

bool CGBMBufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  m_width = width;
  m_height = height;

  const uint32_t drmFormat = MapRetroPlayerFormat(format);

  m_renderNodeFd = OpenRenderNode();
  if (m_renderNodeFd < 0)
  {
    CLog::Log(LOGERROR, "CGBMBufferObject::{} - unable to open DRM render node: {}", __FUNCTION__,
              strerror(errno));
    return false;
  }

  m_device = gbm_create_device(m_renderNodeFd);
  if (!m_device)
  {
    CLog::Log(LOGERROR, "CGBMBufferObject::{} - unable to create GBM device", __FUNCTION__);
    DestroyBufferObject();
    return false;
  }

#if defined(HAS_GBM_MODIFIERS)
  using GbmBoCreateWithModifiers2 = struct gbm_bo* (*)(struct gbm_device*, uint32_t, uint32_t,
                                                       uint32_t, const uint64_t*, const unsigned,
                                                       uint32_t);
  auto createWithModifiers2 = reinterpret_cast<GbmBoCreateWithModifiers2>(
      dlsym(RTLD_DEFAULT, "gbm_bo_create_with_modifiers2"));

  const std::array<uint64_t, 1> modifiers = {DRM_FORMAT_MOD_LINEAR};
  if (createWithModifiers2)
  {
    m_bo = createWithModifiers2(m_device, m_width, m_height, drmFormat, modifiers.data(),
                                modifiers.size(), GBM_BO_USE_RENDERING);
  }

  if (!m_bo)
  {
    m_bo = gbm_bo_create_with_modifiers(m_device, m_width, m_height, drmFormat, modifiers.data(),
                                        modifiers.size());
  }
#endif

  if (!m_bo)
  {
#if defined(GBM_BO_USE_LINEAR)
    m_bo = gbm_bo_create(m_device, m_width, m_height, drmFormat,
                         GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
#endif
  }

  if (!m_bo)
    m_bo = gbm_bo_create(m_device, m_width, m_height, drmFormat, GBM_BO_USE_RENDERING);

  if (!m_bo)
  {
    CLog::Log(LOGERROR, "CGBMBufferObject::{} - gbm_bo_create failed", __FUNCTION__);
    DestroyBufferObject();
    return false;
  }

  m_fd = gbm_bo_get_fd(m_bo);
  if (m_fd < 0)
  {
    CLog::Log(LOGERROR, "CGBMBufferObject::{} - gbm_bo_get_fd failed", __FUNCTION__);
    DestroyBufferObject();
    return false;
  }

  m_stride = gbm_bo_get_stride(m_bo);
  m_size = static_cast<uint64_t>(m_stride) * m_height;

  CLog::Log(LOGDEBUG,
            "CGBMBufferObject::{} - allocated format={} {}x{} stride={} modifier={:#x} fd={}",
            __FUNCTION__, DRMHELPERS::FourCCToString(drmFormat), m_width, m_height, m_stride,
            static_cast<unsigned long long>(GetModifier()), m_fd);

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

  if (m_device)
    gbm_device_destroy(m_device);

  m_device = nullptr;

  if (m_renderNodeFd >= 0)
    close(m_renderNodeFd);

  m_renderNodeFd = -1;

  m_stride = 0;
  m_size = 0;
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
  if (!m_bo || !m_map)
    return;

  gbm_bo_unmap(m_bo, m_map_data);
  m_map_data = nullptr;
  m_map = nullptr;
}

uint64_t CGBMBufferObject::GetModifier()
{
#if defined(HAS_GBM_MODIFIERS)
  if (m_bo)
    return gbm_bo_get_modifier(m_bo);
#endif
  return DRM_FORMAT_MOD_LINEAR;
}
