﻿#pragma once

//  col = floor(vec3(8,8,4)*col)/vec3(8,8,4);

namespace {
  char const vertex_shader__basic[] = R"SHADER(
#version 300 es
layout (location = 0) in vec3 aPos;
void main() {
  gl_Position = vec4(aPos.x, -aPos.y, aPos.z, 1.0);
}
)SHADER";

  char const fragment_shader__mandelbox[] = R"SHADER(
#version 300 es
#define USE_UNIFORMS

precision highp float;

uniform float time;
uniform vec2 resolution;

#ifdef USE_UNIFORMS
uniform vec4 state;

float beat() {
  return state.x;
}

float fade_in() {
  return state.y;
}

float fade_out() {
  return state.z;
}

float fade() {
  return state.w;
}


#else
const float BPM = 145.0/60.;

float fade_in() {
  return 0.;
}

float fade_out() {
  return 0.;
}

float fade() {
  return 1.;
//  return smoothstep(-0.707, 0.707, sin(time));
}

float beat() {
  return exp(-1.*fract(0.25+0.5*time*BPM));
}
#endif

const float fixed_radius2 = 1.9;
const float min_radius2 = 0.1;
const float folding_limit = 1.0;
const float scale = -2.2;

out vec4 fragColor;

#define TIME        time
#define RESOLUTION  resolution
#define PI          3.141592654
#define TAU         (2.0*PI)


//#define THEME0
#define THEME1

#define TOLERANCE       0.001
#define NORMTOL         0.00125
#define MAX_RAY_LENGTH  15.0
#define MAX_RAY_MARCHES 90
#define ROT(a)          mat2(cos(a), sin(a), -sin(a), cos(a))
#define PI              3.141592654
#define TAU             (2.0*PI)

// https://stackoverflow.com/a/17897228/418488
const vec4 hsv2rgb_K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
vec3 hsv2rgb(vec3 c) {
  vec3 p = abs(fract(c.xxx + hsv2rgb_K.xyz) * 6.0 - hsv2rgb_K.www);
  return c.z * mix(hsv2rgb_K.xxx, clamp(p - hsv2rgb_K.xxx, 0.0, 1.0), c.y);
}
#define HSV2RGB(c)  (c.z * mix(hsv2rgb_K.xxx, clamp(abs(fract(c.xxx + hsv2rgb_K.xyz) * 6.0 - hsv2rgb_K.www) - hsv2rgb_K.xxx, 0.0, 1.0), c.y))

mat3 g_rot;

float g_quad = 0.0;
float g_beat = 0.0;

