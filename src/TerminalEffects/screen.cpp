#include "precompiled.hpp"

#include "screen.hpp"

namespace {
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