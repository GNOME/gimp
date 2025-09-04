/*   GIMP - The GNU Image Manipulation Program
 *   Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Amiga IFF plug-in
 *
 *   Copyright (C) 2023 Alex S.
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

#include <libilbm/ilbm.h>
#include <libilbm/ilbmimage.h>
#include <libilbm/interleave.h>
#include <libilbm/byterun.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC       "file-iff-load"
#define LOAD_THUMB_PROC "file-iff-load-thumb"
#define PLUG_IN_BINARY  "file-iff"
#define PLUG_IN_ROLE    "gimp-file-iff"

typedef struct _Iff      Iff;
typedef struct _IffClass IffClass;

struct _Iff
{
  GimpPlugIn      parent_instance;
};

struct _IffClass
{
  GimpPlugInClass parent_class;
};


#define IFF_TYPE  (iff_get_type ())
#define IFF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), IFF_TYPE, Iff))

GType                   iff_get_type         (void) G_GNUC_CONST;


static GList          * iff_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * iff_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * iff_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * iff_load_thumb       (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_image           (GFile                 *file,
                                              GObject               *config,
                                              gboolean               is_thumbnail,
                                              GimpRunMode            run_mode,
                                              GError               **error);

static void             deleave_indexed_row  (IFF_UByte             *bitplanes,
                                              guchar                *pixel_row,
                                              gint                   width,
                                              gint                   nPlanes);

static void             deleave_rgb_row      (IFF_UByte             *bitplanes,
                                              guchar                *pixel_row,
                                              gint                   width,
                                              gint                   nPlanes,
                                              gint                   pixel_size);

static void             deleave_ham_row      (const guchar          *gimp_cmap,
                                              IFF_UByte             *bitplanes,
                                              guchar                *pixel_row,
                                              gint                   width,
                                              gint                   nPlanes);

static void             pbm_row              (IFF_UByte             *bitplanes,
                                              guchar                *pixel_row,
                                              gint                   width);


G_DEFINE_TYPE (Iff, iff, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (IFF_TYPE)
DEFINE_STD_SET_I18N


static void
iff_class_init (IffClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = iff_query_procedures;
  plug_in_class->create_procedure = iff_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
iff_init (Iff *iff)
{
}

static GList *
iff_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static GimpProcedure *
iff_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           iff_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("Amiga IFF"));

      gimp_procedure_set_documentation (procedure,
                                        _("Load file in the IFF file format"),
                                        _("Load file in the IFF file format"),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-ilbm");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "iff,ilbm,lbm,acbm,ham,ham6,ham8");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,FORM");

      gimp_load_procedure_set_thumbnail_loader (GIMP_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = gimp_thumbnail_procedure_new (plug_in, name,
                                                GIMP_PDB_PROC_TYPE_PLUGIN,
                                                iff_load_thumb, NULL, NULL);

      /* TODO: localize when string freeze is over. */
      gimp_procedure_set_documentation (procedure,
                                        "Load IFF file as thumbnail",
                                        "",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Alex S.",
                                      "Alex S.",
                                      "2024");
    }


  return procedure;
}

static GimpValueArray *
iff_load (GimpProcedure         *procedure,
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

  image = load_image (file, G_OBJECT (config), FALSE, run_mode, &error);

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
iff_load_thumb (GimpProcedure       *procedure,
                GFile               *file,
                gint                 size,
                GimpProcedureConfig *config,
                gpointer             run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), TRUE, GIMP_RUN_NONINTERACTIVE,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);
  GIMP_VALUES_SET_INT   (return_vals, 2, gimp_image_get_width  (image));
  GIMP_VALUES_SET_INT   (return_vals, 3, gimp_image_get_height (image));

  gimp_value_array_truncate (return_vals, 4);

  return return_vals;
}

