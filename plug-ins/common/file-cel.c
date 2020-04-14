/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * cel.c -- KISS CEL file format plug-in
 * (copyright) 1997,1998 Nick Lamb (njl195@zepler.org.uk)
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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-cel-load"
#define SAVE_PROC      "file-cel-save"
#define PLUG_IN_BINARY "file-cel"
#define PLUG_IN_ROLE   "gimp-file-cel"


typedef struct _Cel      Cel;
typedef struct _CelClass CelClass;

struct _Cel
{
  GimpPlugIn      parent_instance;
};

struct _CelClass
{
  GimpPlugInClass parent_class;
};


#define CEL_TYPE  (cel_get_type ())
#define CEL (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CEL_TYPE, Cel))

GType                   cel_get_type         (void) G_GNUC_CONST;

static GList          * cel_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * cel_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * cel_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * cel_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              gint                  n_drawables,
                                              GimpDrawable        **drawables,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static gint             load_palette         (GFile                *file,
                                              FILE                 *fp,
                                              guchar                palette[],
                                              GError              **error);
static GimpImage      * load_image           (GFile                *file,
                                              GError              **error);
static gboolean         save_image           (GFile                *file,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GError              **error);
static void             palette_dialog       (const gchar          *title);
static gboolean         need_palette         (GFile                *file,
                                              GError              **error);


G_DEFINE_TYPE (Cel, cel, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (CEL_TYPE)


static gchar *palette_file = NULL;
static gsize  data_length  = 0;


static void
cel_class_init (CelClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = cel_query_procedures;
  plug_in_class->create_procedure = cel_create_procedure;
}

static void
cel_init (Cel *cel)
{
}

static GList *
cel_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static GimpProcedure *
cel_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           cel_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("KISS CEL"));

      gimp_procedure_set_documentation (procedure,
                                        "Loads files in KISS CEL file format",
                                        "This plug-in loads individual KISS "
                                        "cell files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "May 1998");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "cel");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,KiSS\\040");

      GIMP_PROC_ARG_STRING (procedure, "palette-filename",
                            "Palette filename",
                            "Filename to load palette from",
                            NULL,
                            G_PARAM_READWRITE |
                            GIMP_PARAM_NO_VALIDATE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           cel_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, INDEXED*");

      gimp_procedure_set_menu_label (procedure, N_("KISS CEL"));

      gimp_procedure_set_documentation (procedure,
                                        "Exports files in KISS CEL file format",
                                        "This plug-in exports individual KISS "
                                        "cell files.",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "May 1998");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "cel");

      GIMP_PROC_ARG_STRING (procedure, "palette-filename",
                            "Palette filename",
                            "Filename to save palette to",
                            NULL,
                            G_PARAM_READWRITE |
                            GIMP_PARAM_NO_VALIDATE);
    }

  return procedure;
}

static GimpValueArray *
cel_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image         = NULL;
  gboolean        needs_palette = FALSE;
  GError         *error         = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    {
      data_length = gimp_get_data_size (SAVE_PROC);
      if (data_length > 0)
        {
          palette_file = g_malloc (data_length);
          gimp_get_data (SAVE_PROC, palette_file);
        }
      else
        {
          palette_file = g_strdup ("*.kcf");
          data_length = strlen (palette_file) + 1;
        }
    }

  if (run_mode == GIMP_RUN_NONINTERACTIVE)
    {
      palette_file = (gchar *) GIMP_VALUES_GET_STRING (args, 0);
      if (palette_file)
        data_length = strlen (palette_file) + 1;
      else
        data_length = 0;
    }
  else if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      /* Let user choose KCF palette (cancel ignores) */
      needs_palette = need_palette (file, &error);

      if (! error)
        {
          if (needs_palette)
            palette_dialog (_("Load KISS Palette"));

          gimp_set_data (SAVE_PROC, palette_file, data_length);
        }
    }

  if (! error)
    {
      image = load_image (file, &error);
    }

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
cel_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType      status = GIMP_PDB_SUCCESS;
  GimpExportReturn       export = GIMP_EXPORT_CANCEL;
  GError                *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &n_drawables, &drawables, "CEL",
                                  GIMP_EXPORT_CAN_HANDLE_RGB   |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA |
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("CEL format does not support multiple layers."));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }

  if (save_image (file, image, drawables[0], &error))
    {
      if (data_length)
        {
          gimp_set_data (SAVE_PROC, palette_file, data_length);
        }
    }
  else
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    {
      gimp_image_delete (image);
      g_free (drawables);
    }

  return gimp_procedure_new_return_values (procedure, status, error);
}

