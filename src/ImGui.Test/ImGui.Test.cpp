
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

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

    ~on_exit__impl() {
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

  using ABGR = std::uint32_t;

  char const        sixel_base      = 63;
  std::size_t const desired__width  = 800;
  std::size_t const desired__height = 600;
  char const windows_class_name[]   = "ImGui.Test";

  std::size_t viewport__width   = desired__width ;
  std::size_t viewport__height  = desired__height;
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


  void write_pixel_as_sixels(
      HANDLE                    hstdout
    , std::size_t               width
    , std::size_t               height
    , std::vector<ABGR> const & pixels
    , std::vector<GLubyte> &    sixel_pixels
    , std::string &             buffer
    ) {
    if (width > sixel__reps.size()) {
      return;
    }
    auto total_size = width*height;
    assert(hstdout != INVALID_HANDLE_VALUE);
    assert(pixels.size() > 0);
    assert(pixels.size() == total_size);
    assert(sixel_pixels.size() > 0);
    assert(sixel_pixels.size() == total_size);

    {
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

    bool used_colors[256];
    for (std::size_t y6 = 0; y6 < height; y6 += 6) {
      auto y6_off = y6*width;
      auto rem = std::min<std::size_t>(6, height - y6);
      {
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

    buffer.append(sixel__epilogue);

    if (buffer.size() > 0) {
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
  auto dwStyle = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_POPUP;

  auto register_class__result = RegisterClassEx(&wnd_class_ex);
  CHECK(register_class__result, 1, "Window Registration Failed");

  auto unregister_class__on_exit = on_exit([hinstance]{ UnregisterClass(windows_class_name, hinstance); });

  RECT windowRect = { 0, 0, desired__width, desired__height };
  auto rect__result = AdjustWindowRect(&windowRect, dwStyle, 0);
  CHECK(rect__result, 1, "AdjustWindowRect Failed");

  auto hwnd = CreateWindowEx(
    0                                   // Extended style
  , windows_class_name                  // Window class name
  , windows_class_name                  // Window title
  , dwStyle                             // Window style
  , CW_USEDEFAULT                       // StartPosition X
  , CW_USEDEFAULT                       // StartPosition Y
  , windowRect.right-windowRect.left    // Width
  , windowRect.bottom-windowRect.top    // Height
  , nullptr                             // Parent
  , nullptr                             // Menu
  , hinstance                           // Instance
  , nullptr                             // additional params
  );

  CHECK(hwnd, 1, "Window Creation Failed");
  auto destroy_window__on_exit = on_exit([hwnd]{ DestroyWindow(hwnd); });

  // Intentionally ignore return value
  ShowWindow(hwnd, SW_SHOWNORMAL);
  auto update_window__result = UpdateWindow(hwnd);
  assert(update_window__result);

  auto hdc = GetDC(hwnd);
  auto release_dc__on_exit = on_exit([hwnd, hdc]{ ReleaseDC(hwnd, hdc); });

  auto pixel_format = ChoosePixelFormat(hdc, &pixel_format_descriptor);
  CHECK(pixel_format, 1, "ChoosePixelFormat Failed");

  auto set_pixel__result = SetPixelFormat(hdc, pixel_format, &pixel_format_descriptor);
  CHECK(set_pixel__result, 1, "SetPixelFormat Failed");

  auto hrc = wglCreateContext(hdc);
  CHECK(hrc, 1, "wglCreateContext Failed");
  auto delete_context__on_exit = on_exit([hrc]{ wglDeleteContext(hrc); });

  auto make_current__result = wglMakeCurrent(hdc, hrc);
  CHECK(make_current__result, 1, "wglMakeCurrent Failed");
  auto make_current__on_exit = on_exit([]{ wglMakeCurrent(nullptr, nullptr); });

  IMGUI_CHECKVERSION();
  auto imgui__context = ImGui::CreateContext();
  CHECK(imgui__context, 1, "ImGui::CreateContext Failed");
  auto destroy_imgui_context__on_exit = on_exit([]{ ImGui::DestroyContext(); });

  ImGuiIO& io = ImGui::GetIO();

  auto imgui__win32_init = ImGui_ImplWin32_Init(hwnd);
  CHECK(imgui__win32_init, 1, "ImGui_ImplWin32_Init Failed");
  auto shutdown_imgui_win32__on_exit = on_exit([]{ ImGui_ImplWin32_Shutdown(); });

  auto imgui__opengl_init = ImGui_ImplOpenGL2_Init();
  CHECK(imgui__opengl_init, 1, "ImGui_ImplOpenGL2_Init Failed");
  auto shutdown_imgui_opengl__on_exit = on_exit([]{ ImGui_ImplOpenGL2_Shutdown(); });


  glEnable(GL_DEPTH_TEST);

  float clearColor[3] = {0.2F,0.1F,0.3F};
  int rotationAngle   = 0;
  bool wireframeMode  = false;
  bool showDemoWindow = false;

  std::vector<ABGR>     pixels      ;
  std::vector<GLubyte>  sixel_pixels;
  std::string           buffer      ;
  // Reserve 1MiB
  buffer.reserve(1<<20);

  MSG msg = {};
  while (GetMessage(&msg, nullptr, 0, 0) > 0) {
    // Intentionally ignore return value
    TranslateMessage(&msg);
    // Intentionally ignore return value
    DispatchMessage(&msg);

    glClearColor(clearColor[0], clearColor[1], clearColor[2],1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplWin32_NewFrame();
    ImGui_ImplOpenGL2_NewFrame();

    // BEGIN: Intentionally ignore return values from ImGui
    ImGui::NewFrame();

    ImGui::Begin("Control Panel");

    ImGui::SliderInt("Rotation Angle", &rotationAngle, 0, 360);

    ImGui::Checkbox("Wireframe Mode", &wireframeMode);

    ImGui::ColorEdit3("Clear Color", clearColor);

    ImGui::Checkbox("Show Demo Window", &showDemoWindow);
    if (showDemoWindow) {
      ImGui::ShowDemoWindow(&showDemoWindow);
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    // END: Intentionally ignore return values from ImGui

    auto swap_buffers__result = SwapBuffers(hdc);
    assert(swap_buffers__result);

    glReadBuffer(GL_FRONT);
    if (viewport__width > 0 && viewport__height > 0) {
      auto total_size = viewport__width*viewport__height;
      sixel_pixels.resize(total_size);
      pixels.resize(total_size);

      auto pixels__ptr = &pixels.front();

      glReadPixels(
          0
        , 0
        , static_cast<GLsizei>(viewport__width )
        , static_cast<GLsizei>(viewport__height)
        , GL_RGBA
        , GL_UNSIGNED_BYTE
        , pixels__ptr
        );
    
      write_pixel_as_sixels(
          hstdout
        , viewport__width
        , viewport__height
        , pixels
        , sixel_pixels
        , buffer
        );
    }
  }


  return 0;
}