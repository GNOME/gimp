/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Sony Playstation TIM graphics plug-in
 *
 *   Copyright (C) 2025 Alex S.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>


#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-tim-load"
#define EXPORT_PROC    "file-tim-export"
#define PLUG_IN_BINARY "file-tim"
#define PLUG_IN_ROLE   "gimp-file-tim"


typedef struct _TimSharedHeader
{
  guchar  magic[4];
  guchar  type[4];

  guint32 data_offset;
  guchar  x[2];
  guchar  y[2];
} TimSharedHeader;

typedef enum
{
  PSX_4BPP  = 8,
  PSX_8BPP  = 9,
  PSX_16BPP = 2,
  PSX_24BPP = 3
} TimType;

typedef struct _Tim      Tim;
typedef struct _TimClass TimClass;

struct _Tim
{
  GimpPlugIn      parent_instance;
};

struct _TimClass
{
  GimpPlugInClass parent_class;
};


#define TIM_TYPE  (tim_get_type ())
#define TIM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TIM_TYPE, Tim))

GType                   tim_get_type                  (void) G_GNUC_CONST;


static GList          *         tim_query_procedures  (GimpPlugIn            *plug_in);
static GimpProcedure  *         tim_create_procedure  (GimpPlugIn            *plug_in,
                                                       const gchar           *name);

static GimpValueArray *         tim_load              (GimpProcedure         *procedure,
                                                       GimpRunMode            run_mode,
                                                       GFile                 *file,
                                                       GimpMetadata          *metadata,
                                                       GimpMetadataLoadFlags *flags,
                                                       GimpProcedureConfig   *config,
                                                       gpointer               run_data);
static GimpValueArray *        tim_export             (GimpProcedure         *procedure,
                                                       GimpRunMode            run_mode,
                                                       GimpImage             *image,
                                                       GFile                 *file,
                                                       GimpExportOptions     *options,
                                                       GimpMetadata          *metadata,
                                                       GimpProcedureConfig   *config,
                                                       gpointer               run_data);

static GimpImage      *         load_image            (GFile                 *file,
                                                       GObject               *config,
                                                       GimpRunMode            run_mode,
                                                       FILE                  *fp,
                                                       GError               **error);
static gboolean                 export_image          (GFile                 *file,
                                                       GimpProcedureConfig   *config,
                                                       GimpImage             *image,
                                                       GimpDrawable          *drawable,
                                                       GError               **error);

static GimpExportCapabilities   export_edit_options   (GimpProcedure        *procedure,
                                                       GimpProcedureConfig  *config,
                                                       GimpExportOptions    *options,
                                                       gpointer              create_data);

static gboolean                 export_dialog         (GimpImage            *image,
                                                       GimpProcedure        *procedure,
                                                       GObject              *config);

static void                     convert_from_a1r5g5b5 (gushort               data,
                                                       guint                 index,
                                                       guchar               *pixel);
static void                    convert_to_a1r5g5b5    (guchar               *pixel,
                                                       guint                 index,
                                                       guchar               *data);

