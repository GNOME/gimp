/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLinkLayer
 * Copyright (C) 2019 Jehan
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitemtree.h"
#include "gimplink.h"
#include "gimplinklayer.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"
#include "gimprasterizable.h"

#include "gimp-intl.h"

#define EPSILON 1e-10


enum
{
  LINK_LAYER_XCF_NONE              = 0,
  LINK_LAYER_XCF_DONT_AUTO_RENAME  = 1 << 0,
  LINK_LAYER_XCF_MODIFIED          = 1 << 1
};

enum
{
  PROP_0,
  PROP_LINK,
  PROP_AUTO_RENAME,
  PROP_SCALED_ONLY,
  N_PROPS
};

struct _GimpLinkLayerPrivate
{
  GimpLink              *link;
  gboolean               scaled_only;
  gboolean               auto_rename;

  GimpMatrix3            matrix;
  gint                   offset_x;
  gint                   offset_y;
  GimpInterpolationType  interpolation;

  /* A transient value only useful to know when to drop monitoring after
   * a buffer update.
   */
  gboolean               keep_monitoring;
};

static void       gimp_link_layer_rasterizable_iface_init
                                                 (GimpRasterizableInterface *iface);

static void       gimp_link_layer_finalize       (GObject           *object);
static void       gimp_link_layer_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void       gimp_link_layer_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);

static gint64     gimp_link_layer_get_memsize    (GimpObject        *object,
                                                  gint64            *gui_size);

static GimpItem * gimp_link_layer_duplicate      (GimpItem          *item,
                                                  GType              new_type);
static gboolean   gimp_link_layer_rename         (GimpItem          *item,
                                                  const gchar       *new_name,
                                                  const gchar       *undo_desc,
                                                  GError           **error);

static void       gimp_link_layer_translate      (GimpItem              *item,
                                                  gdouble                offset_x,
                                                  gdouble                offset_y,
                                                  gboolean               push_undo);
static void       gimp_link_layer_scale          (GimpItem              *item,
                                                  gint                   new_width,
                                                  gint                   new_height,
                                                  gint                   new_offset_x,
                                                  gint                   new_offset_y,
                                                  GimpInterpolationType  interpolation_type,
                                                  GimpProgress          *progress);
static void       gimp_link_layer_transform      (GimpItem               *item,
                                                  GimpContext            *context,
                                                  const GimpMatrix3      *matrix,
                                                  GimpTransformDirection  direction,
                                                  GimpInterpolationType   interpolation_type,
                                                  GimpTransformResize     clip_result,
                                                  GimpProgress           *progress,
                                                  gboolean                push_undo);

static void       gimp_link_layer_set_buffer     (GimpDrawable      *drawable,
                                                  gboolean           push_undo,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  const GeglRectangle *bounds);
static void       gimp_link_layer_push_undo      (GimpDrawable      *drawable,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height);

static void       gimp_link_layer_set_rasterized (GimpRasterizable  *rasterizable,
                                                  gboolean           rasterized);

static void       gimp_link_layer_convert_type   (GimpLayer         *layer,
                                                  GimpImage         *dest_image,
                                                  const Babl        *new_format,
                                                  GimpColorProfile  *src_profile,
                                                  GimpColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  GimpProgress      *progress);

static void       gimp_link_layer_render_full    (GimpLinkLayer     *layer);
static gboolean   gimp_link_layer_render_link    (GimpLinkLayer     *layer);

static gboolean
               gimp_link_layer_is_scaling_matrix (GimpLinkLayer     *layer,
                                                  const GimpMatrix3 *matrix,
                                                  gint              *new_width,
                                                  gint              *new_height,
                                                  gint              *new_offset_x,
                                                  gint              *new_offset_y);


G_DEFINE_TYPE_WITH_CODE (GimpLinkLayer, gimp_link_layer, GIMP_TYPE_LAYER,
                         G_ADD_PRIVATE (GimpLinkLayer)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RASTERIZABLE,
                                                gimp_link_layer_rasterizable_iface_init))

