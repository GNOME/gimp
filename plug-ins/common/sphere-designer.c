/*
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * SphereDesigner v0.4 - creates textured spheres
 * by Vidar Madsen <vidar@prosalg.no>
 *
 * Status: Last updated 1999-09-11
 *
 * Known issues:
 * - Might crash if you click OK or Cancel before first preview is rendered
 * - Phong might look weird with transparent textures
 *
 * Todo:
 * - Saving / Loading of presets needs an overhaul
 * - Antialiasing
 * - Global controls: Gamma, ++
 * - Beautification of GUI
 * - Clean up messy source (lots of Glade remnants)
 * - (Probably more. ;-)
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-spheredesigner"
#define PLUG_IN_BINARY "sphere-designer"
#define PLUG_IN_ROLE   "gimp-sphere-designer"

#define RESPONSE_RESET 1

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

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

enum
{
  TRIANGLE,
  DISC,
  PLANE,
  SPHERE,
  CYLINDER,
  LIGHT
};

enum
{
  SOLID,
  CHECKER,
  MARBLE,
  LIZARD,
  IMAGE,
  PHONG,
  REFLECTION,
  REFRACTION,
  PERLIN,
  WOOD,
  TRANSPARENT,
  SPIRAL,
  SPOTS,
  SMOKE
};

enum
{
  PERSPECTIVE,
  ORTHOGONAL,
  FISHEYE
};

enum
{
  FOG
};

enum
{
  TYPE,
  TEXTURE,
  NUM_COLUMNS
};


/* World-flags */
#define SMARTAMBIENT 0x00000001

/* Object-flags */
#define NOSHADOW   0x00000001

/* Texture-flags */
#define GRADIENT   0x00000001

typedef struct
{
  gshort  xsize, ysize;
  guchar *rgb;
} image;

typedef struct
{
  gshort        numcol;
  gdouble       pos[MAXCOLPERGRADIENT];
  GimpVector4   color[MAXCOLPERGRADIENT];
} gradient;

typedef struct
{
  gint          majtype;
  gint          type;
  gulong        flags;
  GimpVector4   color1, color2;
  gradient      gradient;
  GimpVector4   ambient, diffuse;
  gdouble       oscale;
  GimpVector4   scale, translate, rotate;
  image         image;
  GimpVector4   reflection;
  GimpVector4   refraction;
  GimpVector4   transparent;
  gdouble       ior;
  GimpVector4   phongcolor;
  gdouble       phongsize;
  gdouble       amount;
  gdouble       exp;
  GimpVector4   turbulence;
} texture;

typedef struct
{
  gshort  type;
  gdouble density;
  GimpVector4  color;
  gdouble turbulence;
} atmos;

typedef struct
{
  gshort  type;
  gulong  flags;
  gshort  numtexture;
  texture texture[MAXTEXTUREPEROBJ];
  gshort  numnormal;
  texture normal[MAXNORMALPEROBJ];
} common;

typedef struct
{
  common com;
  GimpVector4 a, b, c;
} triangle;

typedef struct
{
  common        com;
  GimpVector4   a;
  gdouble       b, r;
} disc;

typedef struct
{
  common        com;
  GimpVector4   a;
  gdouble       r;
} sphere;

typedef struct
{
  common        com;
  GimpVector4   a, b, c;
} cylinder;

typedef struct
{
  common        com;
  GimpVector4   a;
  gdouble       b;
} plane;

typedef struct
{
  common        com;
  GimpVector4   color;
  GimpVector4   a;
} light;

typedef struct
{
  GimpVector4   v1, v2;
  gshort        inside;
  gdouble       ior;
} ray;

typedef union
{
  common   com;
  triangle tri;
  disc     disc;
  plane    plane;
  sphere   sphere;
  cylinder cylinder;
} object;


struct world_t
{
  gint    numobj;
  object  obj[MAXOBJECT];
  gint    numlight;
  light   light[MAXLIGHT];
  gint    numtexture;
  texture texture[MAXTEXTURE];
  gulong  flags;
  gshort  quality;
  gdouble smartambient;
  gshort  numatmos;
  atmos   atmos[MAXATMOS];
};

struct camera_t
{
  GimpVector4 location, lookat, up, right;
  short  type;
  double fov, tilt;
};

static GtkWidget *drawarea = NULL;

static guchar          *img;
static gint             img_stride;
static cairo_surface_t *buffer;

static guint  idle_id = 0;

static sphere s;

struct textures_t
{
  gint   index;
  gchar *s;
  glong  n;
};

static struct textures_t textures[] =
{
  { 0, N_("Solid"),   SOLID   },
  { 1, N_("Checker"), CHECKER },
  { 2, N_("Marble"),  MARBLE  },
  { 3, N_("Lizard"),  LIZARD  },
  { 4, N_("Phong"),   PHONG   },
  { 5, N_("Noise"),   PERLIN  },
  { 6, N_("Wood"),    WOOD    },
  { 7, N_("Spiral"),  SPIRAL  },
  { 8, N_("Spots"),   SPOTS   },
  { 0, NULL,          0       }
};

static inline void vset        (GimpVector4          *v,
                                gdouble               a,
                                gdouble               b,
                                gdouble               c);
static void      restartrender (void);
static void      drawcolor1    (GtkWidget            *widget);
static void      drawcolor2    (GtkWidget            *widget);
static gboolean  render        (void);
static void      realrender    (GimpDrawable         *drawable);
static void      fileselect    (GtkFileChooserAction  action,
                                GtkWidget            *parent);
static gint      traceray      (ray                  *r,
                                GimpVector4          *col,
                                gint                  level,
                                gdouble               imp);
static gdouble   turbulence    (gdouble              *point,
                                gdouble               lofreq,
                                gdouble               hifreq);


#define COLORBUTTONWIDTH  30
#define COLORBUTTONHEIGHT 20

static GtkTreeView *texturelist = NULL;

static GtkAdjustment *scalexscale, *scaleyscale, *scalezscale;
static GtkAdjustment *rotxscale, *rotyscale, *rotzscale;
static GtkAdjustment *posxscale, *posyscale, *poszscale;
static GtkAdjustment *scalescale;
static GtkAdjustment *turbulencescale;
static GtkAdjustment *amountscale;
static GtkAdjustment *expscale;
static GtkWidget     *typemenu;
static GtkWidget     *texturemenu;

#define DOT(a,b) (a[0] * b[0] + a[1] * b[1] + a[2] * b[2])

#define B 256

static gint      p[B + B + 2];
static gdouble   g[B + B + 2][3];
static gboolean  start = TRUE;
static GRand    *gr;


static void
init (void)
{
  gint i, j, k;
  gdouble v[3], s;

  /* Create an array of random gradient vectors uniformly on the unit sphere */

  gr = g_rand_new ();
  g_rand_set_seed (gr, 1);    /* Use static seed, to get reproducible results */

  for (i = 0; i < B; i++)
    {
      do
        {                     /* Choose uniformly in a cube */
          for (j = 0; j < 3; j++)
            v[j] = g_rand_double_range (gr, -1, 1);
          s = DOT (v, v);
        }
      while (s > 1.0);        /* If not in sphere try again */
      s = sqrt (s);
      for (j = 0; j < 3; j++) /* Else normalize */
        g[i][j] = v[j] / s;
    }

/* Create a pseudorandom permutation of [1..B] */

  for (i = 0; i < B; i++)
    p[i] = i;
  for (i = B; i > 0; i -= 2)
    {
      k = p[i];
      p[i] = p[j = g_rand_int_range (gr, 0, B)];
      p[j] = k;
    }

  /* Extend g and p arrays to allow for faster indexing */

  for (i = 0; i < B + 2; i++)
    {
      p[B + i] = p[i];
      for (j = 0; j < 3; j++)
        g[B + i][j] = g[i][j];
    }
  g_rand_free (gr);
}

#define setup(i,b0,b1,r0,r1) \
        t = vec[i] + 10000.; \
        b0 = ((int)t) & (B-1); \
        b1 = (b0+1) & (B-1); \
        r0 = t - (int)t; \
        r1 = r0 - 1.;


