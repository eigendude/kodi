/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPRendererOpenGL.h"

#include "cores/RetroPlayer/buffers/RenderBufferOpenGL.h"
#include "cores/RetroPlayer/buffers/RenderBufferPoolOpenGL.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/shaders/gl/ShaderPresetGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGL.h"
#include "cores/RetroPlayer/shaders/gl/ShaderTextureGLRef.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <cstddef>

#define RP_GL_BLACKSCREEN_FIXES 1

using namespace KODI;
using namespace RETRO;

#if defined(TARGET_DARWIN_OSX)
namespace
{
#if !defined(GL_DRAW_FRAMEBUFFER_BINDING)
#define GL_DRAW_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING
#endif

void DumpGLState(const char* tag)
{
  GLint currentProgram{0};
  GLint vao{0};
  GLint arrayBuffer{0};
  GLint elementArrayBuffer{0};
  GLint activeTexture{0};
  GLint texture2D{0};
  GLint framebuffer{0};
  GLint viewport[4] = {0, 0, 0, 0};

  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
  glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementArrayBuffer);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2D);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
  glGetIntegerv(GL_VIEWPORT, viewport);

  const bool blendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
  const bool depthEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
  const bool scissorEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
  const GLenum lastError = glGetError();

  CLog::Log(LOGDEBUG,
            "RPRendererOpenGL: {} program={}, vao={}, arrayBuffer={}, elementArrayBuffer={}, "
            "activeTexture={}, texture2D={}, drawFbo={}, viewport=[{}, {}, {}, {}], "
            "blend={}, depth={}, scissor={}, glError=0x{:x}",
            tag, currentProgram, vao, arrayBuffer, elementArrayBuffer, activeTexture, texture2D,
            framebuffer, viewport[0], viewport[1], viewport[2], viewport[3], blendEnabled,
            depthEnabled, scissorEnabled, static_cast<unsigned int>(lastError));
}

void EnsureVaoBoundForShaderValidation()
{
  // macOS OpenGL core profile requires a VAO bound for program validation.
  static thread_local GLuint s_validationVao{0};

  GLint boundVao{0};
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVao);
  if (boundVao != 0)
    return;

  if (s_validationVao == 0)
    glGenVertexArrays(1, &s_validationVao);

  if (s_validationVao != 0)
    glBindVertexArray(s_validationVao);
}
} // namespace
#endif

// --- CRendererFactoryOpenGL --------------------------------------------------

std::string CRendererFactoryOpenGL::RenderSystemName() const
{
  return "OpenGL";
}

CRPBaseRenderer* CRendererFactoryOpenGL::CreateRenderer(
    const CRenderSettings& settings,
    CRenderContext& context,
    std::shared_ptr<IRenderBufferPool> bufferPool)
{
#if defined(TARGET_DARWIN_OSX)
  EnsureVaoBoundForShaderValidation();
#endif

  return new CRPRendererOpenGL(settings, context, std::move(bufferPool));
}

RenderBufferPoolVector CRendererFactoryOpenGL::CreateBufferPools(CRenderContext& context)
{
  return {std::make_shared<CRenderBufferPoolOpenGL>()};
}

// --- CRPRendererOpenGL -------------------------------------------------------

CRPRendererOpenGL::CRPRendererOpenGL(const CRenderSettings& renderSettings,
                                     CRenderContext& context,
                                     std::shared_ptr<IRenderBufferPool> bufferPool)
  : CRPBaseRenderer(renderSettings, context, std::move(bufferPool))
{
  m_context.CaptureStateBlock();

  // Initialize CRPBaseRenderer
  m_shaderPreset = std::make_unique<SHADER::CShaderPresetGL>(m_context);

  // Initialize CRPRendererOpenGL
  m_clearColor = m_context.UseLimitedColor() ? (16.0f / 0xff) : 0.0f;

  m_context.ApplyStateBlock();
}

