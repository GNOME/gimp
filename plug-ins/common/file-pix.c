/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Alias|Wavefront pix/matte image reading and writing code
 * Copyright (C) 1997 Mike Taylor
 * (email: mtaylor@aw.sgi.com, WWW: http://reality.sgi.com/mtaylor)
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
 */

/* This plug-in was written using the online documentation from
 * Alias|Wavefront Inc's PowerAnimator product.
 *
 * Bug reports or suggestions should be e-mailed to mtaylor@aw.sgi.com
 */

/* Event history:
 * V 1.0, MT, 02-Jul-97: initial version of plug-in
 * V 1.1, MT, 04-Dec-97: added .als file extension
 */

/* Features
 *  - loads and exports
 *    - 24-bit (.pix)
 *    - 8-bit (.matte, .alpha, or .mask) images
 *
 * NOTE: pix and matte files do not support alpha channels or indexed
 *       color, so neither does this plug-in
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pix-load"
#define SAVE_PROC      "file-pix-save"
#define PLUG_IN_BINARY "file-pix"
#define PLUG_IN_ROLE   "gimp-file-pix"


/* #define PIX_DEBUG */

#ifdef PIX_DEBUG
#    define PIX_DEBUG_PRINT(a,b) g_printerr (a,b)
#else
#    define PIX_DEBUG_PRINT(a,b)
#endif


/**************
 * Prototypes *
 **************/

static void      query      (void);
static void      run        (const gchar      *name,
                             gint              nparams,
                             const GimpParam  *param,
                             gint             *nreturn_vals,
                             GimpParam       **return_vals);

static gint32    load_image (GFile           *file,
                             GError         **error);
static gboolean  save_image (GFile           *file,
                             gint32           image_ID,
                             gint32           drawable_ID,
                             GError         **error);

static gboolean  get_short  (GInputStream    *input,
                             guint16         *value,
                             GError         **error);
static gboolean  put_short  (GOutputStream   *output,
                             guint16          value,
                             GError         **error);


/******************
 * Implementation *
 ******************/

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
  /*
   * Description:
   *     Register the services provided by this plug-in
   */
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",      "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",      "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename",   "The name entered"            }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export"           },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to export the image in" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "loads files of the Alias|Wavefront Pix file format",
                          "loads files of the Alias|Wavefront Pix file format",
                          "Michael Taylor",
                          "Michael Taylor",
                          "1997",
                          N_("Alias Pix image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_uri (LOAD_PROC);
  gimp_register_load_handler (LOAD_PROC, "pix,matte,mask,alpha,als", "");

  gimp_install_procedure (SAVE_PROC,
                          "export file in the Alias|Wavefront pix/matte file format",
                          "export file in the Alias|Wavefront pix/matte file format",
                          "Michael Taylor",
                          "Michael Taylor",
                          "1997",
                          N_("Alias Pix image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "pix,matte,mask,alpha,als", "");
}

/*
 *  Description:
 *      perform registered plug-in function
 *
 *  Arguments:
 *      name         - name of the function to perform
 *      nparams      - number of parameters passed to the function
 *      param        - parameters passed to the function
 *      nreturn_vals - number of parameters returned by the function
 *      return_vals  - parameters returned by the function
 */

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;
  GError           *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      /* Perform the image load */
      image_ID = load_image (g_file_new_for_uri (param[1].data.d_string),
                             &error);

      if (image_ID != -1)
        {
          /* The image load was successful */
          *nreturn_vals = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          /* The image load failed */
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "PIX",
                                      GIMP_EXPORT_CAN_HANDLE_RGB  |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY |
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

      if (status == GIMP_PDB_SUCCESS)
        {
          if (! save_image (g_file_new_for_uri (param[3].data.d_string),
                            image_ID, drawable_ID,
                            &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
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


/*
 * Description:
 *     Reads a 16-bit integer from a file in such a way that the machine's
 *     byte order should not matter.
 */

static gboolean
get_short (GInputStream  *input,
           guint16       *value,
           GError       **error)
{
  guchar buf[2];
  gsize  bytes_read;

  if (! g_input_stream_read_all (input, buf, 2,
                                 &bytes_read, NULL, error) ||
      bytes_read != 2)
    {
      return FALSE;
    }

  if (value)
    *value = (buf[0] << 8) + buf[1];

  return TRUE;
}

/*
 * Description:
 *     Writes a 16-bit integer to a file in such a way that the machine's
 *     byte order should not matter.
 */

static gboolean
put_short (GOutputStream  *output,
           guint16         value,
           GError        **error)
{
  guchar buf[2];

  buf[0] = (value >> 8) & 0xFF;
  buf[1] = value & 0xFF;

  return g_output_stream_write_all (output, buf, 2, NULL, NULL, error);
}

/*
 *  Description:
 *      load the given image into gimp
 *
 *  Arguments:
 *      filename      - name on the file to read
 *
 *  Return Value:
 *      Image id for the loaded image
 *
 */

static gint32
load_image (GFile   *file,
            GError **error)
{
  GInputStream      *input;
  GeglBuffer        *buffer;
  GimpImageBaseType  imgtype;
  GimpImageType      gdtype;
  guchar            *dest;
  guchar            *dest_base;
  gint32             image_ID;
  gint32             layer_ID;
  gushort            width, height, depth;
  gint               i, j, tile_height, row;

  PIX_DEBUG_PRINT ("Opening file: %s\n", filename);

  gimp_progress_init_printf (_("Opening '%s'"),
                             g_file_get_parse_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return -1;

  /* Read header information */
  if (! get_short (input, &width,  error) ||
      ! get_short (input, &height, error) ||
      ! get_short (input, NULL,    error) || /* Discard obsolete field */
      ! get_short (input, NULL,    error) || /* Discard obsolete field */
      ! get_short (input, &depth,  error))
    {
      g_object_unref (input);
      return -1;
    }

  PIX_DEBUG_PRINT ("Width %hu\n",  width);
  PIX_DEBUG_PRINT ("Height %hu\n", height);

  if (depth == 8)
    {
      /* Loading a matte file */
      imgtype = GIMP_GRAY;
      gdtype  = GIMP_GRAY_IMAGE;
    }
  else if (depth == 24)
    {
      /* Loading an RGB file */
      imgtype = GIMP_RGB;
      gdtype  = GIMP_RGB_IMAGE;
    }
  else
    {
      /* Header is invalid */
      g_object_unref (input);
      return -1;
    }

  image_ID = gimp_image_new (width, height, imgtype);
  gimp_image_set_filename (image_ID, g_file_get_uri (file));

  layer_ID = gimp_layer_new (image_ID, _("Background"),
                             width, height,
                             gdtype,
                             100,
                             gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  buffer = gimp_drawable_get_buffer (layer_ID);

  tile_height = gimp_tile_height ();

  if (depth == 24)
    {
      /* Read a 24-bit Pix image */

      dest_base = dest = g_new (guchar, 3 * width * tile_height);

      for (i = 0; i < height;)
        {
          for (dest = dest_base, row = 0;
               row < tile_height && i < height;
               i++, row++)
            {
              guchar record[4];
              gsize  bytes_read;
              guchar count;

              /* Read a row of the image */
              j = 0;
              while (j < width)
                {
                  if (! g_input_stream_read_all (input, record, 4,
                                                 &bytes_read, NULL, error) ||
                      bytes_read != 4)
                    break;

                  for (count = 0; count < record[0]; ++count)
                    {
                      dest[0]   = record[3];
                      dest[1]   = record[2];
                      dest[2]   = record[1];
                      dest += 3;
                      j++;
                      if (j >= width)
                        break;
                    }
                }
            }

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - row, width, row), 0,
                           NULL, dest_base, GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update ((double) i / (double) height);
        }

      g_free (dest_base);
    }
  else
    {
      /* Read an 8-bit Matte image */

      dest_base = dest = g_new (guchar, width * tile_height);

      for (i = 0; i < height;)
        {
          for (dest = dest_base, row = 0;
               row < tile_height && i < height;
               i++, row++)
            {
              guchar record[2];
              gsize  bytes_read;
              guchar count;

              /* Read a row of the image */
              j = 0;
              while (j < width)
                {
                  if (! g_input_stream_read_all (input, record, 2,
                                                 &bytes_read, NULL, error) ||
                      bytes_read != 2)
                    break;

                  for (count = 0; count < record[0]; ++count)
                    {
                      dest[j]   = record[1];
                      j++;
                      if (j >= width)
                        break;
                    }
                }

              dest += width;
            }

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i - row, width, row), 0,
                           NULL, dest_base, GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update ((double) i / (double) height);
        }

      g_free (dest_base);
    }

  g_object_unref (buffer);
  g_object_unref (input);

  gimp_progress_update (1.0);

  return image_ID;
}

/*
 *  Description:
 *      save the given file out as an alias pix or matte file
 *
 *  Arguments:
 *      filename    - name of file to save to
 *      image_ID    - ID of image to save
 *      drawable_ID - current drawable
 */

static gboolean
save_image (GFile   *file,
            gint32   image_ID,
            gint32   drawable_ID,
            GError **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  const Babl    *format;
  GCancellable  *cancellable;
  gint           width;
  gint           height;
  gint           depth, i, j, row, tile_height, rectHeight;
  gboolean       savingColor = TRUE;
  guchar        *src;
  guchar        *src_base;

  gimp_progress_init_printf (_("Exporting '%s'"),
                             g_file_get_parse_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  /* Get info about image */
  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  savingColor = ! gimp_drawable_is_gray (drawable_ID);

  if (savingColor)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("Y' u8");

  depth = babl_format_get_bytes_per_pixel (format);

  /* Write the image header */
  PIX_DEBUG_PRINT ("Width %hu\n",  width);
  PIX_DEBUG_PRINT ("Height %hu\n", height);

  if (! put_short (output, width,  error) ||
      ! put_short (output, height, error) ||
      ! put_short (output, 0,      error) ||
      ! put_short (output, 0,      error))
    {
      cancellable = g_cancellable_new ();
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);

      g_object_unref (output);
      g_object_unref (buffer);
      return FALSE;
    }

  tile_height = gimp_tile_height ();
  src_base    = g_new (guchar, tile_height * width * depth);

  if (savingColor)
    {
      /* Writing a 24-bit Pix image */

      if (! put_short (output, 24, error))
        goto fail;

      for (i = 0; i < height;)
        {
          rectHeight = (tile_height < (height - i)) ?
                        tile_height : (height - i);

          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, rectHeight), 1.0,
                           format, src_base,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (src = src_base, row = 0;
               row < tile_height && i < height;
               i += 1, row += 1)
            {
              /* Write a row of the image */

              guchar record[4];

              record[0] = 1;
              record[3] = src[0];
              record[2] = src[1];
              record[1] = src[2];
              src += depth;
              for (j = 1; j < width; ++j)
                {
                  if ((record[3] != src[0]) ||
                      (record[2] != src[1]) ||
                      (record[1] != src[2]) ||
                      (record[0] == 255))
                    {
                      /* Write current RLE record and start a new one */

                      if (! g_output_stream_write_all (output, record, 4,
                                                       NULL, NULL, error))
                        {
                          goto fail;
                        }

                      record[0] = 1;
                      record[3] = src[0];
                      record[2] = src[1];
                      record[1] = src[2];
                    }
                  else
                    {
                      /* increment run length in current record */
                      record[0]++;
                    }
                  src += depth;
                }

              /* Write last record in row */

              if (! g_output_stream_write_all (output, record, 4,
                                               NULL, NULL, error))
                {
                  goto fail;
                }
            }

          gimp_progress_update ((double) i / (double) height);
        }
    }
  else
    {
      /* Writing a 8-bit Matte (Mask) image */

      if (! put_short (output, 8, error))
        goto fail;

      for (i = 0; i < height;)
        {
          rectHeight = (tile_height < (height - i)) ?
                        tile_height : (height - i);

          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, rectHeight), 1.0,
                           format, src_base,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (src = src_base, row = 0;
               row < tile_height && i < height;
               i += 1, row += 1)
            {
              /* Write a row of the image */

              guchar record[2];

              record[0] = 1;
              record[1] = src[0];
              src += depth;
              for (j = 1; j < width; ++j)
                {
                  if ((record[1] != src[0]) || (record[0] == 255))
                    {
                      /* Write current RLE record and start a new one */

                      if (! g_output_stream_write_all (output, record, 2,
                                                       NULL, NULL, error))
                        {
                          goto fail;
                        }

                      record[0] = 1;
                      record[1] = src[0];
                    }
                  else
                    {
                      /* increment run length in current record */
                      record[0] ++;
                    }
                  src += depth;
                }

              /* Write last record in row */

              if (! g_output_stream_write_all (output, record, 2,
                                               NULL, NULL, error))
                {
                  goto fail;
                }
            }

          gimp_progress_update ((double) i / (double) height);
        }
    }

  g_free (src_base);
  g_object_unref (output);
  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);
  g_object_unref (cancellable);

  g_free (src_base);
  g_object_unref (output);
  g_object_unref (buffer);

  return FALSE;
}
