/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureGL.h"

/*!\brief Wrapper for external OpenGL textures that should not be deleted
 */
class CNonOwningGLTexture : public CGLTexture
{
public:
  CNonOwningGLTexture(unsigned int width,
                      unsigned int height,
                      XB_FMT format,
                      GLuint texture)
    : CGLTexture(width, height, format, texture)
  {
  }

  ~CNonOwningGLTexture() override = default;

  void DestroyTextureObject() override {}
};
