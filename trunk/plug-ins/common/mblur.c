/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Motion Blur plug-in for GIMP 0.99
 * Copyright (C) 1997 Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz)
 *
 * This plug-in is port of Motion Blur plug-in for GIMP 0.54 by
 * Thorsten Martinsen
 *      Copyright (C) 1996 Torsten Martinsen <torsten@danbbs.dk>
 *      Bresenham algorithm stuff hacked from HP2xx written by
 *      Heinz W. Werntges
 *      Changes for version 1.11/1.12 Copyright (C) 1996 Federico Mena Quintero
 *      quartic@polloux.fciencias.unam.mx
 *
 * I also used some code from Whirl and Pinch plug-in by Federico Mena Quintero
 *      (federico@nuclecu.unam.mx)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Version 1.2
 *
 *      Everything is new - no changes
 *
 * TODO:
 *     Bilinear interpolation from original mblur for 0.54
 *     Speed all things up
 *              ? better caching scheme
 *              - while blurring along long trajectory do not average all
 *                pixels but average only few samples
 *     Function for weight of samples along trajectory
 *     Preview
 *     Support paths in GiMP 1.1 :-)
 *     Smash all bugs :-)
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-mblur"
#define PLUG_IN_BINARY  "mblur"
#define PLUG_IN_VERSION "Sep 1997, 1.2"


typedef enum
{
  MBLUR_LINEAR,
  MBLUR_RADIAL,
  MBLUR_ZOOM,
  MBLUR_MAX = MBLUR_ZOOM
} MBlurType;


typedef struct
{
  gint32    mblur_type;
  gint32    length;
  gint32    angle;
  gdouble   center_x;
  gdouble   center_y;
  gboolean  blur_outward;
  gboolean  preview;
} mblur_vals_t;

/***** Prototypes *****/

static void query (void);
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

static void             mblur        (GimpDrawable *drawable,
                                      GimpPreview  *preview);
static void             mblur_linear (GimpDrawable *drawable,
                                      GimpPreview  *preview,
                                      gint          x1,
                                      gint          y1,
                                      gint          width,
                                      gint          height);
static void             mblur_radial (GimpDrawable *drawable,
                                      GimpPreview  *preview,
                                      gint          x1,
                                      gint          y1,
                                      gint          width,
                                      gint          height);
static void             mblur_zoom   (GimpDrawable *drawable,
                                      GimpPreview  *preview,
                                      gint          x1,
                                      gint          y1,
                                      gint          width,
                                      gint          height);

static gboolean         mblur_dialog (gint32        image_ID,
                                      GimpDrawable *drawable);

/***** Variables *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static mblur_vals_t mbvals =
{
  MBLUR_LINEAR, /* mblur_type */
  5,            /* length     */
  45,           /* radius     */
  100000.0,     /* center_x   */
  100000.0,     /* center_y   */
  TRUE,         /* blur_outward */
  FALSE         /* preview    */
};


static GtkObject *length     = NULL;
static GtkObject *angle      = NULL;
static GtkWidget *center     = NULL;
static GtkWidget *dir_button = NULL;
static GtkWidget *preview    = NULL;

static gint       img_width, img_height, img_bpp;
static gboolean   has_alpha;

/***** Functions *****/

