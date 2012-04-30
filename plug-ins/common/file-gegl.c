/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-gegl.c -- GEGL based file format plug-in
 * Copyright (C) 2012 Simon Budig <simon@gimp.org>
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

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC "file-gegl-load"


static void     query             (void);
static void     run               (const gchar      *name,
                                   gint              nparams,
                                   const GimpParam  *param,
                                   gint             *nreturn_vals,
                                   GimpParam       **return_vals);
static gint32   load_image        (const gchar      *filename,
                                   GError          **error);

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query proc */
  run,   /* run_proc */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load." },
    { GIMP_PDB_STRING, "raw-filename", "The name entered" },
  };

  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads images using GEGL.",
                          "The GEGL image loader.",
                          "Simon Budig",
                          "Simon Budig",
                          "2012",
                          N_("image via GEGL"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

#warning need some EXR magic here.
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "exr,hdr",
                                    "",
                                    "");

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-exr");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               image_ID;
  GError            *error = NULL;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  gegl_init (NULL, NULL);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, &error);

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
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type           = GIMP_PDB_STRING;
      values[1].data.d_string  = error->message;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  gint32             image_ID = -1;
  gint32             layer_ID;
  GimpImageType      image_type;
  GimpImageBaseType  base_type;
  GimpPrecision      precision;
  gint               width;
  gint               height;
  GeglNode          *graph, *sink, *source;
  GeglBuffer        *src_buf = NULL, *dest_buf = NULL;
  const Babl        *format, *model, *type;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  graph = gegl_node_new ();
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer", &src_buf,
                              NULL);
  source = gegl_node_new_child (graph,
                                "operation", "gegl:load",
                                "path", filename,
                                NULL);

  gegl_node_connect_to (source, "output",
                        sink,   "input");

  gegl_node_process (sink);
  g_object_unref (graph);

  if (!src_buf)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s'"),
                   gimp_filename_to_utf8 (filename));
      return -1;
    }

  gimp_progress_update (80);

  width = gegl_buffer_get_width (src_buf);
  height = gegl_buffer_get_height (src_buf);
  format = gegl_buffer_get_format (src_buf);

  model = babl_format_get_model (format);

  if (model == babl_model ("Y")  ||
      model == babl_model ("Y'") ||
      model == babl_model ("YA") ||
      model == babl_model ("Y'A"))
    {
      base_type = GIMP_GRAY;
      if (babl_format_has_alpha (format))
        image_type = GIMP_GRAYA_IMAGE;
      else
        image_type = GIMP_GRAY_IMAGE;
    }
  else if (babl_format_is_palette (format))
    {
      base_type = GIMP_INDEXED;
      if (babl_format_has_alpha (format))
        image_type = GIMP_INDEXEDA_IMAGE;
      else
        image_type = GIMP_INDEXED_IMAGE;
    }
  else
    {
      base_type = GIMP_RGB;
      if (babl_format_has_alpha (format))
        image_type = GIMP_RGBA_IMAGE;
      else
        image_type = GIMP_RGB_IMAGE;
    }

  type = babl_format_get_type (format, 0);

  if (type == babl_type ("u8"))
    precision = GIMP_PRECISION_U8;
  else if (type == babl_type ("u16"))
    precision = GIMP_PRECISION_U16;
  else if (type == babl_type ("u32"))
    precision = GIMP_PRECISION_U32;
  else if (type == babl_type ("half"))
    precision = GIMP_PRECISION_HALF;
  else
    precision = GIMP_PRECISION_FLOAT;

  image_ID = gimp_image_new_with_precision (width, height,
                                            base_type, precision);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID,
                             _("Background"),
                             width, height,
                             image_type, 100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);
  dest_buf = gimp_drawable_get_buffer (layer_ID);

  gegl_buffer_copy (src_buf, NULL, dest_buf, NULL);

  g_object_unref (src_buf);
  g_object_unref (dest_buf);

  gimp_progress_update (100);

  return image_ID;
}

