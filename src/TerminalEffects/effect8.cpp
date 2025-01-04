#include "precompiled.hpp"

#include "effect.hpp"

#include "shader_sources.h"

namespace {

#ifdef _DEBUG
  void APIENTRY debug__callback(
      GLenum source           // Where the error came from
    , GLenum type             // The type of error
    , GLuint id               // Error ID
    , GLenum severity         // How serious the error is
    , GLsizei length          // Length of the error message
    , GLchar const* message   // The error message itself
    , void const* userParam   // User-provided data (unused)
    ) {
      std::printf(message);
      std::printf("\n");
  }
  char debug__log[0xFFFF];  // 65535 characters
#endif

  struct t_shader {
    GLuint  program           ;
    GLint   loc__time         ;
    GLint   loc__resolution   ;
    GLint   loc__fixed_radius2;
    GLint   loc__min_radius2  ;
    GLint   loc__folding_limit;
    GLint   loc__scale        ;
  };

  t_shader shader;


  void init_effect() {
    auto fp__glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv");
    auto fp__glGetUniformLocation   = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
    assert(fp__glCreateShaderProgramv);
    assert(fp__glGetUniformLocation);

    GLchar const* fragment_shaders[] = { fragment_shader__source };
    shader.program = fp__glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, fragment_shaders);
    assert(shader.program > 0);

    shader.loc__time          = fp__glGetUniformLocation(shader.program, "time");
    shader.loc__resolution    = fp__glGetUniformLocation(shader.program, "resolution");
    shader.loc__fixed_radius2 = fp__glGetUniformLocation(shader.program, "fixed_radius2");
    shader.loc__min_radius2   = fp__glGetUniformLocation(shader.program, "min_radius2");
    shader.loc__folding_limit = fp__glGetUniformLocation(shader.program, "folding_limit");
    shader.loc__scale         = fp__glGetUniformLocation(shader.program, "scale");
    assert(shader.loc__time > -1);
    assert(shader.loc__resolution > -1);
    assert(shader.loc__fixed_radius2 > -1);
    assert(shader.loc__min_radius2 > -1);
    assert(shader.loc__folding_limit > -1);
    assert(shader.loc__scale > -1);


#ifdef _DEBUG
    auto glGetProgramInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
    glGetProgramInfoLog(shader.program, sizeof(debug__log), NULL, debug__log);
    printf(debug__log);
#endif
  }

  void deinit_effect() {
  }

  void draw_effect(
      sixel_screen const &  sixel_screen
    , GLfloat               time
    , GLfloat               fixed_radius2
    , GLfloat               min_radius2
    , GLfloat               folding_limit
    , GLfloat               scale
    ) {
    auto fp__glUniform1f  = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
    auto fp__glUniform2f  = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
    auto fp__glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
    assert(fp__glUniform1f);
    assert(fp__glUniform1f);
    assert(fp__glUseProgram);

    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    fp__glUseProgram(shader.program);

    assert(shader.loc__time > -1);
    assert(shader.loc__resolution > -1);

    fp__glUniform1f(shader.loc__time, time);
    fp__glUniform2f(
      shader.loc__resolution
    , static_cast<GLfloat>(sixel_screen.viewport__width)
    , static_cast<GLfloat>(sixel_screen.viewport__height)
    );
    fp__glUniform1f(shader.loc__fixed_radius2 , fixed_radius2);
    fp__glUniform1f(shader.loc__min_radius2   , min_radius2);
    fp__glUniform1f(shader.loc__folding_limit , folding_limit);
    fp__glUniform1f(shader.loc__scale         , scale);

    glRects(-1, -1, 1, 1);

    fp__glUseProgram(currentProgram);
  }
}

void effect8(float time, std::size_t beat__start, std::size_t beat__end, screen & screen, sixel_screen const & sixel_screen) {
    draw_effect(
        sixel_screen
      , time
      , 1.9
      , 0.2
      , 1.
      , -2.1
      );

}