// License: Unknown, author: Unknown, found: don't remember
float tanh_approx(float x) {
//  return tanh(x);
  float x2 = x*x;
  return clamp(x*(27.0 + x2)/(27.0+9.0*x2), -1.0, 1.0);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/spherefunctions/spherefunctions.htm
float sphered(vec3 ro, vec3 rd, vec4 sph, float dbuffer) {
  float ndbuffer = dbuffer/sph.w;
  vec3  rc = (ro - sph.xyz)/sph.w;

  float b = dot(rd,rc);
  float c = dot(rc,rc) - 1.0;
  float h = b*b - c;
  if( h<0.0 ) return 0.0;
  h = sqrt( h );
  float t1 = -b - h;
  float t2 = -b + h;

  if( t2<0.0 || t1>ndbuffer ) return 0.0;
  t1 = max( t1, 0.0 );
  t2 = min( t2, ndbuffer );

  float i1 = -(c*t1 + b*t1*t1 + t1*t1*t1/3.0);
  float i2 = -(c*t2 + b*t2*t2 + t2*t2*t2/3.0);
  return (i2-i1)*(3.0/4.0);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
float pmin(float a, float b, float k) {
  float h = clamp(0.5+0.5*(b-a)/k, 0.0, 1.0);
  return mix(b, a, h) - k*h*(1.0-h);
}

// License: CC0, author: Mårten Rånge, found: https://github.com/mrange/glsl-snippets
float pmax(float a, float b, float k) {
  return -pmin(-a, -b, k);
}

// Create a quaternion from axis and angle
vec4 createQuaternion(vec3 axis, float angle) {
  float halfAngle = angle * 0.5;
  float s = sin(halfAngle);
  return vec4(axis * s, cos(halfAngle));
}

// Quaternion multiplication
vec4 multiplyQuaternions(vec4 q1, vec4 q2) {
  return vec4(
    q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
    q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
    q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
    q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
  );
}

// Rotate a vector using a quaternion
mat3 rotationFromQuaternion(vec4 q) {
  // Convert quaternion to a rotation matrix
  mat3 rotationMatrix = mat3(
    1.0 - 2.0 * (q.y * q.y + q.z * q.z),
    2.0 * (q.x * q.y - q.w * q.z),
    2.0 * (q.x * q.z + q.w * q.y),

    2.0 * (q.x * q.y + q.w * q.z),
    1.0 - 2.0 * (q.x * q.x + q.z * q.z),
    2.0 * (q.y * q.z - q.w * q.x),

    2.0 * (q.x * q.z - q.w * q.y),
    2.0 * (q.y * q.z + q.w * q.x),
    1.0 - 2.0 * (q.x * q.x + q.y * q.y)
  );

  return rotationMatrix;
}

// -------------------------------------------------
// Mandelbox - https://www.shadertoy.com/view/XdlSD4


void sphere_fold(inout vec3 z, inout float dz) {
  float r2 = dot(z, z);
  if(r2 < min_radius2) {
    float temp = (fixed_radius2 / min_radius2);
    z *= temp;
    dz *= temp;
  } else if(r2 < fixed_radius2) {
    float temp = (fixed_radius2 / r2);
    z *= temp;
    dz *= temp;
  }
}

void box_fold(inout vec3 z, inout float dz) {
  z = clamp(z, -folding_limit, folding_limit) * 2.0 - z;
}

float mb(vec3 z) {
  float dd = length(z)-mix(2., 2.3, g_beat);
  vec3 offset = z;
  float dr = 1.0;

  for(int n = 0; n < 5; ++n) {
    box_fold(z, dr);
    sphere_fold(z, dr);

    z = scale * z + offset;
    dr = dr * abs(scale) + 1.5;
  }

  float d = (length(z))/abs(dr)-0.04;
  dd = pmax(dd, -d, 0.5);
  if (dd < d) {
    g_quad = -1.0;
  } else {
    g_quad = 1.0;
  }
  g_quad = -g_quad;
  d = min(d, dd);

  return d;
}
// -------------------------------------------------


float df(vec3 p) {
  p *= g_rot;
  float d = mb(p);
  return d;
}

float rayMarch(in vec3 ro, in vec3 rd, out int iter) {
  float t = 2.0;
  int i = 0;
  for (i = 0; i < MAX_RAY_MARCHES; i++) {
    float d = df(ro + rd*t);
    if (d < TOLERANCE || t > MAX_RAY_LENGTH) break;
    t += d;
  }
  iter = i;
  return t;
}

vec3 normal(vec3 pos) {
  vec2  eps = vec2(NORMTOL, 0.0);
  vec3 nor;
  nor.x = df(pos+eps.xyy) - df(pos-eps.xyy);
  nor.y = df(pos+eps.yxy) - df(pos-eps.yxy);
  nor.z = df(pos+eps.yyx) - df(pos-eps.yyx);
  return normalize(nor);
}

float softShadow(in vec3 pos, in vec3 ld, in float ll, float mint, float k) {
  const float minShadow = 0.25;
  float res = 1.0;
  float t = mint;
  for (int i=0; i<25; ++i) {
    float distance = df(pos + ld*t);
    res = min(res, k*distance/t);
    if (ll <= t) break;
    if(res <= minShadow) break;
    t += max(mint*0.2, distance);
  }
  return clamp(res,minShadow,1.0);
}

vec3 render(vec3 ro, vec3 rd) {
  const vec3 lightPos0  = 2.5*vec3(1.0, 1.0, 1.0);
  const vec3 lightPos1  = vec3(0.0);

  const vec3 scol = HSV2RGB(vec3(0.0, 0.95, 0.005));
  vec3 skyCol = vec3(0.0);
  float a = atan(rd.x, rd.z);

  int iter = 0;
  float t = rayMarch(ro, rd, iter);
  float quad = g_quad;

  float tt = float(iter)/float(MAX_RAY_MARCHES);
  float bs = 1.0-tt*tt*tt*tt;

  vec3 pos = ro + t*rd;
  float beat = g_beat;
  float lsd1  = sphered(ro, rd, vec4(lightPos1, mix(2.0, 2.8, beat*beat*beat)), t);

  const vec3 bcol0 = HSV2RGB(vec3(0.6, 0.6, 3.0));
  const vec3 bcol1 = HSV2RGB(vec3(0.55, 0.8, 7.0));
  vec3 bcol   = mix(bcol0, bcol1, beat);
  vec3 gcol   = lsd1*bcol;

  if (t >= MAX_RAY_LENGTH) {
    return skyCol+gcol;
  }

  float d     = df(pos);
  vec3 nor    = normal(pos);
  float fre   = 1.0+dot(nor, rd);
  fre *= fre;
  fre *= fre;

  vec3 lv0    = lightPos0 - pos;
  float ll20  = dot(lv0, lv0);
  float ll0   = sqrt(ll20);
  vec3 ld0    = lv0 / ll0;
  float dm0   = 8.0/ll20;
  float sha0  = softShadow(pos, ld0, ll0, 0.125, 32.0);
  float dif0  = max(dot(nor,ld0),0.0)*dm0;
  float spe0  = pow(max(dot(reflect(rd, nor), ld0), 0.), 10.0);


  vec3 col = vec3(0.0);
  quad = -quad;
  const vec3 black = vec3(0.0);
#if defined(THEME0)
  const vec3 dcol0 = HSV2RGB(vec3(0.6, 0.5, 1.0));
  const vec3 dcol1 = black;
#elif defined(THEME1)
  const vec3 dcol0 = HSV2RGB(vec3(0.55 , 0.5, -0.025));
  const vec3 dcol1 = HSV2RGB(vec3(0.08, 1.0, 1.0));
#else
  const vec3 dcol0 = black;
  const vec3 dcol1 = dcol0;
#endif
  col += dif0*dif0*sha0*mix(dcol0, dcol1, 0.5+0.5*quad);
  col += spe0*bcol*bs*sha0;
  col += gcol;

  return col;
}

// License: Unknown, author: Unknown, found: don't remember
float hash(vec2 co) {
  return fract(sin(dot(co.xy ,vec2(12.9898,58.233))) * 13758.5453);
}

vec3 palette(float a) {
  return 0.5*(1.+sin(vec3(0,1,2)+a));
}


vec3 effect(vec2 p) {
  g_beat = beat();
  const vec2 csz = 1.0/vec2(80.,30.);
  vec2 np = round(p/csz)*csz;
  float fade = fade();
  float h0 = hash(np);
  float f0 = -0.1*h0+0.5*(np.y*np.y+np.x*np.x)+mix(-1.4, 0.6, fade);
  if (f0 < 0.) {
    p = np;
  }
  p *= ROT(-PI*length(p)*fade_out());
  const vec3 cam  = 5.0*vec3(1.0, 0.5, 1.0);
  const vec3 dcam = normalize(vec3(0.0) - cam);
  const vec3 ro = cam;
  const vec3 ww = dcam;
  const vec3 uu = normalize(cross(vec3(0.0,1.0,0.0), ww));
  const vec3 vv = cross(ww,uu);
  const float rdd = 2.0;
  vec3 rd = normalize(-p.x*uu + p.y*vv + rdd*ww);

  vec4 q = createQuaternion(normalize(vec3(1.0,sin(0.33*time),sin(0.707*time))), time);
  //vec4 q = createQuaternion(normalize(cam.zxy), time);
  g_rot = rotationFromQuaternion(q);
  vec3 col = render(ro, rd);
  col += 3.*(palette(time+(np.x+np.y)*PI/2.+PI)+f0)*exp(-10.*f0)*step(0.,f0);

  return col+3.*fade_in();
}

// License: Unknown, author: Matt Taylor (https://github.com/64), found: https://64.github.io/tonemapping/
vec3 aces_approx(vec3 v) {
  v = max(v, 0.0);
  v *= 0.6;
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0, 1.0);
}

void main() {
  vec2 q = gl_FragCoord.xy/RESOLUTION.xy;
  vec2 p = -1.0 + 2.0*q;
  p.x *= RESOLUTION.x/RESOLUTION.y;
  vec3 col = effect(p);
  col = aces_approx(col);
  col = sqrt(col);

  fragColor = vec4(col, 1.0);
}
)SHADER";

  char const fragment_shader__bw[] = R"SHADER(
