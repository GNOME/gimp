/* LIC 0.14 -- image filter plug-in for GIMP
 * Copyright (C) 1996 Tom Bech
 *
 * E-mail: tomb@gimp.org
 * You can contact the original GIMP authors at gimp@xcf.berkeley.edu
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In other words, you can't sue me for whatever happens while using this ;)
 *
 * Changes (post 0.10):
 * -> 0.11: Fixed a bug in the convolution kernels (Tom).
 * -> 0.12: Added Quartic's bilinear interpolation stuff (Tom).
 * -> 0.13 Changed some UI stuff causing trouble with the 0.60 release, added
 *         the (GIMP) tags and changed random() calls to rand() (Tom)
 * -> 0.14 Ported to 0.99.11 (Tom)
 *
 * This plug-in implements the Line Integral Convolution (LIC) as described in
 * Cabral et al. "Imaging vector fields using line integral convolution" in the
 * Proceedings of ACM SIGGRAPH 93. Publ. by ACM, New York, NY, USA. p. 263-270.
 * (See http://www8.cs.umu.se/kurser/TDBD13/VT00/extra/p263-cabral.pdf)
 *
 * Some of the code is based on code by Steinar Haugen (thanks!), the Perlin
 * noise function is practically ripped as is :)
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/************/
/* Typedefs */
/************/

#define numx    40              /* Pseudo-random vector grid size */
#define numy    40

#define PLUG_IN_PROC   "plug-in-lic"
#define PLUG_IN_BINARY "van-gogh-lic"
#define PLUG_IN_ROLE   "gimp-van-gogh-lic"

typedef enum
{
  LIC_HUE,
  LIC_SATURATION,
  LIC_BRIGHTNESS
} LICEffectChannel;


typedef struct _Lic      Lic;
typedef struct _LicClass LicClass;

struct _Lic
{
  GimpPlugIn parent_instance;
};

struct _LicClass
{
  GimpPlugInClass parent_class;
};


#define LIC_TYPE  (lic_get_type ())
#define LIC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIC_TYPE, Lic))

GType                   lic_get_type         (void) G_GNUC_CONST;

static GList          * lic_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * lic_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * lic_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable        **drawables,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

static gboolean         create_main_dialog   (GimpProcedure        *procedure,
                                              GimpProcedureConfig  *config,
                                              GimpDrawable         *drawable);
static void             compute_image        (GimpProcedureConfig  *config,
                                              GimpDrawable         *drawable);

