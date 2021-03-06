// sphere tracing demo with a few random elements
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <algorithm>
#include "render.h"
#include "vec3.h"

float frame_ = 0;

vec3 lcol(0.5,0.7,0.5);
float mshiny[] = {100,10,10,10};
vec3 mcol[] = {
  vec3(1.0f, 1.0f, 1.0f),
  vec3(0.0f, 0.0f, 1.0f),
  vec3(1.0f, 0.2f, 0.0f),
  vec3(0.22f, 0.62f, 0.0f)};

float udRoundBox(const vec3& p, const vec3& b, float r)
{
  return length(max(abs(p)-b,0.0))-r;
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}

vec3 rotateY(vec3 p, float a) {
  float ca = cos(a), sa = sin(a);
  return vec3(p.x*ca-p.z*sa, p.y, p.x*sa+p.z*ca);
}

vec3 rotateX(vec3 p, float a) {
  float ca = cos(a), sa = sin(a);
  return vec3(p.x, p.y*ca-p.z*sa, p.y*sa+p.z*ca);
}

float arc1(const vec3 &q, float r1, float r2, float zpos) {
  //return std::max(
  //    fabsf(q.z + zpos), std::max(
  return std::max(
          std::max(-q.x - q.y, q.x - q.y),
          fabsf(length(vec3(q.x, q.y, 0)) - r1)) - r2;
}

// returns minimum distance to scene, material m, and normal n
float dist(const vec3 &p, int *m) {
  *m = -1;
  float d = 1e30;
  float dplane = p.y + 50;
  if (dplane < d) {
    // *m = ((lrint(p.x*0.01)&1)^(lrint(p.z*0.01)&1));
    *m = 0;
    d = dplane;
  }
  //vec3 q = rotateX(rotateY(p, -frame_*0.07), frame_*0.025);
  //vec3 q = rotateY(p, frame_*0.07);
  vec3 q = p;

  float dcyl = std::max(
      std::max(q.z - 10.0f, -10.0f - q.z),
      length(vec3(q.x, q.y, 0)) - 50.0f);

  dcyl = std::max(dcyl, -arc1(q - vec3(0, -10, 0), 40, 5, 6));
  dcyl = std::max(dcyl, -arc1(q - vec3(0, -25, 0), 35, 4, 6));
  dcyl = std::max(dcyl, -arc1(q - vec3(0, -35, 0), 30, 3, 6));
  if (dcyl < d) {
    d = dcyl;
    *m = 3;
  }

  return d;
}

float shadow(const vec3& ro, const vec3& rd, float mint, float maxt) {
  int m;
  float res = 1.0;
  for (float t = mint; t < maxt; ) {
    float h = dist(ro+rd*t, &m);
    if (h < 0.001) {
      return 0;
    }
    res = std::min( res, 20.0f*h/t );
    t += h;
  }
  return res;
}

vec3 lighting(const vec3 &p, int m, const vec3& lightpos) {
  vec3 lightdir = normalize(lightpos - p);
  // yet another trick from iq: use the distance field to compute dot(light,
  // normal) instead of explicitly finding a normal.
  // http://www.pouet.net/topic.php?which=7535&page=1
  int _m;
  float diffuse = std::max(0.0f, 10.0f*dist(p+lightdir*0.1, &_m));
  float s = std::max(0.3f, shadow(p, lightdir, 0.01, length(p-lightpos)));
  float l = std::max(0.1f, diffuse * s);
  return mcol[m]*l + lcol*pow(l, mshiny[m]);
}

int main()
{
  int x,y;
  render_init();
  for(;;) {
    vec3 campos = vec3(130*sin(frame_*0.02), 50 + 40*sin(frame_*0.03), -130*cos(frame_*0.02));
    vec3 lightpos = vec3(200.0*sin(frame_*0.05),100,campos.z);
    vec3 camz = normalize(campos*-1);
    vec3 camx = normalize(cross(camz, vec3(0,1,0)));
    vec3 camy = normalize(cross(camx, camz));
    for(y=0;y<24;y++) {
      int fg=7, bg=0;
      for(x=0;x<80;x++) {
        vec3 color = vec3(0,0,0);
#ifdef AA
        for(float xx = -0.25;xx<=0.25;xx+=0.5) { // 2 x samples
          for(float yy = -0.75;yy<=0.75;yy+=0.5) { // 4 y samples
            vec3 dir = normalize(vec3(x-40.0f+xx,25.0f-2.0f*y+yy,70.0f));
#else
            vec3 dir = normalize(vec3(x-40.0f,25.0f-2.0f*y,70.0f));
#endif
            dir = camx*dir.x + camy*dir.y + camz*dir.z;
            float t = 0;
            int m = -1;
            for (int iter = 0; iter < 64 && t < 1e6; iter++) {
              vec3 p = dir*t + campos;
              float d = dist(p, &m);
              if (d < 0.001) {
                color = color + lighting(p, m, lightpos);
                break;
              }
              t += d;
            }
#ifdef AA
          }
        }
        render_color(color * 0.125, x, y, &fg, &bg);
#else
        render_color(color, x, y, &fg, &bg);
#endif
      }
      printf("\x1b[0m\n");
    }
    fflush(stdout);
    usleep(20000);
    frame_ += 1;
    printf("\x1b[24A");
  }
}
