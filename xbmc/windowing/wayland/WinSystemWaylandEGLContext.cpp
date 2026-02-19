/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWaylandEGLContext.h"

#include "Connection.h"
#include "ServiceBroker.h"
#include "application/ApplicationMessenger.h"
#include "cores/VideoPlayer/DVDCodecs/DVDFactoryCodec.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <chrono>
#include <cstdlib>

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

  // Never enable the vsync of the EGL implementation, we handle that ourselves
  // in WinSystemWayland
  m_eglContext.SetVSync(false);

  const EGLDisplay eglDisplay{m_eglContext.GetEGLDisplay()};
  const char* eglExtensions{eglQueryString(eglDisplay, EGL_EXTENSIONS)};
  CLog::LogF(LOGDEBUG, "Wayland EGL extensions: {}",
             eglExtensions != nullptr ? eglExtensions : "(null)");

  const char* glClearTestEnv{std::getenv("KODI_WAYLAND_GL_CLEAR_TEST")};
  m_glClearTestEnabled = glClearTestEnv != nullptr && std::string{glClearTestEnv} == "1";
  m_glClearTestStarted = false;
  m_glClearTestStart = {};
  m_glClearTestLoggedRenderer = false;

  // Debug run command: KODI_WAYLAND_GL_CLEAR_TEST=1 gamescope -f -b --expose-wayland -- ./kodi-wayland
  CLog::LogF(LOGDEBUG, "Wayland EGL GL clear test {} (KODI_WAYLAND_GL_CLEAR_TEST)",
             m_glClearTestEnabled ? "enabled" : "disabled");

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
    const EGLDisplay eglDisplay{m_eglContext.GetEGLDisplay()};
    const EGLSurface eglSurface{m_eglContext.GetEGLSurface()};
    const EGLContext eglContext{m_eglContext.GetEGLContext()};

    if (eglGetCurrentDisplay() != eglDisplay || eglGetCurrentSurface(EGL_DRAW) != eglSurface ||
        eglGetCurrentContext() != eglContext)
    {
      CLog::LogF(LOGWARNING,
                 "GL clear test: EGL context not current. display match={} draw surface match={} "
                 "context match={}",
                 eglGetCurrentDisplay() == eglDisplay,
                 eglGetCurrentSurface(EGL_DRAW) == eglSurface,
                 eglGetCurrentContext() == eglContext);
      if (!m_eglContext.BindContext())
      {
        CEGLUtils::Log(LOGERROR, "GL clear test: failed to bind EGL context");
      }
    }

    EGLint width{0};
    EGLint height{0};
    const bool queryWidthOk{eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &width) == EGL_TRUE};
    const EGLint queryWidthError{eglGetError()};
    const bool queryHeightOk{eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &height) == EGL_TRUE};
    const EGLint queryHeightError{eglGetError()};

    if (!queryWidthOk || !queryHeightOk)
    {
      CLog::LogF(LOGERROR,
                 "GL clear test: eglQuerySurface failed. widthOk={} widthErr=0x{:04x}, heightOk={} "
                 "heightErr=0x{:04x}, size={}x{}",
                 queryWidthOk, queryWidthError, queryHeightOk, queryHeightError, width, height);
    }

    const auto now{std::chrono::steady_clock::now()};
    static std::chrono::steady_clock::time_point s_lastZeroSizeLog{};
    if (width == 0 || height == 0)
    {
      if (now - s_lastZeroSizeLog >= std::chrono::seconds(1))
      {
        CLog::LogF(LOGDEBUG,
                   "GL clear test: EGL surface size unavailable ({}x{}), waiting for configure "
                   "before swap",
                   width, height);
        s_lastZeroSizeLog = now;
      }
      FinishFramePresentation();
      return;
    }

    if (!m_glClearTestStarted)
    {
      m_glClearTestStarted = true;
      m_glClearTestStart = now;
      CLog::LogF(LOGINFO, "GL clear test: started at EGL surface size {}x{}", width, height);
    }

    if (!m_glClearTestLoggedRenderer)
    {
      const char* vendor{reinterpret_cast<const char*>(glGetString(GL_VENDOR))};
      const char* renderer{reinterpret_cast<const char*>(glGetString(GL_RENDERER))};
      CLog::LogF(LOGINFO, "GL clear test: GL_VENDOR='{}' GL_RENDERER='{}'", vendor ? vendor : "(null)",
                 renderer ? renderer : "(null)");
      m_glClearTestLoggedRenderer = true;
    }

    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const EGLBoolean swapOk{eglSwapBuffers(eglDisplay, eglSurface)};
    const EGLint swapError{eglGetError()};
    CLog::LogF(LOGDEBUG, "GL clear test: eglSwapBuffers result={} eglError=0x{:04x} size={}x{}", swapOk,
               swapError, width, height);

    if (now - m_glClearTestStart >= std::chrono::seconds(2))
    {
      CLog::LogF(LOGINFO, "GL clear test: completed 2 seconds, requesting quit");
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_QUIT);
      m_glClearTestEnabled = false;
    }

    FinishFramePresentation();
    return;
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
