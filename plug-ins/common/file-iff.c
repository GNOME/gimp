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

/*   IFF and ILBM chunk reading and decompressing algorithms referenced
 *   from libiff and libilbm by Sander van der Burg:
 *   https://github.com/svanderburg/libilbm
 *
 *   RGB8 and RGBN reading code referenced from GIMP IFF-ILBM plug-in by
 *   Emil Brink and Johannes Tevessen:
 *   https://github.com/unwind/gimpilbm
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

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


typedef struct
{
  gushort width;
  gushort height;
  gshort  offset_x;
  gshort  offset_y;
  guchar  n_planes;
  guchar  mask_type;
  guchar  compression;
  guchar  reserved;
  gushort transparent_color;
  guchar  aspect_x;
  guchar  aspect_y;
  gshort  page_width;
  gshort  page_height;
} BMHD;

#define convert_4_to_8_bit(val4)	((((val4) << 4) * (gint16) 17) / 16)


GType                   iff_get_type         (void);


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

static void             set_resolution       (GimpImage             *image,
                                              BMHD                  *bmhd);
static void             update_palette       (guint32               *cmap_size,
                                              guchar                *cmap,
                                              GError               **error);
static void             get_layer_settings   (guint32               *pixel_size,
                                              GimpImageType         *layer_type,
                                              gboolean               ham_mode,
                                              BMHD                  *bmhd);

static guchar         * convert_acbm         (guchar                *body,
                                              guint32                body_size,
                                              BMHD                  *bmhd,
                                              GError               **error);
static guchar         * decompress_pixels    (guchar                *body,
                                              guint32                body_size,
                                              BMHD                  *bmhd,
                                              GError               **error);
static guint32          get_row_length       (gint                   width);

static guchar         * deleave_indexed_row  (guchar                *bitplanes,
                                              gint                   width,
                                              gint                   n_planes);
static guchar         * deleave_rgb_row      (guchar                *bitplanes,
                                              gint                   width,
                                              gint                   n_planes,
                                              gint                   pixel_size);
static guchar         * deleave_ham_row      (const guchar          *gimp_cmap,
                                              guchar                *bitplanes,
                                              gint                   width,
                                              gint                   n_planes);

static guchar         * read_pbm_row         (guchar                *bitplanes,
                                              gint                   width);
static void             read_rgbn            (guchar                *data,
                                              guint32                data_size,
                                              gboolean               is_rgb8,
                                              GeglBuffer            *buffer,
                                              BMHD                  *bmhd);

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
                                        NULL,
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

      gimp_procedure_set_documentation (procedure,
                                        _("Load IFF file as thumbnail"),
                                        NULL,
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
  BMHD         bmhd;
  guchar       cmap[768];
  guint32      cmap_size;
  FILE        *fp;
  gchar        magic[5];
  guint32      file_size;
  gchar        type[5];
  guchar      *pixels = NULL;
  guint32      camg   = 0;

  gboolean     has_bmhd  = FALSE;
  gboolean     ehb_mode  = FALSE;
  gboolean     ham_mode  = FALSE;
  guint32      pixel_size;
  guint32      body_size;

  fp = g_fopen (g_file_peek_path (file), "rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (fread (magic, 1, 4, fp) < 1      ||
      fread (&file_size, 4, 1, fp) < 1 ||
      fread (type, 1, 4, fp)  < 1)
    {
      fclose (fp);
      g_set_error (error, G_FILE_ERROR, 0,
                   "Cannot open file for read: %s\n",
                   g_file_peek_path (file));
      return NULL;
    }
  file_size = GUINT32_FROM_BE (file_size);
  magic[4]  = '\0';
  type[4]   = '\0';

  /* Read chunks */
  while (TRUE)
    {
      gchar   chunk_type[5];
      guint32 chunk_size = 0;

      if (fread (chunk_type, 1, 4, fp) < 1 ||
          fread (&chunk_size, 4, 1, fp) < 1)
        break;

      chunk_type[4] = '\0';
      chunk_size    = GUINT32_FROM_BE (chunk_size);

      if (! strcmp (chunk_type, "BMHD"))
        {
          if (fread (&bmhd, sizeof (BMHD), 1, fp) == 0)
            break;

          has_bmhd    = TRUE;
          bmhd.width  = GUINT16_FROM_BE (bmhd.width);
          bmhd.height = GUINT16_FROM_BE (bmhd.height);
        }
      else if (! strcmp (chunk_type, "CMAP"))
        {
          cmap_size = chunk_size;

          if (fread (cmap, chunk_size, 1, fp) == 0)
            break;
        }
      else if (! strcmp (chunk_type, "CAMG"))
        {
          if (fread (&camg, 4, 1, fp) == 0)
            break;

          camg = GUINT32_FROM_BE (camg);
        }
      else if (! strcmp (chunk_type, "BODY"))
        {
          pixels    = g_try_malloc0 (chunk_size);
          body_size = chunk_size;

          if (fread (pixels, body_size, 1, fp) == 0)
            break;
        }
      else if (! strcmp (chunk_type, "ABIT")) /* ACBM body chunk */
        {
          pixels    = g_try_malloc0 (chunk_size);
          body_size = chunk_size;

          if (fread (pixels, body_size, 1, fp) == 0)
            break;
        }
      else
        {
          if (fseek (fp, chunk_size, SEEK_CUR) != 0)
            break;
        }

      /* Odd lengths are padded by 1 */
      if (chunk_size % 2 != 0)
        if (fseek (fp, 1, SEEK_CUR) != 0)
          break;
    }

  /* If we have valid data, then create image */
  if (has_bmhd && pixels)
    {
      GimpLayer     *layer;
      GeglBuffer    *buffer;
      GimpImageType  layer_type;
      guchar        *real_pixels;
      guint32        row_length = get_row_length (bmhd.width);

      /* Check for ILBM variants in CMAG chunk */
      if (camg)
        {
          if (camg & (1 << 7))
            ehb_mode = TRUE;

          if (camg & (1 << 11) &&
              (bmhd.n_planes >= 5 && bmhd.n_planes <= 8))
            ham_mode = TRUE;
        }

      /* Convert ACBM */
      if (! strcmp (type, "ACBM"))
        {
          guchar *acbm_pixels = convert_acbm (pixels, body_size, &bmhd, error);

          g_free (pixels);
          if (! acbm_pixels)
            {
              fclose (fp);
              return NULL;
            }
          pixels = acbm_pixels;

          bmhd.compression = 0; /* ACBM images are always uncompressed */
        }

      /* Check compression */
      if (bmhd.compression == 1)
        {
          real_pixels = decompress_pixels (pixels, body_size, &bmhd, error);

          g_free (pixels);
          if (! real_pixels)
            {
              fclose (fp);
              return NULL;
            }
        }
      else if (bmhd.compression == 0     ||
               ((bmhd.compression == 4)  &&
                (! strcmp (type, "RGBN") ||
                 ! strcmp (type, "RGB8"))))
        {
          real_pixels = pixels;
        }
      else
        {
          fclose (fp);
          return NULL;
        }

      /* Adjust settings */
      if (ehb_mode)
        update_palette (&cmap_size, cmap, error);

      if (error && *error)
        {
          fclose (fp);
          return NULL;
        }

      get_layer_settings (&pixel_size, &layer_type, ham_mode, &bmhd);

      /* Create image */
      image = gimp_image_new (bmhd.width, bmhd.height,
                              pixel_size == 1 ? GIMP_INDEXED : GIMP_RGB);

      /* Setting resolution for non-square pixel aspect ratios */
      set_resolution (image, &bmhd);

      layer = gimp_layer_new (image, _("Background"), bmhd.width, bmhd.height,
                              layer_type, 100,
                              gimp_image_get_default_new_layer_mode (image));
      gimp_image_insert_layer (image, layer, NULL, 0);

      if (pixel_size == 1)
        gimp_palette_set_colormap (gimp_image_get_palette (image),
                                   babl_format ("R'G'B' u8"),
                                   cmap, cmap_size);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      /* Draw image */
      if (! strcmp (type, "RGBN") || ! strcmp (type, "RGB8"))
        {
          read_rgbn (real_pixels, body_size, (! strcmp (type, "RGB8")), buffer,
                     &bmhd);
        }
      else
        {
          guchar *pointer   = real_pixels;
          guchar *pixel_row = NULL;

          for (gint j = 0; j < bmhd.height; j++)
            {
              if (! strcmp (type, "PBM "))
                pixel_row = read_pbm_row (pointer, bmhd.width);
              else if (pixel_size == 1)
                pixel_row = deleave_indexed_row (pointer, bmhd.width,
                                                 bmhd.n_planes);
              else if (ham_mode)
                pixel_row = deleave_ham_row (cmap, pointer, bmhd.width,
                                             bmhd.n_planes);
              else
                pixel_row = deleave_rgb_row (pointer, bmhd.width,
                                             bmhd.n_planes, pixel_size);

              pointer += (row_length * bmhd.n_planes);
              if (pixel_row)
                {
                  gegl_buffer_set (buffer,
                                   GEGL_RECTANGLE (0, j, bmhd.width, 1), 0,
                                   NULL, pixel_row, GEGL_AUTO_ROWSTRIDE);
                  g_free (pixel_row);
                }
            }
        }
      g_free (real_pixels);
      g_object_unref (buffer);
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("ILBM contains no image data - likely a palette "
                     "file"));
      fclose (fp);
      return NULL;
    }
  fclose (fp);

  return image;
}