/* Peek into the file to determine whether we need a palette */
static gboolean
need_palette (GFile   *file,
              GError **error)
{
  gchar  *filename;
  FILE   *fp;
  guchar  header[32];
  size_t  n_read;

  filename = g_file_get_path (file);
  fp = g_fopen (filename, "rb");
  g_free (filename);

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  n_read = fread (header, 32, 1, fp);

  fclose (fp);

  if (n_read < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF or error while reading image header"));
      return FALSE;
    }

  return (header[5] < 32);
}

/* Load CEL image into GIMP */

static GimpImage *
load_image (GFile   *file,
            GError **error)
{
  gchar      *filename;
  FILE       *fp;            /* Read file pointer */
  guchar      header[32],    /* File header */
              file_mark,     /* KiSS file type */
              bpp;           /* Bits per pixel */
  gint        height, width, /* Dimensions of image */
              offx, offy,    /* Layer offsets */
              colors;       /* Number of colors */

  GimpImage  *image;         /* Image */
  GimpLayer  *layer;         /* Layer */
  guchar     *buf;           /* Temporary buffer */
  guchar     *line;          /* Pixel data */
  GeglBuffer *buffer;        /* Buffer for layer */

  gint        i, j, k;       /* Counters */
  size_t      n_read;        /* Number of items read from file */

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  /* Open the file for reading */
  filename = g_file_get_path (file);
  fp = g_fopen (filename, "r");
  g_free (filename);

  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Get the image dimensions and create the image... */

  n_read = fread (header, 4, 1, fp);

  if (n_read < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("EOF or error while reading image header"));
      fclose (fp);
      return NULL;
    }

  if (strncmp ((const gchar *) header, "KiSS", 4))
    {
      colors= 16;
      bpp = 4;
      width = header[0] + (256 * header[1]);
      height = header[2] + (256 * header[3]);
      offx= 0;
      offy= 0;
    }
  else
    { /* New-style image file, read full header */
      n_read = fread (header, 28, 1, fp);

      if (n_read < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("EOF or error while reading image header"));
          fclose (fp);
          return NULL;
        }

      file_mark = header[0];
      if (file_mark != 0x20 && file_mark != 0x21)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("is not a CEL image file"));
          fclose (fp);
          return NULL;
        }

      bpp = header[1];
      switch (bpp)
        {
        case 4:
        case 8:
        case 32:
          colors = (1 << bpp);
          break;
        default:
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("illegal bpp value in image: %hhu"), bpp);
          fclose (fp);
          return NULL;
        }

      width = header[4] + (256 * header[5]);
      height = header[6] + (256 * header[7]);
      offx = header[8] + (256 * header[9]);
      offy = header[10] + (256 * header[11]);
    }

  if ((width == 0) || (height == 0) || (width + offx > GIMP_MAX_IMAGE_SIZE) ||
      (height + offy > GIMP_MAX_IMAGE_SIZE))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("illegal image dimensions: width: %d, horizontal offset: "
                     "%d, height: %d, vertical offset: %d"),
                   width, offx, height, offy);
      fclose (fp);
      return NULL;
    }

  if (bpp == 32)
    image = gimp_image_new (width + offx, height + offy, GIMP_RGB);
  else
    image = gimp_image_new (width + offx, height + offy, GIMP_INDEXED);

  if (! image)
    {
      g_set_error (error, 0, 0, _("Can't create a new image"));
      fclose (fp);
      return NULL;
    }

  gimp_image_set_file (image, file);

  /* Create an indexed-alpha layer to hold the image... */
  if (bpp == 32)
    layer = gimp_layer_new (image, _("Background"), width, height,
                            GIMP_RGBA_IMAGE,
                            100,
                            gimp_image_get_default_new_layer_mode (image));
  else
    layer = gimp_layer_new (image, _("Background"), width, height,
                            GIMP_INDEXEDA_IMAGE,
                            100,
                            gimp_image_get_default_new_layer_mode (image));
  gimp_image_insert_layer (image, layer, NULL, 0);
  gimp_layer_set_offsets (layer, offx, offy);

  /* Get the drawable and set the pixel region for our load... */

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  /* Read the image in and give it to GIMP a line at a time */
  buf  = g_new (guchar, width * 4);
  line = g_new (guchar, (width + 1) * 4);

  for (i = 0; i < height && !feof(fp); ++i)
    {
      switch (bpp)
        {
        case 4:
          n_read = fread (buf, (width + 1) / 2, 1, fp);

          if (n_read < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("EOF or error while reading image data"));
              fclose (fp);
              return NULL;
            }

          for (j = 0, k = 0; j < width * 2; j+= 4, ++k)
            {
              if (buf[k] / 16 == 0)
                {
                  line[j]     = 16;
                  line[j+ 1 ] = 0;
                }
              else
                {
                  line[j]     = (buf[k] / 16) - 1;
                  line[j + 1] = 255;
                }

              if (buf[k] % 16 == 0)
                {
                  line[j + 2] = 16;
                  line[j + 3] = 0;
                }
              else
                {
                  line[j + 2] = (buf[k] % 16) - 1;
                  line[j + 3] = 255;
                }
            }
          break;

        case 8:
          n_read = fread (buf, width, 1, fp);

          if (n_read < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("EOF or error while reading image data"));
              fclose (fp);
              return NULL;
            }

          for (j = 0, k = 0; j < width * 2; j+= 2, ++k)
            {
              if (buf[k] == 0)
                {
                  line[j]     = 255;
                  line[j + 1] = 0;
                }
              else
                {
                  line[j]     = buf[k] - 1;
                  line[j + 1] = 255;
                }
            }
          break;

        case 32:
          n_read = fread (line, width * 4, 1, fp);

          if (n_read < 1)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("EOF or error while reading image data"));
              fclose (fp);
              return NULL;
            }

          /* The CEL file order is BGR so we need to swap B and R
           * to get the Gimp RGB order.
           */
          for (j= 0; j < width; j++)
            {
              guint8 tmp = line[j*4];
              line[j*4] = line[j*4+2];
              line[j*4+2] = tmp;
            }
          break;

        default:
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported bit depth (%d)!"), bpp);
          fclose (fp);
          return NULL;
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                       NULL, line, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((float) i / (float) height);
    }

  /* Close image files, give back allocated memory */

  fclose (fp);
  g_free (buf);
  g_free (line);

  if (bpp != 32)
    {
      /* Use palette from file or otherwise default grey palette */
      guchar palette[256 * 3];

      /* Open the file for reading if user picked one */
      if (palette_file == NULL)
        {
          fp = NULL;
        }
      else
        {
          fp = g_fopen (palette_file, "r");

          if (fp == NULL)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Could not open '%s' for reading: %s"),
                           gimp_filename_to_utf8 (palette_file),
                           g_strerror (errno));
              return NULL;
            }
        }

      if (fp != NULL)
        {
          colors = load_palette (g_file_new_for_path (palette_file), fp, palette, error);
          fclose (fp);
          if (colors < 0 || *error)
            return NULL;
        }
      else
        {
          for (i= 0; i < colors; ++i)
            {
              palette[i * 3] = palette[i * 3 + 1] = palette[i * 3 + 2]= i * 256 / colors;
            }
        }

      gimp_image_set_colormap (image, palette + 3, colors - 1);
    }

  /* Now get everything redrawn and hand back the finished image */

  g_object_unref (buffer);

  gimp_progress_update (1.0);

  return image;
}