G_DEFINE_TYPE (Tim, tim, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (TIM_TYPE)
DEFINE_STD_SET_I18N


static void
tim_class_init (TimClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = tim_query_procedures;
  plug_in_class->create_procedure = tim_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
tim_init (Tim *tim)
{
}

static GList *
tim_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
tim_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           tim_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure,
                                     _("TIM Image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the TIM file format"),
                                        _("Load file in the Sony Playstation "
                                          "TIM file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Andrew Kieschnick",
                                      "Andrew Kieschnick <andrewk@mail.utexas.edu>",
                                      "1998");

      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "tim");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, tim_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*, GRAY*, INDEXED*");

      gimp_procedure_set_menu_label (procedure, _("Playstation TIM image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Saves files in the TIM file format"),
                                        _("Saves files in the Playstation TIM "
                                          "file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Andrew Kieschnick",
                                      "Andrew Kieschnick <andrewk@mail.utexas.edu>",
                                      "1998");

      gimp_procedure_set_menu_label (procedure, _("Playstation TIM image"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           "Playstation TIM image");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "tim");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              0, export_edit_options, NULL, NULL);

      gimp_procedure_add_choice_argument (procedure, "type",
                                          _("_Type"),
                                          _("Type of TIM texture to export"),
                                          gimp_choice_new_with_values ("pal-4bpp",  PSX_4BPP,  _("4BPP (Indexed)"),  NULL,
                                                                       "pal-8bpp",  PSX_8BPP,  _("8BPP (Indexed)"),  NULL,
                                                                       "rgb-16bpp", PSX_16BPP, _("16BPP (RGB)"),     NULL,
                                                                       "rgb-24bpp", PSX_24BPP, _("24BPP (RGB)"),     NULL,
                                                                       NULL),
                                          "pal-8bpp",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "origin-x",
                                       _("Image _X"),
                                       _("Origin X coordinate"),
                                       0, G_MAXUSHORT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "origin-y",
                                       _("Image _Y"),
                                       _("Origin Y coordinate"),
                                       0, G_MAXUSHORT, 0,
                                       G_PARAM_READWRITE);

      /* Only used with indexed image exports */
      gimp_procedure_add_int_argument (procedure, "palette-x",
                                       _("_Palette X"),
                                       _("Palette X coordinate"),
                                       0, G_MAXUSHORT, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "palette-y",
                                       _("P_alette Y"),
                                       _("Palette Y coordinate"),
                                       0, 511, 0,
                                       G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
tim_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  FILE           *fp;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  image = load_image (file, G_OBJECT (config), run_mode, fp, &error);

  fclose (fp);

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
tim_export (GimpProcedure        *procedure,
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

      if (! export_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      export    = gimp_export_options_get_image (options, &image);
      drawables = gimp_image_list_layers (image);

      if (! export_image (file, config, image, drawables->data, &error))
        status = GIMP_PDB_EXECUTION_ERROR;

      g_list_free (drawables);
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            GimpRunMode   run_mode,
            FILE         *fp,
            GError      **error)
{
  GimpImage         *image = NULL;
  GimpLayer         *layer = NULL;
  GimpImageBaseType  image_type;
  GimpImageType      layer_type;
  GeglBuffer        *buffer;
  TimSharedHeader    tim_header;
  gushort            width;
  gushort            height;

  if (fread (&tim_header, sizeof (TimSharedHeader), 1, fp) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   gimp_file_get_utf8_name (file));
      return NULL;
    }

  if (tim_header.type[0] != PSX_16BPP &&
      tim_header.type[0] != PSX_24BPP &&
      tim_header.type[0] != PSX_4BPP  &&
      tim_header.type[0] != PSX_8BPP)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported TIM image type: %d"),
                   tim_header.type[0]);
      return NULL;
    }

  image_type = GIMP_RGB;
  layer_type = GIMP_RGBA_IMAGE;

  if (tim_header.type[0] == PSX_16BPP ||
      tim_header.type[0] == PSX_24BPP)
    {
      if (fread (&width, 2, 1, fp) == 0 ||
          fread (&height, 2, 1, fp) == 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read header from '%s'"),
                       gimp_file_get_utf8_name (file));
          return NULL;
        }

      if (tim_header.type[0] == PSX_24BPP)
        {
          width      = (gushort) ((gfloat) width / 1.5);
          layer_type = GIMP_RGB_IMAGE;
        }

      image = gimp_image_new (width, height, image_type);
      layer = gimp_layer_new (image, _("Background"),
                              width, height, layer_type, 100,
                              gimp_image_get_default_new_layer_mode (image));
      gimp_image_insert_layer (image, layer, NULL, 0);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      /* Pixels are in A1B5G5R5 format */
      if (tim_header.type[0] == PSX_16BPP)
        {
          gushort pixels[width * 2];

          for (gint i = 0; i < height; i++)
            {
              guchar row[width * 4];

              fread (&pixels, width * 2, 1, fp);

              for (gint j = 0; j < width; j++)
                convert_from_a1r5g5b5 (pixels[j], j, row);

              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                               NULL, row, GEGL_AUTO_ROWSTRIDE);
            }
        }
      else
        {
          guchar pixels[width * 3];

          for (gint i = 0; i < height; i++)
            {
              fread (&pixels, width * 3, 1, fp);

              gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                               babl_format ("R'G'B' u8"), pixels,
                               GEGL_AUTO_ROWSTRIDE);
            }
        }

      g_object_unref (buffer);
    }
  else if (tim_header.type[0] == PSX_4BPP ||
           tim_header.type[0] == PSX_8BPP)
    {
      gushort num_colors;
      gushort num_cluts;

      if (fread (&num_colors, 2, 1, fp) == 0 ||
          fread (&num_cluts, 2, 1, fp) == 0)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Could not read header from '%s'"),
                       gimp_file_get_utf8_name (file));
          return NULL;
        }

     if (num_colors > 0)
      {
        guint    clut_size = num_colors * num_cluts;
        gushort  clut_data[clut_size];
        guchar   color_map[clut_size * 4];
        gboolean promote_to_rgb = FALSE;
        guint32  image_offset;
        gushort  x;
        gushort  y;

        /* TODO: GIMP currently only supports 256 colors in an indexed image.
         * TIM textures can have multiple tables and exceeds that, so we'll
         * convert those to RGB instead*/
        if (clut_size > 256)
          promote_to_rgb = TRUE;

        if (! promote_to_rgb)
          {
            image_type = GIMP_INDEXED;
            layer_type = GIMP_INDEXED_IMAGE;
          }

        if (fread (&clut_data, (clut_size * 2), 1, fp) == 0)
          {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Could not read palette from '%s'"),
                         gimp_file_get_utf8_name (file));
            return NULL;
          }

        for (gint i = 0; i < clut_size; i++)
          convert_from_a1r5g5b5 (clut_data[i], i, color_map);

        if (fread (&image_offset, 4, 1, fp) == 0 ||
            fread (&x, 2, 1, fp) == 0            ||
            fread (&y, 2, 1, fp) == 0            ||
            fread (&width, 2, 1, fp) == 0        ||
            fread (&height, 2, 1, fp) == 0)
          {
            g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                         _("Could not read header from '%s'"),
                         gimp_file_get_utf8_name (file));
            return NULL;
          }

        if (tim_header.type[0] == PSX_4BPP)
          width *= 4;
        else if (tim_header.type[0] == PSX_8BPP)
          width *= 2;

        image = gimp_image_new (width, height, image_type);
        layer = gimp_layer_new (image, _("Background"),
                                width, height, layer_type, 100,
                                gimp_image_get_default_new_layer_mode (image));
        gimp_image_insert_layer (image, layer, NULL, 0);

        gimp_palette_set_colormap (gimp_image_get_palette (image),
                                   babl_format ("R'G'B'A u8"),
                                   (guint8 *) color_map, clut_size * 4);

        buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

        /* Two pixels per byte */
        if (tim_header.type[0] == PSX_4BPP)
          {
            guchar pixels[width];

            for (gint i = 0; i < height; i++)
              {
                guchar row[width];

                fread (&pixels, width, 1, fp);

                for (gint j = 0; j < width; j++)
                  {
                    row[j * 2]     = pixels[j] & 0x0F;
                    row[j * 2 + 1] = pixels[j] >> 4;
                  }

                gegl_buffer_set (buffer, GEGL_RECTANGLE (0, (i * 2), width, 2), 0,
                                 NULL, row, GEGL_AUTO_ROWSTRIDE);

              }
          }
        else if (tim_header.type[0] == PSX_8BPP)
          {
            guchar pixels[width];
            guchar rgb_pixels[width * 4];

            for (gint i = 0; i < height; i++)
              {
                fread (&pixels, width, 1, fp);

                if (! promote_to_rgb)
                  {
                    gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                                     NULL, pixels, GEGL_AUTO_ROWSTRIDE);
                  }
                else
                  {
                    gint index = 0;
                    for (gint i = 0; i < width; i++)
                      {
                        index = pixels[i];
                        for (gint j = 0; j < 4; j++)
                          rgb_pixels[(i * 4) + j] = color_map[index * 4 + j];
                      }

                    gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                                     NULL, rgb_pixels, GEGL_AUTO_ROWSTRIDE);
                  }
              }
          }
        g_object_unref (buffer);
      }
    }

  return image;
}

