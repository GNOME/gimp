/*
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * SphereDesigner v0.4 - creates textured spheres
 * by Vidar Madsen <vidar@prosalg.no>
 *
 * Status: Aug 31 1999 - Messy source, will clean up later.
 *
 * Todo:
 * - Editing of lights
 * - Saving / Loading of presets
 * - Transparency in textures (preliminary work started)
 * - Antialiasing
 * - Global controls: Gamma, ++
 * - Beautification of GUI
 * - (Probably more. ;-)
 */

#define PLUG_IN_NAME "SphereDesigner"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <libgimp/gimp.h>

#ifndef SRAND_FUNC
#define SRAND_FUNC srand
#endif
#ifndef RAND_FUNC
#define RAND_FUNC rand
#endif

#define PREVIEWSIZE 150

/* These must be adjusted as more functionality is added */
#define MAXOBJECT 5
#define MAXLIGHT 5
#define MAXTEXTURE 20
#define MAXTEXTUREPEROBJ 20
#define MAXNORMAL 20
#define MAXNORMALPEROBJ 20
#define MAXATMOS 1
#define MAXCOLPERGRADIENT 5

static void      query  (void);
static void      run    (char      *name,
                         int        nparams,
                         GParam    *param,
                         int       *nreturn_vals,
                         GParam   **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

enum {
  TRIANGLE, DISC, PLANE, SPHERE, CYLINDER, LIGHT
};

enum {
  SOLID, CHECKER, MARBLE, LIZARD, IMAGE, PHONG, REFLECTION, REFRACTION, PERLIN,
  WOOD, TRANSPARENT, SPIRAL, SPOTS, SMOKE
};

enum {
  PERSPECTIVE, ORTHOGONAL, FISHEYE
};

enum {
  FOG
};

/* World-flags */
#define SMARTAMBIENT 0x00000001

/* Object-flags */
#define NOSHADOW   0x00000001

/* Texture-flags */
#define GRADIENT   0x00000001

typedef struct {
  double x,y,z,w;
} vector;

typedef struct {
  short xsize, ysize;
  unsigned char *rgb;
} image;

typedef struct {
  short numcol;
  double pos[MAXCOLPERGRADIENT];
  vector color[MAXCOLPERGRADIENT];
} gradient;

typedef struct {
  short majtype;
  short type;
  unsigned long flags;
  vector color1, color2;
  gradient gradient;
  vector ambient, diffuse;
  vector scale, translate, rotate;
  image image;
  vector reflection;
  vector refraction;
  vector transparent;
  double ior;
  vector phongcolor;
  double phongsize;
  double amount;
  double exp;
  vector turbulence;
} texture;

typedef struct {
  short type;
  double density;
  vector color;
  double turbulence;
} atmos;

typedef struct {
  short type;
  unsigned long flags;
  short numtexture;
  texture texture[MAXTEXTUREPEROBJ];
  short numnormal;
  texture normal[MAXNORMALPEROBJ];
} common;

typedef struct {
  common com;
  vector a,b,c;
} triangle;

typedef struct {
  common com;
  vector a;
  double b, r;
} disc;

typedef struct {
  common com;
  vector a;
  double r;
} sphere;

typedef struct {
  common com;
  vector a, b, c;
} cylinder;

typedef struct {
  common com;
  vector a;
  double b;
} plane;

typedef struct {
  common com;
  vector color;
  vector a;
} light;

typedef struct {
  vector v1, v2;
  short inside;
  double ior;
} ray;

typedef union {
  common com;
  triangle tri;
  disc disc;
  plane plane;
  sphere sphere;
  cylinder cylinder;
} object;


struct world_t {
  int numobj;
  object obj[MAXOBJECT];
  int numlight;
  light light[MAXLIGHT];
  int numtexture;
  texture texture[MAXTEXTURE];
  unsigned long flags;
  short quality;
  double smartambient;
  short numatmos;
  atmos atmos[MAXATMOS];
};

struct camera_t {
  vector location, lookat, up, right;
  short type;
  double fov, tilt;
};

int traceray(ray *r, vector *col, int level, double imp);

GtkWidget *drawarea = NULL;

unsigned char img[PREVIEWSIZE*PREVIEWSIZE*3];

int running = 0;

sphere s;

struct textures_t {
  int index;
  char *s;
  long n;
};

struct textures_t textures[] = {
  {0,"Solid", SOLID},
  {1,"Checker",CHECKER},
  {2,"Marble",MARBLE},
  {3,"Lizard",LIZARD},
  {4,"Phong",PHONG},
  {5,"Noise",PERLIN},
  {6,"Wood",WOOD},
  {7,"Spiral",SPIRAL},
  {8,"Spots",SPOTS},
  {0,NULL,0}
};

struct {
  int solid, phong, light;
} settings = {1,1,1};

inline void vset(vector *v, double a, double b, double c);
gint restartrender(void);
void drawcolor1(GtkWidget *w);
void drawcolor2(GtkWidget *w);
void render(void);
void realrender(GDrawable *drawable);

#define COLORBUTTONWIDTH 30
#define COLORBUTTONHEIGHT 20

GtkWidget *texturelist = NULL;
GtkObject *scalexscale,*scaleyscale,*scalezscale;
GtkObject *rotxscale,*rotyscale,*rotzscale;
GtkObject *posxscale,*posyscale,*poszscale;
GtkObject *turbulencescale;
GtkObject *amountscale;
GtkObject *expscale;
GtkWidget *typemenu_menu;
GtkWidget *texturemenu_menu;
GtkWidget *typemenu;
GtkWidget *texturemenu;


#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])

#define B 256

static int p[B + B + 2];
static double g[B + B + 2][3];
static int start = 1;

void init(void)
{
  int i, j, k;
  double v[3], s;

/* Create an array of random gradient vectors uniformly on the unit sphere */

  SRAND_FUNC(1); /* Use static seed, to get reproducable results */
  for (i = 0 ; i < B ; i++) {
    do {                            /* Choose uniformly in a cube */
      for (j=0 ; j<3 ; j++)
	v[j] = (double)((RAND_FUNC() % (B + B)) - B) / B;
      s = DOT(v,v);
    } while (s > 1.0);              /* If not in sphere try again */
    s = sqrt(s);
    for (j = 0 ; j < 3 ; j++)       /* Else normalize */
      g[i][j] = v[j] / s;
  }

/* Create a pseudorandom permutation of [1..B] */

  for (i = 0 ; i < B ; i++)
    p[i] = i;
  for (i = B ; i > 0 ; i -= 2) {
    k = p[i];
    p[i] = p[j = RAND_FUNC() % B];
    p[j] = k;
  }

/* Extend g and p arrays to allow for faster indexing */

  for (i = 0 ; i < B + 2 ; i++) {
    p[B + i] = p[i];
    for (j = 0 ; j < 3 ; j++)
      g[B + i][j] = g[i][j];
  }
}

#define setup(i,b0,b1,r0,r1) \
        t = vec[i] + 10000.; \
        b0 = ((int)t) & (B-1); \
        b1 = (b0+1) & (B-1); \
        r0 = t - (int)t; \
        r1 = r0 - 1.;

double noise3(double *vec)
{
  int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  double rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v;
  int i, j;

  if(start) {
    start = 0;
    init();
  }

  setup(0, bx0,bx1, rx0,rx1);
  setup(1, by0,by1, ry0,ry1);
  setup(2, bz0,bz1, rz0,rz1);

  i = p[ bx0 ];
  j = p[ bx1 ];
  
  b00 = p[ i + by0 ];
  b10 = p[ j + by0 ];
  b01 = p[ i + by1 ];
  b11 = p[ j + by1 ];
  
#define at(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

#define surve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

  sx = surve(rx0);
  sy = surve(ry0);
  sz = surve(rz0);


  q = g[ b00 + bz0 ] ; u = at(rx0,ry0,rz0);
  q = g[ b10 + bz0 ] ; v = at(rx1,ry0,rz0);
  a = lerp(sx, u, v);

  q = g[ b01 + bz0 ] ; u = at(rx0,ry1,rz0);
  q = g[ b11 + bz0 ] ; v = at(rx1,ry1,rz0);
  b = lerp(sx, u, v);

  c = lerp(sy, a, b);          /* interpolate in y at lo x */

  q = g[ b00 + bz1 ] ; u = at(rx0,ry0,rz1);
  q = g[ b10 + bz1 ] ; v = at(rx1,ry0,rz1);
  a = lerp(sx, u, v);

  q = g[ b01 + bz1 ] ; u = at(rx0,ry1,rz1);
  q = g[ b11 + bz1 ] ; v = at(rx1,ry1,rz1);
  b = lerp(sx, u, v);

  d = lerp(sy, a, b);          /* interpolate in y at hi x */

  return 1.5 * lerp(sz, c, d); /* interpolate in z */
}


double turbulence(double *point, double lofreq, double hifreq)
{
  double noise3(), freq, t, p[3];

  p[0] = point[0] + 123.456;
  p[1] = point[1] + 234.567;
  p[2] = point[2] + 345.678;

  t = 0;
  for (freq = lofreq ; freq < hifreq ; freq *= 2.) {
    t += noise3(p) / freq;
    p[0] *= 2.;
    p[1] *= 2.;
    p[2] *= 2.;
  }
  return t - 0.3; /* readjust to make mean value = 0.0 */
}

struct camera_t camera;
struct world_t world;

inline void vcopy(vector *a, vector *b)
{
  a->x = b->x;
  a->y = b->y;
  a->z = b->z;
  a->w = b->w;
}

inline void vcross(vector *r, vector *a, vector *b)
{
  vector t;
  t.x = a->y * b->z - a->z * b->y;
  t.y = -(a->x * b->z - a->z * b->x);
  t.z = a->x * b->y - a->y * b->x;
  vcopy(r, &t);
}

inline double vdot(vector *a, vector *b)
{
  double s;
  s = a->x * b->x;
  s += a->y * b->y;
  s += a->z * b->z;
  return s;
}

inline double vdist(vector *a, vector *b)
{
  double x,y,z;
  x = a->x - b->x;
  y = a->y - b->y;
  z = a->z - b->z;
  return sqrt(x*x+y*y+z*z);
}

inline double vlen(vector *a)
{
  double l;
  l = sqrt(a->x*a->x + a->y*a->y + a->z*a->z);
  return l;
}

inline void vnorm(vector *a, double v)
{
  double d;
  d  = sqrt(a->x*a->x + a->y*a->y + a->z*a->z);
  a->x *= v/d;
  a->y *= v/d;
  a->z *= v/d;
}

inline void vrotate(vector *axis, double ang, vector *vector)
{
  double rad = ang / 180.0 * G_PI;
  double ax = vector->x;
  double ay = vector->y;
  double az = vector->z;
  double x = axis->x;
  double y = axis->y;
  double z = axis->z;
  double c = cos(rad);
  double s = sin(rad);
  double c1 = 1.0 - c;
  double xx = c1 * x * x;
  double yy = c1 * y * y;
  double zz = c1 * z * z;
  double xy = c1 * x * y;
  double xz = c1 * x * z;
  double yz = c1 * y * z;
  double sx = s * x;
  double sy = s * y;
  double sz = s * z;
  vector->x = (xx + c)*ax + (xy + sz)*ay + (xz - sy)*az;
  vector->y = (xy - sz)*ax + (yy + c)*ay + (yz + sx)*az;
  vector->z = (xz + sy)*ax + (yz - sx)*ay + (zz + c )*az;
}

inline void vset(vector *v, double a, double b, double c)
{
  v->x = a;
  v->y = b;
  v->z = c;
  v->w = 1.0;
}

inline void vcset(vector *v, double a, double b, double c, double d)
{
  v->x = a;
  v->y = b;
  v->z = c;
  v->w = d;
}

inline void vvrotate(vector *p, vector *rot)
{
  vector axis;
  if(rot->x != 0.0) {
    vset(&axis, 1,0,0);
    vrotate(&axis, rot->x, p);
  }
  if(rot->y != 0.0) {
    vset(&axis, 0,1,0);
    vrotate(&axis, rot->y, p);
  }
  if(rot->z != 0.0) {
    vset(&axis, 0,0,1);
    vrotate(&axis, rot->z, p);
  }
}

inline void vsub(vector *a, vector *b)
{
  a->x -= b->x;
  a->y -= b->y;
  a->z -= b->z;
  a->w -= b->w;
}

inline void vadd(vector *a, vector *b)
{
  a->x += b->x;
  a->y += b->y;
  a->z += b->z;
  a->w += b->w;
}

inline void vneg(vector *a)
{
  a->x = -a->x;
  a->y = -a->y;
  a->z = -a->z;
  a->w = -a->w;
}

inline void vmul(vector *v, double a)
{
  v->x *= a;
  v->y *= a;
  v->z *= a;
  v->w *= a;
}

inline void vvmul(vector *a, vector *b)
{
  a->x *= b->x;
  a->y *= b->y;
  a->z *= b->z;
  a->w *= b->w;
}

inline void vvdiv(vector *a, vector *b)
{
  a->x /= b->x;
  a->y /= b->y;
  a->z /= b->z;
}

void vmix(vector *r, vector *a, vector *b, double v)
{
  double i = 1.0 - v;
  r->x = a->x * v + b->x * i;
  r->y = a->y * v + b->y * i;
  r->z = a->z * v + b->z * i;
  r->w = a->w * v + b->w * i;
}

double vmax(vector *a)
{
  double max = fabs(a->x);
  if(fabs(a->y) > max) max = fabs(a->y);
  if(fabs(a->z) > max) max = fabs(a->z);
  if(fabs(a->w) > max) max = fabs(a->w);
  return max;
}

void vavg(vector *a)
{
  double s = (a->x + a->y + a->z) / 3.0;
  a->x = a->y = a->z = s;
}


void trianglenormal(vector *n, double *t, triangle *tri)
{
  triangle tmp;
  vcopy(&tmp.b, &tri->b);
  vcopy(&tmp.c, &tri->c);
  vsub(&tmp.b, &tri->a);
  vsub(&tmp.c, &tri->a);
  vset(&tmp.a, 0,0,0);
  vcross(n, &tmp.b, &tmp.c);
  if(t)
    *t = vdot(&tmp.b, &tmp.c);
}

double checkdisc(ray *r, disc *disc)
{
  vector p, *v = &disc->a;
  double t, d;
  double i,j,k;

  i = r->v2.x - r->v1.x;
  j = r->v2.y - r->v1.y;
  k = r->v2.z - r->v1.z;

  t = -(v->x * r->v1.x + v->y * r->v1.y + v->z * r->v1.z - disc->b) /
    (v->x * i + v->y * j + v->z * k);

  p.x = r->v1.x + i * t;
  p.y = r->v1.y + j * t;
  p.z = r->v1.z + k * t;

  d = vdist(&p, v);

  if(d > disc->r) t = 0.0;

  return t;
}

double checksphere(ray *r, sphere *sphere)
{
  vector cendir, rdir;
  double dirproj, cdlensq;
  double linear, constant, rsq, quadratic, discriminant;
  double smallzero, solmin, solmax, tolerance = 0.001;

  vcopy(&rdir, &r->v2);
  vsub(&rdir, &r->v1);

  rsq = sphere->r * sphere->r;

  vcopy(&cendir, &r->v1);
  vsub(&cendir, &sphere->a);
  dirproj = vdot(&rdir, &cendir);
  cdlensq = vdot(&cendir, &cendir);

  if((cdlensq >= rsq) && (dirproj > 0.0))
    return 0.0;

  linear = 2.0 * dirproj;
  constant = cdlensq - rsq;
  quadratic = vdot(&rdir, &rdir);

  smallzero = (constant / linear);
  if((smallzero < tolerance) && (smallzero > -tolerance)) {
    solmin = -linear / quadratic;

    if (solmin > tolerance) {
      return solmin;
      /*
      *hits = solmin;
      return 1;
      */
    } else
      return 0.0;
  }
  discriminant = linear * linear - 4.0 * quadratic * constant;
  if(discriminant < 0.0)
    return 0.0;
  quadratic *= 2.0;
  discriminant = sqrt(discriminant);
  solmax = (-linear + discriminant) / (quadratic);
  solmin = (-linear - discriminant) / (quadratic);

  if (solmax < tolerance)
    return 0.0;
  
  if(solmin < tolerance) {
    return solmax;
    /*
    *hits = solmax;
    return 1;
    */
  } else {
    return solmin;
    /*
    *hits++ = solmin;
    *hits = solmax;
    return 2;
    */
  }


}

double checkcylinder(ray *r, cylinder *cylinder)
{
  /* fixme */
  return 0;
}


double checkplane(ray *r, plane *plane)
{
  vector *v = &plane->a;
  double t;
  double i,j,k;

  i = r->v2.x - r->v1.x;
  j = r->v2.y - r->v1.y;
  k = r->v2.z - r->v1.z;

  t = -(v->x * r->v1.x + v->y * r->v1.y + v->z * r->v1.z - plane->b) /
    (v->x * i + v->y * j + v->z * k);

  return t;
}

double checktri(ray *r, triangle *tri)
{
  vector ed1, ed2;
  vector tvec, pvec, qvec;
  double det, idet, t, u, v;
  vector *orig, dir;

  orig = &r->v1;
  memcpy(&dir, &r->v2, sizeof(vector));
  vsub(&dir, orig);

  ed1.x = tri->c.x - tri->a.x;
  ed1.y = tri->c.y - tri->a.y;
  ed1.z = tri->c.z - tri->a.z;
  ed2.x = tri->b.x - tri->a.x;
  ed2.y = tri->b.y - tri->a.y;
  ed2.z = tri->b.z - tri->a.z;
  vcross(&pvec, &dir, &ed2);
  det = vdot(&ed1, &pvec);

  idet = 1.0 / det;

  tvec.x = orig->x;
  tvec.y = orig->y;
  tvec.z = orig->z;
  vsub(&tvec, &tri->a);
  u = vdot(&tvec, &pvec) * idet;

  if(u < 0.0) return 0;
  if(u > 1.0) return 0;

  vcross(&qvec, &tvec, &ed1);
  v = vdot(&dir, &qvec) * idet;

  if((v < 0.0) || (u+v > 1.0)) return 0;
  
  t = vdot(&ed2, &qvec) * idet;

  return t;
}

double turbulence(double *point, double lofreq, double hifreq);

void transformpoint(vector *p, texture *t)
{
  double point[3], f;


  if((t->turbulence.x != 0.0) || (t->turbulence.y != 0.0) ||
     (t->turbulence.z != 0.0)) {
    point[0] = p->x;
    point[1] = p->y;
    point[2] = p->z;
    f = turbulence(point,1,256);
    p->x += t->turbulence.x * f;
    p->y += t->turbulence.y * f;
    p->z += t->turbulence.z * f;
  }
  if((t->rotate.x != 0.0) || (t->rotate.y != 0.0) || (t->rotate.z != 0.0))
    vvrotate(p, &t->rotate);
  vvdiv(p, &t->scale);

  vsub(p, &t->translate);
}

void checker(vector *q, vector *col, texture *t)
{
  int c = 0;
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  vmul(&p, 0.25);

  p.x += 0.00001;
  p.y += 0.00001;
  p.z += 0.00001;

  if(p.x < 0.0) p.x = 0.5 - p.x;
  if(p.y < 0.0) p.y = 0.5 - p.y;
  if(p.z < 0.0) p.z = 0.5 - p.z;
  
  if((p.x - (int)p.x) < 0.5) c ^= 1;
  if((p.y - (int)p.y) < 0.5) c ^= 1;
  if((p.z - (int)p.z) < 0.5) c ^= 1;

  if(c) { vcopy(col, &t->color1); }
  else  { vcopy(col, &t->color2); }
}

void gradcolor(vector *col, gradient *t, double val)
{
  int i;
  double d;
  vector tmpcol;

  if(val < 0.0) val = 0.0;
  if(val > 1.0) val = 1.0;
  for(i = 0; i < t->numcol; i++) {
    if(t->pos[i] == val) { vcopy(col, &t->color[i]); return; }
    if(t->pos[i] > val) {
      d = (val - t->pos[i-1]) / (t->pos[i] - t->pos[i-1]);
      vcopy(&tmpcol, &t->color[i]);
      vmul(&tmpcol, d);
      vcopy(col, &tmpcol);
      vcopy(&tmpcol, &t->color[i-1]);
      vmul(&tmpcol, 1.0 - d);
      vadd(col, &tmpcol);
      return;
    }
  }
  fprintf(stderr, "Error in gradient!\n");
  vset(col, 0,1,0);
}

void marble(vector *q, vector *col, texture *t)
{
  double f;
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  f = sin(p.x*4)/2 + 0.5;
  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);
}

