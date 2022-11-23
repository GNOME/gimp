/*
 * SGI image file plug-in for LIGMA.
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
 *   main()                      - Main entry - just call ligma_main()...
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "sgi-lib.h"

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC        "file-sgi-load"
#define SAVE_PROC        "file-sgi-save"
#define PLUG_IN_BINARY   "file-sgi"
#define PLUG_IN_VERSION  "1.1.1 - 17 May 1998"


typedef struct _Sgi      Sgi;
typedef struct _SgiClass SgiClass;

struct _Sgi
{
  LigmaPlugIn      parent_instance;
};

struct _SgiClass
{
  LigmaPlugInClass parent_class;
};


#define SGI_TYPE  (sgi_get_type ())
#define SGI (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SGI_TYPE, Sgi))

GType                   sgi_get_type         (void) G_GNUC_CONST;

static GList          * sgi_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * sgi_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * sgi_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * sgi_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile                *file,
                                              GError              **error);
static gint             save_image           (GFile                *file,
                                              LigmaImage            *image,
                                              LigmaDrawable         *drawable,
                                              GObject              *config,
                                              GError              **error);

static gboolean         save_dialog          (LigmaProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Sgi, sgi, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (SGI_TYPE)
DEFINE_STD_SET_I18N


static void
sgi_class_init (SgiClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = sgi_query_procedures;
  plug_in_class->create_procedure = sgi_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
sgi_init (Sgi *sgi)
{
}

static GList *
sgi_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
sgi_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           sgi_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure,
                                     N_("Silicon Graphics IRIS image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in SGI image file format",
                                        "This plug-in loads SGI image files.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-sgi");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "sgi,rgb,rgba,bw,icon");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,short,474");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           sgi_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure,
                                     N_("Silicon Graphics IRIS image"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_BRUSH);

      ligma_procedure_set_documentation (procedure,
                                        "Exports files in SGI image file format",
                                        "This plug-in exports SGI image files.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Michael Sweet <mike@easysw.com>",
                                      "Copyright 1997-1998 by Michael Sweet",
                                      PLUG_IN_VERSION);

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-sgi");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "sgi,rgb,rgba,bw,icon");

      LIGMA_PROC_ARG_INT (procedure, "compression",
                         "Compression",
                         "Compression level (0 = none, 1 = RLE, 2 = ARLE)",
                         0, 2, SGI_COMP_RLE,
                         LIGMA_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
sgi_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
sgi_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export = LIGMA_EXPORT_CANCEL;
  GError              *error  = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "SGI",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("SGI format does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0],
                        G_OBJECT (config), &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaImage *
load_image (GFile   *file,
            GError **error)
{
  gint           i,           /* Looping var */
                 x,           /* Current X coordinate */
                 y,           /* Current Y coordinate */
                 image_type,  /* Type of image */
                 layer_type,  /* Type of drawable/layer */
                 tile_height, /* Height of tile in LIGMA */
                 count,       /* Count of rows to put in image */
                 bytes;       /* Number of channels to use */
  sgi_t         *sgip;        /* File pointer */
  LigmaImage     *image;       /* Image */
  LigmaLayer     *layer;       /* Layer */
  GeglBuffer    *buffer;      /* Buffer for layer */
  guchar       **pixels,      /* Pixel rows */
                *pptr;        /* Current pixel */
  gushort      **rows;        /* SGI image data */
  LigmaPrecision  precision;
  const Babl    *bablfmt = NULL;

 /*
  * Open the file for reading...
  */

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  sgip = sgiOpen (g_file_peek_path (file), SGI_READ, 0, 0, 0, 0, 0);

  if (! sgip)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading."),
                   ligma_file_get_utf8_name (file));
      free (sgip);
      return NULL;
    };

  /*
   * Get the image dimensions and create the image...
   */

  /* Sanitize dimensions (note that they are unsigned short and can
   * thus never be larger than LIGMA_MAX_IMAGE_SIZE
   */
  if (sgip->xsize == 0 /*|| sgip->xsize > LIGMA_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid width: %hu"), sgip->xsize);
      free (sgip);
      return NULL;
    }

  if (sgip->ysize == 0 /*|| sgip->ysize > LIGMA_MAX_IMAGE_SIZE*/)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
              _("Invalid height: %hu"), sgip->ysize);
      free (sgip);
      return NULL;
    }

  if (sgip->zsize == 0 /*|| sgip->zsize > LIGMA_MAX_IMAGE_SIZE*/)
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
      image_type = LIGMA_GRAY;
      layer_type = LIGMA_GRAY_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("Y' u8");
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("Y' u16");
        }
      break;

    case 2 :    /* Grayscale + alpha */
      image_type = LIGMA_GRAY;
      layer_type = LIGMA_GRAYA_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("Y'A u8");
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("Y'A u16");
        }
      break;

    case 3 :    /* RGB */
      image_type = LIGMA_RGB;
      layer_type = LIGMA_RGB_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B' u8");
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B' u16");
        }
      break;

    case 4 :    /* RGBA */
      image_type = LIGMA_RGB;
      layer_type = LIGMA_RGBA_IMAGE;
      if (sgip->bpp == 1)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u8");
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u16");
        }
      break;

    default:
      image_type = LIGMA_RGB;
      layer_type = LIGMA_RGBA_IMAGE;
      bytes = 4;
      if (sgip->bpp == 1)
        {
          precision = LIGMA_PRECISION_U8_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u8");
        }
      else
        {
          precision = LIGMA_PRECISION_U16_NON_LINEAR;
          bablfmt = babl_format ("R'G'B'A u16");
        }
      break;
    }

  image = ligma_image_new_with_precision (sgip->xsize, sgip->ysize,
                                         image_type, precision);
  if (! image)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not allocate new image: %s",
                   ligma_pdb_get_last_error (ligma_get_pdb ()));
      free (sgip);
      return NULL;
    }

  ligma_image_set_file (image, file);

  /*
   * Create the "background" layer to hold the image...
   */

  layer = ligma_layer_new (image, _("Background"), sgip->xsize, sgip->ysize,
                          layer_type,
                          100,
                          ligma_image_get_default_new_layer_mode (image));
  ligma_image_insert_layer (image, layer, NULL, 0);

  /*
   * Get the drawable and set the pixel region for our load...
   */

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  /*
   * Temporary buffers...
   */

  tile_height = ligma_tile_height ();
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

          ligma_progress_update ((double) y / (double) sgip->ysize);
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

  ligma_progress_update (1.0);

  return image;
}

