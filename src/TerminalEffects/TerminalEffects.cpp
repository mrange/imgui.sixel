#define WIN32_LEAN_AND_MEAN
#define NOMINMAX             // Avoid conflict with C++ std::min and std::max
#define STRICT               // Enforce type safety
#define WINVER 0x0A00        // Target Windows 10
#define _WIN32_WINNT 0x0A00  // Match WINVER

#include <windows.h>

#include <cassert>
#include <algorithm>
#include <string>

namespace {
  static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");

  struct u8bitmap {
    std::u8string   pixels;
    std::size_t     height;
  };

  u8bitmap make_bitmap(std::u8string && pixels) {
    auto height = std::count(pixels.begin(), pixels.end(), u8'\n');
    return u8bitmap {
      std::move(pixels)
    , static_cast<std::size_t>(height)
    };
  }

  u8bitmap impulse = make_bitmap(u8R"BITMAP(
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

  u8bitmap sixel_pixel = make_bitmap(u8R"BITMAP(
  ████████ ██                  ██   ███████  ██                  ██
 ██░░░░░░ ░░                  ░██  ░██░░░░██░░                  ░██
░██        ██ ██   ██  █████  ░██  ░██   ░██ ██ ██   ██  █████  ░██
░█████████░██░░██ ██  ██░░░██ ░██  ░███████ ░██░░██ ██  ██░░░██ ░██
░░░░░░░░██░██ ░░███  ░███████ ░██  ░██░░░░  ░██ ░░███  ░███████ ░██
       ░██░██  ██░██ ░██░░░░  ░██  ░██      ░██  ██░██ ░██░░░░  ░██
 ████████ ░██ ██ ░░██░░██████ ███  ░██      ░██ ██ ░░██░░██████ ███
░░░░░░░░  ░░ ░░   ░░  ░░░░░░ ░░░   ░░       ░░ ░░   ░░  ░░░░░░ ░░░ 
)BITMAP");

  u8bitmap border = make_bitmap(u8R"BITMAP(
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
╚══════════════════════════════════════════════════════════════════════════════╝ 
)BITMAP");
  std::u8string const prelude = u8"\x1B[H";

  std::u8string const setup = u8"\x1B[H" u8R"LOGO(
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

  void write(HANDLE hstdout, std::u8string const & buffer) {
    auto writeOk = WriteFile(
      hstdout
    , &buffer.front()
    , static_cast<DWORD>(buffer.size())
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

  return 0;
}