void lizard(vector *q, vector *col, texture *t)
{
  double f;
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  f = fabs(sin(p.x*4));
  f += fabs(sin(p.y*4));
  f += fabs(sin(p.z*4));
  f /= 3.0;
  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);

}

void wood(vector *q, vector *col, texture *t)
{
  double f;
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  f = fabs(p.x);
  f = f - (int)f;

  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);
}

void spiral(vector *q, vector *col, texture *t)
{
  double f;
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  f = fabs(atan2(p.x, p.z)/G_PI/2 + p.y + 99999);
  f = f - (int)f;

  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);
}

void spots(vector *q, vector *col, texture *t)
{
  double f;
  vector p, r;

  vcopy(&p, q);
  transformpoint(&p, t);

  p.x += 10000.0;
  p.y += 10000.0;
  p.z += 10000.0;

  vset(&r, (int)(p.x+0.5), (int)(p.y+0.5), (int)(p.z+0.5));
  f = vdist(&p,&r);
  f = cos(f*G_PI);
  if(f < 0.0) f = 0.0;
  else if(f > 1.0) f = 1.0;
  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);
}

void perlin(vector *q, vector *col, texture *t)
{
  double f, point[3];
  vector p;

  vcopy(&p, q);
  transformpoint(&p, t);

  point[0] = p.x;
  point[1] = p.y;
  point[2] = p.z;

  f = turbulence(point,1,256)*0.3 + 0.5;

  f = pow(f, t->exp);

  if(t->flags & GRADIENT)
    gradcolor(col, &t->gradient, f);
  else
    vmix(col, &t->color1, &t->color2, f);
}

