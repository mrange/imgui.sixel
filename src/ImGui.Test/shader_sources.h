#pragma once

namespace {
  char const vertex_shader__source[] = R"SHADER(
#version 300 es
layout (location = 0) in vec3 aPos;
void main() {
  gl_Position = vec4(aPos.x, -aPos.y, aPos.z, 1.0);
}
)SHADER";

  char const fragment_shader__source[] = R"SHADER(
#version 300 es

precision highp float;

uniform float time;
uniform vec2 resolution;

#define USE_UNIFORMS
#ifdef USE_UNIFORMS
uniform float fixed_radius2;
uniform float min_radius2;
uniform float folding_limit;
uniform float scale;

#else
const float fixed_radius2 = 1.9;
const float min_radius2 = 0.1;
const float folding_limit = 1.0;
const float scale = -2.5;
#endif


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
  float dd = length(z)-mix(2.1, 2.2, g_beat);
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
  float lsd1  = sphered(ro, rd, vec4(lightPos1, mix(2.3, 2.4, beat)), t);

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
  const vec3 dcol0 = HSV2RGB(vec3(0. , 0.75, -0.25));
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

vec3 effect(vec2 p) {
  const vec3 cam  = 5.0*vec3(1.0, 0.5, 1.0);
  const vec3 dcam = normalize(vec3(0.0) - cam);
  const vec3 ro = cam;
  const vec3 ww = dcam;
  const vec3 uu = normalize(cross(vec3(0.0,1.0,0.0), ww));
  const vec3 vv = cross(ww,uu);
  const float rdd = 2.0;
  vec3 rd = normalize(-p.x*uu + p.y*vv + rdd*ww);

  float a = 0.25*time;
  vec4 q = createQuaternion(normalize(vec3(1.0,sin(0.33*a),sin(0.707*a))), a);
  //vec4 q = createQuaternion(normalize(cam.zxy), time);
  g_rot = rotationFromQuaternion(q);
  vec3 col = render(ro, rd);
  return col;
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
}