MAIN()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",  "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",     "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",  "Input drawable" },
    { GIMP_PDB_INT32,    "type",      "Type of motion blur (0 - linear, 1 - radial, 2 - zoom)" },
    { GIMP_PDB_INT32,     "length",    "Length" },
    { GIMP_PDB_INT32,     "angle",     "Angle" },
    { GIMP_PDB_FLOAT,     "center-x",     "Center X (optional)" },
    { GIMP_PDB_FLOAT,     "center-y",     "Center Y (optional)" },
    { GIMP_PDB_INT32,     "blur-outward", "For radial, 1 for outward, 0 for inward (optional)" },
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Simulate movement using directional blur"),
                          "This plug-in simulates the effect seen when "
                          "photographing a moving object at a slow shutter "
                          "speed. Done by adding multiple displaced copies.",
                          "Torsten Martinsen, Federico Mena Quintero and Daniel Skarda",
                          "Torsten Martinsen, Federico Mena Quintero and Daniel Skarda",
                          PLUG_IN_VERSION,
                          N_("_Motion Blur..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Blur");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status;
  GimpDrawable      *drawable;
  gint               x1, y1, x2, y2;

  INIT_I18N ();

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Get the active drawable info */

  drawable = gimp_drawable_get (param[2].data.d_drawable);

  img_width  = gimp_drawable_width (drawable->drawable_id);
  img_height = gimp_drawable_height (drawable->drawable_id);
  img_bpp    = gimp_drawable_bpp (drawable->drawable_id);

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  mbvals.center_x = (gdouble) (x1 + x2 - 1) / 2.0;
  mbvals.center_y = (gdouble) (y1 + y2 - 1) / 2.0;

  /* Set the tile cache size */
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &mbvals);

      /* Get information from the dialog */
      if (! mblur_dialog (param[1].data.d_image, drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /* Make sure all the arguments are present */
      if (nparams == 9)
        {
          mbvals.blur_outward = param[6].data.d_int32;
          --nparams;
        }

      if (nparams == 8)
        {
          mbvals.center_x = param[6].data.d_float;
          mbvals.center_y = param[7].data.d_float;
        }
      else if (nparams != 6)
        status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
        {
          mbvals.mblur_type = param[3].data.d_int32;
          mbvals.length     = param[4].data.d_int32;
          mbvals.angle      = param[5].data.d_int32;
        }

    if ((mbvals.mblur_type < 0) || (mbvals.mblur_type > MBLUR_MAX))
      status= GIMP_PDB_CALLING_ERROR;
    break;

    case GIMP_RUN_WITH_LAST_VALS:
      /* Possibly retrieve data */
      gimp_get_data (PLUG_IN_PROC, &mbvals);
      break;

    default:
      break;
    }

  /* Blur the image */

  if ((status == GIMP_PDB_SUCCESS) &&
      (gimp_drawable_is_rgb(drawable->drawable_id) ||
       gimp_drawable_is_gray(drawable->drawable_id)))
    {

      /* Run! */
      has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);
      mblur (drawable, NULL);

      /* If run mode is interactive, flush displays */
      if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush ();

      /* Store data */
      if (run_mode == GIMP_RUN_INTERACTIVE)
        gimp_set_data (PLUG_IN_PROC, &mbvals, sizeof (mblur_vals_t));
    }
  else if (status == GIMP_PDB_SUCCESS)
    status = GIMP_PDB_EXECUTION_ERROR;

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static void
mblur_linear (GimpDrawable *drawable,
              GimpPreview  *preview,
              gint          x1,
              gint          y1,
              gint          width,
              gint          height)
{
  GimpPixelRgn      dest_rgn;
  GimpPixelFetcher *pft;
  gpointer          pr;
  GimpRGB           background;

  guchar *dest;
  guchar *d;
  guchar  pixel[4];
  gint32  sum[4];
  gint    progress, max_progress;
  gint    c;
  gint    x, y, i, xx, yy, n;
  gint    dx, dy, px, py, swapdir, err, e, s1, s2;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_pixel_fetcher_set_bg_color (pft, &background);

  progress     = 0;
  max_progress = width * height;

  n = mbvals.length;
  px = (gdouble) n * cos (mbvals.angle / 180.0 * G_PI);
  py = (gdouble) n * sin (mbvals.angle / 180.0 * G_PI);

  /*
   * Initialization for Bresenham algorithm:
   * dx = abs(x2-x1), s1 = sign(x2-x1)
   * dy = abs(y2-y1), s2 = sign(y2-y1)
   */
  if ((dx = px) != 0)
    {
      if (dx < 0)
        {
          dx = -dx;
          s1 = -1;
        }
      else
        s1 = 1;
    }
  else
    s1 = 0;

  if ((dy = py) != 0)
    {
      if (dy < 0)
        {
          dy = -dy;
          s2 = -1;
        }
      else
        s2 = 1;
    }
  else
    s2 = 0;

  if (dy > dx)
    {
      swapdir = dx;
      dx = dy;
      dy = swapdir;
      swapdir = 1;
    }
  else
    swapdir = 0;

  dy *= 2;
  err = dy - dx;        /* Initial error term   */
  dx *= 2;

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < dest_rgn.y + dest_rgn.h; y++)
        {
          d = dest;

          for (x = dest_rgn.x; x < dest_rgn.x + dest_rgn.w; x++)
            {
              xx = x; yy = y; e = err;
              for (c = 0; c < img_bpp; c++)
                sum[c]= 0;

              for (i = 0; i < n; )
                {
                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, pixel);
                  if (has_alpha)
                    {
                      gint32 alpha = pixel[img_bpp-1];

                      sum[img_bpp-1] += alpha;
                      for (c = 0; c < img_bpp-1; c++)
                        sum[c] += pixel[c] * alpha;
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        sum[c] += pixel[c];
                    }
                  i++;

                  while (e >= 0 && dx)
                    {
                      if (swapdir)
                        xx += s1;
                      else
                        yy += s2;
                      e -= dx;
                    }

                  if (swapdir)
                    yy += s2;
                  else
                    xx += s1;

                  e += dy;

                  if ((xx < x1) || (xx >= x1 + width) ||
                      (yy < y1) || (yy >= y1 + height))
                    break;
                }

              if (i == 0)
                {
                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, d);
                }
              else
                {
                  if (has_alpha)
                    {
                      gint32 alpha = sum[img_bpp-1];

                      if ((d[img_bpp-1] = alpha/i) != 0)
                        {
                          for (c = 0; c < img_bpp-1; c++)
                            d[c] = sum[c] / alpha;
                        }
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        d[c] = sum[c] / i;
                    }
                }

              d += dest_rgn.bpp;
            }

          dest += dest_rgn.rowstride;
        }

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((gdouble) progress / max_progress);
        }
    }

  gimp_pixel_fetcher_destroy (pft);
}