/* Helper methods */
static void
set_resolution (GimpImage *image,
                BMHD      *bmhd)
{
  if (bmhd->aspect_x != bmhd->aspect_y &&
      bmhd->aspect_x > 0               &&
      bmhd->aspect_y > 0)
    {
      gdouble image_xres;
      gdouble image_yres;
      gfloat  ratio = (gfloat) bmhd->aspect_x / bmhd->aspect_y;

      gimp_image_get_resolution (image, &image_xres, &image_yres);
      if (ratio < 1)
        image_xres = image_yres * (1 / ratio);
      else
        image_yres = image_xres * ratio;
      gimp_image_set_resolution (image, image_xres, image_yres);
    }
}

static void
update_palette (guint32  *cmap_size,
                guchar   *cmap,
                GError  **error)
{
  /* EHB mode adds 32 more colors. Each are half the RGB values
   * of the first 32 colors */
  if (*cmap_size < 32)
    {
      g_set_error (error, G_FILE_ERROR,
                   g_file_error_from_errno (errno),
                   _("Invalid ILBM colormap size"));
      return;
    }
  else if (*cmap_size > 32)
    {
      *cmap_size = 32;
    }

  for (gint j = 0; j < *cmap_size * 2; j++)
    {
      gint offset_index = j + 32;

      cmap[offset_index * 3]     = cmap[j * 3] / 2;
      cmap[offset_index * 3 + 1] = cmap[(j * 3) + 1] / 2;
      cmap[offset_index * 3 + 2] = cmap[(j * 3) + 2] / 2;
    }
  /* EHB mode always has 64 colors in total */
  *cmap_size = 64;
}