void imagepixel(vector *q, vector *col, texture *t)
{
  vector p;
  int x,y;
  unsigned char *rgb;

  vcopy(&p, q);
  transformpoint(&p, t);

  x = (p.x * t->image.xsize);
  y = (p.y * t->image.ysize);

  x = (x % t->image.xsize + t->image.xsize) % t->image.xsize;
  y = (y % t->image.ysize + t->image.ysize) % t->image.ysize;

  rgb = &t->image.rgb[x*3 + (t->image.ysize-1-y)*t->image.xsize*3];
  vset(col, rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0);
}

double frand(double v)
{
  return (RAND_FUNC() / (double)RAND_MAX - 0.5) * v;
}

int traceray(ray *r, vector *col, int level, double imp);

void objcolor(vector *col, vector *p, common *obj)
{
  int i;
  texture *t;
  vector tmpcol;

  vcset(col, 0,0,0,0);

  for(i = 0; i < obj->numtexture; i++) {
    t = &obj->texture[i];

    if(world.quality < 1) {
      vadd(col, &t->color1);
      continue;
    }

    vset(&tmpcol, 0,0,0);
    switch(t->type) {
    case SOLID:
      vcopy(&tmpcol, &t->color1);
      break;
    case CHECKER:
      checker(p, &tmpcol, t);
      break;
    case MARBLE:
      marble(p, &tmpcol, t);
      break;
    case LIZARD:
      lizard(p, &tmpcol, t);
      break;
    case PERLIN:
      perlin(p, &tmpcol, t);
      break;
    case WOOD:
      wood(p, &tmpcol, t);
      break;
    case SPIRAL:
      spiral(p, &tmpcol, t);
      break;
    case SPOTS:
      spots(p, &tmpcol, t);
      break;
    case IMAGE:
      imagepixel(p, &tmpcol, t);
      break;
    case PHONG:
    case REFRACTION:
    case REFLECTION:
    case TRANSPARENT:
    case SMOKE:
      /* Silently ignore non-color textures */
      break;
    default:
      fprintf(stderr, "Warning: unknown texture %d\n", t->type);
      break;
    }
    vmul(&tmpcol, t->amount);
    vadd(col, &tmpcol);
  }
  if(!i) {
    fprintf(stderr, "Warning: object %p has no textures\n", obj);
  }

}

void objnormal(vector *res, common *obj, vector *p)
{
  int i;

  switch(obj->type) {
  case TRIANGLE:
    trianglenormal(res, NULL, (triangle *)obj);
    break;
  case DISC:
    vcopy(res, &((disc *)obj)->a);
    break;
  case PLANE:
    vcopy(res, &((plane *)obj)->a);
    break;
  case SPHERE:
    vcopy(res, &((sphere *)obj)->a);
    vsub(res, p);
    break;
  case CYLINDER:
    vset(res, 1,1,1); /* fixme */
    break;
  default:
    fprintf(stderr, "objnormal(): Unsupported object type!?\n");
    exit(0);
  }
  vnorm(res, 1.0);

  for(i = 0; i < obj->numnormal; i++) {
    vector tmpcol[3];
    vector q[3], rot = {90,90,90};
    texture *t = &obj->normal[i];
    double nstep = 0.1;

    vcopy(&q[0], p);
    vcopy(&q[1], res); vvrotate(&q[1], &rot);
    vcross(&q[1], &q[1], res);
    vnorm(&q[1], nstep);
    vcross(&q[2], &q[1], res);
    vnorm(&q[2], nstep);

    vadd(&q[1], p);
    vadd(&q[2], p);

    vset(&tmpcol[0], 0,0,0);
    vset(&tmpcol[1], 0,0,0);
    vset(&tmpcol[2], 0,0,0);
    switch(t->type) {
    case MARBLE:
      marble(&q[0], &tmpcol[0], t);
      marble(&q[1], &tmpcol[1], t);
      marble(&q[2], &tmpcol[2], t);
      break;
    case LIZARD:
      lizard(&q[0], &tmpcol[0], t);
      lizard(&q[1], &tmpcol[1], t);
      lizard(&q[2], &tmpcol[2], t);
      break;
    case PERLIN:
      perlin(&q[0], &tmpcol[0], t);
      perlin(&q[1], &tmpcol[1], t);
      perlin(&q[2], &tmpcol[2], t);
      break;
    case WOOD:
      wood(&q[0], &tmpcol[0], t);
      wood(&q[1], &tmpcol[1], t);
      wood(&q[2], &tmpcol[2], t);
      break;
    case SPIRAL:
      spiral(&q[0], &tmpcol[0], t);
      spiral(&q[1], &tmpcol[1], t);
      spiral(&q[2], &tmpcol[2], t);
      break;
    case SPOTS:
      spots(&q[0], &tmpcol[0], t);
      spots(&q[1], &tmpcol[1], t);
      spots(&q[2], &tmpcol[2], t);
      break;
    case IMAGE:
      imagepixel(&q[0], &tmpcol[0], t);
      imagepixel(&q[1], &tmpcol[1], t);
      imagepixel(&q[2], &tmpcol[2], t);
      break;
    case CHECKER:
    case SOLID:
    case PHONG:
    case REFRACTION:
    case REFLECTION:
    case TRANSPARENT:
    case SMOKE:
      continue;
      break;
    default:
      fprintf(stderr, "Warning: unknown texture %d\n", t->type);
      break;
    }
    vavg(&tmpcol[0]);
    vavg(&tmpcol[1]);
    vavg(&tmpcol[2]);
    vsub(&tmpcol[1], &tmpcol[0]);
    vsub(&tmpcol[2], &tmpcol[0]);
    vsub(&q[1], &q[0]);
    vsub(&q[2], &q[0]);
    vadd(&q[1], &tmpcol[1]);
    vadd(&q[2], &tmpcol[2]);
    vcross(&q[0], &q[1], &q[2]);
    vnorm(&q[0], 1.0);
    vmul(&q[0], t->amount);
    vadd(res, &q[0]);
    vnorm(res, 1.0);
  }
}

/* 
   Quality:
   0 = Color only
   1 = Textures
   2 = Light + Normals
   3 = Shadows
   4 = Phong
   5 = Reflection + Refraction
 */

