/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-header-save"
#define PLUG_IN_BINARY "file-header"
#define PLUG_IN_ROLE   "gimp-file-header"


/* Declare some local functions.
 */
static void       query         (void);
static void       run           (const gchar      *name,
                                 gint              nparams,
                                 const GimpParam  *param,
                                 gint             *nreturn_vals,
                                 GimpParam       **return_vals);

static gboolean   save_image    (GFile            *file,
                                 gint32            image_ID,
                                 gint32            drawable_ID,
                                 GError          **error);

static gboolean   print         (GOutputStream    *output,
                                 GError          **error,
                                 const gchar      *format,
                                 ...) G_GNUC_PRINTF (3, 4);


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
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "saves files as C unsigned character array",
                          "FIXME: write help",
                          "Spencer Kimball & Peter Mattis",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          N_("C source code header"),
                          "INDEXED, RGB",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "text/x-chdr");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "h", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      gint32           image_ID;
      gint32           drawable_ID;
      GimpExportReturn export = GIMP_EXPORT_CANCEL;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "Header",
                                      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;

        default:
          break;
        }

      if (! save_image (g_file_new_for_uri (param[3].data.d_string),
                        image_ID, drawable_ID, &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

static gboolean
save_image (GFile   *file,
            gint32   image_ID,
            gint32   drawable_ID,
            GError **error)
{
  GeglBuffer    *buffer;
  const Babl    *format;
  GimpImageType  drawable_type;
  GOutputStream *output;
  gint           x, y, b, c;
  const gchar   *backslash = "\\\\";
  const gchar   *quote     = "\\\"";
  const gchar   *newline   = "\"\n\t\"";
  gchar          buf[4];
  guchar        *d         = NULL;
  guchar        *data      = NULL;
  guchar        *cmap;
  GCancellable  *cancellable;
  gint           colors;
  gint           width;
  gint           height;

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  drawable_type = gimp_drawable_type (drawable_ID);

  if (! print (output, error,
               "/*  GIMP header image file format (%s): %s  */\n\n",
               GIMP_RGB_IMAGE == drawable_type ? "RGB" : "INDEXED",
               gimp_file_get_utf8_name (file)) ||
      ! print (output, error,
               "static unsigned int width = %d;\n", width) ||
      ! print (output, error,
               "static unsigned int height = %d;\n\n", height) ||
      ! print (output, error,
               "/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */\n\n"))
    {
      goto fail;
    }

  switch (drawable_type)
    {
    case GIMP_RGB_IMAGE:
      if (! print (output, error,
                   "#define HEADER_PIXEL(data,pixel) {\\\n"
                   "pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \\\n"
                   "pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \\\n"
                   "pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \\\n"
                   "data += 4; \\\n}\n") ||
          ! print (output, error,
                   "static char *header_data =\n\t\""))
        {
          goto fail;
        }

      format = babl_format ("R'G'B' u8");

      data = g_new (guchar, width * babl_format_get_bytes_per_pixel (format));

      c = 0;
      for (y = 0; y < height; y++)
        {
          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                           format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (x = 0; x < width; x++)
            {
              d = data + x * babl_format_get_bytes_per_pixel (format);

              buf[0] = ((d[0] >> 2) & 0x3F) + 33;
              buf[1] = ((((d[0] & 0x3) << 4) | (d[1] >> 4)) & 0x3F) + 33;
              buf[2] = ((((d[1] & 0xF) << 2) | (d[2] >> 6)) & 0x3F) + 33;
              buf[3] = (d[2] & 0x3F) + 33;

              for (b = 0; b < 4; b++)
                {
                  if (buf[b] == '"')
                    {
                      if (! print (output, error, "%s", quote))
                        goto fail;
                    }
                  else if (buf[b] == '\\')
                    {
                      if (! print (output, error, "%s", backslash))
                        goto fail;
                    }
                  else
                    {
                      if (! print (output, error, "%c", buf[b]))
                        goto fail;
                    }
                }

              c++;
              if (c >= 16)
                {
                  if (! print (output, error, "%s", newline))
                    goto fail;

                  c = 0;
                }
            }
        }

      if (! print (output, error, "\";\n"))
        goto fail;
      break;

    case GIMP_INDEXED_IMAGE:
      if (! print (output, error,
                   "#define HEADER_PIXEL(data,pixel) {\\\n"
                   "pixel[0] = header_data_cmap[(unsigned char)data[0]][0]; \\\n"
                   "pixel[1] = header_data_cmap[(unsigned char)data[0]][1]; \\\n"
                   "pixel[2] = header_data_cmap[(unsigned char)data[0]][2]; \\\n"
                   "data ++; }\n\n"))
        {
          goto fail;
        }

      /* save colormap */
      cmap = gimp_image_get_colormap (image_ID, &colors);

      if (! print (output, error,
                   "static unsigned char header_data_cmap[256][3] = {") ||
          ! print (output, error,
                   "\n\t{%3d,%3d,%3d}",
                   (gint) cmap[0], (gint) cmap[1], (gint) cmap[2]))
        {
          goto fail;
        }

      for (c = 1; c < colors; c++)
        {
          if (! print (output, error,
                       ",\n\t{%3d,%3d,%3d}",
                       (gint) cmap[3 * c],
                       (gint) cmap[3 * c + 1],
                       (gint) cmap[3 * c + 2]))
            {
              goto fail;
            }
        }

      /* fill the rest */
      for ( ; c < 256; c++)
        {
          if (! print (output, error, ",\n\t{255,255,255}"))
            goto fail;
        }

      /* close bracket */
      if (! print (output, error, "\n\t};\n"))
        goto fail;

      g_free (cmap);

      /* save image */
      if (! print (output, error, "static unsigned char header_data[] = {\n\t"))
        goto fail;

      data = g_new (guchar, width * 1);

      c = 0;
      for (y = 0; y < height; y++)
        {
          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                           NULL, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (x = 0; x < width  -1; x++)
            {
              d = data + x * 1;

              if (! print (output, error, "%d,", (gint) d[0]))
                goto fail;

              c++;
              if (c >= 16)
                {
                  if (! print (output, error, "\n\t"))
                    goto fail;

                  c = 0;
                }
            }

          if (y != height - 1)
            {
              if (! print (output, error, "%d,\n\t", (gint) d[1]))
                goto fail;
            }
          else
            {
              if (! print (output, error, "%d\n\t", (gint) d[1]))
                goto fail;
            }

          c = 0; /* reset line counter */
        }

      if (! print (output, error, "};\n"))
        goto fail;
      break;

    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      goto fail;
    }

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_free (data);
  g_object_unref (output);
  g_object_unref (buffer);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);

  g_free (data);
  g_object_unref (output);
  g_object_unref (buffer);
  g_object_unref (cancellable);

  return FALSE;
}

static gboolean
print (GOutputStream  *output,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (output, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}