static GimpImage *
load_image (GFile        *file,
            GObject      *config,
            gboolean      is_thumbnail,
            GimpRunMode   run_mode,
            GError      **error)
{
  GimpImage   *image = NULL;
  GimpLayer   *layer;
  GeglBuffer  *buffer;
  FILE        *fp;
  guint        imagesLength;
  IFF_Chunk   *chunk;
  ILBM_Image **iff_image;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fclose (fp);

  chunk = ILBM_read (g_file_peek_path (file));
  iff_image = ILBM_extractImages (chunk, &imagesLength);

  for (gint i = 0; i < imagesLength; i++)
    {
      ILBM_Image        *true_image   = iff_image[i];
      /* Struct representing bitmap header properties */
      ILBM_BitMapHeader *bitMapHeader;
      /* Struct containing the color palette */
      ILBM_ColorMap     *colorMap;
      ILBM_Viewport     *camg;
      IFF_UByte         *bitplanes;
      GimpImageType      image_type;
      guchar             gimp_cmap[768];  /* Max index is (2^nplanes) - 1 */
      gint               width;
      gint               height;
      gint               nPlanes;
      gint               palette_size = 0;
      gint               row_length;
      gint               pixel_size   = 1;
      gint               y_height     = 0;
      gint               aspect_x     = 0;
      gint               aspect_y     = 0;
      gboolean           ehb_mode     = FALSE;
      gboolean           ham_mode     = FALSE;

      if (! true_image)
        {
          g_message (_("Invalid or missing ILBM image"));
          return image;
        }

      colorMap = true_image->colorMap;
      camg     = true_image->viewport;

      /* Convert ACBM files to ILBM format */
      if (ILBM_imageIsACBM (true_image))
        ILBM_convertACBMToILBM (true_image);

      bitMapHeader = true_image->bitMapHeader;
      if (! bitMapHeader || ! true_image->body)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("ILBM contains no image data - likely a palette "
                         "file"));
          return NULL;
        }

      width      = bitMapHeader->w;
      height     = bitMapHeader->h;
      nPlanes    = bitMapHeader->nPlanes;
      row_length = (width + 15) / 16;
      pixel_size = nPlanes / 8;
      aspect_x   = bitMapHeader->xAspect;
      aspect_y   = bitMapHeader->yAspect;

      /* Check for ILBM variants in CMAG chunk */
      if (camg)
        {
          if (camg->viewportMode & (1 << 7))
            ehb_mode = TRUE;
          if (camg->viewportMode & (1 << 11) &&
              (nPlanes >= 5 && nPlanes <= 8))
            ham_mode = TRUE;
        }

      /* Load palette if it exists */
      if (colorMap)
        {
          palette_size = colorMap->colorRegisterLength;

          if (palette_size < 0 || palette_size > 256)
            {
              g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                           _("Invalid ILBM colormap size"));
              return NULL;
            }

          for (gint j = 0; j < palette_size; j++)
            {
              gimp_cmap[j * 3]     = colorMap->colorRegister[j].red;
              gimp_cmap[j * 3 + 1] = colorMap->colorRegister[j].green;
              gimp_cmap[j * 3 + 2] = colorMap->colorRegister[j].blue;
            }

          if (ehb_mode)
            {
              /* EHB mode adds 32 more colors. Each are half the RGB values
               * of the first 32 colors */
              for (gint j = 0; j < palette_size * 2; j++)
                {
                  gint offset_index = j + 32;

                  gimp_cmap[offset_index * 3]     =
                    colorMap->colorRegister[j].red / 2;
                  gimp_cmap[offset_index * 3 + 1] =
                    colorMap->colorRegister[j].green / 2;
                  gimp_cmap[offset_index * 3 + 2] =
                    colorMap->colorRegister[j].blue / 2;
                }
              /* EHB mode always has 64 colors */
              palette_size = 64;
            }
        }

      if (ham_mode)
        pixel_size = 3;

      if (pixel_size == 4)
        {
          image_type = GIMP_RGBA_IMAGE;
        }
      else if (pixel_size == 3)
        {
          image_type = GIMP_RGB_IMAGE;
        }
      else
        {
          pixel_size = 1;
          image_type = GIMP_INDEXED_IMAGE;
        }

      ILBM_unpackByteRun (true_image);

      if (! image)
        image = gimp_image_new (width, height,
                                pixel_size == 1 ? GIMP_INDEXED : GIMP_RGB);

      layer = gimp_layer_new (image, _("Background"), width, height,
                              image_type, 100,
                              gimp_image_get_default_new_layer_mode (image));
      gimp_image_insert_layer (image, layer, NULL, 0);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      bitplanes = true_image->body->chunkData;

      /* Setting resolution for non-square pixel aspect ratios */
      if (aspect_x != aspect_y && aspect_x > 0 && aspect_y > 0)
        {
          gdouble image_xres;
          gdouble image_yres;
          gfloat  ratio = (gfloat) aspect_x / aspect_y;

          if (! is_thumbnail)
            g_message (_("Non-square pixels. Image might look squashed if "
                         "Dot for Dot mode is enabled."));

          gimp_image_get_resolution (image, &image_xres, &image_yres);
          if (ratio < 1)
            image_xres = image_yres * (1 / ratio);
          else
            image_yres = image_xres * ratio;
          gimp_image_set_resolution (image, image_xres, image_yres);
        }

      /* Loading rows */
      for (gint j = 0; j < height; j++)
        {
          guchar *pixel_row;

          pixel_row = g_malloc (width * pixel_size * sizeof (guchar));

          /* PBM uses one byte per pixel index */
          if (ILBM_imageIsPBM (true_image))
            pbm_row (bitplanes, pixel_row, width);
          else if (pixel_size == 1)
            deleave_indexed_row (bitplanes, pixel_row, width, nPlanes);
          else if (ham_mode)
            deleave_ham_row (gimp_cmap, bitplanes, pixel_row, width, nPlanes);
          else
            deleave_rgb_row (bitplanes, pixel_row, width, nPlanes, pixel_size);

          bitplanes += (row_length * 2 * nPlanes);

          gegl_buffer_set (buffer, GEGL_RECTANGLE (0, y_height, width, 1), 0,
                           NULL, pixel_row, GEGL_AUTO_ROWSTRIDE);

          y_height++;
          g_free (pixel_row);
        }

      if (pixel_size == 1)
        gimp_palette_set_colormap (gimp_image_get_palette (image), babl_format ("R'G'B' u8"), gimp_cmap, palette_size * 3);

      g_object_unref (buffer);
    }
  ILBM_freeImages (iff_image, imagesLength);

  return image;
}