#define parent_class gimp_link_layer_parent_class

static GParamSpec *link_layer_props[N_PROPS] = { NULL, };


static void
gimp_link_layer_class_init (GimpLinkLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);
  GimpLayerClass    *layer_class       = GIMP_LAYER_CLASS (klass);

  object_class->finalize            = gimp_link_layer_finalize;
  object_class->get_property        = gimp_link_layer_get_property;
  object_class->set_property        = gimp_link_layer_set_property;

  gimp_object_class->get_memsize    = gimp_link_layer_get_memsize;

  /* TODO: make a custom icon "gimp-link-layer". */
  viewable_class->default_icon_name = "emblem-symbolic-link";
  viewable_class->default_name      = _("Link Layer");

  item_class->duplicate             = gimp_link_layer_duplicate;
  item_class->rename                = gimp_link_layer_rename;
  item_class->translate             = gimp_link_layer_translate;
  item_class->scale                 = gimp_link_layer_scale;
  item_class->transform             = gimp_link_layer_transform;

  item_class->rename_desc           = _("Rename Link Layer");
  item_class->translate_desc        = _("Move Link Layer");
  item_class->scale_desc            = _("Scale Link Layer");
  item_class->resize_desc           = _("Resize Link Layer");
  item_class->flip_desc             = _("Flip Link Layer");
  item_class->rotate_desc           = _("Rotate Link Layer");
  item_class->transform_desc        = _("Transform Link Layer");

  drawable_class->set_buffer        = gimp_link_layer_set_buffer;
  drawable_class->push_undo         = gimp_link_layer_push_undo;

  layer_class->convert_type         = gimp_link_layer_convert_type;

  link_layer_props[PROP_LINK]        = g_param_spec_object ("link",
                                                            NULL, NULL,
                                                            GIMP_TYPE_LINK,
                                                            GIMP_PARAM_READWRITE |
                                                            GIMP_PARAM_STATIC_STRINGS);

  link_layer_props[PROP_AUTO_RENAME] = g_param_spec_boolean ("auto-rename",
                                                             NULL, NULL,
                                                             TRUE,
                                                             GIMP_PARAM_READWRITE |
                                                             GIMP_PARAM_STATIC_STRINGS);

  link_layer_props[PROP_SCALED_ONLY] = g_param_spec_boolean ("scaled-only",
                                                             NULL, NULL,
                                                             FALSE,
                                                             GIMP_PARAM_READWRITE |
                                                             GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, link_layer_props);
}

static void
gimp_link_layer_init (GimpLinkLayer *layer)
{
  layer->p         = gimp_link_layer_get_instance_private (layer);
  layer->p->link   = NULL;
  gimp_matrix3_identity (&layer->p->matrix);

  layer->p->scaled_only   = FALSE;
  layer->p->auto_rename   = FALSE;
  layer->p->offset_x      = 0;
  layer->p->offset_y      = 0;
  layer->p->interpolation = GIMP_INTERPOLATION_NONE;

  layer->p->keep_monitoring = FALSE;
}

static void
gimp_link_layer_rasterizable_iface_init (GimpRasterizableInterface *iface)
{
  iface->set_rasterized = gimp_link_layer_set_rasterized;
}