#version 300 es
#define USE_UNIFORMS

precision highp float;

uniform float time;
uniform vec2 resolution;

out vec4 fragColor;

#ifdef USE_UNIFORMS
uniform vec4 state;

float beat() {
  return state.x;
}

float fade_in() {
  return state.y;
}

float fade_out() {
  return state.z;
}

float fade() {
  return state.w;
}

#else
const float BPM = 145.0/60.;

float fade_in() {
  return 0.;
}

float fade_out() {
  return 0.;
}

float fade() {
  return smoothstep(-0.707, 0.707, sin(time));
}

float beat() {
  return exp(-1.*fract(0.25+0.5*time*BPM));
}
#endif


#define TIME        time
#define RESOLUTION  resolution

// License CC0: Flying through kaleidoscoped truchet patterns
// Experimenting with simple truchet patterns + kaleidoscope turned out rather nice
//  so I wanted to share.

// SABS by ollj

#define PI              3.141592654
#define TAU             (2.0*PI)
#define TIME            time
#define RESOLUTION      resolution
#define LESS(a,b,c)     mix(a,b,step(0.,c))
#define SABS(x,k)       LESS((.5/(k))*(x)*(x)+(k)*.5,abs(x),abs(x)-(k))
#define ROT(a)          mat2(cos(a), sin(a), -sin(a), cos(a))
#define BPM             145.0

const vec3 std_gamma        = vec3(2.2, 2.2, 2.2);

float hash(float co) {
  return fract(sin(co*12.9898) * 13758.5453);
}

float hash(vec3 co) {
  return fract(sin(dot(co, vec3(12.9898,58.233, 12.9898+58.233))) * 13758.5453);
}

vec2 toPolar(vec2 p) {
  return vec2(length(p), atan(p.y, p.x));
}

vec2 toRect(vec2 p) {
  return vec2(p.x*cos(p.y), p.x*sin(p.y));
}

vec2 mod2_1(inout vec2 p) {
  vec2 c = floor(p + 0.5);
  p = fract(p + 0.5) - 0.5;
  return c;
}

float modMirror1(inout float p, float size) {
  float halfsize = size*0.5;
  float c = floor((p + halfsize)/size);
  p = mod(p + halfsize,size) - halfsize;
  p *= mod(c, 2.0)*2.0 - 1.0;
  return c;
}

