/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* Value-Invert plug-in v1.1 by Adam D. Moss, adam@foxbox.org.  1999/02/27
 *
 * RGB<->HSV math optimizations by Mukund Sivaraman <muks@mukund.org>
 * makes the plug-in 2-3 times faster. Using gimp_pixel_rgns_process()
 * also makes memory management nicer. June 12, 2006.
 *
 * The plug-in only does v = 255 - v; for each pixel in the image, or
 * each entry in the colormap depending upon the type of image, where 'v'
 * is the value in HSV color model.
 *
 * The plug-in code is optimized towards this, in that it is not a full
 * RGB->HSV->RGB transform, but shortcuts many of the calculations to
 * effectively only do v = 255 - v. In fact, hue (0-360) is never
 * calculated. The shortcuts can be derived from running a set of r, g, b
 * values through the RGB->HSV transform and then from HSV->RGB and solving
 * out the redundant portions.
 *
 * The plug-in also uses integer code exclusively for the main transform.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-vinvert"


/* Declare local functions.
 */
static void   query              (void);
static void   run                (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);

static void   vinvert            (GimpDrawable     *drawable);
static void   vinvert_indexed    (gint32            image_ID);
static void   vinvert_render_row (const guchar     *src,
                                  guchar           *dest,
                                  gint              width,
                                  gint              bpp);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive"          },
    { GIMP_PDB_IMAGE,    "image",    "Input image (used for indexed images)" },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"                        }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Invert the brightness of each pixel"),
                          "This function takes an indexed/RGB image and "
                          "inverts its 'value' in HSV space.  The upshot of "
                          "this is that the color and saturation at any given "
                          "point remains the same, but its brightness is "
                          "effectively inverted.  Quite strange.  Sometimes "
                          "produces unpleasant color artifacts on images from "
                          "lossy sources (ie. JPEG).",
                          "Adam D. Moss (adam@foxbox.org), "
                          "Mukund Sivaraman <muks@mukund.org>",
                          "Adam D. Moss (adam@foxbox.org), "
                          "Mukund Sivaraman <muks@mukund.org>",
                          "27th March 1997, "
                          "12th June 2006",
                          N_("_Value Invert"),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Colors/Invert");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;

  INIT_I18N();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  if (strcmp (name, PLUG_IN_PROC) == 0)
    {
      GimpDrawable *drawable;
      gint32        image_ID;

      /*  Get the specified drawable  */

      drawable = gimp_drawable_get (param[2].data.d_drawable);
      image_ID = param[1].data.d_image;

      /*  Make sure that the drawable is indexed or RGB color  */

      if (gimp_drawable_is_rgb (drawable->drawable_id))
        {
          vinvert (drawable);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else if (gimp_drawable_is_indexed (drawable->drawable_id))
        {
          vinvert_indexed (image_ID);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_drawable_detach (drawable);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static void
vinvert (GimpDrawable *drawable)
{
  gint          x, y, width, height;
  gdouble       total, processed;
  gint          update;
  gint          channels;
  GimpPixelRgn  src_rgn, dest_rgn;
  guchar        *src_row, *dest_row;
  gint          i;
  gpointer      pr;

  if (! gimp_drawable_mask_intersect (drawable->drawable_id,
                                      &x, &y, &width, &height))
    return;

  gimp_progress_init (_("Value Invert"));

  channels = gimp_drawable_bpp (drawable->drawable_id);

  gimp_pixel_rgn_init (&src_rgn,  drawable, x, y, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x, y, width, height, TRUE, TRUE);

  total = width * height;
  processed = 0.0;

  for (pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn), update = 0;
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr), update++)
    {
      src_row  = src_rgn.data;
      dest_row = dest_rgn.data;

      for (i = 0; i < src_rgn.h; i++)
        {
          vinvert_render_row (src_row, dest_row, src_rgn.w, channels);

          src_row  += src_rgn.rowstride;
          dest_row += dest_rgn.rowstride;
        }

      processed += src_rgn.w * src_rgn.h;
      update %= 16;

      if (update == 0)
        gimp_progress_update (processed / total);
    }

  gimp_progress_update (1.0);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x, y, width, height);

}

static void
vinvert_indexed (gint32 image_ID)
{
  guchar *cmap;
  gint    ncols;

  cmap = gimp_image_get_colormap (image_ID, &ncols);

  g_return_if_fail (cmap != NULL);

  vinvert_render_row (cmap, cmap, ncols, 3);

  gimp_image_set_colormap (image_ID, cmap, ncols);

  g_free (cmap);
}

static void
vinvert_render_row (const guchar *src,
                    guchar       *dest,
                    gint          width, /* in pixels */
                    gint          bpp)
{
  gint j;
  gint r, g, b;
  gint value, value2, min;
  gint delta;

  for (j = 0; j < width; j++)
    {
      r = *src++;
      g = *src++;
      b = *src++;

      if (r > g)
        {
          value = MAX (r, b);
          min = MIN (g, b);
        }
      else
        {
          value = MAX (g, b);
          min = MIN (r, b);
        }

      delta = value - min;
      if ((value == 0) || (delta == 0))
        {
          r = 255 - value;
          g = 255 - value;
          b = 255 - value;
        }
      else
        {
          value2 = value / 2;

          if (r == value)
            {
              r = 255 - r;
              b = ((r * b) + value2) / value;
              g = ((r * g) + value2) / value;
            }
          else if (g == value)
            {
              g = 255 - g;
              r = ((g * r) + value2) / value;
              b = ((g * b) + value2) / value;
            }
          else
            {
              b = 255 - b;
              g = ((b * g) + value2) / value;
              r = ((b * r) + value2) / value;
            }
        }

      *dest++ = r;
      *dest++ = g;
      *dest++ = b;

      if (bpp == 4)
        *dest++ = *src++;
    }
}
