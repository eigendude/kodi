/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWaylandEGLContext.h"

#include "Connection.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "system_gl.h"

#include <EGL/eglext.h>

#include <cstdlib>
#include <string_view>

using namespace KODI::WINDOWING::WAYLAND;
using namespace KODI::WINDOWING::LINUX;

CWinSystemWaylandEGLContext::CWinSystemWaylandEGLContext()
  : CWinSystemEGL{EGL_PLATFORM_WAYLAND_EXT, "EGL_EXT_platform_wayland"}
{}

bool CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGLint renderableType, EGLint apiType)
{
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CDVDFactoryCodec::ClearHWAccels();
  if (!CWinSystemWaylandImpl::InitWindowSystem())
  {
    return false;
  }

  if (!m_eglContext.CreatePlatformDisplay(GetConnection()->GetDisplay(), GetConnection()->GetDisplay()))
  {
    return false;
  }

  if (!m_eglContext.InitializeDisplay(apiType))
  {
    return false;
  }

  if (!m_eglContext.ChooseConfig(renderableType))
  {
    return false;
  }

  return true;
}

bool CWinSystemWaylandEGLContext::CreateNewWindow(const std::string& name,
                                                  bool fullScreen,
                                                  RESOLUTION_INFO& res)
{
  if (!CWinSystemWaylandImpl::CreateNewWindow(name, fullScreen, res))
  {
    return false;
  }

  if (!CreateContext())
  {
    return false;
  }

  m_nativeWindow = wayland::egl_window_t{GetMainSurface(), GetBufferSize().Width(), GetBufferSize().Height()};

  // CWinSystemWayland::CreateNewWindow sets internal m_bufferSize
  // to the resolution that should be used for the initial surface size
  // - the compositor might want something other than the resolution given
  if (!m_eglContext.CreatePlatformSurface(
          m_nativeWindow.c_ptr(), reinterpret_cast<khronos_uintptr_t>(m_nativeWindow.c_ptr())))
  {
    return false;
  }

  if (!m_eglContext.BindContext())
  {
    return false;
  }

  const char* glClearTestEnv = std::getenv("KODI_WAYLAND_GL_CLEAR_TEST");
  m_glClearTestEnabled = glClearTestEnv != nullptr && std::string_view{glClearTestEnv} == "1";
  m_glClearTestInitialized = false;
  m_glClearTestLoggedRenderer = false;

  if (m_glClearTestEnabled)
  {
    CLog::Log(LOGINFO, "KODI_WAYLAND_GL_CLEAR_TEST enabled");
  }

  // Never enable the vsync of the EGL implementation, we handle that ourselves
  // in WinSystemWayland
  m_eglContext.SetVSync(false);

  return true;
}

bool CWinSystemWaylandEGLContext::DestroyWindow()
{
  m_eglContext.DestroySurface();
  m_nativeWindow = {};

  return CWinSystemWaylandImpl::DestroyWindow();
}

bool CWinSystemWaylandEGLContext::DestroyWindowSystem()
{
  m_eglContext.Destroy();

  return CWinSystemWaylandImpl::DestroyWindowSystem();
}

bool CWinSystemWaylandEGLContext::BindTextureUploadContext()
{
  return m_eglContext.BindTextureUploadContext();
}

bool CWinSystemWaylandEGLContext::UnbindTextureUploadContext()
{
  return m_eglContext.UnbindTextureUploadContext();
}

bool CWinSystemWaylandEGLContext::HasContext()
{
  return m_eglContext.HasContext();
}

CSizeInt CWinSystemWaylandEGLContext::GetNativeWindowAttachedSize()
{
  int width, height;
  m_nativeWindow.get_attached_size(width, height);
  return {width, height};
}

void CWinSystemWaylandEGLContext::SetContextSize(CSizeInt size)
{
  // Change EGL surface size if necessary
  if (GetNativeWindowAttachedSize() != size)
  {
    CLog::LogF(LOGDEBUG, "Updating egl_window size to {}x{}", size.Width(), size.Height());
    m_nativeWindow.resize(size.Width(), size.Height(), 0, 0);
  }
}

void CWinSystemWaylandEGLContext::PresentFrame(bool rendered)
{
  PrepareFramePresentation();

  if (m_glClearTestEnabled)
  {
    if (!m_glClearTestInitialized)
    {
      m_glClearTestStart = std::chrono::steady_clock::now();
      m_glClearTestInitialized = true;
    }

    const EGLDisplay display = m_eglContext.GetEGLDisplay();
    const EGLSurface surface = m_eglContext.GetEGLSurface();

    EGLint width{0};
    EGLint height{0};
    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height))
    {
      CLog::Log(LOGERROR, "GL clear test: eglQuerySurface failed, eglGetError={:#x}", eglGetError());
    }

    if (!m_glClearTestLoggedRenderer)
    {
      const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
      const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
      CLog::Log(LOGINFO, "GL clear test: GL_VENDOR='{}' GL_RENDERER='{}'", vendor ? vendor : "unknown",
                renderer ? renderer : "unknown");
      m_glClearTestLoggedRenderer = true;
    }

    CLog::Log(LOGINFO, "GL clear test: EGL surface size {}x{}", width, height);

    glViewport(0, 0, width, height);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!eglSwapBuffers(display, surface))
    {
      const EGLint error = eglGetError();
      CLog::Log(LOGERROR, "GL clear test: eglSwapBuffers failed, eglGetError={:#x}", error);
      throw std::runtime_error("eglSwapBuffers failed");
    }

    CLog::Log(LOGINFO, "GL clear test: eglSwapBuffers succeeded, eglGetError={:#x}", eglGetError());

    if (std::chrono::steady_clock::now() - m_glClearTestStart < std::chrono::seconds{2})
    {
      FinishFramePresentation();
      return;
    }
  }

  if (rendered)
  {
    if (!m_eglContext.TrySwapBuffers())
    {
      // For now we just hard fail if this fails
      // Theoretically, EGL_CONTEXT_LOST could be handled, but it needs to be checked
      // whether egl implementations actually use it (mesa does not)
      CEGLUtils::Log(LOGERROR, "eglSwapBuffers failed");
      throw std::runtime_error("eglSwapBuffers failed");
    }
    // eglSwapBuffers() (hopefully) calls commit on the surface and flushes
    // ... well mesa does anyway
  }
  else
  {
    // For presentation feedback: Get notification of the next vblank even
    // when contents did not change
    GetMainSurface().commit();
    // Make sure it reaches the compositor
    GetConnection()->GetDisplay().flush();
  }

  FinishFramePresentation();
}
