/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "utils/EGLImage.h"

#include <drm_fourcc.h>

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

// Verify that linear modifiers are explicitly propagated in the EGL import attribute list.
TEST(TestEGLImage, IncludesLinearModifierAttributes)
{
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  CEGLImage::EglAttrs imageAttrs;
  imageAttrs.width = 1280;
  imageAttrs.height = 720;
  imageAttrs.format = DRM_FORMAT_ARGB8888;
  imageAttrs.planes[0].fd = 12;
  imageAttrs.planes[0].offset = 0;
  imageAttrs.planes[0].pitch = 5120;
  imageAttrs.planes[0].modifier = DRM_FORMAT_MOD_LINEAR;

  const auto attrs = CEGLImage::BuildAttributeList(imageAttrs, true);

  EXPECT_TRUE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT));
  EXPECT_TRUE(HasAttribute(attrs, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT));
#else
  GTEST_SKIP() << "EGL_EXT_image_dma_buf_import_modifiers is not available in this build.";
#endif
}
