if(TARGET ${APP_NAME_LC}::OpenGl)
  set(SOURCES RenderSystemGL.cpp
              ScreenshotSurfaceGL.cpp
              GLShader.cpp
              ../opengl/RenderSystemGLBase.cpp)

  set(HEADERS RenderSystemGL.h
              ScreenshotSurfaceGL.h
              GLShader.h
              ../opengl/RenderSystemGLBase.h)

  core_add_library(rendering_gl)
endif()

