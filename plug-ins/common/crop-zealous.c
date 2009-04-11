/*
 * ZealousCrop plug-in version 1.00
 * by Adam D. Moss <adam@foxbox.org>
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

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC "plug-in-zealouscrop"


/* Declare local functions. */
static void        query         (void);
static void        run           (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);

static inline gint colours_equal (const guchar     *col1,
                                  const guchar     *col2,
                                  gint              bytes);
static void        do_zcrop      (GimpDrawable     *drawable,
                                  gint32            image_id);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint bytes;


MAIN ()

static inline gint
colours_equal (const guchar *col1,
               const guchar *col2,
               gint          bytes)
{
  gint b;

  for (b = 0; b < bytes; b++)
    {
      if (col1[b] != col2[b])
        return FALSE;
    }

  return TRUE;
}

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"    },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Autocrop unused space from edges and middle"),
                          "",
                          "Adam D. Moss",
                          "Adam D. Moss",
                          "1997",
                          N_("_Zealous Crop"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Image/Crop");
}

static void
run (const gchar      *name,
     gint              n_params,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpDrawable      *drawable;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_id;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals  = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      if (n_params != 3)
        {
          status = GIMP_PDB_CALLING_ERROR;
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      /*  Get the specified drawable  */
      drawable = gimp_drawable_get(param[2].data.d_drawable);
      image_id = param[1].data.d_image;

      /*  Make sure that the drawable is gray or RGB or indexed  */
      if (gimp_drawable_is_rgb (drawable->drawable_id) ||
          gimp_drawable_is_gray (drawable->drawable_id) ||
          gimp_drawable_is_indexed (drawable->drawable_id))
        {
          gimp_progress_init (_("Zealous cropping"));

          gimp_tile_cache_ntiles (1 +
                                  2 * (drawable->width > drawable->height ?
                                       (drawable->width / gimp_tile_width()) :
                                       (drawable->height / gimp_tile_height())));

          do_zcrop(drawable, image_id);

          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          gimp_drawable_detach (drawable);
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
do_zcrop (GimpDrawable *drawable,
          gint32        image_id)
{
  GimpPixelRgn  srcPR, destPR;
  gint          width, height, x, y;
  guchar       *buffer;
  gint8        *killrows;
  gint8        *killcols;
  gint32        livingrows, livingcols, destrow, destcol;
  gint          total_area, area;

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;

  total_area = width * height * 4;
  area = 0;

  killrows = g_new (gint8, height);
  killcols = g_new (gint8, width);

  buffer = g_malloc ((width > height ? width : height) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  livingrows = 0;
  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, y, width);

      killrows[y] = TRUE;

      for (x = 0; x < width * bytes; x += bytes)
        {
          if (!colours_equal (buffer, &buffer[x], bytes))
            {
              livingrows++;
              killrows[y] = FALSE;
              break;
            }
        }

      area += width;
      if (y % 20 == 0)
        gimp_progress_update ((double) area / (double) total_area);
    }


  livingcols = 0;
  for (x = 0; x < width; x++)
    {
      gimp_pixel_rgn_get_col (&srcPR, buffer, x, 0, height);

      killcols[x] = TRUE;

      for (y = 0; y < height * bytes; y += bytes)
        {
          if (!colours_equal(buffer, &buffer[y], bytes))
            {
              livingcols++;
              killcols[x] = FALSE;
              break;
            }
        }

      area += height;
      if (x % 20 == 0)
        gimp_progress_update ((double) area / (double) total_area);
    }


  if ((livingcols == 0 || livingrows==0) ||
      (livingcols == width && livingrows == height))
    {
      g_message (_("Nothing to crop."));
      return;
    }

  destrow = 0;

  for (y = 0; y < height; y++)
    {
      if (!killrows[y])
        {
          gimp_pixel_rgn_get_row (&srcPR, buffer, 0, y, width);
          gimp_pixel_rgn_set_row (&destPR, buffer, 0, destrow, width);
          destrow++;
        }

      area += width;
      if (y % 20 == 0)
        gimp_progress_update ((double) area / (double) total_area);
    }


  destcol = 0;
  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, TRUE);

  for (x = 0; x < width; x++)
    {
      if (!killcols[x])
        {
          gimp_pixel_rgn_get_col (&srcPR, buffer, x, 0, height);
          gimp_pixel_rgn_set_col (&destPR, buffer, destcol, 0, height);
          destcol++;
        }

      area += height;
      if (x % 20 == 0)
        gimp_progress_update ((double) area / (double) total_area);
    }

  g_free (buffer);

  g_free (killrows);
  g_free (killcols);

  gimp_progress_update (1.00);

  gimp_image_undo_group_start (image_id);

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_image_crop (image_id, livingcols, livingrows, 0, 0);

  gimp_image_undo_group_end (image_id);
}