static void
get_layer_settings (guint32       *pixel_size,
                    GimpImageType *layer_type,
                    gboolean       ham_mode,
                    BMHD          *bmhd)
{
  *pixel_size = bmhd->n_planes / 8;

  if (ham_mode)
    *pixel_size = 3;

  if (bmhd->n_planes == 13)
    *pixel_size = 3;
  else if (bmhd->n_planes == 25)
    *pixel_size = 4;

  if (*pixel_size == 4)
    {
      *layer_type = GIMP_RGBA_IMAGE;
    }
  else if (*pixel_size == 3)
    {
      *layer_type = GIMP_RGB_IMAGE;
    }
  else
    {
      *pixel_size = 1;
      *layer_type = GIMP_INDEXED_IMAGE;
    }
}

static guchar *
convert_acbm (guchar   *body,
              guint32   body_size,
              BMHD     *bmhd,
              GError  **error)
{
  guchar *pixels = NULL;
  gsize   alloc;
  guint32 offsets[32]; /* Maximum number of planes */
  guint32 offset      = 0;
  guint32 base_offset = 0;
  guint32 row_length  = get_row_length (bmhd->width);
  gsize   interleaved;
  gsize   plane_length;

  if (! g_size_checked_mul (&plane_length, row_length, bmhd->height)   ||
      ! g_size_checked_mul (&interleaved, row_length,  bmhd->n_planes) ||
      ! g_size_checked_mul (&alloc, interleaved, bmhd->height) ||
      ! (pixels = g_try_malloc0 (alloc)))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   bmhd->width, bmhd->height);
      return NULL;
    }

  for (gint i = 0; i < bmhd->n_planes; i++)
    {
      offsets[i] = offset;
      offset += plane_length;
    }

  for (gint i = 0; i < bmhd->n_planes; i++)
    {
      guint32 count = 0;

      offset = base_offset;
      for (gint j = 0; j < bmhd->height; j++)
        {
          if (offset > alloc ||
              ((offsets[i] + count) > body_size))
            break;

          memcpy (pixels + offset, body + offsets[i] + count, row_length);

          count  += row_length;
          offset += interleaved;
        }
      base_offset += row_length;
    }

  return pixels;
}

