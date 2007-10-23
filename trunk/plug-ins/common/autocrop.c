/*
 * Autocrop plug-in version 1.00
 * by Tim Newsome <drz@froody.bloke.com>
 */

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

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define AUTOCROP_PROC       "plug-in-autocrop"
#define AUTOCROP_LAYER_PROC "plug-in-autocrop-layer"


/* Declare local functions. */
static void      query         (void);
static void      run           (const gchar      *name,
                                gint              nparams,
                                const GimpParam  *param,
                                gint             *nreturn_vals,
                                GimpParam       **return_vals);

static gboolean  colors_equal  (const guchar *col1,
                                const guchar *col2,
                                gint          bytes);
static gint      guess_bgcolor (GimpPixelRgn *pr,
                                gint          width,
                                gint          height,
                                gint          bytes,
                                gboolean      has_alpha,
                                guchar       *color);

static void      autocrop      (GimpDrawable *drawable,
                                gint32        image_id,
                                gboolean      show_progress,
                                gboolean      layer_only);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,   /* quit_proc  */
  query,  /* query_proc */
  run,    /* run_proc   */
};

static gint bytes;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",    "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable", "Input drawable"               }
  };

  gimp_install_procedure (AUTOCROP_PROC,
                          N_("Remove empty borders from the image"),
                          "",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("Autocrop Imag_e"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (AUTOCROP_PROC, "<Image>/Image/Crop");

  gimp_install_procedure (AUTOCROP_LAYER_PROC,
                          N_("Remove empty borders from the layer"),
                          "",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("Autocrop Lay_er"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (AUTOCROP_LAYER_PROC, "<Image>/Layer/Crop");
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
  gboolean           interactive;

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;
  interactive = (run_mode != GIMP_RUN_NONINTERACTIVE);

  INIT_I18N();

  if (n_params != 3)
    {
      status = GIMP_PDB_CALLING_ERROR;
      goto out;
    }

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);
  image_id = param[1].data.d_image;

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_is_rgb (drawable->drawable_id) ||
      gimp_drawable_is_gray (drawable->drawable_id)  ||
      gimp_drawable_is_indexed (drawable->drawable_id))
    {
      if (interactive)
        gimp_progress_init (_("Cropping"));

      gimp_tile_cache_ntiles (MAX (drawable->width / gimp_tile_width (),
                                   drawable->height / gimp_tile_height ()) + 1);

      autocrop (drawable, image_id, interactive,
                strcmp (name, AUTOCROP_LAYER_PROC) == 0);

      if (interactive)
        gimp_displays_flush ();
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

 out:
  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

static void
autocrop (GimpDrawable *drawable,
          gint32        image_id,
          gboolean      show_progress,
          gboolean      layer_only)
{
  GimpPixelRgn  srcPR;
  gint          width, height;
  gint          off_x, off_y;
  gint          x1, y1, x2, y2, i;
  gboolean      abort;
  guchar       *buffer;
  guchar        color[4] = {0, 0, 0, 0};
  gint32        layer_id = 0;

  width  = drawable->width;
  height = drawable->height;
  bytes  = drawable->bpp;
  gimp_drawable_offsets (drawable->drawable_id, &off_x, &off_y);

  if (layer_only)
    {
      layer_id = gimp_image_get_active_layer (image_id);
      if (layer_id == -1)
        {
          gimp_drawable_detach (drawable);
          return;
        }
    }

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);

  /* First, let's figure out what exactly to crop. */
  buffer = g_malloc ((width > height ? width : height) * bytes);

  guess_bgcolor (&srcPR, width, height, bytes,
                 gimp_drawable_has_alpha (drawable->drawable_id),
                 color);

  /* Check how many of the top lines are uniform. */
  abort = FALSE;
  for (y1 = 0; y1 < height && !abort; y1++)
    {
      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, y1, width);

      for (i = 0; i < width && !abort; i++)
        abort = !colors_equal (color, buffer + i * bytes, bytes);
    }

  if (y1 == height && !abort)
    {
      /* whee - a plain color drawable. */
      x1 = 0;
      x2 = width;
      y1 = 0;
      y2 = height;
      goto done;
    }

  if (show_progress)
    gimp_progress_update (0.25);

  /* Check how many of the bottom lines are uniform. */
  abort = FALSE;
  for (y2 = height - 1; y2 >= 0 && !abort; y2--)
    {
      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, y2, width);

      for (i = 0; i < width && !abort; i++)
        abort = !colors_equal (color, buffer + i * bytes, bytes);
    }

  y2++; /* to make y2 - y1 == height */

  /* The coordinates are now the first rows which DON'T match
   * the color. Crop instead to one row larger:
   */
  if (y1 > 0)
    y1--;

  if (y2 < height)
    y2++;

  if (show_progress)
    gimp_progress_update (0.5);

  /* Check how many of the left lines are uniform. */
  abort = FALSE;
  for (x1 = 0; x1 < width && !abort; x1++)
    {
      gimp_pixel_rgn_get_col (&srcPR, buffer, x1, y1, y2 - y1);

      for (i = 0; i < y2 - y1 && !abort; i++)
        abort = !colors_equal (color, buffer + i * bytes, bytes);
    }

  if (show_progress)
    gimp_progress_update (0.75);

  /* Check how many of the right lines are uniform. */
  abort = FALSE;
  for (x2 = width - 1; x2 >= 0 && !abort; x2--)
    {
      gimp_pixel_rgn_get_col (&srcPR, buffer, x2, y1, y2 - y1);

      for (i = 0; i < y2 - y1 && !abort; i++)
        abort = !colors_equal (color, buffer + i * bytes, bytes);
    }

  x2++; /* to make x2 - x1 == width */

  /* The coordinates are now the first columns which DON'T match
   * the color. Crop instead to one column larger:
   */
  if (x1 > 0)
    x1--;

  if (x2 < width)
    x2++;

 done:
  g_free (buffer);
  gimp_drawable_detach (drawable);

  if (layer_only)
    {
      if (x2 - x1 != width || y2 - y1 != height)
        {
          gimp_layer_resize (layer_id, x2 - x1, y2 - y1, -x1, -y1);
        }
    }
  else
    {
      /* convert to image coordinates */
      x1 += off_x; x2 += off_x;
      y1 += off_y; y2 += off_y;

      gimp_image_undo_group_start (image_id);

      if (x1 < 0 || y1 < 0 ||
          x2 > gimp_image_width (image_id) ||
          y2 > gimp_image_height (image_id))
        {
          /*
           * partially outside the image area, we need to
           * resize the image to be able to crop properly.
           */
          gimp_image_resize (image_id, x2 - x1, y2 - y1, -x1, -y1);

          x2 -= x1;
          y2 -= y1;

          x1 = y1 = 0;
        }

      gimp_image_crop (image_id, x2 - x1, y2 - y1, x1, y1);

      gimp_image_undo_group_end (image_id);
    }

  if (show_progress)
    gimp_progress_update (1.0);
}

