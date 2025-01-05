#include "precompiled.hpp"

#include "effect.hpp"

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
}

void effect1(effect_input const & ei) {
  auto time = ei.time;

  auto df = [](float x, float y) -> float {
    const float m = 0.5;
    float l = lengthf(x,y);
    l = std::fmodf(l+(0.5F*m),m)-(0.5F*m);
    return std::abs(l)-(m*0.25F);
  };

  for (std::size_t y = 0; y < ei.screen.height; ++y) {
    auto py = (-1.F*ei.screen.height+2.F*(y+0.5F))/ei.screen.height;
    for (std::size_t x = 0; x < ei.screen.width; ++x) {
      auto px = (-1.F*ei.screen.width+2.F*(x+0.5F))/(2*ei.screen.height);

      auto px0 = px;
      auto py0 = py;

      px0 -= std::sinf(0.707f*(time+100));
      py0 -= std::sinf((time+100));

      auto px1 = px;
      auto py1 = py;

      px1 -= std::sinf(0.5F*(time+123));
      py1 -= std::sinf(0.707F*(time+123));
      float sm = 0.125F*lengthf(px, py);

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
      col *= smoothstep(-0.5F, 0.5F, -std::cosf(time-py-0.25F*px*px));
      ei.screen.draw__pixel(
          L' '
        , vec3{0,0,0}
        , col
        , x
        , y
        );
    }
  }
  ei.screen.draw__bitmap(impulse2  , time, 8, 6);
//    screen.draw__bitmap(gerp        , time, 5, 6);

}
