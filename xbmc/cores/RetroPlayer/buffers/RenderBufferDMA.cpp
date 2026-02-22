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
#if defined(HAVE_LINUX_DMA_HEAP)
#include "utils/DMAHeapBufferObject.h"
#endif
#include "utils/DmaBufImportProber.h"
#include "utils/EGLImage.h"
#include "utils/StringUtils.h"

#if defined(HAVE_GBM)
#include "utils/GBMBufferObject.h"
#endif
#include "utils/log.h"
#include "windowing/WinSystem.h"
#include "windowing/linux/WinSystemEGL.h"

#include <optional>

using namespace KODI;
using namespace RETRO;

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

  const EGLDisplay eglDisplay = winSystemEGL->GetEGLDisplay();
  m_egl = std::make_unique<CEGLImage>(eglDisplay);
  m_prober = std::make_unique<CDmaBufImportProber>(eglDisplay);

  CLog::Log(LOGDEBUG, "CRenderBufferDMA: using BufferObject type: {}", m_bo->GetName());
}

CRenderBufferDMA::~CRenderBufferDMA()
{
  DeleteTexture();
}

CDmaBufImportProber::ProbeKey CRenderBufferDMA::BuildProbeKey(unsigned int width,
                                                               unsigned int height) const
{
  const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const auto* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

  CDmaBufImportProber::ProbeKey key;
  key.gpu = StringUtils::Format("{}:{}", vendor ? vendor : "unknown", renderer ? renderer : "unknown");
  key.drmFourcc = m_fourcc;
  key.widthBucket = width <= 512 ? 512 : (width <= 1024 ? 1024 : (width <= 2048 ? 2048 : 4096));
  key.heightBucket = height <= 512 ? 512 : (height <= 1024 ? 1024 : (height <= 2048 ? 2048 : 4096));
  return key;
}

bool CRenderBufferDMA::Allocate(AVPixelFormat format, unsigned int width, unsigned int height)
{
  // Initialize IRenderBuffer
  m_format = format;
  m_width = width;
  m_height = height;

  const auto probeKey = BuildProbeKey(width, height);

  auto allocateFn = [](CDmaBufImportProber::AllocatorType allocator,
                       uint32_t drmFourcc,
                       uint32_t boWidth,
                       uint32_t boHeight,
                       uint64_t modifier,
                       uint32_t strideAlignment) -> std::unique_ptr<IBufferObject> {
    std::unique_ptr<IBufferObject> bo;

    switch (allocator)
    {
      case CDmaBufImportProber::AllocatorType::GBMModifiers:
      {
#if defined(HAVE_GBM)
        auto gbm = std::make_unique<CGBMBufferObject>();
        if (!gbm->CreateBufferObjectWithModifier(drmFourcc, boWidth, boHeight, modifier))
          return nullptr;
        bo = std::move(gbm);
        break;
#else
        return nullptr;
#endif
      }
      case CDmaBufImportProber::AllocatorType::GBMImplicit:
      {
#if defined(HAVE_GBM)
        auto gbm = std::make_unique<CGBMBufferObject>();
        if (!gbm->CreateBufferObject(drmFourcc, boWidth, boHeight))
          return nullptr;
        bo = std::move(gbm);
        break;
#else
        return nullptr;
#endif
      }
      case CDmaBufImportProber::AllocatorType::DMAHeapImplicit:
      {
#if defined(HAVE_LINUX_DMA_HEAP)
        auto dma = std::make_unique<CDMAHeapBufferObject>();
        if (!dma->CreateBufferObject(drmFourcc, boWidth, boHeight))
          return nullptr;
        bo = std::move(dma);
        break;
#else
        return nullptr;
#endif
      }
      case CDmaBufImportProber::AllocatorType::DMAHeapAligned:
      {
#if defined(HAVE_LINUX_DMA_HEAP)
        auto dma = std::make_unique<CDMAHeapBufferObject>();
        if (!dma->CreateBufferObjectAligned(drmFourcc, boWidth, boHeight, strideAlignment))
          return nullptr;
        bo = std::move(dma);
        break;
#else
        return nullptr;
#endif
      }
    }

    return bo;
  };

  auto probeResult = m_prober->ProbeAndAdapt(probeKey, width, height, m_fourcc, allocateFn,
                                             true /*prefer linear*/);
  if (!probeResult.success)
  {
    CRPRendererDMAUtils::DisableForSession(m_fourcc, m_width, m_height, DRM_FORMAT_MOD_INVALID,
                                           "all non-copy dma-buf import paths failed");
    return false;
  }

  m_bo = allocateFn(probeResult.allocator, m_fourcc, m_width, m_height, probeResult.modifier,
                    probeResult.strideAlignment);
  if (!m_bo)
    return false;

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
  if (m_bo->GetFd() < 0)
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

  const bool success = m_egl->CreateImage(attribs);
  if (success)
    m_egl->UploadImage(m_textureTarget);
  else
  {
    const auto probeKey = BuildProbeKey(m_width, m_height);
    m_prober->Invalidate(probeKey);

    CRPRendererDMAUtils::DisableForSession(m_fourcc, m_width, m_height, m_bo->GetModifier(),
                                           "eglCreateImageKHR failed");
  }

  m_egl->DestroyImage();

  glBindTexture(m_textureTarget, 0);

  return success;
}

void CRenderBufferDMA::DeleteTexture()
{
  if (glIsTexture(m_textureId))
    glDeleteTextures(1, &m_textureId);

  m_textureId = 0;
}
