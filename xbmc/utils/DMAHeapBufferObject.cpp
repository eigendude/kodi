/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DMAHeapBufferObject.h"

#include "utils/BufferObjectFactory.h"
#include "utils/log.h"

#include <array>

#include <drm_fourcc.h>
#include <fcntl.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace
{

constexpr std::array<const char*, 2> DMA_HEAP_PATHS = {
    "/dev/dma_heap/system",
    "/dev/dma_heap/system-uncached",
};

const char* g_dmaHeapPath{nullptr};

bool IsAllocatorAllowed()
{
  const auto allocator = CBufferObjectFactory::GetDmabufAllocatorPreference();
  return allocator == CBufferObjectFactory::DmabufAllocator::AUTO ||
         allocator == CBufferObjectFactory::DmabufAllocator::DMA_HEAP;
}

} // namespace

std::unique_ptr<CBufferObject> CDMAHeapBufferObject::Create()
{
  return std::make_unique<CDMAHeapBufferObject>();
}

void CDMAHeapBufferObject::Register()
{
  if (!IsAllocatorAllowed())
    return;

  for (auto path : DMA_HEAP_PATHS)
  {
    int fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd < 0)
      continue;

    close(fd);
    g_dmaHeapPath = path;
    break;
  }

  if (g_dmaHeapPath == nullptr)
  {
    if (CBufferObjectFactory::GetDmabufAllocatorPreference() ==
        CBufferObjectFactory::DmabufAllocator::DMA_HEAP)
    {
      CLog::Log(LOGWARNING,
                "CDMAHeapBufferObject::{} - dma_heap allocator was forced, but no dma-heap"
                " device was found",
                __FUNCTION__);
    }
    return;
  }

  CLog::Log(LOGDEBUG, "CDMAHeapBufferObject::{} - using {}", __FUNCTION__, g_dmaHeapPath);
  CBufferObjectFactory::RegisterBufferObject(CDMAHeapBufferObject::Create);
}

CDMAHeapBufferObject::~CDMAHeapBufferObject()
{
  ReleaseMemory();
  DestroyBufferObject();

  if (m_dmaheapfd >= 0)
  {
    close(m_dmaheapfd);
    m_dmaheapfd = -1;
  }
}

bool CDMAHeapBufferObject::CreateBufferObject(uint32_t format, uint32_t width, uint32_t height)
{
  if (m_fd >= 0)
    return true;

  switch (format)
  {
    case DRM_FORMAT_ARGB8888:
      m_stride = width * 4;
      break;
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_RGB565:
      m_stride = width * 2;
      break;
    default:
      throw std::runtime_error("CDMAHeapBufferObject: pixel format not implemented");
  }

  return CreateBufferObject(static_cast<uint64_t>(m_stride) * height);
}

bool CDMAHeapBufferObject::CreateBufferObject(uint64_t size)
{
  m_size = size;

  if (m_dmaheapfd < 0)
  {
    m_dmaheapfd = open(g_dmaHeapPath, O_RDWR | O_CLOEXEC);
    if (m_dmaheapfd < 0)
    {
      CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - failed to open {}: {}", __FUNCTION__,
                g_dmaHeapPath, strerror(errno));
      return false;
    }
  }

  struct dma_heap_allocation_data allocData{};
  allocData.len = m_size;
  allocData.fd_flags = O_CLOEXEC | O_RDWR;
  allocData.heap_flags = 0;

  if (ioctl(m_dmaheapfd, DMA_HEAP_IOCTL_ALLOC, &allocData) < 0)
  {
    CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - ioctl DMA_HEAP_IOCTL_ALLOC failed: {}",
              __FUNCTION__, strerror(errno));
    return false;
  }

  m_fd = allocData.fd;
  m_size = allocData.len;

  if (m_fd < 0 || m_size == 0)
  {
    CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - invalid allocation data: fd={} len={}",
              __FUNCTION__, m_fd, m_size);
    return false;
  }

  return true;
}

void CDMAHeapBufferObject::DestroyBufferObject()
{
  if (m_fd < 0)
    return;

  int ret = close(m_fd);
  if (ret < 0)
    CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - close failed, errno={}", __FUNCTION__,
              strerror(errno));

  m_fd = -1;
  m_stride = 0;
  m_size = 0;
}

uint8_t* CDMAHeapBufferObject::GetMemory()
{
  if (m_fd < 0)
    return nullptr;

  if (m_map)
    return m_map;

  m_map = static_cast<uint8_t*>(mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd,
                                     0));
  if (m_map == MAP_FAILED)
  {
    CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - mmap failed: {}", __FUNCTION__,
              strerror(errno));
    m_map = nullptr;
    return nullptr;
  }

  return m_map;
}

void CDMAHeapBufferObject::ReleaseMemory()
{
  if (!m_map)
    return;

  int ret = munmap(m_map, m_size);
  if (ret < 0)
    CLog::Log(LOGERROR, "CDMAHeapBufferObject::{} - munmap failed: {}", __FUNCTION__,
              strerror(errno));

  m_map = nullptr;
}
