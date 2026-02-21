/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererDMAUtils.h"

#if !defined(HAS_GLES)
#include "RPRendererDMAOpenGL.h"
#else
#include "RPRendererDMAOpenGLES.h"
#endif

#include "utils/DRMHelpers.h"
#include "utils/log.h"

#include <atomic>

using namespace KODI;
using namespace RETRO;

namespace
{
std::atomic_bool g_dmaSupportedForSession{true};
}

bool CRPRendererDMAUtils::SupportsScalingMethod(SCALINGMETHOD method)
{
#if !defined(HAS_GLES)
  return CRPRendererDMAOpenGL::SupportsScalingMethod(method);
#else
  return CRPRendererDMAOpenGLES::SupportsScalingMethod(method);
#endif
}

bool CRPRendererDMAUtils::IsSupportedForSession()
{
  return g_dmaSupportedForSession.load();
}

void CRPRendererDMAUtils::DisableForSession(
    uint32_t fourcc, unsigned int width, unsigned int height, uint64_t modifier, const char* reason)
{
  g_dmaSupportedForSession.store(false);

  CLog::Log(LOGWARNING,
            "RetroPlayer[RENDER]: DMAOpenGL disabled for this session ({}) after dma-buf "
            "import failure, attempted {} {}x{}, modifier {}",
            reason, DRMHELPERS::FourCCToString(fourcc), width, height,
            DRMHELPERS::ModifierToString(modifier));
}
