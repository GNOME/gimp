/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-gegl-tile-compat.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable-private.h" /* eek */
#include "core/gimpgrid.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-metadata.h"
#include "core/gimpimage-private.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpimage-undo.h"
#include "core/gimpitemstack.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimplayermask.h"
#include "core/gimpparasitelist.h"
#include "core/gimpprogress.h"
#include "core/gimpselection.h"
#include "core/gimptemplate.h"

#include "text/gimptextlayer.h"
#include "text/gimptextlayer-xcf.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpstroke.h"
#include "vectors/gimpbezierstroke.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpvectors-compat.h"

#include "xcf-private.h"
#include "xcf-load.h"
#include "xcf-read.h"
#include "xcf-seek.h"

#include "gimp-intl.h"


#define MAX_XCF_PARASITE_DATA_LEN (256L * 1024 * 1024)

/* #define GIMP_XCF_PATH_DEBUG */


static void            xcf_load_add_masks     (GimpImage     *image);
static gboolean        xcf_load_image_props   (XcfInfo       *info,
                                               GimpImage     *image);
static gboolean        xcf_load_layer_props   (XcfInfo       *info,
                                               GimpImage     *image,
                                               GimpLayer    **layer,
                                               GList        **item_path,
                                               gboolean      *apply_mask,
                                               gboolean      *edit_mask,
                                               gboolean      *show_mask,
                                               guint32       *text_layer_flags,
                                               guint32       *group_layer_flags);
static gboolean        xcf_load_channel_props (XcfInfo       *info,
                                               GimpImage     *image,
                                               GimpChannel  **channel);
static gboolean        xcf_load_prop          (XcfInfo       *info,
                                               PropType      *prop_type,
                                               guint32       *prop_size);
static GimpLayer     * xcf_load_layer         (XcfInfo       *info,
                                               GimpImage     *image,
                                               GList        **item_path);
static GimpChannel   * xcf_load_channel       (XcfInfo       *info,
                                               GimpImage     *image);
static GimpLayerMask * xcf_load_layer_mask    (XcfInfo       *info,
                                               GimpImage     *image);
static gboolean        xcf_load_buffer        (XcfInfo       *info,
                                               GeglBuffer    *buffer);
static gboolean        xcf_load_level         (XcfInfo       *info,
                                               GeglBuffer    *buffer);
static gboolean        xcf_load_tile          (XcfInfo       *info,
                                               GeglBuffer    *buffer,
                                               GeglRectangle *tile_rect,
                                               const Babl    *format);
static gboolean        xcf_load_tile_rle      (XcfInfo       *info,
                                               GeglBuffer    *buffer,
                                               GeglRectangle *tile_rect,
                                               const Babl    *format,
                                               gint           data_length);
static GimpParasite  * xcf_load_parasite      (XcfInfo       *info);
static gboolean        xcf_load_old_paths     (XcfInfo       *info,
                                               GimpImage     *image);
static gboolean        xcf_load_old_path      (XcfInfo       *info,
                                               GimpImage     *image);
static gboolean        xcf_load_vectors       (XcfInfo       *info,
                                               GimpImage     *image);
static gboolean        xcf_load_vector        (XcfInfo       *info,
                                               GimpImage     *image);

static gboolean        xcf_skip_unknown_prop  (XcfInfo       *info,
                                               gsize          size);


#define xcf_progress_update(info) G_STMT_START  \
  {                                             \
    if (info->progress)                         \
      gimp_progress_pulse (info->progress);     \
  } G_STMT_END


