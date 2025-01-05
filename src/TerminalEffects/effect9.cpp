#include "precompiled.hpp"

#include "effect.hpp"

namespace {
  auto gen__col = [](float & fade) { 
    return [&fade](float time, std::size_t x, std::size_t y) -> vec3 { 
      if (fade <= 0.F) {
        return vec3();
      }
      auto col = palette(time-(x+2.0F*y)/20.F);
      col += std::expf(-2.F*std::max(fade-0.125F,0.F));
      return mix(col, vec3(0.025, 0.075, 0.125), smoothstep(0.5F, 1.0F, fade));
    };
  };

  float meditation__fade;
  f__generate_color const meditation__col  = gen__col(meditation__fade);
  bitmap const meditation = make_bitmap(meditation__col, LR"BITMAP(
• ▌ ▄ ·. ▄▄▄ .·▄▄▄▄  ▪  ▄▄▄▄▄ ▄▄▄· ▄▄▄▄▄▪         ▐ ▄ 
·██ ▐███▪▀▄.▀·██▪ ██ ██ •██  ▐█ ▀█ •██  ██ ▪     •█▌▐█
▐█ ▌▐▌▐█·▐▀▀▪▄▐█· ▐█▌▐█· ▐█.▪▄█▀▀█  ▐█.▪▐█· ▄█▀▄ ▐█▐▐▌
██ ██▌▐█▌▐█▄▄▌██. ██ ▐█▌ ▐█▌·▐█ ▪▐▌ ▐█▌·▐█▌▐█▌.▐▌██▐█▌
▀▀  █▪▀▀▀ ▀▀▀ ▀▀▀▀▀• ▀▀▀ ▀▀▀  ▀  ▀  ▀▀▀ ▀▀▀ ▀█▄▀▪▀▀ █▪
)BITMAP");

  float spiritualism__fade;
  f__generate_color const spiritualism__col  = gen__col(spiritualism__fade);
  bitmap const spiritualism = make_bitmap(spiritualism__col, LR"BITMAP(
.▄▄ ·  ▄▄▄·▪  ▄▄▄  ▪  ▄▄▄▄▄▄• ▄▌ ▄▄▄· ▄▄▌  ▪  .▄▄ · • ▌ ▄ ·.     
▐█ ▀. ▐█ ▄███ ▀▄ █·██ •██  █▪██▌▐█ ▀█ ██•  ██ ▐█ ▀. ·██ ▐███▪    
▄▀▀▀█▄ ██▀·▐█·▐▀▀▄ ▐█· ▐█.▪█▌▐█▌▄█▀▀█ ██▪  ▐█·▄▀▀▀█▄▐█ ▌▐▌▐█·    
▐█▄▪▐█▐█▪·•▐█▌▐█•█▌▐█▌ ▐█▌·▐█▄█▌▐█ ▪▐▌▐█▌▐▌▐█▌▐█▄▪▐███ ██▌▐█▌    
 ▀▀▀▀ .▀   ▀▀▀.▀  ▀▀▀▀ ▀▀▀  ▀▀▀  ▀  ▀ .▀▀▀ ▀▀▀ ▀▀▀▀ ▀▀  █▪▀▀▀    
)BITMAP");

  float heart__fade;
  f__generate_color const heart__col  = [](float time, std::size_t x, std::size_t y) -> vec3 {
    auto col = hsv2rgb_approx(
        0.95
      , std::clamp(0.33F+ (0.125F*x+y)/12.F, 0.F, 1.F)
      , std::clamp(1.0F-y/20.F, 0.F, 1.F)
      );
    col += std::expf(-4.F*std::max(heart__fade-0.125F,0.F));
    return col;
  };
  bitmap const heart = make_bitmap(heart__col, LR"BITMAP(
   ▓▓▓▓▓▓▓▓▓                 ▓▓▓▓▓▓▓▓▓
  ▓█████████▓               ▓█████████▓
▓██████████████▓         ▓██████████████▓
▓████████████████▓     ▓████████████████▓
▓█████████████████▓▓▓▓▓█████████████████▓
▓███████████████████████████████████████▓
 ▓█████████████████████████████████████▓
   ▓█████████████████████████████████▓
     ▓█████████████████████████████▓
       ▓█████████████████████████▓
         ▓█████████████████████▓
           ▓█████████████████▓
             ▓█████████████▓
               ▓█████████▓
                 ▓█████▓
                   ▓▓▓
)BITMAP");
}

void effect9(effect_input const & ei) {
  meditation__fade = linstep(music__from_nbeat(ei.beat__start), music__from_nbeat(ei.beat__start+5), ei.time);
  ei.screen.draw__bitmap(meditation, ei.time, 9, 4);
  spiritualism__fade = linstep(music__from_nbeat(ei.beat__start+3), music__from_nbeat(ei.beat__start+8), ei.time);
  ei.screen.draw__bitmap(spiritualism, ei.time, 7, 19);

  if (music__nbeat(ei.time) > ei.beat__start + 7) {
    heart__fade = linstep(music__from_nbeat(ei.beat__start+7), music__from_nbeat(ei.beat__start+12), ei.time);
    ei.screen.draw__bitmap(heart, ei.time, 16, 6);
  }
}
