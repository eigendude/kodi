/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderSystemGLBase.h"

#include "MatrixGL.h"
#include "utils/MathUtils.h"

void CRenderSystemGLBase::SetViewPort(const CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  glScissor(static_cast<GLint>(viewPort.x1),
            static_cast<GLint>(m_height - viewPort.y1 - viewPort.Height()),
            static_cast<GLsizei>(viewPort.Width()),
            static_cast<GLsizei>(viewPort.Height()));
  glViewport(static_cast<GLint>(viewPort.x1),
             static_cast<GLint>(m_height - viewPort.y1 - viewPort.Height()),
             static_cast<GLsizei>(viewPort.Width()),
             static_cast<GLsizei>(viewPort.Height()));
  m_viewPort[0] = viewPort.x1;
  m_viewPort[1] = m_height - viewPort.y1 - viewPort.Height();
  m_viewPort[2] = viewPort.Width();
  m_viewPort[3] = viewPort.Height();
}

void CRenderSystemGLBase::GetViewPort(CRect& viewPort)
{
  if (!m_bRenderCreated)
    return;

  viewPort.x1 = m_viewPort[0];
  viewPort.y1 = m_height - m_viewPort[1] - m_viewPort[3];
  viewPort.x2 = m_viewPort[0] + m_viewPort[2];
  viewPort.y2 = viewPort.y1 + m_viewPort[3];
}

bool CRenderSystemGLBase::ScissorsCanEffectClipping()
{
  return CurrentShaderHardwareClipIsPossible();
}

CRect CRenderSystemGLBase::ClipRectToScissorRect(const CRect& rect)
{
  if (!CurrentShaderHardwareClipIsPossible())
    return CRect();

  float xFactor = CurrentShaderClipXFactor();
  float xOffset = CurrentShaderClipXOffset();
  float yFactor = CurrentShaderClipYFactor();
  float yOffset = CurrentShaderClipYOffset();
  return CRect(rect.x1 * xFactor + xOffset,
               rect.y1 * yFactor + yOffset,
               rect.x2 * xFactor + xOffset,
               rect.y2 * yFactor + yOffset);
}

void CRenderSystemGLBase::SetScissors(const CRect& rect)
{
  if (!m_bRenderCreated)
    return;
  GLint x1 = MathUtils::round_int(static_cast<double>(rect.x1));
  GLint y1 = MathUtils::round_int(static_cast<double>(rect.y1));
  GLint x2 = MathUtils::round_int(static_cast<double>(rect.x2));
  GLint y2 = MathUtils::round_int(static_cast<double>(rect.y2));
  glScissor(x1, m_height - y2, x2 - x1, y2 - y1);
}

void CRenderSystemGLBase::ResetScissors()
{
  SetScissors(CRect(0, 0, static_cast<float>(m_width), static_cast<float>(m_height)));
}

void CRenderSystemGLBase::SetDepthCulling(DEPTH_CULLING culling)
{
  if (culling == DEPTH_CULLING_OFF)
  {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
  }
  else if (culling == DEPTH_CULLING_BACK_TO_FRONT)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GEQUAL);
  }
  else if (culling == DEPTH_CULLING_FRONT_TO_BACK)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_GREATER);
  }
}

void CRenderSystemGLBase::SetCameraPosition(const CPoint& camera, int screenWidth, int screenHeight, float stereoFactor)
{
  if (!m_bRenderCreated)
    return;

  CPoint offset = camera - CPoint(screenWidth * 0.5f, screenHeight * 0.5f);

  float w = static_cast<float>(m_viewPort[2]) * 0.5f;
  float h = static_cast<float>(m_viewPort[3]) * 0.5f;

  glMatrixModview->LoadIdentity();
  glMatrixModview->Translatef(-(w + offset.x - stereoFactor), +(h + offset.y), 0);
  glMatrixModview->LookAt(0.0f, 0.0f, -2.0f * h, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f);
  glMatrixModview.Load();

  glMatrixProject->LoadIdentity();
  glMatrixProject->Frustum((-w - offset.x) * 0.5f, (w - offset.x) * 0.5f,
                           (-h + offset.y) * 0.5f, (h + offset.y) * 0.5f, h, 100 * h);
  glMatrixProject.Load();
}

void CRenderSystemGLBase::Project(float& x, float& y, float& z)
{
  GLfloat coordX, coordY, coordZ;
  if (CMatrixGL::Project(x, y, z, glMatrixModview.Get(), glMatrixProject.Get(), m_viewPort, &coordX, &coordY, &coordZ))
  {
    x = coordX;
    y = static_cast<float>(m_viewPort[1] + m_viewPort[3] - coordY);
    z = 0;
  }
}