GimpImage *
xcf_load_image (Gimp     *gimp,
                XcfInfo  *info,
                GError  **error)
{
  GimpImage          *image = NULL;
  const GimpParasite *parasite;
  gboolean            has_metadata = FALSE;
  guint32             saved_pos;
  guint32             offset;
  gint                width;
  gint                height;
  gint                image_type;
  GimpPrecision       precision = GIMP_PRECISION_U8_GAMMA;
  gint                num_successful_elements = 0;

  /* read in the image width, height and type */
  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &image_type, 1);

  if (info->file_version >= 4)
    {
      gint p;

      info->cp += xcf_read_int32 (info->input, (guint32 *) &p, 1);

      if (info->file_version == 4)
        {
          switch (p)
            {
            case 0: precision = GIMP_PRECISION_U8_GAMMA;     break;
            case 1: precision = GIMP_PRECISION_U16_GAMMA;    break;
            case 2: precision = GIMP_PRECISION_U32_LINEAR;   break;
            case 3: precision = GIMP_PRECISION_HALF_LINEAR;  break;
            case 4: precision = GIMP_PRECISION_FLOAT_LINEAR; break;
            default:
              goto hard_error;
            }
        }
      else if (info->file_version == 5 ||
               info->file_version == 6)
        {
          switch (p)
            {
            case 100: precision = GIMP_PRECISION_U8_LINEAR; break;
            case 150: precision = GIMP_PRECISION_U8_GAMMA; break;
            case 200: precision = GIMP_PRECISION_U16_LINEAR; break;
            case 250: precision = GIMP_PRECISION_U16_GAMMA; break;
            case 300: precision = GIMP_PRECISION_U32_LINEAR; break;
            case 350: precision = GIMP_PRECISION_U32_GAMMA; break;
            case 400: precision = GIMP_PRECISION_HALF_LINEAR; break;
            case 450: precision = GIMP_PRECISION_HALF_GAMMA; break;
            case 500: precision = GIMP_PRECISION_FLOAT_LINEAR; break;
            case 550: precision = GIMP_PRECISION_FLOAT_GAMMA; break;
            default:
              goto hard_error;
            }
        }
      else
        {
          precision = p;
        }
    }

  image = gimp_create_image (gimp, width, height, image_type, precision,
                             FALSE);

  gimp_image_undo_disable (image);

  xcf_progress_update (info);

  /* read the image properties */
  if (! xcf_load_image_props (info, image))
    goto hard_error;

  /* check for a GimpGrid parasite */
  parasite = gimp_image_parasite_find (GIMP_IMAGE (image),
                                       gimp_grid_parasite_name ());
  if (parasite)
    {
      GimpGrid *grid = gimp_grid_from_parasite (parasite);

      if (grid)
        {
          GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

          gimp_parasite_list_remove (private->parasites,
                                     gimp_parasite_name (parasite));

          gimp_image_set_grid (GIMP_IMAGE (image), grid, FALSE);
          g_object_unref (grid);
        }
    }

  /* check for a metadata parasite */
  parasite = gimp_image_parasite_find (GIMP_IMAGE (image),
                                       "gimp-image-metadata");
  if (parasite)
    {
      GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
      GimpMetadata     *metadata;
      const gchar      *meta_string;

      meta_string = (gchar *) gimp_parasite_data (parasite);
      metadata = gimp_metadata_deserialize (meta_string);

      if (metadata)
        {
          has_metadata = TRUE;

          gimp_image_set_metadata (image, metadata, FALSE);
          g_object_unref (metadata);
        }

      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (parasite));
    }

  /* migrate the old "exif-data" parasite */
  parasite = gimp_image_parasite_find (GIMP_IMAGE (image),
                                       "exif-data");
  if (parasite)
    {
      GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

      if (has_metadata)
        {
          g_printerr ("xcf-load: inconsistent metadata discovered: XCF file "
                      "has both 'gimp-image-metadata' and 'exif-data' "
                      "parasites, dropping old 'exif-data'\n");
        }
      else
        {
          GimpMetadata *metadata = gimp_image_get_metadata (image);
          GError       *my_error = NULL;

          if (metadata)
            g_object_ref (metadata);
          else
            metadata = gimp_metadata_new ();

          if (! gimp_metadata_set_from_exif (metadata,
                                             gimp_parasite_data (parasite),
                                             gimp_parasite_data_size (parasite),
                                             &my_error))
            {
              gimp_message (gimp, G_OBJECT (info->progress),
                            GIMP_MESSAGE_WARNING,
                            _("Corrupt 'exif-data' parasite discovered.\n"
                              "Exif data could not be migrated: %s"),
                            my_error->message);
              g_clear_error (&my_error);
            }
          else
            {
              gimp_image_set_metadata (image, metadata, FALSE);
            }

          g_object_unref (metadata);
        }

      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (parasite));
    }

  /* migrate the old "gimp-metadata" parasite */
  parasite = gimp_image_parasite_find (GIMP_IMAGE (image),
                                       "gimp-metadata");
  if (parasite)
    {
      GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

      if (has_metadata)
        {
          g_printerr ("xcf-load: inconsistent metadata discovered: XCF file "
                      "has both 'gimp-image-metadata' and 'gimp-metadata' "
                      "parasites, dropping old 'gimp-metadata'\n");
        }
      else
        {
          GimpMetadata *metadata = gimp_image_get_metadata (image);
          GError       *my_error = NULL;

          if (metadata)
            g_object_ref (metadata);
          else
            metadata = gimp_metadata_new ();

          if (! gimp_metadata_set_from_xmp (metadata,
                                            gimp_parasite_data (parasite),
                                            gimp_parasite_data_size (parasite),
                                            &my_error))
            {
              gimp_message (gimp, G_OBJECT (info->progress),
                            GIMP_MESSAGE_WARNING,
                            _("Corrupt 'gimp-metadata' parasite discovered.\n"
                              "XMP data could not be migrated: %s"),
                            my_error->message);
              g_clear_error (&my_error);
            }
          else
            {
              gimp_image_set_metadata (image, metadata, FALSE);
            }

          g_object_unref (metadata);
        }

      gimp_parasite_list_remove (private->parasites,
                                 gimp_parasite_name (parasite));
    }

  xcf_progress_update (info);

  while (TRUE)
    {
      GimpLayer *layer;
      GList     *item_path = NULL;

      /* read in the offset of the next layer */
      info->cp += xcf_read_int32 (info->input, &offset, 1);

      /* if the offset is 0 then we are at the end
       *  of the layer list.
       */
      if (offset == 0)
        break;

      /* save the current position as it is where the
       *  next layer offset is stored.
       */
      saved_pos = info->cp;

      /* seek to the layer offset */
      if (! xcf_seek_pos (info, offset, NULL))
        goto error;

      /* read in the layer */
      layer = xcf_load_layer (info, image, &item_path);
      if (!layer)
        goto error;

      num_successful_elements++;

      xcf_progress_update (info);

      /* add the layer to the image if its not the floating selection */
      if (layer != info->floating_sel)
        {
          GimpContainer *layers = gimp_image_get_layers (image);
          GimpContainer *container;
          GimpLayer     *parent;

          if (item_path)
            {
              if (info->floating_sel)
                {
                  /* there is a floating selection, but it will get
                   * added after all layers are loaded, so toplevel
                   * layer indices are off-by-one. Adjust item paths
                   * accordingly:
                   */
                  gint toplevel_index;

                  toplevel_index = GPOINTER_TO_UINT (item_path->data);

                  toplevel_index--;

                  item_path->data = GUINT_TO_POINTER (toplevel_index);
                }

              parent = GIMP_LAYER
                (gimp_item_stack_get_parent_by_path (GIMP_ITEM_STACK (layers),
                                                     item_path,
                                                     NULL));

              container = gimp_viewable_get_children (GIMP_VIEWABLE (parent));

              g_list_free (item_path);
            }
          else
            {
              parent    = NULL;
              container = layers;
            }

          gimp_image_add_layer (image, layer,
                                parent,
                                gimp_container_get_n_children (container),
                                FALSE);
        }

      /* restore the saved position so we'll be ready to
       *  read the next offset.
       */
      if (! xcf_seek_pos (info, saved_pos, NULL))
        goto error;
    }

  while (TRUE)
    {
      GimpChannel *channel;

      /* read in the offset of the next channel */
      info->cp += xcf_read_int32 (info->input, &offset, 1);

      /* if the offset is 0 then we are at the end
       *  of the channel list.
       */
      if (offset == 0)
        break;

      /* save the current position as it is where the
       *  next channel offset is stored.
       */
      saved_pos = info->cp;

      /* seek to the channel offset */
      if (! xcf_seek_pos (info, offset, NULL))
        goto error;

      /* read in the channel */
      channel = xcf_load_channel (info, image);
      if (!channel)
        goto error;

      num_successful_elements++;

      xcf_progress_update (info);

      /* add the channel to the image if its not the selection */
      if (channel != gimp_image_get_mask (image))
        gimp_image_add_channel (image, channel,
                                NULL, /* FIXME tree */
                                gimp_container_get_n_children (gimp_image_get_channels (image)),
                                FALSE);

      /* restore the saved position so we'll be ready to
       *  read the next offset.
       */
      if (! xcf_seek_pos (info, saved_pos, NULL))
        goto error;
    }

  xcf_load_add_masks (image);

  if (info->floating_sel && info->floating_sel_drawable)
    floating_sel_attach (info->floating_sel, info->floating_sel_drawable);

  if (info->active_layer)
    gimp_image_set_active_layer (image, info->active_layer);

  if (info->active_channel)
    gimp_image_set_active_channel (image, info->active_channel);

  gimp_image_set_filename (image, info->filename);

  if (info->tattoo_state > 0)
    gimp_image_set_tattoo_state (image, info->tattoo_state);

  gimp_image_undo_enable (image);

  return image;

 error:
  if (num_successful_elements == 0)
    goto hard_error;

  gimp_message_literal (gimp, G_OBJECT (info->progress), GIMP_MESSAGE_WARNING,
                        _("This XCF file is corrupt!  I have loaded as much "
                          "of it as I can, but it is incomplete."));

  xcf_load_add_masks (image);

  gimp_image_undo_enable (image);

  return image;

 hard_error:
  g_set_error_literal (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("This XCF file is corrupt!  I could not even "
                         "salvage any partial image data from it."));

  if (image)
    g_object_unref (image);

  return NULL;
}

static void
xcf_load_add_masks (GimpImage *image)
{
  GList *layers;
  GList *list;

  layers = gimp_image_get_layer_list (image);

  for (list = layers; list; list = g_list_next (list))
    {
      GimpLayer     *layer = list->data;
      GimpLayerMask *mask;

      mask = g_object_get_data (G_OBJECT (layer), "gimp-layer-mask");

      if (mask)
        {
          gboolean apply_mask;
          gboolean edit_mask;
          gboolean show_mask;

          apply_mask = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (layer),
                                                           "gimp-layer-mask-apply"));
          edit_mask = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (layer),
                                                          "gimp-layer-mask-edit"));
          show_mask = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (layer),
                                                          "gimp-layer-mask-show"));

          gimp_layer_add_mask (layer, mask, FALSE, NULL);

          gimp_layer_set_apply_mask (layer, apply_mask, FALSE);
          gimp_layer_set_edit_mask  (layer, edit_mask);
          gimp_layer_set_show_mask  (layer, show_mask, FALSE);

          g_object_set_data (G_OBJECT (layer), "gimp-layer-mask",       NULL);
          g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-apply", NULL);
          g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-edit",  NULL);
          g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-show",  NULL);
        }
    }

  g_list_free (layers);
}

