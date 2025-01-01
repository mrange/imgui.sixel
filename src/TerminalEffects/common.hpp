#pragma once

#include "precompiled.hpp"

static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");
float       constexpr pi              = 3.141592654F;
float       constexpr tau             = 2*pi;
std::size_t constexpr screen__width   = 80;
std::size_t constexpr screen__height  = 30;
float       constexpr end__time       = 1E6;

float       constexpr music__bpm              = 80;
float       constexpr music__beat_time        = music__bpm/60;
float       constexpr music__subdivision_time = music__beat_time/4;
float       constexpr music__bar_time         = music__beat_time*4;

int         music__nbeat        (float time);
int         music__nsubdivision (float time);
int         music__nbar         (float time);
float       music__fbeat        (float time);
float       music__fsubdivision (float time);
float       music__fbar         (float time);

std::size_t pick_a_number(std::size_t min, std::size_t max);

float pick_a_float(float min, float max);

inline float dotf(float x, float y) {
  return std::sqrtf(x*x+y*y);
}

inline float lengthf(float x, float y) {
  return std::sqrtf(dotf(x,y));
}

inline float fractf(float x) {
  return x-std::floorf(x);
}

inline float roundf(float x) {
  return std::floorf(x+0.5F);
}

inline float mix(float b, float e, float x) {
  return b+(e-b)*x;
}

// License: Unknown, author: Unknown, found: don't remember
inline float hash(float co) {
  return fractf(sinf(co*12.9898F) * 13758.5453F);
}

float smoothstep(float edge0, float edge1, float x);

// License: Unknown, author: Claude Brezinski, found: https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
float tanh_approxf(float x);

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
float pmin(float a, float b, float k);

// License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
float pmax(float a, float b, float k);

struct rotator {
  float const c;
  float const s;
  explicit rotator(float a) 
  : c(std::cosf(a))
  , s(std::sinf(a)) {
  }

  inline void operator()(float & x, float & y) const noexcept {
    auto xx = c*x+s*y;
    auto yy = -s*x+c*y;
    x = xx;
    y = yy;
  }

};


