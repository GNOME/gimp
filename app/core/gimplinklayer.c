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
#include "gimpobjectqueue.h"
#include "gimpprogress.h"

#include "gimplink.h"
#include "gimplinklayer.h"

#include "gimp-intl.h"


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
  PROP_MODIFIED,
  PROP_SCALED_ONLY
};

struct _GimpLinkLayerPrivate
{
  GimpLink *link;
  gboolean  modified;
  gboolean  scaled_only;
  gboolean  auto_rename;
};

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
static void       gimp_link_layer_scale          (GimpItem              *item,
                                                  gint                   new_width,
                                                  gint                   new_height,
                                                  gint                   new_offset_x,
                                                  gint                   new_offset_y,
                                                  GimpInterpolationType  interpolation_type,
                                                  GimpProgress          *progress);

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

static void       gimp_link_layer_convert_type   (GimpLayer         *layer,
                                                  GimpImage         *dest_image,
                                                  const Babl        *new_format,
                                                  GimpColorProfile  *src_profile,
                                                  GimpColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  GimpProgress      *progress);

static void       gimp_link_layer_link_changed   (GimpLinkLayer     *layer);
static gboolean   gimp_link_layer_render         (GimpLinkLayer     *layer);

static void       gimp_link_layer_set_xcf_flags  (GimpLinkLayer     *layer,
                                                  guint32            flags);

