/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/EGLImage.h"

#include <gtest/gtest.h>

namespace
{

bool HasAttribute(const std::vector<EGLint>& attrs, EGLint key)
{
  for (size_t i = 0; i + 1 < attrs.size() && attrs[i] != EGL_NONE; i += 2)
  {
    if (attrs[i] == key)
      return true;
  }

  return false;
}

} // namespace

TEST(TestEGLImage, BuildAttributeListOmitsModifierAttrsForImplicitBuffers)
{
  // Verify implicitly-allocated dmabufs do not advertise modifier attributes.
  CEGLImage::EglAttrs imageAttrs;
  imageAttrs.width = 320;
  imageAttrs.height = 240;
  imageAttrs.format = DRM_FORMAT_ARGB8888;
  imageAttrs.planes[0].fd = 7;
  imageAttrs.planes[0].pitch = 1280;
  imageAttrs.planes[0].modifier = DRM_FORMAT_MOD_INVALID;

  const auto attrs = CEGLImage::BuildAttributeList(imageAttrs, true);

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  EXPECT_FALSE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT));
  EXPECT_FALSE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT));
#endif
}

TEST(TestEGLImage, BuildAttributeListIncludesModifierAttrsForExplicitLinearBuffers)
{
  // Verify explicitly-negotiated linear modifiers are passed during EGL import.
  CEGLImage::EglAttrs imageAttrs;
  imageAttrs.width = 320;
  imageAttrs.height = 240;
  imageAttrs.format = DRM_FORMAT_ARGB8888;
  imageAttrs.planes[0].fd = 7;
  imageAttrs.planes[0].pitch = 1280;
  imageAttrs.planes[0].modifier = DRM_FORMAT_MOD_LINEAR;

  const auto attrs = CEGLImage::BuildAttributeList(imageAttrs, true);

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  EXPECT_TRUE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT));
  EXPECT_TRUE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT));
#endif
}