float smoothKaleidoscope(inout vec2 p, float sm, float rep) {
  vec2 hp = p;

  vec2 hpp = toPolar(hp);
  float rn = modMirror1(hpp.y, TAU/rep);

  float sa = PI/rep - SABS(PI/rep - abs(hpp.y), sm);
  hpp.y = sign(hpp.y)*(sa);

  hp = toRect(hpp);

  p = hp;

  return rn;
}

vec3 toScreenSpace(vec3 col) {
  return pow(col, 1.0/std_gamma);
}

vec3 alphaBlend(vec3 back, vec4 front) {
  vec3 colb = back.xyz;
  vec3 colf = front.xyz;
  vec3 xyz = mix(colb, colf.xyz, front.w);
  return xyz;
}

float circle(vec2 p, float r) {
  return length(p) - r;
}

vec3 offset(float z) {
  float a = z;
  vec2 p = -0.075*(vec2(cos(a), sin(a*sqrt(2.0))) + vec2(cos(a*sqrt(0.75)), sin(a*sqrt(0.5))));
  return vec3(p, z);
}

vec3 doffset(float z) {
  float eps = 0.1;
  return 0.5*(offset(z + eps) - offset(z - eps))/eps;
}

vec3 ddoffset(float z) {
  float eps = 0.1;
  return 0.125*(doffset(z + eps) - doffset(z - eps))/eps;
}

// -----------------------------------------------------------------------------
// PLANE0__BEGIN
// -----------------------------------------------------------------------------

const float plane0_lw = 0.05;

const mat2[] plane0_rots = mat2[](ROT(0.0*PI/2.0), ROT(1.00*PI/2.0), ROT(2.0*PI/2.0), ROT(3.0*PI/2.0));

vec2 plane0_cell0(vec2 p, float h) {
  float d0  = circle(p-vec2(0.5), 0.5);
  float d1  = circle(p+vec2(0.5), 0.5);

  float d = 1E6;
  d = min(d, d0);
  d = min(d, d1);
  return vec2(d, 1E6); // 1E6 gives a nice looking bug, 1E4 produces a more "correct" result
}

vec2 plane0_cell1(vec2 p, float h) {
  float d0  = abs(p.x);
  float d1  = abs(p.y);
  float d2  = circle(p, mix(0.2, 0.4, h));

  float d = 1E6;
  d = min(d, d0);
  d = min(d, d1);
  d = min(d, d2);
  return vec2(d, d2+plane0_lw);
}

vec2 plane0_df(vec3 pp, float h, out vec3 n) {
  vec2 p = pp.xy*ROT(TAU*h+TIME*fract(23.0*h)*0.5);
  float hd = circle(p, 0.4);

  vec2 hp = p;
  float rep = 2.0*floor(mix(5.0, 25.0, fract(h*13.0)));
  float sm = mix(0.05, 0.125, fract(h*17.0))*24.0/rep;
  float kn = 0.0;
  kn = smoothKaleidoscope(hp, sm, rep);
  vec2 hn = mod2_1(hp);
  float r = hash(vec3(hn, h));

  hp *= plane0_rots[int(r*4.0)];
  float rr = fract(r*31.0);
  vec2 cd0 = plane0_cell0(hp, rr);
  vec2 cd1 = plane0_cell1(hp, rr);
  vec2 d0 = mix(cd0, cd1, vec2(fract(r*13.0) > 0.5));

  hd = min(hd, d0.y);

  float d = 1E6;
  d = min(d, d0.x);
  d = abs(d) - plane0_lw;
  d = min(d, hd - plane0_lw*2.0);

  n = vec3(hn, kn);

  return vec2(hd, d);
}

vec4 plane0(vec3 ro, vec3 rd, vec3 pp, vec3 off, float aa, float n) {

  float h = hash(n);
  float s = mix(0.05, 0.25, h);

  vec3 hn;
  vec3 p = pp-off*vec3(1.0, 1.0, 0.0);

  vec2 dd = plane0_df(p/s, h, hn)*s;
  float d = dd.y;

  float a  = smoothstep(-aa, aa, -d);
  float ha = smoothstep(-aa, aa, dd.x);

  vec4 col = vec4(mix(vec3(1.0), vec3(0.0), a), ha);

  return col;
}

// -----------------------------------------------------------------------------
// PLANE0__END
// -----------------------------------------------------------------------------

vec4 plane(vec3 ro, vec3 rd, vec3 pp, vec3 off, float aa, float n) {
  return plane0(ro, rd, pp, off, aa, n);
}

vec3 skyColor(vec3 ro, vec3 rd) {
  float ld = max(dot(rd, vec3(0.0, 0.0, 1.0)), 0.0);
  return vec3(tanh(3.0*pow(ld, 100.0)));
}

