/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GIMP Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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
#include <stdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/* #define ICO_DBG */

#include "main.h"
#include "icoload.h"
#include "icosave.h"

#include "libgimp/stdplugins-intl.h"


static void   query (void);
static void   run   (const gchar      *name,
                     gint              nparams,
                     const GimpParam  *param,
                     gint             *nreturn_vals,
                     GimpParam       **return_vals);


GimpPlugInInfo PLUG_IN_INFO =
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
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,   "raw_filename", "The name entered"             }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw_filename", "The name entered" },
  };

  gimp_install_procedure ("file_ico_load",
                          "Loads files of Windows ICO file format",
                          "Loads files of Windows ICO file format",
                          "Christian Kreibich <christian@whoop.org>",
                          "Christian Kreibich <christian@whoop.org>",
                          "2002",
                          N_("Microsoft Windows icon"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime ("file_ico_load", "image/x-ico");
  gimp_register_magic_load_handler ("file_ico_load",
                                    "ico",
                                    "",
                                    "0,string,\\000\\001\\000\\000,0,string,\\000\\002\\000\\000");

  gimp_install_procedure ("file_ico_save",
                          "Saves files in Windows ICO file format",
                          "Saves files in Windows ICO file format",
                          "Christian Kreibich <christian@whoop.org>",
                          "Christian Kreibich <christian@whoop.org>",
                          "2002",
                          N_("Microsoft Windows icon"),
                          "INDEXEDA, GRAYA, RGBA",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime ("file_ico_save", "image/x-ico");
  gimp_register_save_handler ("file_ico_save", "ico", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  gint32             image_ID;
  gint32             drawable_ID;
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;

  INIT_I18N ();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_ico_load") == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 3)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          image_ID = LoadICO (param[1].data.d_string);

          if (image_ID != -1)
            {
              *nreturn_vals = 2;
              values[1].type         = GIMP_PDB_IMAGE;
              values[1].data.d_image = image_ID;
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else if (strcmp (name, "file_ico_save") == 0)
    {
      gchar *file_name;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      file_name   = param[3].data.d_string;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams < 5)
            status = GIMP_PDB_CALLING_ERROR;
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          status = SaveICO (file_name, image_ID);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}


guint8 *
ico_alloc_map (gint  width,
               gint  height,
               gint  bpp,
               gint *length)
{
  gint    len = 0;
  guint8 *map = NULL;

  switch (bpp)
    {
    case 1:
      if ((width % 32) == 0)
        len = (width * height / 8);
      else
        len = 4 * ((width/32 + 1) * height);
      break;

    case 4:
      if ((width % 8) == 0)
        len = (width * height / 2);
      else
        len = 4 * ((width/8 + 1) * height);
      break;

    case 8:
      if ((width % 4) == 0)
        len = width * height;
      else
        len = 4 * ((width/4 + 1) * height);
      break;

    default:
      len = width * height * (bpp/8);
    }

  *length = len;
  map = g_new0 (guint8, len);

  return map;
}


static gboolean
ico_cmap_contains_black (guchar *cmap,
                         gint    num_colors)
{
  gint i;

  for (i = 0; i < num_colors; i++)
    {
      if ((cmap[3*i] == 0)   &&
          (cmap[3*i+1] == 0) &&
          (cmap[3*i+2] == 0))
        {
          return TRUE;
        }
    }

  return FALSE;
}


void
ico_image_reduce_layer_bpp (guint32 layer,
                            gint    bpp)
{
  GimpPixelRgn  src_pixel_rgn, dst_pixel_rgn;
  gint32        tmp_image;
  gint32        tmp_layer;
  gint          w, h;
  guchar       *buffer;

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);

  if (bpp <= 8)
    {
      GimpDrawable *drawable = gimp_drawable_get (layer);
      GimpDrawable *tmp;

      buffer = g_new (guchar, w * h * 4);

      tmp_image = gimp_image_new (gimp_drawable_width (layer),
                                  gimp_drawable_height (layer),
                                  GIMP_RGB);

      tmp_layer = gimp_layer_new (tmp_image, "tmp", w, h,
                                  GIMP_RGBA_IMAGE, 100, GIMP_NORMAL_MODE);

      tmp = gimp_drawable_get (tmp_layer);

      gimp_pixel_rgn_init (&src_pixel_rgn, drawable,  0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pixel_rgn, tmp, 0, 0, w, h, TRUE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
      gimp_image_add_layer (tmp_image, tmp_layer, 0);

      gimp_image_convert_indexed (tmp_image,
                                  GIMP_FS_DITHER,
                                  GIMP_MAKE_PALETTE,
                                  1 << bpp,
                                  TRUE,
                                  FALSE,
                                  "dummy");

      gimp_image_convert_rgb (tmp_image);

      gimp_pixel_rgn_init (&src_pixel_rgn, tmp, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pixel_rgn, drawable,  0, 0, w, h, TRUE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);

      gimp_drawable_detach (tmp);
      gimp_image_delete (tmp_image);

      gimp_drawable_detach (drawable);

      g_free (buffer);
    }
}


void
ico_image_get_reduced_buf (guint32   layer,
                           gint      bpp,
                           gint     *num_colors,
                           guchar  **cmap_out,
                           guchar  **buf_out)
{
  GimpPixelRgn    src_pixel_rgn, dst_pixel_rgn;
  gint32          tmp_image;
  gint32          tmp_layer;
  gint            w, h;
  guchar         *buffer;
  guchar         *cmap;
  GimpDrawable   *drawable = gimp_drawable_get (layer);

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);
  *cmap_out = NULL;
  *num_colors = 0;

  buffer = g_new (guchar, w * h * 4);

  if (bpp <= 8)
    {
      GimpDrawable *tmp;

      tmp_image = gimp_image_new (gimp_drawable_width (layer),
                                  gimp_drawable_height (layer),
                                  GIMP_RGB);

      tmp_layer = gimp_layer_new (tmp_image, "tmp", w, h,
                                  gimp_drawable_type (layer),
                                  100, GIMP_NORMAL_MODE);

      tmp = gimp_drawable_get (tmp_layer);

      gimp_pixel_rgn_init (&src_pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_init (&dst_pixel_rgn, tmp,      0, 0, w, h, TRUE, FALSE);
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, buffer, 0, 0, w, h);
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
      gimp_drawable_flush (tmp);

      gimp_image_add_layer (tmp_image, tmp_layer, 0);

      gimp_image_convert_indexed (tmp_image,
                                  GIMP_FS_DITHER,
                                  GIMP_MAKE_PALETTE,
                                  1 << bpp,
                                  TRUE,
                                  FALSE,
                                  "dummy");

      cmap = gimp_image_get_colormap (tmp_image, num_colors);

      if (*num_colors == (1 << bpp) &&
          !ico_cmap_contains_black(cmap, *num_colors))
        {
          /* Damn. Windows icons with color maps need the color black.
             We need to eliminate one more color to make room for black: */

          gimp_image_convert_rgb (tmp_image);

          gimp_pixel_rgn_init (&dst_pixel_rgn, tmp, 0, 0, w, h, TRUE, FALSE);
          gimp_pixel_rgn_set_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
          gimp_drawable_flush (tmp);

          gimp_image_convert_indexed (tmp_image,
                                      GIMP_FS_DITHER,
                                      GIMP_MAKE_PALETTE,
                                      (1 << bpp) - 1,
                                      TRUE,
                                      FALSE,
                                      "dummy");

          cmap = gimp_image_get_colormap (tmp_image, num_colors);
          *cmap_out = g_memdup (cmap, *num_colors * 3);

          gimp_image_convert_rgb (tmp_image);
        }

      gimp_pixel_rgn_init (&dst_pixel_rgn, tmp, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);

      gimp_drawable_detach (tmp);
      gimp_image_delete (tmp_image);
    }
  else
    {
      gimp_pixel_rgn_init (&dst_pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
      gimp_pixel_rgn_get_rect (&dst_pixel_rgn, buffer, 0, 0, w, h);
    }

  gimp_drawable_detach (drawable);

  *buf_out = buffer;
}


static void
ico_free_color_item (gpointer data1,
                     gpointer data2,
                     gpointer data3)
{
  g_free (data1);

  /* Shut up warnings: */
  data2 = NULL;
  data3 = NULL;
}


gint
ico_get_layer_num_colors (gint32    layer,
                          gboolean *uses_alpha_levels)
{
  GimpPixelRgn    pixel_rgn;
  gint            x, y, w, h, alpha, num_colors = 0;
  guint32        *buffer = NULL, *color;
  GHashTable     *hash;
  GimpDrawable   *drawable = gimp_drawable_get (layer);

  w = gimp_drawable_width (layer);
  h = gimp_drawable_height (layer);
  buffer = g_new (gint32, w * h);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&pixel_rgn, (guchar*) buffer, 0, 0, w, h);

  gimp_drawable_detach (drawable);

  hash = g_hash_table_new (g_int_hash, g_int_equal);
  *uses_alpha_levels = FALSE;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
        color = g_new0 (guint32, 1);
        *color = buffer[y * w + x];
        alpha = ((guint8*) color)[3];

        if (alpha != 0 && alpha != 255)
          *uses_alpha_levels = TRUE;

        g_hash_table_insert (hash, color, color);
      }

  num_colors = g_hash_table_size (hash);

  g_hash_table_foreach (hash, ico_free_color_item, NULL);
  g_hash_table_destroy (hash);

  g_free (buffer);

  return num_colors;
}



void
ico_cleanup (MsIcon *ico)
{
  gint i;

  if (!ico)
    return;

  if (ico->fp)
    fclose (ico->fp);

  if (ico->icon_dir)
    g_free (ico->icon_dir);

  if (ico->icon_data)
    {
      for (i = 0; i < ico->icon_count; i++)
        {
          g_free (ico->icon_data[i].palette);
          g_free (ico->icon_data[i].xor_map);
          g_free (ico->icon_data[i].and_map);
        }
      g_free (ico->icon_data);
    }
}
