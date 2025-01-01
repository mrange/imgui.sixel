#include "precompiled.hpp"

#include "screen.hpp"

#include <cstdio>

void effect0(float time, screen & screen);
void effect1(float time, screen & screen);
void effect2(float time, screen & screen);
void effect3(float time, screen & screen);
void effect4(float time, screen & screen);
void effect5(float time, screen & screen);
void effect6(float time, screen & screen);
void effect7(float time, screen & screen);

namespace {
  std::u8string to_u8string(std::string const & s) {
    return std::u8string(reinterpret_cast<char8_t const *>(s.c_str()), s.size());
  }

  std::u8string const prelude__goto_top    = u8"\x1B[?25l\x1B[H";
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

  bitmap const border = make_bitmap(col__rainbow, LR"BITMAP(
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
      HANDLE          hstdout
    , float           time
    , std::u8string & output
    , screen const &  screen
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

      wchar_t fg__white[]   = L"\x1B[38;2;255;255;255m";

      wchar_t bg__blue[]    = L"\x1B[48;2;10;10;80m";

      wchar_t* beat__colors[] = {
        fg__red
      , fg__orange
      , fg__yellow
      , fg__white
      };

      wchar_t   info__buffer[4096];
      auto      beat__i        = music__nsubdivision(time)%4;
      wchar_t   beat__progress  = L"⣀⣤⣶⣿"[beat__i];
      wchar_t*  beat__color     = beat__colors[beat__i];
      
      auto sub = fractf(time);
      auto sec = static_cast<int>(std::floorf(time));
      auto min = sec/60;
      sec -= min*60;
      auto ms  = static_cast<int>(sub*1000);

      auto info__len = swprintf_s(
          info__buffer
        , L"%s%s%c %s[%s%03d%s/%s%03d%s] %s%02d%s:%s%02d%s.%s%03d %56s %s%c"
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
        , L"GERP 2025"
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


}

int main() {
  //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
  auto hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
  CHECK_CONDITION(hstdout != INVALID_HANDLE_VALUE);
  
  auto result__setUtf8 = SetConsoleOutputCP(CP_UTF8);
  CHECK_CONDITION(result__setUtf8);

  DWORD consoleMode;
  auto result__get_console_mode = GetConsoleMode(hstdout, &consoleMode);
  CHECK_CONDITION(result__get_console_mode);

  //consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  // No flickering with just ENABLE_PROCESSED_OUTPUT?
  consoleMode = ENABLE_PROCESSED_OUTPUT;

  auto result__set_console_mode = SetConsoleMode(hstdout, consoleMode);
  CHECK_CONDITION(result__set_console_mode);

  std::u8string output;
  output.reserve(16384);

  screen screen = make_screen(screen__width, screen__height);

  auto before = GetTickCount64();
  while(true) {

    auto now  = GetTickCount64();
    auto time = (now - before) / 1000.0f;

    screen.clear();

    effect7(time, screen);
    screen.draw__bitmap(border     , time,  0,  0);

    write__screen(hstdout, time, output, screen);

    // 60 FPS
    auto const desired_wait = 1.F/60;
    auto frame_time         = (GetTickCount64() - now) / 1000.0f;

    if (frame_time > desired_wait) {
      Sleep(0);
    } else {
      auto sleep_for = static_cast<DWORD>(std::roundf((desired_wait-frame_time)*1000));
      // printf("%d\n",sleep_for);
      Sleep(sleep_for);
    }

  }

  return 0;
}