static gdouble
noise3 (gdouble * vec)
{
  gint    bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
  gdouble rx0, rx1, ry0, ry1, rz0, rz1, *q, sx, sy, sz, a, b, c, d, t, u, v;
  gint    i, j;

  if (start)
    {
      start = FALSE;
      init ();
    }

  setup (0, bx0, bx1, rx0, rx1);
  setup (1, by0, by1, ry0, ry1);
  setup (2, bz0, bz1, rz0, rz1);

  i = p[bx0];
  j = p[bx1];

  b00 = p[i + by0];
  b10 = p[j + by0];
  b01 = p[i + by1];
  b11 = p[j + by1];

#define at(rx,ry,rz) ( rx * q[0] + ry * q[1] + rz * q[2] )

#define surve(t) ( t * t * (3. - 2. * t) )

#define lerp(t, a, b) ( a + t * (b - a) )

  sx = surve (rx0);
  sy = surve (ry0);
  sz = surve (rz0);


  q = g[b00 + bz0];
  u = at (rx0, ry0, rz0);
  q = g[b10 + bz0];
  v = at (rx1, ry0, rz0);
  a = lerp (sx, u, v);

  q = g[b01 + bz0];
  u = at (rx0, ry1, rz0);
  q = g[b11 + bz0];
  v = at (rx1, ry1, rz0);
  b = lerp (sx, u, v);

  c = lerp (sy, a, b);          /* interpolate in y at lo x */

  q = g[b00 + bz1];
  u = at (rx0, ry0, rz1);
  q = g[b10 + bz1];
  v = at (rx1, ry0, rz1);
  a = lerp (sx, u, v);

  q = g[b01 + bz1];
  u = at (rx0, ry1, rz1);
  q = g[b11 + bz1];
  v = at (rx1, ry1, rz1);
  b = lerp (sx, u, v);

  d = lerp (sy, a, b);          /* interpolate in y at hi x */

  return 1.5 * lerp (sz, c, d); /* interpolate in z */
}

static double
turbulence (gdouble * point, gdouble lofreq, gdouble hifreq)
{
  gdouble freq, t, p[3];

  p[0] = point[0] + 123.456;
  p[1] = point[1] + 234.567;
  p[2] = point[2] + 345.678;

  t = 0;
  for (freq = lofreq; freq < hifreq; freq *= 2.)
    {
      t += noise3 (p) / freq;
      p[0] *= 2.;
      p[1] *= 2.;
      p[2] *= 2.;
    }
  return t - 0.3;               /* readjust to make mean value = 0.0 */
}

static struct world_t  world;

static inline void
vcopy (GimpVector4 *a, GimpVector4 *b)
{
  *a = *b;
}

static inline void
vcross (GimpVector4 *r, GimpVector4 *a, GimpVector4 *b)
{
  r->x = a->y * b->z - a->z * b->y;
  r->y = -(a->x * b->z - a->z * b->x);
  r->z = a->x * b->y - a->y * b->x;
}

static inline gdouble
vdot (GimpVector4 *a, GimpVector4 *b)
{
  return a->x * b->x + a->y * b->y + a->z * b->z;
}

static inline gdouble
vdist (GimpVector4 *a, GimpVector4 *b)
{
  gdouble x, y, z;

  x = a->x - b->x;
  y = a->y - b->y;
  z = a->z - b->z;

  return sqrt (x * x + y * y + z * z);
}

static inline gdouble
vdist2 (GimpVector4 *a, GimpVector4 *b)
{
  gdouble x, y, z;

  x = a->x - b->x;
  y = a->y - b->y;
  z = a->z - b->z;

  return x * x + y * y + z * z;
}

static inline gdouble
vlen (GimpVector4 *a)
{
  return sqrt (a->x * a->x + a->y * a->y + a->z * a->z);
}

static inline void
vnorm (GimpVector4 *a, gdouble v)
{
  gdouble d;

  d = vlen (a);
  a->x *= v / d;
  a->y *= v / d;
  a->z *= v / d;
}

static inline void
vrotate (GimpVector4 *axis, gdouble ang, GimpVector4 *vector)
{
  gdouble rad = ang / 180.0 * G_PI;
  gdouble ax  = vector->x;
  gdouble ay  = vector->y;
  gdouble az  = vector->z;
  gdouble x   = axis->x;
  gdouble y   = axis->y;
  gdouble z   = axis->z;
  gdouble c   = cos (rad);
  gdouble s   = sin (rad);
  gdouble c1  = 1.0 - c;
  gdouble xx  = c1 * x * x;
  gdouble yy  = c1 * y * y;
  gdouble zz  = c1 * z * z;
  gdouble xy  = c1 * x * y;
  gdouble xz  = c1 * x * z;
  gdouble yz  = c1 * y * z;
  gdouble sx  = s * x;
  gdouble sy  = s * y;
  gdouble sz  = s * z;

  vector->x = (xx + c) * ax + (xy + sz) * ay + (xz - sy) * az;
  vector->y = (xy - sz) * ax + (yy + c) * ay + (yz + sx) * az;
  vector->z = (xz + sy) * ax + (yz - sx) * ay + (zz + c) * az;
}

static inline void
vset (GimpVector4 *v, gdouble a, gdouble b, gdouble c)
{
  v->x = a;
  v->y = b;
  v->z = c;
  v->w = 1.0;
}

static inline void
vcset (GimpVector4 *v, gdouble a, gdouble b, gdouble c, gdouble d)
{
  v->x = a;
  v->y = b;
  v->z = c;
  v->w = d;
}

static inline void
vvrotate (GimpVector4 *p, GimpVector4 *rot)
{
  GimpVector4 axis;

  if (rot->x != 0.0)
    {
      vset (&axis, 1, 0, 0);
      vrotate (&axis, rot->x, p);
    }
  if (rot->y != 0.0)
    {
      vset (&axis, 0, 1, 0);
      vrotate (&axis, rot->y, p);
    }
  if (rot->z != 0.0)
    {
      vset (&axis, 0, 0, 1);
      vrotate (&axis, rot->z, p);
    }
}

static inline void
vsub (GimpVector4 *a, GimpVector4 *b)
{
  a->x -= b->x;
  a->y -= b->y;
  a->z -= b->z;
  a->w -= b->w;
}

static inline void
vadd (GimpVector4 *a, GimpVector4 *b)
{
  a->x += b->x;
  a->y += b->y;
  a->z += b->z;
  a->w += b->w;
}

static inline void
vneg (GimpVector4 *a)
{
  a->x = -a->x;
  a->y = -a->y;
  a->z = -a->z;
  a->w = -a->w;
}

static inline void
vmul (GimpVector4 *v, gdouble a)
{
  v->x *= a;
  v->y *= a;
  v->z *= a;
  v->w *= a;
}

static inline void
vvmul (GimpVector4 *a, GimpVector4 *b)
{
  a->x *= b->x;
  a->y *= b->y;
  a->z *= b->z;
  a->w *= b->w;
}

static inline void
vvdiv (GimpVector4 *a, GimpVector4 *b)
{
  a->x /= b->x;
  a->y /= b->y;
  a->z /= b->z;
}

static void
vmix (GimpVector4 *r, GimpVector4 *a, GimpVector4 *b, gdouble v)
{
  gdouble i = 1.0 - v;

  r->x = a->x * v + b->x * i;
  r->y = a->y * v + b->y * i;
  r->z = a->z * v + b->z * i;
  r->w = a->w * v + b->w * i;
}

static double
vmax (GimpVector4 *a)
{
  gdouble max = fabs (a->x);

  if (fabs (a->y) > max)
    max = fabs (a->y);
  if (fabs (a->z) > max)
    max = fabs (a->z);
  if (fabs (a->w) > max)
    max = fabs (a->w);

  return max;
}

#if 0
static void
vavg (GimpVector4 * a)
{
  gdouble s;

  s = (a->x + a->y + a->z) / 3.0;
  a->x = a->y = a->z = s;
}
#endif

static void
trianglenormal (GimpVector4 * n, gdouble *t, triangle * tri)
{
  triangle tmp;
  vcopy (&tmp.b, &tri->b);
  vcopy (&tmp.c, &tri->c);
  vsub (&tmp.b, &tri->a);
  vsub (&tmp.c, &tri->a);
  vset (&tmp.a, 0, 0, 0);
  vcross (n, &tmp.b, &tmp.c);
  if (t)
    *t = vdot (&tmp.b, &tmp.c);
}

static gdouble
checkdisc (ray * r, disc * disc)
{
  GimpVector4 p, *v = &disc->a;
  gdouble t, d2;
  gdouble i, j, k;

  i = r->v2.x - r->v1.x;
  j = r->v2.y - r->v1.y;
  k = r->v2.z - r->v1.z;

  t = -(v->x * r->v1.x + v->y * r->v1.y + v->z * r->v1.z - disc->b) /
    (v->x * i + v->y * j + v->z * k);

  p.x = r->v1.x + i * t;
  p.y = r->v1.y + j * t;
  p.z = r->v1.z + k * t;

  d2 = vdist2 (&p, v);

  if (d2 > disc->r * disc->r)
    t = 0.0;

  return t;
}