static void
gimp_link_layer_finalize (GObject *object)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (object);

  g_clear_object (&layer->p->link);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_link_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (object);

  switch (property_id)
    {
    case PROP_LINK:
      g_value_set_object (value, layer->p->link);
      break;
    case PROP_AUTO_RENAME:
      g_value_set_boolean (value, layer->p->auto_rename);
      break;
    case PROP_SCALED_ONLY:
      g_value_set_boolean (value, layer->p->scaled_only);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_link_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (object);

  switch (property_id)
    {
    case PROP_LINK:
      gimp_link_layer_set_link (layer, g_value_get_object (value), FALSE);
      break;
    case PROP_AUTO_RENAME:
      layer->p->auto_rename = g_value_get_boolean (value);
      break;
    case PROP_SCALED_ONLY:
      layer->p->scaled_only = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_link_layer_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpLinkLayer *layer   = GIMP_LINK_LAYER (object);
  gint64         memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (layer->p->link),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_link_layer_duplicate (GimpItem *item,
                           GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_LINK_LAYER (new_item))
    {
      GimpLinkLayer *layer     = GIMP_LINK_LAYER (item);
      GimpLinkLayer *new_layer = GIMP_LINK_LAYER (new_item);
      GimpLink      *link      = NULL;
      GeglBuffer    *buffer;
      gint           width;
      gint           height;

      if (layer->p->link)
        {
          link = gimp_link_duplicate (layer->p->link);
          gimp_link_layer_set_link (new_layer, link, FALSE);
        }

      gimp_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
      width  = gegl_buffer_get_width (buffer);
      height = gegl_buffer_get_height (buffer);

      if (width  != gimp_item_get_width  (GIMP_ITEM (new_layer)) ||
          height != gimp_item_get_height (GIMP_ITEM (new_layer)))
        {
          GeglBuffer    *new_buffer;
          GeglRectangle  bounds;

          gimp_item_get_offset (GIMP_ITEM (new_layer), &bounds.x, &bounds.y);
          bounds.width  = 0;
          bounds.height = 0;

          new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                        gimp_drawable_get_format (GIMP_DRAWABLE (new_layer)));
          GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (GIMP_DRAWABLE (new_layer),
                                                          FALSE, NULL,
                                                          new_buffer, &bounds);
          g_object_unref (new_buffer);
        }

      gimp_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                             gimp_drawable_get_buffer (GIMP_DRAWABLE (new_layer)), NULL);

      new_layer->p->scaled_only   = layer->p->scaled_only;
      new_layer->p->auto_rename   = layer->p->auto_rename;
      new_layer->p->matrix        = layer->p->matrix;
      new_layer->p->offset_x      = layer->p->offset_x;
      new_layer->p->offset_y      = layer->p->offset_y;
      new_layer->p->interpolation = layer->p->interpolation;

      new_layer->p->keep_monitoring = FALSE;

      if (gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (layer)))
        gimp_rasterizable_set_undo_rasterized (GIMP_RASTERIZABLE (new_layer), TRUE);

      g_clear_object (&link);
    }

  return new_item;
}

static gboolean
gimp_link_layer_rename (GimpItem     *item,
                        const gchar  *new_name,
                        const gchar  *undo_desc,
                        GError      **error)
{
  if (GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error))
    {
      g_object_set (item, "auto-rename", FALSE, NULL);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_link_layer_translate (GimpItem *item,
                           gdouble   offset_x,
                           gdouble   offset_y,
                           gboolean  push_undo)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (item);

  if (! gimp_matrix3_is_identity (&layer->p->matrix))
    gimp_matrix3_translate (&layer->p->matrix, offset_x, offset_y);

  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y, push_undo);
}

