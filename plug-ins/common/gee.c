/*
 * (c) Adam D. Moss : 1998-2000 : adam@gimp.org : adam@foxbox.org
 *
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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-the-slimy-egg"
#define PLUG_IN_BINARY "gee"

/* These aren't really redefinable, easily. */
#define IWIDTH  256
#define IHEIGHT 256


/* Declare local functions. */
static void       query (void);
static void       run   (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);

static void       do_fun                   (void);

static void       window_response_callback (GtkWidget *widget);
static gboolean   do_iteration             (void);

static void       render_frame             (void);
static void       init_preview_misc        (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/* Global widgets'n'stuff */
static guchar     *disp;      /* RGBX preview data      */
static guchar     *env;       /* src warping image data */
static guchar     *bump1base;
static guchar     *bump1;
static guchar     *bump2base;
static guchar     *bump2;
static guchar     *srcbump;
static guchar     *destbump;

static guint       idle_tag;
static GtkWidget  *drawing_area;

static gint32      image_id;
static GimpDrawable      *drawable;
static GimpImageBaseType  imagetype;
static guchar     *palette;
static gint        ncolours;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input Image"                          },
    { GIMP_PDB_DRAWABLE, "drawable", "Input Drawable"                       }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("A big hello from the GIMP team!"),
                          "Beyond help.",
                          "Adam D. Moss <adam@gimp.org>",
                          "Adam D. Moss <adam@gimp.org>",
                          "2000",
                          N_("Gee Slime"),
                          "RGB*, INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  if (run_mode == GIMP_RUN_NONINTERACTIVE ||
      n_params != 3)
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      image_id = param[1].data.d_image;
      drawable = gimp_drawable_get (param[2].data.d_drawable);

      if (drawable)
        do_fun ();
      else
        status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
build_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GtkWidget *frame;
  gchar     *tmp;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new (_("Gee Slime"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
                            NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("Thank You for Choosing GIMP"),
                                  GTK_RESPONSE_OK);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (window_response_callback),
                    NULL);

  tmp = g_strdup_printf (_("A less obsolete creation by %s"),
                         "Adam D. Moss / adam@gimp.org / adam@foxbox.org "
                         "/ 1998-2000");
  gimp_help_set_help_data (button, tmp, NULL);
  g_free (tmp);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, IWIDTH, IHEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), drawing_area);
  gtk_widget_show (drawing_area);

  gtk_widget_show (dialog);

  idle_tag = g_idle_add_full (G_PRIORITY_LOW,
                              (GSourceFunc) do_iteration,
                              NULL,
                              NULL);
}

/* #define LIGHT 0x19
#define LIGHT 0x1a
#define LIGHT 0x21 */
#define LIGHT 0x0d
static guchar llut[256];

static void
gen_llut (void)
{
  int i,k;

  for (i = 0; i < 256; i++)
    {
      k = i + ((LIGHT * ( /* (255*255)- */ i * i)) / (255 * 255));
      k = k + 8;
      k = (k > 255 ? 255 : k);
      llut[i] = k;
    }
}

static void
do_fun (void)
{
  imagetype = gimp_image_base_type (image_id);

  if (imagetype == GIMP_INDEXED)
    palette = gimp_image_get_colormap (image_id, &ncolours);
  else
    if (imagetype == GIMP_GRAY)
      {
        int i;
        palette = g_new (guchar, 256 * 3);
        for (i = 0; i < 256; i++)
          {
            palette[i * 3 + 0] =
            palette[i * 3 + 1] =
            palette[i * 3 + 2] = i;
          }
      }

  /* cache hint */
  gimp_tile_cache_ntiles (1);

  init_preview_misc ();
  build_dialog ();

  gen_llut ();

  render_frame ();

  gtk_main ();
}

static void
show (void)
{
  if (gtk_widget_is_drawable (drawing_area))
    {
      GtkStyle *style = gtk_widget_get_style (drawing_area);

      gdk_draw_rgb_32_image (gtk_widget_get_window (drawing_area),
                             style->white_gc,
                             0, 0, IWIDTH, IHEIGHT,
                             GDK_RGB_DITHER_NORMAL,
                             disp, IWIDTH * 4);
    }
}