static gdouble
checksphere (ray * r, sphere * sphere)
{
  GimpVector4 cendir, rdir;
  gdouble dirproj, cdlensq;
  gdouble linear, constant, rsq, quadratic, discriminant;
  gdouble smallzero, solmin, solmax, tolerance = 0.001;

  vcopy (&rdir, &r->v2);
  vsub (&rdir, &r->v1);

  rsq = sphere->r * sphere->r;

  vcopy (&cendir, &r->v1);
  vsub (&cendir, &sphere->a);
  dirproj = vdot (&rdir, &cendir);
  cdlensq = vdot (&cendir, &cendir);

  if ((cdlensq >= rsq) && (dirproj > 0.0))
    return 0.0;

  linear = 2.0 * dirproj;
  constant = cdlensq - rsq;
  quadratic = vdot (&rdir, &rdir);

  smallzero = (constant / linear);
  if ((smallzero < tolerance) && (smallzero > -tolerance))
    {
      solmin = -linear / quadratic;

      if (solmin > tolerance)
        {
          return solmin;
          /*
           *hits = solmin;
           return 1;
           */
        }
      else
        return 0.0;
    }
  discriminant = linear * linear - 4.0 * quadratic * constant;
  if (discriminant < 0.0)
    return 0.0;
  quadratic *= 2.0;
  discriminant = sqrt (discriminant);
  solmax = (-linear + discriminant) / (quadratic);
  solmin = (-linear - discriminant) / (quadratic);

  if (solmax < tolerance)
    return 0.0;

  if (solmin < tolerance)
    {
      return solmax;
      /*
       * hits = solmax;
       * return 1;
       */
    }
  else
    {
      return solmin;
      /*
       * hits++ = solmin;
       * hits = solmax;
       * return 2;
       */
    }
}

static gdouble
checkcylinder (ray * r, cylinder * cylinder)
{
  /* FIXME */
  return 0.0;
}


static gdouble
checkplane (ray * r, plane * plane)
{
  GimpVector4 *v = &plane->a;
  gdouble t;
  gdouble i, j, k;

  i = r->v2.x - r->v1.x;
  j = r->v2.y - r->v1.y;
  k = r->v2.z - r->v1.z;

  t = -(v->x * r->v1.x + v->y * r->v1.y + v->z * r->v1.z - plane->b) /
    (v->x * i + v->y * j + v->z * k);

  return t;
}

static gdouble
checktri (ray * r, triangle * tri)
{
  GimpVector4  ed1, ed2;
  GimpVector4  tvec, pvec, qvec;
  gdouble det, idet, t, u, v;
  GimpVector4 *orig, dir;

  orig = &r->v1;
  dir = r->v2;
  vsub (&dir, orig);

  ed1.x = tri->c.x - tri->a.x;
  ed1.y = tri->c.y - tri->a.y;
  ed1.z = tri->c.z - tri->a.z;
  ed2.x = tri->b.x - tri->a.x;
  ed2.y = tri->b.y - tri->a.y;
  ed2.z = tri->b.z - tri->a.z;
  vcross (&pvec, &dir, &ed2);
  det = vdot (&ed1, &pvec);

  idet = 1.0 / det;

  tvec.x = orig->x;
  tvec.y = orig->y;
  tvec.z = orig->z;
  vsub (&tvec, &tri->a);
  u = vdot (&tvec, &pvec) * idet;

  if (u < 0.0)
    return 0;
  if (u > 1.0)
    return 0;

  vcross (&qvec, &tvec, &ed1);
  v = vdot (&dir, &qvec) * idet;

  if ((v < 0.0) || (u + v > 1.0))
    return 0;

  t = vdot (&ed2, &qvec) * idet;

  return t;
}

static void
transformpoint (GimpVector4 * p, texture * t)
{
  gdouble point[3], f;

  if ((t->rotate.x != 0.0) || (t->rotate.y != 0.0) || (t->rotate.z != 0.0))
    vvrotate (p, &t->rotate);
  vvdiv (p, &t->scale);

  vsub (p, &t->translate);

  if ((t->turbulence.x != 0.0) || (t->turbulence.y != 0.0) ||
      (t->turbulence.z != 0.0))
    {
      point[0] = p->x;
      point[1] = p->y;
      point[2] = p->z;
      f = turbulence (point, 1, 256);
      p->x += t->turbulence.x * f;
      p->y += t->turbulence.y * f;
      p->z += t->turbulence.z * f;
    }
}

static void
checker (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gint   c = 0;
  GimpVector4 p;

  p = *q;
  transformpoint (&p, t);

  vmul (&p, 0.25);

  p.x += 0.00001;
  p.y += 0.00001;
  p.z += 0.00001;

  if (p.x < 0.0)
    p.x = 0.5 - p.x;
  if (p.y < 0.0)
    p.y = 0.5 - p.y;
  if (p.z < 0.0)
    p.z = 0.5 - p.z;

  if ((p.x - (gint) p.x) < 0.5)
    c ^= 1;
  if ((p.y - (gint) p.y) < 0.5)
    c ^= 1;
  if ((p.z - (gint) p.z) < 0.5)
    c ^= 1;

  *col = (c) ? t->color1 : t->color2;
}

static void
gradcolor (GimpVector4 *col, gradient *t, gdouble val)
{
  gint    i;
  gdouble d;
  GimpVector4  tmpcol;

  val = CLAMP (val, 0.0, 1.0);

  for (i = 0; i < t->numcol; i++)
    {
      if (t->pos[i] == val)
        {
          *col = t->color[i];
          return;
        }
      if (t->pos[i] > val)
        {
          d = (val - t->pos[i - 1]) / (t->pos[i] - t->pos[i - 1]);
          vcopy (&tmpcol, &t->color[i]);
          vmul (&tmpcol, d);
          vcopy (col, &tmpcol);
          vcopy (&tmpcol, &t->color[i - 1]);
          vmul (&tmpcol, 1.0 - d);
          vadd (col, &tmpcol);
          return;
        }
    }
  g_printerr ("Error in gradient!\n");
  vset (col, 0, 1, 0);
}

