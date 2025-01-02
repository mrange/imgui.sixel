#include "precompiled.hpp"

#include "effect.hpp"

namespace {
  bitmap const code_by = make_bitmap(col__white, LR"BITMAP(
_________            .___       ___.          
╲_   ___ ╲  ____   __| _╱____   ╲_ |__ ___.__.
╱    ╲  ╲╱ ╱  _ ╲ ╱ __ |╱ __ ╲   | __ <   |  |
╲     ╲___(  <_> ) ╱_╱ ╲  ___╱   | ╲_╲ ╲___  |
 ╲______  ╱╲____╱╲____ |╲___  >  |___  ╱ ____|
        ╲╱            ╲╱    ╲╱       ╲╱╲╱     
)BITMAP");

  bitmap const gfx_by = make_bitmap(col__white, LR"BITMAP(
  ________                    .__    .__                ___.          
 ╱  _____╱___________  ______ |  |__ |__| ____   ______ ╲_ |__ ___.__.
╱   ╲  __╲_  __ ╲__  ╲ ╲____ ╲|  |  ╲|  |╱ ___╲ ╱  ___╱  | __ <   |  |
╲    ╲_╲  ╲  | ╲╱╱ __ ╲|  |_> >   Y  ╲  ╲  ╲___ ╲___ ╲   | ╲_╲ ╲___  |
 ╲______  ╱__|  (____  ╱   __╱|___|  ╱__|╲___  >____  >  |___  ╱ ____|
        ╲╱           ╲╱|__|        ╲╱        ╲╱     ╲╱       ╲╱╲╱     
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

}

void effect2(float time, std::size_t beat__start, std::size_t beat__end, screen & screen) {

  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*(y+0.5F))/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*(x+0.5F))/(2*screen.height);

      auto p = vec2 {px, py};
      auto h0 = hash(p+std::floorf(-0.25F*time+py*py+0.33F*hash(p)));

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
        , hsv2rgb_approx(std::sinf(time*0.707F)*px*py+0.5F*py*py-0.5F*time, mix(1,0.5F,py*py), 1+py*py)*(smoothstep(0,1,py*py))
        , vec3 {0,0,0}
        , x
        , y
        );
    }
  }

  draw__border(time, screen);

  auto selection= (music__nbeat(time)-beat__start)/16;
  auto nbeat    = selection*16+beat__start;
  auto opacity  = std::expf(-0.7*std::max(0.F, time-music__from_nbeat(nbeat+2)));

  switch(selection) {
  case 0:
    screen.draw__bitmap(gfx_by, time, 4, 12, opacity);
    break;
  case 1:
    screen.draw__bitmap(glimglam, time, 12, 12, opacity);
    break;
  case 2:
    screen.draw__bitmap(code_by, time, 15, 12, opacity);
    break;
  case 3:
    screen.draw__bitmap(lance, time, 4, 9, opacity);
    break;
  default:
    break;
  }

  { 
    auto gcol = palette(-time*tau/(music__beat_time*16)-2)*0.025F;
    auto const sub = vec3(2,3,1)*0.0033;
    float start = music__from_nbeat(beat__start);
    float end   = music__from_nbeat(beat__start+4);
    float fade  = smoothstep(end, start, time);
    screen.apply_to_all([fade,sub,gcol](auto x, auto y, auto p, auto& s, auto& f, auto& b) {
      f += fade;
      b += fade;
      auto dot = (p*vec2 {1,2}).length2();
      auto add = gcol/std::max(dot, 1E-3F);
      add -= sub*dot;
      f += add;
      b += add;
    });
  }
}