void calclight(vector *col, vector *point, common *obj)
{
  int i, j, o;
  ray r;
  double d, b, a;
  vector lcol;
  vector norm;
  vector pcol;

  vcset(col, 0,0,0,0);

  objcolor(&pcol, point, obj);
  a = pcol.w;

  if(world.quality < 2) {
    vcopy(col, &pcol);
    return;
  }

  for(i = 0; i < obj->numtexture; i++) {
    if(obj->texture[i].type == PHONG) continue;
    if(obj->texture[i].type == REFLECTION) continue;
    if(obj->texture[i].type == REFRACTION) continue;
    if(obj->texture[i].type == TRANSPARENT) continue;
    if(obj->texture[i].type == SMOKE) continue;
    vcopy(&lcol, &pcol);
    vvmul(&lcol, &obj->texture[i].ambient);
    vadd(col, &lcol);
  }

  objnormal(&norm, obj, point);
  vnorm(&norm, 1.0);

  r.inside = -1;
  r.ior = 1.0;

  for(i = 0; i < world.numlight; i++) {
    vcopy(&r.v1, point);
    vcopy(&r.v2, &world.light[i].a);
    vmix(&r.v1, &r.v1, &r.v2, 0.9999);
    d = vdist(&r.v1, &r.v2);

    if(world.quality >= 3) {
      o = 0;
      if(!(world.light[i].com.flags & NOSHADOW))
	o = traceray(&r, NULL, -1, 1.0);
      if(o) {
	continue;
      }
    }

    /* OK, light is visible */

    vsub(&r.v1, &r.v2);
    vnorm(&r.v1, 1.0);
    b = vdot(&r.v1, &norm);

    b = fabs(b);

    for(j = 0; j < obj->numtexture; j++) {
      if(obj->texture[j].type == PHONG) continue;
      if(obj->texture[j].type == REFLECTION) continue;
      if(obj->texture[j].type == REFRACTION) continue;
      if(obj->texture[i].type == TRANSPARENT) continue;
      if(obj->texture[i].type == SMOKE) continue;
      vcopy(&lcol, &pcol);
      vvmul(&lcol, &world.light[i].color);
      vvmul(&lcol, &obj->texture[j].diffuse);
      vmul(&lcol, b);
      vadd(col, &lcol);
    }
  }
  col->w = a;
}

void calcphong(common *obj, ray *r2, vector *col)
{
  int i, j, o;
  ray r;
  double d, b;
  vector lcol;
  vector norm;
  vector pcol;
  double ps;

  vcopy(&pcol, col);

  vcopy(&norm, &r2->v2);
  vsub(&norm, &r2->v1);
  vnorm(&norm, 1.0);

  r.inside = -1;
  r.ior = 1.0;

  for(i = 0; i < world.numlight; i++) {
    vcopy(&r.v1, &r2->v1);
    vcopy(&r.v2, &world.light[i].a);
    vmix(&r.v1, &r.v1, &r.v2, 0.9999);
    d = vdist(&r.v1, &r.v2);

    o = traceray(&r, NULL, -1, 1.0);
    if(o) {
      continue;
    }

    /* OK, light is visible */

    vsub(&r.v1, &r.v2);
    vnorm(&r.v1, 1.0);
    b = -vdot(&r.v1, &norm);
    
    for(j = 0; j < obj->numtexture; j++) {
      if(obj->texture[j].type != PHONG) continue;

      ps = obj->texture[j].phongsize;

      if(b < (1-ps)) continue;
      ps = (b-(1-ps))/ps;

      vcopy(&lcol, &obj->texture[j].phongcolor);
      vvmul(&lcol, &world.light[i].color);
      vmul(&lcol, ps);
      vadd(col, &lcol);
    }
  }
}

int traceray(ray *r, vector *col, int level, double imp)
{
  int i, b = -1;
  double t = -1.0, min = 0.0;
  int type = -1;
  common *obj, *bobj = NULL;
  int hits = 0;
  vector p;

  if((level == 0) || (imp < 0.005)) {
    vset(col, 0,1,0);
    return 0;
  }

  for(i = 0; i < world.numobj; i++) {
    obj = (common *)&world.obj[i];
    switch(obj->type) {
    case TRIANGLE:
      t = checktri(r, (triangle *)&world.obj[i]);
      break;
    case DISC:
      t = checkdisc(r, (disc *)&world.obj[i]);
      break;
    case PLANE:
      t = checkplane(r, (plane *)&world.obj[i]);
      break;
    case SPHERE:
      t = checksphere(r, (sphere *)&world.obj[i]);
      break;
    case CYLINDER:
      t = checkcylinder(r, (cylinder *)&world.obj[i]);
      break;
    default:
      fprintf(stderr, "Illegal object!!\n");
      exit(0);
    }
    if(t <= 0.0) continue;

    if(!(obj->flags & NOSHADOW) && (level == -1)) {
      return i+1;
    }

    hits++;
    if((!bobj) || (t < min)) {

      min = t;
      b = i;
      type = obj->type;
      bobj = obj;
    }
  }
  if(level == -1) return 0;

  if(bobj) {
    p.x = r->v1.x + (r->v2.x - r->v1.x) * min;
    p.y = r->v1.y + (r->v2.y - r->v1.y) * min;
    p.z = r->v1.z + (r->v2.z - r->v1.z) * min;

    calclight(col, &p, bobj);

    if(world.flags & SMARTAMBIENT) {
      double ambient = 0.3 * exp(-min/world.smartambient);
      vector lcol;
      objcolor(&lcol, &p, bobj);
      vmul(&lcol, ambient);
      vadd(col, &lcol);
    }

    for(i = 0; i < bobj->numtexture; i++) {
      
      if((world.quality >= 4) && ((bobj->texture[i].type == REFLECTION) || (bobj->texture[i].type == PHONG))) {

	vector refcol, norm;
	ray ref;
	vcopy(&ref.v1, &p);
	vcopy(&ref.v2, &r->v1);
	ref.inside = r->inside;
	ref.ior = r->ior;

	vmix(&ref.v1, &ref.v1, &ref.v2, 0.9999); /* push it a tad */

	vsub(&ref.v2, &p);
	objnormal(&norm, bobj, &p);
	vnorm(&norm, 1.0);
	vrotate(&norm, 180.0, &ref.v2);

	vmul(&norm, -0.0001); /* push it a tad */
	vadd(&ref.v1, &norm);

	vnorm(&ref.v2, 1.0);
	vadd(&ref.v2, &p);

	if((world.quality >= 5) && (bobj->texture[i].type == REFLECTION)) {
	  traceray(&ref, &refcol, level - 1, imp * vmax(&bobj->texture[i].reflection));
	  vvmul(&refcol, &bobj->texture[i].reflection);
	  vadd(col, &refcol);
	}
	if(bobj->texture[i].type == PHONG) {
	  vcset(&refcol,0,0,0,0);
	  calcphong(bobj, &ref, &refcol);
	  vadd(col, &refcol);
	}

      }

      if((world.quality >= 5) && (col->w < 1.0)) {
	vector refcol;
	ray ref;

	vcopy(&ref.v1, &p);
	vcopy(&ref.v2, &p);
	vsub(&ref.v2, &r->v1);
	vnorm(&ref.v2, 1.0);
	vadd(&ref.v2, &p);

	vmix(&ref.v1, &ref.v1, &ref.v2, 0.999); /* push it a tad */
	traceray(&ref, &refcol, level - 1, imp * (1.0 - col->w));
	vmul(&refcol, (1.0 - col->w));
	vadd(col, &refcol);
      }

      if((world.quality >= 5) && (bobj->texture[i].type == TRANSPARENT)) {
	vector refcol;
	ray ref;

	vcopy(&ref.v1, &p);
	vcopy(&ref.v2, &p);
	vsub(&ref.v2, &r->v1);
	vnorm(&ref.v2, 1.0);
	vadd(&ref.v2, &p);

	vmix(&ref.v1, &ref.v1, &ref.v2, 0.999); /* push it a tad */

	traceray(&ref, &refcol, level - 1, imp * vmax(&bobj->texture[i].transparent));
	vvmul(&refcol, &bobj->texture[i].transparent);

	vadd(col, &refcol);
      }

      if((world.quality >= 5) && (bobj->texture[i].type == SMOKE)) {
	vector smcol, raydir, norm;
	double tran;
	ray ref;

	vcopy(&ref.v1, &p);
	vcopy(&ref.v2, &p);
	vsub(&ref.v2, &r->v1);
	vnorm(&ref.v2, 1.0);
	vadd(&ref.v2, &p);

	objnormal(&norm, bobj, &p);
	vcopy(&raydir, &r->v2);
	vsub(&raydir, &r->v1);
	vnorm(&raydir, 1.0);
	tran = vdot(&norm, &raydir);
	if(tran < 0.0) continue;
	tran *= tran;
	vcopy(&smcol, &bobj->texture[i].color1);
	vmul(&smcol, tran);
	vadd(col, &smcol);
      }

      if((world.quality >= 5) && (bobj->texture[i].type == REFRACTION)) {
	vector refcol, norm, tmpv;
	ray ref;
	double c1, c2, n1, n2, n;

	vcopy(&ref.v1, &p);
	vcopy(&ref.v2, &p);
	vsub(&ref.v2, &r->v1);
	vadd(&ref.v2, &r->v2);

	vmix(&ref.v1, &ref.v1, &ref.v2, 0.999); /* push it a tad */

	vsub(&ref.v2, &p);
	objnormal(&norm, bobj, &p);

	if(r->inside == b) {
	  ref.inside = -1;
	  ref.ior = 1.0;
	} else {
	  ref.inside = b;
	  ref.ior = bobj->texture[i].ior;
	}

	c1 = vdot(&norm, &ref.v2);

	if(ref.inside < 0) c1 = -c1;
	
	n1 = r->ior;   /* IOR of current media  */
	n2 = ref.ior;  /* IOR of new media  */
	n = n1 / n2;
	c2 = 1.0 - n*n * (1.0 - c1*c1);

	if(c2 < 0.0) {
	  /* FIXME: Internal reflection should occur */
	  c2 = sqrt(-c2);

	} else {
	  c2 = sqrt(c2);
	}

	vmul(&ref.v2, n);
	vcopy(&tmpv, &norm);
	vmul(&tmpv, n * c1 - c2);
	vadd(&ref.v2, &tmpv);

	vnorm(&ref.v2, 1.0);
	vadd(&ref.v2, &p);

	traceray(&ref, &refcol, level - 1, imp * vmax(&bobj->texture[i].refraction));

	vvmul(&refcol, &bobj->texture[i].refraction);
	vadd(col, &refcol);
      }
    }
  } else {
    vcset(col, 0,0,0,0);
    min = 10000.0;
    vcset(&p,0,0,0,0);
  }

  for(i = 0; i < world.numatmos; i++) {
    vector tmpcol;
    if(world.atmos[i].type == FOG) {
      double v, pt[3];
      pt[0] = p.x; pt[1] = p.y; pt[2] = p.z;
      if((v = world.atmos[i].turbulence) > 0.0)
	v = turbulence(pt,1,256) * world.atmos[i].turbulence;
      v = exp(-(min+v) / world.atmos[i].density);
      vmul(col, v);
      vcopy(&tmpcol, &world.atmos[i].color);
      vmul(&tmpcol, 1.0 - v);
      vadd(col, &tmpcol);
    }
  }

  return hits;
}