static void
marble (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gdouble f;
  GimpVector4 p;

  p = *q;
  transformpoint (&p, t);

  f = sin (p.x * 4) / 2 + 0.5;
  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
lizard (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gdouble f;
  GimpVector4 p;

  p = *q;
  transformpoint (&p, t);

  f = fabs (sin (p.x * 4));
  f += fabs (sin (p.y * 4));
  f += fabs (sin (p.z * 4));
  f /= 3.0;
  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
wood (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gdouble f;
  GimpVector4 p;

  p = *q;
  transformpoint (&p, t);

  f = fabs (p.x);
  f = f - (int) f;

  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
spiral (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gdouble f;
  GimpVector4 p;

  p = *q;
  transformpoint (&p, t);

  f = fabs (atan2 (p.x, p.z) / G_PI / 2 + p.y + 99999);
  f = f - (int) f;

  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
spots (GimpVector4 *q, GimpVector4 *col, texture *t)
{
  gdouble f;
  GimpVector4 p, r;

  p = *q;
  transformpoint (&p, t);

  p.x += 10000.0;
  p.y += 10000.0;
  p.z += 10000.0;

  vset (&r, (gint) (p.x + 0.5), (gint) (p.y + 0.5), (gint) (p.z + 0.5));
  f = vdist (&p, &r);
  f = cos (f * G_PI);
  f = CLAMP (f, 0.0, 1.0);
  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
perlin (GimpVector4 * q, GimpVector4 * col, texture * t)
{
  gdouble f, point[3];
  GimpVector4  p;

  p = *q;
  transformpoint (&p, t);

  point[0] = p.x;
  point[1] = p.y;
  point[2] = p.z;

  f = turbulence (point, 1, 256) * 0.3 + 0.5;

  f = pow (f, t->exp);

  if (t->flags & GRADIENT)
    gradcolor (col, &t->gradient, f);
  else
    vmix (col, &t->color1, &t->color2, f);
}

static void
imagepixel (GimpVector4 * q, GimpVector4 * col, texture * t)
{
  GimpVector4 p;
  gint x, y;
  guchar *rgb;

  p = *q;
  transformpoint (&p, t);

  x = (p.x * t->image.xsize);
  y = (p.y * t->image.ysize);

  x = (x % t->image.xsize + t->image.xsize) % t->image.xsize;
  y = (y % t->image.ysize + t->image.ysize) % t->image.ysize;

  rgb = &t->image.rgb[x * 3 + (t->image.ysize - 1 - y) * t->image.xsize * 3];
  vset (col, rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0);
}

static void
objcolor (GimpVector4 *col, GimpVector4 *p, common *obj)
{
  gint     i;
  texture *t;
  GimpVector4   tmpcol;

  vcset (col, 0, 0, 0, 0);

  for (i = 0; i < obj->numtexture; i++)
    {
      t = &obj->texture[i];

      if (world.quality < 1)
        {
          vadd (col, &t->color1);
          continue;
        }

      vset (&tmpcol, 0, 0, 0);
      switch (t->type)
        {
        case SOLID:
          vcopy (&tmpcol, &t->color1);
          break;
        case CHECKER:
          checker (p, &tmpcol, t);
          break;
        case MARBLE:
          marble (p, &tmpcol, t);
          break;
        case LIZARD:
          lizard (p, &tmpcol, t);
          break;
        case PERLIN:
          perlin (p, &tmpcol, t);
          break;
        case WOOD:
          wood (p, &tmpcol, t);
          break;
        case SPIRAL:
          spiral (p, &tmpcol, t);
          break;
        case SPOTS:
          spots (p, &tmpcol, t);
          break;
        case IMAGE:
          imagepixel (p, &tmpcol, t);
          break;
        case PHONG:
        case REFRACTION:
        case REFLECTION:
        case TRANSPARENT:
        case SMOKE:
          /* Silently ignore non-color textures */
          continue;
          break;
        default:
          g_printerr ("Warning: unknown texture %d\n", t->type);
          break;
        }
      vmul (&tmpcol, t->amount);
      vadd (col, &tmpcol);
    }
  if (!i)
    {
      g_printerr ("Warning: object %p has no textures\n", obj);
    }
}

static void
objnormal (GimpVector4 *res, common *obj, GimpVector4 *p)
{
  gint i;

  switch (obj->type)
    {
    case TRIANGLE:
      trianglenormal (res, NULL, (triangle *) obj);
      break;
    case DISC:
      vcopy (res, &((disc *) obj)->a);
      break;
    case PLANE:
      vcopy (res, &((plane *) obj)->a);
      break;
    case SPHERE:
      vcopy (res, &((sphere *) obj)->a);
      vsub (res, p);
      break;
    case CYLINDER:
      vset (res, 1, 1, 1);      /* fixme */
      break;
    default:
      g_error ("objnormal(): Unsupported object type!?\n");
    }
  vnorm (res, 1.0);

  for (i = 0; i < obj->numnormal; i++)
    {
      gint     k;
      GimpVector4   tmpcol[6];
      GimpVector4   q[6], nres;
      texture *t = &obj->normal[i];
      gdouble  nstep = 0.1;

      vset (&nres, 0, 0, 0);
      for (k = 0; k < 6; k++)
        {
          vcopy (&q[k], p);
        }
      q[0].x += nstep;
      q[1].x -= nstep;
      q[2].y += nstep;
      q[3].y -= nstep;
      q[4].z += nstep;
      q[5].z -= nstep;

      switch (t->type)
        {
        case MARBLE:
          for (k = 0; k < 6; k++)
            marble (&q[k], &tmpcol[k], t);
          break;
        case LIZARD:
          for (k = 0; k < 6; k++)
            lizard (&q[k], &tmpcol[k], t);
          break;
        case PERLIN:
          for (k = 0; k < 6; k++)
            perlin (&q[k], &tmpcol[k], t);
          break;
        case WOOD:
          for (k = 0; k < 6; k++)
            wood (&q[k], &tmpcol[k], t);
          break;
        case SPIRAL:
          for (k = 0; k < 6; k++)
            spiral (&q[k], &tmpcol[k], t);
          break;
        case SPOTS:
          for (k = 0; k < 6; k++)
            spots (&q[k], &tmpcol[k], t);
          break;
        case IMAGE:
          for (k = 0; k < 6; k++)
            imagepixel (&q[k], &tmpcol[k], t);
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
          g_printerr ("Warning: unknown texture %d\n", t->type);
          break;
        }

      nres.x = tmpcol[0].x - tmpcol[1].x;
      nres.y = tmpcol[2].x - tmpcol[3].x;
      nres.z = tmpcol[4].x - tmpcol[5].x;
      vadd (&nres, res);
      vnorm (&nres, 1.0);
      vmul (&nres, t->amount);
      vadd (res, &nres);
      vnorm (res, 1.0);
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

static void
calclight (GimpVector4 * col, GimpVector4 * point, common * obj)
{
  gint i, j;
  ray r;
  gdouble b, a;
  GimpVector4 lcol;
  GimpVector4 norm;
  GimpVector4 pcol;

  vcset (col, 0, 0, 0, 0);

  objcolor (&pcol, point, obj);
  a = pcol.w;

  if (world.quality < 2)
    {
      vcopy (col, &pcol);
      return;
    }

  for (i = 0; i < obj->numtexture; i++)
    {
      if (obj->texture[i].type == PHONG)
        continue;
      if (obj->texture[i].type == REFLECTION)
        continue;
      if (obj->texture[i].type == REFRACTION)
        continue;
      if (obj->texture[i].type == TRANSPARENT)
        continue;
      if (obj->texture[i].type == SMOKE)
        continue;
      vcopy (&lcol, &pcol);
      vvmul (&lcol, &obj->texture[i].ambient);
      vadd (col, &lcol);
    }

  objnormal (&norm, obj, point);
  vnorm (&norm, 1.0);

  r.inside = -1;
  r.ior = 1.0;

  for (i = 0; i < world.numlight; i++)
    {
      vcopy (&r.v1, point);
      vcopy (&r.v2, &world.light[i].a);
      vmix (&r.v1, &r.v1, &r.v2, 0.9999);

      vsub (&r.v1, &r.v2);
      vnorm (&r.v1, 1.0);
      b = vdot (&r.v1, &norm);

      if (b < 0.0)
        continue;

      for (j = 0; j < obj->numtexture; j++)
        {
          if (obj->texture[j].type == PHONG)
            continue;
          if (obj->texture[j].type == REFLECTION)
            continue;
          if (obj->texture[j].type == REFRACTION)
            continue;
          if (obj->texture[j].type == TRANSPARENT)
            continue;
          if (obj->texture[j].type == SMOKE)
            continue;
          vcopy (&lcol, &pcol);
          vvmul (&lcol, &world.light[i].color);
          vvmul (&lcol, &obj->texture[j].diffuse);
          vmul (&lcol, b);
          vadd (col, &lcol);
        }
    }
  col->w = a;
}

static void
calcphong (common * obj, ray * r2, GimpVector4 * col)
{
  gint    i, j;
  ray     r;
  gdouble b;
  GimpVector4  lcol;
  GimpVector4  norm;
  GimpVector4  pcol;
  gdouble ps;

  vcopy (&pcol, col);

  vcopy (&norm, &r2->v2);
  vsub (&norm, &r2->v1);
  vnorm (&norm, 1.0);

  r.inside = -1;
  r.ior = 1.0;

  for (i = 0; i < world.numlight; i++)
    {
      vcopy (&r.v1, &r2->v1);
      vcopy (&r.v2, &world.light[i].a);
      vmix (&r.v1, &r.v1, &r.v2, 0.9999);

      if (traceray (&r, NULL, -1, 1.0))
        continue;

      /* OK, light is visible */

      vsub (&r.v1, &r.v2);
      vnorm (&r.v1, 1.0);
      b = -vdot (&r.v1, &norm);

      for (j = 0; j < obj->numtexture; j++)
        {
          if (obj->texture[j].type != PHONG)
            continue;

          ps = obj->texture[j].phongsize;

          if (b < (1 - ps))
            continue;
          ps = (b - (1 - ps)) / ps;

          vcopy (&lcol, &obj->texture[j].phongcolor);
          vvmul (&lcol, &world.light[i].color);
          vmul (&lcol, ps);
          vadd (col, &lcol);
        }
    }
}

static int
traceray (ray * r, GimpVector4 * col, gint level, gdouble imp)
{
  gint     i, b = -1;
  gdouble  t = -1.0, min = 0.0;
  common  *obj, *bobj = NULL;
  gint     hits = 0;
  GimpVector4   p;

  if ((level == 0) || (imp < 0.005))
    {
      vset (col, 0, 1, 0);
      return 0;
    }

  for (i = 0; i < world.numobj; i++)
    {
      obj = (common *) & world.obj[i];
      switch (obj->type)
        {
        case TRIANGLE:
          t = checktri (r, (triangle *) & world.obj[i]);
          break;
        case DISC:
          t = checkdisc (r, (disc *) & world.obj[i]);
          break;
        case PLANE:
          t = checkplane (r, (plane *) & world.obj[i]);
          break;
        case SPHERE:
          t = checksphere (r, (sphere *) & world.obj[i]);
          break;
        case CYLINDER:
          t = checkcylinder (r, (cylinder *) & world.obj[i]);
          break;
        default:
          g_error ("Illegal object!!\n");
        }
      if (t <= 0.0)
        continue;

      if (!(obj->flags & NOSHADOW) && (level == -1))
        {
          return i + 1;
        }

      hits++;
      if ((!bobj) || (t < min))
        {

          min = t;
          b = i;
          bobj = obj;
        }
    }
  if (level == -1)
    return 0;

  if (bobj)
    {
      p.x = r->v1.x + (r->v2.x - r->v1.x) * min;
      p.y = r->v1.y + (r->v2.y - r->v1.y) * min;
      p.z = r->v1.z + (r->v2.z - r->v1.z) * min;

      calclight (col, &p, bobj);

      if (world.flags & SMARTAMBIENT)
        {
          gdouble ambient = 0.3 * exp (-min / world.smartambient);
          GimpVector4 lcol;
          objcolor (&lcol, &p, bobj);
          vmul (&lcol, ambient);
          vadd (col, &lcol);
        }

      for (i = 0; i < bobj->numtexture; i++)
        {

          if ((world.quality >= 4)
              && ((bobj->texture[i].type == REFLECTION)
                  || (bobj->texture[i].type == PHONG)))
            {

              GimpVector4 refcol, norm, ocol;
              ray ref;

              objcolor (&ocol, &p, bobj);

              vcopy (&ref.v1, &p);
              vcopy (&ref.v2, &r->v1);
              ref.inside = r->inside;
              ref.ior = r->ior;

              vmix (&ref.v1, &ref.v1, &ref.v2, 0.9999); /* push it a tad */

              vsub (&ref.v2, &p);
              objnormal (&norm, bobj, &p);
              vnorm (&norm, 1.0);
              vrotate (&norm, 180.0, &ref.v2);

              vmul (&norm, -0.0001);    /* push it a tad */
              vadd (&ref.v1, &norm);

              vnorm (&ref.v2, 1.0);
              vadd (&ref.v2, &p);

              if ((world.quality >= 5)
                  && (bobj->texture[i].type == REFLECTION))
                {
                  traceray (&ref, &refcol, level - 1,
                            imp * vmax (&bobj->texture[i].reflection));
                  vvmul (&refcol, &bobj->texture[i].reflection);
                  refcol.w = ocol.w;
                  vadd (col, &refcol);
                }
              if (bobj->texture[i].type == PHONG)
                {
                  vcset (&refcol, 0, 0, 0, 0);
                  calcphong (bobj, &ref, &refcol);
                  refcol.w = ocol.w;
                  vadd (col, &refcol);
                }

            }

          if ((world.quality >= 5) && (col->w < 1.0))
            {
              GimpVector4 refcol;
              ray ref;

              vcopy (&ref.v1, &p);
              vcopy (&ref.v2, &p);
              vsub (&ref.v2, &r->v1);
              vnorm (&ref.v2, 1.0);
              vadd (&ref.v2, &p);

              vmix (&ref.v1, &ref.v1, &ref.v2, 0.999);  /* push it a tad */
              traceray (&ref, &refcol, level - 1, imp * (1.0 - col->w));
              vmul (&refcol, (1.0 - col->w));
              vadd (col, &refcol);
            }

          if ((world.quality >= 5) && (bobj->texture[i].type == TRANSPARENT))
            {
              GimpVector4 refcol;
              ray ref;

              vcopy (&ref.v1, &p);
              vcopy (&ref.v2, &p);
              vsub (&ref.v2, &r->v1);
              vnorm (&ref.v2, 1.0);
              vadd (&ref.v2, &p);

              vmix (&ref.v1, &ref.v1, &ref.v2, 0.999);  /* push it a tad */

              traceray (&ref, &refcol, level - 1,
                        imp * vmax (&bobj->texture[i].transparent));
              vvmul (&refcol, &bobj->texture[i].transparent);

              vadd (col, &refcol);
            }

          if ((world.quality >= 5) && (bobj->texture[i].type == SMOKE))
            {
              GimpVector4 smcol, raydir, norm;
              double tran;
              ray ref;

              vcopy (&ref.v1, &p);
              vcopy (&ref.v2, &p);
              vsub (&ref.v2, &r->v1);
              vnorm (&ref.v2, 1.0);
              vadd (&ref.v2, &p);

              objnormal (&norm, bobj, &p);
              vcopy (&raydir, &r->v2);
              vsub (&raydir, &r->v1);
              vnorm (&raydir, 1.0);
              tran = vdot (&norm, &raydir);
              if (tran < 0.0)
                continue;
              tran *= tran;
              vcopy (&smcol, &bobj->texture[i].color1);
              vmul (&smcol, tran);
              vadd (col, &smcol);
            }

          if ((world.quality >= 5) && (bobj->texture[i].type == REFRACTION))
            {
              GimpVector4 refcol, norm, tmpv;
              ray ref;
              double c1, c2, n1, n2, n;

              vcopy (&ref.v1, &p);
              vcopy (&ref.v2, &p);
              vsub (&ref.v2, &r->v1);
              vadd (&ref.v2, &r->v2);

              vmix (&ref.v1, &ref.v1, &ref.v2, 0.999);  /* push it a tad */

              vsub (&ref.v2, &p);
              objnormal (&norm, bobj, &p);

              if (r->inside == b)
                {
                  ref.inside = -1;
                  ref.ior = 1.0;
                }
              else
                {
                  ref.inside = b;
                  ref.ior = bobj->texture[i].ior;
                }

              c1 = vdot (&norm, &ref.v2);

              if (ref.inside < 0)
                c1 = -c1;

              n1 = r->ior;      /* IOR of current media  */
              n2 = ref.ior;     /* IOR of new media  */
              n = n1 / n2;
              c2 = 1.0 - n * n * (1.0 - c1 * c1);

              if (c2 < 0.0)
                {
                  /* FIXME: Internal reflection should occur */
                  c2 = sqrt (-c2);

                }
              else
                {
                  c2 = sqrt (c2);
                }

              vmul (&ref.v2, n);
              vcopy (&tmpv, &norm);
              vmul (&tmpv, n * c1 - c2);
              vadd (&ref.v2, &tmpv);

              vnorm (&ref.v2, 1.0);
              vadd (&ref.v2, &p);

              traceray (&ref, &refcol, level - 1,
                        imp * vmax (&bobj->texture[i].refraction));

              vvmul (&refcol, &bobj->texture[i].refraction);
              vadd (col, &refcol);
            }
        }
    }
  else
    {
      vcset (col, 0, 0, 0, 0);
      min = 10000.0;
      vcset (&p, 0, 0, 0, 0);
    }

  for (i = 0; i < world.numatmos; i++)
    {
      GimpVector4 tmpcol;
      if (world.atmos[i].type == FOG)
        {
          gdouble v, pt[3];
          pt[0] = p.x;
          pt[1] = p.y;
          pt[2] = p.z;
          if ((v = world.atmos[i].turbulence) > 0.0)
            v = turbulence (pt, 1, 256) * world.atmos[i].turbulence;
          v = exp (-(min + v) / world.atmos[i].density);
          vmul (col, v);
          vcopy (&tmpcol, &world.atmos[i].color);
          vmul (&tmpcol, 1.0 - v);
          vadd (col, &tmpcol);
        }
    }

  return hits;
}

static void
setdefaults (texture * t)
{
  memset (t, 0, sizeof (texture));
  t->type = SOLID;
  vcset (&t->color1, 1, 1, 1, 1);
  vcset (&t->color2, 0, 0, 0, 1);
  vcset (&t->diffuse, 1, 1, 1, 1);
  vcset (&t->ambient, 0, 0, 0, 1);
  vset (&t->scale, 1, 1, 1);
  vset (&t->rotate, 0, 0, 0);
  vset (&t->translate, 0, 0, 0);
  t->oscale = 1.0;
  t->amount = 1.0;
  t->exp = 1.0;
}

static gchar *
mklabel (texture * t)
{
  struct textures_t *l;
  static gchar tmps[100];

  if (t->majtype == 0)
    strcpy (tmps, _("Texture"));
  else if (t->majtype == 1)
    strcpy (tmps, _("Bumpmap"));
  else if (t->majtype == 2)
    strcpy (tmps, _("Light"));
  else
    strcpy (tmps, "<unknown>");
  if ((t->majtype == 0) || (t->majtype == 1))
    {
      strcat (tmps, " / ");
      l = textures;
      while (l->s)
        {
          if (t->type == l->n)
            {
              strcat (tmps, gettext (l->s));
              break;
            }
          l++;
        }
    }
  return tmps;
}

static texture *
currenttexture (void)
{
  GtkTreeSelection *sel;
  GtkTreeIter       iter;
  texture          *t = NULL;

  sel = gtk_tree_view_get_selection (texturelist);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gtk_tree_model_get (gtk_tree_view_get_model (texturelist), &iter,
                          TEXTURE, &t,
                          -1);
    }

  return t;
}

static void
relabel (void)
{
  GtkTreeModel     *model;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;
  texture          *t = NULL;

  sel = gtk_tree_view_get_selection (texturelist);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      model = gtk_tree_view_get_model (texturelist);

      gtk_tree_model_get (model, &iter,
                          TEXTURE, &t,
                          -1);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          TYPE, mklabel (t),
                          -1);
    }
}

static gboolean noupdate = FALSE;

static void
setvals (texture *t)
{
  struct textures_t *l;

  if (!t)
    return;

  noupdate = TRUE;
  gtk_adjustment_set_value (amountscale, t->amount);

  gtk_adjustment_set_value (scalescale, t->oscale);

  gtk_adjustment_set_value (scalexscale, t->scale.x);
  gtk_adjustment_set_value (scaleyscale, t->scale.y);
  gtk_adjustment_set_value (scalezscale, t->scale.z);

  gtk_adjustment_set_value (rotxscale, t->rotate.x);
  gtk_adjustment_set_value (rotyscale, t->rotate.y);
  gtk_adjustment_set_value (rotzscale, t->rotate.z);

  gtk_adjustment_set_value (posxscale, t->translate.x);
  gtk_adjustment_set_value (posyscale, t->translate.y);
  gtk_adjustment_set_value (poszscale, t->translate.z);

  gtk_adjustment_set_value (turbulencescale, t->turbulence.x);
  gtk_adjustment_set_value (expscale, t->exp);

  drawcolor1 (NULL);
  drawcolor2 (NULL);

  l = textures;
  while (l->s)
    {
      if (l->n == t->type)
        {
          gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (texturemenu),
                                         l->index);
          break;
        }
      l++;
    }

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (typemenu), t->majtype);

  noupdate = FALSE;
}

static void
selectitem (GtkTreeSelection *treeselection,
            gpointer          data)
{
  setvals (currenttexture ());
}

static void
addtexture (void)
{
  GtkListStore *list_store;
  GtkTreeIter   iter;
  gint          n = s.com.numtexture;

  if (n == MAXTEXTUREPEROBJ - 1)
    return;

  setdefaults (&s.com.texture[n]);

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (texturelist));

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter,
                      TYPE,    mklabel (&s.com.texture[n]),
                      TEXTURE, &s.com.texture[n],
                      -1);
  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (texturelist),
                                  &iter);

  s.com.numtexture++;

  restartrender ();
}