static void
mblur_radial (GimpDrawable *drawable,
              GimpPreview  *preview,
              gint          x1,
              gint          y1,
              gint          width,
              gint          height)
{
  GimpPixelRgn      dest_rgn;
  GimpPixelFetcher *pft;
  gpointer          pr;
  GimpRGB           background;

  gdouble   center_x;
  gdouble   center_y;
  guchar   *dest;
  guchar   *d;
  guchar    pixel[4];
  gint32    sum[4];

  gint      progress, max_progress, c;

  gint      x, y, i, n, xr, yr;
  gint      count, R, r, w, h, step;
  gfloat    angle, theta, * ct, * st, offset, xx, yy;

  /* initialize */

  xx = 0.0;
  yy = 0.0;

  center_x = mbvals.center_x;
  center_y = mbvals.center_y;

  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_pixel_fetcher_set_bg_color (pft, &background);

  progress     = 0;
  max_progress = width * height;

  angle = gimp_deg_to_rad (mbvals.angle);

  w = MAX (img_width  - center_x, center_x);
  h = MAX (img_height - center_y, center_y);
  R = sqrt (w * w + h * h);
  n = 4 * angle * sqrt (R) + 2;
  theta = angle / ((float) (n - 1));

  ct = g_new (float, n);
  st = g_new (float, n);

  offset = theta * (n - 1) / 2;
  for (i = 0; i < n; ++i)
    {
      ct[i] = cos (theta * i - offset);
      st[i] = sin (theta * i - offset);
    }

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < dest_rgn.y + dest_rgn.h; y++)
        {
          d = dest;

          for (x = dest_rgn.x; x < dest_rgn.x + dest_rgn.w; x++)
            {
              xr = x - center_x;
              yr = y - center_y;
              r = sqrt (xr * xr + yr * yr);

              if (r == 0)
                step = 1;
              else if ((step = R / r) == 0)
                step = 1;
              else if (step > n-1)
                step = n-1;

              for (c = 0; c < img_bpp; c++)
                sum[c] = 0;

              for (i = 0, count = 0; i < n; i += step)
                {
                  xx = center_x + xr * ct[i] - yr * st[i];
                  yy = center_y + xr * st[i] + yr * ct[i];

                  if ((yy < y1) || (yy >= y1 + height) ||
                      (xx < x1) || (xx >= x1 + width))
                    continue;

                  ++count;
                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, pixel);

                  if (has_alpha)
                    {
                      gint32 alpha = pixel[img_bpp-1];

                      sum[img_bpp-1] += alpha;

                      for (c = 0; c < img_bpp-1; c++)
                        sum[c] += pixel[c] * alpha;
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        sum[c] += pixel[c];
                    }
                }

              if (count == 0)
                {
                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, d);
                }
              else
                {
                  if (has_alpha)
                    {
                      gint32 alpha = sum[img_bpp-1];

                      if ((d[img_bpp-1] = alpha/count) != 0)
                        {
                          for (c = 0; c < img_bpp-1; c++)
                            d[c] = sum[c] / alpha;
                        }
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        d[c] = sum[c] / count;
                    }
                }

              d += dest_rgn.bpp;
            }

          dest += dest_rgn.rowstride;
        }

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / max_progress);
        }
    }

  gimp_pixel_fetcher_destroy (pft);

  g_free (ct);
  g_free (st);
}

