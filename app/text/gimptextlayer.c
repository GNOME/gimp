/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextLayer
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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
#include <pango/pangocairo.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpimage.h"
#include "core/gimpimage-color-profile.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitemtree.h"
#include "core/gimpparasitelist.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextlayer-transform.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TEXT,
  PROP_AUTO_RENAME,
  PROP_MODIFIED
};

struct _GimpTextLayerPrivate
{
  GimpTextDirection base_dir;
};

static void       gimp_text_layer_finalize       (GObject           *object);
static void       gimp_text_layer_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void       gimp_text_layer_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);

static gint64     gimp_text_layer_get_memsize    (GimpObject        *object,
                                                  gint64            *gui_size);

static GimpItem * gimp_text_layer_duplicate      (GimpItem          *item,
                                                  GType              new_type);
static gboolean   gimp_text_layer_rename         (GimpItem          *item,
                                                  const gchar       *new_name,
                                                  const gchar       *undo_desc,
                                                  GError           **error);

static void       gimp_text_layer_set_buffer     (GimpDrawable      *drawable,
                                                  gboolean           push_undo,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  const GeglRectangle *bounds);
static void       gimp_text_layer_push_undo      (GimpDrawable      *drawable,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height);

static void       gimp_text_layer_convert_type   (GimpLayer         *layer,
                                                  GimpImage         *dest_image,
                                                  const Babl        *new_format,
                                                  GimpColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  GimpProgress      *progress);

static void       gimp_text_layer_text_changed   (GimpTextLayer     *layer);
static gboolean   gimp_text_layer_render         (GimpTextLayer     *layer);
static void       gimp_text_layer_render_layout  (GimpTextLayer     *layer,
                                                  GimpTextLayout    *layout);


G_DEFINE_TYPE_WITH_PRIVATE (GimpTextLayer, gimp_text_layer, GIMP_TYPE_LAYER)

#define parent_class gimp_text_layer_parent_class


static void
gimp_text_layer_class_init (GimpTextLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);
  GimpLayerClass    *layer_class       = GIMP_LAYER_CLASS (klass);

  object_class->finalize            = gimp_text_layer_finalize;
  object_class->get_property        = gimp_text_layer_get_property;
  object_class->set_property        = gimp_text_layer_set_property;

  gimp_object_class->get_memsize    = gimp_text_layer_get_memsize;

  viewable_class->default_icon_name = "gimp-text-layer";

  item_class->duplicate             = gimp_text_layer_duplicate;
  item_class->rename                = gimp_text_layer_rename;

#if 0
  item_class->scale                 = gimp_text_layer_scale;
  item_class->flip                  = gimp_text_layer_flip;
  item_class->rotate                = gimp_text_layer_rotate;
  item_class->transform             = gimp_text_layer_transform;
#endif

  item_class->default_name          = _("Text Layer");
  item_class->rename_desc           = _("Rename Text Layer");
  item_class->translate_desc        = _("Move Text Layer");
  item_class->scale_desc            = _("Scale Text Layer");
  item_class->resize_desc           = _("Resize Text Layer");
  item_class->flip_desc             = _("Flip Text Layer");
  item_class->rotate_desc           = _("Rotate Text Layer");
  item_class->transform_desc        = _("Transform Text Layer");

  drawable_class->set_buffer        = gimp_text_layer_set_buffer;
  drawable_class->push_undo         = gimp_text_layer_push_undo;

  layer_class->convert_type         = gimp_text_layer_convert_type;

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_TEXT,
                           "text",
                           NULL, NULL,
                           GIMP_TYPE_TEXT,
                           GIMP_PARAM_STATIC_STRINGS);

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
}

static void
gimp_text_layer_init (GimpTextLayer *layer)
{
  layer->text          = NULL;
  layer->text_parasite = NULL;
  layer->private       = gimp_text_layer_get_instance_private (layer);
}