static gboolean
xcf_load_image_props (XcfInfo   *info,
                      GimpImage *image)
{
  PropType prop_type;
  guint32  prop_size;

  while (TRUE)
    {
      if (! xcf_load_prop (info, &prop_type, &prop_size))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;

        case PROP_COLORMAP:
          {
            guint32 n_colors;
            guchar  cmap[GIMP_IMAGE_COLORMAP_SIZE];

            info->cp += xcf_read_int32 (info->input, &n_colors, 1);

            if (n_colors > (GIMP_IMAGE_COLORMAP_SIZE / 3))
              {
                gimp_message (info->gimp, G_OBJECT (info->progress),
                              GIMP_MESSAGE_ERROR,
                              "Maximum colormap size (%d) exceeded",
                              GIMP_IMAGE_COLORMAP_SIZE);
                return FALSE;
              }

            if (info->file_version == 0)
              {
                gint i;

                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      _("XCF warning: version 0 of XCF file format\n"
                                        "did not save indexed colormaps correctly.\n"
                                        "Substituting grayscale map."));

                if (! xcf_seek_pos (info, info->cp + n_colors, NULL))
                  return FALSE;

                for (i = 0; i < n_colors; i++)
                  {
                    cmap[i * 3 + 0] = i;
                    cmap[i * 3 + 1] = i;
                    cmap[i * 3 + 2] = i;
                  }
              }
            else
              {
                info->cp += xcf_read_int8 (info->input, cmap, n_colors * 3);
              }

            /* only set color map if image is indexed, this is just
             * sanity checking to make sure gimp doesn't end up with
             * an image state that is impossible.
             */
            if (gimp_image_get_base_type (image) == GIMP_INDEXED)
              gimp_image_set_colormap (image, cmap, n_colors, FALSE);
          }
          break;

        case PROP_COMPRESSION:
          {
            guint8 compression;

            info->cp += xcf_read_int8 (info->input, (guint8 *) &compression, 1);

            if ((compression != COMPRESS_NONE) &&
                (compression != COMPRESS_RLE) &&
                (compression != COMPRESS_ZLIB) &&
                (compression != COMPRESS_FRACTAL))
              {
                gimp_message (info->gimp, G_OBJECT (info->progress),
                              GIMP_MESSAGE_ERROR,
                              "Unknown compression type: %d",
			      (gint) compression);
                return FALSE;
              }

            info->compression = compression;
          }
          break;

        case PROP_GUIDES:
          {
            GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);
            gint32            position;
            gint8             orientation;
            gint              i, nguides;

            nguides = prop_size / (4 + 1);
            for (i = 0; i < nguides; i++)
              {
                info->cp += xcf_read_int32 (info->input,
                                            (guint32 *) &position, 1);
                info->cp += xcf_read_int8 (info->input,
                                           (guint8 *) &orientation, 1);

                /*  skip -1 guides from old XCFs  */
                if (position < 0)
                  continue;

                switch (orientation)
                  {
                  case XCF_ORIENTATION_HORIZONTAL:
                    gimp_image_add_hguide (image, position, FALSE);
                    break;

                  case XCF_ORIENTATION_VERTICAL:
                    gimp_image_add_vguide (image, position, FALSE);
                    break;

                  default:
                    gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                          GIMP_MESSAGE_WARNING,
                                          "Guide orientation out of range in XCF file");
                    continue;
                  }
              }

            /*  this is silly as the order of guides doesn't really matter,
             *  but it restores the list to its original order, which
             *  cannot be wrong  --Mitch
             */
            private->guides = g_list_reverse (private->guides);
          }
          break;

        case PROP_SAMPLE_POINTS:
          {
            gint32 x, y;
            gint   i, n_sample_points;

            n_sample_points = prop_size / (4 + 4);
            for (i = 0; i < n_sample_points; i++)
              {
                info->cp += xcf_read_int32 (info->input, (guint32 *) &x, 1);
                info->cp += xcf_read_int32 (info->input, (guint32 *) &y, 1);

                gimp_image_add_sample_point_at_pos (image, x, y, FALSE);
              }
          }
          break;

        case PROP_RESOLUTION:
          {
            gfloat xres, yres;

            info->cp += xcf_read_float (info->input, &xres, 1);
            info->cp += xcf_read_float (info->input, &yres, 1);

            if (xres < GIMP_MIN_RESOLUTION || xres > GIMP_MAX_RESOLUTION ||
                yres < GIMP_MIN_RESOLUTION || yres > GIMP_MAX_RESOLUTION)
              {
                GimpTemplate *template = image->gimp->config->default_image;

                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      "Warning, resolution out of range in XCF file");
                xres = gimp_template_get_resolution_x (template);
                yres = gimp_template_get_resolution_y (template);
              }

            gimp_image_set_resolution (image, xres, yres);
          }
          break;

        case PROP_TATTOO:
          {
            info->cp += xcf_read_int32 (info->input, &info->tattoo_state, 1);
          }
          break;

        case PROP_PARASITES:
          {
            glong base = info->cp;

            while (info->cp - base < prop_size)
              {
                GimpParasite *p     = xcf_load_parasite (info);
                GError       *error = NULL;

                if (! p)
                  return FALSE;

                if (! gimp_image_parasite_validate (image, p, &error))
                  {
                    gimp_message (info->gimp, G_OBJECT (info->progress),
                                  GIMP_MESSAGE_WARNING,
                                  "Warning, invalid image parasite in XCF file: %s",
                                  error->message);
                    g_clear_error (&error);
                  }
                else
                  {
                    gimp_image_parasite_attach (image, p);
                  }

                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Error while loading an image's parasites");
          }
          break;

        case PROP_UNIT:
          {
            guint32 unit;

            info->cp += xcf_read_int32 (info->input, &unit, 1);

            if ((unit <= GIMP_UNIT_PIXEL) ||
                (unit >= gimp_unit_get_number_of_built_in_units ()))
              {
                gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                      GIMP_MESSAGE_WARNING,
                                      "Warning, unit out of range in XCF file, "
                                      "falling back to inches");
                unit = GIMP_UNIT_INCH;
              }

            gimp_image_set_unit (image, unit);
          }
          break;

        case PROP_PATHS:
          xcf_load_old_paths (info, image);
          break;

        case PROP_USER_UNIT:
          {
            gchar    *unit_strings[5];
            float     factor;
            guint32   digits;
            GimpUnit  unit;
            gint      num_units;
            gint      i;

            info->cp += xcf_read_float (info->input, &factor, 1);
            info->cp += xcf_read_int32 (info->input, &digits, 1);
            info->cp += xcf_read_string (info->input, unit_strings, 5);

            for (i = 0; i < 5; i++)
              if (unit_strings[i] == NULL)
                unit_strings[i] = g_strdup ("");

            num_units = gimp_unit_get_number_of_units ();

            for (unit = gimp_unit_get_number_of_built_in_units ();
                 unit < num_units; unit++)
              {
                /* if the factor and the identifier match some unit
                 * in unitrc, use the unitrc unit
                 */
                if ((ABS (gimp_unit_get_factor (unit) - factor) < 1e-5) &&
                    (strcmp (unit_strings[0],
                             gimp_unit_get_identifier (unit)) == 0))
                  {
                    break;
                  }
              }

            /* no match */
            if (unit == num_units)
              unit = gimp_unit_new (unit_strings[0],
                                    factor,
                                    digits,
                                    unit_strings[1],
                                    unit_strings[2],
                                    unit_strings[3],
                                    unit_strings[4]);

            gimp_image_set_unit (image, unit);

            for (i = 0; i < 5; i++)
              g_free (unit_strings[i]);
          }
         break;

        case PROP_VECTORS:
          {
            guint32 base = info->cp;

            if (xcf_load_vectors (info, image))
              {
                if (base + prop_size != info->cp)
                  {
                    g_printerr ("Mismatch in PROP_VECTORS size: "
                                "skipping %d bytes.\n",
                                base + prop_size - info->cp);
                    xcf_seek_pos (info, base + prop_size, NULL);
                  }
              }
            else
              {
                /* skip silently since we don't understand the format and
                 * xcf_load_vectors already explained what was wrong
                 */
                xcf_seek_pos (info, base + prop_size, NULL);
              }
          }
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown image property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}

