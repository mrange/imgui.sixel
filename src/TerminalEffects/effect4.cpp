#include "precompiled.hpp"

#include "screen.hpp"

namespace {

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


  float vnoise(vec2 p) {
    auto i = p.floor();
    auto f = p.fract();

    auto u = f*f*(f*-2+3);

    auto a = hash(i + vec2(0,0));
    auto b = hash(i + vec2(1,0));
    auto c = hash(i + vec2(0,1));
    auto d = hash(i + vec2(1,1));

    auto m0 = mix(a, b, u.x);
    auto m1 = mix(c, d, u.x);
    auto m2 = mix(m0, m1, u.y);

    return m2;
  }

}

void effect4(float time, screen & screen) {
  auto const rot1 = rotator {-1};
  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*y)/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*x )/(2*screen.height);
      auto p = vec2 {px, py};
      p += 1E-3;

      auto l = p.length();
      auto r = rotator {time-l};
      r(p.x, p.y);
      
      p *= std::expf(-time);
      auto ang= std::atan2(p.y, p.x);

      auto col = vec3 {0,0,0};
      if (ang < pi-2E-1) {
        auto vp = vec2 { std::logf(l), ang*6.F/tau };
  //      auto vp = p;
        auto h = 0.F;
        auto a = 1.0F;
        for (std::size_t i = 0; i < 3; ++i) {
          h   += a*vnoise(vp);
          a   *= 0.5F;
          vp  *= 2.03F;
          rot1(vp.x, vp.y);
          vp  += 1.234;
        }
        col = palette(h-time+lengthf(px, py));
      } else {
        col = palette(time).sqrt();
      }

      col += 0.05;
      col *= 1.F/l;
      col -= 0.25*l;
      col = aces_approx(col);
      screen.draw__pixel(
          L' '
        , vec3 {0,0,0}
        , col
        , x
        , y
        );
    }
  }
  screen.draw__bitmap(impulse2  , time, 8, 6);
}
