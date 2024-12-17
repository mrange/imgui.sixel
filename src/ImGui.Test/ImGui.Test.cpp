
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>
#include <gl/GL.h>
#include "glext.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl2.h"


#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <format>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "shader_sources.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define CHECK(expr, ret, message)                                   \
  if (!(expr)) {                                                    \
    MessageBox(nullptr, message, "Error from ImGui.Test", MB_OK);   \
    return ret;                                                     \
  }

namespace {
  template<typename TOnExit>
  struct on_exit__impl {
    on_exit__impl ()                                = delete;
    on_exit__impl (on_exit__impl const &)           = delete;
    on_exit__impl& operator=(on_exit__impl const &) = delete;

    on_exit__impl (TOnExit && on_exit)
      : suppress (false)
      , on_exit  (std::move(on_exit)) {
    }

    on_exit__impl (on_exit__impl && impl)
      : suppress (impl.suppress)
      , on_exit  (std::move(impl.on_exit)) {
      impl.suppress = true;
    }

    ~on_exit__impl() noexcept {
      if(!suppress) {
        suppress = true;
        on_exit();
      }
    }

    bool suppress;
  private:
    TOnExit on_exit;
  };

  template<typename TOnExit>
  auto on_exit(TOnExit && on_exit) {
    return on_exit__impl<std::decay_t<TOnExit>>(std::move(on_exit));
  }

  struct ticks__timer {
    inline ticks__timer(DWORD64 * p) noexcept 
      : p(p)
      , b(__rdtsc()) {
    }

    inline ~ticks__timer() noexcept {
      if (p) {
        *p += __rdtsc()-b;
      }
    }
  private:
    DWORD64 * p;
    DWORD64   b;
  };

  using ABGR = std::uint32_t;

  char const        sixel_base      = 63;
  std::size_t const desired__width  = 800;
  std::size_t const desired__height = 600;
  char const windows_class_name[]   = "ImGui.Test";

  std::size_t viewport__width       = desired__width ;
  std::size_t viewport__height      = desired__height;
  std::string const sixel__prelude  = "\x1B[?25l\x1B[H\x1B[12t\x1BP7;1;q";
  std::string const sixel__epilogue = "\x1B\\";

  std::array<std::string, 256> generate_col_selectors() {
    std::array<std::string,256> res;

    for (std::size_t i=0; i < res.size(); ++i) {
      res[i] = std::format(
        "#{}"
      , i);
    }

    return res;
  }
  std::array<std::string, 256> const sixel__col_selectors = generate_col_selectors();

  std::array<std::string, 2048> generate_reps() {
    std::array<std::string,2048> res;

    for (std::size_t i=0; i < res.size(); ++i) {
      res[i] = std::format(
        "!{}"
      , i);
    }

    return res;
  }
  std::array<std::string, 2048> const sixel__reps = generate_reps();

  std::string generate_palette() {
    // 3 bits for red
    // 3 bits for green
    // 2 bits for blue

    std::string palette;
    for (auto red = 0; red < 8; ++red) {
      for (auto green = 0; green < 8; ++green) {
        for (auto blue = 0; blue < 4; ++blue) {
          auto idx = (red << 5)|(green<<2)|blue;
          palette.append(std::format(
            "#{};2;{};{};{}"
          , idx
          , static_cast<int>(std::round(red  *100.0/7.0))
          , static_cast<int>(std::round(green*100.0/7.0))
          , static_cast<int>(std::round(blue *100.0/3.0))
          ));
        }
      }
    }
    return palette;
  }
  std::string const sixel__palette = generate_palette();

  struct ticks__write_pixel_as_sixels {
    DWORD64 total       ;
    DWORD64 sixel_pixel ;
    DWORD64 sixel_buffer;
    DWORD64 used_colors ;
    DWORD64 write_file  ;
  };