static gboolean
xcf_load_layer_props (XcfInfo    *info,
                      GimpImage  *image,
                      GimpLayer **layer,
                      GList     **item_path,
                      gboolean   *apply_mask,
                      gboolean   *edit_mask,
                      gboolean   *show_mask,
                      guint32    *text_layer_flags,
                      guint32    *group_layer_flags)
{
  PropType prop_type;
  guint32  prop_size;

  while (TRUE)
    {
      if (! xcf_load_prop (info, &prop_type, &prop_size))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;

        case PROP_ACTIVE_LAYER:
          info->active_layer = *layer;
          break;

        case PROP_FLOATING_SELECTION:
          info->floating_sel = *layer;
          info->cp +=
            xcf_read_int32 (info->input,
                            (guint32 *) &info->floating_sel_offset, 1);
          break;

        case PROP_OPACITY:
          {
            guint32 opacity;

            info->cp += xcf_read_int32 (info->input, &opacity, 1);
            gimp_layer_set_opacity (*layer, (gdouble) opacity / 255.0, FALSE);
          }
          break;

        case PROP_VISIBLE:
          {
            gboolean visible;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &visible, 1);
            gimp_item_set_visible (GIMP_ITEM (*layer), visible, FALSE);
          }
          break;

        case PROP_LINKED:
          {
            gboolean linked;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &linked, 1);
            gimp_item_set_linked (GIMP_ITEM (*layer), linked, FALSE);
          }
          break;

        case PROP_LOCK_CONTENT:
          {
            gboolean lock_content;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &lock_content, 1);

            if (gimp_item_can_lock_content (GIMP_ITEM (*layer)))
              gimp_item_set_lock_content (GIMP_ITEM (*layer),
                                          lock_content, FALSE);
          }
          break;

        case PROP_LOCK_ALPHA:
          {
            gboolean lock_alpha;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &lock_alpha, 1);

            if (gimp_layer_can_lock_alpha (*layer))
              gimp_layer_set_lock_alpha (*layer, lock_alpha, FALSE);
          }
          break;

        case PROP_LOCK_POSITION:
          {
            gboolean lock_position;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &lock_position, 1);

            if (gimp_item_can_lock_position (GIMP_ITEM (*layer)))
              gimp_item_set_lock_position (GIMP_ITEM (*layer),
                                           lock_position, FALSE);
          }
          break;

        case PROP_APPLY_MASK:
          info->cp += xcf_read_int32 (info->input, (guint32 *) apply_mask, 1);
          break;

        case PROP_EDIT_MASK:
          info->cp += xcf_read_int32 (info->input, (guint32 *) edit_mask, 1);
          break;

        case PROP_SHOW_MASK:
          info->cp += xcf_read_int32 (info->input, (guint32 *) show_mask, 1);
          break;

        case PROP_OFFSETS:
          {
            guint32 offset_x;
            guint32 offset_y;

            info->cp += xcf_read_int32 (info->input, &offset_x, 1);
            info->cp += xcf_read_int32 (info->input, &offset_y, 1);

            gimp_item_set_offset (GIMP_ITEM (*layer), offset_x, offset_y);
          }
          break;

        case PROP_MODE:
          {
            guint32 mode;

            info->cp += xcf_read_int32 (info->input, &mode, 1);
            gimp_layer_set_mode (*layer, (GimpLayerModeEffects) mode, FALSE);
          }
          break;

        case PROP_TATTOO:
          {
            GimpTattoo tattoo;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &tattoo, 1);
            gimp_item_set_tattoo (GIMP_ITEM (*layer), tattoo);
          }
          break;

        case PROP_PARASITES:
          {
            glong base = info->cp;

            while (info->cp - base < prop_size)
              {
                GimpParasite *p     = xcf_load_parasite (info);
                GError       *error = NULL;

                if (! p)
                  return FALSE;

                if (! gimp_item_parasite_validate (GIMP_ITEM (*layer), p, &error))
                  {
                    gimp_message (info->gimp, G_OBJECT (info->progress),
                                  GIMP_MESSAGE_WARNING,
                                  "Warning, invalid layer parasite in XCF file: %s",
                                  error->message);
                    g_clear_error (&error);
                  }
                else
                  {
                    gimp_item_parasite_attach (GIMP_ITEM (*layer), p, FALSE);
                  }

                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Error while loading a layer's parasites");
          }
          break;

        case PROP_TEXT_LAYER_FLAGS:
          info->cp += xcf_read_int32 (info->input, text_layer_flags, 1);
          break;

        case PROP_GROUP_ITEM:
          {
            GimpLayer *group;

            group = gimp_group_layer_new (image);

            gimp_object_set_name (GIMP_OBJECT (group),
                                  gimp_object_get_name (*layer));

            g_object_ref_sink (*layer);
            g_object_unref (*layer);
            *layer = group;
          }
          break;

        case PROP_ITEM_PATH:
          {
            glong  base = info->cp;
            GList *path = NULL;

            while (info->cp - base < prop_size)
              {
                guint32 index;

                info->cp += xcf_read_int32 (info->input, &index, 1);

                path = g_list_append (path, GUINT_TO_POINTER (index));
              }

            *item_path = path;
          }
          break;

        case PROP_GROUP_ITEM_FLAGS:
          info->cp += xcf_read_int32 (info->input, group_layer_flags, 1);
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown layer property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}

static gboolean
xcf_load_channel_props (XcfInfo      *info,
                        GimpImage    *image,
                        GimpChannel **channel)
{
  PropType prop_type;
  guint32  prop_size;

  while (TRUE)
    {
      if (! xcf_load_prop (info, &prop_type, &prop_size))
        return FALSE;

      switch (prop_type)
        {
        case PROP_END:
          return TRUE;

        case PROP_ACTIVE_CHANNEL:
          info->active_channel = *channel;
          break;

        case PROP_SELECTION:
          {
            GimpChannel *mask;

            mask =
              gimp_selection_new (image,
                                  gimp_item_get_width  (GIMP_ITEM (*channel)),
                                  gimp_item_get_height (GIMP_ITEM (*channel)));
            gimp_image_take_mask (image, mask);

            g_object_unref (GIMP_DRAWABLE (mask)->private->buffer);
            GIMP_DRAWABLE (mask)->private->buffer =
              GIMP_DRAWABLE (*channel)->private->buffer;
            GIMP_DRAWABLE (*channel)->private->buffer = NULL;
            g_object_unref (*channel);
            *channel = mask;
            (*channel)->boundary_known = FALSE;
            (*channel)->bounds_known   = FALSE;
          }
          break;

        case PROP_OPACITY:
          {
            guint32 opacity;

            info->cp += xcf_read_int32 (info->input, &opacity, 1);
            gimp_channel_set_opacity (*channel, opacity / 255.0, FALSE);
          }
          break;

        case PROP_VISIBLE:
          {
            gboolean visible;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &visible, 1);
            gimp_item_set_visible (GIMP_ITEM (*channel),
                                   visible ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_LINKED:
          {
            gboolean linked;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &linked, 1);
            gimp_item_set_linked (GIMP_ITEM (*channel),
                                  linked ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_LOCK_CONTENT:
          {
            gboolean lock_content;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &lock_content, 1);
            gimp_item_set_lock_content (GIMP_ITEM (*channel),
                                        lock_content ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_LOCK_POSITION:
          {
            gboolean lock_position;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &lock_position, 1);
            gimp_item_set_lock_position (GIMP_ITEM (*channel),
                                         lock_position ? TRUE : FALSE, FALSE);
          }
          break;

        case PROP_SHOW_MASKED:
          {
            gboolean show_masked;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &show_masked, 1);
            gimp_channel_set_show_masked (*channel, show_masked);
          }
          break;

        case PROP_COLOR:
          {
            guchar col[3];

            info->cp += xcf_read_int8 (info->input, (guint8 *) col, 3);
            gimp_rgb_set_uchar (&(*channel)->color, col[0], col[1], col[2]);
          }
          break;

        case PROP_TATTOO:
          {
            GimpTattoo tattoo;

            info->cp += xcf_read_int32 (info->input, (guint32 *) &tattoo, 1);
            gimp_item_set_tattoo (GIMP_ITEM (*channel), tattoo);
          }
          break;

        case PROP_PARASITES:
          {
            glong base = info->cp;

            while ((info->cp - base) < prop_size)
              {
                GimpParasite *p     = xcf_load_parasite (info);
                GError       *error = NULL;

                if (! p)
                  return FALSE;

                if (! gimp_item_parasite_validate (GIMP_ITEM (*channel), p,
                                                    &error))
                  {
                    gimp_message (info->gimp, G_OBJECT (info->progress),
                                  GIMP_MESSAGE_WARNING,
                                  "Warning, invalid channel parasite in XCF file: %s",
                                  error->message);
                    g_clear_error (&error);
                  }
                else
                  {
                    gimp_item_parasite_attach (GIMP_ITEM (*channel), p, FALSE);
                  }

                gimp_parasite_free (p);
              }

            if (info->cp - base != prop_size)
              gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                    GIMP_MESSAGE_WARNING,
                                    "Error while loading a channel's parasites");
          }
          break;

        default:
#ifdef GIMP_UNSTABLE
          g_printerr ("unexpected/unknown channel property: %d (skipping)\n",
                      prop_type);
#endif
          if (! xcf_skip_unknown_prop (info, prop_size))
            return FALSE;
          break;
        }
    }

  return FALSE;
}