static guchar *
decompress_pixels (guchar   *body,
                   guint32   body_size,
                   BMHD     *bmhd,
                   GError  **error)
{
  guchar   *pixels;
  gsize     alloc        = 0;
  guint32   row_size     = get_row_length (bmhd->width);
  gsize     input_bytes  = 0;
  gsize     output_bytes = 0;
  gboolean  valid        = TRUE;

  if (! g_size_checked_mul (&alloc, row_size, bmhd->height) ||
      ! g_size_checked_mul (&alloc, alloc, bmhd->n_planes)  ||
      ! (pixels = g_try_malloc0 (alloc)))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   bmhd->width, bmhd->height);
      return NULL;
    }

  while (input_bytes < body_size && valid)
    {
      gint32 data = (gchar) body[input_bytes];

      input_bytes++;

      if (data >= 0 && data <= 127)
        {
          for (gint i = 0; i < data + 1; i++)
            {
              if (output_bytes >= alloc)
                {
                  valid = FALSE;
                  break;
                }

              pixels[output_bytes] = body[input_bytes];
              input_bytes++;
              output_bytes++;
            }
        }
      else if (data >= -127 && data <= -1)
        {
          guchar ubyte;

          if (input_bytes >= body_size)
            {
              valid = FALSE;
              break;
            }

          ubyte = body[input_bytes];
          input_bytes++;

          for (gint i = 0; i < -data + 1; i++)
            {
              if (output_bytes >= alloc)
                {
                  valid = FALSE;
                  break;
                }

              pixels[output_bytes] = ubyte;
              output_bytes++;
            }
        }
    }

  return pixels;
}

static guint32
get_row_length (gint width)
{
  guint32 row_length = width / 16;

  /* Handle odd widths */
  if (width % 16 != 0)
    row_length++;

  row_length *= 2;

  return row_length;
}

static guchar *
deleave_indexed_row (guchar *bitplanes,
                     gint    width,
                     gint    n_planes)
{
  gint    row_length = get_row_length (width);
  gsize   alloc;
  guchar *pixel_row;

  if (! g_size_checked_mul (&alloc, row_length, 8)   ||
      ! g_size_checked_mul (&alloc, alloc, n_planes) ||
      ! (pixel_row = g_try_malloc0 (alloc)))
    return NULL;

  /* Deleave rows */
  for (gint i = 0; i < row_length; i++)
    {
      for (gint j = 0; j < 8; j++)
        {
          guint8 bitmask = (1 << (8 - j)) - (1 << (7 - j));

          for (gint k = 0; k < n_planes; k++)
            {
              guint8 update = (1 << (k + 1)) - (1 << (k));

              if (bitplanes[i + (row_length * k)] & bitmask)
                pixel_row[j + (i * 8)] += update;
            }
        }
    }

  return pixel_row;
}

static guchar *
deleave_rgb_row (guchar *bitplanes,
                 gint    width,
                 gint    n_planes,
                 gint    pixel_size)
{
  gint    row_length    = get_row_length (width);
  gint    current_pixel = 0;
  gsize   alloc         = 0;
  guchar *pixel_row;

  if (! g_size_checked_mul (&alloc, row_length, 8)     ||
      ! g_size_checked_mul (&alloc, alloc, pixel_size) ||
      ! (pixel_row = g_try_malloc0 (alloc)))
    return NULL;

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

  return pixel_row;
}