void setdefaults(texture *t)
{
  memset(t, 0, sizeof(texture));
  t->type = SOLID;
  vcset(&t->color1, 1,1,1,1);
  vcset(&t->color2, 0,0,0,1);
  vcset(&t->diffuse, 1,1,1,1);
  vcset(&t->ambient, 0,0,0,1);
  vset(&t->scale,1,1,1);
  vset(&t->rotate,0,0,0);
  vset(&t->translate,0,0,0);
  t->amount = 1.0;
  t->exp = 1.0;
}

char *mklabel(texture *t)
{
  struct textures_t *l;
  static char tmps[100];
  if(t->majtype == 0) strcpy(tmps, "Texture");
  else if(t->majtype == 1) strcpy(tmps, "Bumpmap");
  else if(t->majtype == 2) strcpy(tmps, "Light");
  else strcpy(tmps, "(unknown!?)");
  if((t->majtype == 0) || (t->majtype == 1)) {
    strcat(tmps, " / ");
    l = textures;
    while(l->s) {
      if(t->type == l->n) {
	strcat(tmps, l->s);
	break;
    }
      l++;
    }
  }
  return tmps;
}

GtkWidget *currentitem(GtkWidget *list)
{
  GList *h;
  GtkWidget *tmpw;
  h = GTK_LIST(list)->selection;
  if(!h) return NULL;
  tmpw = h->data;
  return tmpw;
}

texture *currenttexture(void)
{
  GtkWidget *tmpw;
  texture *t;
  tmpw = currentitem(texturelist);
  if(!tmpw) return NULL;
  t = gtk_object_get_data(GTK_OBJECT(tmpw), "texture");
  return t;
}

void relabel(void)
{
  GtkWidget *tmpw = currentitem(texturelist);
  texture *t = currenttexture();
  if(!tmpw || !t) return;
  tmpw = GTK_BIN(tmpw)->child;
  gtk_label_set_text(GTK_LABEL(tmpw), mklabel(t));
}

int noupdate = 0;

void setvals(texture *t)
{
  struct textures_t *l;

  if(!t) return;

  noupdate = 1;
  gtk_adjustment_set_value(GTK_ADJUSTMENT(amountscale), t->amount);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(scalexscale), t->scale.x);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(scaleyscale), t->scale.y);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(scalezscale), t->scale.z);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(rotxscale), t->rotate.x);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(rotyscale), t->rotate.y);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(rotzscale), t->rotate.z);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(posxscale), t->translate.x);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(posyscale), t->translate.y);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(poszscale), t->translate.z);

  gtk_adjustment_set_value(GTK_ADJUSTMENT(turbulencescale), t->turbulence.x);
  gtk_adjustment_set_value(GTK_ADJUSTMENT(expscale), t->exp);

  drawcolor1(NULL);
  drawcolor2(NULL);

  l = textures;
  while(l->s) {
    if(l->n == t->type) {
      gtk_option_menu_set_history(GTK_OPTION_MENU(texturemenu), l->index);
      break;
    }
    l++;
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(typemenu), t->majtype);

  noupdate = 0;
}


void selectitem(GtkWidget *wg, GtkWidget *p)
{
  setvals(currenttexture());
}

void addtexture(void)
{
  GtkWidget *item;
  int n = s.com.numtexture;

  if(n == MAXTEXTUREPEROBJ-1) return;

  setdefaults(&s.com.texture[n]);

  item = gtk_list_item_new_with_label(mklabel(&s.com.texture[n]));
  gtk_object_set_data (GTK_OBJECT(item), "texture", &s.com.texture[n]);
  gtk_container_add(GTK_CONTAINER(texturelist), item);
  gtk_widget_show(item);

  gtk_list_select_child(GTK_LIST(texturelist), item);

  s.com.numtexture++;
  restartrender();
}

void duptexture(void)
{
  GtkWidget *item;
  texture *t = currenttexture();
  int n = s.com.numtexture;

  if(n == MAXTEXTUREPEROBJ-1) return;
  if(!t) return;

  memcpy(&s.com.texture[n], t, sizeof(texture));

  item = gtk_list_item_new_with_label(mklabel(&s.com.texture[n]));
  gtk_object_set_data (GTK_OBJECT(item), "texture", &s.com.texture[n]);
  gtk_container_add(GTK_CONTAINER(texturelist), item);
  gtk_widget_show(item);

  gtk_list_select_child(GTK_LIST(texturelist), item);

  s.com.numtexture++;
  restartrender();
}

void rebuildlist(void)
{
  GtkWidget *item;
  int n;

  for(n = 0; n < s.com.numtexture; n++) {
    if(s.com.numtexture && (s.com.texture[n].majtype < 0)) {
      int i;
      for(i = n; i < s.com.numtexture-1; i++)
	memcpy(&s.com.texture[i],&s.com.texture[i+1],sizeof(texture));
      s.com.numtexture--;
      n--;
    }
  }

  for(n = 0; n < s.com.numtexture; n++) {
    item = gtk_list_item_new_with_label(mklabel(&s.com.texture[n]));
    gtk_object_set_data (GTK_OBJECT(item), "texture", &s.com.texture[n]);
    gtk_container_add(GTK_CONTAINER(texturelist), item);
    gtk_widget_show(item);
  }
  restartrender();
}

void sphere_reset(void)
{
  s.com.numtexture = 3;

  setdefaults(&s.com.texture[0]);
  setdefaults(&s.com.texture[1]);
  setdefaults(&s.com.texture[2]);

  s.com.texture[1].majtype = 2;
  vset(&s.com.texture[1].color1, 1,1,1);
  vset(&s.com.texture[1].translate, -15,-15,-15);

  s.com.texture[2].majtype = 2;
  vset(&s.com.texture[2].color1, 0,0.4,0.4);
  vset(&s.com.texture[2].translate, 15,15,-15);

  gtk_list_clear_items(GTK_LIST(texturelist), 0, -1);
  rebuildlist();
  restartrender();
}

void deltexture(void)
{
  texture *t;
  GtkWidget *tmpw;
  tmpw = currentitem(texturelist);
  if(!tmpw) return;
  t = currenttexture();
  if(!t) return;
  t->majtype = -1;
  gtk_widget_destroy(tmpw);
}

void initworld(void)
{
  int i;

  memset(&world, 0, sizeof(world));

  s.com.type = SPHERE;
  s.a.x = s.a.y = s.a.z = 0.0;
  s.r = 4.0;

  memcpy(&world.obj[0], &s, sizeof(s));
  world.numobj = 1;

  world.obj[0].com.numtexture = 0;
  world.obj[0].com.numnormal = 0;
  
  for(i = 0; i < s.com.numtexture; i++) {
    common *c = &s.com;
    common *d = &world.obj[0].com;
    texture *t = &c->texture[i];
    if((t->amount <= 0.0) || (t->majtype < 0)) continue;
    if(t->majtype == 0) { /* Normal texture */
      if(t->type == PHONG) {
	memcpy(&t->phongcolor, &t->color1, sizeof(t->color1));
	t->phongsize = t->scale.x / 25.0;
      }
      memcpy(&d->texture[d->numtexture],t,sizeof(texture));
      d->numtexture++;
    } else if(t->majtype == 1) { /* Bumpmap */
      memcpy(&d->normal[d->numnormal],t,sizeof(texture));
      d->numnormal++;
    } else if(t->majtype == 2) { /* Lightsource */
      light l;
      vcopy(&l.a, &t->translate);
      vcopy(&l.color, &t->color1);
      memcpy(&world.light[world.numlight], &l, sizeof(l));
      world.numlight++;
    }
  }

  world.quality = 5;

  world.flags |= SMARTAMBIENT;
  world.smartambient = 40.0;
}

void drawit(gpointer data)
{
  if(!drawarea) return;
  gdk_draw_rgb_image(drawarea->window,
                     drawarea->style->white_gc,
                     0, 0, PREVIEWSIZE, PREVIEWSIZE, GDK_RGB_DITHER_MAX,
                     data, PREVIEWSIZE*3);
}

static gint
expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  drawit(img);
  return FALSE;
}

gint restartrender(void)
{
  if(running) {
    running = 2;
    return 0;
  }
  render();
  return 0;
}