static gboolean
xcf_load_prop (XcfInfo  *info,
               PropType *prop_type,
               guint32  *prop_size)
{
  if (G_UNLIKELY (xcf_read_int32 (info->input, (guint32 *) prop_type, 1) != 4))
    return FALSE;

  info->cp += 4;

  if (G_UNLIKELY (xcf_read_int32 (info->input, (guint32 *) prop_size, 1) != 4))
    return FALSE;

  info->cp += 4;

  return TRUE;
}

static GimpLayer *
xcf_load_layer (XcfInfo    *info,
                GimpImage  *image,
                GList     **item_path)
{
  GimpLayer         *layer;
  GimpLayerMask     *layer_mask;
  guint32            hierarchy_offset;
  guint32            layer_mask_offset;
  gboolean           apply_mask = TRUE;
  gboolean           edit_mask  = FALSE;
  gboolean           show_mask  = FALSE;
  gboolean           active;
  gboolean           floating;
  guint32            group_layer_flags = 0;
  guint32            text_layer_flags = 0;
  gint               width;
  gint               height;
  gint               type;
  GimpImageBaseType  base_type;
  gboolean           has_alpha;
  const Babl        *format;
  gboolean           is_fs_drawable;
  gchar             *name;

  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment in our caller.
   */
  is_fs_drawable = (info->cp == info->floating_sel_offset);

  /* read in the layer width, height, type and name */
  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &type, 1);
  info->cp += xcf_read_string (info->input, &name, 1);

  switch (type)
    {
    case GIMP_RGB_IMAGE:
      base_type = GIMP_RGB;
      has_alpha = FALSE;
      break;

    case GIMP_RGBA_IMAGE:
      base_type = GIMP_RGB;
      has_alpha = TRUE;
      break;

    case GIMP_GRAY_IMAGE:
      base_type = GIMP_GRAY;
      has_alpha = FALSE;
      break;

    case GIMP_GRAYA_IMAGE:
      base_type = GIMP_GRAY;
      has_alpha = TRUE;
      break;

    case GIMP_INDEXED_IMAGE:
      base_type = GIMP_INDEXED;
      has_alpha = FALSE;
      break;

    case GIMP_INDEXEDA_IMAGE:
      base_type = GIMP_INDEXED;
      has_alpha = TRUE;
      break;

    default:
      return NULL;
    }

  /* do not use gimp_image_get_layer_format() because it might
   * be the floating selection of a channel or mask
   */
  format = gimp_image_get_format (image, base_type,
                                  gimp_image_get_precision (image),
                                  has_alpha);

  /* create a new layer */
  layer = gimp_layer_new (image, width, height,
                          format, name, 255, GIMP_NORMAL_MODE);
  g_free (name);
  if (! layer)
    return NULL;

  /* read in the layer properties */
  if (! xcf_load_layer_props (info, image, &layer, item_path,
                              &apply_mask, &edit_mask, &show_mask,
                              &text_layer_flags, &group_layer_flags))
    goto error;

  xcf_progress_update (info);

  /* call the evil text layer hack that might change our layer pointer */
  active   = (info->active_layer == layer);
  floating = (info->floating_sel == layer);

  if (gimp_text_layer_xcf_load_hack (&layer))
    {
      gimp_text_layer_set_xcf_flags (GIMP_TEXT_LAYER (layer),
                                     text_layer_flags);

      if (active)
        info->active_layer = layer;
      if (floating)
        info->floating_sel = layer;
    }

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->input, &hierarchy_offset, 1);
  info->cp += xcf_read_int32 (info->input, &layer_mask_offset, 1);

  /* read in the hierarchy (ignore it for group layers, both as an
   * optimization and because the hierarchy's extents don't match
   * the group layer's tiles)
   */
  if (! gimp_viewable_get_children (GIMP_VIEWABLE (layer)))
    {
      if (! xcf_seek_pos (info, hierarchy_offset, NULL))
        goto error;

      if (! xcf_load_buffer (info,
                             gimp_drawable_get_buffer (GIMP_DRAWABLE (layer))))
        goto error;

      xcf_progress_update (info);
    }
  else
    {
      gboolean expanded = group_layer_flags & XCF_GROUP_ITEM_EXPANDED;

      gimp_viewable_set_expanded (GIMP_VIEWABLE (layer), expanded);
    }

  /* read in the layer mask */
  if (layer_mask_offset != 0)
    {
      if (! xcf_seek_pos (info, layer_mask_offset, NULL))
        goto error;

      layer_mask = xcf_load_layer_mask (info, image);
      if (! layer_mask)
        goto error;

      xcf_progress_update (info);

      /* don't add the layer mask yet, that won't work for group
       * layers which update their size automatically; instead
       * attach it so it can be added when all layers are loaded
       */
      g_object_set_data_full (G_OBJECT (layer), "gimp-layer-mask",
                              g_object_ref_sink (layer_mask),
                              (GDestroyNotify) g_object_unref);
      g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-apply",
                         GINT_TO_POINTER (apply_mask));
      g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-edit",
                         GINT_TO_POINTER (edit_mask));
      g_object_set_data (G_OBJECT (layer), "gimp-layer-mask-show",
                         GINT_TO_POINTER (show_mask));
    }

  /* attach the floating selection... */
  if (is_fs_drawable)
    info->floating_sel_drawable = GIMP_DRAWABLE (layer);

  return layer;

 error:
  g_object_unref (layer);
  return NULL;
}