CRPRendererOpenGL::~CRPRendererOpenGL()
{
  if (m_glInitialized)
  {
    glDeleteBuffers(1, &m_mainIndexVBO);
    glDeleteBuffers(1, &m_mainVertexVBO);
    glDeleteBuffers(1, &m_blackbarsVertexVBO);

    glDeleteVertexArrays(1, &m_mainVAO);
    glDeleteVertexArrays(1, &m_blackbarsVAO);
  }
}

void CRPRendererOpenGL::InitializeGLResources()
{
  // Lazily initialize GL resources on the render thread with a current context.
  if (m_glInitialized)
    return;

  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLint posLoc = m_context.GUIShaderGetPos();
  GLint tex0Loc = m_context.GUIShaderGetCoord0();

  const GLubyte idx[4] = {0, 1, 3, 2}; // Determines order of triangle strip

  // Set up main screen VAO/VBO
  glGenVertexArrays(1, &m_mainVAO);
  glBindVertexArray(m_mainVAO);

  glGenBuffers(1, &m_mainVertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);

  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, x)));
  glEnableVertexAttribArray(posLoc);
  glVertexAttribPointer(tex0Loc, 2, GL_FLOAT, 0, sizeof(PackedVertex),
                        reinterpret_cast<const GLvoid*>(offsetof(PackedVertex, u1)));
  glEnableVertexAttribArray(tex0Loc);

  glGenBuffers(1, &m_mainIndexVBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mainIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 4, idx, GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);

  GLint blackbarsPosLoc = m_context.GUIShaderGetPos();

  // Set up black bars VAO/VBO
  glGenVertexArrays(1, &m_blackbarsVAO);
  glBindVertexArray(m_blackbarsVAO);

  glGenBuffers(1, &m_blackbarsVertexVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_blackbarsVertexVBO);

  glVertexAttribPointer(blackbarsPosLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Svertex), 0);
  glEnableVertexAttribArray(blackbarsPosLoc);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();

  m_glInitialized = true;
}

void CRPRendererOpenGL::RenderInternal(bool clear, uint8_t alpha)
{
  InitializeGLResources();

  if (clear)
  {
    if (alpha == 255)
      DrawBlackBars();
    else
      ClearBackBuffer();
  }

  Render(alpha);

  glEnable(GL_BLEND);
}

void CRPRendererOpenGL::FlushInternal()
{
  if (!m_bConfigured)
    return;

  glFinish();
}

void CRPRendererOpenGL::InitializeRenderer()
{
  InitializeGLResources();
}

bool CRPRendererOpenGL::Supports(RENDERFEATURE feature) const
{
  return feature == RENDERFEATURE::STRETCH || feature == RENDERFEATURE::ZOOM ||
         feature == RENDERFEATURE::PIXEL_RATIO || feature == RENDERFEATURE::ROTATION;
}

bool CRPRendererOpenGL::SupportsScalingMethod(SCALINGMETHOD method)
{
  return method == SCALINGMETHOD::AUTO || method == SCALINGMETHOD::NEAREST ||
         method == SCALINGMETHOD::LINEAR;
}

