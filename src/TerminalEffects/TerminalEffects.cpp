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
#include <map>
#include <string>
#include <random>
#include <vector>

#include "vectors.hh"

using namespace vectors;

namespace {
  static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");
  const float pi                    = 3.141592654F;
  const float tau                   = 2*pi;
  const std::size_t screen__width   = 80;
  const std::size_t screen__height  = 30;

  std::mt19937        random__generator (19740531);

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

  // License: Unknown, author: Unknown, found: don't remember
  float hash(float co) {
    return fractf(sinf(co*12.9898F) * 13758.5453F);
  }

  // License: Unknown, author: Unknown, found: don't remember
  float hash(vec2 const & co) {
    return fractf(sin(co.dot(vec2(12.9898,58.233F))) * 13758.5453F);
  }

  // License: Unknown, author: Matt Taylor (https://github.com/64), found: https://64.github.io/tonemapping/
  vec3 aces_approx(vec3 v) {
    auto f = [](float v) {
      const float a = 2.51;
      const float b = 0.03;
      const float c = 2.43;
      const float d = 0.59;
      const float e = 0.14;

      v = std::max(v, 0.0F);
      v *= 0.6F;
      return std::clamp<float>((v*(a*v+b))/(v*(c*v+d)+e), 0, 1);
    };

    return vec3 {
      f(v.x)
    , f(v.y)
    , f(v.z)
    };
  }

  // License: Unknown, author: XorDev, found: https://x.com/XorDev/status/1808902860677001297
  vec3 hsv2rgb_approx(float h, float s, float v) {
    float r  = (std::cosf(h*tau+0)*s+2-s)*v*0.5F;
    float g  = (std::cosf(h*tau+4)*s+2-s)*v*0.5F;
    float b  = (std::cosf(h*tau+2)*s+2-s)*v*0.5F;
    
    return vec3 {
      r
    , g
    , b
    };
  }

  // License: Unknown, author: XorDev, found: https://x.com/XorDev/status/1808902860677001297
  vec3 palette(float a) {
    float r  = (1+std::sinf(a+0))*0.5F;
    float g  = (1+std::sinf(a+1))*0.5F;
    float b  = (1+std::sinf(a+2))*0.5F;
    
    return vec3 {
      r
    , g
    , b
    };
  }

  using f__generate_color = std::function<vec3 (float time, std::size_t x, std::size_t y)>;

  f__generate_color const col__white    = [](float time, std::size_t x, std::size_t y) -> vec3 { return vec3 {1,1,1}; };
  f__generate_color const col__black    = [](float time, std::size_t x, std::size_t y) -> vec3 { return vec3 {0,0,0}; };
  f__generate_color const col__gray     = [](float time, std::size_t x, std::size_t y) -> vec3 { return vec3 {0.5,0.5,0.5}; };
  f__generate_color const col__graybar  = [](float time, std::size_t x, std::size_t y) -> vec3 { 
    auto c = std::sqrtf(std::clamp<float>(y/10.0F, 0, 1));
    c = 1-c;
    return vec3 {c,c,c}; 
  };

  f__generate_color const col__rainbow  = [](float time, std::size_t x, std::size_t y) -> vec3 { 
    return palette(time-(x+2.0F*y)/20.F);
  };

  f__generate_color const col__flame  = [](float time, std::size_t x, std::size_t y) -> vec3 { 
    auto c = std::clamp<float>(1-y/10.0F, 0, 1);
    return hsv2rgb_approx(0.05,0.5F,c); 
  };

  struct bitmap {
    std::wstring          shapes;
    std::size_t           width ;
    std::size_t           height;
    f__generate_color     foreground;

    bitmap with__foreground(
        f__generate_color f
      ) const {
      auto c = *this;
      c.foreground = std::move(f);
      return c;
    }

  };

  struct screen {
    std::vector<wchar_t>  shapes      ;
    std::vector<vec3>      foreground  ;
    std::vector<vec3>      background  ;
    std::size_t           width       ;
    std::size_t           height      ;

    void clear() {
      shapes.clear();
      foreground.clear();
      background.clear();
      shapes.resize(width*height, L' ');
      foreground.resize(width*height  , vec3 {1,1,1});
      background.resize(width*height  , vec3 {0,0,0});
    }