static GimpChannel *
xcf_load_channel (XcfInfo   *info,
                  GimpImage *image)
{
  GimpChannel *channel;
  guint32      hierarchy_offset;
  gint         width;
  gint         height;
  gboolean     is_fs_drawable;
  gchar       *name;
  GimpRGB      color = { 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE };

  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment in our caller.
   */
  is_fs_drawable = (info->cp == info->floating_sel_offset);

  /* read in the layer width, height and name */
  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);
  info->cp += xcf_read_string (info->input, &name, 1);

  /* create a new channel */
  channel = gimp_channel_new (image, width, height, name, &color);
  g_free (name);
  if (!channel)
    return NULL;

  /* read in the channel properties */
  if (!xcf_load_channel_props (info, image, &channel))
    goto error;

  xcf_progress_update (info);

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->input, &hierarchy_offset, 1);

  /* read in the hierarchy */
  if (!xcf_seek_pos (info, hierarchy_offset, NULL))
    goto error;

  if (!xcf_load_buffer (info,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (channel))))
    goto error;

  xcf_progress_update (info);

  if (is_fs_drawable)
    info->floating_sel_drawable = GIMP_DRAWABLE (channel);

  return channel;

 error:
  g_object_unref (channel);
  return NULL;
}

static GimpLayerMask *
xcf_load_layer_mask (XcfInfo   *info,
                     GimpImage *image)
{
  GimpLayerMask *layer_mask;
  GimpChannel   *channel;
  guint32        hierarchy_offset;
  gint           width;
  gint           height;
  gboolean       is_fs_drawable;
  gchar         *name;
  GimpRGB        color = { 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE };

  /* check and see if this is the drawable the floating selection
   *  is attached to. if it is then we'll do the attachment in our caller.
   */
  is_fs_drawable = (info->cp == info->floating_sel_offset);

  /* read in the layer width, height and name */
  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);
  info->cp += xcf_read_string (info->input, &name, 1);

  /* create a new layer mask */
  layer_mask = gimp_layer_mask_new (image, width, height, name, &color);
  g_free (name);
  if (!layer_mask)
    return NULL;

  /* read in the layer_mask properties */
  channel = GIMP_CHANNEL (layer_mask);
  if (!xcf_load_channel_props (info, image, &channel))
    goto error;

  xcf_progress_update (info);

  /* read the hierarchy and layer mask offsets */
  info->cp += xcf_read_int32 (info->input, &hierarchy_offset, 1);

  /* read in the hierarchy */
  if (! xcf_seek_pos (info, hierarchy_offset, NULL))
    goto error;

  if (!xcf_load_buffer (info,
                        gimp_drawable_get_buffer (GIMP_DRAWABLE (layer_mask))))
    goto error;

  xcf_progress_update (info);

  /* attach the floating selection... */
  if (is_fs_drawable)
    info->floating_sel_drawable = GIMP_DRAWABLE (layer_mask);

  return layer_mask;

 error:
  g_object_unref (layer_mask);
  return NULL;
}

static gboolean
xcf_load_buffer (XcfInfo    *info,
                 GeglBuffer *buffer)
{
  const Babl *format;
  guint32     saved_pos;
  guint32     offset;
  guint32     junk;
  gint        width;
  gint        height;
  gint        bpp;

  format = gegl_buffer_get_format (buffer);

  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &bpp, 1);

  /* make sure the values in the file correspond to the values
   *  calculated when the TileManager was created.
   */
  if (width  != gegl_buffer_get_width (buffer)  ||
      height != gegl_buffer_get_height (buffer) ||
      bpp    != babl_format_get_bytes_per_pixel (format))
    return FALSE;

  /* load in the levels...we make sure that the number of levels
   *  calculated when the TileManager was created is the same
   *  as the number of levels found in the file.
   */

  info->cp += xcf_read_int32 (info->input, &offset, 1); /* top level */

  /* discard offsets for layers below first, if any.
   */
  do
    {
      info->cp += xcf_read_int32 (info->input, &junk, 1);
    }
  while (junk != 0);

  /* save the current position as it is where the
   *  next level offset is stored.
   */
  saved_pos = info->cp;

  /* seek to the level offset */
  if (!xcf_seek_pos (info, offset, NULL))
    return FALSE;

  /* read in the level */
  if (!xcf_load_level (info, buffer))
    return FALSE;

  /* restore the saved position so we'll be ready to
   *  read the next offset.
   */
  if (!xcf_seek_pos (info, saved_pos, NULL))
    return FALSE;

  return TRUE;
}


static gboolean
xcf_load_level (XcfInfo    *info,
                GeglBuffer *buffer)
{
  const Babl *format;
  gint        bpp;
  guint32     saved_pos;
  guint32     offset, offset2;
  gint        n_tile_rows;
  gint        n_tile_cols;
  guint       ntiles;
  gint        width;
  gint        height;
  gint        i;
  gint        fail;

  format = gegl_buffer_get_format (buffer);
  bpp    = babl_format_get_bytes_per_pixel (format);

  info->cp += xcf_read_int32 (info->input, (guint32 *) &width, 1);
  info->cp += xcf_read_int32 (info->input, (guint32 *) &height, 1);

  if (width  != gegl_buffer_get_width (buffer) ||
      height != gegl_buffer_get_height (buffer))
    return FALSE;

  /* read in the first tile offset.
   *  if it is '0', then this tile level is empty
   *  and we can simply return.
   */
  info->cp += xcf_read_int32 (info->input, &offset, 1);
  if (offset == 0)
    return TRUE;

  n_tile_rows = gimp_gegl_buffer_get_n_tile_rows (buffer, XCF_TILE_HEIGHT);
  n_tile_cols = gimp_gegl_buffer_get_n_tile_cols (buffer, XCF_TILE_WIDTH);

  ntiles = n_tile_rows * n_tile_cols;
  for (i = 0; i < ntiles; i++)
    {
      GeglRectangle rect;

      fail = FALSE;

      if (offset == 0)
        {
          gimp_message_literal (info->gimp, G_OBJECT (info->progress),
                                GIMP_MESSAGE_ERROR,
                                "not enough tiles found in level");
          return FALSE;
        }

      /* save the current position as it is where the
       *  next tile offset is stored.
       */
      saved_pos = info->cp;

      /* read in the offset of the next tile so we can calculate the amount
         of data needed for this tile*/
      info->cp += xcf_read_int32 (info->input, &offset2, 1);

      /* if the offset is 0 then we need to read in the maximum possible
         allowing for negative compression */
      if (offset2 == 0)
        offset2 = offset + XCF_TILE_WIDTH * XCF_TILE_WIDTH * bpp * 1.5;
                                        /* 1.5 is probably more
                                           than we need to allow */

      /* seek to the tile offset */
      if (! xcf_seek_pos (info, offset, NULL))
        return FALSE;

      /* get the tile from the tile manager */
      gimp_gegl_buffer_get_tile_rect (buffer,
                                      XCF_TILE_WIDTH, XCF_TILE_HEIGHT,
                                      i, &rect);

      /* read in the tile */
      switch (info->compression)
        {
        case COMPRESS_NONE:
          if (!xcf_load_tile (info, buffer, &rect, format))
            fail = TRUE;
          break;
        case COMPRESS_RLE:
          if (!xcf_load_tile_rle (info, buffer, &rect, format,
                                  offset2 - offset))
            fail = TRUE;
          break;
        case COMPRESS_ZLIB:
          g_error ("xcf: zlib compression unimplemented");
          fail = TRUE;
          break;
        case COMPRESS_FRACTAL:
          g_error ("xcf: fractal compression unimplemented");
          fail = TRUE;
          break;
        }

      if (fail)
        return FALSE;

      /* restore the saved position so we'll be ready to
       *  read the next offset.
       */
      if (!xcf_seek_pos (info, saved_pos, NULL))
        return FALSE;

      /* read in the offset of the next tile */
      info->cp += xcf_read_int32 (info->input, &offset, 1);
    }

  if (offset != 0)
    {
      gimp_message (info->gimp, G_OBJECT (info->progress), GIMP_MESSAGE_ERROR,
                    "encountered garbage after reading level: %d", offset);
      return FALSE;
    }

  return TRUE;
}