static void
duptexture (void)
{
  GtkListStore *list_store;
  GtkTreeIter   iter;
  texture      *t = currenttexture ();
  gint          n = s.com.numtexture;

  if (n == MAXTEXTUREPEROBJ - 1)
    return;
  if (!t)
    return;

  s.com.texture[n] = *t;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (texturelist));

  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter,
                      TYPE,    mklabel (&s.com.texture[n]),
                      TEXTURE, &s.com.texture[n],
                      -1);
  gtk_tree_selection_select_iter (gtk_tree_view_get_selection (texturelist),
                                  &iter);

  s.com.numtexture++;

  restartrender ();
}

static void
rebuildlist (void)
{
  GtkListStore     *list_store;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;
  gint              n;

  sel = gtk_tree_view_get_selection (texturelist);

  for (n = 0; n < s.com.numtexture; n++)
    {
      if (s.com.numtexture && (s.com.texture[n].majtype < 0))
        {
          gint i;

          for (i = n; i < s.com.numtexture - 1; i++)
            s.com.texture[i] = s.com.texture[i + 1];

          s.com.numtexture--;
          n--;
        }
    }

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (texturelist));

  for (n = 0; n < s.com.numtexture; n++)
    {
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          TYPE,    mklabel (&s.com.texture[n]),
                          TEXTURE, &s.com.texture[n],
                          -1);
    }

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    gtk_tree_selection_select_iter (sel, &iter);

  restartrender ();
}