void destroy_window(GtkWidget  *widget, GtkWidget **window)
{
  *window = NULL;
}

void selecttexture(GtkWidget *wg, gpointer data)
{
  texture *t;
  int n = (long)data;
  if(noupdate) return;
  t = currenttexture();
  if(!t) return;
  t->type = n;
  relabel();
  restartrender();
}

void selecttype(GtkWidget *wg, gpointer data)
{
  texture *t;
  long n = (long)data;
  if(noupdate) return;
  t = currenttexture();
  if(!t) return;
  t->majtype = n;
  relabel();
  restartrender();
}

void getscales(GtkWidget *wg, gpointer data)
{
  double f;
  texture *t;
  if(noupdate) return;
  t = currenttexture();
  if(!t) return;
  t->amount = GTK_ADJUSTMENT(amountscale)->value;
  t->exp = GTK_ADJUSTMENT(expscale)->value;
  f = GTK_ADJUSTMENT(turbulencescale)->value;
  vset(&t->turbulence, f,f,f);

  t->scale.x = GTK_ADJUSTMENT(scalexscale)->value;
  t->scale.y = GTK_ADJUSTMENT(scaleyscale)->value;
  t->scale.z = GTK_ADJUSTMENT(scalezscale)->value;

  t->rotate.x = GTK_ADJUSTMENT(rotxscale)->value;
  t->rotate.y = GTK_ADJUSTMENT(rotyscale)->value;
  t->rotate.z = GTK_ADJUSTMENT(rotzscale)->value;

  t->translate.x = GTK_ADJUSTMENT(posxscale)->value;
  t->translate.y = GTK_ADJUSTMENT(posyscale)->value;
  t->translate.z = GTK_ADJUSTMENT(poszscale)->value;
}

void mktexturemenu(GtkWidget *texturemenu_menu)
{
  GtkWidget *item;
  struct textures_t *t;

  t = textures;
  while(t->s) {
    item = gtk_menu_item_new_with_label (t->s);
    gtk_widget_show(item);
    gtk_menu_append (GTK_MENU (texturemenu_menu), item);
    gtk_signal_connect (GTK_OBJECT(item), "activate",
			GTK_SIGNAL_FUNC(selecttexture), (void *)t->n);
    t++;
  }
}

void drawcolor1(GtkWidget *w)
{
  static GtkWidget *lastw = NULL;
  int x, y;
  guchar buf[COLORBUTTONWIDTH*3];
  texture *t = currenttexture();

  if(w) lastw = w;
  else w = lastw;
  if(!w) return;
  if(!t) return;

  for(x = 0; x < COLORBUTTONWIDTH; x++) {
    buf[x*3+0] = t->color1.x * 255.0;
    buf[x*3+1] = t->color1.y * 255.0;
    buf[x*3+2] = t->color1.z * 255.0;
  }
  for(y = 0; y < COLORBUTTONHEIGHT; y++)
    gtk_preview_draw_row (GTK_PREVIEW(w), buf, 0, y, COLORBUTTONWIDTH);
  gtk_widget_draw(w, NULL);
}

void drawcolor2(GtkWidget *w)
{
  static GtkWidget *lastw = NULL;
  int x, y;
  guchar buf[COLORBUTTONWIDTH*3];
  texture *t = currenttexture();

  if(w) lastw = w;
  else w = lastw;
  if(!w) return;
  if(!t) return;

  for(x = 0; x < COLORBUTTONWIDTH; x++) {
    buf[x*3+0] = t->color2.x * 255.0;
    buf[x*3+1] = t->color2.y * 255.0;
    buf[x*3+2] = t->color2.z * 255.0;
  }
  for(y = 0; y < COLORBUTTONHEIGHT; y++)
    gtk_preview_draw_row (GTK_PREVIEW(w), buf, 0, y, COLORBUTTONWIDTH);
  gtk_widget_draw(w, NULL);
}

void selectcolor1_ok(GtkWidget *w, gpointer d)
{
  texture *t = currenttexture();
  GtkWidget *win = (GtkWidget *)d;
  gdouble tmpcol[4];
  if(!t) return;
  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (win)->colorsel), tmpcol);
  t->color1.x = tmpcol[0];
  t->color1.y = tmpcol[1];
  t->color1.z = tmpcol[2];
  t->color1.w = tmpcol[3];
  drawcolor1(NULL);
  gtk_widget_destroy(win);
}

void selectcolor2_ok(GtkWidget *w, gpointer d)
{
  texture *t = currenttexture();
  GtkWidget *win = (GtkWidget *)d;
  gdouble tmpcol[4];
  if(!t) return;
  gtk_color_selection_get_color (GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (win)->colorsel), tmpcol);
  t->color2.x = tmpcol[0];
  t->color2.y = tmpcol[1];
  t->color2.z = tmpcol[2];
  t->color2.w = tmpcol[3];
  drawcolor2(NULL);
  gtk_widget_destroy(win);
}

void selectcolor1(vector *col)
{
  static GtkWidget *window = NULL;
  gdouble tmpcol[4];
  texture *t = currenttexture();

  if(!t) return;

  if(window) {
    gtk_widget_show(window);
    gdk_window_raise(window->window);
    return;
  }

  window = gtk_color_selection_dialog_new("Color Selection Dialog");
  
  gtk_color_selection_set_opacity (GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG (window)->colorsel), TRUE);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                      &window);
  tmpcol[0] = t->color1.x;
  tmpcol[1] = t->color1.y;
  tmpcol[2] = t->color1.z;
  tmpcol[3] = t->color1.w;

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(window)->colorsel), tmpcol);

  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->ok_button),
		     "clicked",
                     GTK_SIGNAL_FUNC(selectcolor1_ok), window);
  gtk_signal_connect_object(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT (window));
  gtk_widget_show (window);
}

void selectcolor2(vector *col)
{
  static GtkWidget *window = NULL;
  gdouble tmpcol[4];
  texture *t = currenttexture();

  if(!t) return;

  if(window) {
    gtk_widget_show(window);
    gdk_window_raise(window->window);
    return;
  }

  window = gtk_color_selection_dialog_new("Color Selection Dialog");

  gtk_color_selection_set_opacity (GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG (window)->colorsel), TRUE);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC(gtk_widget_destroyed),
                      &window);
  tmpcol[0] = t->color2.x;
  tmpcol[1] = t->color2.y;
  tmpcol[2] = t->color2.z;
  tmpcol[3] = t->color2.w;

  gtk_color_selection_set_color(GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(window)->colorsel), tmpcol);

  gtk_signal_connect(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->ok_button),
		     "clicked",
                     GTK_SIGNAL_FUNC(selectcolor2_ok), window);
  gtk_signal_connect_object(GTK_OBJECT(GTK_COLOR_SELECTION_DIALOG(window)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
                            GTK_OBJECT (window));
  gtk_widget_show (window);
}

int do_run = 0;

void sphere_ok(GtkWidget *widget, gpointer data)
{
  running = -1;
  do_run = 1;
  gtk_widget_hide (GTK_WIDGET (data));
  gtk_main_quit();
}

void sphere_cancel(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_main_quit();
}

