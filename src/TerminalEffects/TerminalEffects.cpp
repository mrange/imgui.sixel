#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>

#include <cassert>
#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <string>
#include <vector>

namespace {
  static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");
  const float pi  = 3.141592654F;
  const float tau = 2*pi;

  std::u8string to_u8string(std::string const & s) {
    return std::u8string(reinterpret_cast<char8_t const *>(s.c_str()), s.size());
  }

  std::u8string const prelude              = u8"\x1B[?25l\x1B[H";
  std::u8string const reset__colors        = u8"\x1B[0m";
  std::u8string const prelude__foreground  = u8"\x1B[38;2";
  std::u8string const prelude__background  = u8"\x1B[48;2";
  
  std::array<std::u8string, 256> generate__color_values() {
    std::array<std::u8string, 256> res;
    for (std::size_t i = 0; i < 256; ++i) {
      res[i] = to_u8string(std::format(";{}", i));
    }
    return res;
  }
  std::array<std::u8string, 256> color_values = generate__color_values();

  struct rgb {
    float red   ;
    float green ;
    float blue  ;
  };
  // License: Unknown, author: XorDev, found: https://x.com/XorDev/status/1808902860677001297
  rgb hsv2rgb_approx(float h, float s, float v) {
    float r  = (std::cosf(h*tau+0)*s+2-s)*v*0.5F;
    float g  = (std::cosf(h*tau+4)*s+2-s)*v*0.5F;
    float b  = (std::cosf(h*tau+2)*s+2-s)*v*0.5F;
    
    return {
      r
    , g
    , b
    };
  }

  // License: Unknown, author: XorDev, found: https://x.com/XorDev/status/1808902860677001297
  rgb palette(float a) {
    float r  = (1+std::sinf(a+0))*0.5F;
    float g  = (1+std::sinf(a+1))*0.5F;
    float b  = (1+std::sinf(a+2))*0.5F;
    
    return {
      r
    , g
    , b
    };
  }

  float mix(float b, float e, float x) {
    return b+(e-b)*x;
  }

