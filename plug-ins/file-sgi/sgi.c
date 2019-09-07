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


#define LOAD_PROC        "file-sgi-load"
#define SAVE_PROC        "file-sgi-save"
#define PLUG_IN_BINARY   "file-sgi"
#define PLUG_IN_ROLE     "gimp-file-sgi"
#define PLUG_IN_VERSION  "1.1.1 - 17 May 1998"


typedef struct _Sgi      Sgi;
typedef struct _SgiClass SgiClass;

struct _Sgi
{
  GimpPlugIn      parent_instance;
};

struct _SgiClass
{
  GimpPlugInClass parent_class;
};


#define SGI_TYPE  (sgi_get_type ())
#define SGI (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SGI_TYPE, Sgi))

GType                   sgi_get_type         (void) G_GNUC_CONST;

static GList          * sgi_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * sgi_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * sgi_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * sgi_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static GimpImage      * load_image           (const gchar          *filename,
                                              GError              **error);
static gint             save_image           (const gchar          *filename,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GError              **error);

static gboolean         save_dialog          (void);


G_DEFINE_TYPE (Sgi, sgi, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SGI_TYPE)


static gint  compression = SGI_COMP_RLE;


static void
sgi_class_init (SgiClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sgi_query_procedures;
  plug_in_class->create_procedure = sgi_create_procedure;
}

static void
sgi_init (Sgi *sgi)
{
}

static GList *
sgi_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
sgi_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           sgi_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     N_("Silicon Graphics IRIS image"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in SGI image file format",
                                        "This plug-in loads SGI image files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-sgi");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "sgi,rgb,rgba,bw,icon");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,short,474");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           sgi_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure,
                                     N_("Silicon Graphics IRIS image"));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        "Exports files in SGI image file format",
                                        "This plug-in exports SGI image files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-sgi");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "sgi,rgb,rgba,bw,icon");

      GIMP_PROC_ARG_INT (procedure, "compression",
                         "Compression",
                         "Compression level (0 = none, 1 = RLE, 2 = ARLE)",
                         0, 2, 1,
                         G_PARAM_STATIC_STRINGS);
    }

  return procedure;
}

static GimpValueArray *
sgi_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  gchar          *filename;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  filename = g_file_get_path (file);

  image = load_image (filename, &error);

  g_free (filename);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
sgi_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY, FALSE);

      export = gimp_export_image (&image, &drawable, "SGI",
                                  GIMP_EXPORT_CAN_HANDLE_RGB     |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (SAVE_PROC, &compression);

      if (! save_dialog ())
        status = GIMP_PDB_CANCEL;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      compression = GIMP_VALUES_GET_INT (args, 0);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (SAVE_PROC, &compression);
      break;

    default:
      break;
    };

  if (status == GIMP_PDB_SUCCESS)
    {
      gchar *filename = g_file_get_path (file);

      if (save_image (filename, image, drawable,
                      &error))
        {
          gimp_set_data (SAVE_PROC, &compression, sizeof (compression));
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      g_free (filename);
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
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
  GimpImage     *image;       /* Image */
  GimpLayer     *layer;       /* Layer */
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
      return NULL;
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
      return NULL;
    }

  if (sgip->ysize == 0 /*|| sgip->ysize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid height: %hu"), sgip->ysize);
      return NULL;
    }

  if (sgip->zsize == 0 /*|| sgip->zsize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid number of channels: %hu"), sgip->zsize);
      return NULL;
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
  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not allocate new image: %s",
                   gimp_pdb_get_last_error (gimp_get_pdb ()));
      return NULL;
    }

  gimp_image_set_filename (image, filename);

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new (image, _("Background"), sgip->xsize, sgip->ysize,
                          layer_type,
                          100,
                          gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);

  /*
   * Get the drawable and set the pixel region for our load...
   */

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

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

static gint
save_image (const gchar  *filename,
            GimpImage    *image,
            GimpDrawable *drawable,
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

  width  = gimp_drawable_width  (drawable);
  height = gimp_drawable_height (drawable);

  buffer = gimp_drawable_get_buffer (drawable);

  switch (gimp_drawable_type (drawable))
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
