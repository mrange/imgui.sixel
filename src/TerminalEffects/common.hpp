#pragma once

#include "precompiled.hpp"

static_assert(sizeof(char) == sizeof(char8_t), "Must be same size");
float       const pi              = 3.141592654F;
float       const tau             = 2*pi;
std::size_t const screen__width   = 80;
std::size_t const screen__height  = 30;
float       const end__time       = 1E6;

std::size_t pick_a_number(std::size_t min, std::size_t max);

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

inline float smoothstep(float edge0, float edge1, float x) {
  float t = std::clamp<float>((x - edge0) / (edge1 - edge0), 0.0F, 1.0F);
  return t * t * (3.0F - 2.0F * t);
}

// License: Unknown, author: Claude Brezinski, found: https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
inline float tanh_approxf(float x) {
  //  Found this somewhere on the interwebs
  //  return tanh(x);
  float x2 = x*x;
  return std::clamp<float>(x*(27 + x2)/(27+9*x2), -1, 1);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
inline float pmin(float a, float b, float k) {
  float h = std::clamp<float>(0.5F+0.5F*(b-a)/k, 0.0F, 1.0F);
  return mix(b, a, h) - k*h*(1.0F-h);
}

// License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
inline float pmax(float a, float b, float k) {
  return -pmin(-a, -b, k);
}

// License: Unknown, author: Unknown, found: don't remember
inline float hash(float co) {
  return fractf(sinf(co*12.9898F) * 13758.5453F);
}

