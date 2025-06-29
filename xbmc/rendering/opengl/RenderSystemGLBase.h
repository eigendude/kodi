/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "rendering/RenderSystem.h"
#include "utils/Geometry.h"
#include "system_gl.h"

class CRenderSystemGLBase : public CRenderSystemBase
{
public:
  CRenderSystemGLBase() = default;
  ~CRenderSystemGLBase() override = default;

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect& rect) override;
  void SetScissors(const CRect& rect) override;
  void ResetScissors() override;

  void SetDepthCulling(DEPTH_CULLING culling) override;

  void SetCameraPosition(const CPoint& camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;
  void Project(float& x, float& y, float& z) override;

protected:
  virtual bool CurrentShaderHardwareClipIsPossible() = 0;
  virtual float CurrentShaderClipXFactor() const = 0;
  virtual float CurrentShaderClipXOffset() const = 0;
  virtual float CurrentShaderClipYFactor() const = 0;
  virtual float CurrentShaderClipYOffset() const = 0;

  int m_width = 0;
  int m_height = 0;
  GLint m_viewPort[4] = {};
};