GtkWidget* makewindow (void)
{
  GtkWidget *window;
  GtkWidget *table1;
  GtkWidget *frame2;
  GtkWidget *frame3;
  GtkWidget *viewport2;
  GtkWidget *hbox1;
  GtkWidget *addbutton;
  GtkWidget *dupbutton;
  GtkWidget *delbutton;
  GtkWidget *hbox2;
  GtkWidget *okbutton;
  GtkWidget *cancelbutton;
  GtkWidget *resetbutton;
  GtkWidget *frame4;
  GtkWidget *table2;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *label4;
  GtkWidget *hbox3;
  GtkWidget *colorbutton1;
  GtkWidget *colorbutton2;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkWidget *item;
  GtkWidget *label7;
  GtkWidget *label8;
  GtkWidget *updatebutton;
  GtkWidget *label1;
  GtkWidget *_scalescale;
  GtkWidget *_rotscale;
  GtkWidget *_turbulencescale;
  GtkWidget *_amountscale;
  GtkWidget *_expscale;
  GtkWidget *tmpw;

  gdk_rgb_set_verbose(FALSE);
  gdk_rgb_init ();
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
  gtk_widget_set_default_visual (gdk_rgb_get_visual ());

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "window", window);
  gtk_container_border_width (GTK_CONTAINER (window), 5);
  gtk_window_set_title (GTK_WINDOW (window), "SphereDesigner");
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);

  table1 = gtk_table_new (3, 3, FALSE);
  gtk_object_set_data (GTK_OBJECT (window), "table1", table1);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (window), table1);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

  frame2 = gtk_frame_new ("Preview");
  gtk_object_set_data (GTK_OBJECT (window), "frame2", frame2);
  gtk_widget_show (frame2);
  gtk_table_attach (GTK_TABLE (table1), frame2, 0, 1, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  drawarea = gtk_drawing_area_new ();
  gtk_object_set_data (GTK_OBJECT (window), "drawarea", drawarea);
  gtk_widget_show (drawarea);
  gtk_container_add (GTK_CONTAINER (frame2), drawarea);
  gtk_widget_set_usize (drawarea, PREVIEWSIZE, PREVIEWSIZE);
  gtk_signal_connect (GTK_OBJECT (drawarea), "expose_event",
                      (GtkSignalFunc) expose_event, NULL);

  frame3 = gtk_frame_new ("Textures");
  gtk_object_set_data (GTK_OBJECT (window), "frame3", frame3);
  gtk_widget_show (frame3);
  gtk_table_attach (GTK_TABLE (table1), frame3, 1, 2, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  viewport2 = gtk_viewport_new (NULL, NULL);
  gtk_object_set_data (GTK_OBJECT (window), "viewport2", viewport2);
  gtk_widget_set_usize (viewport2, 150, -1);
  gtk_widget_show (viewport2);
  gtk_container_add (GTK_CONTAINER (frame3), viewport2);

  texturelist = gtk_list_new ();
  gtk_object_set_data (GTK_OBJECT (window), "texturelist", texturelist);
  gtk_widget_show (texturelist);
  gtk_container_add (GTK_CONTAINER (viewport2), texturelist);
  gtk_signal_connect (GTK_OBJECT(texturelist), "selection_changed",
                      GTK_SIGNAL_FUNC(selectitem), texturelist);

  hbox1 = gtk_hbox_new (TRUE, 0);
  gtk_object_set_data (GTK_OBJECT (window), "hbox1", hbox1);
  gtk_widget_show (hbox1);
  gtk_table_attach (GTK_TABLE (table1), hbox1, 1, 2, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  addbutton = gtk_button_new_with_label ("Add");
  gtk_object_set_data (GTK_OBJECT (window), "addbutton", addbutton);
  gtk_widget_show (addbutton);
  gtk_box_pack_start (GTK_BOX (hbox1), addbutton, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (addbutton), "clicked",
                             GTK_SIGNAL_FUNC (addtexture), NULL);

  dupbutton = gtk_button_new_with_label ("Dup");
  gtk_object_set_data (GTK_OBJECT (window), "dupbutton", dupbutton);
  gtk_widget_show (dupbutton);
  gtk_box_pack_start (GTK_BOX (hbox1), dupbutton, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (dupbutton), "clicked",
                             GTK_SIGNAL_FUNC (duptexture), NULL);

  delbutton = gtk_button_new_with_label ("Del");
  gtk_object_set_data (GTK_OBJECT (window), "delbutton", delbutton);
  gtk_widget_show (delbutton);
  gtk_box_pack_start (GTK_BOX (hbox1), delbutton, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (delbutton), "clicked",
                             GTK_SIGNAL_FUNC (deltexture), NULL);

  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_object_set_data (GTK_OBJECT (window), "hbox2", hbox2);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table1), hbox2, 0, 3, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  okbutton = gtk_button_new_with_label ("OK");
  gtk_object_set_data (GTK_OBJECT (window), "okbutton", okbutton);
  gtk_widget_show (okbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), okbutton, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (okbutton), "clicked",
		      GTK_SIGNAL_FUNC (sphere_ok), window);

  cancelbutton = gtk_button_new_with_label ("Cancel");
  gtk_object_set_data (GTK_OBJECT (window), "cancelbutton", cancelbutton);
  gtk_widget_show (cancelbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), cancelbutton, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
		      GTK_SIGNAL_FUNC (sphere_cancel), window);
  
  resetbutton = gtk_button_new_with_label ("Reset");
  gtk_object_set_data (GTK_OBJECT (window), "resetbutton", cancelbutton);
  gtk_widget_show (resetbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), resetbutton, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (resetbutton), "clicked",
		      GTK_SIGNAL_FUNC (sphere_reset), window);

  frame4 = gtk_frame_new ("Properties");
  gtk_object_set_data (GTK_OBJECT (window), "frame4", frame4);
  gtk_widget_show (frame4);
  gtk_table_attach (GTK_TABLE (table1), frame4, 2, 3, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  table2 = gtk_table_new (6, 4, FALSE);
  gtk_object_set_data (GTK_OBJECT (window), "table2", table2);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (frame4), table2);
  gtk_container_border_width (GTK_CONTAINER (table2), 5);

  label2 = gtk_label_new ("Type:");
  gtk_object_set_data (GTK_OBJECT (window), "label2", label2);
  gtk_widget_show (label2);
  gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

  label3 = gtk_label_new ("Texture:");
  gtk_object_set_data (GTK_OBJECT (window), "label3", label3);
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table2), label3, 0, 1, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

  label4 = gtk_label_new ("Colors:");
  gtk_object_set_data (GTK_OBJECT (window), "label4", label4);
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table2), label4, 0, 1, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (window), "hbox3", hbox3);
  gtk_widget_show (hbox3);
  gtk_table_attach (GTK_TABLE (table2), hbox3, 1, 2, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);

  colorbutton1 = gtk_button_new();
  gtk_object_set_data (GTK_OBJECT (window), "colorbutton1", colorbutton1);
  gtk_widget_show (colorbutton1);
  gtk_box_pack_start (GTK_BOX (hbox3), colorbutton1, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (colorbutton1), "clicked",
                      (GtkSignalFunc)selectcolor1, NULL);
  tmpw = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (tmpw), COLORBUTTONWIDTH, COLORBUTTONHEIGHT);
  gtk_container_add (GTK_CONTAINER (colorbutton1), tmpw);
  gtk_widget_show(tmpw);
  drawcolor1(tmpw);

  colorbutton2 = gtk_button_new();
  gtk_object_set_data (GTK_OBJECT (window), "colorbutton2", colorbutton2);
  gtk_widget_show (colorbutton2);
  gtk_box_pack_start (GTK_BOX (hbox3), colorbutton2, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (colorbutton2), "clicked",
                      (GtkSignalFunc)selectcolor2, NULL);
  tmpw = gtk_preview_new(GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (tmpw), COLORBUTTONWIDTH, COLORBUTTONHEIGHT);
  gtk_container_add (GTK_CONTAINER (colorbutton2), tmpw);
  gtk_widget_show(tmpw);
  drawcolor2(tmpw);

  label5 = gtk_label_new ("Turbulence:");
  gtk_object_set_data (GTK_OBJECT (window), "label5", label5);
  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table2), label5, 0, 1, 4, 5,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);

  _turbulencescale = gtk_hscale_new (GTK_ADJUSTMENT (turbulencescale = gtk_adjustment_new (0.0, 0.0, 5.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_turbulencescale", _turbulencescale);
  gtk_widget_show (_turbulencescale);
  gtk_table_attach (GTK_TABLE (table2), _turbulencescale, 1, 2, 4, 5,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_scale_set_digits (GTK_SCALE (_turbulencescale), 2);
  gtk_signal_connect(GTK_OBJECT(turbulencescale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);





  label6 = gtk_label_new ("Scale X:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (scalexscale = gtk_adjustment_new (1.0, 0.0, 5.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 3, 4, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(scalexscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Scale Y:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (scaleyscale = gtk_adjustment_new (1.0, 0.0, 5.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 3, 4, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(scaleyscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Scale Z:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (scalezscale = gtk_adjustment_new (1.0, 0.0, 5.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 3, 4, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(scalezscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);



  label6 = gtk_label_new ("Rotate X:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 3, 4,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _rotscale = gtk_hscale_new (GTK_ADJUSTMENT (rotxscale = gtk_adjustment_new (1.0, 0.0, 360.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_rotscale", _rotscale);
  gtk_scale_set_digits (GTK_SCALE (_rotscale), 2);
  gtk_widget_show (_rotscale);
  gtk_table_attach (GTK_TABLE (table2), _rotscale, 3, 4, 3, 4,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(rotxscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Rotate Y:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 4, 5,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _rotscale = gtk_hscale_new (GTK_ADJUSTMENT (rotyscale = gtk_adjustment_new (1.0, 0.0, 360.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_rotscale", _rotscale);
  gtk_scale_set_digits (GTK_SCALE (_rotscale), 2);
  gtk_widget_show (_rotscale);
  gtk_table_attach (GTK_TABLE (table2), _rotscale, 3, 4, 4, 5,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(rotyscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Rotate Z:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 2, 3, 5, 6,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _rotscale = gtk_hscale_new (GTK_ADJUSTMENT (rotzscale = gtk_adjustment_new (1.0, 0.0, 360.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_rotscale", _rotscale);
  gtk_scale_set_digits (GTK_SCALE (_rotscale), 2);
  gtk_widget_show (_rotscale);
  gtk_table_attach (GTK_TABLE (table2), _rotscale, 3, 4, 5, 6,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(rotzscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);



  label6 = gtk_label_new ("Pos X:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 5, 6, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (posxscale = gtk_adjustment_new (0.0, -20.0, 20.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 6, 7, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(posxscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Pos Y:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 5, 6, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (posyscale = gtk_adjustment_new (1.0, -20.0, 20.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 6, 7, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(posyscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label6 = gtk_label_new ("Pos Z:");
  gtk_object_set_data (GTK_OBJECT (window), "label6", label6);
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table2), label6, 5, 6, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);

  _scalescale = gtk_hscale_new (GTK_ADJUSTMENT (poszscale = gtk_adjustment_new (1.0, -20.0, 20.1, 0.1, 0.1, 0.1)));
  gtk_object_set_data (GTK_OBJECT (window), "_scalescale", _scalescale);
  gtk_scale_set_digits (GTK_SCALE (_scalescale), 2);
  gtk_widget_show (_scalescale);
  gtk_table_attach (GTK_TABLE (table2), _scalescale, 6, 7, 2, 3,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  gtk_signal_connect(GTK_OBJECT(poszscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);






  typemenu = gtk_option_menu_new ();
  gtk_object_set_data (GTK_OBJECT (window), "typemenu", typemenu);
  gtk_widget_show (typemenu);
  gtk_table_attach (GTK_TABLE (table2), typemenu, 1, 2, 0, 1,
                    (GtkAttachOptions) GTK_EXPAND, (GtkAttachOptions) GTK_EXPAND, 0, 0);
  typemenu_menu = gtk_menu_new ();
  item = gtk_menu_item_new_with_label ("Texture");
  gtk_widget_show (item);
  gtk_signal_connect (GTK_OBJECT(item), "activate",
		      GTK_SIGNAL_FUNC(selecttype), NULL);
  gtk_menu_append (GTK_MENU (typemenu_menu), item);

  item = gtk_menu_item_new_with_label ("Bump");
  gtk_widget_show (item);
  gtk_signal_connect (GTK_OBJECT(item), "activate",
		      GTK_SIGNAL_FUNC(selecttype), (long *)1);
  gtk_menu_append (GTK_MENU (typemenu_menu), item);

  item = gtk_menu_item_new_with_label ("Light");
  gtk_widget_show (item);
  gtk_signal_connect (GTK_OBJECT(item), "activate",
		      GTK_SIGNAL_FUNC(selecttype), (long *)2);
  gtk_menu_append (GTK_MENU (typemenu_menu), item);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (typemenu), typemenu_menu);

  texturemenu = gtk_option_menu_new ();
  gtk_object_set_data (GTK_OBJECT (window), "texturemenu", texturemenu);
  gtk_widget_show (texturemenu);
  gtk_table_attach (GTK_TABLE (table2), texturemenu, 1, 2, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND, (GtkAttachOptions) GTK_EXPAND, 0, 0);

  texturemenu_menu = gtk_menu_new ();
  mktexturemenu(texturemenu_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (texturemenu), texturemenu_menu);

  label7 = gtk_label_new ("Amount:");
  gtk_object_set_data (GTK_OBJECT (window), "label7", label7);
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table2), label7, 0, 1, 5, 6,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);

  _amountscale = gtk_hscale_new (GTK_ADJUSTMENT (amountscale = gtk_adjustment_new (1.0, 0, 1.01, .01, .01, .01)));
  gtk_object_set_data (GTK_OBJECT (window), "_amountscale", _amountscale);
  gtk_widget_show (_amountscale);
  gtk_table_attach (GTK_TABLE (table2), _amountscale, 1, 2, 5, 6,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_digits (GTK_SCALE (_amountscale), 2);
  gtk_signal_connect(GTK_OBJECT(amountscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  label8 = gtk_label_new ("Exp:");
  gtk_object_set_data (GTK_OBJECT (window), "label8", label8);
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table2), label8, 0, 1, 6, 7,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);

  _expscale = gtk_hscale_new (GTK_ADJUSTMENT (expscale = gtk_adjustment_new (1.0, 0, 1.01, .01, .01, .01)));
  gtk_object_set_data (GTK_OBJECT (window), "_expscale", _expscale);
  gtk_widget_show (_expscale);
  gtk_table_attach (GTK_TABLE (table2), _expscale, 1, 2, 6, 7,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_scale_set_digits (GTK_SCALE (_expscale), 2);
  gtk_signal_connect(GTK_OBJECT(expscale), "value_changed",
                     (GtkSignalFunc)getscales, NULL);

  updatebutton = gtk_button_new_with_label ("Update");
  gtk_object_set_data (GTK_OBJECT (window), "updatebutton", updatebutton);
  gtk_widget_show (updatebutton);
  gtk_table_attach (GTK_TABLE (table1), updatebutton, 0, 1, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND | GTK_FILL, (GtkAttachOptions) GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT(updatebutton), "clicked", GTK_SIGNAL_FUNC(restartrender), NULL);

  label1 = gtk_label_new ("by Vidar Madsen\nSeptember 1999");
  gtk_object_set_data (GTK_OBJECT (window), "label1", label1);
  gtk_widget_show (label1);
  gtk_table_attach (GTK_TABLE (table1), label1, 2, 3, 1, 2,
                    (GtkAttachOptions) GTK_EXPAND, (GtkAttachOptions) GTK_EXPAND, 0, 0);

  gtk_widget_show(window);

  return window;
}

unsigned char pixelval(double v)
{
  v += 0.5;
  if(v < 0.0) return 0;
  if(v > 255.0) return 255;
  return v;
}

void render(void)
{
  int x, y, p;
  ray r;
  vector col;
  int hit;
  int tx, ty;
  guchar *dest_row;
  int bpp;

  r.v1.z = -10.0;
  r.v2.z = 0.0;

  running = 2;

  tx = PREVIEWSIZE;
  ty = PREVIEWSIZE;
  bpp = 3;

  while(running > 0) {

    if(running == 2) {
      running = 1;
      initworld();
    }
    if(world.obj[0].com.numtexture == 0) break;

    for(y = 0; y < ty; y++) {
      dest_row = img + y*PREVIEWSIZE*3;

      for(x = 0; x < tx; x++) {
	int g, gridsize = 16;
	g = ((x/gridsize+y/gridsize)%2)*60+100;

	r.v1.x = r.v2.x = 8.5 * (x / (float)(tx-1) - 0.5);
	r.v1.y = r.v2.y = 8.5 * (y / (float)(ty-1) - 0.5);

	p = x*bpp;

	hit = traceray(&r, &col, 10, 1.0);

	if(col.w < 0.0) col.w = 0.0;
	else if(col.w > 1.0) col.w = 1.0;

	dest_row[p+0] = pixelval(255 * col.x) * col.w + g * (1.0 - col.w);
	dest_row[p+1] = pixelval(255 * col.y) * col.w + g * (1.0 - col.w);
	dest_row[p+2] = pixelval(255 * col.z) * col.w + g * (1.0 - col.w);

	if(running != 1) {
	  break;
	}
      }
      drawit(img);
      while(gtk_events_pending())
	gtk_main_iteration();
      if(running != 1) {
	break;
      }
    }
    if(running == 1) break;
    if(running == -1) break;
  }
  running = 0;
  drawit(img);
}

void realrender(GDrawable *drawable)
{
  int x, y, alpha;
  ray r;
  vector rcol;
  int hit;
  int tx, ty;
  gint x1, y1, x2, y2;
  guchar *dest;
  int bpp;
  guchar tmpcol[4], bgcol[3];
  GPixelRgn pr;
  guchar *buffer;

  if(running > 0) return; /* Fixme: abort preview-render instead! */

  initworld();

  r.v1.z = -10.0;
  r.v2.z = 0.0;

  alpha = gimp_drawable_has_alpha(drawable->id);

  gimp_palette_get_background(bgcol,bgcol+1,bgcol+2);

  gimp_pixel_rgn_init(&pr, drawable, 0, 0,
		      gimp_drawable_width(drawable->id),
		      gimp_drawable_height(drawable->id), TRUE, TRUE);
  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  bpp = gimp_drawable_bpp(drawable->id);
  buffer = g_malloc((x2 - x1) * bpp);

  tx = x2 - x1;
  ty = y2 - y1;

  gimp_progress_init ("Rendering...");

  for(y = 0; y < ty; y++) {
    dest = buffer;
    for(x = 0; x < tx; x++) {
      r.v1.x = r.v2.x = 8.5 * (x / (float)(tx-1) - 0.5);
      r.v1.y = r.v2.y = 8.5 * (y / (float)(ty-1) - 0.5);

      hit = traceray(&r, &rcol, 10, 1.0);
      if(!hit && !alpha) {
	memcpy(tmpcol, bgcol, sizeof(bgcol));
      } else {
	tmpcol[0] = pixelval(255 * rcol.x);
	tmpcol[1] = pixelval(255 * rcol.y);
	tmpcol[2] = pixelval(255 * rcol.z);
	tmpcol[3] = pixelval(255 * rcol.w);
      }
      memcpy(dest, tmpcol, bpp);
      dest += bpp;
    }
    gimp_pixel_rgn_set_row(&pr, buffer, x1, y1 + y, x2 - x1);
    gimp_progress_update((double)y / (double)ty);
  }
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

static void query (void)
{
  static GParamDef args[] = {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;
  gimp_install_procedure ("plug_in_spheredesigner",
                          "Renders textures spheres",
                          "This plugin can be used to create textured and/or bumpmapped spheres, and uses a small lightweight raytracer to perform the task with good quality",
			  "Vidar Madsen",
                          "Vidar Madsen",
                          "1999",
                          "<Image>/Filters/Render/SphereDesigner",
                          "RGB*, GRAY*",
                          PROC_PLUG_IN,
			  nargs, nreturn_vals,
                          args, return_vals);
}


int sphere_main(GDrawable *drawable)
{
  gchar **argv;
  gint argc = 1;
  initworld();

  argc = 1;
  argv = g_new(char *,1);
  argv[0] = g_strdup("spheredesigner");
  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  memset(img, 0, PREVIEWSIZE*PREVIEWSIZE*3);
  makewindow();

  if(!s.com.numtexture)
    sphere_reset();
  else
    rebuildlist();

  drawit(img);
  gtk_main();
  gdk_flush();
  return do_run;
}

static void run (gchar *name, gint nparams, GParam *param, gint *nreturn_vals, GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch(run_mode) {
  case RUN_INTERACTIVE:
    s.com.numtexture = 0;
    gimp_get_data(PLUG_IN_NAME, &s);
    if(!sphere_main(drawable)) {
      gimp_drawable_detach(drawable);
      return;
    }
    break;
  case RUN_WITH_LAST_VALS:
    s.com.numtexture = 0;
    gimp_get_data(PLUG_IN_NAME, &s);
    if(s.com.numtexture == 0) {
      gimp_drawable_detach(drawable);
      return;
    }
    break;
  case RUN_NONINTERACTIVE:
  default:
    /* Not implementet yet... */
    gimp_drawable_detach(drawable);
    return;
  }

  gimp_set_data(PLUG_IN_NAME, &s, sizeof(s));

  realrender(drawable);
  gimp_displays_flush ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  
  gimp_drawable_detach (drawable);
}

MAIN()