static gboolean
export_image (GFile                *file,
              GimpProcedureConfig  *config,
              GimpImage            *image,
              GimpDrawable         *drawable,
              GError              **error)
{
  FILE       *fp;
  gsize       file_size;
  GeglBuffer *buffer;
  guchar     *pixels;
  guint16     width;
  guint16     height;
  guchar      magic[4] = {0x10, 0, 0, 0};
  guchar      type;
  guint16     origin_x;
  guint16     origin_y;
  guint16     palette_x;
  guint16     palette_y;

  g_object_get (config,
                "origin-x",  &origin_x,
                "origin-y",  &origin_y,
                "palette-x", &palette_x,
                "palette-y", &palette_y,
                NULL);
  type = (guchar) gimp_procedure_config_get_choice_id (config, "type");

  buffer = gimp_drawable_get_buffer (drawable);
  width  = (guint16) gegl_buffer_get_width (buffer);
  height = (guint16) gegl_buffer_get_height (buffer);

  if (gegl_buffer_get_width (buffer) > G_MAXUSHORT ||
      gegl_buffer_get_height (buffer) > G_MAXUSHORT)
    {
      g_set_error (error, 0, 0,
                   _("Unable to export '%s'.  "
                     "The TIM file format does not support images that are "
                     "more than %d pixels wide or tall."),
                   gimp_file_get_utf8_name (file), G_MAXUSHORT);

      return FALSE;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  /* Write header */
  fwrite (&magic, 4, 1, fp);

  fputc (type, fp);
  for (gint i = 0; i < 3; i++)
    fputc (0, fp);

  /* RGB images exported */
  if (type == PSX_16BPP ||
      type == PSX_24BPP)
    {
      const Babl *format       = babl_format ("R'G'B' u8");
      guint16     export_width = width;

      /* Come back once we have the final file size */
      for (gint i = 0; i < 4; i++)
        fputc (0, fp);

      if (type == PSX_24BPP)
        export_width *= 1.5;

      fwrite (&origin_x, 2, 1, fp);
      fwrite (&origin_y, 2, 1, fp);

      fwrite (&export_width, 2, 1, fp);
      fwrite (&height, 2, 1, fp);

        /* Write pixel data */
      for (gint i = 0; i < height; i++)
        {
          pixels = g_malloc (width * 3);

          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, 1),
                           1.0, format, pixels,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          if (type == PSX_24BPP)
            {
              for (gint j = 0; j < (width * 3); j++)
                fwrite (&pixels[j], 1, 1, fp);
            }
          else if (type == PSX_16BPP)
            {
              guchar converted[2];

              for (gint j = 0; j < width; j++)
                {
                  convert_to_a1r5g5b5 (pixels, (j * 3), converted);
                  fwrite (&converted, 2, 1, fp);
                }
            }

          g_free (pixels);
          gimp_progress_update (i / (gdouble) height);
        }

      /* Update file size */
      fseek (fp, 0, SEEK_END);
      file_size = ftell (fp) + 12;
      fseek (fp, 9, SEEK_SET);

      fwrite (&file_size, 4, 1, fp);
    }
  else
    {
      guchar  *colormap;
      gint     n_colors;
      guint32  colormap_size;
      guint16  palette_count;
      guint16  clut_count;
      guint32  image_size;
      guint16  export_width;
      guchar   converted[2];
      guchar  *indexes;

      colormap = gimp_palette_get_colormap (gimp_image_get_palette (image),
                                            babl_format ("R'G'B' u8"),
                                            &n_colors, NULL);

      /* Colormap size will be either a multiple of 16 or 256 */
      if (type == PSX_4BPP)
        colormap_size = ceil (n_colors / 16.0) * 16;
      else
        colormap_size = ceil (n_colors / 256.0) * 256;

      colormap_size = (colormap_size * 2) + 12;

      /* Palette Header */
      fwrite (&colormap_size, 4, 1, fp);
      fwrite (&palette_x, 2, 1, fp);
      fwrite (&palette_y, 2, 1, fp);

      if (type == PSX_4BPP)
        palette_count = 16;
      else
        palette_count = 256;

      clut_count = ceil ((gfloat) n_colors / palette_count);

      fwrite (&palette_count, 2, 1, fp);
      fwrite (&clut_count, 2, 1, fp);

      /* Writing actual palette */
      for (gint i = 0; i < n_colors; i++)
        {
          convert_to_a1r5g5b5 (colormap, (i * 3), converted);
          fwrite (&converted, 2, 1, fp);
        }
      /* Fill in remaining entries with padding */
      n_colors = (palette_count * clut_count) - n_colors;
      for (gint i = 0; i < n_colors; i++)
        fwrite (&converted, 2, 1, fp);

      /* Image header */
      image_size = width * height;
      if (type == PSX_4BPP)
        image_size /= 2;

      image_size += 12;

      fwrite (&image_size, 4, 1, fp);
      fwrite (&origin_x, 2, 1, fp);
      fwrite (&origin_y, 2, 1, fp);

      if (type == PSX_4BPP)
        export_width = width / 4;
      else
        export_width = width / 2;

      export_width = GUINT16_TO_LE (export_width);
      height       = GUINT16_TO_LE (height);

      fwrite (&export_width, 2, 1, fp);
      fwrite (&height, 2, 1, fp);

      /* Writing actual image data */
      indexes = g_malloc ((guint32) width * height);
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height),
                       1.0, NULL, indexes,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (type == PSX_8BPP)
        {
          for (gint j = 0; j < (width * height); j++)
            {
              /* Index can't exceed the palette size. In-game,
               * the palette can be swapped so we can access
               * the other colors */
              indexes[j] %= palette_count;

              fwrite (&indexes[j], 1, 1, fp);
            }
        }
      else
        {
          guchar compressed = 0;

          for (gint j = 0; j < (width * height); j += 2)
            {
              compressed = indexes[j] % palette_count;

              if ((j + 1) < (width * height))
                {
                  indexes[j + 1] %= palette_count;

                  compressed += (indexes[j + 1] << 4);
                }

              fwrite (&compressed, 1, 1, fp);
            }
        }
    }

  gimp_progress_update (1.0);

  fclose (fp);

  if (buffer)
    g_object_unref (buffer);

  return TRUE;
}