static void
gimp_link_layer_scale (GimpItem              *item,
                       gint                   new_width,
                       gint                   new_height,
                       gint                   new_offset_x,
                       gint                   new_offset_y,
                       GimpInterpolationType  interpolation_type,
                       GimpProgress          *progress)
{
  GimpLinkLayer   *link_layer = GIMP_LINK_LAYER (item);
  GimpLayer       *layer      = GIMP_LAYER (item);
  GimpObjectQueue *queue      = NULL;

  if (progress && layer->mask)
    {
      GimpLayerMask *mask;

      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      gimp_object_queue_push (queue, layer);
      gimp_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    gimp_object_queue_pop (queue);

  if (gimp_link_is_vector (link_layer->p->link)    &&
      gimp_link_is_monitored (link_layer->p->link) &&
      gimp_matrix3_is_identity (&link_layer->p->matrix))
    {
      /* Non-modified vector images are always recomputed from the
       * source file and therefore are always sharp.
       */
      gimp_link_set_size (link_layer->p->link, new_width, new_height, FALSE);
      gimp_item_set_offset (item, new_offset_x, new_offset_y);
      gimp_link_layer_render_link (link_layer);
    }
  else if (! gimp_matrix3_is_identity (&link_layer->p->matrix))
    {
      GimpMatrix3 matrix = link_layer->p->matrix;
      gdouble     x_scale_factor;
      gdouble     y_scale_factor;
      gint        offset_x;
      gint        offset_y;

      x_scale_factor = (gdouble) new_width / gimp_item_get_width (item);
      y_scale_factor = (gdouble) new_height / gimp_item_get_height (item);

      gimp_matrix3_scale (&matrix, x_scale_factor, y_scale_factor);

      gimp_link_layer_set_transform (link_layer, &matrix, interpolation_type, TRUE);

      /* Unfortunately we can't know the proper translation offset to
       * set before we replay the full matrix (the offsets may have been
       * changed in previous transformations).
       * To avoid re-loading the source image twice though, I don't call
       * gimp_link_layer_set_transform() again but directly edit the
       * matrix and set the offset.
       */
      gimp_item_get_offset (item, &offset_x, &offset_y);
      gimp_matrix3_translate (&link_layer->p->matrix,
                              new_offset_x - offset_x,
                              new_offset_y - offset_y);
      gimp_item_set_offset (item, new_offset_x, new_offset_y);
      gimp_drawable_update_all (GIMP_DRAWABLE (layer));
    }
  else if (gimp_link_is_monitored (link_layer->p->link))
    {
      GimpMatrix3 matrix = link_layer->p->matrix;
      gdouble     x_scale_factor;
      gdouble     y_scale_factor;
      gint        offset_x;
      gint        offset_y;

      x_scale_factor = (gdouble) new_width / gimp_item_get_width (item);
      y_scale_factor = (gdouble) new_height / gimp_item_get_height (item);

      gimp_matrix3_scale (&matrix, x_scale_factor, y_scale_factor);
      gimp_item_get_offset (item, &offset_x, &offset_y);
      gimp_matrix3_translate (&matrix,
                              new_offset_x - offset_x,
                              new_offset_y - offset_y);
      gimp_link_layer_set_transform (link_layer, &matrix, interpolation_type, TRUE);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->scale (GIMP_ITEM (layer),
                                             new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
    }

  if (layer->mask)
    {
      if (queue)
        gimp_object_queue_pop (queue);

      gimp_item_scale (GIMP_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type, progress);
    }

  g_clear_object (&queue);
}

static void
gimp_link_layer_transform (GimpItem               *item,
                           GimpContext            *context,
                           const GimpMatrix3      *matrix,
                           GimpTransformDirection  direction,
                           GimpInterpolationType   interpolation_type,
                           GimpTransformResize     clip_result,
                           GimpProgress           *progress,
                           gboolean                push_undo)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (item);
  gboolean       keep_monitoring;

  if (gimp_matrix3_is_identity (matrix))
    return;

  if (push_undo)
    gimp_image_undo_push_link_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                     _("Transform Link Layer"), layer);

  if (gimp_link_is_vector (layer->p->link)    &&
      gimp_link_is_monitored (layer->p->link) &&
      gimp_matrix3_is_identity (&layer->p->matrix))
    {
      gint new_width;
      gint new_height;
      gint new_offset_x;
      gint new_offset_y;

      if (gimp_link_layer_is_scaling_matrix (layer, matrix,
                                             &new_width, &new_height,
                                             &new_offset_x, &new_offset_y))
        {
          /* Scaling when no other transformation has happened yet is
           * special-case for vector links, because we can do even
           * better by reloading from source.
           */
          gimp_link_layer_scale (item, new_width, new_height,
                                 new_offset_x, new_offset_y,
                                 interpolation_type, progress);
          return;
        }
    }

  /* Clipping would produce different results when applied on a single
   * step of multiple transformations. So only "adjust" transformations
   * are stored non-destructively. Any clip/crop transformation triggers
   * a destructive transform.
   *
   * Note: we could also store non-destructively a single clipped
   * transformation, but that would be harder to document and explain.
   */
  keep_monitoring = (gimp_link_is_monitored (layer->p->link) &&
                     clip_result == GIMP_TRANSFORM_RESIZE_ADJUST);

  if (keep_monitoring)
    {
      GimpMatrix3 left  = *matrix;
      GimpMatrix3 right = layer->p->matrix;

      if (direction == GIMP_TRANSFORM_BACKWARD)
        gimp_matrix3_invert (&left);

      gimp_matrix3_mult (&left, &right);
      gimp_link_layer_set_transform (layer, &right, interpolation_type, TRUE);
    }
  else
    {
      /* Reset the transformation matrix. */
      gimp_matrix3_identity (&layer->p->matrix);

      GIMP_ITEM_CLASS (parent_class)->transform (item, context,
                                                 matrix, direction,
                                                 interpolation_type,
                                                 clip_result, progress,
                                                 push_undo);
    }
}