float psin(float x) {
  return 0.5+0.5*sin(x);
}
vec3 color(vec3 ww, vec3 uu, vec3 vv, vec3 ro, vec2 p) {
  const float per = 40.;
  float lp = length(p);
  vec2 np = p + 1.0/RESOLUTION.xy;
  float rdd = (2.0+0.5*lp*tanh(lp+0.9*psin(per*p.x)*psin(per*p.y)));
//  float rdd = (2.0+1.5*tanh(lp));
  vec3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);
  vec3 nrd = normalize(np.x*uu + np.y*vv + rdd*ww);

  const vec3 errorCol = vec3(1.0, 0.0, 0.0);

  float planeDist = 1.0;
  const int furthest = 6;
  const int fadeFrom = max(furthest-4, 0);

  float nz = floor(ro.z / planeDist);

  vec3 skyCol = skyColor(ro, rd);

  vec3 col = skyCol;

  for (int i = furthest; i >= 1 ; --i) {
    float pz = planeDist*nz + planeDist*float(i);

    float pd = (pz - ro.z)/rd.z;

    if (pd > 0.0) {
      vec3 pp = ro + rd*pd;
      vec3 npp = ro + nrd*pd;

      float aa = 3.0*length(pp - npp);

      vec3 off = offset(pp.z);

      vec4 pcol = plane(ro, rd, pp, off, aa, nz+float(i));

      float nz = pp.z-ro.z;
      float fadeIn = (1.0-smoothstep(planeDist*float(fadeFrom), planeDist*float(furthest), nz));
      float fadeOut = smoothstep(0.0, planeDist*0.1, nz);
      pcol.xyz = mix(skyCol, pcol.xyz, (fadeIn));
      pcol.w *= fadeOut;

      col = alphaBlend(col, pcol);
    } else {
      break;
    }

  }

  return col;
}

vec3 effect(vec2 p, vec2 q) {
  float tm  = TIME*0.5*BPM/60.+0.25;
  vec3 ro   = offset(tm);
  vec3 dro  = doffset(tm);
  vec3 ddro = ddoffset(tm);

  vec3 ww = normalize(dro);
  vec3 uu = normalize(cross(normalize(vec3(0.0,1.0,0.0)+ddro), ww));
  vec3 vv = normalize(cross(ww, uu));

  vec3 col = color(ww, uu, vv, ro, p);
  col += fade_in();
  col = sqrt(col);
  return col;
}

void main() {
  vec2 q = gl_FragCoord.xy/RESOLUTION.xy;
  vec2 p = -1. + 2. * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;

  vec3 col = effect(p, q);

  fragColor = vec4(col, 1.0);
}
)SHADER";

  char const fragment_shader__psychedelic[] = R"SHADER(
#version 300 es
#define USE_UNIFORMS

precision highp float;

uniform float time;
uniform vec2 resolution;

out vec4 fragColor;

const float bpm = 145.0;

#ifdef USE_UNIFORMS
uniform vec4 state;

float beat() {
  return state.x;
}

float fade_in() {
  return state.y;
}

float fade_out() {
  return state.z;
}

float fade() {
  return state.w;
}

#else

float fade_in() {
  return 0.;
}

float fade_out() {
  return smoothstep(-0.707, 0.707, sin(time));
}

float fade() {
  return 1.0;
}

float beat() {
  return exp(-1.*fract(time*bpm/60.));
}
#endif


#define TIME        time
#define RESOLUTION  resolution

#define PI              3.141592654
#define TAU             (2.0*PI)
#define TTIME           (TAU*TIME)

#define ROT(a)          mat2(cos(a), sin(a), -sin(a), cos(a))

const vec2 hexcell_sz       = vec2(1.0, sqrt(3.0));
const vec2 hexcell_hsz      = 0.5*hexcell_sz;

vec3 hsv2rgb(vec3 hsv) {
  return (cos(hsv.x*2.*acos(-1.)+vec3(0,4,2))*hsv.y+2.-hsv.y)*hsv.z/2.;
}

vec3 palette(float a) {
  return 0.5*(1.+sin(vec3(0,1,2)+a));
}

float hash(float co) {
  return fract(sin(co*12.9898) * 13758.5453);
}

float pmin(float a, float b, float k) {
  float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
  return mix( b, a, h ) - k*h*(1.0-h);
}

float pabs(float a, float k) {
  return -pmin(a,-a, k);
}

vec2 toPolar(vec2 p) {
  return vec2(length(p), atan(p.y, p.x));
}

vec2 toRect(vec2 p) {
  return vec2(p.x*cos(p.y), p.x*sin(p.y));
}

float tanh_approx(float x) {
  float x2 = x*x;
  return clamp(x*(27.0 + x2)/(27.0+9.0*x2), -1.0, 1.0);
}

vec2 mod2_1(inout vec2 p) {
  vec2 c = floor(p + 0.5);
  p = fract(p + 0.5) - 0.5;
  return c;
}

float modMirror1(inout float p, float size) {
  float halfsize = size*0.5;
  float c = floor((p + halfsize)/size);
  p = mod(p + halfsize,size) - halfsize;
  p *= mod(c, 2.0)*2.0 - 1.0;
  return c;
}

float modPolar(inout vec2 p, float repetitions) {
  float angle = 2.0*PI/repetitions;
  float a = atan(p.y, p.x) + angle/2.;
  float r = length(p);
  float c = floor(a/angle);
  a = mod(a,angle) - angle/2.;
  p = vec2(cos(a), sin(a))*r;
  if (abs(c) >= (repetitions/2.0)) c = abs(c);
  return c;
}