    void draw__pixel(
      wchar_t s
    , vec3     f
    , vec3     b
    , int     x
    , int     y
    ) {
      assert(width*height == shapes.size());
      assert(width*height == foreground.size());
      assert(width*height == background.size());
      if (x >= 0 && x < width) {
        if (y >= 0 && y < height) {
          auto off = x + y*width;
          shapes[off]     = s;
          foreground[off] = f;
          background[off] = b;
        }
      }
    }

    void draw__bitmap(
      bitmap const &  bmp
    , float           time
    , int             x
    , int             y
    , float           opacity = 1
    ) {
      assert(bmp.foreground);
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
          auto s = bmp.shapes[from__off+xx+from__x];
          if (s > 32) {
            auto f = bmp.foreground(time, xx+from__x, yy+from__y);
            shapes[to__off+xx+to__x]      = s;
            foreground[to__off+xx+to__x]  = mix(foreground[to__off+xx+to__x], f, opacity);
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
    , std::wstring      pixels    
    ) {
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
    };
  }

  bitmap const impulse = make_bitmap(col__graybar, LR"BITMAP(
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

  bitmap const impulse2 = make_bitmap(col__black, LR"BITMAP(
 ██▓ ███▄ ▄███▓ ██▓███   █    ██  ██▓      ██████ ▓█████  ▐██▌
▓██▒▓██▒▀█▀ ██▒▓██░  ██▒ ██  ▓██▒▓██▒    ▒██    ▒ ▓█   ▀  ▐██▌
▒██▒▓██    ▓██░▓██░ ██▓▒▓██  ▒██░▒██░    ░ ▓██▄   ▒███    ▐██▌
░██░▒██    ▒██ ▒██▄█▓▒ ▒▓▓█  ░██░▒██░      ▒   ██▒▒▓█  ▄  ▓██▒
░██░▒██▒   ░██▒▒██▒ ░  ░▒▒█████▓ ░██████▒▒██████▒▒░▒████▒ ▒▄▄ 
░▓  ░ ▒░   ░  ░▒▓▒░ ░  ░░▒▓▒ ▒ ▒ ░ ▒░▓  ░▒ ▒▓▒ ▒ ░░░ ▒░ ░ ░▀▀▒
 ▒ ░░  ░      ░░▒ ░     ░░▒░ ░ ░ ░ ░ ▒  ░░ ░▒  ░ ░ ░ ░  ░ ░  ░
 ▒ ░░      ░   ░░        ░░░ ░ ░   ░ ░   ░  ░  ░     ░       ░
 ░         ░               ░         ░  ░      ░     ░  ░ ░   

                                     ╭─────╮
               ╭─────────────────────┤▄▀▄▀▄├───────────────╮
   ┼───────────┼ ▀ G L I M G L A M ▄ │▄▀▄▀▄│ ▄ L A N C E ▀ │
   │ ▄ J E Z ▀ ┼──────────┼──────────┼─────┼────┼──────────┼
   ╰───────────┼          │ ▀ L O N G S H O T ▄ │
                          ╘═════════════════════╛
)BITMAP");

  bitmap const lance = make_bitmap(col__rainbow, LR"BITMAP(
                    ___           ___           ___           ___     
                   ╱╲  ╲         ╱╲  ╲         ╱╲__╲         ╱╲__╲    
                  ╱  ╲  ╲        ╲ ╲  ╲       ╱ ╱  ╱        ╱ ╱ _╱_   
                 ╱ ╱╲ ╲  ╲        ╲ ╲  ╲     ╱ ╱  ╱        ╱ ╱ ╱╲__╲  
  ___     ___   ╱ ╱ ╱  ╲  ╲   _____╲ ╲  ╲   ╱ ╱  ╱  ___   ╱ ╱ ╱ ╱ _╱_ 
 ╱╲  ╲   ╱╲__╲ ╱ ╱_╱ ╱╲ ╲__╲ ╱        ╲__╲ ╱ ╱__╱  ╱╲__╲ ╱ ╱_╱ ╱ ╱╲__╲
 ╲ ╲  ╲ ╱ ╱  ╱ ╲ ╲╱ ╱  ╲╱__╱ ╲  ______╱__╱ ╲ ╲  ╲ ╱ ╱  ╱ ╲ ╲╱ ╱ ╱ ╱  ╱
  ╲ ╲  ╱ ╱  ╱   ╲  ╱__╱       ╲ ╲  ╲        ╲ ╲  ╱ ╱  ╱   ╲  ╱_╱ ╱  ╱ 
   ╲ ╲╱ ╱  ╱     ╲ ╲  ╲        ╲ ╲  ╲        ╲ ╲╱ ╱  ╱     ╲ ╲╱ ╱  ╱  
    ╲  ╱  ╱       ╲ ╲__╲        ╲ ╲__╲        ╲  ╱  ╱       ╲  ╱  ╱   
     ╲╱__╱         ╲╱__╱         ╲╱__╱         ╲╱__╱         ╲╱__╱    
)BITMAP");

  bitmap const glimglam = make_bitmap(col__rainbow, LR"BITMAP(
  ________.__  .__                .__                  
 ╱  _____╱|  | |__| _____    ____ |  | _____    _____  
╱   ╲  ___|  | |  |╱     ╲  ╱ ___╲|  | ╲__  ╲  ╱     ╲ 
╲    ╲_╲  ╲  |_|  |  Y Y  ╲╱ ╱_╱  >  |__╱ __ ╲|  Y Y  ╲
 ╲______  ╱____╱__|__|_|  ╱╲___  ╱|____(____  ╱__|_|  ╱
        ╲╱              ╲╱╱_____╱           ╲╱      ╲╱ 
)BITMAP");

  bitmap const code_by = make_bitmap(col__white, LR"BITMAP(
_________            .___       ___.          
╲_   ___ ╲  ____   __| _╱____   ╲_ |__ ___.__.
╱    ╲  ╲╱ ╱  _ ╲ ╱ __ |╱ __ ╲   | __ <   |  |
╲     ╲___(  <_> ) ╱_╱ ╲  ___╱   | ╲_╲ ╲___  |
 ╲______  ╱╲____╱╲____ |╲___  >  |___  ╱ ____|
        ╲╱            ╲╱    ╲╱       ╲╱╲╱     
)BITMAP");

  bitmap const gerp = make_bitmap(col__black, LR"BITMAP(
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
░░░░░░░░      ░░░░░░░░░        ░░░░░░░░       ░░░░░░░░░       ░░░░░░░░
▒▒▒▒▒▒▒  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒  ▒▒▒▒  ▒▒▒▒▒▒▒▒  ▒▒▒▒  ▒▒▒▒▒▒▒
▓▓▓▓▓▓▓  ▓▓▓   ▓▓▓▓▓▓▓▓      ▓▓▓▓▓▓▓▓▓▓       ▓▓▓▓▓▓▓▓▓       ▓▓▓▓▓▓▓▓
███████  ████  ████████  ██████████████  ███  █████████  █████████████
████████      █████████        ████████  ████  ████████  █████████████
██████████████████████████████████████████████████████████████████████

      
         ╭─────────────────────────────────────────────────╮
         │     .         .        ♥       *             .  │
         │ *       ████     ████     ████    ██████     .  │
  ╭────╮ │   *    █░░░ █   █░░░██   █░░░ █  ░█░░░░         │ ╭────╮ 
  ╱ ◢◣ ╲ │       ░    ░█  ░█  █░█  ░    ░█  ░█████   *     │ ╱ ◢◣ ╲ 
 ╱━━━━━━╲├────━━════███════█ █ ░█═════███═════════█═━━─────┤╱━━━━━━╲
 ╰○○○○○○╯│      *  █░░    ░██  ░█    █░░         ░█      . │╰○○○○○○╯
         │  .     █       ░█   ░█   █        █   ░█   . *  │
         │       ░██████  ░ ████   ░██████  ░ ████         │
         │.  ♥   ░░░░░░    ░░░░    ░░░░░░    ░░░░   ♥      │
         │              *        .        *                │
         ╘═════════════════════════════════════════════════╛
)BITMAP");          

  bitmap const sixel_pixel = make_bitmap(col__rainbow, LR"BITMAP(
  ████████ ██                  ██   ███████  ██                  ██
 ██░░░░░░ ░░                  ░██  ░██░░░░██░░                  ░██
░██        ██ ██   ██  █████  ░██  ░██   ░██ ██ ██   ██  █████  ░██
░█████████░██░░██ ██  ██░░░██ ░██  ░███████ ░██░░██ ██  ██░░░██ ░██
░░░░░░░░██░██ ░░███  ░███████ ░██  ░██░░░░  ░██ ░░███  ░███████ ░██
       ░██░██  ██░██ ░██░░░░  ░██  ░██      ░██  ██░██ ░██░░░░  ░██
 ████████ ░██ ██ ░░██░░██████ ███  ░██      ░██ ██ ░░██░░██████ ███
░░░░░░░░  ░░ ░░   ░░  ░░░░░░ ░░░   ░░       ░░ ░░   ░░  ░░░░░░ ░░░ 
)BITMAP");

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
    , vec3 const &           color
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

  void effect0(float time, screen & screen) {
    int xx = std::roundf(8+std::sinf(time+100)*8);
    int yy = std::roundf(8+std::sinf(0.707f*(time+100))*8);

    screen.draw__bitmap(impulse    , time, xx, yy);
    screen.draw__bitmap(sixel_pixel, time, yy, xx);
  }


  // License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
  float pmin(float a, float b, float k) {
    float h = std::clamp<float>(0.5F+0.5F*(b-a)/k, 0.0F, 1.0F);
    return mix(b, a, h) - k*h*(1.0F-h);
  }

  // License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
  float pmax(float a, float b, float k) {
    return -pmin(-a, -b, k);
  }

  float length(float x, float y) {
    return std::sqrtf(x*x+y*y);
  }

  void effect1(float time, screen & screen) {
    auto df = [](float x, float y) -> float {
      const float m = 0.5;
      float l = length(x,y);
      l = std::fmodf(l+(0.5F*m),m)-(0.5F*m);
      return std::abs(l)-(m*0.25F);
    };

    for (std::size_t y = 0; y < screen.height; ++y) {
      auto py = (-1.F*screen.height+2.F*y)/screen.height;
      for (std::size_t x = 0; x < screen.width; ++x) {
        auto px = (-1.F*screen.width+2.F*x )/screen.width;

        auto px0 = px;
        auto py0 = py;

        px0 -= std::sinf(0.707f*(time+100));
        py0 -= std::sinf((time+100));

        auto px1 = px;
        auto py1 = py;

        px1 -= std::sinf(0.5F*(time+123));
        py1 -= std::sinf(0.707F*(time+123));
        float sm = 0.125F*length(px, py);

        auto d0 = df(px0, py0);
        auto d1 = df(px1, py1);
        auto d  = d0;
        d = pmax(d, d1, sm);
        float dd = -d0;
        dd = pmax(dd, -d1, sm);
        d =  std::min(d, dd);

        auto col0 = palette(d+time+py);
        auto col1 = palette(d+1.5F+time*0.707F+py);
        auto col = d < 0.0 ? col0 : col1;
        col *= smoothstep(-0.5F, 0.5F, -std::cosf(time-py-0.5F*px*px));
        screen.draw__pixel(
            L' '
          , vec3{0,0,0}
          , col
          , x
          , y
          );
      }
    }
    screen.draw__bitmap(impulse2  , time, 8, 6);
//    screen.draw__bitmap(gerp        , time, 5, 6);

  }

  void effect2(float time, screen & screen) {

    for (std::size_t y = 0; y < screen.height; ++y) {
      auto py = (-1.F*screen.height+2.F*y)/screen.height;
      for (std::size_t x = 0; x < screen.width; ++x) {
        auto px = (-1.F*screen.width+2.F*x)/screen.width;

        auto p = vec2 {px, py};
        auto h0 = hash(p+std::floorf(-0.25*time+py*py+0.33*hash(p)));

        auto shape = L'╳';
        if (h0 > 0.55) {
          shape = L'╱'; 
        } else if (h0 > 0.1) {
          shape = L'╲';
        } else {
          shape = L'_';
        }
        screen.draw__pixel(
            shape
          , hsv2rgb_approx(std::sinf(time*0.707F)*px*py+0.5*py*py-0.5*time, mix(1.0,0.5,py*py), 1.0+py*py)*(smoothstep(0,1,py*py))
          , vec3 {0,0,0}
          , x
          , y
          );
      }
    }

    auto sel = std::fmodf(std::floorf(time/tau), 2.0F);
    auto opacity = smoothstep(0.5F, 0.707F, -std::cosf(time));

    if (sel == 0) {
      screen.draw__bitmap(code_by, time, 14, 12, opacity);
    } else {
      screen.draw__bitmap(lance, time, 4, 9, opacity);
    }
  }

  struct cell {
    wchar_t shape               ;
    char    connection__single  ;
    char    connection__double  ;
  };

  cell cells[] {
    { L' ', 0b0000, 0b0000 }
  , { L'─', 0b1010, 0b0000 }
  , { L'│', 0b0101, 0b0000 }
  , { L'┌', 0b0011, 0b0000 }
  , { L'┐', 0b1001, 0b0000 }
  , { L'└', 0b0110, 0b0000 }
  , { L'┘', 0b1100, 0b0000 }
  , { L'├', 0b0111, 0b0000 }
  , { L'┤', 0b1101, 0b0000 }
  , { L'┬', 0b1011, 0b0000 }
  , { L'┴', 0b1110, 0b0000 }
  , { L'┼', 0b1111, 0b0000 }

  , { L'═', 0b0000 , 0b1010}
  , { L'║', 0b0000 , 0b0101}
  , { L'╔', 0b0000 , 0b0011}
  , { L'╗', 0b0000 , 0b1001}
  , { L'╚', 0b0000 , 0b0110}
  , { L'╝', 0b0000 , 0b1100}
  , { L'╠', 0b0000 , 0b0111}
  , { L'╣', 0b0000 , 0b1101}
  , { L'╦', 0b0000 , 0b1011}
  , { L'╩', 0b0000 , 0b1110}
  , { L'╬', 0b0000 , 0b1111}

  , { L'╒', 0b0000 , 0b0000}
  , { L'╓', 0b0000 , 0b0000}
  , { L'╕', 0b0000 , 0b0000}
  , { L'╖', 0b0000 , 0b0000}
  , { L'╘', 0b0000 , 0b0000}
  , { L'╙', 0b0000 , 0b0000}
  , { L'╛', 0b0000 , 0b0000}
  , { L'╜', 0b0000 , 0b0000}
  , { L'╞', 0b0000 , 0b0000}
  , { L'╟', 0b0000 , 0b0000}
  , { L'╡', 0b0000 , 0b0000}
  , { L'╢', 0b0000 , 0b0000}
  , { L'╤', 0b0000 , 0b0000}
  , { L'╥', 0b0000 , 0b0000}
  , { L'╧', 0b0000 , 0b0000}
  , { L'╨', 0b0000 , 0b0000}
  , { L'╪', 0b0000 , 0b0000}
  , { L'╫', 0b0000 , 0b0000}

  , { L'╴', 0b1000 , 0b0000}
  , { L'╵', 0b0100 , 0b0000}
  , { L'╶', 0b0010 , 0b0000}
  , { L'╷', 0b0001 , 0b0000}

  , { L'╭', 0b1000 , 0b0000}
  , { L'╯', 0b0100 , 0b0000}
  , { L'╮', 0b0010 , 0b0000}
  , { L'╰', 0b0001 , 0b0000}
  };

  std::map<wchar_t, cell> create__lookup__cell() {
    std::map<wchar_t, cell> res;

    for (auto & c : cells) {
      auto [_, success] = res.insert(std::make_pair(c.shape, c));
      assert(success);
    }

    return res;
  }
  std::map<wchar_t, cell> lookup__cell = create__lookup__cell();

  enum connection {
    undecided
  , free
  , no_connection
  , connected_to_single
  , connected_to_double
  };

  struct qc {
    std::size_t freedom ;
    wchar_t     shape   ;
    std::size_t x       ;
    std::size_t y       ;

    qc() 
      : freedom (4)
      , shape   (0)
      , x       (0)
      , y       (0) {
    }
  };

  using qcs = std::array<qc, screen__width*screen__height>;

  connection determine__connection(
      qcs const & res
    , qc const &  sel
    , char        test
    , int         delta__x
    , int         delta__y
    ) {
    auto conn = undecided;
    auto neighbour  = res[(sel.x+delta__x)+(sel.y+delta__y)*screen__width];
    auto shape      = neighbour.shape;
    if (shape == 0) {
      return free;
    } else {
      auto f = lookup__cell.find(shape);
      if (f == lookup__cell.end()) {
        return free;
      }
      auto connection__single = f->second.connection__single;
      auto connection__double = f->second.connection__double;
      assert((connection__single&connection__double) == 0);

      auto has__single = (test & connection__single) != 0;
      auto has__double = (test & connection__double) != 0;
      assert(!(has__single&&has__double));

      if (has__single) {
        return connected_to_single;
      } else if (has__double) {
        return connected_to_double;
      } else {
        return no_connection;
      }
    }
  }

  bool check__candidate(
    cell const &  c
  , connection    conn
  , char          test
  )
  {
    assert((c.connection__single&c.connection__double) == 0);
    switch(conn) {
    case free               :
      return true;
    case no_connection      :
      return (test & c.connection__single) == 0 && (test & c.connection__double) == 0;
    case connected_to_single:
      return (test & c.connection__single) != 0;
    case connected_to_double:
      return (test & c.connection__double) != 0;
    default:
      assert(false);
      return false;
    }
  }

  qcs create__board() {
    qcs res;

    for (std::size_t y = 0; y < screen__height; ++y) {
      auto y__off = y*screen__width;
      for (std::size_t x = 0; x < screen__width; ++x) {
        auto & qc = res[y__off+x];
        qc.x = x;
        qc.y = y;
      }
    }

    std::vector<qc>   candidates__qc;
    std::vector<cell> candidates__cell;
    candidates__qc.reserve(res.size());
    candidates__cell.reserve(128);

    while(true) {
      std::size_t min   = 1000;
      std::size_t open  = 0   ;
      for (std::size_t i = 0; i < res.size(); ++i) {
        auto & qc = res[i];
        if (qc.shape == 0) {
          min = std::min(min, qc.freedom);
          ++open;
        }
      }

      if (open == 0) {
        break;
      }

      candidates__qc.clear();
      for (std::size_t i = 0; i < res.size(); ++i) {
        auto & qc = res[i];
        if (qc.freedom == min && qc.shape == 0) {
          candidates__qc.push_back(qc);
        }
      }

      assert(candidates__qc.size() > 0);
      std::shuffle(candidates__qc.begin(), candidates__qc.end(), random__generator);

      auto sel = candidates__qc.front();
      assert(sel.shape == 0);

      auto  left    = undecided;
      auto  top     = undecided;
      auto  right   = undecided;
      auto  bottom  = undecided;

      if (sel.x == 0) {
        left = free;
      } else {
        left = determine__connection(res, sel, 0b0010, -1, 0);
      }

      if (sel.x >= screen__width - 1) {
        right = free;
      } else {
        right = determine__connection(res, sel, 0b1000, 1, 0);
      }

      if (sel.y == 0) {
        top = free;
      } else {
        top = determine__connection(res, sel, 0b0001, 0, -1);
      }

      if (sel.y >= screen__height - 1) {
        bottom = free;
      } else {
        bottom = determine__connection(res, sel, 0b0100, 0, 1);
      }

      assert(left   != undecided);
      assert(top    != undecided);
      assert(right  != undecided);
      assert(bottom != undecided);

      candidates__cell.clear();
      for (auto & c : cells) {
        auto candidate = true;

        candidate &= check__candidate(c, left   , 0b1000);
        candidate &= check__candidate(c, top    , 0b0100);
        candidate &= check__candidate(c, right  , 0b0010);
        candidate &= check__candidate(c, bottom , 0b0001);

        if (candidate) {
          candidates__cell.push_back(c);
        }
      }

      auto & update = res[sel.x+sel.y*screen__width];

      // assert(candidates__cell.size() > 0);
      if (candidates__cell.size() == 0) {
        update.shape   = L'*';
        update.freedom = 0;
      } else {
        std::shuffle(candidates__cell.begin(), candidates__cell.end(), random__generator);
        auto c = candidates__cell.front();
        update.shape = c.shape;
        update.freedom = 0;
      }

    }

    return res;
  }
  qcs const board = create__board();

  void effect3(float time, screen & screen) {

  /*
  ┌─┬─┐
  │ │ │
  ├─┼─┤
  │ │ │
  └─┴─┘

  ╔╗╔═╗╔═╗╔══╗╔╗╔╗╔╗ ╔══╗╔══╗╔╗
  ╠╣║ ╚╝ ║║╔╗║║║║║║║ ║╔═╝║╔═╝║║
  ║║║ ** ║║╚╝║║║║║║║ ║╚═╗║╚═╗║║
  ║║║╔╗╔╗║║╔═╝║║║║║║ ╚═╗║║╔═╝║║
  ║║║║╚╝║║║║  ║╚╝║║╚╗╔═╝║║╚═╗╠╣
  ╚╝╚╝  ╚╝╚╝  ╚══╝╚═╝╚══╝╚══╝╚╝
  ╭╮╭─╮╭─╮╭──╮╭╮╭╮╭╮ ╭──╮╭──╮╭╮
  ├┤│ ╰╯ ││╭╮│││││││ │╭─╯│╭─╯││
  │││ ** ││╰╯│││││││ │╰─╮│╰─╮││
  │││╭╮╭╮││╭─╯││││││ ╰─╮││╭─╯││
  ││││╰╯││││  │╰╯││╰╮╭─╯││╰─╮├┤
  ╰╯╰╯  ╰╯╰╯  ╰──╯╰─╯╰──╯╰──╯╰╯

  ╔╗╔╧╗╔╩╗╔═╩╗╔╗╔╗╔╗ ╔╧═╗╔═╩╗╔╗
  ╠╣║ ╚╝ ║║╔╗║║║║║║║ ║╔═╝║╔═╝║║
  ╢║║ ** ║║╚╝║║║║║║║ ║╚═╗║╚═╗║║
  ║║║╔╗╔╗║║╔╤╝║║║║║║ ╚═╗║║╔═╝║╠ 
  ╢║║║╚╝║║║║  ║╚╝║║╚╗╔═╝║║╚═╗╠╣
  ╚╝╚╝  ╚╝╚╝  ╚╤═╝╚╦╝╚═╦╝╚╤═╝╚╝
  ┌╮┌┴╮╭╨┐┌─╨╮┌╮╭┐┌╮ ╭──┐╭─╨┐╭┐
  ├┤│ ╰╯ ││╭╮│││││││ │╭─╯│╭─╯││
  ┤││ ** ││└╯│││││││ │└─╮│└─╮││
  │││╭╮╭╮││╭┬╯││││││ ╰─╮││╭─╯│╞
  ┤│││└╯││││  │└╯││└╮╭─┘││└─╮├┤
  └╯└╯  ╰┘└╯  └┬─╯└╥╯└─╥╯╰┬─┘╰┘

  Single lines:     ─ │ ┌ ┐ └ ┘ ├ ┤ ┬ ┴ ┼
  Double lines:     ═ ║ ╔ ╗ ╚ ╝ ╠ ╣ ╦ ╩ ╬
  Mixed doubles:    ╒ ╓ ╕ ╖ ╘ ╙ ╛ ╜ ╞ ╟ ╡ ╢ ╤ ╥ ╧ ╨ ╪ ╫
  Half elements:    ╴ ╵ ╶ ╷
  Bend elements:    ╭ ╯ ╮ ╰
  */

    for (std::size_t y = 0; y < screen.height; ++y) {
      auto y__off = y*screen__width;
      for (std::size_t x = 0; x < screen.width; ++x) {
        auto & qc = board[x+y__off];
        screen.draw__pixel(
            qc.shape
          , vec3 {1,1,1}
          , vec3 {0,0,0}
          , x
          , y
          );
      }
    }
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

  screen screen = make_screen(screen__width, screen__height);

  auto before = GetTickCount64();
  while(true) {

    auto now  = GetTickCount64();
    auto time = (now - before) / 1000.0f;

    screen.clear();

    effect3(time, screen);
    screen.draw__bitmap(border     , time,  0,  0);

    output.clear();
    output.append(prelude);
    write(hstdout, output, screen);
  
    Sleep(20);  // 50 FPS
  }

  return 0;
}
