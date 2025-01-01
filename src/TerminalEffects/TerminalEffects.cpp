#include "precompiled.hpp"

#include "screen.hpp"

#include <cstdio>
#include <gl/GL.h>
#include "glext.h"


#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "opengl32.lib")

void effect0(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect1(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect2(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect3(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect4(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect5(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect6(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);
void effect7(float time, std::size_t beat__start, std::size_t beat__end, screen & screen);

namespace {
  std::size_t const desired__width  = 800;
  std::size_t const desired__height = 600;
  wchar_t const windows_class_name[]= L"TerminalEffects";

  std::size_t viewport__width       = desired__width ;
  std::size_t viewport__height      = desired__height;

  std::u8string to_u8string(std::string const & s) {
    return std::u8string(reinterpret_cast<char8_t const *>(s.c_str()), s.size());
  }

  std::u8string const prelude__goto_top    = u8"\x1B[?25l\x1B[H";
  std::u8string const reset__colors        = u8"\x1B[0m";
  std::u8string const prelude__foreground  = u8"\x1B[38;2";
  std::u8string const prelude__background  = u8"\x1B[48;2";
  
  using f__effect = std::function<void (
      float       time
    , std::size_t beat__start
    , std::size_t beat__end
    , screen  &   screen
    )>;

  struct script_part {
    std::size_t   beat__start ;
    f__effect     effect      ;
    std::wstring  name        ;
  };

  struct effective_script_part {
    std::size_t beat__start ;
    std::size_t beat__end   ;
    f__effect   effect      ;
    std::wstring  name        ;
  };

  std::array<effective_script_part, music__beat_length> effective_script;
  auto script = std::to_array<script_part>({
    {0  , effect7, L"Running INTRO.COM"}
  , {64 , effect0, L"FITB"}
  });

  script_part get__script_part(std::size_t i) {
    if (i < script.size()) {
      return script[i];
    } else {
      return script_part {
        effective_script.size()
      , effect0
      , L"FITB"
      };
    }
  }


  std::array<std::u8string, 256> generate__color_values() {
    std::array<std::u8string, 256> res;
    for (std::size_t i = 0; i < 256; ++i) {
      res[i] = to_u8string(std::format(";{}", i));
    }
    return res;
  }
  std::array<std::u8string, 256> color_values = generate__color_values();

  void wchar_to_utf8(std::u8string & output, wchar_t wc) {
    uint32_t codepoint = wc; // Assume UTF-32
    if (codepoint <= 0x7F) {
      output.push_back(static_cast<char8_t>(codepoint));
    } else if (codepoint <= 0x7FF) {
      output.push_back(static_cast<char8_t>(0xC0 | ((codepoint >> 6) & 0x1F)));
      output.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      output.push_back(static_cast<char8_t>(0xE0 | ((codepoint >> 12) & 0x0F)));
      output.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
      output.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
      output.push_back(static_cast<char8_t>(0xF0 | ((codepoint >> 18) & 0x07)));
      output.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 12) & 0x3F)));
      output.push_back(static_cast<char8_t>(0x80 | ((codepoint >> 6) & 0x3F)));
      output.push_back(static_cast<char8_t>(0x80 | (codepoint & 0x3F)));
    }
  }
  void wchars_to_utf8(std::u8string & output, std::wstring const & wcs) {
    for (auto wc : wcs) {
      wchar_to_utf8(output, wc);
    }
  }

  void write__color(
      std::u8string &       output
    , std::u8string const & prelude
    , vec3 const &          color
    ) {
    output.append(prelude);
    auto to_i = [](float v) -> std::size_t {
      return static_cast<std::size_t>(std::roundf(std::sqrtf(std::clamp<float>(v, 0, 1))*255));
    };
    output.append(color_values[to_i(color.x)]);
    output.append(color_values[to_i(color.y)]);
    output.append(color_values[to_i(color.z)]);
    output.push_back(u8'm');
  }

  void write__reset_color(
      std::u8string &       output
    ) {
    output.append(reset__colors);
  }

  void write__screen(
      HANDLE                        hstdout
    , float                         time
    , effective_script_part const & part
    , std::u8string &               output
    , screen const &                screen
    ) {
    output.clear();
    output.append(prelude__goto_top);

    auto w = screen.width;
    auto h = screen.height;
    assert(w*h == screen.shapes.size());
    assert(w*h == screen.foreground.size());
    assert(w*h == screen.background.size());

    vec3 foreground = vec3 {1,1,1};
    vec3 background = vec3 {0,0,0};

    for (std::size_t y = 0; y < h; ++y) {
      write__color(output, prelude__foreground, foreground);
      write__color(output, prelude__background, background);
      auto y__off = y*w;
      for (std::size_t x = 0; x < w; ++x) {
        auto wc = screen.shapes[y__off+x];
        auto new_foreground = screen.foreground[y__off+x];
        auto new_background = screen.background[y__off+x];

        if (memcmp(&new_foreground, &foreground, sizeof(foreground)) != 0) {
          foreground = new_foreground;
          write__color(output, prelude__foreground, foreground);
        }

        if (memcmp(&new_background, &background, sizeof(background)) != 0) {
          background = new_background;
          write__color(output, prelude__background, background);
        }

        wchar_to_utf8(output, wc);
      }
      write__reset_color(output);
      wchar_to_utf8(output, L'\n');
    }

    {
      
      /*⣀⣤⣶⣿ */
      wchar_t fg__red[]     = L"\x1B[38;2;255;85;85m";  
      wchar_t fg__orange[]  = L"\x1B[38;2;255;165;85m"; 
      wchar_t fg__yellow[]  = L"\x1B[38;2;255;255;85m"; 
      wchar_t fg__muted[]   = L"\x1B[38;2;80;80;120m";
      wchar_t fg__hilight[] = L"\x1B[38;2;160;160;220m";
      wchar_t fg__cyan[]    = L"\x1B[38;2;85;255;255m";

      wchar_t fg__white[]   = L"\x1B[38;2;255;255;255m";

      wchar_t bg__blue[]    = L"\x1B[48;2;10;10;80m";

      wchar_t* beat__colors[] = {
        fg__red
      , fg__orange
      , fg__yellow
      , fg__white
      };

      wchar_t   info__buffer[2048];
      auto      beat__i         = music__nsubdivision(time)%4;
      wchar_t   beat__progress  = L"⣀⣤⣶⣿"[beat__i];
      wchar_t*  beat__color     = beat__colors[beat__i];

      auto      run__i          = static_cast<int>(fractf(0.5F*time/music__beat_time)*8);
      wchar_t   run__progress   = L"⣤⣆⡇⠏⠛⠹⢸⣰"[run__i];

      wchar_t const * part__progress_parts[] = {
        L"▱▱▱▱▱"
      , L"▰▱▱▱▱"
      , L"▰▰▱▱▱"
      , L"▰▰▰▱▱"
      , L"▰▰▰▰▱"
      , L"▰▰▰▰▰"
      };
      int part__i;
      {
        auto x = time - music__from_nbeat(part.beat__start);
        auto y = music__from_nbeat(part.beat__end) - music__from_nbeat(part.beat__start);
        part__i = std::clamp<int>(static_cast<int>(6*x/y), 0, 5);
      }
      wchar_t const * part__progress = part__progress_parts[part__i] ;

      auto sub = fractf(time);
      auto sec = static_cast<int>(std::floorf(time));
      auto min = sec/60;
      sec -= min*60;
      auto ms  = static_cast<int>(sub*1000);

      auto info__len = swprintf_s(
          info__buffer
        , L"%s%s%c %s[%s%03d%s/%s%03d%s] %s%02d%s:%s%02d%s.%s%03d %c %s%-38s %s%s GERP 2025 %s%c"
        , bg__blue
        , beat__color
        , beat__progress
        , fg__muted
        , fg__hilight
        , music__nbeat(time)
        , fg__muted
        , fg__hilight
        , music__nbar(time)
        , fg__muted
        , fg__hilight
        , min
        , fg__muted
        , fg__hilight
        , sec
        , fg__muted
        , fg__hilight
        , ms
        , run__progress
        , fg__cyan
        , part.name.c_str()
        , fg__hilight
        , part__progress
        , beat__color
        , beat__progress
        );
      assert(info__len > -1);
      for (auto i = 0; i < info__len; ++i) {
        wchar_to_utf8(output, info__buffer[i]);
      }

      write__reset_color(output);
    }


    auto writeOk = WriteFile(
      hstdout
    , &output.front()
    , static_cast<DWORD>(output.size())
    , nullptr
    , nullptr
    );
    assert(writeOk);
//    auto flushOk = FlushFileBuffers(hstdout);
//    assert(flushOk);
  }

  LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
  try {
    {
      // Create effective script
      std::size_t idx = 0;
      auto current    = get__script_part(idx);
      auto next       = get__script_part(idx+1);
      for (std::size_t i = 0; i < effective_script.size(); ++i) {
        if (i >= next.beat__start) {
          ++idx;
          current = get__script_part(idx);
          next    = get__script_part(idx + 1);
        }
        effective_script[i] = effective_script_part {
          current.beat__start
        , next.beat__start
        , current.effect
        , current.name
        };
      }
    }
    CHECK_HRESULT(CoInitialize(0));
    auto onexit__counitialize = on_exit([]{ CoUninitialize(); });

    CHECK_HRESULT(MFStartup(MF_VERSION));
    auto onexit__mfshutdown = on_exit([]{ MFShutdown(); });

    auto hinstance = GetModuleHandle(0);
    CHECK_CONDITION(hinstance);

    wnd_class_ex.hInstance = hinstance;
    auto dw_style = WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_POPUP;

    auto result__register_class = RegisterClassEx(&wnd_class_ex);
    CHECK_CONDITION(result__register_class);

    auto on_exit__unregister_class = on_exit([hinstance]{ UnregisterClass(windows_class_name, hinstance); });

    RECT window_rect = { 0, 0, desired__width, desired__height };
    auto result__rect= AdjustWindowRect(&window_rect, dw_style, 0);
    CHECK_CONDITION(result__rect);

    auto wnd__width   = window_rect.right-window_rect.left;
    auto wnd__height  = window_rect.bottom-window_rect.top;
    auto wnd__x       = GetSystemMetrics(SM_CXSCREEN) - wnd__width;
    auto wnd__y       = 0;

    auto hwnd = CreateWindowEx(
      0                   // Extended style
    , windows_class_name  // Window class name
    , windows_class_name  // Window title
    , dw_style            // Window style
    , wnd__x              // StartPosition X
    , wnd__y              // StartPosition Y
    , wnd__width          // Width
    , wnd__height         // Height
    , nullptr             // Parent
    , nullptr             // Menu
    , hinstance           // Instance
    , nullptr             // additional params
    );
    CHECK_CONDITION(hwnd);
    auto on_exit__destroy_window = on_exit([hwnd]{ DestroyWindow(hwnd); });

    // Intentionally ignore return value
    ShowWindow(hwnd, SW_SHOWNORMAL);
    auto result__update_window = UpdateWindow(hwnd);
    assert(result__update_window);

    auto hdc = GetDC(hwnd);
    CHECK_CONDITION(hdc);
    auto on_exit__release_dc = on_exit([hwnd, hdc]{ ReleaseDC(hwnd, hdc); });

    auto pixel_format = ChoosePixelFormat(hdc, &pixel_format_descriptor);
    CHECK_CONDITION(pixel_format);

    CHECK_CONDITION(SetPixelFormat(hdc, pixel_format, &pixel_format_descriptor));

    auto hrc = wglCreateContext(hdc);
    CHECK_CONDITION(hrc);
    auto on_exit__delete_context = on_exit([hrc]{ wglDeleteContext(hrc); });

    auto result__make_current = wglMakeCurrent(hdc, hrc);
    CHECK_CONDITION(result__make_current);
    auto on_exit__make_current = on_exit([]{ wglMakeCurrent(nullptr, nullptr); });

    IMFPMediaPlayer * player = nullptr;
    CHECK_HRESULT(MFPCreateMediaPlayer(
      LR"PATH(D:\assets\astroboy--my-head-is-spiritualism.wav)PATH"
    , FALSE   
    , 0       
    , nullptr 
    , hwnd 
    , &player
    ));
    assert(player);
    auto onexit__release_player = on_exit([player]{ player->Release(); });

    auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CHECK_CONDITION(hstdout != INVALID_HANDLE_VALUE);
  
    CHECK_CONDITION(SetConsoleOutputCP(CP_UTF8));

    DWORD consoleMode;
    CHECK_CONDITION(GetConsoleMode(hstdout, &consoleMode));

    //consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // No flickering with just ENABLE_PROCESSED_OUTPUT?
    consoleMode = ENABLE_PROCESSED_OUTPUT;

    CHECK_CONDITION(SetConsoleMode(hstdout, consoleMode));

    std::u8string output;
    output.reserve(16384);

    CHECK_HRESULT(player->Play());
    auto onexit__stop_player = on_exit([player]{ player->Stop(); });

    screen screen = make_screen(screen__width, screen__height);

#define MUSIC_TIME
#ifdef MUSIC_TIME
#else
    auto before = GetTickCount64();
#endif
    auto done = false;
    auto msg = MSG {};

    while(!done) {
      while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) done = true;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      glClearColor(0,0,0,1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#ifdef MUSIC_TIME
      float time = 0;
      PROPVARIANT position_value;
      if (SUCCEEDED(player->GetPosition(MFP_POSITIONTYPE_100NS, &position_value))) {
        if (position_value.vt == VT_I8) {
          time = position_value.hVal.QuadPart/1E7F;
        }
      };
#else
      auto now  = GetTickCount64();
      auto time = (now - before) / 1000.0f;
#endif

      auto nbeat = music__nbeat(time);
      done |= nbeat >= effective_script.size();
      nbeat = std::clamp<std::size_t>(nbeat, 0, effective_script.size()-1);

      screen.clear();

      auto & part = effective_script[nbeat];

      part.effect(time, part.beat__start, part.beat__end, screen);

      write__screen(
          hstdout
        , time
        , part
        , output
        , screen);

      auto result__swap_buffers = SwapBuffers(hdc);
      assert(result__swap_buffers);
    }

    return 0;
  } catch (std::runtime_error const & e) {
    std::printf("An error occured: %s", e.what());
    return 98;
  } catch (...) {
    std::printf("An error occured: Unknown");
    return 99;
  }
}