static gint
load_palette (GFile       *file,
              FILE        *fp,
              guchar       palette[],
              GError     **error)
{
  guchar        header[32];     /* File header */
  guchar        buffer[2];
  guchar        file_mark, bpp;
  gint          i, colors = 0;
  size_t        n_read;

  n_read = fread (header, 4, 1, fp);

  if (n_read < 1)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s': EOF or error while reading palette header"),
                   gimp_file_get_utf8_name (file));
      return -1;
    }

  if (!strncmp ((const gchar *) header, "KiSS", 4))
    {
      n_read = fread (header+4, 28, 1, fp);

      if (n_read < 1)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': EOF or error while reading palette header"),
                       gimp_file_get_utf8_name (file));
          return -1;
        }

      file_mark = header[4];
      if (file_mark != 0x10)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': is not a KCF palette file"),
                       gimp_file_get_utf8_name (file));
          return -1;
        }

      bpp = header[5];
      if (bpp != 12 && bpp != 24)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': illegal bpp value in palette: %hhu"),
                       gimp_file_get_utf8_name (file), bpp);
          return -1;
        }

      colors = header[8] + header[9] * 256;
      if (colors != 16 && colors != 256)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("'%s': illegal number of colors: %u"),
                       gimp_file_get_utf8_name (file), colors);
          return -1;
        }

      switch (bpp)
        {
        case 12:
          for (i = 0; i < colors; ++i)
            {
              n_read = fread (buffer, 1, 2, fp);

              if (n_read < 2)
                {
                  g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                               _("'%s': EOF or error while reading "
                                 "palette data"),
                               gimp_file_get_utf8_name (file));
                  return -1;
                }

              palette[i*3]= buffer[0] & 0xf0;
              palette[i*3+1]= (buffer[1] & 0x0f) * 16;
              palette[i*3+2]= (buffer[0] & 0x0f) * 16;
            }
          break;
        case 24:
          n_read = fread (palette, colors, 3, fp);

          if (n_read < 3)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("'%s': EOF or error while reading palette data"),
                           gimp_file_get_utf8_name (file));
              return -1;
            }
          break;
        default:
          g_assert_not_reached ();
        }
    }
  else
    {
      colors = 16;
      fseek (fp, 0, SEEK_SET);
      for (i= 0; i < colors; ++i)
        {
          n_read = fread (buffer, 1, 2, fp);

          if (n_read < 2)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("'%s': EOF or error while reading palette data"),
                           gimp_file_get_utf8_name (file));
              return -1;
            }

          palette[i*3] = buffer[0] & 0xf0;
          palette[i*3+1] = (buffer[1] & 0x0f) * 16;
          palette[i*3+2] = (buffer[0] & 0x0f) * 16;
        }
    }

  return colors;
}