static void
gimp_text_layer_finalize (GObject *object)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (object);

  g_clear_object (&layer->text);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_object (value, text_layer->text);
      break;
    case PROP_AUTO_RENAME:
      g_value_set_boolean (value, text_layer->auto_rename);
      break;
    case PROP_MODIFIED:
      g_value_set_boolean (value, text_layer->modified);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      gimp_text_layer_set_text (text_layer, g_value_get_object (value));
      break;
    case PROP_AUTO_RENAME:
      text_layer->auto_rename = g_value_get_boolean (value);
      break;
    case PROP_MODIFIED:
      text_layer->modified = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_text_layer_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (object);
  gint64         memsize    = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (text_layer->text),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_text_layer_duplicate (GimpItem *item,
                           GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_TEXT_LAYER (new_item))
    {
      GimpTextLayer *layer     = GIMP_TEXT_LAYER (item);
      GimpTextLayer *new_layer = GIMP_TEXT_LAYER (new_item);

      gimp_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      if (layer->text)
        {
          GimpText *text = gimp_config_duplicate (GIMP_CONFIG (layer->text));

          gimp_text_layer_set_text (new_layer, text);

          g_object_unref (text);
        }

      /*  this is just the parasite name, not a pointer to the parasite  */
      if (layer->text_parasite)
        new_layer->text_parasite = layer->text_parasite;

      new_layer->private->base_dir = layer->private->base_dir;
    }

  return new_item;
}

static gboolean
gimp_text_layer_rename (GimpItem     *item,
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
gimp_text_layer_set_buffer (GimpDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (drawable);
  GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));

  if (push_undo && ! layer->modified)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! layer->modified)
    {
      gimp_image_undo_push_text_layer_modified (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      gimp_image_undo_group_end (image);
    }
}

static void
gimp_text_layer_push_undo (GimpDrawable *drawable,
                           const gchar  *undo_desc,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  GimpTextLayer *layer = GIMP_TEXT_LAYER (drawable);
  GimpImage     *image = gimp_item_get_image (GIMP_ITEM (layer));

  if (! layer->modified)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE, undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! layer->modified)
    {
      gimp_image_undo_push_text_layer_modified (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      gimp_image_undo_group_end (image);
    }
}

static void
gimp_text_layer_convert_type (GimpLayer         *layer,
                              GimpImage         *dest_image,
                              const Babl        *new_format,
                              GimpColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              GimpProgress      *progress)
{
  GimpTextLayer *text_layer = GIMP_TEXT_LAYER (layer);
  GimpImage     *image      = gimp_item_get_image (GIMP_ITEM (text_layer));

  if (! text_layer->text   ||
      text_layer->modified ||
      layer_dither_type != GEGL_DITHER_NONE)
    {
      GIMP_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
                                                     new_format,
                                                     dest_profile,
                                                     layer_dither_type,
                                                     mask_dither_type,
                                                     push_undo,
                                                     progress);
    }
  else
    {
      if (push_undo)
        gimp_image_undo_push_text_layer_convert (image, NULL, text_layer);

      text_layer->convert_format = new_format;

      gimp_text_layer_render (text_layer);

      text_layer->convert_format = NULL;
    }
}


/*  public functions  */

/**
 * gimp_text_layer_new:
 * @image: the #GimpImage the layer should belong to
 * @text: a #GimpText object
 *
 * Creates a new text layer.
 *
 * Return value: a new #GimpTextLayer or %NULL in case of a problem
 **/
GimpLayer *
gimp_text_layer_new (GimpImage *image,
                     GimpText  *text)
{
  GimpTextLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  if (! text->text && ! text->markup)
    return NULL;

  layer =
    GIMP_TEXT_LAYER (gimp_drawable_new (GIMP_TYPE_TEXT_LAYER,
                                        image, NULL,
                                        0, 0, 1, 1,
                                        gimp_image_get_layer_format (image,
                                                                     TRUE)));

  gimp_layer_set_mode (GIMP_LAYER (layer),
                       gimp_image_get_default_new_layer_mode (image),
                       FALSE);

  gimp_text_layer_set_text (layer, text);

  if (! gimp_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  return GIMP_LAYER (layer);
}

void
gimp_text_layer_set_text (GimpTextLayer *layer,
                          GimpText      *text)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYER (layer));
  g_return_if_fail (text == NULL || GIMP_IS_TEXT (text));

  if (layer->text == text)
    return;

  if (layer->text)
    {
      g_signal_handlers_disconnect_by_func (layer->text,
                                            G_CALLBACK (gimp_text_layer_text_changed),
                                            layer);

      g_clear_object (&layer->text);
    }

  if (text)
    {
      layer->text = g_object_ref (text);
      layer->private->base_dir = layer->text->base_dir;

      g_signal_connect_object (text, "changed",
                               G_CALLBACK (gimp_text_layer_text_changed),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "text");
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
}

GimpText *
gimp_text_layer_get_text (GimpTextLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);

  return layer->text;
}