static guchar *
deleave_ham_row (const guchar *gimp_cmap,
                 guchar       *bitplanes,
                 gint          width,
                 gint          n_planes)
{
  const gint  control_index[3] = {2, 0, 1};
  const gint  row_length       = get_row_length (width);
  gint        prior_rgb[3]     = {0, 0, 0};
  gint        current_index    = 0;
  gsize       allocation       = 0;
  guchar     *pixel_row;

  if (! g_size_checked_mul (&allocation, width, 3)             ||
      ! g_size_checked_mul (&allocation, allocation, n_planes) ||
      ! g_size_checked_mul (&allocation, allocation, 4)        ||
      ! (pixel_row = g_try_malloc0 (allocation)))
    return NULL;

  /* Deleave rows */
  for (gint i = 0; i < row_length; i++)
    {
      for (gint j = 0; j < 8; j++)
        {
          guint8 bitmask = (1 << (8 - j)) - (1 << (7 - j));
          guint8 control = 0;
          guint8 color   = 0;
          guint8 index   = 0;

          for (gint k = 0; k < n_planes; k++)
            {
              if (bitplanes[i + (row_length * k)] & bitmask)
                {
                  gint limit = n_planes < 7 ? 4 : 6;

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

              if (n_planes < 7)
                prior_rgb[modify] = color;
              else
                prior_rgb[modify] = (color << 2) + (prior_rgb[modify] & 3);
            }

          pixel_row[current_index]     = prior_rgb[0];
          pixel_row[current_index + 1] = prior_rgb[1];
          pixel_row[current_index + 2] = prior_rgb[2];

          current_index += 3;
        }
    }

  return pixel_row;
}

static guchar *
read_pbm_row (guchar *bitplanes,
              gint    width)
{
  guchar * pixel_row = g_malloc0 (width);

  for (gint i = 0; i < width; i++)
    pixel_row[i] = bitplanes[i];

  return pixel_row;
}

static void
read_rgbn (guchar     *data,
           guint32     data_size,
           gboolean    is_rgb8,
           GeglBuffer *buffer,
           BMHD       *bmhd)
{
  guchar   *pixels    = NULL;
  gboolean  has_alpha = FALSE;
  guint32   i         = 0;
  guint32   j         = 0;
  guint32   count     = data_size;
  guint32   pixel_size;
  gsize     alloc;

  if (bmhd->n_planes == 25)
    has_alpha = TRUE;

  pixel_size = (has_alpha) ? 4 : 3;

  if (! g_size_checked_mul (&alloc, bmhd->width, bmhd->height) ||
      ! g_size_checked_mul (&alloc, alloc, pixel_size)         ||
      ! (pixels = g_try_malloc0 (alloc)))
    return;

  while (count > 0 && i < (data_size - 2))
    {
      guchar  rgb[4];
      guint32 repeat;

      if (is_rgb8)
        {
          guint32 input;

          if (i >= (data_size - 3))
            break;

          input = (((guint32) data[i]) << 24)     |
                  (((guint32) data[i + 1]) << 16) |
                  (((guint32) data[i + 2]) << 8)  |
                  data[i + 3];
          i += 4;

          rgb[0] = input >> 24;
          rgb[1] = (input >> 16) & 0xFF;
          rgb[2] = (input >> 8) & 0xFF;
          if (has_alpha)
          	rgb[3] = ((input >> 7) & 1) ? 0 : 255;

          repeat = input & 0x7F;
        }
      else
        {
          guint16 input = (((gushort) data[i]) << 8) | data[i + 1];

          i += 2;

          rgb[0] = convert_4_to_8_bit (input >> 12);
          rgb[1] = convert_4_to_8_bit ((input >> 8) & 0xF);
          rgb[2] = convert_4_to_8_bit ((input >> 4) & 0xF);
          if (has_alpha)
            rgb[3] = ((input >> 3) & 1) ? 0 : 255;

          repeat = input & 7;
        }

      if (! repeat)
        {
          if (i >= data_size)
            break;

          repeat = data[i];
          i++;

          if (! repeat)
            {
              if (i >= (data_size - 1))
                break;

              repeat = (((gushort) data[i]) << 8) | data[i + 1];
              i += 2;

              if (! repeat)
                repeat = 65536;
            }
        }

      while (count && repeat)
        {
          if (j >= alloc - 2)
            break;

          pixels[j++] = rgb[0];
          pixels[j++] = rgb[1];
          pixels[j++] = rgb[2];
          if (has_alpha && j <= alloc)
            pixels[j++] = rgb[3];

          count--;
          repeat--;
        }
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, bmhd->width, bmhd->height), 0,
                 NULL, pixels, GEGL_AUTO_ROWSTRIDE);
}