/* Rendering Functions */

static void
bumpbob (int x, int y, int size)
{
  gint j;

  for (j = 0; j < size; j++)
    {
      int i;
      for (i = 0; i < size; i++)
        {
          int k;
#define BOB_INC 45
          k = destbump[i + x + (y + j) * IWIDTH] + BOB_INC;
          if (k & 256)
            destbump[i + x + (y + j) * IWIDTH] = 255;
          else
            destbump[i + x + (y + j) * IWIDTH] = k;
        }
    }
}

/* Adam's sillier algorithm. */
static void
iterate (void)
{
  static guint frame = 0;

  gint     i, j;
  gint     thisbump;
  guchar   sx, sy;
  guint32 *dest;
  guint32 *environment;
  guchar  *basebump;
  guint    basesx;
  guint    basesy;
  GRand   *gr;
#define bx (-(123-128))
#define by (128+123)
#define cx (by)
#define cy (-bx)
#define bycxmcybx (by*cx-cy*bx)
#define bx2 (((bx)<<19)/bycxmcybx)
#define by2 (((by)<<19)/bycxmcybx)
#define cx2 (((cx)<<19)/bycxmcybx)
#define cy2 (((cy)<<19)/bycxmcybx)

  gr = g_rand_new ();
  frame++;

  environment = (guint32*) env;
  dest = (guint32*) disp;
  srcbump  = (frame & 1) ? bump1 : bump2;
  destbump = (frame & 1) ? bump2 : bump1;

  /* A little sub-pixel jitter to liven things up. */
  basesx = (g_rand_int_range (gr, 0, 29 << 19) / bycxmcybx) +
            ((-128 -((128 * 256) / (cx + bx))) << 11);
  basesy = (g_rand_int_range (gr, 0, 29 << 19) / bycxmcybx) +
            ((-128 - ((128 * 256) / (cy + by))) << 11);

  basebump = srcbump;


  /* MELT DISTORTION MAP, APPLY IT */
  j = IHEIGHT;
  while (j--)
    {
      guint tx;
      guint ty;

      ty = (basesy += cx2);
      tx = (basesx -= bx2);

      i = IWIDTH;
      while (i--)
        {
          guchar *bptr =
            (srcbump + (
                        (
                         ((255 & (
                                  (tx >> 11)
                                 )))
                         |
                         ((((255 & (
                                    (ty >> 11)
                                    )) << 8)))
                         )
                        ));

          thisbump = (11 * *(basebump) + (
                                          *(bptr - IWIDTH) +
                                          *(bptr - 1) +
                                          *(bptr) +
                                          *(bptr + 1) +
                                          *(bptr + IWIDTH)
                                          )
                      );
          basebump++;

          tx += by2;
          ty -= cy2;

          /* TODO: Can accelerate search for non-zero bumps with
             casting an aligned long-word search. */
          if (thisbump == 0)
            {
              *(dest++) = *( environment + (i | (j << 8)));
              *(destbump++) = 0;
            }
          else
            {
              if (thisbump <= (131 << 4))
                {
                  thisbump >>= 4;
                  *destbump = thisbump;
                }
              else
                *destbump = thisbump = 131;

              sy = j + (((thisbump) - *(destbump + IWIDTH)));
              sx = i + (((thisbump) - *(++destbump)));
              *dest++ = *(environment + (sx | (sy << 8)));
            }
        }
    }

  srcbump  = (frame & 1) ? bump1 : bump2;
  destbump = (frame & 1) ? bump2 : bump1;
  dest = (guint32 *) disp;
  memset(destbump, 0, IWIDTH);

#if 1
  /*  CAUSTICS!  */
  /* The idea here is that we refract IWIDTH*IHEIGHT parallel rays
     through the surface of the slime and illuminate the points
     where they hit the backing-image.  There are some unrealistic
     shortcuts taken, but the result is quite pleasing.
  */
  j = IHEIGHT;
  while (j--)
    {
      i = IWIDTH;
      while (i--)
        {
          /* Apply caustics */
          sx = *(destbump++);
          if (sx != 0)
            {
              guchar *cptr;

              sy = j + (((sx) - *(destbump + IWIDTH - 1)));
              sx = i + (((sx) - *(destbump)));

              cptr = (guchar*)(dest + (0xffff ^ (sx | (sy << 8))));

              *cptr = llut[*cptr]; cptr++;
              *cptr = llut[*cptr]; cptr++;
              *cptr = llut[*cptr];
              /* this second point of light's offset (1 across, 1 down)
                 isn't really 'right' but it gives a more pleasing,
                 diffuse look. */
              cptr += 2 + IWIDTH * 4;

              *cptr = llut[*cptr]; cptr++;
              *cptr = llut[*cptr]; cptr++;
              *cptr = llut[*cptr];
            }
        }
    }
#endif


    /* Interactive bumpmap */
#define BOBSIZE 6
#define BOBSPREAD 40
#define BOBS_PER_FRAME 70
  destbump = (frame & 1) ? bump2 : bump1;
  {
    gint            rxp, ryp, posx, posy;
    GdkModifierType mask;
    gint            size, i;

    gdk_window_get_pointer (gtk_widget_get_window (drawing_area),
                            &rxp, &ryp, &mask);

    for (i = 0; i < BOBS_PER_FRAME; i++)
      {
        size = g_rand_int_range (gr, 1, BOBSIZE);

        posx = rxp + BOBSPREAD/2 -
          RINT (sqrt (g_rand_double_range (gr, 0, BOBSPREAD)) *
               g_rand_int_range (gr, 0, BOBSPREAD));
        posy = ryp + BOBSPREAD/2 -
          RINT (sqrt (g_rand_double_range (gr, 0, BOBSPREAD)) *
               g_rand_int_range (gr, 0, BOBSPREAD));

        if (! ((posx < 1) || (posx > IWIDTH-size) ||
               (posy < 1) || (posy > IHEIGHT-size)))
          bumpbob (posx, posy, size);
      }
  }
}