static gboolean
xcf_load_tile (XcfInfo       *info,
               GeglBuffer    *buffer,
               GeglRectangle *tile_rect,
               const Babl    *format)
{
  gint    bpp       = babl_format_get_bytes_per_pixel (format);
  gint    tile_size = bpp * tile_rect->width * tile_rect->height;
  guchar *tile_data = g_alloca (tile_size);

  info->cp += xcf_read_int8 (info->input, tile_data, tile_size);

  gegl_buffer_set (buffer, tile_rect, 0, format, tile_data,
                   GEGL_AUTO_ROWSTRIDE);

  return TRUE;
}

static gboolean
xcf_load_tile_rle (XcfInfo       *info,
                   GeglBuffer    *buffer,
                   GeglRectangle *tile_rect,
                   const Babl    *format,
                   gint           data_length)
{
  gint    bpp       = babl_format_get_bytes_per_pixel (format);
  gint    tile_size = bpp * tile_rect->width * tile_rect->height;
  guchar *tile_data = g_alloca (tile_size);
  gint    i;
  gsize   nmemb_read_successfully;
  guchar *xcfdata;
  guchar *xcfodata;
  guchar *xcfdatalimit;

  /* Workaround for bug #357809: avoid crashing on g_malloc() and skip
   * this tile (return TRUE without storing data) as if it did not
   * contain any data.  It is better than returning FALSE, which would
   * skip the whole hierarchy while there may still be some valid
   * tiles in the file.
   */
  if (data_length <= 0)
    return TRUE;

  xcfdata = xcfodata = g_alloca (data_length);

  /* we have to read directly instead of xcf_read_* because we may be
   * reading past the end of the file here
   */
  g_input_stream_read_all (info->input, xcfdata, data_length,
                           &nmemb_read_successfully, NULL, NULL);

  if (nmemb_read_successfully == 0)
    return TRUE;

  info->cp += nmemb_read_successfully;

  xcfdatalimit = &xcfodata[nmemb_read_successfully - 1];

  for (i = 0; i < bpp; i++)
    {
      guchar *data  = tile_data + i;
      gint    size  = tile_rect->width * tile_rect->height;
      gint    count = 0;
      guchar  val;
      gint    length;
      gint    j;

      while (size > 0)
        {
          if (xcfdata > xcfdatalimit)
            {
              goto bogus_rle;
            }

          val = *xcfdata++;

          length = val;
          if (length >= 128)
            {
              length = 255 - (length - 1);
              if (length == 128)
                {
                  if (xcfdata >= xcfdatalimit)
                    {
                      goto bogus_rle;
                    }

                  length = (*xcfdata << 8) + xcfdata[1];
                  xcfdata += 2;
                }

              count += length;
              size -= length;

              if (size < 0)
                {
                  goto bogus_rle;
                }

              if (&xcfdata[length-1] > xcfdatalimit)
                {
                  goto bogus_rle;
                }

              while (length-- > 0)
                {
                  *data = *xcfdata++;
                  data += bpp;
                }
            }
          else
            {
              length += 1;
              if (length == 128)
                {
                  if (xcfdata >= xcfdatalimit)
                    {
                      goto bogus_rle;
                    }

                  length = (*xcfdata << 8) + xcfdata[1];
                  xcfdata += 2;
                }

              count += length;
              size -= length;

              if (size < 0)
                {
                  goto bogus_rle;
                }

              if (xcfdata > xcfdatalimit)
                {
                  goto bogus_rle;
                }

              val = *xcfdata++;

              for (j = 0; j < length; j++)
                {
                  *data = val;
                  data += bpp;
                }
            }
        }
    }

  gegl_buffer_set (buffer, tile_rect, 0, format, tile_data,
                   GEGL_AUTO_ROWSTRIDE);

  return TRUE;

 bogus_rle:
  return FALSE;
}

static GimpParasite *
xcf_load_parasite (XcfInfo *info)
{
  GimpParasite *parasite;
  gchar        *name;
  guint32       flags;
  guint32       size;
  gpointer      data;

  info->cp += xcf_read_string (info->input, &name, 1);
  info->cp += xcf_read_int32  (info->input, &flags, 1);
  info->cp += xcf_read_int32  (info->input, &size, 1);

  if (size > MAX_XCF_PARASITE_DATA_LEN)
    {
      g_warning ("Maximum parasite data length (%ld bytes) exceeded. "
                 "Possibly corrupt XCF file.", MAX_XCF_PARASITE_DATA_LEN);
      g_free (name);
      return NULL;
    }

  data = g_new (gchar, size);
  info->cp += xcf_read_int8 (info->input, data, size);

  parasite = gimp_parasite_new (name, flags, size, data);

  g_free (name);
  g_free (data);

  return parasite;
}

static gboolean
xcf_load_old_paths (XcfInfo   *info,
                    GimpImage *image)
{
  guint32      num_paths;
  guint32      last_selected_row;
  GimpVectors *active_vectors;

  info->cp += xcf_read_int32 (info->input, &last_selected_row, 1);
  info->cp += xcf_read_int32 (info->input, &num_paths, 1);

  while (num_paths-- > 0)
    xcf_load_old_path (info, image);

  active_vectors =
    GIMP_VECTORS (gimp_container_get_child_by_index (gimp_image_get_vectors (image),
                                                     last_selected_row));

  if (active_vectors)
    gimp_image_set_active_vectors (image, active_vectors);

  return TRUE;
}

static gboolean
xcf_load_old_path (XcfInfo   *info,
                   GimpImage *image)
{
  gchar                  *name;
  guint32                 locked;
  guint8                  state;
  guint32                 closed;
  guint32                 num_points;
  guint32                 version; /* changed from num_paths */
  GimpTattoo              tattoo = 0;
  GimpVectors            *vectors;
  GimpVectorsCompatPoint *points;
  gint                    i;

  info->cp += xcf_read_string (info->input, &name, 1);
  info->cp += xcf_read_int32  (info->input, &locked, 1);
  info->cp += xcf_read_int8   (info->input, &state, 1);
  info->cp += xcf_read_int32  (info->input, &closed, 1);
  info->cp += xcf_read_int32  (info->input, &num_points, 1);
  info->cp += xcf_read_int32  (info->input, &version, 1);

  if (version == 2)
    {
      guint32 dummy;

      /* Had extra type field and points are stored as doubles */
      info->cp += xcf_read_int32 (info->input, (guint32 *) &dummy, 1);
    }
  else if (version == 3)
    {
      guint32 dummy;

      /* Has extra tatto field */
      info->cp += xcf_read_int32 (info->input, (guint32 *) &dummy,  1);
      info->cp += xcf_read_int32 (info->input, (guint32 *) &tattoo, 1);
    }
  else if (version != 1)
    {
      g_warning ("Unknown path type. Possibly corrupt XCF file");

      return FALSE;
    }

  /* skip empty compatibility paths */
  if (num_points == 0)
    {
      g_free (name);
      return FALSE;
    }

  points = g_new0 (GimpVectorsCompatPoint, num_points);

  for (i = 0; i < num_points; i++)
    {
      if (version == 1)
        {
          gint32 x;
          gint32 y;

          info->cp += xcf_read_int32 (info->input, &points[i].type, 1);
          info->cp += xcf_read_int32 (info->input, (guint32 *) &x,  1);
          info->cp += xcf_read_int32 (info->input, (guint32 *) &y,  1);

          points[i].x = x;
          points[i].y = y;
        }
      else
        {
          gfloat x;
          gfloat y;

          info->cp += xcf_read_int32 (info->input, &points[i].type, 1);
          info->cp += xcf_read_float (info->input, &x,              1);
          info->cp += xcf_read_float (info->input, &y,              1);

          points[i].x = x;
          points[i].y = y;
        }
    }

  vectors = gimp_vectors_compat_new (image, name, points, num_points, closed);

  g_free (name);
  g_free (points);

  gimp_item_set_linked (GIMP_ITEM (vectors), locked, FALSE);

  if (tattoo)
    gimp_item_set_tattoo (GIMP_ITEM (vectors), tattoo);

  gimp_image_add_vectors (image, vectors,
                          NULL, /* can't be a tree */
                          gimp_container_get_n_children (gimp_image_get_vectors (image)),
                          FALSE);

  return TRUE;
}

