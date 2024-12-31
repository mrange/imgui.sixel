#include "precompiled.hpp"

#include "screen.hpp"

namespace {
  bitmap const code_by = make_bitmap(col__white, LR"BITMAP(
_________            .___       ___.          
╲_   ___ ╲  ____   __| _╱____   ╲_ |__ ___.__.
╱    ╲  ╲╱ ╱  _ ╲ ╱ __ |╱ __ ╲   | __ <   |  |
╲     ╲___(  <_> ) ╱_╱ ╲  ___╱   | ╲_╲ ╲___  |
 ╲______  ╱╲____╱╲____ |╲___  >  |___  ╱ ____|
        ╲╱            ╲╱    ╲╱       ╲╱╲╱     
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



void effect2(float time, screen & screen) {

  for (std::size_t y = 0; y < screen.height; ++y) {
    auto py = (-1.F*screen.height+2.F*y)/screen.height;
    for (std::size_t x = 0; x < screen.width; ++x) {
      auto px = (-1.F*screen.width+2.F*x)/(2*screen.height);

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

  auto sel = std::fmodf(std::floorf(time/tau), 2.0F);
  auto opacity = smoothstep(0.5F, 0.707F, -std::cosf(time));

  if (sel == 0) {
    screen.draw__bitmap(code_by, time, 14, 12, opacity);
  } else {
    screen.draw__bitmap(lance, time, 4, 9, opacity);
  }
}