static void
mblur_zoom (GimpDrawable *drawable,
            GimpPreview  *preview,
            gint          x1,
            gint          y1,
            gint          width,
            gint          height)
{
  GimpPixelRgn      dest_rgn;
  GimpPixelFetcher *pft;
  gpointer          pr;
  GimpRGB           background;

  gdouble   center_x;
  gdouble   center_y;
  guchar   *dest, *d;
  guchar    pixel[4];
  gint32    sum[4];

  gint      progress, max_progress;
  gint      x, y, i, xx, yy, n, c;
  gfloat    f;
  gint      drawable_x1, drawable_y1;
  gint      drawable_x2, drawable_y2;

  /* initialize */

  xx = 0.0;
  yy = 0.0;
  center_x = mbvals.center_x;
  center_y = mbvals.center_y;

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &drawable_x1, &drawable_y1,
                             &drawable_x2, &drawable_y2);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
                       x1, y1, width, height, (preview == NULL), TRUE);

  pft = gimp_pixel_fetcher_new (drawable, FALSE);

  gimp_context_get_background (&background);
  gimp_pixel_fetcher_set_bg_color (pft, &background);

  progress     = 0;
  max_progress = width * height;

  n = mbvals.length;
  f = 0.02;

  for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      dest = dest_rgn.data;

      for (y = dest_rgn.y; y < dest_rgn.y + dest_rgn.h; y++)
        {
          d = dest;

          for (x = dest_rgn.x; x < dest_rgn.x + dest_rgn.w; x++)
            {
              for (c = 0; c < img_bpp; c++)
                sum[c] = 0;

              for (i = 0; i < n; ++i)
                {
                  if (mbvals.blur_outward)
                    {
                      xx = center_x + (x - center_x) * (1.0 - f * i);
                      yy = center_y + (y - center_y) * (1.0 - f * i);
                    }
                  else
                    {
                      xx = center_x + (x - center_x) * (1.0 + f * i);
                      yy = center_y + (y - center_y) * (1.0 + f * i);
                    }

                  if ((yy < drawable_y1) || (yy >= drawable_y2) ||
                      (xx < drawable_x1) || (xx >= drawable_x2))
                    break;

                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, pixel);
                  if (has_alpha)
                    {
                      gint32 alpha = pixel[img_bpp-1];

                      sum[img_bpp-1] += alpha;

                      for (c = 0; c < img_bpp-1; c++)
                        sum[c] += pixel[c] * alpha;
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        sum[c] += pixel[c];
                    }
                }

              if (i == 0)
                {
                  gimp_pixel_fetcher_get_pixel (pft, xx, yy, d);
                }
              else
                {
                  if (has_alpha)
                    {
                      gint32 alpha = sum[img_bpp-1];

                      if ((d[img_bpp-1] = alpha/i) != 0)
                        {
                          for (c = 0; c < img_bpp-1; c++)
                            d[c] = sum[c] / alpha;
                        }
                    }
                  else
                    {
                      for (c = 0; c < img_bpp; c++)
                        d[c] = sum[c] / i;
                    }
                }

              d += dest_rgn.bpp;
            }

          dest += dest_rgn.rowstride;
        }

      if (preview)
        {
          gimp_drawable_preview_draw_region (GIMP_DRAWABLE_PREVIEW (preview),
                                             &dest_rgn);
        }
      else
        {
          progress += dest_rgn.w * dest_rgn.h;
          gimp_progress_update ((double) progress / max_progress);
        }
    }

  gimp_pixel_fetcher_destroy (pft);
}

static void
mblur (GimpDrawable *drawable,
       GimpPreview  *preview)
{
  gint x, y;
  gint width, height;

  if (preview)
    {
      gimp_preview_get_position (preview, &x, &y);
      gimp_preview_get_size (preview, &width, &height);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &x, &y, &width, &height);
      width  -= x;
      height -= y;
    }

  if (width < 1 || height < 1)
    return;

  if (! preview)
    gimp_progress_init (_("Motion blurring"));

  switch (mbvals.mblur_type)
    {
    case MBLUR_LINEAR:
      mblur_linear (drawable, preview, x, y, width, height);
      break;

    case MBLUR_RADIAL:
      mblur_radial (drawable, preview, x, y, width, height);
      break;

    case MBLUR_ZOOM:
      mblur_zoom (drawable, preview, x, y, width, height);
      break;

    default:
      break;
    }

  if (! preview)
    {
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x, y, width, height);
    }
}