static void
deltexture (void)
{
  GtkTreeSelection *sel;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  texture          *t = NULL;

  sel = gtk_tree_view_get_selection (texturelist);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      model = gtk_tree_view_get_model (texturelist);

      gtk_tree_model_get (model, &iter,
                          TEXTURE, &t,
                          -1);
      t->majtype = -1;
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    }

  restartrender ();
}

static void
loadit (const gchar * fn)
{
  FILE    *f;
  gchar    endbuf[21 * (G_ASCII_DTOSTR_BUF_SIZE + 1)];
  gchar   *end = endbuf;
  gchar    line[1024];
  gchar    fmt_str[16];
  gint     i;
  texture *t;
  gint     majtype, type;

  f = g_fopen (fn, "rt");
  if (! f)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (fn), g_strerror (errno));
      return;
    }

  if (2 != fscanf (f, "%d %d", &majtype, &type) || majtype < 0 || majtype > 2)
    {
      g_message (_("File '%s' is not a valid save file."),
                 gimp_filename_to_utf8 (fn));
      fclose (f);
      return;
    }

  rewind (f);

  s.com.numtexture = 0;

  snprintf (fmt_str, sizeof (fmt_str), "%%d %%d %%%" G_GSIZE_FORMAT "s", sizeof (endbuf) - 1);

  while (!feof (f))
    {

      if (!fgets (line, 1023, f))
        break;

      i = s.com.numtexture;
      t = &s.com.texture[i];
      setdefaults (t);

      if (sscanf (line, fmt_str, &t->majtype, &t->type, end) != 3)
        t->color1.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color1.y = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color1.z = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color1.w = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color2.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color2.y = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color2.z = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->color2.w = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->oscale = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->turbulence.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->amount = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->exp = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->scale.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->scale.y = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->scale.z = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->rotate.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->rotate.y = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->rotate.z = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->translate.x = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->translate.y = g_ascii_strtod (end, &end);
      if (end && errno != ERANGE)
        t->translate.z = g_ascii_strtod (end, &end);

      s.com.numtexture++;
    }

  fclose (f);
}

static void
loadpreset_response (GtkWidget *dialog,
                     gint       response_id,
                     gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model = gtk_tree_view_get_model (texturelist);
      gchar        *name;

      name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      gtk_list_store_clear (GTK_LIST_STORE (model));

      loadit (name);
      g_free (name);

      rebuildlist ();
    }

  gtk_widget_hide (dialog);
}

static void
saveit (const gchar *fn)
{
  gint   i;
  FILE  *f;
  gchar  buf[G_ASCII_DTOSTR_BUF_SIZE];

  f = g_fopen (fn, "wt");
  if (!f)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (fn), g_strerror (errno));
      return;
    }

  for (i = 0; i < s.com.numtexture; i++)
    {
      texture *t = &s.com.texture[i];

      if (t->majtype < 0)
        continue;

      fprintf (f, "%d %d", t->majtype, t->type);
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color1.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color1.y));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color1.z));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color1.w));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color2.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color2.y));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color2.z));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->color2.w));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->oscale));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->turbulence.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->amount));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->exp));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->scale.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->scale.y));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->scale.z));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->rotate.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->rotate.y));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->rotate.z));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->translate.x));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->translate.y));
      fprintf (f, " %s", g_ascii_dtostr (buf, sizeof (buf), t->translate.z));
      fprintf (f, "\n");
    }

  fclose (f);
}

static void
savepreset_response (GtkWidget *dialog,
                     gint       response_id,
                     gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      saveit (name);
      g_free (name);
    }

  gtk_widget_hide (dialog);
}

static void
loadpreset (GtkWidget *widget,
            GtkWidget *parent)
{
  fileselect (GTK_FILE_CHOOSER_ACTION_OPEN, parent);
}

static void
savepreset (GtkWidget *widget,
            GtkWidget *parent)
{
  fileselect (GTK_FILE_CHOOSER_ACTION_SAVE, parent);
}

static void
fileselect (GtkFileChooserAction  action,
            GtkWidget            *parent)
{
  static GtkWidget *windows[2] = { NULL, NULL };

  gchar *titles[]   = { N_("Open File"), N_("Save File") };
  void  *handlers[] = { loadpreset_response,   savepreset_response };

  if (! windows[action])
    {
      GtkWidget *dialog = windows[action] =
        gtk_file_chooser_dialog_new (gettext (titles[action]),
                                     GTK_WINDOW (parent),
                                     action,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,

                                     action == GTK_FILE_CHOOSER_ACTION_OPEN ?
                                     _("_Open") : _("_Save"),
                                     GTK_RESPONSE_OK,

                                     NULL);

      gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

      if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                        TRUE);

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &windows[action]);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (handlers[action]),
                        NULL);
    }

  gtk_window_present (GTK_WINDOW (windows[action]));
}

static void
initworld (void)
{
  gint i;

  memset (&world, 0, sizeof (world));

  s.com.type = SPHERE;
  s.a.x = s.a.y = s.a.z = 0.0;
  s.r = 4.0;

  /* not: world.obj[0] = s;
   * s is a sphere so error C2115: '=' : incompatible types
   */
  memcpy (&world.obj[0], &s, sizeof (s));
  world.numobj = 1;

  world.obj[0].com.numtexture = 0;
  world.obj[0].com.numnormal = 0;

  for (i = 0; i < s.com.numtexture; i++)
    {
      common *c = &s.com;
      common *d = &world.obj[0].com;
      texture *t = &c->texture[i];
      if ((t->amount <= 0.0) || (t->majtype < 0))
        continue;
      if (t->majtype == 0)
        {                       /* Normal texture */
          if (t->type == PHONG)
            {
              t->phongcolor = t->color1;
              t->phongsize = t->oscale / 25.0;
            }
          d->texture[d->numtexture] = *t;
          vmul (&d->texture[d->numtexture].scale,
                d->texture[d->numtexture].oscale);
          d->numtexture++;
        }
      else if (t->majtype == 1)
        {                       /* Bumpmap */
          d->normal[d->numnormal] = *t;
          vmul (&d->normal[d->numnormal].scale,
                d->texture[d->numnormal].oscale);
          d->numnormal++;
        }
      else if (t->majtype == 2)
        {                       /* Lightsource */
          light l;
          vcopy (&l.a, &t->translate);
          vcopy (&l.color, &t->color1);
          vmul (&l.color, t->amount);
          world.light[world.numlight] = l;
          world.numlight++;
        }
    }

  world.quality = 5;

  world.flags |= SMARTAMBIENT;
  world.smartambient = 40.0;
}