static gint
guess_bgcolor (GimpPixelRgn *pr,
               gint          width,
               gint          height,
               gint          bytes,
               gboolean      has_alpha,
               guchar       *color)
{
  guchar tl[4], tr[4], bl[4], br[4];

  gimp_pixel_rgn_get_pixel (pr, tl, 0, 0);
  gimp_pixel_rgn_get_pixel (pr, tr, width - 1, 0);
  gimp_pixel_rgn_get_pixel (pr, bl, 0, height - 1);
  gimp_pixel_rgn_get_pixel (pr, br, width - 1, height - 1);

  /* First check if there's transparency to crop. */
  if (has_alpha)
    {
      gint alpha = bytes - 1;

      if ((tl[alpha] == 0 && tr[alpha] == 0) ||
          (tl[alpha] == 0 && bl[alpha] == 0) ||
          (tr[alpha] == 0 && br[alpha] == 0) ||
          (bl[alpha] == 0 && br[alpha] == 0))
        {
          return 2;
        }
    }

  /* Algorithm pinched from pnmcrop.
   * To guess the background, first see if 3 corners are equal.
   * Then if two are equal. Otherwise average the colors.
   */

  if (colors_equal (tr, bl, bytes) && colors_equal (tr, br, bytes))
    {
      memcpy (color, tr, bytes);
      return 3;
    }
  else if (colors_equal (tl, bl, bytes) && colors_equal (tl, br, bytes))
    {
      memcpy (color, tl, bytes);
      return 3;
    }
  else if (colors_equal (tl, tr, bytes) && colors_equal (tl, br, bytes))
    {
      memcpy (color, tl, bytes);
      return 3;
    }
  else if (colors_equal (tl, tr, bytes) && colors_equal (tl, bl, bytes))
    {
      memcpy (color, tl, bytes);
      return 3;
    }
  else if (colors_equal (tl, tr, bytes) || colors_equal (tl, bl, bytes) ||
           colors_equal (tl, br, bytes))
    {
      memcpy (color, tl, bytes);
      return 2;
    }
  else if (colors_equal (tr, bl, bytes) || colors_equal (tr, bl, bytes))
    {
      memcpy (color, tr, bytes);
      return 2;
    }
  else if (colors_equal (br, bl, bytes))
    {
      memcpy (color, br, bytes);
      return 2;
    }
  else
    {
      while (bytes--)
        color[bytes] = (tl[bytes] + tr[bytes] + bl[bytes] + br[bytes]) / 4;

      return 0;
    }
}

static gboolean
colors_equal (const guchar 	*col1,
              const guchar	*col2,
              gint  		 bytes)
{
  gboolean equal = TRUE;
  gint b;

  if ((bytes == 2 || bytes == 4) &&     /* HACK! */
      col1[bytes-1] == 0 &&
      col2[bytes-1] == 0)
    {
      return TRUE;                      /* handle zero alpha */
    }

  for (b = 0; b < bytes; b++)
    {
      if (col1[b] != col2[b])
        {
          equal = FALSE;
          break;
        }
    }

  return equal;
}