static void
gimp_link_layer_set_buffer (GimpDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (drawable);
  GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));

  if (push_undo)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE_MOD, undo_desc);
      gimp_image_undo_push_link_layer (image, NULL, layer);
    }

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  FALSE, undo_desc,
                                                  buffer,
                                                  bounds);

  if (push_undo)
    {
      if (layer->p->link                          &&
          gimp_link_is_monitored (layer->p->link) &&
          ! layer->p->keep_monitoring)
        gimp_link_freeze (layer->p->link);

      gimp_image_undo_group_end (image);
    }
}

static void
gimp_link_layer_push_undo (GimpDrawable *drawable,
                           const gchar  *undo_desc,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (drawable);
  GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));
  gboolean       monitored;

  monitored = gimp_link_is_monitored (layer->p->link);

  if (monitored)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE, undo_desc);

      gimp_image_undo_push_link_layer (image, NULL, layer);
      gimp_link_freeze (layer->p->link);
    }

  GIMP_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (monitored)
    {
      gimp_image_undo_group_end (image);
    }
}

static void
gimp_link_layer_set_rasterized (GimpRasterizable *rasterizable,
                                gboolean          rasterized)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (rasterizable);

  if (rasterized)
    {
      gimp_link_freeze (layer->p->link);
    }
  else
    {
      gimp_link_thaw (layer->p->link);
      gimp_matrix3_identity (&layer->p->matrix);
      gimp_link_layer_render_link (layer);
    }
}

static void
gimp_link_layer_convert_type (GimpLayer         *layer,
                              GimpImage         *dest_image,
                              const Babl        *new_format,
                              GimpColorProfile  *src_profile,
                              GimpColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              GimpProgress      *progress)
{
  GimpLinkLayer *link_layer = GIMP_LINK_LAYER (layer);
  GimpImage     *image      = gimp_item_get_image (GIMP_ITEM (link_layer));

  GIMP_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
                                                 new_format,
                                                 src_profile,
                                                 dest_profile,
                                                 layer_dither_type,
                                                 mask_dither_type,
                                                 push_undo,
                                                 progress);

  if (link_layer->p->link && gimp_link_is_monitored (link_layer->p->link))
    {
      if (push_undo)
        gimp_image_undo_push_link_layer (image, NULL, link_layer);

      gimp_link_layer_render_link (link_layer);
    }
}


/*  public functions  */