void CRPRendererOpenGL::ClearBackBuffer()
{
  glClearColor(m_clearColor, m_clearColor, m_clearColor, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void CRPRendererOpenGL::DrawBlackBars()
{
  InitializeGLResources();

  glDisable(GL_BLEND);

  m_context.EnableGUIShader(GL_SHADER_METHOD::DEFAULT);

  GLint uniColLoc = m_context.GUIShaderGetUniCol();

  glUniform4f(uniColLoc, m_clearColor / 255.0f, m_clearColor / 255.0f, m_clearColor / 255.0f, 1.0f);

  Svertex vertices[24];
  GLubyte count = 0;

  // top quad
  if (m_rotatedDestCoords[0].y > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = 0.0;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = 0;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[0].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[0].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // bottom quad
  if (m_rotatedDestCoords[2].y < m_context.GetScreenHeight())
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[2].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[2].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_context.GetScreenHeight();
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_context.GetScreenHeight();
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // left quad
  if (m_rotatedDestCoords[0].x > 0.0f)
  {
    GLubyte quad = count;
    vertices[quad].x = 0.0;
    vertices[quad].y = m_rotatedDestCoords[0].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_rotatedDestCoords[0].x;
    vertices[quad + 1].y = m_rotatedDestCoords[0].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_rotatedDestCoords[3].x;
    vertices[quad + 2].y = m_rotatedDestCoords[3].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = 0;
    vertices[quad + 4].y = m_rotatedDestCoords[3].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  // right quad
  if (m_rotatedDestCoords[2].x < m_context.GetScreenWidth())
  {
    GLubyte quad = count;
    vertices[quad].x = m_rotatedDestCoords[1].x;
    vertices[quad].y = m_rotatedDestCoords[1].y;
    vertices[quad].z = 0;
    vertices[quad + 1].x = m_context.GetScreenWidth();
    vertices[quad + 1].y = m_rotatedDestCoords[1].y;
    vertices[quad + 1].z = 0;
    vertices[quad + 2].x = m_context.GetScreenWidth();
    vertices[quad + 2].y = m_rotatedDestCoords[2].y;
    vertices[quad + 2].z = 0;
    vertices[quad + 3] = vertices[quad + 2];
    vertices[quad + 4].x = m_rotatedDestCoords[1].x;
    vertices[quad + 4].y = m_rotatedDestCoords[2].y;
    vertices[quad + 4].z = 0;
    vertices[quad + 5] = vertices[quad];
    count += 6;
  }

  glBindVertexArray(m_blackbarsVAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_blackbarsVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Svertex) * count, &vertices[0], GL_DYNAMIC_DRAW);

  glDrawArrays(GL_TRIANGLES, 0, count);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}

void CRPRendererOpenGL::Render(uint8_t alpha)
{
  InitializeGLResources();

  auto renderBuffer = static_cast<CRenderBufferOpenGL*>(m_renderBuffer);
  if (renderBuffer == nullptr)
    return;

  // Drop cached textures if target size is changed
  if (m_fullDestWidth != m_lastTargetWidth || m_fullDestHeight != m_lastTargetHeight)
  {
    m_RBTexturesMap.clear();
    m_lastTargetWidth = m_fullDestWidth;
    m_lastTargetHeight = m_fullDestHeight;
  }

  RenderBufferTextures* rbTextures;

  const auto it = m_RBTexturesMap.find(renderBuffer);
  if (it != m_RBTexturesMap.end())
  {
    rbTextures = it->second.get();
  }
  else
  {
    rbTextures = new RenderBufferTextures{
        // Source texture
        std::make_shared<SHADER::CShaderTextureGLRef>(
            static_cast<unsigned int>(renderBuffer->GetWidth()),
            static_cast<unsigned int>(renderBuffer->GetHeight()), renderBuffer->TextureID()),
        // Target texture
        std::make_shared<SHADER::CShaderTextureGL>(static_cast<unsigned int>(m_fullDestWidth),
                                                   static_cast<unsigned int>(m_fullDestHeight)),
    };

    std::shared_ptr<SHADER::CShaderTextureGL> target = rbTextures->target;

    target->CreateTextureObject(); // Create new internal texture

    const GLfloat blackBorder[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    glBindTexture(m_textureTarget, target->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#if RP_GL_BLACKSCREEN_FIXES && defined(TARGET_DARWIN_OSX)
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
#endif
    glTexParameterfv(m_textureTarget, GL_TEXTURE_BORDER_COLOR, blackBorder);

    // Force alpha to 1, because shader can leave it undefined
    glTexParameteri(m_textureTarget, GL_TEXTURE_SWIZZLE_A, GL_ONE);

    glTexImage2D(m_textureTarget, 0, GL_RGBA8, target->GetWidth(), target->GetHeight(), 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, nullptr);

    m_RBTexturesMap.emplace(renderBuffer, rbTextures);
  }

  std::shared_ptr<SHADER::CShaderTextureGLRef> source = rbTextures->source;
  std::shared_ptr<SHADER::CShaderTextureGL> target = rbTextures->target;

  static GLuint s_lastLoggedTextureId{0};
  if (renderBuffer->TextureID() != s_lastLoggedTextureId)
  {
    s_lastLoggedTextureId = renderBuffer->TextureID();
    CLog::Log(
        LOGDEBUG,
        "RPRendererOpenGL: Render buffer sourceTex={}, source={}x{}, target={}x{}, fullDest={}x{}, "
        "screen={}x{}, rect=[{:.3f},{:.3f}]-[{:.3f},{:.3f}]",
        renderBuffer->TextureID(), renderBuffer->GetWidth(), renderBuffer->GetHeight(),
        target->GetWidth(), target->GetHeight(), m_fullDestWidth, m_fullDestHeight,
        m_context.GetScreenWidth(), m_context.GetScreenHeight(), m_sourceRect.x1, m_sourceRect.y1,
        m_sourceRect.x2, m_sourceRect.y2);
  }

  Updateshaders();

  // Use video shader preset
  if (m_bUseShaderPreset)
  {
    GLint filter = GL_NEAREST;
    if (m_shaderPreset->GetPasses().front().filterType == SHADER::FilterType::LINEAR)
      filter = GL_LINEAR;

    glBindTexture(m_textureTarget, source->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (!m_shaderPreset->RenderUpdate(*source, *target))
    {
      m_bShadersNeedUpdate = false;
      m_bUseShaderPreset = false;
    }

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, target->GetTextureID());
  }
  else
  {
    GLint filter = GL_NEAREST;
    if (GetRenderSettings().VideoSettings().GetScalingMethod() == SCALINGMETHOD::LINEAR)
      filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0); // GUI shader samples from texture unit 0
    glBindTexture(m_textureTarget, source->GetTextureID());
    glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  if (alpha < 255)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_BLEND);
  }

  // Use GUI shader
  m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);

  GLint currentProgram{0};
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
#if RP_GL_BLACKSCREEN_FIXES && defined(TARGET_DARWIN_OSX)
  if (currentProgram == 0)
  {
    CLog::Log(LOGERROR,
              "RPRendererOpenGL: GL_CURRENT_PROGRAM is 0 after EnableGUIShader(TEXTURE), retrying");
    m_context.EnableGUIShader(GL_SHADER_METHOD::TEXTURE);
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram == 0)
    {
      CLog::Log(LOGERROR,
                "RPRendererOpenGL: aborting draw because GL_CURRENT_PROGRAM remains 0 after retry");
      m_context.DisableGUIShader();
      return;
    }
  }
#endif

#if defined(TARGET_DARWIN_OSX)
  static bool s_loggedAfterShaderEnable{false};
  if (!s_loggedAfterShaderEnable)
  {
    DumpGLState("after EnableGUIShader(TEXTURE)");
    s_loggedAfterShaderEnable = true;
  }
#endif

  GLint uniColLoc = m_context.GUIShaderGetUniCol();
  GLint depthLoc = m_context.GUIShaderGetDepth();

  // Setup color values
  GLubyte col[4];
  const uint32_t color = (alpha << 24) | 0xFFFFFF;
  col[0] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::R, color);
  col[1] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::G, color);
  col[2] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::B, color);
  col[3] = UTILS::GL::GetChannelFromARGB(UTILS::GL::ColorChannel::A, color);

  glUniform4f(uniColLoc, (col[0] / 255.0f), (col[1] / 255.0f), (col[2] / 255.0f),
              (col[3] / 255.0f));
  glUniform1f(depthLoc, -1.0f);

  // Setup destination rectangle
  CRect rect = m_sourceRect;
  rect.x1 /= renderBuffer->GetWidth();
  rect.x2 /= renderBuffer->GetWidth();
  rect.y1 /= renderBuffer->GetHeight();
  rect.y2 /= renderBuffer->GetHeight();

  PackedVertex vertex[4];

  // Setup vertex position values
  for (unsigned int i = 0; i < 4; i++)
  {
    vertex[i].x = m_rotatedDestCoords[i].x;
    vertex[i].y = m_rotatedDestCoords[i].y;
    vertex[i].z = 0.0f;
  }

  // Setup texture coordinates
  vertex[0].u1 = vertex[3].u1 = rect.x1;
  vertex[0].v1 = vertex[1].v1 = rect.y1;
  vertex[1].u1 = vertex[2].u1 = rect.x2;
  vertex[2].v1 = vertex[3].v1 = rect.y2;