  float smoothstep(float edge0, float edge1, float x) {
    float t = std::clamp<float>((x - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
  }

  using f__generate_color = std::function<rgb (float time, std::size_t x, std::size_t y)>;


  f__generate_color const col__white    = [](float time, std::size_t x, std::size_t y) -> rgb { return {1,1,1}; };
  f__generate_color const col__black    = [](float time, std::size_t x, std::size_t y) -> rgb { return {0,0,0}; };
  f__generate_color const col__gray     = [](float time, std::size_t x, std::size_t y) -> rgb { return {0.5,0.5,0.5}; };
  f__generate_color const col__graybar  = [](float time, std::size_t x, std::size_t y) -> rgb { 
    auto c = std::sqrtf(std::clamp<float>(y/10.0F, 0, 1));
    c = 1-c;
    return {c,c,c}; 
  };

  f__generate_color const col__rainbow  = [](float time, std::size_t x, std::size_t y) -> rgb { 
    return palette(time-(x+2.0*y)/20.F);
  };

  f__generate_color const col__flame  = [](float time, std::size_t x, std::size_t y) -> rgb { 
    auto c = std::clamp<float>(1-y/10.0F, 0, 1);
    return hsv2rgb_approx(0.05,0.5F,c); 
  };
  struct bitmap {
    std::wstring          shapes;
    std::size_t           width ;
    std::size_t           height;
    f__generate_color     f__foreground;
    f__generate_color     f__background;
  };

  struct screen {
    std::vector<wchar_t>  shapes      ;
    std::vector<rgb>      foreground  ;
    std::vector<rgb>      background  ;
    std::size_t           width       ;
    std::size_t           height      ;

    void clear() {
      shapes.clear();
      foreground.clear();
      background.clear();
      shapes.resize(width*height, L' ');
      foreground.resize(width*height  , {1,1,1});
      background.resize(width*height  , {0,0,0});
    }

    void draw__bitmap(
      bitmap const &  bmp
    , float           time
    , int             x
    , int             y
    ) {
      assert(bmp.f__foreground);
      assert(bmp.f__background);
      std::size_t from__x = std::clamp<int>(-x, 0, bmp.width - 1);
      std::size_t from__y = std::clamp<int>(-y, 0, bmp.height- 1);

      std::size_t to__x = std::clamp<int>(x, 0, width);
      std::size_t to__y = std::clamp<int>(y, 0, height);

      std::size_t effective__width  = std::min(bmp.width  - from__x, width - to__x);
      std::size_t effective__height = std::min(bmp.height - from__y, height - to__y);

      for (std::size_t yy = 0; yy < effective__height; ++yy) {
        assert(yy + from__y < bmp.height);
        assert(yy + to__y   < height);
        auto from__off= (yy+from__y)*bmp.width;
        auto to__off  = (yy+to__y)*width;
        for (std::size_t xx = 0; xx < effective__width; ++xx) {
          assert(xx + from__x < bmp.width);
          assert(xx + to__x   < width);
          auto c = bmp.shapes[from__off+xx+from__x];
          if (c > 32) {
            shapes[to__off+xx+to__x] = c;
            foreground[to__off+xx+to__x] = bmp.f__foreground(time, xx+from__x, yy+from__y);
            background[to__off+xx+to__x] = bmp.f__background(time, xx+from__x, yy+from__y);
          }
        }
      }
    }
  };

  void ltrim__inplace(std::wstring & s) {
    auto b = s.begin();
    auto e = s.end();

    for (auto i = b; i != e; ++i) {
      auto c = *i;
      if (c == L'\n') {
        // Found a new line, drop all leading white space chars including the new line
        ++i;
        s.erase(b, i);
        return;
      } else if (c <= 32) {
        // Found leading whitespace, continue
      } else {
        // Found non whitespace char, assuming entire string should be preserved
        return;
      }
    }
  }

  void rtrim__inplace(std::wstring & s) {
    auto b = s.rbegin();
    auto e = s.rend();

    for (auto i = b; i != e; ++i) {
      auto c = *i;
      if (c == L'\n') {
        // Found a new line, drop all leading white space chars including the new line
        ++i;
        s.erase(i.base(), b.base());
        return;
      } else if (c <= 32) {
        // Found leading whitespace, continue
      } else {
        // Found non whitespace char, assuming entire string should be preserved
        return;
      }
    }
  }

  void trim__inplace(std::wstring & s) {
    rtrim__inplace(s);
    ltrim__inplace(s);
  }

  screen make_screen(std::size_t w, std::size_t h) {
    assert(w > 0);
    assert(h > 0);
    return {
      {}
    , {}
    , {}
    , w
    , h
    };
  }

  bitmap make_bitmap(
      f__generate_color foreground
    , f__generate_color background
    , std::wstring      pixels    ) {
    std::size_t max__width      = 0;
    std::size_t max__height     = 0;

    std::size_t current__width  = 0;

    trim__inplace(pixels);

    for (std::size_t i = 0; i < pixels.size(); ++i) {
      auto c = pixels[i];

      if (c == '\n') {
        ++max__height;
        max__width = std::max(current__width, max__width);
        current__width = 0;
      } else if (c < 32) {
        // Skip non-printable chars
      } else {
        ++current__width;
      }
    }

    if (pixels.size() > 0) {
      if (pixels.back() > 31) {
        ++max__height;
      }
    }

    assert(max__width > 0);
    assert(max__height > 0);

    std::wstring result;
    result.reserve(max__width*max__height);

    current__width = 0;
    for (std::size_t i = 0; i < pixels.size(); ++i) {
      auto c = pixels[i];

      if (c == '\n') {
        for (std::size_t j = current__width; j < max__width; ++j) {
          result.push_back(L' ');
        }
        current__width = 0;
      } else if (c < 32) {
        // Skip non-printable chars
      } else {
        result.push_back(c);
        ++current__width;
      }
    }

    if (current__width > 0) {
      for (std::size_t j = current__width; j < max__width; ++j) {
        result.push_back(L' ');
      }
    }

    assert(result.size() == max__width*max__height);

    return bitmap {
      std::move(result)
    , max__width
    , max__height
    , std::move(foreground)
    , std::move(background)
    };
  }

  bitmap impulse = make_bitmap(col__graybar, col__black, LR"BITMAP(
 ██▓ ███▄ ▄███▓ ██▓███   █    ██  ██▓      ██████ ▓█████  ▐██▌
▓██▒▓██▒▀█▀ ██▒▓██░  ██▒ ██  ▓██▒▓██▒    ▒██    ▒ ▓█   ▀  ▐██▌
▒██▒▓██    ▓██░▓██░ ██▓▒▓██  ▒██░▒██░    ░ ▓██▄   ▒███    ▐██▌
░██░▒██    ▒██ ▒██▄█▓▒ ▒▓▓█  ░██░▒██░      ▒   ██▒▒▓█  ▄  ▓██▒
░██░▒██▒   ░██▒▒██▒ ░  ░▒▒█████▓ ░██████▒▒██████▒▒░▒████▒ ▒▄▄ 
░▓  ░ ▒░   ░  ░▒▓▒░ ░  ░░▒▓▒ ▒ ▒ ░ ▒░▓  ░▒ ▒▓▒ ▒ ░░░ ▒░ ░ ░▀▀▒
 ▒ ░░  ░      ░░▒ ░     ░░▒░ ░ ░ ░ ░ ▒  ░░ ░▒  ░ ░ ░ ░  ░ ░  ░
 ▒ ░░      ░   ░░        ░░░ ░ ░   ░ ░   ░  ░  ░     ░       ░
 ░         ░               ░         ░  ░      ░     ░  ░ ░   
)BITMAP");

  bitmap sixel_pixel = make_bitmap(col__rainbow, col__black, LR"BITMAP(
  ████████ ██                  ██   ███████  ██                  ██
 ██░░░░░░ ░░                  ░██  ░██░░░░██░░                  ░██
░██        ██ ██   ██  █████  ░██  ░██   ░██ ██ ██   ██  █████  ░██
░█████████░██░░██ ██  ██░░░██ ░██  ░███████ ░██░░██ ██  ██░░░██ ░██
░░░░░░░░██░██ ░░███  ░███████ ░██  ░██░░░░  ░██ ░░███  ░███████ ░██
       ░██░██  ██░██ ░██░░░░  ░██  ░██      ░██  ██░██ ░██░░░░  ░██
 ████████ ░██ ██ ░░██░░██████ ███  ░██      ░██ ██ ░░██░░██████ ███
░░░░░░░░  ░░ ░░   ░░  ░░░░░░ ░░░   ░░       ░░ ░░   ░░  ░░░░░░ ░░░ 
)BITMAP");

  bitmap border = make_bitmap(col__white, col__black, LR"BITMAP(
╔══════════════════════════════════════════════════════════════════════════════╗ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
║                                                                              ║ 
╚══════════════════════════════════════════════════════════════════════════════╝ 
)BITMAP");

  std::wstring const setup = L"\x1B[H" LR"LOGO(
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║                                                                              ║
║         ██▓ ███▄ ▄███▓ ██▓███   █    ██  ██▓      ██████ ▓█████  ▐██▌        ║
║        ▓██▒▓██▒▀█▀ ██▒▓██░  ██▒ ██  ▓██▒▓██▒    ▒██    ▒ ▓█   ▀  ▐██▌        ║
║        ▒██▒▓██    ▓██░▓██░ ██▓▒▓██  ▒██░▒██░    ░ ▓██▄   ▒███    ▐██▌        ║
║        ░██░▒██    ▒██ ▒██▄█▓▒ ▒▓▓█  ░██░▒██░      ▒   ██▒▒▓█  ▄  ▓██▒        ║
║        ░██░▒██▒   ░██▒▒██▒ ░  ░▒▒█████▓ ░██████▒▒██████▒▒░▒████▒ ▒▄▄         ║
║        ░▓  ░ ▒░   ░  ░▒▓▒░ ░  ░░▒▓▒ ▒ ▒ ░ ▒░▓  ░▒ ▒▓▒ ▒ ░░░ ▒░ ░ ░▀▀▒        ║
║         ▒ ░░  ░      ░░▒ ░     ░░▒░ ░ ░ ░ ░ ▒  ░░ ░▒  ░ ░ ░ ░  ░ ░  ░        ║
║         ▒ ░░      ░   ░░        ░░░ ░ ░   ░ ░   ░  ░  ░     ░       ░        ║
║         ░         ░               ░         ░  ░      ░     ░  ░ ░           ║
║                                                                              ║
║                                                                              ║
║                                                                              ║
║                               P R E S E N T S                                ║
║                                                                              ║
║                                                                              ║
║       ████████ ██                  ██   ███████  ██                  ██      ║
║      ██░░░░░░ ░░                  ░██  ░██░░░░██░░                  ░██      ║
║     ░██        ██ ██   ██  █████  ░██  ░██   ░██ ██ ██   ██  █████  ░██      ║
║     ░█████████░██░░██ ██  ██░░░██ ░██  ░███████ ░██░░██ ██  ██░░░██ ░██      ║
║     ░░░░░░░░██░██ ░░███  ░███████ ░██  ░██░░░░  ░██ ░░███  ░███████ ░██      ║
║            ░██░██  ██░██ ░██░░░░  ░██  ░██      ░██  ██░██ ░██░░░░  ░██      ║
║      ████████ ░██ ██ ░░██░░██████ ███  ░██      ░██ ██ ░░██░░██████ ███      ║
║     ░░░░░░░░  ░░ ░░   ░░  ░░░░░░ ░░░   ░░       ░░ ░░   ░░  ░░░░░░ ░░░       ║
║                                                                              ║
║     Designed for Cascadia Code font                                          ║
║                                Ensure that the entire border is visible      ║
╚══════════════════════════════════════════════════════════════════════════════╝
)LOGO";

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
    , rgb const &           color
    ) {
    output.append(prelude);
    auto to_i = [](float v) -> float {
      return static_cast<std::size_t>(std::roundf(std::sqrtf(std::clamp<float>(v, 0, 1))*255));
    };
    output.append(color_values[to_i(color.red)]);
    output.append(color_values[to_i(color.green)]);
    output.append(color_values[to_i(color.blue)]);
    output.push_back(u8'm');
  }