G_DEFINE_TYPE (Lic, lic, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (LIC_TYPE)
DEFINE_STD_SET_I18N


static void
lic_class_init (LicClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = lic_query_procedures;
  plug_in_class->create_procedure = lic_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
lic_init (Lic *lic)
{
}

static GList *
lic_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
lic_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            lic_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Van Gogh (LIC)..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Artistic");

      gimp_procedure_set_documentation (procedure,
                                        _("Special effects that nobody "
                                          "understands"),
                                        "No help yet",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 0.14, September 24 1997");

      gimp_procedure_add_choice_argument (procedure, "effect-channel",
                                          _("E_ffect Channel"),
                                          _("Effect Channel"),
                                          gimp_choice_new_with_values ("hue",        LIC_HUE,        _("Hue"),        NULL,
                                                                       "saturation", LIC_SATURATION, _("Saturation"), NULL,
                                                                       "brightness", LIC_BRIGHTNESS, _("Brightness"), NULL,
                                                                       NULL),
                                          "brightness",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "effect-operator",
                                          _("Effect O_perator"),
                                          _("Effect Operator"),
                                          gimp_choice_new_with_values ("derivative", 0, _("Derivative"), NULL,
                                                                       "gradient",   1, _("Gradient"),   NULL,
                                                                       NULL),
                                          "gradient",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "effect-convolve",
                                          _("Con_volve"),
                                          _("Convolve"),
                                          gimp_choice_new_with_values ("with-white-noise",  0, _("With white noise"),  NULL,
                                                                       "with-source-image", 1, _("With source image"), NULL,
                                                                       NULL),
                                          "with-source-image",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "effect-image",
                                            _("Effect i_mage"),
                                            _("Effect image"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "filter-length",
                                          _("Fil_ter length"),
                                          _("Filter length"),
                                          0.1, 64.0, 5.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "noise-magnitude",
                                          _("_Noise Magnitude"),
                                          _("Noise Magnitude"),
                                          1.0, 5.0, 2.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "integration-steps",
                                          _("Inte_gration steps"),
                                          _("Integration steps"),
                                          1.0, 40.0, 25.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "min-value",
                                          _("Minimum v_alue"),
                                          _("Minimum value"),
                                          -100.0, 0, -25.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "max-value",
                                          _("Ma_ximum value"),
                                          _("Maximum value"),
                                          0.0, 100, 25.0,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
lic_run (GimpProcedure        *procedure,
         GimpRunMode           run_mode,
         GimpImage            *image,
         GimpDrawable        **drawables,
         GimpProcedureConfig  *config,
         gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  /* Make sure that the drawable is RGBA or RGB color */
  /* ================================================ */

  if (gimp_drawable_is_rgb (drawable))
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          if (! create_main_dialog (procedure, config, drawable))
            {
              return gimp_procedure_new_return_values (procedure,
                                                       GIMP_PDB_CANCEL,
                                                       NULL);
            }
          /*  fallthrough  */

        case GIMP_RUN_WITH_LAST_VALS:
          compute_image (config, drawable);
          break;

        default:
          break;
        }
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

/*****************************/
/* Global variables and such */
/*****************************/

static gdouble G[numx][numy][2];

static gdouble l      = 10.0;
static gdouble dx     =  2.0;
static gdouble dy     =  2.0;
static gdouble minv   = -2.5;
static gdouble maxv   =  2.5;
static gdouble isteps = 20.0;

static gboolean source_drw_has_alpha = FALSE;

static gint    effect_width, effect_height;
static gint    border_x, border_y, border_w, border_h;

/************************/
/* Convenience routines */
/************************/

static void
peek (GeglBuffer *buffer,
      gint        x,
      gint        y,
      gdouble    *color)
{
  gegl_buffer_sample (buffer, x, y, NULL,
                      color, babl_format ("RGBA double"),
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
}

static void
poke (GeglBuffer *buffer,
      gint        x,
      gint        y,
      gdouble    *color)
{
  gegl_buffer_set (buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                   babl_format ("RGBA double"), color,
                   GEGL_AUTO_ROWSTRIDE);
}

static gint
peekmap (const guchar *image,
         gint          x,
         gint          y)
{
  while (x < 0)
    x += effect_width;
  x %= effect_width;

  while (y < 0)
    y += effect_height;
  y %= effect_height;

  return (gint) image[x + effect_width * y];
}

/*************/
/* Main part */
/*************/

/***************************************************/
/* Compute the derivative in the x and y direction */
/* We use these convolution kernels:               */
/*     |1 0 -1|     |  1   2   1|                  */
/* DX: |2 0 -2| DY: |  0   0   0|                  */
/*     |1 0 -1|     | -1  -2  -1|                  */
/* (It's a variation of the Sobel kernels, really)  */
/***************************************************/

static gint
gradx (const guchar *image,
       gint          x,
       gint          y)
{
  gint val = 0;

  val = val +     peekmap (image, x-1, y-1);
  val = val -     peekmap (image, x+1, y-1);

  val = val + 2 * peekmap (image, x-1, y);
  val = val - 2 * peekmap (image, x+1, y);

  val = val +     peekmap (image, x-1, y+1);
  val = val -     peekmap (image, x+1, y+1);

  return val;
}

static gint
grady (const guchar *image,
       gint          x,
       gint          y)
{
  gint val = 0;

  val = val +     peekmap (image, x-1, y-1);
  val = val + 2 * peekmap (image, x,   y-1);
  val = val +     peekmap (image, x+1, y-1);

  val = val -     peekmap (image, x-1, y+1);
  val = val - 2 * peekmap (image, x,   y+1);
  val = val -     peekmap (image, x+1, y+1);

  return val;
}

/************************************/
/* A nice 2nd order cubic spline :) */
/************************************/

static gdouble
cubic (gdouble t)
{
  gdouble at = fabs (t);

  return (at < 1.0) ? at * at * (2.0 * at - 3.0) + 1.0 : 0.0;
}

static gdouble
omega (gdouble u,
       gdouble v,
       gint    i,
       gint    j)
{
  while (i < 0)
    i += numx;

  while (j < 0)
    j += numy;

  i %= numx;
  j %= numy;

  return cubic (u) * cubic (v) * (G[i][j][0]*u + G[i][j][1]*v);
}

/*************************************************************/
/* The noise function (2D variant of Perlins noise function) */
/*************************************************************/

static gdouble
noise (gdouble x,
       gdouble y)
{
  gint i, sti = (gint) floor (x / dx);
  gint j, stj = (gint) floor (y / dy);

  gdouble sum = 0.0;

  /* Calculate the gdouble sum */
  /* ======================== */

  for (i = sti; i <= sti + 1; i++)
    for (j = stj; j <= stj + 1; j++)
      sum += omega ((x - (gdouble) i * dx) / dx,
                    (y - (gdouble) j * dy) / dy,
                    i, j);

  return sum;
}

/*************************************************/
/* Generates pseudo-random vectors with length 1 */
/*************************************************/

static void
generatevectors (void)
{
  gdouble alpha;
  gint i, j;
  GRand *gr;

  gr = g_rand_new();

  for (i = 0; i < numx; i++)
    {
      for (j = 0; j < numy; j++)
        {
          alpha = g_rand_double_range (gr, 0, 2) * G_PI;
          G[i][j][0] = cos (alpha);
          G[i][j][1] = sin (alpha);
        }
    }

  g_rand_free (gr);
}

/* A simple triangle filter */
/* ======================== */

static gdouble
filter (gdouble u)
{
  gdouble f = 1.0 - fabs (u) / l;

  return (f < 0.0) ? 0.0 : f;
}

/******************************************************/
/* Compute the Line Integral Convolution (LIC) at x,y */
/******************************************************/

static gdouble
lic_noise (gint    x,
           gint    y,
           gdouble vx,
           gdouble vy)
{
  gdouble i = 0.0;
  gdouble f1 = 0.0, f2 = 0.0;
  gdouble u, step = 2.0 * l / isteps;
  gdouble xx = (gdouble) x, yy = (gdouble) y;
  gdouble c, s;

  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;

  /* Calculate integral numerically */
  /* ============================== */

  f1 = filter (-l) * noise (xx + l * c , yy + l * s);

  for (u = -l + step; u <= l; u += step)
    {
      f2 = filter (u) * noise ( xx - u * c , yy - u * s);
      i += (f1 + f2) * 0.5 * step;
      f1 = f2;
    }

  i = (i - minv) / (maxv - minv);

  i = CLAMP (i, 0.0, 1.0);

  i = (i / 2.0) + 0.5;

  return i;
}

static void
getpixel (GeglBuffer *buffer,
          gdouble    *p,
          gdouble     u,
          gdouble     v)
{
  register gint x1, y1, x2, y2;
  gint width, height;
  gdouble     pp[4];
  gdouble     pixels[16];

  width  = border_w;
  height = border_h;

  x1 = (gint)u;
  y1 = (gint)v;

  if (x1 < 0)
    x1 = width - (-x1 % width);
  else
    x1 = x1 % width;

  if (y1 < 0)
    y1 = height - (-y1 % height);
  else
    y1 = y1 % height;

  x2 = (x1 + 1) % width;
  y2 = (y1 + 1) % height;

  peek (buffer, x1, y1, pp);
  for (gint i = 0; i < 4; i++)
    pixels[i] = pp[i];
  peek (buffer, x2, y1, pp);
  for (gint i = 0; i < 4; i++)
    pixels[i + 4] = pp[i];
  peek (buffer, x1, y2, pp);
  for (gint i = 0; i < 4; i++)
    pixels[i + 8] = pp[i];
  peek (buffer, x2, y2, pp);
  for (gint i = 0; i < 4; i++)
    pixels[i + 12] = pp[i];

  gimp_bilinear_rgb (u, v, pixels, source_drw_has_alpha, p);
}

static void
lic_image (GeglBuffer *buffer,
           gint        x,
           gint        y,
           gdouble     vx,
           gdouble     vy,
           gdouble    *color)
{
  gdouble u, step = 2.0 * l / isteps;
  gdouble xx = (gdouble) x, yy = (gdouble) y;
  gdouble c, s;
  gdouble col[4] = { 0, 0, 0, 0 };
  gdouble col1[4], col2[4], col3[4];

  /* Get vector at x,y */
  /* ================= */

  c = vx;
  s = vy;

  /* Calculate integral numerically */
  /* ============================== */

  getpixel (buffer, col1, xx + l * c, yy + l * s);

  if (source_drw_has_alpha)
    {
      for (gint i = 0; i < 4; i++)
        col1[i] *= filter (-l);
    }
  else
    {
      for (gint i = 0; i < 3; i++)
        col1[i] *= filter (-l);
    }

  for (u = -l + step; u <= l; u += step)
    {
      getpixel (buffer, col2, xx - u * c, yy - u * s);

      if (source_drw_has_alpha)
        {
          for (gint i = 0; i < 4; i++)
            {
              col2[i] *= filter (u);
              col3[i] = col1[i];
            }

          for (gint i = 0; i < 4; i++)
            {
              col3[i] += col2[i];
              col3[i] *= (0.5 * step);
              col[i] += col3[i];
            }
        }
      else
        {
          for (gint i = 0; i < 3; i++)
            {
              col2[i] *= filter (u);
              col3[i] = col1[i];
            }

          for (gint i = 0; i < 3; i++)
            {
              col3[i] += col2[i];
              col3[i] *= (0.5 * step);
              col[i] += col3[i];
            }
        }

      for (gint i = 0; i < 4; i++)
        col1[i] = col2[i];
    }

  if (source_drw_has_alpha)
    {
      for (gint i = 0; i < 4; i++)
        col[i] *= (1.0 / l);
    }
  else
    {
      for (gint i = 0; i < 3; i++)
        col[i] *= (1.0 / l);
    }

  for (gint i = 0; i < 4; i++)
    color[i] = CLAMP (col[i], 0.0, 1.0);
}

static guchar *
rgb_to_hsl (GimpDrawable     *drawable,
            LICEffectChannel  effect_channel)
{
  GeglBuffer *buffer;
  guchar     *themap;
  gfloat      data[3];
  gint        x, y;
  gint        height_max;
  gint        width_max;
  gdouble     val = 0.0;
  gint64      maxc, index = 0;
  GRand      *gr;

  gr = g_rand_new ();

  width_max  = gimp_drawable_get_width (drawable);
  height_max = gimp_drawable_get_height (drawable);

  maxc = width_max * height_max;

  buffer = gimp_drawable_get_buffer (drawable);

  themap = g_new (guchar, maxc);

  for (y = 0; y < height_max; y++)
    {
      for (x = 0; x < width_max; x++)
        {
          gegl_buffer_sample (buffer, x, y, NULL,
                              data, babl_format ("HSL float"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

          switch (effect_channel)
            {
            case LIC_HUE:
              val = data[0] * 255;
              break;
            case LIC_SATURATION:
              val = data[1] * 255;
              break;
            case LIC_BRIGHTNESS:
              val = data[2] * 255;
              break;
            }

          /* add some random to avoid unstructured areas. */
          val += g_rand_double_range (gr, -1.0, 1.0);

          themap[index++] = (guchar) CLAMP0255 (RINT (val));
        }
    }

  g_object_unref (buffer);

  g_rand_free (gr);

  return themap;
}


static void
compute_lic (GimpProcedureConfig *config,
             GimpDrawable        *drawable,
             const guchar        *scalarfield,
             gboolean             rotate)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  gint        xcount, ycount;
  gdouble     color[4] = { 0.0, 0.0, 0.0, 1.0 };
  gdouble     vx, vy, tmp;
  gint        effect_convolve;

  effect_convolve = gimp_procedure_config_get_choice_id (config,
                                                         "effect-convolve");

  src_buffer  = gimp_drawable_get_buffer (drawable);
  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  for (ycount = 0; ycount < border_h; ycount++)
    {
      for (xcount = 0; xcount < border_w; xcount++)
        {
          /* Get derivative at (x,y) and normalize it */
          /* ============================================================== */

          vx = gradx (scalarfield, xcount, ycount);
          vy = grady (scalarfield, xcount, ycount);

          /* Rotate if needed */
          if (rotate)
            {
              tmp = vy;
              vy = -vx;
              vx = tmp;
            }

          tmp = sqrt (vx * vx + vy * vy);
          if (tmp >= 0.000001)
            {
              tmp = 1.0 / tmp;
              vx *= tmp;
              vy *= tmp;
            }

          /* Convolve with the LIC at (x,y) */
          /* ============================== */

          if (effect_convolve == 0)
            {
              peek (src_buffer, xcount, ycount, color);

              tmp = lic_noise (xcount, ycount, vx, vy);

              if (source_drw_has_alpha)
                {
                  for (gint i = 0; i < 4; i++)
                    color[i] *= tmp;
                }
              else
                {
                  for (gint i = 0; i < 3; i++)
                    color[i] *= tmp;
                }
            }
          else
            {
              lic_image (src_buffer, xcount, ycount, vx, vy, color);
            }

          poke (dest_buffer, xcount, ycount, color);
        }

      gimp_progress_update ((gfloat) ycount / (gfloat) border_h);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  gimp_progress_update (1.0);
}

static void
compute_image (GimpProcedureConfig *config,
               GimpDrawable        *drawable)
{
  GimpDrawable *effect_image;
  guchar       *scalarfield = NULL;
  gdouble       filtlen;
  gdouble       noisemag;
  gdouble       intsteps;
  gdouble       minv;
  gdouble       maxv;
  gint          effect_channel;
  gint          effect_operator;
  gint          effect_convolve;

  g_object_get (config,
                "filter-length",     &filtlen,
                "noise-magnitude",   &noisemag,
                "effect-image",      &effect_image,
                "integration-steps", &intsteps,
                "min-value",         &minv,
                "max-value",         &maxv,
                NULL);
  effect_channel  = gimp_procedure_config_get_choice_id (config,
                                                         "effect-channel");
  effect_operator = gimp_procedure_config_get_choice_id (config,
                                                         "effect-operator");
  effect_convolve = gimp_procedure_config_get_choice_id (config,
                                                         "effect-convolve");

  if (! effect_image)
    return;

  /* Get some useful info on the input drawable */
  /* ========================================== */
  if (! gimp_drawable_mask_intersect (drawable,
                                      &border_x, &border_y,
                                      &border_w, &border_h))
    return;

  gimp_progress_init (_("Van Gogh (LIC)"));

  if (effect_convolve == 0)
    generatevectors ();

  if (filtlen < 0.1)
    filtlen = 0.1;

  l = filtlen;
  dx = dy = noisemag;
  minv = minv / 10.0;
  maxv = maxv / 10.0;
  isteps = intsteps;

  source_drw_has_alpha = gimp_drawable_has_alpha (drawable);

  effect_width =  gimp_drawable_get_width  (effect_image);
  effect_height = gimp_drawable_get_height (effect_image);

  switch (effect_channel)
    {
    case LIC_HUE:
      scalarfield = rgb_to_hsl (effect_image, LIC_HUE);
      break;
    case LIC_SATURATION:
      scalarfield = rgb_to_hsl (effect_image, LIC_SATURATION);
      break;
    case LIC_BRIGHTNESS:
      scalarfield = rgb_to_hsl (effect_image, LIC_BRIGHTNESS);
      break;
    }
  g_clear_object (&effect_image);

  compute_lic (config, drawable, scalarfield, effect_operator);

  g_free (scalarfield);

  /* Update image */
  /* ============ */

  gimp_drawable_merge_shadow (drawable, TRUE);
  gimp_drawable_update (drawable, border_x, border_y, border_w, border_h);

  gimp_displays_flush ();
}

/**************************/
/* Below is only UI stuff */
/**************************/

static gboolean
create_main_dialog (GimpProcedure       *procedure,
                    GimpProcedureConfig *config,
                    GimpDrawable        *drawable)
{
  GtkWidget *dialog;
  GtkWidget *chooser;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Van Gogh (LIC)"));

  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "filter-length", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "noise-magnitude", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "integration-steps", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "min-value", 1.0);
  gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                         "max-value", 1.0);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), NULL);

  gtk_widget_set_visible (dialog, TRUE);

  /* TODO: Currently we can't serialize GimpDrawable parameters, so this sets
   * the parameter to the current image as a default value */
  chooser = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                              "effect-image", G_TYPE_NONE);
  gimp_drawable_chooser_set_drawable (GIMP_DRAWABLE_CHOOSER (chooser),
                                      drawable);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