static gint
save_image (GFile        *file,
            LigmaImage    *image,
            LigmaDrawable *drawable,
            GObject      *config,
            GError      **error)
{
  gint         compression;
  gint         i, j;        /* Looping var */
  gint         x;           /* Current X coordinate */
  gint         y;           /* Current Y coordinate */
  gint         width;       /* Drawable width */
  gint         height;      /* Drawable height */
  gint         tile_height; /* Height of tile in LIGMA */
  gint         count;       /* Count of rows to put in image */
  gint         zsize;       /* Number of channels in file */
  sgi_t       *sgip;        /* File pointer */
  GeglBuffer  *buffer;      /* Buffer for layer */
  const Babl  *format;
  guchar     **pixels;      /* Pixel rows */
  guchar      *pptr;        /* Current pixel */
  gushort    **rows;        /* SGI image data */

  g_object_get (config,
                "compression", &compression,
                NULL);

  /*
   * Get the drawable for the current image...
   */

  width  = ligma_drawable_get_width  (drawable);
  height = ligma_drawable_get_height (drawable);

  buffer = ligma_drawable_get_buffer (drawable);

  switch (ligma_drawable_type (drawable))
    {
    case LIGMA_GRAY_IMAGE:
      zsize = 1;
      format = babl_format ("Y' u8");
      break;

    case LIGMA_GRAYA_IMAGE:
      zsize = 2;
      format = babl_format ("Y'A u8");
      break;

    case LIGMA_RGB_IMAGE:
    case LIGMA_INDEXED_IMAGE:
      zsize = 3;
      format = babl_format ("R'G'B' u8");
      break;

    case LIGMA_RGBA_IMAGE:
    case LIGMA_INDEXEDA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      zsize = 4;
      break;

    default:
      return FALSE;
    }

  /*
   * Open the file for writing...
   */

  ligma_progress_init_printf (_("Exporting '%s'"),
                             ligma_file_get_utf8_name (file));

  sgip = sgiOpen (g_file_peek_path (file), SGI_WRITE, compression, 1,
                  width, height, zsize);

  if (! sgip)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for writing."),
                   ligma_file_get_utf8_name (file));
      return FALSE;
    };

  /*
   * Allocate memory for "tile_height" rows...
   */

  tile_height = ligma_tile_height ();
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

      ligma_progress_update ((double) y / (double) height);
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

  ligma_progress_update (1.0);

  return TRUE;
}

static gboolean
save_dialog (LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget    *dialog;
  GtkWidget    *grid;
  GtkListStore *store;
  GtkWidget    *combo;
  gboolean      run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as SGI"));

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  store = ligma_int_store_new (_("No compression"),
                              SGI_COMP_NONE,
                              _("RLE compression"),
                              SGI_COMP_RLE,
                              _("Aggressive RLE (not supported by SGI)"),
                              SGI_COMP_ARLE,
                              NULL);
  combo = ligma_prop_int_combo_box_new (config, "compression",
                                       LIGMA_INT_STORE (store));
  g_object_unref (store);

  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Compression _type:"), 1.0, 0.5,
                            combo, 1);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
