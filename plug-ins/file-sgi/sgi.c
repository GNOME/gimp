/*
 * SGI image file plug-in for GIMP.
 *
 * Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   save_image()                - Export the specified image to a PNG file.
 *   save_ok_callback()          - Destroy the export dialog and export the image.
 *   save_dialog()               - Pop up the export dialog.
 *
 */

#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "sgi-lib.h"

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define LOAD_PROC        "file-sgi-load"
#define SAVE_PROC        "file-sgi-save"
#define PLUG_IN_BINARY   "file-sgi"
#define PLUG_IN_ROLE     "gimp-file-sgi"
#define PLUG_IN_VERSION  "1.1.1 - 17 May 1998"


/*
 * Local functions...
 */

static void     query       (void);
static void     run         (const gchar      *name,
                             gint              nparams,
                             const GimpParam  *param,
                             gint             *nreturn_vals,
                             GimpParam       **return_vals);

static gint32   load_image  (const gchar      *filename,
                             GError          **error);
static gint     save_image  (const gchar      *filename,
                             gint32            image_ID,
                             gint32            drawable_ID,
                             GError          **error);

static gboolean save_dialog (void);

/*
 * Globals...
 */

const GimpPlugInInfo  PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint  compression = SGI_COMP_RLE;


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,      "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING,     "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING,     "raw-filename", "The name of the file to load" },
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,      "image",        "Output image" },
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to export" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to export the image in" },
    { GIMP_PDB_INT32,    "compression",  "Compression level (0 = none, 1 = RLE, 2 = ARLE)" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in SGI image file format",
                          "This plug-in loads SGI image files.",
                          "Michael Sweet <mike@easysw.com>",
                          "Copyright 1997-1998 by Michael Sweet",
                          PLUG_IN_VERSION,
                          N_("Silicon Graphics IRIS image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args,
                          load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-sgi");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "sgi,rgb,rgba,bw,icon",
                                    "",
                                    "0,short,474");

  gimp_install_procedure (SAVE_PROC,
                          "Exports files in SGI image file format",
                          "This plug-in exports SGI image files.",
                          "Michael Sweet <mike@easysw.com>",
                          "Copyright 1997-1998 by Michael Sweet",
                          PLUG_IN_VERSION,
                          N_("Silicon Graphics IRIS image"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args),
                          0,
                          save_args,
                          NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-sgi");
  gimp_register_save_handler (SAVE_PROC, "sgi,rgb,rgba,bw,icon", "");
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
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

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

          export = gimp_export_image (&image_ID, &drawable_ID, "SGI",
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;
        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*
           * Possibly retrieve data...
           */
          gimp_get_data (SAVE_PROC, &compression);

          /*
           * Then acquire information with a dialog...
           */
          if (!save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*
           * Make sure all the arguments are there!
           */
          if (nparams != 6)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              compression = param[5].data.d_int32;

              if (compression < 0 || compression > 2)
                status = GIMP_PDB_CALLING_ERROR;
            };
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*
           * Possibly retrieve data...
           */
          gimp_get_data (SAVE_PROC, &compression);
          break;

        default:
          break;
        };

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (param[3].data.d_string, image_ID, drawable_ID,
                          &error))
            {
              gimp_set_data (SAVE_PROC, &compression, sizeof (compression));
            }
          else
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
 * 'load_image()' - Load a PNG image into a new image window.
 */

static gint32
load_image (const gchar  *filename,
            GError      **error)
{
  gint           i,           /* Looping var */
                 x,           /* Current X coordinate */
                 y,           /* Current Y coordinate */
                 image_type,  /* Type of image */
                 layer_type,  /* Type of drawable/layer */
                 tile_height, /* Height of tile in GIMP */
                 count,       /* Count of rows to put in image */
                 bytes;       /* Number of channels to use */
  sgi_t         *sgip;        /* File pointer */
  gint32         image,       /* Image */
                 layer;       /* Layer */
  GeglBuffer    *buffer;      /* Buffer for layer */
  guchar       **pixels,      /* Pixel rows */
                *pptr;        /* Current pixel */
  gushort      **rows;        /* SGI image data */

 /*
  * Open the file for reading...
  */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  sgip = sgiOpen (filename, SGI_READ, 0, 0, 0, 0, 0);
  if (sgip == NULL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading."),
                   gimp_filename_to_utf8 (filename));
      free (sgip);
      return -1;
    };

  /*
   * Get the image dimensions and create the image...
   */

  /* Sanitize dimensions (note that they are unsigned short and can
   * thus never be larger than GIMP_MAX_IMAGE_SIZE
   */
  if (sgip->xsize == 0 /*|| sgip->xsize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid width: %hu"), sgip->xsize);
      free (sgip);
      return -1;
    }

  if (sgip->ysize == 0 /*|| sgip->ysize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid height: %hu"), sgip->ysize);
      free (sgip);
      return -1;
    }

  if (sgip->zsize == 0 /*|| sgip->zsize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid number of channels: %hu"), sgip->zsize);
      free (sgip);
      return -1;
    }

  bytes = sgip->zsize;

  switch (sgip->zsize)
    {
    case 1 :    /* Grayscale */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      break;

    case 2 :    /* Grayscale + alpha */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      break;

    case 3 :    /* RGB */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      break;

    case 4 :    /* RGBA */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      break;

    default:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      bytes = 4;
      break;
    }

  image = gimp_image_new (sgip->xsize, sgip->ysize, image_type);
  if (image == -1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not allocate new image: %s",
                   gimp_get_pdb_error());
      free (sgip);
      return -1;
    }

  gimp_image_set_filename (image, filename);

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new (image, _("Background"), sgip->xsize, sgip->ysize,
                          layer_type,
                          100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, -1, 0);

  /*
   * Get the drawable and set the pixel region for our load...
   */

  buffer = gimp_drawable_get_buffer (layer);

  /*
   * Temporary buffers...
   */

  tile_height = gimp_tile_height ();
  pixels      = g_new (guchar *, tile_height);
  pixels[0]   = g_new (guchar, ((gsize) tile_height) * sgip->xsize * bytes);

  for (i = 1; i < tile_height; i ++)
    pixels[i] = pixels[0] + sgip->xsize * bytes * i;

  rows    = g_new (unsigned short *, sgip->zsize);
  rows[0] = g_new (unsigned short, ((gsize) sgip->xsize) * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Load the image...
   */

  for (y = 0, count = 0;
       y < sgip->ysize;
       y ++, count ++)
    {
      if (count >= tile_height)
        {
          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - count,
                                                   sgip->xsize, count), 0,
                           NULL, pixels[0], GEGL_AUTO_ROWSTRIDE);

          count = 0;

          gimp_progress_update ((double) y / (double) sgip->ysize);
        }

      for (i = 0; i < sgip->zsize; i ++)
        if (sgiGetRow (sgip, rows[i], sgip->ysize - 1 - y, i) < 0)
          g_printerr ("sgiGetRow(sgip, rows[i], %d, %d) failed!\n",
                      sgip->ysize - 1 - y, i);

      if (sgip->bpp == 1)
        {
          /*
           * 8-bit (unsigned) pixels...
           */

          for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
            for (i = 0; i < bytes; i ++, pptr ++)
              *pptr = rows[i][x];
        }
      else
        {
          /*
           * 16-bit (unsigned) pixels...
           */

          for (x = 0, pptr = pixels[count]; x < sgip->xsize; x ++)
            for (i = 0; i < bytes; i ++, pptr ++)
              *pptr = rows[i][x] >> 8;
        }
    }

  /*
   * Do the last n rows (count always > 0)
   */

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - count,
                                           sgip->xsize, count), 0,
                   NULL, pixels[0], GEGL_AUTO_ROWSTRIDE);

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixels[0]);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return image;
}


