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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimp.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpfilloptions.h"
#include "core/gimpimage.h"
#include "core/gimpselection.h"
#include "core/gimprasterizable.h"

#include "path/gimppath.h"

#include "gimperror.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplayervectormask.h"

#include "gimp-intl.h"

static void    gimp_layer_vector_mask_rasterizable_iface_init
                                                     (GimpRasterizableInterface *iface);

static void    gimp_layer_vector_mask_set_rasterized (GimpRasterizable          *rasterizable,
                                                      gboolean                   rasterized);


static void   gimp_layer_vector_mask_render_path     (GimpLayerVectorMask       *layer_mask);

G_DEFINE_TYPE_WITH_CODE (GimpLayerVectorMask, gimp_layer_vector_mask, GIMP_TYPE_LAYER_MASK,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RASTERIZABLE,
                                                gimp_layer_vector_mask_rasterizable_iface_init))

#define parent_class gimp_layer_vector_mask_parent_class


static void
gimp_layer_vector_mask_class_init (GimpLayerVectorMaskClass *klass)
{
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class     = GIMP_ITEM_CLASS (klass);

  viewable_class->default_icon_name = "gimp-tool-path";
  viewable_class->default_name      = _("Vector Mask");

  item_class->translate_desc           = C_("undo-type", "Move Layer Vector Mask");
  item_class->to_selection_desc        = C_("undo-type", "Layer Vector Mask to Selection");
}

static void
gimp_layer_vector_mask_init (GimpLayerVectorMask *layer_mask)
{
  layer_mask->layer = NULL;
  layer_mask->path  = NULL;
}

static void
gimp_layer_vector_mask_rasterizable_iface_init (GimpRasterizableInterface *iface)
{
  iface->set_rasterized = gimp_layer_vector_mask_set_rasterized;
}

static void
gimp_layer_vector_mask_set_rasterized (GimpRasterizable *rasterizable,
                                       gboolean          rasterized)
{
  if (! rasterized)
    gimp_layer_vector_mask_render  (GIMP_LAYER_VECTOR_MASK (rasterizable));
}


gboolean
gimp_layer_vector_mask_render (GimpLayerVectorMask *layer_mask)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer_mask);
  GimpLayer    *layer    = NULL;
  GeglBuffer   *buffer   = NULL;
  GimpItem     *item     = GIMP_ITEM (layer_mask);
  GimpImage    *image    = gimp_item_get_image (item);
  gint          x        = 0;
  gint          y        = 0;
  gint          width    = gimp_image_get_width (image);
  gint          height   = gimp_image_get_height (image);

  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);

  layer = gimp_layer_mask_get_layer (GIMP_LAYER_MASK (layer_mask));

  g_object_freeze_notify (G_OBJECT (drawable));

  /* Resize layer according to path size */
  gimp_item_bounds (GIMP_ITEM (layer), &x, &y, &width, &height);

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                            gimp_drawable_get_format (drawable));
  gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  gimp_item_set_offset (GIMP_ITEM (layer_mask), x, y);

  /* make the layer background transparent */
  gimp_drawable_fill (GIMP_DRAWABLE (layer_mask),
                      gimp_get_user_context (image->gimp),
                      GIMP_FILL_BACKGROUND);

  /* render path to the layer */
  gimp_layer_vector_mask_render_path (layer_mask);

  g_object_thaw_notify (G_OBJECT (drawable));

  return TRUE;
}

static void
gimp_layer_vector_mask_render_path (GimpLayerVectorMask *layer_mask)
{
  GimpImage              *image     = gimp_item_get_image (GIMP_ITEM (layer_mask));
  GimpChannel            *selection = gimp_image_get_mask (image);
  GimpFillOptions        *fill_options;
  GeglColor              *fill_color;
  GList                  *drawables;

  /* Don't mask these fill/stroke operations  */
  gimp_selection_suspend (GIMP_SELECTION (selection));

  fill_color = gegl_color_new ("white");

  fill_options = gimp_fill_options_new (image->gimp, NULL, FALSE);
  gimp_fill_options_set_style (GIMP_FILL_OPTIONS (fill_options),
                               GIMP_FILL_STYLE_FG_COLOR);
  gimp_context_set_foreground (GIMP_CONTEXT (fill_options),
                               fill_color);

  /* Fill the path object onto the layer */
  gimp_drawable_fill_path (GIMP_DRAWABLE (layer_mask),
                           fill_options,
                           layer_mask->path, FALSE, NULL);

  drawables = g_list_prepend (NULL, GIMP_DRAWABLE (layer_mask));

  g_list_free (drawables);

  /*gimp_rasterizable_auto_rename (GIMP_RASTERIZABLE (layer_mask),
                                 GIMP_OBJECT (layer_mask->path),
                                 NULL);*/

  gimp_selection_resume (GIMP_SELECTION (selection));

  g_clear_object (&fill_color);
  g_object_unref (fill_options);
}

/* Public Functions */

GimpLayerVectorMask *
gimp_layer_vector_mask_new (GimpImage   *image,
                            GimpPath    *path,
                            gint         width,
                            gint         height,
                            const gchar *name)
{
  GimpLayerVectorMask *layer_mask;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  layer_mask =
    GIMP_LAYER_VECTOR_MASK (gimp_drawable_new (GIMP_TYPE_LAYER_VECTOR_MASK,
                                               image, name,
                                               0, 0, width, height,
                                               gimp_image_get_mask_format (image)));

  layer_mask->path = path;

  /*  set the layer_mask color and opacity  */
  gimp_channel_set_show_masked (GIMP_CHANNEL (layer_mask), TRUE);

  /*  selection mask variables  */
  GIMP_CHANNEL (layer_mask)->x2 = width;
  GIMP_CHANNEL (layer_mask)->y2 = height;

  return layer_mask;
}

void
gimp_layer_vector_mask_set_path (GimpLayerVectorMask *layer_mask,
                                 GimpPath            *path)
{
  g_return_if_fail (GIMP_IS_LAYER_MASK (layer_mask));
  g_return_if_fail (GIMP_IS_PATH (path));

  layer_mask->path = path;

  gimp_layer_vector_mask_render (layer_mask);
}

GimpPath *
gimp_layer_vector_mask_get_path (GimpLayerVectorMask *layer_mask)
{
  g_return_val_if_fail (GIMP_IS_LAYER_VECTOR_MASK (layer_mask), NULL);

  return layer_mask->path;
}