static GimpExportCapabilities
export_edit_options (GimpProcedure        *procedure,
                     GimpProcedureConfig  *config,
                     GimpExportOptions    *options,
                     gpointer              create_data)
{
  GimpExportCapabilities capabilities;
  gint                   type;

  g_object_get (G_OBJECT (config),
                "type", &type,
                NULL);

  capabilities = (GIMP_EXPORT_CAN_HANDLE_INDEXED);

  if (type == PSX_16BPP || type == PSX_24BPP)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_RGB;

  return capabilities;
}


static gboolean
export_dialog (GimpImage     *image,
               GimpProcedure *procedure,
               GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *box;
  gboolean   run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "image-origin-label", _("Image Origin"),
                                   FALSE, FALSE);

  box = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "image-box", "origin-x", "origin-y",
                                        NULL);
  gtk_box_set_spacing (GTK_BOX (box), 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "image-frame", "image-origin-label",
                                    FALSE, "image-box");

  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "palette-origin-label", _("Palette Origin"),
                                   FALSE, FALSE);

  box = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                        "palette-box", "palette-x",
                                        "palette-y", NULL);
  gtk_box_set_spacing (GTK_BOX (box), 12);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "palette-frame", "palette-origin-label",
                                    FALSE, "palette-box");

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog), "type",
                              "image-frame", "palette-frame", NULL);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
convert_from_a1r5g5b5 (gushort  data,
                       guint    index,
                       guchar  *pixel)
{
  data = GUINT16_FROM_LE (data);

  pixel[index * 4]     = (data & 0x1F) << 3;
  pixel[index * 4 + 1] = ((data >> 5) & 0x1F) << 3;
  pixel[index * 4 + 2] = (data >> 10) << 3;
  pixel[index * 4 + 3] = 255;
}

static void
convert_to_a1r5g5b5 (guchar *pixel,
                     guint   index,
                     guchar *data)
{
  data[1] = ((pixel[index + 2] >> 3) << 2) & 0x7C;
  data[1] += (pixel[index + 1] >> 6) & 0x03;

  data[0] = ((pixel[index + 1] >> 3) << 5) & 0xE0;
  data[0] += (pixel[index] >> 3) & 0x1F;
}
