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
 *   export_image()              - Export the specified image to a PNG file.
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
#define EXPORT_PROC      "file-sgi-export"
#define PLUG_IN_BINARY   "file-sgi"
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
#define SGI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SGI_TYPE, Sgi))

GType                   sgi_get_type         (void) G_GNUC_CONST;

static GList          * sgi_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * sgi_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * sgi_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * sgi_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              GError               **error);
static gint             export_image         (GFile                 *file,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              GObject               *config,
                                              GError               **error);

static gboolean         save_dialog          (GimpProcedure         *procedure,
                                              GObject               *config,
                                              GimpImage             *image);


G_DEFINE_TYPE (Sgi, sgi, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (SGI_TYPE)
DEFINE_STD_SET_I18N


static void
sgi_class_init (SgiClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sgi_query_procedures;
  plug_in_class->create_procedure = sgi_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
  list = g_list_append (list, g_strdup (EXPORT_PROC));

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
                                     _("Silicon Graphics IRIS image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in SGI image file format"),
                                        _("This plug-in loads SGI image files."),
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
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, sgi_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure,
                                     _("Silicon Graphics IRIS image"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "SGI");
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_BRUSH);

      gimp_procedure_set_documentation (procedure,
                                        _("Exports files in SGI image file format"),
                                        _("This plug-in exports SGI image files."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-sgi");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "sgi,rgb,rgba,bw,icon");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      gimp_procedure_add_choice_argument (procedure, "compression",
                                          _("Compression _type"),
                                          _("Compression level"),
                                          gimp_choice_new_with_values ("none", SGI_COMP_NONE, _("No compression"),                        NULL,
                                                                       "rle",  SGI_COMP_RLE,  _("RLE compression"),                       NULL,
                                                                       "arle", SGI_COMP_ARLE, _("Aggressive RLE (not supported by SGI)"), NULL,
                                                                       NULL),
                                          "rle",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
sgi_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

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
sgi_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! export_image (file, image, drawables->data,
                          G_OBJECT (config), &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile   *file,
            GError **error)
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
  GimpPrecision  precision;
  const Babl    *bablfmt = NULL;

 /*
  * Open the file for reading...
  */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  sgip = sgiOpen (g_file_peek_path (file), SGI_READ, 0, 0, 0, 0, 0);

  if (! sgip)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading."),
                   gimp_file_get_utf8_name (file));
      free (sgip);
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
      free (sgip);
      return NULL;
    }

  if (sgip->ysize == 0 /*|| sgip->ysize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid height: %hu"), sgip->ysize);
      free (sgip);
      return NULL;
    }

  if (sgip->zsize == 0 /*|| sgip->zsize > GIMP_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid number of channels: %hu"), sgip->zsize);
      free (sgip);
      return NULL;
    }

  bytes = sgip->zsize;

  switch (sgip->zsize)
    {
    case 1 :    /* Grayscale */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("Y' u8");
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("Y' u16");
        }
      break;

    case 2 :    /* Grayscale + alpha */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("Y'A u8");
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("Y'A u16");
        }
      break;

    case 3 :    /* RGB */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B' u8");
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B' u16");
        }
      break;

    case 4 :    /* RGBA */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u8");
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u16");
        }
      break;

    default:
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      bytes = 4;
      if (sgip->bpp == 1)
        {
          precision = GIMP_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u8");
        }
      else
        {
          precision = GIMP_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u16");
        }
      break;
    }

  image = gimp_image_new_with_precision (sgip->xsize, sgip->ysize,
                                         image_type, precision);
  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not allocate new image: %s",
                   gimp_pdb_get_last_error (gimp_get_pdb ()));
      free (sgip);
      return NULL;
    }

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
  pixels[0]   = g_new (guchar, ((gsize) tile_height) * sgip->xsize * sgip->bpp * bytes);

  for (i = 1; i < tile_height; i ++)
    pixels[i] = pixels[0] + sgip->xsize * sgip->bpp * bytes * i;

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
                           bablfmt, pixels[0], GEGL_AUTO_ROWSTRIDE);

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

          guint16 *pixels16;

          for (x = 0, pixels16 = (guint16 *) pixels[count]; x < sgip->xsize; x ++)
            for (i = 0; i < bytes; i ++, pixels16 ++)
              *pixels16 = rows[i][x];
        }
    }

  /*
   * Do the last n rows (count always > 0)
   */

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y - count,
                                           sgip->xsize, count), 0,
                   bablfmt, pixels[0], GEGL_AUTO_ROWSTRIDE);

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
export_image (GFile        *file,
              GimpImage    *image,
              GimpDrawable *drawable,
              GObject      *config,
              GError      **error)
{
  gint         compression;
  gint         i, j;        /* Looping var */
  gint         x;           /* Current X coordinate */
  gint         y;           /* Current Y coordinate */
  gint         width;       /* Drawable width */
  gint         height;      /* Drawable height */
  gint         tile_height; /* Height of tile in GIMP */
  gint         count;       /* Count of rows to put in image */
  gint         zsize;       /* Number of channels in file */
  sgi_t       *sgip;        /* File pointer */
  GeglBuffer  *buffer;      /* Buffer for layer */
  const Babl  *format;
  guchar     **pixels;      /* Pixel rows */
  guchar      *pptr;        /* Current pixel */
  gushort    **rows;        /* SGI image data */

  compression =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "compression");

  /*
   * Get the drawable for the current image...
   */

  width  = gimp_drawable_get_width  (drawable);
  height = gimp_drawable_get_height (drawable);

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
                             gimp_file_get_utf8_name (file));

  sgip = sgiOpen (g_file_peek_path (file), SGI_WRITE, compression, 1,
                  width, height, zsize);

  if (! sgip)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for writing."),
                   gimp_file_get_utf8_name (file));
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
   * Export the image...
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
save_dialog (GimpProcedure *procedure,
             GObject       *config,
             GimpImage     *image)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  gboolean   run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "sgi-vbox", "compression", NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "sgi-vbox", NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
