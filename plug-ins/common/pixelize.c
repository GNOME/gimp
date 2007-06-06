/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Pixelize plug-in (ported to GIMP v1.0)
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 * original pixelize.c for GIMP 0.54 by Tracy Scott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * version 1.04
 * This version requires GIMP v0.99.10 or above.
 *
 * This plug-in "pixelizes" the image.
 *
 *	Eiichi Takamori <taka@ma1.seikyou.ne.jp>
 *	http://ha1.seikyou.ne.jp/home/taka/gimp/
 *
 * Changes from version 1.03 to version 1.04:
 * - Added gtk_rc_parse
 * - Added entry with scale
 * - Fixed bug that large pixelwidth >=64 sometimes caused core dump
 *
 * Changes from gimp-0.99.9/plug-ins/pixelize.c to version 1.03:
 * - Fixed comments and help strings
 * - Fixed `RGB, GRAY' to `RGB*, GRAY*'
 * - Fixed procedure db name `pixelize' to `plug_in_pixelize'
 *
 * Differences from Tracy Scott's original `pixelize' plug-in:
 *
 * - Algorithm is modified to work around with the tile management.
 *   The way of pixelizing is switched by the value of pixelwidth.  If
 *   pixelwidth is greater than (or equal to) tile width, then this
 *   plug-in makes GimpPixelRgn with that width and proceeds. Otherwise,
 *   it makes the region named `PixelArea', whose size is smaller than
 *   tile width and is multiply of pixel width, and acts onto it.
 */

/* pixelize filter written for GIMP
 *  -Tracy Scott
 *
 * This filter acts as a low pass filter on the color components of
 * the provided region
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Some useful macros */

#define PIXELIZE_PROC   "plug-in-pixelize"
#define PIXELIZE2_PROC  "plug-in-pixelize2"
#define PLUG_IN_BINARY  "pixelize"
#define TILE_CACHE_SIZE  16
#define SCALE_WIDTH     125
#define ENTRY_WIDTH       6

typedef struct
{
  gint pixelwidth;
  gint pixelheight;
} PixelizeValues;

typedef struct
{
  gint x, y, w, h;
  gint width;
  gint height;
  guchar *data;
} PixelArea;

/* Declare local functions.
 */
static void      query             (void);
static void      run               (const gchar      *name,
                                    gint              nparams,
                                    const GimpParam  *param,
                                    gint             *nreturn_vals,
                                    GimpParam       **return_vals);

static gboolean  pixelize_dialog   (GimpDrawable  *drawable);
static void      update_pixelsize  (GimpSizeEntry *sizeentry,
                                    GimpPreview   *preview);

static void      pixelize          (GimpDrawable  *drawable,
                                    GimpPreview   *preview);
static void      pixelize_large    (GimpDrawable  *drawable,
                                    gint           pixelwidth,
                                    gint           pixelheight,
                                    GimpPreview   *preview);
static void      pixelize_small    (GimpDrawable  *drawable,
                                    gint           pixelwidth,
                                    gint           pixelheight,
                                    gint           tile_width,
                                    gint           tile_height);
static void      pixelize_sub      (gint           pixelwidth,
                                    gint           pixelheight,
                                    gint           bpp,
                                    gint           has_alpha);

/***** Local vars *****/

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

static PixelizeValues pvals =
{
  10,
  10
};

static PixelArea area;

/***** Functions *****/

MAIN ()

static void
query (void)
{
  static const GimpParamDef pixelize_args[]=
  {
    { GIMP_PDB_INT32,    "run-mode",    "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",       "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable",    "Input drawable"               },
    { GIMP_PDB_INT32,    "pixel-width", "Pixel width (the decrease in resolution)" }
  };

  static const GimpParamDef pixelize2_args[]=
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image (unused)"         },
    { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable"               },
    { GIMP_PDB_INT32,    "pixel-width",  "Pixel width (the decrease in horizontal resolution)" },
    { GIMP_PDB_INT32,    "pixel-height", "Pixel height (the decrease in vertical resolution)" }
  };

  gimp_install_procedure (PIXELIZE_PROC,
                          N_("Simplify image into an array of solid-colored squares"),
                          "Pixelize the contents of the specified drawable "
                          "with speficied pixelizing width.",
                          "Spencer Kimball & Peter Mattis, Tracy Scott, "
                          "(ported to 1.0 by) Eiichi Takamori",
                          "Spencer Kimball & Peter Mattis, Tracy Scott",
                          "1995",
                          N_("_Pixelize..."),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (pixelize_args), 0,
                          pixelize_args, NULL);

  gimp_plugin_menu_register (PIXELIZE_PROC, "<Image>/Filters/Blur");

  gimp_install_procedure (PIXELIZE2_PROC,
                          "Pixelize the contents of the specified drawable",
                          "Pixelize the contents of the specified drawable "
                          "with speficied pixelizing width.",
                          "Spencer Kimball & Peter Mattis, Tracy Scott, "
                          "(ported to 1.0 by) Eiichi Takamori",
                          "Spencer Kimball & Peter Mattis, Tracy Scott",
                          "2001",
                          NULL,
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (pixelize2_args), 0,
                          pixelize2_args, NULL);
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

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data (PIXELIZE_PROC, &pvals);

      /*  First acquire information with a dialog  */
      if (! pixelize_dialog (drawable))
        {
          gimp_drawable_detach (drawable);
          return;
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if ((! strcmp (name, PIXELIZE_PROC) && nparams != 4) ||
          (! strcmp (name, PIXELIZE2_PROC) && nparams != 5))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          pvals.pixelwidth  = (gdouble) param[3].data.d_int32;

          if (nparams == 4)
            pvals.pixelheight = pvals.pixelwidth;
          else
            pvals.pixelheight = (gdouble) param[4].data.d_int32;
        }

      if ((status == GIMP_PDB_SUCCESS) &&
          (pvals.pixelwidth <= 0 || pvals.pixelheight <= 0))
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (PIXELIZE_PROC, &pvals);
      break;

    default:
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id))
        {
          gimp_progress_init (_("Pixelizing"));

          /*  set the tile cache size  */
          gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

          /*  run the pixelize effect  */
          pixelize (drawable, NULL);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*  Store data  */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PIXELIZE_PROC, &pvals, sizeof (PixelizeValues));
        }
      else
        {
          /* g_message ("pixelize: cannot operate on indexed color images"); */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gboolean
pixelize_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *sizeentry;
  guint32    image_id;
  GimpUnit   unit;
  gdouble    xres, yres;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Pixelize"), PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PIXELIZE_PROC,

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

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start_defaults (GTK_BOX (main_vbox), preview);
  gtk_widget_show (preview);
  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (pixelize),
                            drawable);

  image_id = gimp_drawable_get_image (drawable->drawable_id);
  unit = gimp_image_get_unit (image_id);
  gimp_image_get_resolution (image_id, &xres, &yres);

  sizeentry = gimp_coordinates_new (unit, "%a", TRUE, TRUE, ENTRY_WIDTH,
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE,
                                    TRUE, FALSE,

                                    _("Pixel _width:"),
                                    pvals.pixelwidth, xres,
                                    1, drawable->width,
                                    1, drawable->width,

                                    _("Pixel _height:"),
                                    pvals.pixelheight, yres,
                                    1, drawable->height,
                                    1, drawable->height);

  gtk_box_pack_start (GTK_BOX (main_vbox), sizeentry, FALSE, FALSE, 0);
  gtk_widget_show (sizeentry);
  g_signal_connect (sizeentry, "value-changed",
                    G_CALLBACK (update_pixelsize),
                    preview);
  g_signal_connect (sizeentry, "refval-changed",
                    G_CALLBACK (update_pixelsize),
                    preview);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
update_pixelsize (GimpSizeEntry *sizeentry,
                  GimpPreview   *preview)
{
  pvals.pixelwidth  = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry),
                                                  0);

  pvals.pixelheight = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (sizeentry),
                                                  1);
  gimp_preview_invalidate (preview);
}

/*
  Pixelize Effect
 */

static void
pixelize (GimpDrawable *drawable,
          GimpPreview  *preview)
{
  gint tile_width;
  gint tile_height;
  gint pixelwidth;
  gint pixelheight;

  tile_width  = gimp_tile_width ();
  tile_height = gimp_tile_height ();

  pixelwidth  = pvals.pixelwidth;
  pixelheight = pvals.pixelheight;

  if (pixelwidth < 0)
    pixelwidth = - pixelwidth;
  if (pixelwidth < 1)
    pixelwidth = 1;

  if (pixelheight < 0)
    pixelheight = - pixelheight;
  if (pixelheight < 1)
    pixelheight = 1;

  if (pixelwidth >= tile_width || pixelheight >= tile_height || preview)
    pixelize_large (drawable, pixelwidth, pixelheight, preview);
  else
    pixelize_small (drawable, pixelwidth, pixelheight, tile_width, tile_height);
}

/*
  This function operates on the image when pixelwidth >= tile_width.
  It simply sets the size of GimpPixelRgn as pixelwidth and proceeds.
 */
static void
pixelize_large (GimpDrawable *drawable,
                gint          pixelwidth,
                gint          pixelheight,
                GimpPreview  *preview)
{
  GimpPixelRgn  src_rgn, dest_rgn;
  guchar       *src_row, *dest_row;
  guchar       *src, *dest = NULL, *d;
  gulong        average[4];
  gint          row, col, b, bpp, has_alpha;
  gint          x, y, x_step, y_step;
  gint          i, j;
  gulong        count;
  gint          x1, y1, x2, y2;
  gint          width, height;
  gint          progress = 0, max_progress = 1;
  gpointer      pr;

  bpp = gimp_drawable_bpp (drawable->drawable_id);
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  if (preview)
    {
      gimp_preview_get_position (preview, &x1, &y1);
      gimp_preview_get_size (preview, &width, &height);

      x2 = x1 + width;
      y2 = y1 + height;
      dest = g_new (guchar, width * height * bpp);
    }
  else
    {
      gimp_drawable_mask_bounds (drawable->drawable_id,
                                 &x1, &y1, &x2, &y2);
      width  = x2 - x1;
      height = y2 - y1;

      /* Initialize progress */
      progress = 0;
      max_progress = 2 * width * height;
    }

  for (y = y1; y < y2; y += pixelheight - (y % pixelheight))
    {
      for (x = x1; x < x2; x += pixelwidth - (x % pixelwidth))
        {
          x_step = pixelwidth  - (x % pixelwidth);
          y_step = pixelheight - (y % pixelheight);
          x_step = MIN (x_step, x2 - x);
          y_step = MIN (y_step, y2 - y);

          gimp_pixel_rgn_init (&src_rgn, drawable,
                               x, y, x_step, y_step, FALSE, FALSE);
          for (b = 0; b < bpp; b++)
            average[b] = 0;
          count = 0;

          for (pr = gimp_pixel_rgns_register (1, &src_rgn);
               pr != NULL;
               pr = gimp_pixel_rgns_process (pr))
            {
              src_row = src_rgn.data;
              for (row = 0; row < src_rgn.h; row++)
                {
                  src = src_row;
                  if (has_alpha)
                    {
                      for (col = 0; col < src_rgn.w; col++)
                        {
                          gulong alpha = src[bpp - 1];

                          average[bpp - 1] += alpha;
                          for (b = 0; b < bpp - 1; b++)
                            average[b] += src[b] * alpha;
                          src += src_rgn.bpp;
                        }
                    }
                  else
                    {
                      for (col = 0; col < src_rgn.w; col++)
                        {
                          for (b = 0; b < bpp; b++)
                            average[b] += src[b];
                          src += src_rgn.bpp;
                        }
                    }
                  src_row += src_rgn.rowstride;
                }
              count += src_rgn.w * src_rgn.h;
              if (!preview)
                {
                  /* Update progress */
                  progress += src_rgn.w * src_rgn.h;
                  gimp_progress_update ((double) progress / (double) max_progress);
                }
            }

          if (count > 0)
            {
              if (has_alpha)
                {
                  gulong alpha = average[bpp - 1];

                  if ((average[bpp - 1] = alpha / count))
                    for (b = 0; b < bpp - 1; b++)
                      average[b] /= alpha;
                }
              else
                {
                  for (b = 0; b < bpp; b++)
                    average[b] /= count;
                }
            }

          if (preview)
            {
              dest_row = dest + ((y - y1) * width + (x - x1)) * bpp;
              for (j = 0; j < y_step; j++)
                {
                  d = dest_row;
                  for (i = 0; i < x_step; i++)
                    for (b = 0; b < bpp; b++)
                      *d++ = average[b];
                  dest_row += width * bpp;
                }
            }
          else
            {
              gimp_pixel_rgn_init (&dest_rgn, drawable,
                                   x, y, x_step, y_step,
                                   TRUE, TRUE);
              for (pr = gimp_pixel_rgns_register (1, &dest_rgn);
                   pr != NULL;
                   pr = gimp_pixel_rgns_process (pr))
                {
                  dest_row = dest_rgn.data;
                  for (row = 0; row < dest_rgn.h; row++)
                    {
                    dest = dest_row;
                    for (col = 0; col < dest_rgn.w; col++)
                      {
                        for (b = 0; b < bpp; b++)
                          dest[b] = average[b];

                        dest  += dest_rgn.bpp;
                      }
                    dest_row += dest_rgn.rowstride;
                  }
                /* Update progress */
                progress += dest_rgn.w * dest_rgn.h;
                gimp_progress_update ((double) progress / (double) max_progress);
              }
            }
        }
    }

  if (preview)
    {
      gimp_preview_draw_buffer (preview, dest, width * bpp);
      g_free (dest);
    }
  else
    {
      /*  update the blurred region	 */
      gimp_drawable_flush (drawable);
      gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
      gimp_drawable_update (drawable->drawable_id, x1, y1, width, height);
    }
}


/*
   This function operates on PixelArea, whose width and height are
   multiply of pixel width, and less than the tile size (to enhance
   its speed).

   If any coordinates of mask boundary is not multiply of pixel width
   (e.g.  x1 % pixelwidth != 0), operates on the region whose width
   or height is the remainder.
 */
static void
pixelize_small (GimpDrawable *drawable,
                gint          pixelwidth,
                gint          pixelheight,
                gint          tile_width,
                gint          tile_height)
{
  GimpPixelRgn src_rgn, dest_rgn;
  gint         bpp, has_alpha;
  gint         x1, y1, x2, y2;
  gint         progress, max_progress;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  bpp = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  area.width  = (tile_width  / pixelwidth)  * pixelwidth;
  area.height = (tile_height / pixelheight) * pixelheight;
  area.data= g_new (guchar, (glong) bpp * area.width * area.height);

  for (area.y = y1; area.y < y2;
       area.y += area.height - (area.y % area.height))
    {
      area.h = area.height - (area.y % area.height);
      area.h = MIN (area.h, y2 - area.y);

      for (area.x = x1; area.x < x2;
           area.x += area.width - (area.x % area.width))
        {
          area.w = area.width - (area.x % area.width);
          area.w = MIN(area.w, x2 - area.x);

          gimp_pixel_rgn_get_rect (&src_rgn, area.data,
                                   area.x, area.y, area.w, area.h);

          pixelize_sub (pixelwidth, pixelheight, bpp, has_alpha);

          gimp_pixel_rgn_set_rect (&dest_rgn, area.data,
                                   area.x, area.y, area.w, area.h);

          /* Update progress */
          progress += area.w * area.h;
          gimp_progress_update ((double) progress / (double) max_progress);
      }
    }

  g_free(area.data);

  /*  update the pixelized region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, (x2 - x1), (y2 - y1));
}

/*
  This function acts on one PixelArea.	Since there were so many
  nested FORs in pixelize_small(), I put a few of them here...
  */

static void
pixelize_sub (gint pixelwidth,
              gint pixelheight,
              gint bpp,
              gint has_alpha)
{
  gulong  average[4];      /* bpp <= 4 */
  gint    x, y, w, h;
  guchar *buf_row, *buf;
  gint    row, col;
  gint    rowstride;
  gint    count;
  gint    i;

  rowstride = area.w * bpp;

  for (y = area.y; y < area.y + area.h; y += pixelheight - (y % pixelheight))
    {
      h = pixelheight - (y % pixelheight);
      h = MIN (h, area.y + area.h - y);

      for (x = area.x; x < area.x + area.w; x += pixelwidth - (x % pixelwidth))
        {
          w = pixelwidth - (x % pixelwidth);
          w = MIN (w, area.x + area.w - x);

          for (i = 0; i < bpp; i++)
            average[i] = 0;
          count = 0;

          /* Read */
          buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

          for (row = 0; row < h; row++)
            {
              buf = buf_row;
              if (has_alpha)
                {
                  for (col = 0; col < w; col++)
                    {
                      gulong alpha = buf[bpp-1];

                      average[bpp-1] += alpha;
                      for (i = 0; i < bpp-1; i++)
                          average[i] += buf[i] * alpha;
                      buf += bpp;
                    }
                }
              else
                {
                  for (col = 0; col < w; col++)
                    {
                      for (i = 0; i < bpp; i++)
                          average[i] += buf[i];
                      buf += bpp;
                    }
                }
              buf_row += rowstride;
            }

          count += w*h;

          /* Average */
          if (count > 0)
            {
              if (has_alpha)
                {
                  gulong alpha = average[bpp-1];

                  if ((average[bpp-1] = alpha / count))
                    {
                      for (i = 0; i < bpp-1; i++)
                        average[i] /= alpha;
                    }
                }
              else
                {
                  for (i = 0; i < bpp; i++)
                    average[i] /= count;
                }
            }

          /* Write */
          buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

          for (row = 0; row < h; row++)
            {
              buf = buf_row;
              for (col = 0; col < w; col++)
                {
                  for (i = 0; i < bpp; i++)
                    buf[i] = average[i];

                  count++;
                  buf += bpp;
                }
              buf_row += rowstride;
            }
        }
    }
}