/**
 * gimp_link_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @text: a #GimpText object
 *
 * Creates a new link layer.
 *
 * Return value: a new #GimpLinkLayer or %NULL in case of a problem
 **/
GimpLayer *
gimp_link_layer_new (GimpImage *image,
                     GimpLink  *link)
{
  GimpLinkLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  layer =
    GIMP_LINK_LAYER (gimp_drawable_new (GIMP_TYPE_LINK_LAYER,
                                        image, NULL,
                                        0, 0, 1, 1,
                                        gimp_image_get_layer_format (image,
                                                                     TRUE)));
  gimp_layer_set_mode (GIMP_LAYER (layer),
                       gimp_image_get_default_new_layer_mode (image),
                       FALSE);

  if (! gimp_link_layer_set_link (layer, link, FALSE))
    g_clear_object (&layer);

  return GIMP_LAYER (layer);
}

GimpLink *
gimp_link_layer_get_link (GimpLinkLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), NULL);

  return layer->p->link;
}

gboolean
gimp_link_layer_set_link (GimpLinkLayer *layer,
                          GimpLink      *link,
                          gboolean       push_undo)
{
  return gimp_link_layer_set_link_with_matrix (layer, link, NULL,
                                               GIMP_INTERPOLATION_NONE,
                                               0, 0, push_undo);
}

gboolean
gimp_link_layer_set_link_with_matrix (GimpLinkLayer         *layer,
                                      GimpLink              *link,
                                      GimpMatrix3           *matrix,
                                      GimpInterpolationType  interpolation_type,
                                      gint                   offset_x,
                                      gint                   offset_y,
                                      gboolean               push_undo)
{
  gboolean rendered = FALSE;

  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), FALSE);
  g_return_val_if_fail (GIMP_IS_LINK (link), FALSE);

  if (gimp_item_is_attached (GIMP_ITEM (layer)) && push_undo)
    gimp_image_undo_push_link_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                     _("Set layer link"), layer);

  if (layer->p->link)
    {
      g_signal_handlers_disconnect_by_func (layer->p->link,
                                            G_CALLBACK (gimp_link_layer_render_full),
                                            layer);

    }

  g_set_object (&layer->p->link, link);

  if (link)
    {
      g_signal_connect_object (link, "changed",
                               G_CALLBACK (gimp_link_layer_render_full),
                               layer, G_CONNECT_SWAPPED);

      if (gimp_link_is_monitored (link))
        {
          if (matrix == NULL)
            {
              gimp_matrix3_identity (&layer->p->matrix);
              rendered = gimp_link_layer_render_link (layer);
            }
          else
            {
              if (! gimp_matrix3_is_identity (matrix))
                {
                  layer->p->offset_x = offset_x;
                  layer->p->offset_y = offset_y;
                }
              rendered = gimp_link_layer_set_transform (layer, matrix, interpolation_type, push_undo);
            }
        }
    }

  g_object_notify (G_OBJECT (layer), "link");
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  return rendered;
}

gboolean
gimp_link_layer_is_monitored (GimpLinkLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), FALSE);

  return (GIMP_LINK_LAYER (layer)->p->link &&
          gimp_link_is_monitored (GIMP_LINK_LAYER (layer)->p->link));
}

gboolean
gimp_link_layer_get_transform (GimpLinkLayer         *layer,
                               GimpMatrix3           *matrix,
                               gint                  *offset_x,
                               gint                  *offset_y,
                               GimpInterpolationType *interpolation)
{
  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), FALSE);

  *matrix        = layer->p->matrix;
  *offset_x      = layer->p->offset_x;
  *offset_y      = layer->p->offset_y;
  *interpolation = layer->p->interpolation;

  return ! gimp_matrix3_is_identity (matrix);
}