/****************************************
 *                 UI
 ****************************************/

static void
mblur_set_sensitivity (void)
{
  if (!length || !angle)
    return;                     /* Not initialized yet */

  switch (mbvals.mblur_type)
    {
    case MBLUR_LINEAR:
      gimp_scale_entry_set_sensitive (length, TRUE);
      gimp_scale_entry_set_sensitive (angle, TRUE);
      gtk_widget_set_sensitive (center, FALSE);
      gtk_widget_set_sensitive (dir_button, FALSE);
      break;

    case MBLUR_RADIAL:
      gimp_scale_entry_set_sensitive (length, FALSE);
      gimp_scale_entry_set_sensitive (angle, TRUE);
      gtk_widget_set_sensitive (center, TRUE);
      gtk_widget_set_sensitive (dir_button, FALSE);
      break;

    case MBLUR_ZOOM:
      gimp_scale_entry_set_sensitive (length, TRUE);
      gimp_scale_entry_set_sensitive (angle, FALSE);
      gtk_widget_set_sensitive (center, TRUE);
      gtk_widget_set_sensitive (dir_button, TRUE);
      break;

    default:
      break;
    }
}

static void
mblur_radio_button_update (GtkWidget *widget,
                           gpointer   data)
{
  gimp_radio_button_update (widget, data);
  mblur_set_sensitivity ();
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void
mblur_center_update (GimpSizeEntry *entry)
{
  mbvals.center_x = gimp_size_entry_get_refval (entry, 0);
  mbvals.center_y = gimp_size_entry_get_refval (entry, 1);
}

static gboolean
mblur_dialog (gint32        image_ID,
              GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *spinbutton;
  GtkWidget *label;
  GtkWidget *button;
  GtkObject *adj;
  gdouble    xres, yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Motion Blur"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, &mbvals.preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (mblur),
                            drawable);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gimp_int_radio_group_new (TRUE, _("Blur Type"),
                                    G_CALLBACK (mblur_radio_button_update),
                                    &mbvals.mblur_type, mbvals.mblur_type,

                                    _("_Linear"), MBLUR_LINEAR, NULL,
                                    _("_Radial"), MBLUR_RADIAL, NULL,
                                    _("_Zoom"),   MBLUR_ZOOM,   NULL,

                                    NULL);

  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  center = gimp_frame_new (_("Blur Center"));
  gtk_box_pack_start (GTK_BOX (hbox), center, FALSE, FALSE, 0);
  gtk_widget_show (center);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (center), vbox);
  gtk_widget_show (vbox);

  gimp_image_get_resolution (image_ID, &xres, &yres);

  entry = gimp_size_entry_new (1,
                               GIMP_UNIT_PIXEL, "%a",
                               TRUE, FALSE, FALSE, 5,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 0, 6);
  gtk_table_set_col_spacing (GTK_TABLE (entry), 2, 6);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (mblur_center_update),
                    NULL);
  g_signal_connect_swapped (entry, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  spinbutton = gimp_spin_button_new (&adj, 1, 0, 1, 1, 10, 1, 1, 2);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, mbvals.center_x);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_X:"), 0, 0, 0.0);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1, yres, TRUE);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, mbvals.center_y);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                        _("_Y:"), 1, 0, 0.0);

  button = gtk_check_button_new_with_mnemonic (_("Blur _outward"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                mbvals.blur_outward);
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &mbvals.blur_outward);
  g_signal_connect_swapped (button, "toggled",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  dir_button = button;

  frame = gimp_frame_new (_("Blur Parameters"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  length = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                                 _("L_ength:"), 150, 3,
                                 mbvals.length, 1.0, 256.0, 1.0, 8.0, 0,
                                 TRUE, 0, 0,
                                 NULL, NULL);

  g_signal_connect (length, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mbvals.length);
  g_signal_connect_swapped (length, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  angle = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
                                _("_Angle:"), 150, 3,
                                mbvals.angle, 0.0, 360.0, 1.0, 15.0, 0,
                                TRUE, 0, 0,
                                NULL, NULL);

  g_signal_connect (angle, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &mbvals.angle);
  g_signal_connect_swapped (angle, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  mblur_set_sensitivity ();

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