static void
deleave_indexed_row (IFF_UByte *bitplanes,
                     guchar    *pixel_row,
                     gint       width,
                     gint       nPlanes)
{
  guchar index[width];
  gint   row_length = ((width + 15) / 16) * 2;

  /* Initialize index array */
  for (gint i = 0; i < width; i++)
    index[i] = 0;

  /* Deleave rows */
  for (gint i = 0; i < row_length; i++)
    {
      for (gint j = 0; j < 8; j++)
        {
          guint8 bitmask = (1 << (8 - j)) - (1 << (7 - j));

          for (gint k = 0; k < nPlanes; k++)
            {
              guint8 update = (1 << (k + 1)) - (1 << (k));

              if (bitplanes[i + (row_length * k)] & bitmask)
                index[j + (i * 8)] += update;
            }
        }
    }

  /* Associate palette with pixels */
  for (gint i = 0; i < width; i++)
    pixel_row[i] = index[i];
}

static void
deleave_ham_row (const guchar *gimp_cmap,
                 IFF_UByte    *bitplanes,
                 guchar       *pixel_row,
                 gint          width,
                 gint          nPlanes)
{
  const gint control_index[3] = {2, 0, 1};
  const gint row_length       = ((width + 15) / 16) * 2;
  gint       prior_rgb[3]     = {0, 0, 0};
  gint       current_index    = 0;

  /* Deleave rows */
  for (gint i = 0; i < row_length; i++)
    {
      for (gint j = 0; j < 8; j++)
        {
          guint8 bitmask = (1 << (8 - j)) - (1 << (7 - j));
          guint8 control = 0;
          guint8 color   = 0;
          guint8 index   = 0;

          for (gint k = 0; k < nPlanes; k++)
            {
              if (bitplanes[i + (row_length * k)] & bitmask)
                {
                  gint limit = nPlanes < 7 ? 4 : 6;

                  /* The last two planes are control values.
                   * Everything else is either an index or a color.
                   * For HAM 5/6 colors, we use the 4 bits as both
                   * upper and lower bit modifiers. For HAM 7/8,
                   * we replace the 6 MSB with the color value. */
                  if (k < limit)
                    {
                      gint update = 1 << k;

                      index += update;
                      if (limit == 4)
                        color += update + (update << 4);
                      else
                        color += update;
                    }
                  else
                    {
                      control += 1 << (k - limit);
                    }
                }
            }

          if (control == 0)
            {
              prior_rgb[0] = gimp_cmap[index * 3];
              prior_rgb[1] = gimp_cmap[index * 3 + 1];
              prior_rgb[2] = gimp_cmap[index * 3 + 2];
            }
          else
            {
              /* Determines which RGB component to modify */
              gint modify = control_index[control - 1];

              if (nPlanes < 7)
                prior_rgb[modify] = color;
              else
                prior_rgb[modify] = (color << 2) + (prior_rgb[modify] & 3);
            }

          pixel_row[current_index * 3]     = prior_rgb[0];
          pixel_row[current_index * 3 + 1] = prior_rgb[1];
          pixel_row[current_index * 3 + 2] = prior_rgb[2];

          current_index++;
        }
    }
}

static void
deleave_rgb_row (IFF_UByte  *bitplanes,
                     guchar *pixel_row,
                     gint    width,
                     gint    nPlanes,
                     gint    pixel_size)
{
  gint row_length    = ((width + 15) / 16) * 2;
  gint current_pixel = 0;

  /* Initialize index array */
  for (gint i = 0; i < (width * pixel_size); i++)
    pixel_row[i] = 0;

  /* Deleave rows */
  for (gint i = 0; i < row_length; i++)
    {
      for (gint j = 0; j < 8; j++)
        {
          guint8 bitmask = (1 << (8 - j)) - (1 << (7 - j));

          for (gint k = 0; k < pixel_size; k++)
            {
              for (gint l = 0; l < 8; l++)
                {
                  guint8 update = (1 << (l + 1)) - (1 << (l));

                  if (bitplanes[i + (row_length * (l + k * 8))] & bitmask)
                    pixel_row[current_pixel] += update;
                }
              current_pixel++;
            }
        }
    }
}

static void
pbm_row (IFF_UByte *bitplanes,
         guchar    *pixel_row,
         gint       width)
{
  for (gint i = 0; i < width; i++)
    pixel_row[i] = bitplanes[i];
}
