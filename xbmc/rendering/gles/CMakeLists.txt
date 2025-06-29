if(TARGET ${APP_NAME_LC}::OpenGLES)
  set(SOURCES RenderSystemGLES.cpp
              ScreenshotSurfaceGLES.cpp
              GLESShader.cpp
              ../opengl/RenderSystemGLBase.cpp)

  set(HEADERS RenderSystemGLES.h
              ScreenshotSurfaceGLES.h
              GLESShader.h
              ../opengl/RenderSystemGLBase.h)

  core_add_library(rendering_gles)
endif()