static void
render_frame (void)
{
  iterate ();
  show ();
}

static void
init_preview_misc (void)
{
  GimpPixelRgn pixel_rgn;
  gint         i;
  gboolean     has_alpha;

  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  env = g_new (guchar, 4 * IWIDTH * IHEIGHT * 2);
  disp = g_new (guchar, (IWIDTH + 2 + IWIDTH * IHEIGHT) * 4);
  bump1base = g_new (guchar, IWIDTH * IHEIGHT + IWIDTH + IWIDTH);
  bump2base = g_new (guchar, IWIDTH * IHEIGHT + IWIDTH + IWIDTH);

  bump1 = &bump1base[IWIDTH];
  bump2 = &bump2base[IWIDTH];

  if ((drawable->width < 256) || (drawable->height < 256))
    {
      for (i = 0; i < 256; i++)
        {
          if (i < drawable->height)
            {
              gimp_pixel_rgn_init (&pixel_rgn,
                                   drawable,
                                   drawable->width > 256 ?
                                         drawable->width / 2 - 128 : 0,
                                   (drawable->height > 256 ?
                                         drawable->height / 2 - 128 : 0 ) + i,
                                   MIN (256 , drawable->width),
                                   1,
                                   FALSE,
                                   FALSE);
              gimp_pixel_rgn_get_rect (&pixel_rgn,
                                       &env[(256 * i +
                                             (
                                              (
                                               drawable->width < 256 ?
                                                    (256 - drawable->width) / 2 : 0
                                              )
                                              +
                                              (
                                               drawable->height < 256 ?
                                                    (256 - drawable->height) / 2 : 0
                                               ) * 256
                                              )) *
                                           gimp_drawable_bpp (drawable->drawable_id)
                                       ],
                                       drawable->width > 256 ?
                                            (drawable->width / 2 - 128) : 0,
                                       (drawable->height > 256 ?
                                             (drawable->height / 2 - 128) : 0) + i,
                                       MIN (256, drawable->width),
                                       1);
            }
        }
    }
  else
    {
      gimp_pixel_rgn_init (&pixel_rgn,
                           drawable,
                           drawable->width  > 256 ? (drawable->width  / 2 - 128) : 0,
                           drawable->height > 256 ? (drawable->height / 2 - 128) : 0,
                           MIN (256, drawable->width),
                           MIN (256, drawable->height),
                           FALSE,
                           FALSE);
      gimp_pixel_rgn_get_rect (&pixel_rgn,
                               env,
                               drawable->width  > 256 ? (drawable->width  / 2 - 128) : 0,
                               drawable->height > 256 ? (drawable->height / 2 - 128) : 0,
                               MIN (256, drawable->width),
                               MIN (256, drawable->height));
    }

  gimp_drawable_detach (drawable);

  /* convert the image data of varying types into flat grey or rgb. */
  switch (imagetype)
    {
    case GIMP_GRAY:
    case GIMP_INDEXED:
      if (has_alpha)
        {
          for (i = IWIDTH * IHEIGHT; i > 0; i--)
            {
              env[4 * (i - 1) + 2] =
                ((palette[3 * (env[(i - 1) * 2]) + 2] *
                  env[(i - 1) * 2 + 1]) / 255)
                + ((255 - env[(i - 1) * 2 + 1]) *
                    ((i & 255) ^ (i >> 8))) / 255;
              env[4 * (i - 1) + 1] =
                ((palette[3 * (env[(i - 1) * 2]) + 1] *
                  env[(i - 1) * 2 + 1])/255)
                + ((255 - env[(i - 1) * 2 + 1]) *
                    ((i & 255) ^ (i >> 8))) / 255;
              env[4 * (i - 1) + 0] =
                ((palette[3 * (env[(i - 1) * 2]) + 0]
                  * env[(i - 1) * 2 + 1]) / 255)
                + ((255 - env[(i - 1) * 2 + 1])
                    * ((i & 255) ^ (i >> 8))) / 255;
            }
        }
      else
        {
          for (i = IWIDTH * IHEIGHT; i > 0; i--)
            {
              env[4 * (i - 1) + 2] = palette[3 * (env[i - 1]) + 2];
              env[4 * (i - 1) + 1] = palette[3 * (env[i - 1]) + 1];
              env[4 * (i - 1) + 0] = palette[3 * (env[i - 1]) + 0];
            }
        }
      break;

    case GIMP_RGB:
      if (has_alpha)
        {
          for (i = 0; i < IWIDTH * IHEIGHT; i++)
            {
              env[i * 4 + 2] =
                (env[i * 4 + 2] * env[i * 4 + 3]) / 255
                + ((255 - env[i * 4 + 3]) * ((i & 255) ^ (i >> 8))) / 255;
              env[i * 4 + 1] =
                (env[i * 4 + 1] * env[i * 4 + 3]) / 255
                + ((255 - env[i * 4 + 3]) * ((i & 255) ^ (i >> 8))) / 255;
              env[i * 4 + 0] =
                (env[i * 4 + 0] * env[i * 4 + 3]) / 255
                + ((255 - env[i * 4 + 3]) * ((i & 255) ^ (i >> 8))) / 255;
            }
        }
      else
        {
          for (i = IWIDTH * IHEIGHT; i > 0; i--)
            {
              env[4 * (i - 1) + 2] = env[(i - 1) * 3 + 2];
              env[4 * (i - 1) + 1] = env[(i - 1) * 3 + 1];
              env[4 * (i - 1) + 0] = env[(i - 1) * 3 + 0];
            }
        }
      break;

    default:
      break;
    }

  /* Finally, 180-degree flip the environmental image! */
  for (i = 0; i < IWIDTH * IHEIGHT / 2; i++)
    {
      guchar t;
      t = env[4 * i + 0];
      env[4 * i + 0] = env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 0];
      env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 0] = t;
      t = env[4 * i + 1];
      env[4 * i + 1] = env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 1];
      env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 1] = t;
      t = env[4 * i + 2];
      env[4 * i + 2] = env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 2];
      env[4 * (IWIDTH * IHEIGHT - (i + 1)) + 2] = t;
    }
}

static gboolean
do_iteration (void)
{
  render_frame ();
  show ();

  return TRUE;
}

static void
window_response_callback (GtkWidget *widget)
{
  g_source_remove (idle_tag);
  idle_tag = 0;

  gtk_widget_destroy (widget);
}