  void write_pixel_as_sixels(
      HANDLE                        hstdout
    , std::size_t                   width
    , std::size_t                   height
    , std::vector<ABGR> const &     pixels
    , std::vector<GLubyte> &        sixel_pixels
    , std::string &                 buffer
    , ticks__write_pixel_as_sixels &ticks
    ) {
    ticks__timer time__total(&ticks.total);

    if (width > sixel__reps.size()) {
      return;
    }

    auto total_size = width*height;
    assert(hstdout != INVALID_HANDLE_VALUE);
    assert(pixels.size() > 0);
    assert(pixels.size() == total_size);

    sixel_pixels.resize(total_size);


    {
      ticks__timer time__sixel_pixel(&ticks.sixel_pixel);
      // Convert from ABGR to sixel pixels
      for (std::size_t y = 0; y < height; ++y) {
        // Flip y axis
        auto from_y_off = (height - y - 1)*width;
        auto to_y_off   = y*width;
        for (std::size_t x = 0; x < width; ++x) {
          auto from_i = x+from_y_off;
          auto to_i   = x+to_y_off;
          auto abgr = pixels[from_i];
          // 3 bits for red
          // 3 bits for green
          // 2 bits for blue
          auto red          = (abgr           )&(0x7<<5);
          auto green        = (abgr >> (8+5-2))&(0x7<<2);
          auto blue         = (abgr >> (8+8+6))&(0x3   );
          auto sixel_pixel  = red|green|blue;
          sixel_pixels[to_i]   = sixel_pixel;
        }
      }
    }
  

    buffer.clear();

    buffer.append(sixel__prelude);

    buffer.append(sixel__palette);

    {
      ticks__timer time__sixel_pixel(&ticks.sixel_buffer);
      bool used_colors[256];
      for (std::size_t y6 = 0; y6 < height; y6 += 6) {
        auto y6_off = y6*width;
        auto rem = std::min<std::size_t>(6, height - y6);
        {
          ticks__timer time__used_colors(&ticks.used_colors);
          // Find colors used in this group of 6 lines
          memset(used_colors, 0, sizeof(used_colors));
          for (std::size_t x = 0; x < width; ++x) {
            // TODO: This would be more effective loop more over y then x rather
            //  than x then y
            auto y_off = y6_off;
            for (std::size_t i = 0; i < rem; ++i) {
              auto sixel_pixel = sixel_pixels[x + y_off];
              used_colors[sixel_pixel] = true;
              y_off += width;
            }
          }
        }

        {
          // Convert colors in use to sixels
          for (std::size_t current_col = 0; current_col < 256; ++current_col) {
            if (!used_colors[current_col]) {
              continue;
            }

            buffer.append(sixel__col_selectors[current_col]);

            auto repeated_sixel     = sixel_base;
            std::size_t sixel_reps  = 0;

            for (std::size_t x = 0; x < width; ++x) {
              GLubyte sixel = 0;
              auto y_off = y6_off;
              for (std::size_t i = 0; i < rem; ++i) {
                auto sixel_pixel = sixel_pixels[x + y_off];
                if (current_col == sixel_pixel) {
                  sixel |= 1U << i;
                }
                y_off += width;
              }
              char sixel_char = sixel_base + sixel;

              // Handle run-length encoding
              if (repeated_sixel == sixel_char) {
                ++sixel_reps;
              } else {
                // Output previous run
                if (sixel_reps > 3) {
                  // Use RLE for runs longer than 3

                  buffer.append(sixel__reps[sixel_reps]);
                  buffer.push_back(repeated_sixel);
                } else {
                  // Direct output for short runs
                  buffer.append(sixel_reps, repeated_sixel);
                }

                repeated_sixel  = sixel_char;
                sixel_reps      = 1;
              }

            }

            // Output final run if not empty
            if (repeated_sixel != sixel_base) {
              if (sixel_reps > 3) {
                // Use RLE for runs longer than 3

                buffer.append(sixel__reps[sixel_reps]);
                buffer.push_back(repeated_sixel);
              } else {
                // Direct output for short runs
                buffer.append(sixel_reps, repeated_sixel);
              }
            }

            buffer.push_back('$');  // Return to start of line
          }
          buffer.push_back('-');  // Move to next row
        }

      }
    }

    buffer.append(sixel__epilogue);

    if (buffer.size() > 0) {
      ticks__timer time__sixel_pixel(&ticks.write_file);
      auto writeOk = WriteFile(
        hstdout
      , &buffer.front()
      , static_cast<DWORD>(buffer.size())
      , nullptr
      , nullptr
      );
      auto err = GetLastError();
      assert(writeOk);
    }
  }

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