static gboolean
draw (GtkWidget *widget,
      cairo_t   *cr)
{
  cairo_set_source_surface (cr, buffer, 0.0, 0.0);

  cairo_paint (cr);

  return TRUE;
}

static void
restartrender (void)
{
  if (idle_id)
    g_source_remove (idle_id);

  idle_id = g_idle_add ((GSourceFunc) render, NULL);
}

static void
selecttexture (GtkWidget *widget,
               gpointer   data)
{
  texture *t;

  if (noupdate)
    return;

  t = currenttexture ();
  if (!t)
    return;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &t->type);

  relabel ();
  restartrender ();
}

static void
selecttype (GtkWidget *widget,
            gpointer   data)
{
  texture *t;

  if (noupdate)
    return;

  t = currenttexture ();
  if (!t)
    return;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &t->majtype);

  relabel ();
  restartrender ();
}

static void
getscales (GtkWidget *widget,
           gpointer   data)
{
  gdouble f;
  texture *t;

  if (noupdate)
    return;

  t = currenttexture ();
  if (!t)
    return;

  t->amount = gtk_adjustment_get_value (amountscale);
  t->exp = gtk_adjustment_get_value (expscale);

  f = gtk_adjustment_get_value (turbulencescale);
  vset (&t->turbulence, f, f, f);

  t->oscale = gtk_adjustment_get_value (scalescale);

  t->scale.x = gtk_adjustment_get_value (scalexscale);
  t->scale.y = gtk_adjustment_get_value (scaleyscale);
  t->scale.z = gtk_adjustment_get_value (scalezscale);

  t->rotate.x = gtk_adjustment_get_value (rotxscale);
  t->rotate.y = gtk_adjustment_get_value (rotyscale);
  t->rotate.z = gtk_adjustment_get_value (rotzscale);

  t->translate.x = gtk_adjustment_get_value (posxscale);
  t->translate.y = gtk_adjustment_get_value (posyscale);
  t->translate.z = gtk_adjustment_get_value (poszscale);

  restartrender ();
}


static void
color1_changed (GimpColorButton *button)
{
  texture *t = currenttexture ();

  if (t)
    {
      GimpRGB color;

      gimp_color_button_get_color (button, &color);

      t->color1.x = color.r;
      t->color1.y = color.g;
      t->color1.z = color.b;
      t->color1.w = color.a;

      restartrender ();
    }
}

static void
color2_changed (GimpColorButton *button)
{
  texture *t = currenttexture ();

  if (t)
    {
      GimpRGB color;

      gimp_color_button_get_color (button, &color);

      t->color2.x = color.r;
      t->color2.y = color.g;
      t->color2.z = color.b;
      t->color2.w = color.a;

      restartrender ();
    }
}

static void
drawcolor1 (GtkWidget *w)
{
  static GtkWidget *lastw = NULL;

  GimpRGB  color;
  texture *t = currenttexture ();

  if (w)
    lastw = w;
  else
    w = lastw;

  if (!w)
    return;
  if (!t)
    return;

  gimp_rgba_set (&color,
                 t->color1.x, t->color1.y, t->color1.z, t->color1.w);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (w), &color);
}

static void
drawcolor2 (GtkWidget *w)
{
  static GtkWidget *lastw = NULL;

  GimpRGB  color;
  texture *t = currenttexture ();

  if (w)
    lastw = w;
  else
    w = lastw;

  if (!w)
    return;
  if (!t)
    return;

  gimp_rgba_set (&color,
                 t->color2.x, t->color2.y, t->color2.z, t->color2.w);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (w), &color);
}

static gboolean do_run = FALSE;

static void
sphere_response (GtkWidget *widget,
                 gint       response_id,
                 gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      s.com.numtexture = 3;

      setdefaults (&s.com.texture[0]);
      setdefaults (&s.com.texture[1]);
      setdefaults (&s.com.texture[2]);

      s.com.texture[1].majtype = 2;
      vset (&s.com.texture[1].color1, 1, 1, 1);
      vset (&s.com.texture[1].translate, -15, -15, -15);

      s.com.texture[2].majtype = 2;
      vset (&s.com.texture[2].color1, 0, 0.4, 0.4);
      vset (&s.com.texture[2].translate, 15, 15, -15);

      gtk_list_store_clear (GTK_LIST_STORE (gtk_tree_view_get_model (texturelist)));

      rebuildlist ();
      break;

    case GTK_RESPONSE_OK:
      if (idle_id)
        {
          g_source_remove (idle_id);
          idle_id = 0;
        }

      do_run = TRUE;

    default:
      gtk_widget_hide (widget);
      gtk_main_quit ();
      break;
    }
}

