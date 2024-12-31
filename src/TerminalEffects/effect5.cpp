#include "precompiled.hpp"

#include "screen.hpp"

namespace {

  using stars = std::array<vec3, 200>;

  stars create__stars() {
    stars res;
    for (std::size_t i = 0; i < res.size(); ++i) {
      res[i] = vec3 {
        pick_a_float(-0.5F, 0.5)
      , pick_a_float(-0.5F, 0.5)
      , pick_a_float(-0.5F, 0.5)
      };
    }
    return res;
  }

  stars universe = create__stars();

}

void effect5(float time, screen & screen) {
  float const zf  = 1.0;
  auto hwidth     = 0.5F*screen__width;
  auto hheight    = 0.5F*screen__height;

  auto rot0 = rotator {0.05F};
  auto rot1 = rotator {0.05F*std::sinf(time)};

  for (std::size_t i = 0; i < universe.size(); ++i) {
    auto & star = universe[i];
//    rot0(star.x,star.y);
//    rot1(star.z,star.y);
//    rot0(star.x,star.y);
    star.z -= 0.0125;
  }

  for (std::size_t i = 0; i < universe.size(); ++i) {
    auto & star = universe[i];
    star.x = fractf(star.x+0.5F)-0.5F;
    star.y = fractf(star.y+0.5F)-0.5F;
    star.z = fractf(star.z+0.5F)-0.5F;
  }

  for (std::size_t i = 0; i < universe.size(); ++i) {
    auto star = universe[i];
    auto zz = zf/(zf+star.z);
    int xx = std::roundf(std::floorf(zz*hwidth*star.x)*2)+hwidth;
    int yy = std::roundf(zz*hheight*2*star.y)+hheight;
    auto col = palette(tau*hash(i)).sqrt();
    col *= smoothstep(0.5F, -0.5F, star.z);
    /* █■▪ */
    auto shape = L'▪';
    if (star.z < -0.25F) {
      shape = L'█';
    } else if (star.z < 0.F) {
      shape = L'■';
    }
    /*
    auto shape = L'∘';
    if (star.z < -0.25F) {
      shape = L'◯';
    } else if (star.z < 0.F) {
      shape = L'○';
    }
    */
    screen.draw__pixel(
        shape
      , col
      , vec3 {0,0,0}
      , xx
      , yy
      );
    if (shape == L'█') {
      screen.draw__pixel(
          shape
        , col
        , vec3 {0,0,0}
        , xx+1
        , yy
        );
    }
  }

  auto gcol = palette(-time)*0.05F;

  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*y)/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*x )/(2*screen.height);
      px += 1E-3;
      py += 1E-3;
      wchar_t s;
      vec3    f;
      vec3    b;
      if (screen.get__pixel(s,f,b,x,y)) {
        f += gcol/dotf(px,py);
        b += gcol/dotf(px,py);
        screen.draw__pixel(s,f,b,x,y);
      }
    }
  }
}