gboolean
gimp_link_layer_set_transform (GimpLinkLayer         *layer,
                               GimpMatrix3           *matrix,
                               GimpInterpolationType  interpolation_type,
                               gboolean               push_undo)
{
  Gimp     *gimp;
  gboolean  rendered;

  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), FALSE);
  g_return_val_if_fail (gimp_link_is_monitored (layer->p->link), FALSE);

  gimp = (gimp_item_get_image (GIMP_ITEM (layer)))->gimp;

  if (gimp_matrix3_is_identity (&layer->p->matrix))
    /* First transformation: store the initial offset. */
    gimp_item_get_offset (GIMP_ITEM (layer), &layer->p->offset_x, &layer->p->offset_y);

  gimp_item_set_offset (GIMP_ITEM (layer), layer->p->offset_x, layer->p->offset_y);

  rendered = gimp_link_layer_render_link (layer);
  if (! gimp_matrix3_is_identity (matrix))
    {
      layer->p->keep_monitoring = TRUE;
      GIMP_ITEM_CLASS (parent_class)->transform (GIMP_ITEM (layer),
                                                 gimp_get_user_context (gimp),
                                                 matrix,
                                                 GIMP_TRANSFORM_FORWARD,
                                                 interpolation_type,
                                                 GIMP_TRANSFORM_RESIZE_ADJUST,
                                                 NULL, push_undo);
      layer->p->keep_monitoring = FALSE;
    }

  layer->p->matrix = *matrix;

  /* Interpolations are used to obtain reasonable quality. Considering
   * that doing a single transform will always be better than several
   * transforms in a row, it's OK to just drop the interpolation
   * algorithm of previous transforms. We just store the last one.
   */
  layer->p->interpolation = interpolation_type;

  return rendered;
}

gboolean
gimp_item_is_link_layer (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return (GIMP_IS_LINK_LAYER (item) &&
          gimp_link_layer_is_monitored (GIMP_LINK_LAYER (item)));
}

void
gimp_link_layer_set_xcf_flags (GimpLinkLayer *layer,
                               guint32        flags)
{
  g_return_if_fail (GIMP_IS_LINK_LAYER (layer));

  g_object_set (layer,
                "auto-rename", (flags & LINK_LAYER_XCF_DONT_AUTO_RENAME) == 0,
                NULL);

  if ((flags & LINK_LAYER_XCF_MODIFIED) != 0)
    gimp_link_freeze (layer->p->link);
}

guint32
gimp_link_layer_get_xcf_flags (GimpLinkLayer *link_layer)
{
  guint flags = 0;

  g_return_val_if_fail (GIMP_IS_LINK_LAYER (link_layer), 0);

  if (! link_layer->p->auto_rename)
    flags |= LINK_LAYER_XCF_DONT_AUTO_RENAME;

  if (! gimp_link_is_monitored (link_layer->p->link))
    flags |= LINK_LAYER_XCF_MODIFIED;

  return flags;
}


/*  private functions  */

static void
gimp_link_layer_render_full (GimpLinkLayer *layer)
{
  gimp_link_layer_render_link (layer);
  if (! gimp_matrix3_is_identity (&layer->p->matrix))
    {
      Gimp *gimp = (gimp_item_get_image (GIMP_ITEM (layer)))->gimp;

      gimp_item_set_offset (GIMP_ITEM (layer), layer->p->offset_x, layer->p->offset_y);

      layer->p->keep_monitoring = TRUE;
      GIMP_ITEM_CLASS (parent_class)->transform (GIMP_ITEM (layer),
                                                 gimp_get_user_context (gimp),
                                                 &layer->p->matrix,
                                                 GIMP_TRANSFORM_FORWARD,
                                                 layer->p->interpolation,
                                                 GIMP_TRANSFORM_RESIZE_ADJUST,
                                                 NULL, FALSE);
      layer->p->keep_monitoring = FALSE;
    }
}

