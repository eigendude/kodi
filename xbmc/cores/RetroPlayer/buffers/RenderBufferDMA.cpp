/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderBufferDMA.h"

#include "ServiceBroker.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererDMAUtils.h"
#include "utils/BufferObject.h"
#include "utils/DRMHelpers.h"
#include "utils/EGLImage.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

#include <fstream>
#include <mutex>
#include <set>

#include <cstdlib>


using namespace KODI;
using namespace RETRO;

namespace
{
std::mutex g_dmaImportFailureLogMutex;
std::set<std::pair<uint32_t, uint64_t>> g_dmaImportFailureLogKeys;

bool ShouldLogImportFailure(uint32_t fourcc, uint64_t modifier)
{
  std::unique_lock<std::mutex> lock(g_dmaImportFailureLogMutex);
  return g_dmaImportFailureLogKeys.emplace(fourcc, modifier).second;
}

std::string GetFdInfo(int fd)
{
  if (fd < 0)
    return "fd < 0";

  const std::string fdInfoPath = StringUtils::Format("/proc/self/fdinfo/{}", fd);
  std::ifstream fdInfo(fdInfoPath);
  if (!fdInfo.is_open())
    return StringUtils::Format("unable to open {}", fdInfoPath);

  return std::string(std::istreambuf_iterator<char>(fdInfo), std::istreambuf_iterator<char>());
}
} // namespace

CRenderBufferDMA::CRenderBufferDMA(CRenderContext& context, int fourcc)
  : m_context(context),
    m_fourcc(fourcc),
    m_bo(CBufferObject::GetBufferObject(false))
{
  auto winSystemEGL =
      dynamic_cast<KODI::WINDOWING::LINUX::CWinSystemEGL*>(CServiceBroker::GetWinSystem());

  if (winSystemEGL == nullptr)
    throw std::runtime_error("dynamic_cast failed to cast to CWinSystemEGL. This is likely due to "
                             "a build misconfiguration as DMA can only be used with EGL and "
                             "specifically platforms that implement CWinSystemEGL");

  m_egl = std::make_unique<CEGLImage>(winSystemEGL->GetEGLDisplay());

  CLog::Log(LOGDEBUG, "CRenderBufferDMA: using BufferObject type: {}", m_bo->GetName());
}

CRenderBufferDMA::~CRenderBufferDMA()
{
  DeleteTexture();
}

bool CRenderBufferDMA::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  if (!CRPRendererDMAUtils::IsSupportedForSession())
    return false;

  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  m_bo->CreateBufferObject(m_fourcc, m_width, m_height);

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  if (!m_egl->SupportsFormatAndModifier(m_fourcc, m_bo->GetModifier()))
  {
    CRPRendererDMAUtils::DisableForSession(m_fourcc, m_width, m_height, m_bo->GetModifier(),
                                           "unsupported EGL dma-buf format/modifier");
    return false;
  }
#endif

  return true;
}

size_t CRenderBufferDMA::GetFrameSize() const
{
  return m_bo->GetStride() * m_height;
}

uint8_t* CRenderBufferDMA::GetMemory()
{
  m_bo->SyncStart();
  return m_bo->GetMemory();
}

void CRenderBufferDMA::ReleaseMemory()
{
  m_bo->ReleaseMemory();
  m_bo->SyncEnd();
}

void CRenderBufferDMA::CreateTexture()
{
  glGenTextures(1, &m_textureId);

  glBindTexture(m_textureTarget, m_textureId);

  glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(m_textureTarget, 0);
}

bool CRenderBufferDMA::UploadTexture()
{
  if (!CRPRendererDMAUtils::IsSupportedForSession() || m_bo->GetFd() < 0)
    return false;

  if (!glIsTexture(m_textureId))
    CreateTexture();

  glBindTexture(m_textureTarget, m_textureId);

  std::array<CEGLImage::EglPlane, CEGLImage::MAX_NUM_PLANES> planes;

  planes[0].fd = m_bo->GetFd();
  planes[0].offset = 0;
  planes[0].pitch = m_bo->GetStride();
  planes[0].modifier = m_bo->GetModifier();

  CEGLImage::EglAttrs attribs;

  attribs.width = m_width;
  attribs.height = m_height;
  attribs.format = m_fourcc;
  attribs.planes = planes;

#if !defined(NDEBUG)
  const char* simulateImportFailure =
      std::getenv("KODI_RETROPLAYER_SIMULATE_DMABUF_IMPORT_FAILURE");
  const bool shouldSimulateImportFailure =
      simulateImportFailure != nullptr && simulateImportFailure[0] != '\0';
#else
  constexpr bool shouldSimulateImportFailure = false;
#endif

  const bool success = !shouldSimulateImportFailure && m_egl->CreateImage(attribs);
  if (success)
  {
    m_egl->UploadImage(m_textureTarget);
    m_egl->DestroyImage();
  }
  else
  {
    if (ShouldLogImportFailure(m_fourcc, m_bo->GetModifier()))
    {
      CLog::Log(
          LOGERROR,
          "CRenderBufferDMA::{} - failed to import dma-buf: egl_error={:#x}, fourcc={}, "
          "modifier={}, size={}x{}, pitch={}, offset={}, plane_fds=[{}, {}, {}]",
          __FUNCTION__, m_egl->GetLastError(), DRMHELPERS::FourCCToString(m_fourcc),
          DRMHELPERS::ModifierToString(m_bo->GetModifier()), m_width, m_height, m_bo->GetStride(),
          0, planes[0].fd, planes[1].fd, planes[2].fd);

      CLog::Log(LOGERROR,
                "CRenderBufferDMA::{} - /proc/self/fdinfo/{}:\n{}",
                __FUNCTION__,
                planes[0].fd,
                GetFdInfo(planes[0].fd));
    }

    CRPRendererDMAUtils::DisableForSession(
        m_fourcc, m_width, m_height, m_bo->GetModifier(),
        shouldSimulateImportFailure ? "simulated eglCreateImageKHR failure"
                                    : "eglCreateImageKHR failed");
  }

  glBindTexture(m_textureTarget, 0);

  return success;
}

void CRenderBufferDMA::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