G_DEFINE_TYPE_WITH_PRIVATE (GimpLinkLayer, gimp_link_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_link_layer_parent_class


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

  viewable_class->default_icon_name = "emblem-symbolic-link";
  viewable_class->default_name      = _("Link Layer");

  item_class->duplicate             = gimp_link_layer_duplicate;
  item_class->rename                = gimp_link_layer_rename;
  item_class->scale                 = gimp_link_layer_scale;

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

  g_object_class_install_property (object_class, PROP_LINK,
                                   g_param_spec_object ("link",
                                                        NULL, NULL,
                                                        GIMP_TYPE_LINK,
                                                        GIMP_PARAM_READWRITE));

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_AUTO_RENAME,
                            "auto-rename",
                            NULL, NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_MODIFIED,
                            "modified",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SCALED_ONLY,
                            "scaled-only",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_link_layer_init (GimpLinkLayer *layer)
{
  layer->p       = gimp_link_layer_get_instance_private (layer);
  layer->p->link = NULL;
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
    case PROP_MODIFIED:
      g_value_set_boolean (value, layer->p->modified);
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
      gimp_link_layer_set_link (layer, g_value_get_object (value), TRUE);
      break;
    case PROP_AUTO_RENAME:
      layer->p->auto_rename = g_value_get_boolean (value);
      break;
    case PROP_MODIFIED:
      layer->p->modified = g_value_get_boolean (value);
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

      if (layer->p->link)
        {
          GimpLink *link = gimp_link_duplicate (layer->p->link);

          /* XXX Shouldn't we block rendering it, and just always copy
           * the buffer? This should be faster.
           */
          gimp_link_layer_set_link (new_layer, link, FALSE);
        }

      gimp_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      if (layer->p->modified)
        {
          GeglBuffer *buffer;
          gint            width;
          gint            height;

          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));
          width  = gegl_buffer_get_width (buffer);
          height = gegl_buffer_get_height (buffer);

          if (width  != gimp_item_get_width  (GIMP_ITEM (new_layer)) ||
              height != gimp_item_get_height (GIMP_ITEM (new_layer)))
            {
              GeglBuffer *new_buffer;

              new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                            gimp_drawable_get_format (GIMP_DRAWABLE (new_layer)));
              gimp_drawable_set_buffer (GIMP_DRAWABLE (new_layer), FALSE, NULL, new_buffer);
              g_object_unref (new_buffer);
            }

          gimp_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                                 gimp_drawable_get_buffer (GIMP_DRAWABLE (new_layer)), NULL);
        }

      new_layer->p->modified    = layer->p->modified;
      new_layer->p->scaled_only = layer->p->scaled_only;
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

  if (gimp_link_is_vector (link_layer->p->link) && ! link_layer->p->modified)
    {
      /* Non-modified vector images are always recomputed from the
       * source file and therefore are always sharp.
       */
      gimp_link_set_size (link_layer->p->link, new_width, new_height);
      gimp_link_layer_render (link_layer);
    }
  else
    {
      gboolean scaled_only = FALSE;

      if (! link_layer->p->modified || link_layer->p->scaled_only)
        {
          /* Raster images whose only modification are previous scaling
           * are scaled back from the source file. Though they are still
           * considered "modified" after a scaling (unlike vector
           * images) and therefore are demoted to work like normal
           * layers, scaling is still special-cased for better image
           * quality.
           */
          gimp_link_layer_render (link_layer);
          scaled_only = TRUE;
        }

      GIMP_ITEM_CLASS (parent_class)->scale (GIMP_ITEM (layer),
                                             new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
      g_object_set (layer, "scaled-only", scaled_only, NULL);
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
gimp_link_layer_set_buffer (GimpDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  GimpLinkLayer *layer = GIMP_LINK_LAYER (drawable);
  GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));

  if (push_undo && ! layer->p->modified)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer,
                                                  bounds);

  if (push_undo && ! layer->p->modified)
    {
      gimp_image_undo_push_link_layer (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);
      g_object_set (drawable, "scaled-only", FALSE, NULL);

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

  if (! layer->p->modified)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE, undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! layer->p->modified)
    {
      gimp_image_undo_push_link_layer (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      gimp_image_undo_group_end (image);
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

  if (! link_layer->p->link   ||
      link_layer->p->modified ||
      layer_dither_type != GEGL_DITHER_NONE)
    {
      GIMP_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
                                                     new_format,
                                                     src_profile,
                                                     dest_profile,
                                                     layer_dither_type,
                                                     mask_dither_type,
                                                     push_undo,
                                                     progress);
    }
  else
    {
      if (push_undo)
        gimp_image_undo_push_link_layer (image, NULL, link_layer);

      gimp_link_layer_render (link_layer);
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
    {
      g_object_unref (layer);
      return NULL;
    }

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
  gboolean rendered = FALSE;

  g_return_val_if_fail (GIMP_IS_LINK_LAYER (layer), FALSE);
  g_return_val_if_fail (GIMP_IS_LINK (link), FALSE);

  /* TODO: look deeper into the link paths. */
  if (layer->p->link == link)
    return ! gimp_link_is_broken (link, FALSE);

  if (gimp_item_is_attached (GIMP_ITEM (layer)) && push_undo)
    gimp_image_undo_push_link_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                     _("Set layer link"), layer);

  if (layer->p->link)
    {
      g_signal_handlers_disconnect_by_func (layer->p->link,
                                            G_CALLBACK (gimp_link_layer_link_changed),
                                            layer);

      g_clear_object (&layer->p->link);
    }

  if (link)
    {
      layer->p->link = g_object_ref (link);

      g_signal_connect_object (link, "changed",
                               G_CALLBACK (gimp_link_layer_link_changed),
                               layer, G_CONNECT_SWAPPED);

      g_object_set (layer, "modified", FALSE, NULL);
      rendered = gimp_link_layer_render (layer);
    }

  g_object_notify (G_OBJECT (layer), "link");
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));

  return rendered;
}

/**
 * gimp_link_layer_discard:
 * @layer: a #GimpLinkLayer
 *
 * Discards the link. This makes @layer behave like a
 * normal layer.
 */
void
gimp_link_layer_discard (GimpLinkLayer *layer)
{
  g_return_if_fail (GIMP_IS_LINK_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));

  gimp_image_undo_push_link_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                   _("Discard Link"), layer);

  layer->p->modified = TRUE;
}

void
gimp_link_layer_monitor (GimpLinkLayer *layer)
{
  g_return_if_fail (GIMP_IS_LINK_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));

  gimp_image_undo_push_link_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                   _("Monitor Link"), layer);

  layer->p->modified = FALSE;
  gimp_link_layer_render (layer);
}

gboolean
gimp_item_is_link_layer (GimpItem *item)
{
  return (GIMP_IS_LINK_LAYER (item)       &&
          ! GIMP_LINK_LAYER (item)->p->modified);
}