    GLchar const* fragment_shaders[] = { fragment_shader_source };
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
      GLfloat time
    , GLfloat fixed_radius2
    , GLfloat min_radius2
    , GLfloat folding_limit
    , GLfloat scale
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
    , static_cast<GLfloat>(viewport__width)
    , static_cast<GLfloat>(viewport__height)
    );
    fp__glUniform1f(shader.loc__fixed_radius2 , fixed_radius2);
    fp__glUniform1f(shader.loc__min_radius2   , min_radius2);
    fp__glUniform1f(shader.loc__folding_limit , folding_limit);
    fp__glUniform1f(shader.loc__scale         , scale);

    glRects(-1, -1, 1, 1);

    fp__glUseProgram(currentProgram);
  }

  LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
      return true;

    switch (uMsg) {
    case WM_SIZE: {
      int width = LOWORD(lParam);
      int height = HIWORD(lParam);

      glViewport(0, 0, width, height);

      viewport__width   = width;
      viewport__height  = height;

      return 0;
    }
    case WM_CLOSE:
      ::PostQuitMessage(0);
      return 0;
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  PIXELFORMATDESCRIPTOR pixel_format_descriptor = {
    sizeof(PIXELFORMATDESCRIPTOR)
  , 1
  , PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER
  , PFD_TYPE_RGBA
  , 32
  , 0
  , 0
  , 0
  , 0
  , 0
  , 0
  , 8
  , 0
  , 0
  , 0
  , 0
  , 0
  , 0
  , 24
  , 8
  , 0
  , PFD_MAIN_PLANE
  , 0
  , 0
  , 0
  , 0
  };

  WNDCLASSEX wnd_class_ex = {
    sizeof(WNDCLASSEX)
  , CS_HREDRAW | CS_VREDRAW | CS_OWNDC
  , WindowProc
  , 0
  , 0
  , 0
  , LoadIcon(nullptr, IDI_APPLICATION)
  , LoadCursor(nullptr, IDC_ARROW)
  , reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)
  , nullptr
  , windows_class_name
  , nullptr
  };

}