static gboolean
save_image (GFile         *file,
            GimpImage     *image,
            GimpDrawable  *drawable,
            GError       **error)
{
  GOutputStream *output;
  GeglBuffer    *buffer;
  const Babl    *format;
  GCancellable  *cancellable;
  gint           width;
  gint           height;
  guchar         header[32];    /* File header */
  gint           bpp;           /* Bit per pixel */
  gint           colors;        /* Number of colors */
  gint           type;          /* type of layer */
  gint           offx, offy;    /* Layer offsets */
  guchar        *buf  = NULL;   /* Temporary buffer */
  guchar        *line = NULL;   /* Pixel data */
  gint           i, j, k;       /* Counters */

  /* Check that this is an indexed image, fail otherwise */
  type = gimp_drawable_type (drawable);

  if (type == GIMP_INDEXEDA_IMAGE)
    {
      bpp    = 4;
      format = NULL;
    }
  else
    {
      bpp    = 32;
      format = babl_format ("R'G'B'A u8");
    }

  /* Find out how offset this layer was */
  gimp_drawable_offsets (drawable, &offx, &offy);

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

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

  /* Headers */
  memset (header, 0, 32);
  strcpy ((gchar *) header, "KiSS");
  header[4]= 0x20;

  /* Work out whether to save as 8bit or 4bit */
  if (bpp < 32)
    {
      g_free (gimp_image_get_colormap (image, &colors));

      if (colors > 15)
        {
          header[5] = 8;
        }
      else
        {
          header[5] = 4;
        }
    }
  else
    {
      header[5] = 32;
    }

  /* Fill in the blanks ... */
  header[8]  = width % 256;
  header[9]  = width / 256;
  header[10] = height % 256;
  header[11] = height / 256;
  header[12] = offx % 256;
  header[13] = offx / 256;
  header[14] = offy % 256;
  header[15] = offy / 256;

  if (! g_output_stream_write_all (output, header, 32, NULL,
                                   NULL, error))
    goto fail;

  /* Arrange for memory etc. */
  buf  = g_new (guchar, width * 4);
  line = g_new (guchar, (width + 1) * 4);

  /* Get the image from GIMP one line at a time and write it out */
  for (i = 0; i < height; ++i)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, 1), 1.0,
                       format, line,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      memset (buf, 0, width);

      if (bpp == 32)
        {
          for (j = 0; j < width; j++)
            {
              buf[4 * j]     = line[4 * j + 2];   /* B */
              buf[4 * j + 1] = line[4 * j + 1];   /* G */
              buf[4 * j + 2] = line[4 * j + 0];   /* R */
              buf[4 * j + 3] = line[4 * j + 3];   /* Alpha */
            }

          if (! g_output_stream_write_all (output, buf, width * 4, NULL,
                                           NULL, error))
            goto fail;
        }
      else if (colors > 16)
        {
          for (j = 0, k = 0; j < width * 2; j += 2, ++k)
            {
              if (line[j + 1] > 127)
                {
                  buf[k]= line[j] + 1;
                }
            }

          if (! g_output_stream_write_all (output, buf, width, NULL,
                                           NULL, error))
            goto fail;
        }
      else
        {
          for (j = 0, k = 0; j < width * 2; j+= 4, ++k)
            {
              buf[k] = 0;

              if (line[j + 1] > 127)
                {
                  buf[k] += (line[j] + 1)<< 4;
                }

              if (line[j + 3] > 127)
                {
                  buf[k] += (line[j + 2] + 1);
                }
            }

          if (! g_output_stream_write_all (output, buf, width + 1 / 2, NULL,
                                           NULL, error))
            goto fail;
        }

      gimp_progress_update ((float) i / (float) height);
    }

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  gimp_progress_update (1.0);

  g_free (buf);
  g_free (line);
  g_object_unref (buffer);
  g_object_unref (output);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);
  g_object_unref (cancellable);

  g_free (buf);
  g_free (line);
  g_object_unref (buffer);
  g_object_unref (output);

  return FALSE;
}

static void
palette_dialog (const gchar *title)
{
  GtkWidget *dialog;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gtk_file_chooser_dialog_new (title, NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Open"),   GTK_RESPONSE_OK,

                                        NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), palette_file);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  gtk_widget_show (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      g_free (palette_file);
      palette_file = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      data_length = strlen (palette_file) + 1;
    }

  gtk_widget_destroy (dialog);
}