  void write__reset_color(
      std::u8string &       output
    ) {
    output.append(reset__colors);
  }

  void write(
      HANDLE          hstdout
    , std::u8string & output
    , screen const &  screen
    ) {

    auto w = screen.width;
    auto h = screen.height;
    assert(w*h == screen.shapes.size());
    assert(w*h == screen.foreground.size());
    assert(w*h == screen.background.size());

    rgb foreground = {1,1,1};
    rgb background = {0,0,0};
    write__color(output, prelude__foreground, foreground);
    write__color(output, prelude__background, background);

    for (std::size_t y = 0; y < h; ++y) {
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
      wchar_to_utf8(output, L'\n');
    }

    write__reset_color(output);

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

}

int main() {
  //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
  auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
  assert(hstdout != INVALID_HANDLE_VALUE);
  
  auto result__setUtf8 = SetConsoleOutputCP(CP_UTF8);
  assert(result__setUtf8);

  DWORD consoleMode;
  auto result__get_console_mode = GetConsoleMode(hstdout, &consoleMode);
  assert(result__get_console_mode);

  //consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  // No flickering with just ENABLE_PROCESSED_OUTPUT?
  consoleMode = ENABLE_PROCESSED_OUTPUT;

  auto result__set_console_mode = SetConsoleMode(hstdout, consoleMode);
  assert(result__set_console_mode);

  std::u8string output;
  output.reserve(16384);

  screen screen = make_screen(80, 30);

  auto before = GetTickCount64();
  while(true) {

    auto now  = GetTickCount64();
    auto time = (now - before) / 1000.0f;

    screen.clear();

    int xx = std::roundf(8+std::sinf(time+100)*8);
    int yy = std::roundf(8+std::sinf(0.707f*(time+100))*8);

    screen.draw__bitmap(impulse    , time, xx, yy);
    screen.draw__bitmap(sixel_pixel, time, yy, xx);
    screen.draw__bitmap(border     , time,  0,  0);

    output.clear();
    output.append(prelude);
    write(hstdout, output, screen);
  
    Sleep(20);  // 50 FPS
  }

  return 0;
}