int main() {
  //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
  auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
  CHECK(hstdout != INVALID_HANDLE_VALUE, 1, "GetStdHandle Failed");
  
  auto hinstance = GetModuleHandle(0);
  CHECK(hinstance, 1, "GetModuleHandle Failed");

  wnd_class_ex.hInstance = hinstance;
  auto dw_style = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_POPUP;

  auto result__register_class = RegisterClassEx(&wnd_class_ex);
  CHECK(result__register_class, 1, "Window Registration Failed");

  auto on_exit__unregister_class = on_exit([hinstance]{ UnregisterClass(windows_class_name, hinstance); });

  RECT window_rect = { 0, 0, desired__width, desired__height };
  auto result__rect= AdjustWindowRect(&window_rect, dw_style, 0);
  CHECK(result__rect, 1, "AdjustWindowRect Failed");

  auto hwnd = CreateWindowEx(
    0                                   // Extended style
  , windows_class_name                  // Window class name
  , windows_class_name                  // Window title
  , dw_style                            // Window style
  , CW_USEDEFAULT                       // StartPosition X
  , CW_USEDEFAULT                       // StartPosition Y
  , window_rect.right-window_rect.left  // Width
  , window_rect.bottom-window_rect.top  // Height
  , nullptr                             // Parent
  , nullptr                             // Menu
  , hinstance                           // Instance
  , nullptr                             // additional params
  );

  CHECK(hwnd, 1, "Window Creation Failed");
  auto on_exit__destroy_window = on_exit([hwnd]{ DestroyWindow(hwnd); });

  // Intentionally ignore return value
  ShowWindow(hwnd, SW_SHOWNORMAL);
  auto result__update_window = UpdateWindow(hwnd);
  assert(result__update_window);

  auto hdc = GetDC(hwnd);
  auto on_exit__release_dc = on_exit([hwnd, hdc]{ ReleaseDC(hwnd, hdc); });

  auto pixel_format = ChoosePixelFormat(hdc, &pixel_format_descriptor);
  CHECK(pixel_format, 1, "ChoosePixelFormat Failed");

  auto result__set_pixel = SetPixelFormat(hdc, pixel_format, &pixel_format_descriptor);
  CHECK(result__set_pixel, 1, "SetPixelFormat Failed");

  auto hrc = wglCreateContext(hdc);
  CHECK(hrc, 1, "wglCreateContext Failed");
  auto on_exit__delete_context = on_exit([hrc]{ wglDeleteContext(hrc); });

  auto result__make_current = wglMakeCurrent(hdc, hrc);
  CHECK(result__make_current, 1, "wglMakeCurrent Failed");
  auto on_exit__make_current = on_exit([]{ wglMakeCurrent(nullptr, nullptr); });

#ifdef _DEBUG
  glEnable(GL_DEBUG_OUTPUT);
  auto glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
  glDebugMessageCallback(debug__callback, 0);
#endif

  init_effect();

  IMGUI_CHECKVERSION();
  auto imgui__context = ImGui::CreateContext();
  CHECK(imgui__context, 1, "ImGui::CreateContext Failed");
  auto on_exit__destroy_imgui_context = on_exit([]{ ImGui::DestroyContext(); });

  ImGuiIO& io = ImGui::GetIO();

  auto imgui__win32_init = ImGui_ImplWin32_Init(hwnd);
  CHECK(imgui__win32_init, 1, "ImGui_ImplWin32_Init Failed");
  auto on_exit__shutdown_imgui_win32 = on_exit([]{ ImGui_ImplWin32_Shutdown(); });

  auto imgui__opengl_init = ImGui_ImplOpenGL2_Init();
  CHECK(imgui__opengl_init, 1, "ImGui_ImplOpenGL2_Init Failed");
  auto on_exit__shutdown_imgui_opengl = on_exit([]{ ImGui_ImplOpenGL2_Shutdown(); });

  glEnable(GL_DEPTH_TEST);
#ifdef _DEBUG
  glDisable(GL_DEBUG_OUTPUT);
#endif

  float fixed_radius2     = 1.9F;
  float min_radius2       = 0.1F;
  float folding_limit     = 1.0F;
  float scale             = 2.5F;
  bool  show_demo_window  = false;

  ticks__write_pixel_as_sixels ticks;

  std::vector<ABGR>     pixels      ;
  std::vector<GLubyte>  sixel_pixels;
  std::string           buffer      ;
  // Reserve 1MiB
  buffer.reserve(1<<20);

  auto before = GetTickCount64();
  auto done = false;
  MSG msg = {};
  while (!done) {
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) done = true;
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }

    glClearColor(0,0,0,1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto now  = GetTickCount64();
    auto time = (now - before) / 1000.0f;

    draw_effect(
        time
      , fixed_radius2
      , min_radius2
      , folding_limit
      , -scale
      );

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplOpenGL2_NewFrame();

    // BEGIN: Intentionally ignore return values from ImGui
    ImGui::NewFrame();

    ImGui::Begin("Timing Info");

    ImGui::LabelText("Sixel Pixel" , "%0.2f" , (1.0*ticks.sixel_pixel)/ticks.total);
    ImGui::LabelText("Sixel Buffer", "%0.2f"  , (1.0*ticks.sixel_buffer)/ticks.total);
    ImGui::LabelText("Used Colors" , "%0.2f"  , (1.0*ticks.used_colors)/ticks.total);
    ImGui::LabelText("Write File"  , "%0.2f"  , (1.0*ticks.write_file)/ticks.total);

    ImGui::End();
/*
    ImGui::Begin("Control Panel");

    ImGui::SliderFloat("Fixed Radius" , &fixed_radius2, 0, 3);
    ImGui::SliderFloat("Min Radius"   , &min_radius2  , 0, 3);
    ImGui::SliderFloat("Folding Limit", &folding_limit, 0, 3);
    ImGui::SliderFloat("Scale"        , &scale        , 1, 4);

    ImGui::Checkbox("Show Demo Window", &show_demo_window);
    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::End();
*/
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    // END: Intentionally ignore return values from ImGui

    auto result__swap_buffers = SwapBuffers(hdc);
    assert(result__swap_buffers);

    glReadBuffer(GL_FRONT);
    if (viewport__width > 0 && viewport__height > 0) {
      auto total_size = viewport__width*viewport__height;
      pixels.resize(total_size);

      auto ptr__pixels = &pixels.front();

      glReadPixels(
          0
        , 0
        , static_cast<GLsizei>(viewport__width )
        , static_cast<GLsizei>(viewport__height)
        , GL_RGBA
        , GL_UNSIGNED_BYTE
        , ptr__pixels
        );
    
      memset(&ticks, 0, sizeof(ticks));

      write_pixel_as_sixels(
          hstdout
        , viewport__width
        , viewport__height
        , pixels
        , sixel_pixels
        , buffer
        , ticks
        );
    }
  }


  return 0;
}