#if RP_GL_BLACKSCREEN_FIXES && defined(TARGET_DARWIN_OSX)
  if (source->GetTextureID() == 0)
  {
    CLog::Log(LOGERROR, "RPRendererOpenGL: source texture id is 0, skipping draw");
    m_context.DisableGUIShader();
    return;
  }

  GLint viewport[4] = {0, 0, 0, 0};
  glGetIntegerv(GL_VIEWPORT, viewport);
  if (viewport[2] == 0 || viewport[3] == 0)
  {
    CLog::Log(LOGERROR,
              "RPRendererOpenGL: viewport is {}x{}, forcing viewport to screen {}x{}",
              viewport[2], viewport[3], m_context.GetScreenWidth(), m_context.GetScreenHeight());
    glViewport(0, 0, m_context.GetScreenWidth(), m_context.GetScreenHeight());
  }

  GLint drawFbo{0};
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
  static bool s_loggedUnexpectedFbo{false};
  if (drawFbo != 0 && !s_loggedUnexpectedFbo)
  {
    CLog::Log(LOGDEBUG,
              "RPRendererOpenGL: draw framebuffer binding is {} (expected 0 for default framebuffer)",
              drawFbo);
    s_loggedUnexpectedFbo = true;
  }

  const bool depthWasEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
  const bool cullWasEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
  const bool scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
  if (depthWasEnabled || cullWasEnabled || scissorWasEnabled)
  {
    static bool s_loggedForcedState{false};
    if (!s_loggedForcedState)
    {
      CLog::Log(LOGDEBUG,
                "RPRendererOpenGL: forcing GL state depth={}, cull={}, scissor={} to disabled before draw",
                depthWasEnabled, cullWasEnabled, scissorWasEnabled);
      s_loggedForcedState = true;
    }
  }
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_SCISSOR_TEST);
#endif