static gboolean
xcf_load_vectors (XcfInfo   *info,
                  GimpImage *image)
{
  guint32      version;
  guint32      active_index;
  guint32      num_paths;
  GimpVectors *active_vectors;

#ifdef GIMP_XCF_PATH_DEBUG
  g_printerr ("xcf_load_vectors\n");
#endif

  info->cp += xcf_read_int32  (info->input, &version, 1);

  if (version != 1)
    {
      gimp_message (info->gimp, G_OBJECT (info->progress),
                    GIMP_MESSAGE_WARNING,
                    "Unknown vectors version: %d (skipping)", version);
      return FALSE;
    }

  info->cp += xcf_read_int32 (info->input, &active_index, 1);
  info->cp += xcf_read_int32 (info->input, &num_paths,    1);

#ifdef GIMP_XCF_PATH_DEBUG
  g_printerr ("%d paths (active: %d)\n", num_paths, active_index);
#endif

  while (num_paths-- > 0)
    if (! xcf_load_vector (info, image))
      return FALSE;

  /* FIXME tree */
  active_vectors =
    GIMP_VECTORS (gimp_container_get_child_by_index (gimp_image_get_vectors (image),
                                                     active_index));

  if (active_vectors)
    gimp_image_set_active_vectors (image, active_vectors);

#ifdef GIMP_XCF_PATH_DEBUG
  g_printerr ("xcf_load_vectors: loaded %d bytes\n", info->cp - base);
#endif
  return TRUE;
}

static gboolean
xcf_load_vector (XcfInfo   *info,
                 GimpImage *image)
{
  gchar       *name;
  GimpTattoo   tattoo = 0;
  guint32      visible;
  guint32      linked;
  guint32      num_parasites;
  guint32      num_strokes;
  GimpVectors *vectors;
  gint         i;

#ifdef GIMP_XCF_PATH_DEBUG
  g_printerr ("xcf_load_vector\n");
#endif

  info->cp += xcf_read_string (info->input, &name,          1);
  info->cp += xcf_read_int32  (info->input, &tattoo,        1);
  info->cp += xcf_read_int32  (info->input, &visible,       1);
  info->cp += xcf_read_int32  (info->input, &linked,        1);
  info->cp += xcf_read_int32  (info->input, &num_parasites, 1);
  info->cp += xcf_read_int32  (info->input, &num_strokes,   1);

#ifdef GIMP_XCF_PATH_DEBUG
  g_printerr ("name: %s, tattoo: %d, visible: %d, linked: %d, "
              "num_parasites %d, num_strokes %d\n",
              name, tattoo, visible, linked, num_parasites, num_strokes);
#endif

  vectors = gimp_vectors_new (image, name);
  g_free (name);

  gimp_item_set_visible (GIMP_ITEM (vectors), visible, FALSE);
  gimp_item_set_linked (GIMP_ITEM (vectors), linked, FALSE);

  if (tattoo)
    gimp_item_set_tattoo (GIMP_ITEM (vectors), tattoo);

  for (i = 0; i < num_parasites; i++)
    {
      GimpParasite *parasite = xcf_load_parasite (info);
      GError       *error    = NULL;

      if (! parasite)
        return FALSE;

      if (! gimp_item_parasite_validate (GIMP_ITEM (vectors), parasite, &error))
        {
          gimp_message (info->gimp, G_OBJECT (info->progress),
                        GIMP_MESSAGE_WARNING,
                        "Warning, invalid vectors parasite in XCF file: %s",
                        error->message);
          g_clear_error (&error);
        }
      else
        {
          gimp_item_parasite_attach (GIMP_ITEM (vectors), parasite, FALSE);
        }

      gimp_parasite_free (parasite);
    }

  for (i = 0; i < num_strokes; i++)
    {
      guint32      stroke_type_id;
      guint32      closed;
      guint32      num_axes;
      guint32      num_control_points;
      guint32      type;
      gfloat       coords[8] = GIMP_COORDS_DEFAULT_VALUES;
      GimpStroke  *stroke;
      gint         j;

      GimpValueArray *control_points;
      GValue          value = { 0, };
      GimpAnchor      anchor = { { 0, } };
      GType           stroke_type;

      g_value_init (&value, GIMP_TYPE_ANCHOR);

      info->cp += xcf_read_int32 (info->input, &stroke_type_id,     1);
      info->cp += xcf_read_int32 (info->input, &closed,             1);
      info->cp += xcf_read_int32 (info->input, &num_axes,           1);
      info->cp += xcf_read_int32 (info->input, &num_control_points, 1);

#ifdef GIMP_XCF_PATH_DEBUG
      g_printerr ("stroke_type: %d, closed: %d, num_axes %d, len %d\n",
                  stroke_type_id, closed, num_axes, num_control_points);
#endif

      switch (stroke_type_id)
        {
        case XCF_STROKETYPE_BEZIER_STROKE:
          stroke_type = GIMP_TYPE_BEZIER_STROKE;
          break;

        default:
          g_printerr ("skipping unknown stroke type\n");
          xcf_seek_pos (info,
                        info->cp + 4 * num_axes * num_control_points,
                        NULL);
          continue;
        }

      if (num_axes < 2 || num_axes > 6)
        {
          g_printerr ("bad number of axes in stroke description\n");
          return FALSE;
        }

      control_points = gimp_value_array_new (num_control_points);

      anchor.selected = FALSE;

      for (j = 0; j < num_control_points; j++)
        {
          info->cp += xcf_read_int32 (info->input, &type, 1);
          info->cp += xcf_read_float (info->input, coords, num_axes);

          anchor.type              = type;
          anchor.position.x        = coords[0];
          anchor.position.y        = coords[1];
          anchor.position.pressure = coords[2];
          anchor.position.xtilt    = coords[3];
          anchor.position.ytilt    = coords[4];
          anchor.position.wheel    = coords[5];

          g_value_set_boxed (&value, &anchor);
          gimp_value_array_append (control_points, &value);

#ifdef GIMP_XCF_PATH_DEBUG
          g_printerr ("Anchor: %d, (%f, %f, %f, %f, %f, %f)\n", type,
                      coords[0], coords[1], coords[2], coords[3],
                      coords[4], coords[5]);
#endif
        }

      g_value_unset (&value);

      stroke = g_object_new (stroke_type,
                             "closed",         closed,
                             "control-points", control_points,
                             NULL);

      gimp_vectors_stroke_add (vectors, stroke);

      g_object_unref (stroke);
      gimp_value_array_unref (control_points);
    }

  gimp_image_add_vectors (image, vectors,
                          NULL, /* FIXME tree */
                          gimp_container_get_n_children (gimp_image_get_vectors (image)),
                          FALSE);

  return TRUE;
}

static gboolean
xcf_skip_unknown_prop (XcfInfo *info,
                       gsize   size)
{
  guint8 buf[16];
  guint  amount;

  while (size > 0)
    {
      if (g_input_stream_is_closed (info->input))
        return FALSE;

      amount = MIN (16, size);
      info->cp += xcf_read_int8 (info->input, buf, amount);
      size -= MIN (16, amount);
    }

  return TRUE;
}