void
gimp_text_layer_set (GimpTextLayer *layer,
                     const gchar   *undo_desc,
                     const gchar   *first_property_name,
                     ...)
{
  GimpImage *image;
  GimpText  *text;
  va_list    var_args;

  g_return_if_fail (gimp_item_is_text_layer (GIMP_ITEM (layer)));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));

  text = gimp_text_layer_get_text (layer);
  if (! text)
    return;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT, undo_desc);

  g_object_freeze_notify (G_OBJECT (layer));

  if (layer->modified)
    {
      gimp_image_undo_push_text_layer_modified (image, NULL, layer);

      /*  pass copy_tiles = TRUE so we not only ref the tiles; after
       *  being a text layer again, undo doesn't care about the
       *  layer's pixels any longer because they are generated, so
       *  changing the text would happily overwrite the layer's
       *  pixels, changing the pixels on the undo stack too without
       *  any chance to ever undo again.
       */
      gimp_image_undo_push_drawable_mod (image, NULL,
                                         GIMP_DRAWABLE (layer), TRUE);
    }

  gimp_image_undo_push_text_layer (image, undo_desc, layer, NULL);

  va_start (var_args, first_property_name);

  g_object_set_valist (G_OBJECT (text), first_property_name, var_args);

  va_end (var_args);

  g_object_set (layer, "modified", FALSE, NULL);

  g_object_thaw_notify (G_OBJECT (layer));

  gimp_image_undo_group_end (image);
}

/**
 * gimp_text_layer_discard:
 * @layer: a #GimpTextLayer
 *
 * Discards the text information. This makes @layer behave like a
 * normal layer.
 */
void
gimp_text_layer_discard (GimpTextLayer *layer)
{
  g_return_if_fail (GIMP_IS_TEXT_LAYER (layer));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (layer)));

  if (! layer->text)
    return;

  gimp_image_undo_push_text_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                   _("Discard Text Information"),
                                   layer, NULL);

  gimp_text_layer_set_text (layer, NULL);
}

gboolean
gimp_item_is_text_layer (GimpItem *item)
{
  return (GIMP_IS_TEXT_LAYER (item)    &&
          GIMP_TEXT_LAYER (item)->text &&
          GIMP_TEXT_LAYER (item)->modified == FALSE);
}


/*  private functions  */

static const Babl *
gimp_text_layer_get_format (GimpTextLayer *layer)
{
  if (layer->convert_format)
    return layer->convert_format;

  return gimp_drawable_get_format (GIMP_DRAWABLE (layer));
}