static gboolean
gimp_link_layer_render_link (GimpLinkLayer *layer)
{
  GimpDrawable   *drawable;
  GimpItem       *item;
  GimpImage      *image;
  GeglBuffer     *buffer;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;

  if (! layer->p->link)
    return FALSE;

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  image    = gimp_item_get_image (item);

  gimp_image_get_resolution (image, &xres, &yres);
  /* TODO: I could imagine a GimpBusyBox (to be made as GimpProgress) in
   * the later list showing layer update.
   */
  buffer = gimp_link_get_buffer (layer->p->link);

  if (! buffer)
    return FALSE;

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  g_object_freeze_notify (G_OBJECT (drawable));

  if ((width  != gimp_item_get_width  (item) ||
       height != gimp_item_get_height (item)))
    {
      GeglBuffer    *new_buffer;
      GeglRectangle  bounds;

      gimp_item_get_offset (GIMP_ITEM (drawable),
                            &bounds.x, &bounds.y);
      bounds.width  = 0;
      bounds.height = 0;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                    gimp_drawable_get_format (drawable));
      GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                      FALSE, NULL,
                                                      new_buffer, &bounds);
      g_object_unref (new_buffer);

      if (gimp_layer_get_mask (GIMP_LAYER (layer)))
        {
          GimpLayerMask *mask = gimp_layer_get_mask (GIMP_LAYER (layer));

          static GimpContext *unused_eek = NULL;

          if (! unused_eek)
            unused_eek = gimp_context_new (image->gimp, "eek", NULL);

          gimp_item_resize (GIMP_ITEM (mask),
                            unused_eek, GIMP_FILL_TRANSPARENT,
                            width, height, 0, 0);
        }
    }

  if (layer->p->auto_rename)
    {
      GimpItem *item = GIMP_ITEM (layer);
      gchar    *name = NULL;

      if (layer->p->link)
        {
          name = g_strdup (gimp_object_get_name (layer->p->link));
        }

      if (! name || ! name[0])
        {
          g_free (name);
          name = g_strdup (_("Link Layer"));
        }

      if (gimp_item_is_attached (item))
        {
          gimp_item_tree_rename_item (gimp_item_get_tree (item), item,
                                      name, FALSE, NULL);
          g_free (name);
        }
      else
        {
          gimp_object_take_name (GIMP_OBJECT (layer), name);
        }
    }

  gimp_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                         gimp_drawable_get_buffer (drawable), NULL);

  g_object_thaw_notify (G_OBJECT (drawable));

  gimp_drawable_update (drawable, 0, 0, width, height);
  gimp_image_flush (image);

  return (width > 0 && height > 0);
}

static gboolean
gimp_link_layer_is_scaling_matrix (GimpLinkLayer     *layer,
                                   const GimpMatrix3 *matrix,
                                   gint              *new_width,
                                   gint              *new_height,
                                   gint              *new_offset_x,
                                   gint              *new_offset_y)
{
  gboolean is_scaling;

  /* Scaling 3x3 matrix on a 2D plane (with optional translation). */
  is_scaling = (matrix->coeff[0][0] > 0.0 && matrix->coeff[0][1] == 0.0 &&
                matrix->coeff[1][0] < EPSILON && matrix->coeff[1][1] > 0.0 &&
                matrix->coeff[2][0] < EPSILON && matrix->coeff[2][1] < EPSILON && matrix->coeff[2][2] == 1.0);

  if (is_scaling)
    {
      gint width;
      gint height;
      gint offset_x;
      gint offset_y;

      width  = gimp_item_get_width (GIMP_ITEM (layer));
      height = gimp_item_get_height (GIMP_ITEM (layer));
      gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

      *new_width  = (gint) (width * matrix->coeff[0][0]);
      *new_height = (gint) (height * matrix->coeff[1][1]);

      *new_offset_x = (gint) (offset_x * matrix->coeff[0][0] + matrix->coeff[0][2]);
      *new_offset_y = (gint) (offset_y * matrix->coeff[1][1] + matrix->coeff[1][2]);
    }

  return is_scaling;
}
