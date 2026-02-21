/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"

#include <cstdint>

namespace KODI
{
namespace RETRO
{
class CRPRendererDMAUtils
{
public:
  /*!
   * \brief Check if the DMA renderer supports the specified scaling method
   *
   * Note how the graphics API (OpenGL vs. OpenGLES) is abstracted here. The
   * reason is because this interface is exposed to the DMA buffer code, which
   * is currently independent of graphics API (i.e. no use of `HAS_GLES` in
   * the DMA buffer code).
   */
  static bool SupportsScalingMethod(SCALINGMETHOD method);

  /*! \brief Runtime state that controls whether DMA OpenGL/GLES renderers may be used. */
  static bool IsSupportedForSession();

  /*! \brief Disable DMA rendering for this Kodi session after an import/capability failure. */
  static void DisableForSession(uint32_t fourcc,
                                unsigned int width,
                                unsigned int height,
                                uint64_t modifier,
                                const char* reason);

#ifndef NDEBUG
  /*! \brief Debug-only knob to simulate dma-buf import failures. */
  static bool ShouldSimulateImportFailure();
#endif
};
} // namespace RETRO
} // namespace KODI