static void
gimp_text_layer_text_changed (GimpTextLayer *layer)
{
  /*  If the text layer was created from a parasite, it's time to
   *  remove that parasite now.
   */
  if (layer->text_parasite)
    {
      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is loaded from XCF.
       */
      gimp_item_parasite_detach (GIMP_ITEM (layer), layer->text_parasite,
                                 FALSE);
      layer->text_parasite = NULL;
    }

  if (layer->text->box_mode == GIMP_TEXT_BOX_DYNAMIC)
    {
      gint                old_width;
      gint                new_width;
      GimpItem           *item         = GIMP_ITEM (layer);
      GimpTextDirection   old_base_dir = layer->private->base_dir;
      GimpTextDirection   new_base_dir = layer->text->base_dir;

      old_width = gimp_item_get_width (item);
      gimp_text_layer_render (layer);
      new_width = gimp_item_get_width (item);

      if (old_base_dir != new_base_dir)
        {
          switch (old_base_dir)
            {
            case GIMP_TEXT_DIRECTION_LTR:
            case GIMP_TEXT_DIRECTION_RTL:
            case GIMP_TEXT_DIRECTION_TTB_LTR:
            case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              switch (new_base_dir)
                {
                case GIMP_TEXT_DIRECTION_TTB_RTL:
                case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  gimp_item_translate (item, -new_width, 0, FALSE);
                  break;

                case GIMP_TEXT_DIRECTION_LTR:
                case GIMP_TEXT_DIRECTION_RTL:
                case GIMP_TEXT_DIRECTION_TTB_LTR:
                case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  break;
                }
              break;

            case GIMP_TEXT_DIRECTION_TTB_RTL:
            case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              switch (new_base_dir)
                {
                case GIMP_TEXT_DIRECTION_LTR:
                case GIMP_TEXT_DIRECTION_RTL:
                case GIMP_TEXT_DIRECTION_TTB_LTR:
                case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  gimp_item_translate (item, old_width, 0, FALSE);
                  break;

                case GIMP_TEXT_DIRECTION_TTB_RTL:
                case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  break;
                }
              break;
            }
        }
      else if ((new_base_dir == GIMP_TEXT_DIRECTION_TTB_RTL ||
              new_base_dir == GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          if (old_width != new_width)
            gimp_item_translate (item, old_width - new_width, 0, FALSE);
        }
    }
  else
    gimp_text_layer_render (layer);

  layer->private->base_dir = layer->text->base_dir;
}

static gboolean
gimp_text_layer_render (GimpTextLayer *layer)
{
  GimpDrawable   *drawable;
  GimpItem       *item;
  GimpImage      *image;
  GimpContainer  *container;
  GimpTextLayout *layout;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;
  GError         *error = NULL;

  if (! layer->text)
    return FALSE;

  drawable  = GIMP_DRAWABLE (layer);
  item      = GIMP_ITEM (layer);
  image     = gimp_item_get_image (item);
  container = gimp_data_factory_get_container (image->gimp->font_factory);

  gimp_data_factory_data_wait (image->gimp->font_factory);

  if (gimp_container_is_empty (container))
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("Due to lack of any fonts, "
                              "text functionality is not available."));
      return FALSE;
    }

  gimp_image_get_resolution (image, &xres, &yres);

  layout = gimp_text_layout_new (layer->text, xres, yres, &error);
  if (error)
    {
      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  if (gimp_text_layout_get_size (layout, &width, &height) &&
      (width  != gimp_item_get_width  (item) ||
       height != gimp_item_get_height (item) ||
       gimp_text_layer_get_format (layer) !=
       gimp_drawable_get_format (drawable)))
    {
      GeglBuffer *new_buffer;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                    gimp_text_layer_get_format (layer));
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

  if (layer->auto_rename)
    {
      GimpItem *item = GIMP_ITEM (layer);
      gchar    *name = NULL;

      if (layer->text->text)
        {
          name = gimp_utf8_strtrim (layer->text->text, 30);
        }
      else if (layer->text->markup)
        {
          gchar *tmp = gimp_markup_extract_text (layer->text->markup);
          name = gimp_utf8_strtrim (tmp, 30);
          g_free (tmp);
        }

      if (! name || ! name[0])
        {
          g_free (name);
          name = g_strdup (_("Empty Text Layer"));
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

  if (width > 0 && height > 0)
    gimp_text_layer_render_layout (layer, layout);

  g_object_unref (layout);

  g_object_thaw_notify (G_OBJECT (drawable));

  return (width > 0 && height > 0);
}

static void
gimp_text_layer_render_layout (GimpTextLayer  *layer,
                               GimpTextLayout *layout)
{
  GimpDrawable       *drawable = GIMP_DRAWABLE (layer);
  GimpItem           *item     = GIMP_ITEM (layer);
  GimpImage          *image    = gimp_item_get_image (item);
  GeglBuffer         *buffer;
  GimpColorTransform *transform;
  cairo_t            *cr;
  cairo_surface_t    *surface;
  gint                width;
  gint                height;
  cairo_status_t      status;

  g_return_if_fail (gimp_drawable_has_alpha (drawable));

  width  = gimp_item_get_width  (item);
  height = gimp_item_get_height (item);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  status = cairo_surface_status (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      GimpImage *image = gimp_item_get_image (item);

      gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("Your text cannot be rendered. It is likely too big. "
                              "Please make it shorter or use a smaller font."));
      cairo_surface_destroy (surface);
      return;
    }

  cr = cairo_create (surface);
  gimp_text_layout_render (layout, cr, layer->text->base_dir, FALSE);
  cairo_destroy (cr);

  cairo_surface_flush (surface);

  buffer = gimp_cairo_surface_create_buffer (surface);

  transform = gimp_image_get_color_transform_from_srgb_u8 (image);

  if (transform)
    {
      gimp_color_transform_process_buffer (transform,
                                           buffer,
                                           NULL,
                                           gimp_drawable_get_buffer (drawable),
                                           NULL);
    }
  else
    {
      gimp_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                             gimp_drawable_get_buffer (drawable), NULL);
    }

  g_object_unref (buffer);
  cairo_surface_destroy (surface);

  gimp_drawable_update (drawable, 0, 0, width, height);
}
