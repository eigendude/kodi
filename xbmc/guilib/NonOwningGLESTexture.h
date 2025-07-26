/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureGLES.h"

/*!\brief Wrapper for external GLES textures that should not be deleted
 */
class CNonOwningGLESTexture : public CGLESTexture
{
public:
  CNonOwningGLESTexture(unsigned int width,
                        unsigned int height,
                        XB_FMT format,
                        GLuint texture)
    : CGLESTexture(width, height, format, texture)
  {
  }

  ~CNonOwningGLESTexture() override = default;

  void DestroyTextureObject() override {}
};