#if defined(TARGET_DARWIN_OSX)
  static bool s_loggedBeforeBindMainVao{false};
  if (!s_loggedBeforeBindMainVao)
  {
    DumpGLState("before bind main VAO");
    s_loggedBeforeBindMainVao = true;
  }
#endif

#if RP_GL_BLACKSCREEN_FIXES && defined(TARGET_DARWIN_OSX)
  if (m_mainVAO == 0)
  {
    CLog::Log(LOGERROR, "RPRendererOpenGL: m_mainVAO is 0, skipping draw");
    m_context.DisableGUIShader();
    return;
  }
#endif

  glBindVertexArray(m_mainVAO);

  glBindBuffer(GL_ARRAY_BUFFER, m_mainVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(PackedVertex) * 4, &vertex[0], GL_DYNAMIC_DRAW);

#if RP_GL_BLACKSCREEN_FIXES && defined(TARGET_DARWIN_OSX)
  GLint boundTexture{0};
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
  if (boundTexture == 0)
  {
    CLog::Log(LOGERROR, "RPRendererOpenGL: GL_TEXTURE_BINDING_2D is 0 before draw, skipping draw");
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_context.DisableGUIShader();
    return;
  }
#endif

  const GLenum preDrawError = glGetError();
  if (preDrawError != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "RPRendererOpenGL: GL error before draw: 0x{:x}",
              static_cast<unsigned int>(preDrawError));
#if defined(TARGET_DARWIN_OSX)
    DumpGLState("before draw");
#endif
  }

  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, nullptr);

  const GLenum postDrawError = glGetError();
  if (postDrawError != GL_NO_ERROR)
  {
    CLog::Log(LOGERROR, "RPRendererOpenGL: GL error after draw: 0x{:x}",
              static_cast<unsigned int>(postDrawError));
#if defined(TARGET_DARWIN_OSX)
    DumpGLState("after draw");
#endif
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_context.DisableGUIShader();
}