static GtkWidget *
makewindow (void)
{
  GtkListStore      *store;
  GtkTreeViewColumn *col;
  GtkTreeSelection  *selection;
  GtkWidget  *window;
  GtkWidget  *main_hbox;
  GtkWidget  *main_vbox;
  GtkWidget  *table;
  GtkWidget  *frame;
  GtkWidget  *scrolled;
  GtkWidget  *hbox;
  GtkWidget  *vbox;
  GtkWidget  *button;
  GtkWidget  *list;
  GimpRGB     rgb = { 0, 0, 0, 0 };

  window = gimp_dialog_new (_("Sphere Designer"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            _("_Reset"),  RESPONSE_RESET,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (window),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (window));

  g_signal_connect (window, "response",
                    G_CALLBACK (sphere_response),
                    NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (window))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  drawarea = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), drawarea);
  gtk_widget_set_size_request (drawarea, PREVIEWSIZE, PREVIEWSIZE);
  gtk_widget_show (drawarea);

  g_signal_connect (drawarea, "draw",
                    G_CALLBACK (draw), NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (loadpreset),
                    window);

  button = gtk_button_new_with_mnemonic (_("_Save"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (savepreset),
                    window);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_end (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
  gtk_widget_show (scrolled);

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  texturelist = GTK_TREE_VIEW (list);

  selection = gtk_tree_view_get_selection (texturelist);

  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  g_signal_connect (selection, "changed",
                    G_CALLBACK (selectitem),
                    NULL);

  gtk_widget_set_size_request (list, -1, 150);
  gtk_container_add (GTK_CONTAINER (scrolled), list);
  gtk_widget_show (list);

  col = gtk_tree_view_column_new_with_attributes (_("Layers"),
                                                  gtk_cell_renderer_text_new (),
                                                  "text", TYPE,
                                                  NULL);
  gtk_tree_view_append_column (texturelist, col);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (addtexture), NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (_("D_uplicate"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (duptexture), NULL);
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (deltexture), NULL);
  gtk_widget_show (button);

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, FALSE, FALSE, 0);
  gtk_widget_show (main_hbox);

  frame = gimp_frame_new (_("Properties"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (7, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  typemenu = gimp_int_combo_box_new (_("Texture"), 0,
                                     _("Bump"),    1,
                                     _("Light"),   2,
                                     NULL);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (typemenu), 0,
                              G_CALLBACK (selecttype),
                              NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Type:"), 0.0, 0.5,
                             typemenu, 2, FALSE);

  texturemenu = g_object_new (GIMP_TYPE_INT_COMBO_BOX, NULL);
  {
    struct textures_t *t;

    for (t = textures; t->s; t++)
      gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (texturemenu),
                                 GIMP_INT_STORE_VALUE, t->n,
                                 GIMP_INT_STORE_LABEL, gettext (t->s),
                                 -1);
  }

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (texturemenu), 0,
                              G_CALLBACK (selecttexture),
                              NULL);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Texture:"), 0.0, 0.5,
                             texturemenu, 2, FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Colors:"), 0.0, 0.5,
                             hbox, 2, FALSE);

  button = gimp_color_button_new (_("Color Selection Dialog"),
                                  COLORBUTTONWIDTH, COLORBUTTONHEIGHT, &rgb,
                                  GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  drawcolor1 (button);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (color1_changed),
                    NULL);

  button = gimp_color_button_new (_("Color Selection Dialog"),
                                  COLORBUTTONWIDTH, COLORBUTTONHEIGHT, &rgb,
                                  GIMP_COLOR_AREA_FLAT);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);
  drawcolor2 (button);

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (color2_changed),
                    NULL);

  scalescale = gimp_scale_entry_new (GTK_TABLE (table), 0, 3, _("Scale:"),
                                     100, -1, 1.0, 0.0, 10.0, 0.1, 1.0, 1,
                                     TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (scalescale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  turbulencescale = gimp_scale_entry_new (GTK_TABLE (table), 0, 4,
                                          _("Turbulence:"),
                                          100, -1, 1.0, 0.0, 10.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (turbulencescale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  amountscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 5, _("Amount:"),
                                       100, -1, 1.0, 0.0, 1.0, 0.01, 0.1, 2,
                                       TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (amountscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  expscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 6, _("Exp.:"),
                                   100, -1, 1.0, 0.0, 1.0, 0.01, 0.1, 2,
                                   TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (expscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  frame = gimp_frame_new (_("Transformations"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (9, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), 5, 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  scalexscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 0, _("Scale X:"),
                                      100, -1, 1.0, 0.0, 10.0, 0.1, 1.0, 2,
                                      TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (scalexscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  scaleyscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 1, _("Scale Y:"),
                                      100, -1, 1.0, 0.0, 10.0, 0.1, 1.0, 2,
                                      TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (scaleyscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);
  scalezscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 2, _("Scale Z:"),
                                      100, -1, 1.0, 0.0, 10.0, 0.1, 1.0, 2,
                                      TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (scalezscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  rotxscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 3, _("Rotate X:"),
                                    100, -1, 0.0, 0.0, 360.0, 1.0, 10.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (rotxscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  rotyscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 4, _("Rotate Y:"),
                                    100, -1, 0.0, 0.0, 360.0, 1.0, 10.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (rotyscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  rotzscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 5, _("Rotate Z:"),
                                    100, -1, 0.0, 0.0, 360.0, 1.0, 10.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (rotzscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  posxscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 6, _("Position X:"),
                                    100, -1, 0.0, -20.0, 20.0, 0.1, 1.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (posxscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  posyscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 7, _("Position Y:"),
                                    100, -1, 0.0, -20.0, 20.0, 0.1, 1.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (posyscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  poszscale = gimp_scale_entry_new (GTK_TABLE (table), 0, 8, _("Position Z:"),
                                    100, -1, 0.0, -20.0, 20.0, 0.1, 1.0, 1,
                                    TRUE, 0.0, 0.0, NULL, NULL);
  g_signal_connect (poszscale, "value-changed",
                    G_CALLBACK (getscales),
                    NULL);

  gtk_widget_show (window);

  return window;
}

static guchar
pixelval (gdouble v)
{
  v += 0.5;
  if (v < 0.0)
    return 0;
  if (v > 255.0)
    return 255;
  return v;
}

static gboolean
render (void)
{
  GimpVector4  col;
  guchar *dest_row;
  ray     r;
  gint    x, y, p;
  gint    tx  = PREVIEWSIZE;
  gint    ty  = PREVIEWSIZE;
  gint    bpp = 4;

  idle_id = 0;

  initworld ();

  r.v1.z = -10.0;
  r.v2.z = 0.0;

  if (world.obj[0].com.numtexture > 0)
    {
      cairo_surface_flush (buffer);

      for (y = 0; y < ty; y++)
        {
          dest_row = img + y * img_stride;

          for (x = 0; x < tx; x++)
            {
              gint g, gridsize = 16;

              g = ((x / gridsize + y / gridsize) % 2) * 60 + 100;

              r.v1.x = r.v2.x = 8.5 * (x / (float) (tx - 1) - 0.5);
              r.v1.y = r.v2.y = 8.5 * (y / (float) (ty - 1) - 0.5);

              p = x * bpp;

              traceray (&r, &col, 10, 1.0);

              if (col.w < 0.0)
                col.w = 0.0;
              else if (col.w > 1.0)
                col.w = 1.0;

              GIMP_CAIRO_RGB24_SET_PIXEL ((dest_row + p),
                pixelval (255 * col.x) * col.w + g * (1.0 - col.w),
                pixelval (255 * col.y) * col.w + g * (1.0 - col.w),
                pixelval (255 * col.z) * col.w + g * (1.0 - col.w));
             }
        }

      cairo_surface_mark_dirty (buffer);
    }

  gtk_widget_queue_draw (drawarea);

  return FALSE;
}

static void
realrender (GimpDrawable *drawable)
{
  gint          x, y;
  ray           r;
  GimpVector4   rcol;
  gint          width, height;
  gint          x1, y1;
  guchar       *dest;
  gint          bpp;
  GimpPixelRgn  pr, dpr;
  guchar       *buffer, *ibuffer;

  initworld ();

  r.v1.z = -10.0;
  r.v2.z = 0.0;

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x1, &y1, &width, &height))
    return;

  gimp_pixel_rgn_init (&pr, drawable, 0, 0,
                       gimp_drawable_width (drawable->drawable_id),
                       gimp_drawable_height (drawable->drawable_id), FALSE,
                       FALSE);
  gimp_pixel_rgn_init (&dpr, drawable, 0, 0,
                       gimp_drawable_width (drawable->drawable_id),
                       gimp_drawable_height (drawable->drawable_id), TRUE,
                       TRUE);
  bpp = gimp_drawable_bpp (drawable->drawable_id);
  buffer = g_malloc (width * 4);
  ibuffer = g_malloc (width * 4);

  gimp_progress_init (_("Rendering sphere"));

  for (y = 0; y < height; y++)
    {
      dest = buffer;
      for (x = 0; x < width; x++)
        {
          r.v1.x = r.v2.x = 8.1 * (x / (float) (width - 1) - 0.5);
          r.v1.y = r.v2.y = 8.1 * (y / (float) (height - 1) - 0.5);

          traceray (&r, &rcol, 10, 1.0);
          dest[0] = pixelval (255 * rcol.x);
          dest[1] = pixelval (255 * rcol.y);
          dest[2] = pixelval (255 * rcol.z);
          dest[3] = pixelval (255 * rcol.w);
          dest += 4;
        }
      gimp_pixel_rgn_get_row (&pr, ibuffer, x1, y1 + y, width);
      for (x = 0; x < width; x++)
        {
          gint   k, dx = x * 4, sx = x * bpp;
          gfloat a     = buffer[dx + 3] / 255.0;

          for (k = 0; k < bpp; k++)
            {
              ibuffer[sx + k] =
                buffer[dx + k] * a + ibuffer[sx + k] * (1.0 - a);
            }
        }
      gimp_pixel_rgn_set_row (&dpr, ibuffer, x1, y1 + y, width);
      gimp_progress_update ((gdouble) y / (gdouble) height);
    }
  gimp_progress_update (1.0);
  g_free (buffer);
  g_free (ibuffer);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
}

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Create an image of a textured sphere"),
                          "This plug-in can be used to create textured and/or "
                          "bumpmapped spheres, and uses a small lightweight "
                          "raytracer to perform the task with good quality",
                          "Vidar Madsen",
                          "Vidar Madsen",
                          "1999",
                          N_("Sphere _Designer..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Render");
}

static gboolean
sphere_main (GimpDrawable *drawable)
{
  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  img_stride = cairo_format_stride_for_width (CAIRO_FORMAT_RGB24, PREVIEWSIZE);
  img = g_malloc0 (img_stride * PREVIEWSIZE);

  buffer = cairo_image_surface_create_for_data (img, CAIRO_FORMAT_RGB24,
                                                PREVIEWSIZE,
                                                PREVIEWSIZE,
                                                img_stride);

  makewindow ();

  if (s.com.numtexture == 0)
    {
      /* Setup and use default list */
      sphere_response (NULL, RESPONSE_RESET, NULL);
    }
  else
    {
      /* Reuse the list from a previous invocation */
      rebuildlist ();
    }

  gtk_main ();

  cairo_surface_destroy (buffer);
  g_free (img);

  return do_run;
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               x, y, w, h;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  if (! gimp_drawable_mask_intersect (drawable->drawable_id, &x, &y, &w, &h))
    {
      g_message (_("Region selected for plug-in is empty"));
      return;
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      s.com.numtexture = 0;
      gimp_get_data (PLUG_IN_PROC, &s);
      if (!sphere_main (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;
    case GIMP_RUN_WITH_LAST_VALS:
      s.com.numtexture = 0;
      gimp_get_data (PLUG_IN_PROC, &s);
      if (s.com.numtexture == 0)
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;
    case GIMP_RUN_NONINTERACTIVE:
    default:
      /* Not implemented yet... */
      gimp_drawable_detach (drawable);
      return;
    }

  gimp_set_data (PLUG_IN_PROC, &s, sizeof (s));

  realrender (drawable);
  gimp_displays_flush ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

MAIN ()