float hex(vec2 p, float r) {
  const vec3 k = vec3(-sqrt(3.0)/2.0,1.0/2.0,sqrt(3.0)/3.0);
  p = p.yx;
  p = abs(p);
  p -= 2.0*min(dot(k.xy,p),0.0)*k.xy;
  p -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
  return length(p)*sign(p.y);
}

vec2 hextile(inout vec2 p) {
  vec2 p1 = mod(p, hexcell_sz)-hexcell_hsz;
  vec2 p2 = mod(p - hexcell_hsz, hexcell_sz)-hexcell_hsz;
  vec2 p3 = mix(p2, p1, vec2(dot(p1, p1) < dot(p2, p2)));
  vec2 n = round((p3 - p + hexcell_hsz)/hexcell_hsz);

  p = p3;

  return round(n*2.0)/2.0;
}

float circle(vec2 p, float r) {
  return length(p) - r;
}

float box(vec2 p, vec2 b) {
  vec2 d = abs(p)-b;
  return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float unevenCapsule(vec2 p, float r1, float r2, float h) {
  p.x = abs(p.x);
  float b = (r1-r2)/h;
  float a = sqrt(1.0-b*b);
  float k = dot(p,vec2(-b,a));
  if( k < 0.0 ) return length(p) - r1;
  if( k > a*h ) return length(p-vec2(0.0,h)) - r2;
  return dot(p, vec2(a,b) ) - r1;
}

vec2 cogwheel(vec2 p, float innerRadius, float outerRadius, float cogs, float holes) {
  float cogWidth  = 0.25*innerRadius*TAU/cogs;

  float d0 = circle(p, innerRadius);

  vec2 icp = p;
  modPolar(icp, holes);
  icp -= vec2(innerRadius*0.55, 0.0);
  float d1 = circle(icp, innerRadius*0.25);

  vec2 cp = p;
  modPolar(cp, cogs);
  cp -= vec2(innerRadius, 0.0);
  float d2 = unevenCapsule(cp.yx, cogWidth, cogWidth*0.75, (outerRadius-innerRadius));

  float d3 = circle(p, innerRadius*0.20);

  float hd = 1E6;

  hd = min(hd, d1);
  hd = min(hd, d3);

  float d = 1E6;
  d = min(d, d0);
  d = pmin(d, d2, 0.5*cogWidth);
  d = min(d, d2);
  d = max(d, -d1);
  d = max(d, -d3);

  return vec2(hd, d);
}

float smoothKaleidoscope(inout vec2 p, float sm, float rep) {
  vec2 hp = p;

  vec2 hpp = toPolar(hp);
  float rn = modMirror1(hpp.y, TAU/rep);

  float sa = PI/rep - pabs(PI/rep - abs(hpp.y), sm);
  hpp.y = sign(hpp.y)*(sa);

  hp = toRect(hpp);

  p = hp;

  return rn;
}

vec4 alphaBlend(vec4 back, vec4 front) {
  float w = front.w + back.w*(1.0-front.w);
  vec3 xyz = (front.xyz*front.w + back.xyz*back.w*(1.0-front.w))/w;
  return w > 0.0 ? vec4(xyz, w) : vec4(0.0);
}

vec3 alphaBlend(vec3 back, vec4 front) {
  vec3 colb = back.xyz;
  vec3 colf = front.xyz;
  vec3 xyz = mix(colb, colf.xyz, front.w);
  return xyz;
}

vec3 offset(float z) {
  float a = z;
  vec2 p = -0.075*(vec2(cos(a), sin(a*sqrt(2.0))) + vec2(cos(a*sqrt(0.75)), sin(a*sqrt(0.5))));
  return vec3(1.5*p, z);
}

vec3 doffset(float z) {
  float eps = 0.1;
  return 0.5*(offset(z + eps) - offset(z - eps))/eps;
}

vec3 ddoffset(float z) {
  float eps = 0.1;
  return 0.5*(doffset(z + eps) - doffset(z - eps))/eps;
}

const float plane0_lw = 0.05;

const mat2[] plane0_rots = mat2[](ROT(0.0*PI/2.0), ROT(1.00*PI/2.0), ROT(2.0*PI/2.0), ROT(3.0*PI/2.0));

const float plane0_smallCount = 16.0;

vec2 plane0_df0(vec2 p2, float n, float nr, out vec2 np2) {
  vec2 p = p2;
  vec2 hp = p;

  float hd = hex(hp, 0.5-plane0_lw)-plane0_lw;
  vec2 hn = hextile(hp);

  float d  = 1E6;
  d = min(d, circle(hp, 0.44));
  d = min(d, hd- plane0_lw);
  d = abs(d) - plane0_lw;

  np2 = hn;

  return vec2(hd, d);
}

vec2 plane0_cell0(vec2 p) {
  float d0  = circle(p-vec2(0.5), 0.5);
  float d1  = circle(p+vec2(0.5), 0.5);

  float d = 1E6;
  d = min(d, d0);
  d = min(d, d1);
  return vec2(d, 1E6);
}

vec2 plane0_cell1(vec2 p) {
  float d0  = abs(p.x);
  float d1  = abs(p.y);
  float d2 = circle(p, 0.25);

  float d = 1E6;
  d = min(d, d0);
  d = min(d, d1);
  return vec2(d, d2);
}

vec2 plane0_df1(vec2 p2, float n, float nr, out vec2 np2) {
  vec2 p = p2;
  float hd = hex(p, 0.4);

  vec2 hp = p;
  vec2 hn = mod2_1(hp);
  float r = hash(vec3(hn, n)+100.);

  hp *= plane0_rots[int(r*4.0)];

  vec2 cd0 = plane0_cell0(hp);
  vec2 cd1 = plane0_cell1(hp);
  vec2 d0 = mix(cd0, cd1, fract(r*13.) > 0.5);

  hd = min(hd, d0.y);

  float d = 1E6;
  d = min(d, d0.x);
  d = abs(d) - plane0_lw;
  d = min(d, hd - plane0_lw*2.);

  np2 = hn;

  return vec2(hd, d);
}

float psin(float a) {
  return 0.5 + 0.5*sin(a);
}


vec2 plane0_cell2(vec2 p, float r) {
  vec2 d = vec2(1E6);
  const float bigCount = 60.0;

  vec2 cp0 = p;
  cp0 *= ROT(-TTIME/bigCount);
  vec2 d0 = cogwheel(cp0, 0.36, 0.38, bigCount, 5.0);

  vec2 cp1 = p;
  float nm = modPolar(cp1, 6.0);

  cp1 -= vec2(0.5, 0.0);
  cp1 *= ROT(0.2+TAU*nm/2.0 + TTIME/plane0_smallCount);
  vec2 d1 = cogwheel(cp1, 0.11, 0.125, plane0_smallCount, 5.0);

  d = min(d, d0);
  d = min(d, d1);
  return d;
}

vec2 plane0_cell3(vec2 p, float r) {
  vec2 d = vec2(1E6);
  vec2 cp0 = p;
  float nm = modPolar(cp0, 6.0);
  vec2 cp1 = cp0;
  const float off = 0.275;
  const float count = plane0_smallCount + 2.0;
  cp0 -= vec2(off, 0.0);
  cp0 *= ROT(TAU*nm/2.0 - TTIME/count);
  vec2 d0 = cogwheel(cp0, 0.09, 0.105, count, 5.0);


  cp1 -= vec2(0.5, 0.0);
  cp1 *= ROT(0.2+TAU*nm/2.0 + TTIME/plane0_smallCount);
  vec2 d1 = cogwheel(cp1, 0.11, 0.125, plane0_smallCount, 5.0);

  float l = length(p);
  float d2 = l - (off+0.055);
  float d3 = d2 + 0.020;;

  vec2 tp0 = p;
  modPolar(tp0, 60.0);
  tp0.x -= off;
  float d4 = box(tp0, vec2(0.0125, 0.005));

  float ctime = -(TIME*0.05 + r)*TAU;

  vec2 tp1 = p;
  tp1 *= ROT(ctime*12.0);
  tp1.x -= 0.13;
  float d5 = box(tp1, vec2(0.125, 0.005));

  vec2 tp2 = p;
  tp2 *= ROT(ctime);
  tp2.x -= 0.13*0.5;
  float d6 = box(tp2, vec2(0.125*0.5, 0.0075));

  float d7 = l - 0.025;
  float d8 = l - 0.0125;

  d = min(d, d0);
  d = min(d, d1);

  float hd = d.x;
  hd = max(hd, -d2);
  hd = min(hd, d8);

  d = min(d, d2);
  d = max(d, -d3);
  d = min(d, d4);
  d = min(d, d5);
  d = min(d, d6);
  d = min(d, d7);
  d = max(d, -d8);

  return vec2(hd, d.y);
}

vec2 plane0_df4(vec2 p2, float n, float nr, out vec2 np2) {
  float hd = hex(p2, 0.45);

  vec2 hp = p2;

  vec2 hn = hextile(hp);
  float r = hash(vec3(hn, n));

  vec2 d;

  if (hd <0.05) {
    d = vec2(hd, hd-0.05);
  } else if (r < 0.5) {
    d = plane0_cell2(hp, r);
  } else {
    d = plane0_cell3(hp, r);
  }

  np2 = hn;
  return d;
}

vec2 plane0_df(vec3 pp, float n, out vec3 np) {
  float nr = hash(n);
  vec2 p2 = pp.xy;

  p2 *= ROT(TAU*nr+0.5*TIME*fract(3.0*nr));

  float rep = 2.0*floor(mix(3.0, 20.0, fract(7.0*nr)));
  float sm = mix(0.1, 0.025, fract(13.*nr))*24.0/rep;
  float skn = 0.;

  skn = smoothKaleidoscope(p2, sm, rep);

  float s = mix(0.05, 0.125, fract(nr*17.));

  vec2 np2;
  vec2 d = plane0_df1(p2/s, n, nr, np2)*s;

  np = vec3(np2, skn);

  return d;
}

vec4 plane(vec3 ro, vec3 rd, vec3 pp, vec3 off, float aa, float n) {
  vec3 hn;
  vec3 p = pp-off*vec3(1.0, 1.0, 0.0);
  float dc = length(p.xy);

  vec2 dd = plane0_df(p, n, hn);
  float d = dd.y;
  float a  = smoothstep(-aa, aa, -d);
  float ha = smoothstep(-aa, aa, dd.x);

  vec3 hsv = vec3(1.0, 0.0, 1.0);

  hsv = (vec3(sin(n*0.35)+sin(5.5*d+1.5*dc+length(p.xy-vec2(0.0, -0.25))-TTIME/13.0), 1.0, clamp(1.0-tanh_approx(200.*d), 0.0, 1.0)));
  vec3 baseCol = hsv2rgb(hsv)*fade();

  float m = mix(-1.0, 1.0, mod(n, 2.0));

  vec4 acol = vec4(baseCol, clamp(ha*pow(hsv.z, 0.05), 0.0, 1.0));
  vec4 bcol = vec4(0.0);
  vec4 ccol = vec4(mix(baseCol, vec3(0.0), a), ha*mix(0.66, 1.0, hsv.z));
  vec4 dcol = m*mix(bcol, acol, (1.0-tanh_approx(3.*length(p.xy))));
  vec4 col = ccol;
  return col;
}

vec3 skyColor(vec3 ro, vec3 rd) {
  float b = beat();
  float ld = max(dot(rd, vec3(0.0, 0.0, 1.0)), 0.0);
  vec3 baseCol = 3.*(0.25+palette(time))*(b*b*pow(ld, mix(40.0, 80.0, fade())));
  return baseCol;

}

const float planeDist = 1.0-0.88;
vec3 color(vec3 ww, vec3 uu, vec3 vv, vec3 ro, vec2 p) {

  vec2 aa = 1.0/RESOLUTION.xy;

  float fo = fade_out();
  float lp = length(p);
  p *= ROT(PI*lp*fo);
  vec2 np = p + aa;
  const float per = TAU*10.0;
  float rdd = (2.0+0.5*lp*tanh_approx(lp+0.9*psin(per*p.x)*psin(per*p.y)));
  rdd += -1.66*fo;
  vec3 rd = normalize(p.x*uu + p.y*vv + rdd*ww);
  vec3 nrd = normalize(np.x*uu + np.y*vv + rdd*ww);

  const int furthest = 11;
  const int fadeFrom = max(furthest-8, 0);
  const float fadeDist = planeDist*float(furthest - fadeFrom);
  float nz = floor(ro.z / planeDist);

  vec3 skyCol = skyColor(ro, rd);

  vec4 acol = vec4(0.0);
  const float cutOff = 0.95;
  bool cutOut = false;

  for (int i = 1; i <= furthest; ++i) {
    float pz = planeDist*nz + planeDist*float(i);

    float pd = (pz - ro.z)/rd.z;

    if (pd > 0.0 && acol.w < cutOff) {
      vec3 pp = ro + rd*pd;
      vec3 npp = ro + nrd*pd;

      float aa = 3.0*length(pp - npp);

      vec3 off = offset(pp.z);

      vec4 pcol = plane(ro, rd, pp, off, aa, nz+float(i));

      float nz = pp.z-ro.z;
      float fadeIn = exp(-2.5*max((nz - planeDist*float(fadeFrom))/fadeDist, 0.0));
      float fadeOut = smoothstep(0.0, planeDist*0.1, nz);
      pcol.xyz = mix(skyCol, pcol.xyz, (fadeIn));
      pcol.w *= fadeOut;

      pcol = clamp(pcol, 0.0, 1.0);

      acol = alphaBlend(pcol, acol);
    } else {
      cutOut = true;
      break;
    }

  }

  vec3 col = alphaBlend(skyCol, acol);
  col = pow(col+0.25*palette(time*TAU)*fo*fo*fo, vec3(mix(1.0, 0.125, fo)));
  col *= smoothstep(mix(4.0, 0.0, fo*fo*fo), 0.0, lp);
  return col;
}

vec3 postProcess(vec3 col, vec2 q) {
  col = clamp(col, 0.0, 1.0);
  col = sqrt(col);
  col = col*0.6+0.4*col*col*(3.0-2.0*col);
  col = mix(col, vec3(dot(col, vec3(0.33))), -0.5);
  return col;
}

vec3 effect(vec2 p, vec2 q) {
  float tm = TIME*planeDist*bpm/60.;
  vec3 ro   = offset(tm);
  vec3 dro  = doffset(tm);
  vec3 ddro = ddoffset(tm);

  vec3 ww = normalize(dro);
  vec3 uu = normalize(cross(normalize(vec3(0.0,1.0,0.0)+ddro), ww));
  vec3 vv = normalize(cross(ww, uu));

  vec3 col = color(ww, uu, vv, ro, p);
  col = postProcess(col, q);
  col += fade_in();
  return col;
}

void main(void) {
  vec2 q = gl_FragCoord.xy/RESOLUTION.xy;
  vec2 p = -1. + 2. * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;

  vec3 col = effect(p, q);
  fragColor = vec4(col, 1.0);
}
)SHADER";
}