/* LIGMA - The GNU Image Manipulation Program
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

#include "openexr-wrapper.h"

#define LOAD_PROC       "file-exr-load"
#define PLUG_IN_BINARY  "file-exr"
#define PLUG_IN_VERSION "0.0.0"


typedef struct _Exr      Exr;
typedef struct _ExrClass ExrClass;

struct _Exr
{
  LigmaPlugIn      parent_instance;
};

struct _ExrClass
{
  LigmaPlugInClass parent_class;
};


#define EXR_TYPE  (exr_get_type ())
#define EXR (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EXR_TYPE, Exr))

GType                   exr_get_type         (void) G_GNUC_CONST;

static GList          * exr_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * exr_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * exr_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile                *file,
                                              gboolean              interactive,
                                              GError              **error);
static void             sanitize_comment     (gchar                *comment);


G_DEFINE_TYPE (Exr, exr, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (EXR_TYPE)
DEFINE_STD_SET_I18N


static void
exr_class_init (ExrClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = exr_query_procedures;
  plug_in_class->create_procedure = exr_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
exr_init (Exr *exr)
{
}

static GList *
exr_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (LOAD_PROC));
}

static LigmaProcedure *
exr_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           exr_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("OpenEXR image"));

      ligma_procedure_set_documentation (procedure,
                                        _("Loads files in the OpenEXR file format"),
                                        "This plug-in loads OpenEXR files. ",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Dominik Ernst <dernst@gmx.de>, "
                                      "Mukund Sivaraman <muks@banu.com>",
                                      "Dominik Ernst <dernst@gmx.de>, "
                                      "Mukund Sivaraman <muks@banu.com>",
                                      NULL);

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-exr");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "exr");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,long,0x762f3101");
    }

  return procedure;
}

static LigmaValueArray *
exr_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error  = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, run_mode == LIGMA_RUN_INTERACTIVE,
                      &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals =  ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_SUCCESS,
                                                   NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaImage *
load_image (GFile        *file,
            gboolean      interactive,
            GError      **error)
{
  EXRLoader        *loader;
  gint              width;
  gint              height;
  gboolean          has_alpha;
  LigmaImageBaseType image_type;
  LigmaPrecision     image_precision;
  LigmaImage        *image = NULL;
  LigmaImageType     layer_type;
  LigmaLayer        *layer;
  const Babl       *format;
  GeglBuffer       *buffer = NULL;
  gint              bpp;
  gint              tile_height;
  gchar            *pixels = NULL;
  gint              begin;
  gint32            success = FALSE;
  gchar            *comment = NULL;
  LigmaColorProfile *profile = NULL;
  guchar           *exif_data;
  guint             exif_size;
  guchar           *xmp_data;
  guint             xmp_size;

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  loader = exr_loader_new (g_file_peek_path (file));

  if (! loader)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s' for reading"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  width  = exr_loader_get_width (loader);
  height = exr_loader_get_height (loader);

  if ((width < 1) || (height < 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image dimensions from '%s'"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  has_alpha = exr_loader_has_alpha (loader) ? TRUE : FALSE;

  switch (exr_loader_get_precision (loader))
    {
    case PREC_UINT:
      image_precision = LIGMA_PRECISION_U32_LINEAR;
      break;
    case PREC_HALF:
      image_precision = LIGMA_PRECISION_HALF_LINEAR;
      break;
    case PREC_FLOAT:
      image_precision = LIGMA_PRECISION_FLOAT_LINEAR;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image precision from '%s'"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  switch (exr_loader_get_image_type (loader))
    {
    case IMAGE_TYPE_RGB:
      image_type = LIGMA_RGB;
      layer_type = has_alpha ? LIGMA_RGBA_IMAGE : LIGMA_RGB_IMAGE;
      break;
    case IMAGE_TYPE_GRAY:
      image_type = LIGMA_GRAY;
      layer_type = has_alpha ? LIGMA_GRAYA_IMAGE : LIGMA_GRAY_IMAGE;
      break;
    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error querying image type from '%s'"),
                   ligma_file_get_utf8_name (file));
      goto out;
    }

  image = ligma_image_new_with_precision (width, height,
                                         image_type, image_precision);
  if (! image)
    {
      g_set_error (error, 0, 0,
                   _("Could not create new image for '%s': %s"),
                   ligma_file_get_utf8_name (file),
                   ligma_pdb_get_last_error (ligma_get_pdb ()));
      goto out;
    }

  ligma_image_set_file (image, file);

  /* try to load an icc profile, it will be generated on the fly if
   * chromaticities are given
   */
  if (image_type == LIGMA_RGB)
    {
      profile = exr_loader_get_profile (loader);

      if (profile)
        ligma_image_set_color_profile (image, profile);
    }

  layer = ligma_layer_new (image, _("Background"), width, height,
                          layer_type, 100,
                          ligma_image_get_default_new_layer_mode (image));
  ligma_image_insert_layer (image, layer, NULL, 0);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));
  format = ligma_drawable_get_format (LIGMA_DRAWABLE (layer));
  bpp = babl_format_get_bytes_per_pixel (format);

  tile_height = ligma_tile_height ();
  pixels = g_new0 (gchar, tile_height * width * bpp);

  for (begin = 0; begin < height; begin += tile_height)
    {
      gint end;
      gint num;
      gint i;

      end = MIN (begin + tile_height, height);
      num = end - begin;

      for (i = 0; i < num; i++)
        {
          gint retval;

          retval = exr_loader_read_pixel_row (loader,
                                              pixels + (i * width * bpp),
                                              bpp, begin + i);
          if (retval < 0)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Error reading pixel data from '%s'"),
                           ligma_file_get_utf8_name (file));
              goto out;
            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, begin, width, num),
                       0, NULL, pixels, GEGL_AUTO_ROWSTRIDE);

      ligma_progress_update ((gdouble) begin / (gdouble) height);
    }

  /* try to read the file comment */
  comment = exr_loader_get_comment (loader);
  if (comment)
    {
      LigmaParasite *parasite;

      sanitize_comment (comment);
      parasite = ligma_parasite_new ("ligma-comment",
                                    LIGMA_PARASITE_PERSISTENT,
                                    strlen (comment) + 1,
                                    comment);
      ligma_image_attach_parasite (image, parasite);
      ligma_parasite_free (parasite);
    }

  /* check if the image contains Exif or Xmp data and read it */
  exif_data = exr_loader_get_exif (loader, &exif_size);
  xmp_data  = exr_loader_get_xmp  (loader, &xmp_size);

  if (exif_data || xmp_data)
    {
      LigmaMetadata          *metadata = ligma_metadata_new ();
      LigmaMetadataLoadFlags  flags    = LIGMA_METADATA_LOAD_ALL;

      if (exif_data)
        {
          ligma_metadata_set_from_exif (metadata, exif_data, exif_size, NULL);
          g_free (exif_data);
        }

      if (xmp_data)
        {
          ligma_metadata_set_from_xmp (metadata, xmp_data, xmp_size, NULL);
          g_free (xmp_data);
        }

      if (comment)
        flags &= ~LIGMA_METADATA_LOAD_COMMENT;

      if (profile)
        flags &= ~LIGMA_METADATA_LOAD_COLORSPACE;

      ligma_image_metadata_load_finish (image, "image/exr",
                                       metadata, flags);
      g_object_unref (metadata);
    }

  ligma_progress_update (1.0);

  success = TRUE;

 out:
  g_clear_object (&profile);
  g_clear_object (&buffer);
  g_clear_pointer (&pixels, g_free);
  g_clear_pointer (&comment, g_free);
  g_clear_pointer (&loader, exr_loader_unref);

  if (success)
    return image;

  if (image)
    ligma_image_delete (image);

  return NULL;
}

/* copy & pasted from file-jpeg/jpeg-load.c */
static void
sanitize_comment (gchar *comment)
{
  const gchar *start_invalid;

  if (! g_utf8_validate (comment, -1, &start_invalid))
    {
      guchar *c;

      for (c = (guchar *) start_invalid; *c; c++)
        {
          if (*c > 126 || (*c < 32 && *c != '\t' && *c != '\n' && *c != '\r'))
            *c = '?';
        }
    }
}
