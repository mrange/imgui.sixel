#include "precompiled.hpp"

#include "screen.hpp"

namespace {
  vec2 mandelmap(vec2 p, vec2 c) {
    auto z = p;
    for (std::size_t i = 0; i < 8; ++i) {
      auto zx = z.x*z.x-z.y*z.y+c.x;
      auto zy = 2*z.x*z.y+c.y;
      z.x = zx;
      z.y = zy;
    }
    return z;
  }
}

void effect7(float time, screen & screen) {
  auto rot = rotator {time};
  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*(y+0.5F))/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*(x+0.5F) )/(2*screen.height);
      auto p = vec2 {px, py};

      auto mp = vec2 { p.y, p.x };
      mp.x -= 0.25;
      mp *= 0.4;
      auto mmp = mandelmap(mp, mp);

      auto pp = mmp;

//      rot(pp.x, pp.y);
      pp.x += 0.33*time;
      pp *= 1.0;
      auto np = pp.round();
      auto cp = pp-np;

      auto ml = mmp.length();

      auto col = vec3 {0,0,0};

      if (ml < 2.) {
        auto d = cp.length()-0.35F;
        col = palette(d+time)*0.001/std::max(d*d, 0.0001F);
      }


      screen.draw__pixel(
          L' '
        , vec3 {0,0,0}
        , col
        , x
        , y
        );
    }
  }
}
