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

  /*!
   * \brief Runtime state that controls whether DMA OpenGL/GLES renderers may
   * be used.
   *
   * \return `true` if the current Kodi session supports DMA renderers, `false`
   * if DMA renderers should be disabled for the current session
   */
  static bool IsSupportedForSession();

  /*!
   * \brief Disable DMA rendering for this Kodi session after an
   * import/capability failure.
   *
   * \param fourcc The fourcc code of the buffer that failed to import
   * \param width The width of the buffer that failed to import
   * \param height The height of the buffer that failed to import
   * \param modifier The modifier of the buffer that failed to import
   * \param reason A human-readable string describing the failure reason, which
   * will be logged for debugging purposes
   */
  static void DisableForSession(uint32_t fourcc,
                                unsigned int width,
                                unsigned int height,
                                uint64_t modifier,
                                const char* reason);
};
} // namespace RETRO
} // namespace KODI
