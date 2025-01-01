#include "precompiled.hpp"

#include "effect.hpp"

/*
* 
      
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
*/

namespace {
  bitmap const gerp = make_bitmap(col__white, LR"BITMAP(
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
░░░░░░░░      ░░░░░░░░░        ░░░░░░░░       ░░░░░░░░░       ░░░░░░░░
▒▒▒▒▒▒▒  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒  ▒▒▒▒▒▒▒▒▒▒▒▒▒▒  ▒▒▒▒  ▒▒▒▒▒▒▒▒  ▒▒▒▒  ▒▒▒▒▒▒▒
▓▓▓▓▓▓▓  ▓▓▓   ▓▓▓▓▓▓▓▓      ▓▓▓▓▓▓▓▓▓▓       ▓▓▓▓▓▓▓▓▓       ▓▓▓▓▓▓▓▓
███████  ████  ████████  ██████████████  ███  █████████  █████████████
████████      █████████        ████████  ████  ████████  █████████████
██████████████████████████████████████████████████████████████████████
)BITMAP");          

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

void effect7(float time, std::size_t beat__start, std::size_t beat__end, screen & screen) {
  auto rot = rotator {time};
  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*(y+0.5F))/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*(x+0.5F) )/(2*screen.height);
      auto p = vec2 {px, py};

      auto vp= p;
      auto constexpr per = tau/6;
      auto constexpr amp = 8.0F;
      vp.x += amp*std::sinf(time*(per/amp));
      vp.y += amp*std::sinf(time*(per*0.707F/amp));
      auto n = vnoise(vp);

      auto mp = vec2 { p.y, p.x };
      mp.x -= 0.25;
      mp *= 0.4;
      auto mmp = mandelmap(mp, mp);

      auto pp = mmp;

//      rot(pp.x, pp.y);
      pp *= (0.5F+n);
      pp.x += 0.13F*time;
      auto np = pp.round();
      auto cp = pp-np;

      auto ml = mmp.length();

      auto col = vec3 {0,0,0};

      if (ml < 2.) {
        auto d = cp.length()-0.5F*n;
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

  screen.draw__bitmap(gerp, time, 5, 12);

  {
    wchar_t s;
    vec3    f;
    vec3    b;
    wchar_t const t2025[] = L"2025";
    for (std::size_t i = 0; i < 4; ++i) {
      auto c = t2025[i];
      if (screen.get__pixel(
          s
        , f
        , b
        , 67+i*2
        , 18
        )) {
        screen.draw__pixel(
            c
          , b
          , vec3 {1,1,1}
          , 67+i*2
          , 18
          );
      }
    }
  }

  draw__border(time, screen);
}