/*
 * 'save_image()' - Save the specified image to a SGI file.
 */

static gint
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            GError      **error)
{
  gint         i, j,        /* Looping var */
               x,           /* Current X coordinate */
               y,           /* Current Y coordinate */
               width,       /* Drawable width */
               height,      /* Drawable height */
               tile_height, /* Height of tile in GIMP */
               count,       /* Count of rows to put in image */
               zsize;       /* Number of channels in file */
  sgi_t       *sgip;        /* File pointer */
  GeglBuffer  *buffer;      /* Buffer for layer */
  const Babl  *format;
  guchar     **pixels,      /* Pixel rows */
              *pptr;        /* Current pixel */
  gushort    **rows;        /* SGI image data */

  /*
   * Get the drawable for the current image...
   */

  width  = gimp_drawable_width  (drawable_ID);
  height = gimp_drawable_height (drawable_ID);

  buffer = gimp_drawable_get_buffer (drawable_ID);

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_GRAY_IMAGE:
      zsize = 1;
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      zsize = 2;
      format = babl_format ("Y'A u8");
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_INDEXED_IMAGE:
      zsize = 3;
      format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      zsize = 4;
      break;

    default:
      return FALSE;
    }

  /*
   * Open the file for writing...
   */

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_filename_to_utf8 (filename));

  sgip = sgiOpen (filename, SGI_WRITE, compression, 1,
                  width, height, zsize);
  if (sgip == NULL)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for writing."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    };

  /*
   * Allocate memory for "tile_height" rows...
   */

  tile_height = gimp_tile_height ();
  pixels      = g_new (guchar *, tile_height);
  pixels[0]   = g_new (guchar, ((gsize) tile_height) * width * zsize);

  for (i = 1; i < tile_height; i ++)
    pixels[i]= pixels[0] + width * zsize * i;

  rows    = g_new (gushort *, sgip->zsize);
  rows[0] = g_new (gushort, ((gsize) sgip->xsize) * sgip->zsize);

  for (i = 1; i < sgip->zsize; i ++)
    rows[i] = rows[0] + i * sgip->xsize;

  /*
   * Save the image...
   */

  for (y = 0; y < height; y += count)
    {
      /*
       * Grab more pixel data...
       */

      if ((y + tile_height) >= height)
        count = height - y;
      else
        count = tile_height;

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, count), 1.0,
                       format, pixels[0],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      /*
       * Convert to shorts and write each color plane separately...
       */

      for (i = 0, pptr = pixels[0]; i < count; i ++)
        {
          for (x = 0; x < width; x ++)
            for (j = 0; j < zsize; j ++, pptr ++)
              rows[j][x] = *pptr;

          for (j = 0; j < zsize; j ++)
            sgiPutRow (sgip, rows[j], height - 1 - y - i, j);
        };

      gimp_progress_update ((double) y / (double) height);
    }

  /*
   * Done with the file...
   */

  sgiClose (sgip);

  g_free (pixels[0]);
  g_free (pixels);
  g_free (rows[0]);
  g_free (rows);

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("SGI"), PLUG_IN_BINARY, SAVE_PROC);

  frame = gimp_int_radio_group_new (TRUE, _("Compression type"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &compression, compression,

                                    _("_No compression"),
                                    SGI_COMP_NONE, NULL,
                                    _("_RLE compression"),
                                    SGI_COMP_RLE, NULL,
                                    _("_Aggressive RLE\n(not supported by SGI)"),
                                    SGI_COMP_ARLE, NULL,

                                    NULL);

  gtk_container_set_border_width (GTK_CONTAINER (frame), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
