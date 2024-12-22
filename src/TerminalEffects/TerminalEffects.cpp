﻿#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>

#include <cassert>
#include <algorithm>
#include <functional>
#include <string>

namespace {
  static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");

  struct rgb {
    float red   ;
    float green ;
    float blue  ;
  };
  using f__line_col = std::function<rgb(int y)>;

  f__line_col line_col__white = [](int y) { return rgb {1.0,1.0,1.0}; };
  f__line_col line_col__gray  = [](int y) { return rgb {0.5,0.5,0.5}; };
    
  struct bitmap {
    std::wstring    pixels;
    std::size_t     width ;
    std::size_t     height;
  };

  struct screen {
    std::wstring    pixels;
    std::size_t     width ;
    std::size_t     height;

    void clear() {
      pixels.reserve((width+1)*height);
      pixels.clear();
      for (std::size_t i = 0; i < height; ++i) {
        pixels.append(width, L' ');
        pixels.push_back(L'\n');
      }
    }

    void draw__bitmap(
      bitmap const &  bmp
    , int             x
    , int             y
    , f__line_col     line_col
    ) {
 
      std::size_t from__x = std::clamp<int>(-x, 0, bmp.width - 1);
      std::size_t from__y = std::clamp<int>(-y, 0, bmp.height- 1);

      std::size_t to__x = std::clamp<int>(x, 0, width);
      std::size_t to__y = std::clamp<int>(y, 0, height);

      std::size_t effective__width  = std::min(bmp.width  - from__x, width - to__x);
      std::size_t effective__height = std::min(bmp.height - from__y, height - to__y);

      for (std::size_t yy = 0; yy < effective__height; ++yy) {
        auto rgb = line_col(yy+from__y);
        assert(yy + from__y < bmp.height);
        assert(yy + to__y   < height);
        auto from__off= (yy+from__y)*bmp.width;
        auto to__off  = (yy+to__y)*(width+1);
        for (std::size_t xx = 0; xx < effective__width; ++xx) {
          assert(xx + from__x < bmp.width);
          assert(xx + to__x   < width);
          auto c = bmp.pixels[from__off+xx+from__x];
          if (c > 32) {
            pixels[to__off+xx+to__x] = c;
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

  bitmap make_bitmap(std::wstring && pixels) {
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
    };
  }

  bitmap impulse = make_bitmap(LR"BITMAP(
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

  bitmap sixel_pixel = make_bitmap(LR"BITMAP(
  ████████ ██                  ██   ███████  ██                  ██
 ██░░░░░░ ░░                  ░██  ░██░░░░██░░                  ░██
░██        ██ ██   ██  █████  ░██  ░██   ░██ ██ ██   ██  █████  ░██
░█████████░██░░██ ██  ██░░░██ ░██  ░███████ ░██░░██ ██  ██░░░██ ░██
░░░░░░░░██░██ ░░███  ░███████ ░██  ░██░░░░  ░██ ░░███  ░███████ ░██
       ░██░██  ██░██ ░██░░░░  ░██  ░██      ░██  ██░██ ░██░░░░  ░██
 ████████ ░██ ██ ░░██░░██████ ███  ░██      ░██ ██ ░░██░░██████ ███
░░░░░░░░  ░░ ░░   ░░  ░░░░░░ ░░░   ░░       ░░ ░░   ░░  ░░░░░░ ░░░ 
)BITMAP");

  bitmap border = make_bitmap(LR"BITMAP(
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
  std::u8string const prelude = u8"\x1B[H";

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


  void wchar_to_utf8(std::u8string & output, const std::wstring& input) {
      for (wchar_t wc : input) {
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
  }

  void write(HANDLE hstdout, std::u8string output, std::wstring const & input) {

    wchar_to_utf8(output, input);

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


  screen screen = {
    L""
  , 80
  , 30
  };

  auto before = GetTickCount64();
  while(true) {

    auto now  = GetTickCount64();
    auto time = (now - before) / 1000.0f;

    screen.clear();

    auto xx = roundf(8+sinf(time+100)*16);
    auto yy = roundf(8+sinf(0.707f*(time+100))*16);

    screen.draw__bitmap(impulse    , xx,yy, line_col__gray);
    screen.draw__bitmap(sixel_pixel, yy,xx, line_col__gray);
    screen.draw__bitmap(border     ,  0, 0, line_col__white);

    output.append(prelude);
    write(hstdout, output, screen.pixels);
  
    Sleep(20);  // 50 FPS
  }

  return 0;
}