guint32
gimp_link_layer_get_xcf_flags (GimpLinkLayer *link_layer)
{
  guint flags = 0;

  g_return_val_if_fail (GIMP_IS_LINK_LAYER (link_layer), 0);

  if (! link_layer->p->auto_rename)
    flags |= LINK_LAYER_XCF_DONT_AUTO_RENAME;

  if (link_layer->p->modified)
    flags |= LINK_LAYER_XCF_MODIFIED;

  return flags;
}

/**
 * gimp_link_layer_from_layer:
 * @layer: a #GimpLayer object
 * @link: a #GimpLink object
 * @flags: flags as retrieved from the XCF file.
 *
 * Converts a standard #GimpLayer into a #GimpLinkLayer.
 * The new link layer takes ownership of the @link.
 * The old @layer object is freed and replaced in-place by the new
 * #GimpLinkLayer.
 *
 * This is a hack similar to the one used to load text layers from XCF,
 * since at first they are loaded as normal layers, and only later
 * promoted to link layers when the corresponding property is read from
 * the file.
 **/
void
gimp_link_layer_from_layer (GimpLayer **layer,
                            GimpLink   *link,
                            guint32     flags)
{
  GimpLinkLayer *link_layer;
  GimpDrawable  *drawable;

  g_return_if_fail (GIMP_IS_LAYER (*layer));
  g_return_if_fail (GIMP_IS_LINK (link));

  link_layer = g_object_new (GIMP_TYPE_LINK_LAYER,
                             "image", gimp_item_get_image (GIMP_ITEM (*layer)),
                             NULL);

  gimp_item_replace_item (GIMP_ITEM (link_layer), GIMP_ITEM (*layer));

  drawable = GIMP_DRAWABLE (link_layer);
  gimp_drawable_steal_buffer (drawable, GIMP_DRAWABLE (*layer));

  gimp_layer_set_opacity         (GIMP_LAYER (link_layer),
                                  gimp_layer_get_opacity (*layer), FALSE);
  gimp_layer_set_mode            (GIMP_LAYER (link_layer),
                                  gimp_layer_get_mode (*layer), FALSE);
  gimp_layer_set_blend_space     (GIMP_LAYER (link_layer),
                                  gimp_layer_get_blend_space (*layer), FALSE);
  gimp_layer_set_composite_space (GIMP_LAYER (link_layer),
                                  gimp_layer_get_composite_space (*layer), FALSE);
  gimp_layer_set_composite_mode  (GIMP_LAYER (link_layer),
                                  gimp_layer_get_composite_mode (*layer), FALSE);
  gimp_layer_set_lock_alpha      (GIMP_LAYER (link_layer),
                                  gimp_layer_get_lock_alpha (*layer), FALSE);

  gimp_link_layer_set_link (link_layer, link, FALSE);
  gimp_link_layer_set_xcf_flags (link_layer, flags);

  g_object_unref (link);
  g_object_unref (*layer);

  *layer = GIMP_LAYER (link_layer);
}

/*  private functions  */

static void
gimp_link_layer_link_changed (GimpLinkLayer *layer)
{
  if (! layer->p->modified)
    gimp_link_layer_render (layer);
}

static gboolean
gimp_link_layer_render (GimpLinkLayer *layer)
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
  buffer = gimp_link_get_buffer (layer->p->link, NULL, NULL);

  if (! buffer)
    return FALSE;

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  g_object_freeze_notify (G_OBJECT (drawable));

  if ((width  != gimp_item_get_width  (item) ||
       height != gimp_item_get_height (item)))
    {
      GeglBuffer *new_buffer;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                    gimp_drawable_get_format (drawable));
      gimp_drawable_set_buffer (drawable, FALSE, NULL, new_buffer);
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
  g_object_unref (buffer);

  g_object_thaw_notify (G_OBJECT (drawable));

  gimp_drawable_update (drawable, 0, 0, width, height);
  gimp_image_flush (image);

  return (width > 0 && height > 0);
}

static void
gimp_link_layer_set_xcf_flags (GimpLinkLayer *layer,
                               guint32        flags)
{
  g_return_if_fail (GIMP_IS_LINK_LAYER (layer));

  g_object_set (layer,
                "auto-rename", (flags & LINK_LAYER_XCF_DONT_AUTO_RENAME) == 0,
                "modified",    (flags & LINK_LAYER_XCF_MODIFIED)         != 0,
                NULL);
}
