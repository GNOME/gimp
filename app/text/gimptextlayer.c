/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextLayer
 * Copyright (C) 2002-2004  Sven Neumann <sven@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "text-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmaitemtree.h"
#include "core/ligmaparasitelist.h"
#include "core/ligmapattern.h"
#include "core/ligmatempbuf.h"

#include "ligmatext.h"
#include "ligmatextlayer.h"
#include "ligmatextlayer-transform.h"
#include "ligmatextlayout.h"
#include "ligmatextlayout-render.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_TEXT,
  PROP_AUTO_RENAME,
  PROP_MODIFIED
};

struct _LigmaTextLayerPrivate
{
  LigmaTextDirection base_dir;
};

static void       ligma_text_layer_finalize       (GObject           *object);
static void       ligma_text_layer_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void       ligma_text_layer_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);

static gint64     ligma_text_layer_get_memsize    (LigmaObject        *object,
                                                  gint64            *gui_size);

static LigmaItem * ligma_text_layer_duplicate      (LigmaItem          *item,
                                                  GType              new_type);
static gboolean   ligma_text_layer_rename         (LigmaItem          *item,
                                                  const gchar       *new_name,
                                                  const gchar       *undo_desc,
                                                  GError           **error);

static void       ligma_text_layer_set_buffer     (LigmaDrawable      *drawable,
                                                  gboolean           push_undo,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  const GeglRectangle *bounds);
static void       ligma_text_layer_push_undo      (LigmaDrawable      *drawable,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height);

static void       ligma_text_layer_convert_type   (LigmaLayer         *layer,
                                                  LigmaImage         *dest_image,
                                                  const Babl        *new_format,
                                                  LigmaColorProfile  *src_profile,
                                                  LigmaColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  LigmaProgress      *progress);

static void       ligma_text_layer_text_changed   (LigmaTextLayer     *layer);
static gboolean   ligma_text_layer_render         (LigmaTextLayer     *layer);
static void       ligma_text_layer_render_layout  (LigmaTextLayer     *layer,
                                                  LigmaTextLayout    *layout);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaTextLayer, ligma_text_layer, LIGMA_TYPE_LAYER)

#define parent_class ligma_text_layer_parent_class


static void
ligma_text_layer_class_init (LigmaTextLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);
  LigmaDrawableClass *drawable_class    = LIGMA_DRAWABLE_CLASS (klass);
  LigmaLayerClass    *layer_class       = LIGMA_LAYER_CLASS (klass);

  object_class->finalize            = ligma_text_layer_finalize;
  object_class->get_property        = ligma_text_layer_get_property;
  object_class->set_property        = ligma_text_layer_set_property;

  ligma_object_class->get_memsize    = ligma_text_layer_get_memsize;

  viewable_class->default_icon_name = "ligma-text-layer";

  item_class->duplicate             = ligma_text_layer_duplicate;
  item_class->rename                = ligma_text_layer_rename;

#if 0
  item_class->scale                 = ligma_text_layer_scale;
  item_class->flip                  = ligma_text_layer_flip;
  item_class->rotate                = ligma_text_layer_rotate;
  item_class->transform             = ligma_text_layer_transform;
#endif

  item_class->default_name          = _("Text Layer");
  item_class->rename_desc           = _("Rename Text Layer");
  item_class->translate_desc        = _("Move Text Layer");
  item_class->scale_desc            = _("Scale Text Layer");
  item_class->resize_desc           = _("Resize Text Layer");
  item_class->flip_desc             = _("Flip Text Layer");
  item_class->rotate_desc           = _("Rotate Text Layer");
  item_class->transform_desc        = _("Transform Text Layer");

  drawable_class->set_buffer        = ligma_text_layer_set_buffer;
  drawable_class->push_undo         = ligma_text_layer_push_undo;

  layer_class->convert_type         = ligma_text_layer_convert_type;

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_TEXT,
                           "text",
                           NULL, NULL,
                           LIGMA_TYPE_TEXT,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_AUTO_RENAME,
                            "auto-rename",
                            NULL, NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_MODIFIED,
                            "modified",
                            NULL, NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_text_layer_init (LigmaTextLayer *layer)
{
  layer->text          = NULL;
  layer->text_parasite = NULL;
  layer->private       = ligma_text_layer_get_instance_private (layer);
}

static void
ligma_text_layer_finalize (GObject *object)
{
  LigmaTextLayer *layer = LIGMA_TEXT_LAYER (object);

  g_clear_object (&layer->text);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_text_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  LigmaTextLayer *text_layer = LIGMA_TEXT_LAYER (object);

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
ligma_text_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaTextLayer *text_layer = LIGMA_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      ligma_text_layer_set_text (text_layer, g_value_get_object (value));
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
ligma_text_layer_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaTextLayer *text_layer = LIGMA_TEXT_LAYER (object);
  gint64         memsize    = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (text_layer->text),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static LigmaItem *
ligma_text_layer_duplicate (LigmaItem *item,
                           GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_TEXT_LAYER (new_item))
    {
      LigmaTextLayer *layer     = LIGMA_TEXT_LAYER (item);
      LigmaTextLayer *new_layer = LIGMA_TEXT_LAYER (new_item);

      ligma_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      if (layer->text)
        {
          LigmaText *text = ligma_config_duplicate (LIGMA_CONFIG (layer->text));

          ligma_text_layer_set_text (new_layer, text);

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
ligma_text_layer_rename (LigmaItem     *item,
                        const gchar  *new_name,
                        const gchar  *undo_desc,
                        GError      **error)
{
  if (LIGMA_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc, error))
    {
      g_object_set (item, "auto-rename", FALSE, NULL);

      return TRUE;
    }

  return FALSE;
}

static void
ligma_text_layer_set_buffer (LigmaDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  LigmaTextLayer *layer = LIGMA_TEXT_LAYER (drawable);
  LigmaImage     *image = ligma_item_get_image (LIGMA_ITEM (layer));

  if (push_undo && ! layer->modified)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  LIGMA_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! layer->modified)
    {
      ligma_image_undo_push_text_layer_modified (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      ligma_image_undo_group_end (image);
    }
}

static void
ligma_text_layer_push_undo (LigmaDrawable *drawable,
                           const gchar  *undo_desc,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  LigmaTextLayer *layer = LIGMA_TEXT_LAYER (drawable);
  LigmaImage     *image = ligma_item_get_image (LIGMA_ITEM (layer));

  if (! layer->modified)
    ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_DRAWABLE, undo_desc);

  LIGMA_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! layer->modified)
    {
      ligma_image_undo_push_text_layer_modified (image, NULL, layer);

      g_object_set (drawable, "modified", TRUE, NULL);

      ligma_image_undo_group_end (image);
    }
}

static void
ligma_text_layer_convert_type (LigmaLayer         *layer,
                              LigmaImage         *dest_image,
                              const Babl        *new_format,
                              LigmaColorProfile  *src_profile,
                              LigmaColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              LigmaProgress      *progress)
{
  LigmaTextLayer *text_layer = LIGMA_TEXT_LAYER (layer);
  LigmaImage     *image      = ligma_item_get_image (LIGMA_ITEM (text_layer));

  if (! text_layer->text   ||
      text_layer->modified ||
      layer_dither_type != GEGL_DITHER_NONE)
    {
      LIGMA_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
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
        ligma_image_undo_push_text_layer_convert (image, NULL, text_layer);

      text_layer->convert_format = new_format;

      ligma_text_layer_render (text_layer);

      text_layer->convert_format = NULL;
    }
}


/*  public functions  */

/**
 * ligma_text_layer_new:
 * @image: the #LigmaImage the layer should belong to
 * @text: a #LigmaText object
 *
 * Creates a new text layer.
 *
 * Returns: (nullable): a new #LigmaTextLayer or %NULL in case of a problem
 **/
LigmaLayer *
ligma_text_layer_new (LigmaImage *image,
                     LigmaText  *text)
{
  LigmaTextLayer *layer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT (text), NULL);

  if (! text->text && ! text->markup)
    return NULL;

  layer =
    LIGMA_TEXT_LAYER (ligma_drawable_new (LIGMA_TYPE_TEXT_LAYER,
                                        image, NULL,
                                        0, 0, 1, 1,
                                        ligma_image_get_layer_format (image,
                                                                     TRUE)));

  ligma_layer_set_mode (LIGMA_LAYER (layer),
                       ligma_image_get_default_new_layer_mode (image),
                       FALSE);

  ligma_text_layer_set_text (layer, text);

  if (! ligma_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  return LIGMA_LAYER (layer);
}

void
ligma_text_layer_set_text (LigmaTextLayer *layer,
                          LigmaText      *text)
{
  g_return_if_fail (LIGMA_IS_TEXT_LAYER (layer));
  g_return_if_fail (text == NULL || LIGMA_IS_TEXT (text));

  if (layer->text == text)
    return;

  if (layer->text)
    {
      g_signal_handlers_disconnect_by_func (layer->text,
                                            G_CALLBACK (ligma_text_layer_text_changed),
                                            layer);

      g_clear_object (&layer->text);
    }

  if (text)
    {
      layer->text = g_object_ref (text);
      layer->private->base_dir = layer->text->base_dir;

      g_signal_connect_object (text, "changed",
                               G_CALLBACK (ligma_text_layer_text_changed),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "text");
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (layer));
}

LigmaText *
ligma_text_layer_get_text (LigmaTextLayer *layer)
{
  g_return_val_if_fail (LIGMA_IS_TEXT_LAYER (layer), NULL);

  return layer->text;
}

void
ligma_text_layer_set (LigmaTextLayer *layer,
                     const gchar   *undo_desc,
                     const gchar   *first_property_name,
                     ...)
{
  LigmaImage *image;
  LigmaText  *text;
  va_list    var_args;

  g_return_if_fail (ligma_item_is_text_layer (LIGMA_ITEM (layer)));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)));

  text = ligma_text_layer_get_text (layer);
  if (! text)
    return;

  image = ligma_item_get_image (LIGMA_ITEM (layer));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TEXT, undo_desc);

  g_object_freeze_notify (G_OBJECT (layer));

  if (layer->modified)
    {
      ligma_image_undo_push_text_layer_modified (image, NULL, layer);

      /*  pass copy_tiles = TRUE so we not only ref the tiles; after
       *  being a text layer again, undo doesn't care about the
       *  layer's pixels any longer because they are generated, so
       *  changing the text would happily overwrite the layer's
       *  pixels, changing the pixels on the undo stack too without
       *  any chance to ever undo again.
       */
      ligma_image_undo_push_drawable_mod (image, NULL,
                                         LIGMA_DRAWABLE (layer), TRUE);
    }

  ligma_image_undo_push_text_layer (image, undo_desc, layer, NULL);

  va_start (var_args, first_property_name);

  g_object_set_valist (G_OBJECT (text), first_property_name, var_args);

  va_end (var_args);

  g_object_set (layer, "modified", FALSE, NULL);

  g_object_thaw_notify (G_OBJECT (layer));

  ligma_image_undo_group_end (image);
}

/**
 * ligma_text_layer_discard:
 * @layer: a #LigmaTextLayer
 *
 * Discards the text information. This makes @layer behave like a
 * normal layer.
 */
void
ligma_text_layer_discard (LigmaTextLayer *layer)
{
  g_return_if_fail (LIGMA_IS_TEXT_LAYER (layer));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (layer)));

  if (! layer->text)
    return;

  ligma_image_undo_push_text_layer (ligma_item_get_image (LIGMA_ITEM (layer)),
                                   _("Discard Text Information"),
                                   layer, NULL);

  ligma_text_layer_set_text (layer, NULL);
}

gboolean
ligma_item_is_text_layer (LigmaItem *item)
{
  return (LIGMA_IS_TEXT_LAYER (item)    &&
          LIGMA_TEXT_LAYER (item)->text &&
          LIGMA_TEXT_LAYER (item)->modified == FALSE);
}


/*  private functions  */

static const Babl *
ligma_text_layer_get_format (LigmaTextLayer *layer)
{
  if (layer->convert_format)
    return layer->convert_format;

  return ligma_drawable_get_format (LIGMA_DRAWABLE (layer));
}

static void
ligma_text_layer_text_changed (LigmaTextLayer *layer)
{
  /*  If the text layer was created from a parasite, it's time to
   *  remove that parasite now.
   */
  if (layer->text_parasite)
    {
      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is loaded from XCF.
       */
      ligma_item_parasite_detach (LIGMA_ITEM (layer), layer->text_parasite,
                                 FALSE);
      layer->text_parasite = NULL;
    }

  if (layer->text->box_mode == LIGMA_TEXT_BOX_DYNAMIC)
    {
      gint                old_width;
      gint                new_width;
      LigmaItem           *item         = LIGMA_ITEM (layer);
      LigmaTextDirection   old_base_dir = layer->private->base_dir;
      LigmaTextDirection   new_base_dir = layer->text->base_dir;

      old_width = ligma_item_get_width (item);
      ligma_text_layer_render (layer);
      new_width = ligma_item_get_width (item);

      if (old_base_dir != new_base_dir)
        {
          switch (old_base_dir)
            {
            case LIGMA_TEXT_DIRECTION_LTR:
            case LIGMA_TEXT_DIRECTION_RTL:
            case LIGMA_TEXT_DIRECTION_TTB_LTR:
            case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              switch (new_base_dir)
                {
                case LIGMA_TEXT_DIRECTION_TTB_RTL:
                case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  ligma_item_translate (item, -new_width, 0, FALSE);
                  break;

                case LIGMA_TEXT_DIRECTION_LTR:
                case LIGMA_TEXT_DIRECTION_RTL:
                case LIGMA_TEXT_DIRECTION_TTB_LTR:
                case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  break;
                }
              break;

            case LIGMA_TEXT_DIRECTION_TTB_RTL:
            case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              switch (new_base_dir)
                {
                case LIGMA_TEXT_DIRECTION_LTR:
                case LIGMA_TEXT_DIRECTION_RTL:
                case LIGMA_TEXT_DIRECTION_TTB_LTR:
                case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  ligma_item_translate (item, old_width, 0, FALSE);
                  break;

                case LIGMA_TEXT_DIRECTION_TTB_RTL:
                case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  break;
                }
              break;
            }
        }
      else if ((new_base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL ||
              new_base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          if (old_width != new_width)
            ligma_item_translate (item, old_width - new_width, 0, FALSE);
        }
    }
  else
    ligma_text_layer_render (layer);

  layer->private->base_dir = layer->text->base_dir;
}

static gboolean
ligma_text_layer_render (LigmaTextLayer *layer)
{
  LigmaDrawable   *drawable;
  LigmaItem       *item;
  LigmaImage      *image;
  LigmaContainer  *container;
  LigmaTextLayout *layout;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;
  GError         *error = NULL;

  if (! layer->text)
    return FALSE;

  drawable  = LIGMA_DRAWABLE (layer);
  item      = LIGMA_ITEM (layer);
  image     = ligma_item_get_image (item);
  container = ligma_data_factory_get_container (image->ligma->font_factory);

  ligma_data_factory_data_wait (image->ligma->font_factory);

  if (ligma_container_is_empty (container))
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_ERROR,
                            _("Due to lack of any fonts, "
                              "text functionality is not available."));
      return FALSE;
    }

  ligma_image_get_resolution (image, &xres, &yres);

  layout = ligma_text_layout_new (layer->text, xres, yres, &error);
  if (error)
    {
      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  if (ligma_text_layout_get_size (layout, &width, &height) &&
      (width  != ligma_item_get_width  (item) ||
       height != ligma_item_get_height (item) ||
       ligma_text_layer_get_format (layer) !=
       ligma_drawable_get_format (drawable)))
    {
      GeglBuffer *new_buffer;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                    ligma_text_layer_get_format (layer));
      ligma_drawable_set_buffer (drawable, FALSE, NULL, new_buffer);
      g_object_unref (new_buffer);

      if (ligma_layer_get_mask (LIGMA_LAYER (layer)))
        {
          LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (layer));

          static LigmaContext *unused_eek = NULL;

          if (! unused_eek)
            unused_eek = ligma_context_new (image->ligma, "eek", NULL);

          ligma_item_resize (LIGMA_ITEM (mask),
                            unused_eek, LIGMA_FILL_TRANSPARENT,
                            width, height, 0, 0);
        }
    }

  if (layer->auto_rename)
    {
      LigmaItem *item = LIGMA_ITEM (layer);
      gchar    *name = NULL;

      if (layer->text->text)
        {
          name = ligma_utf8_strtrim (layer->text->text, 30);
        }
      else if (layer->text->markup)
        {
          gchar *tmp = ligma_markup_extract_text (layer->text->markup);
          name = ligma_utf8_strtrim (tmp, 30);
          g_free (tmp);
        }

      if (! name || ! name[0])
        {
          g_free (name);
          name = g_strdup (_("Empty Text Layer"));
        }

      if (ligma_item_is_attached (item))
        {
          ligma_item_tree_rename_item (ligma_item_get_tree (item), item,
                                      name, FALSE, NULL);
          g_free (name);
        }
      else
        {
          ligma_object_take_name (LIGMA_OBJECT (layer), name);
        }
    }

  if (width > 0 && height > 0)
    ligma_text_layer_render_layout (layer, layout);

  g_object_unref (layout);

  g_object_thaw_notify (G_OBJECT (drawable));

  return (width > 0 && height > 0);
}

static void
ligma_text_layer_set_dash_info (cairo_t      *cr,
                               gdouble       width,
                               gdouble       dash_offset,
                               const GArray *dash_info)
{
  if (dash_info && dash_info->len >= 2)
    {
      gint     n_dashes = dash_info->len;
      gdouble *dashes   = g_new (gdouble, dash_info->len);
      gint     i;

      dash_offset = dash_offset * MAX (width, 1.0);

      for (i = 0; i < n_dashes; i++)
        dashes[i] = MAX (width, 1.0) * g_array_index (dash_info, gdouble, i);

      /* correct 0.0 in the first element (starts with a gap) */

      if (dashes[0] == 0.0)
        {
          gdouble first;

          first = dashes[1];

          /* shift the pattern to really starts with a dash and
           * use the offset to skip into it.
           */
          for (i = 0; i < n_dashes - 2; i++)
            {
              dashes[i] = dashes[i + 2];
              dash_offset += dashes[i];
            }

          if (n_dashes % 2 == 1)
            {
              dashes[n_dashes - 2] = first;
              n_dashes--;
            }
          else if (dash_info->len > 2)
            {
              dashes[n_dashes - 3] += first;
              n_dashes -= 2;
            }
        }

      /* correct odd number of dash specifiers */

      if (n_dashes % 2 == 1)
        {
          gdouble last = dashes[n_dashes - 1];

          dashes[0] += last;
          dash_offset += last;
          n_dashes--;
        }

      if (n_dashes >= 2)
        cairo_set_dash (cr,
                        dashes,
                        n_dashes,
                        dash_offset);

      g_free (dashes);
    }
}

static cairo_surface_t *
ligma_temp_buf_create_cairo_surface (LigmaTempBuf *temp_buf)
{
  cairo_surface_t *surface;
  gboolean         has_alpha;
  const Babl      *format;
  const Babl      *fish = NULL;
  const guchar    *data;
  gint             width;
  gint             height;
  gint             bpp;
  guchar          *pixels;
  gint             rowstride;
  gint             i;

  g_return_val_if_fail (temp_buf != NULL, NULL);

  data      = ligma_temp_buf_get_data (temp_buf);
  format    = ligma_temp_buf_get_format (temp_buf);
  width     = ligma_temp_buf_get_width (temp_buf);
  height    = ligma_temp_buf_get_height (temp_buf);
  bpp       = babl_format_get_bytes_per_pixel (format);
  has_alpha = babl_format_has_alpha (format);

  surface = cairo_image_surface_create (has_alpha ?
                                        CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
                                        width, height);

  pixels    = cairo_image_surface_get_data (surface);
  rowstride = cairo_image_surface_get_stride (surface);

  if (format != babl_format (has_alpha ? "cairo-ARGB32" : "cairo-RGB24"))
    fish = babl_fish (format, babl_format (has_alpha ? "cairo-ARGB32" : "cairo-RGB24"));

  for (i = 0; i < height; i++)
    {
      if (fish)
        babl_process (fish, data, pixels, width);
      else
        memcpy (pixels, data, width * bpp);

      data += width * bpp;
      pixels += rowstride;
    }

  return surface;
}

static void
ligma_text_layer_render_layout (LigmaTextLayer  *layer,
                               LigmaTextLayout *layout)
{
  LigmaDrawable       *drawable = LIGMA_DRAWABLE (layer);
  LigmaItem           *item     = LIGMA_ITEM (layer);
  LigmaImage          *image    = ligma_item_get_image (item);
  GeglBuffer         *buffer;
  LigmaColorTransform *transform;
  cairo_t            *cr;
  cairo_surface_t    *surface;
  gint                width;
  gint                height;
  cairo_status_t      status;

  g_return_if_fail (ligma_drawable_has_alpha (drawable));

  width  = ligma_item_get_width  (item);
  height = ligma_item_get_height (item);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  status = cairo_surface_status (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      LigmaImage *image = ligma_item_get_image (item);

      ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_ERROR,
                            _("Your text cannot be rendered. It is likely too big. "
                              "Please make it shorter or use a smaller font."));
      cairo_surface_destroy (surface);
      return;
    }

  cr = cairo_create (surface);
  if (layer->text->outline != LIGMA_TEXT_OUTLINE_STROKE_ONLY)
    {
      cairo_save (cr);

      ligma_text_layout_render (layout, cr, layer->text->base_dir, FALSE);

      cairo_restore (cr);
    }

  if (layer->text->outline != LIGMA_TEXT_OUTLINE_NONE)
    {
      LigmaText *text = layer->text;
      LigmaRGB   col  = text->outline_foreground;

      cairo_save (cr);

      cairo_set_antialias (cr, text->outline_antialias ?
                           CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE);
      cairo_set_line_cap (cr,
                          text->outline_cap_style == LIGMA_CAP_BUTT ? CAIRO_LINE_CAP_BUTT :
                          text->outline_cap_style == LIGMA_CAP_ROUND ? CAIRO_LINE_CAP_ROUND :
                          CAIRO_LINE_CAP_SQUARE);
      cairo_set_line_join (cr, text->outline_join_style == LIGMA_JOIN_MITER ? CAIRO_LINE_JOIN_MITER :
                               text->outline_join_style == LIGMA_JOIN_ROUND ? CAIRO_LINE_JOIN_ROUND :
                               CAIRO_LINE_JOIN_BEVEL);
      cairo_set_miter_limit (cr, text->outline_miter_limit);

      if (text->outline_dash_info)
        ligma_text_layer_set_dash_info (cr, text->outline_width, text->outline_dash_offset, text->outline_dash_info);

      if (text->outline_style == LIGMA_FILL_STYLE_PATTERN && text->outline_pattern)
        {
          LigmaTempBuf     *tempbuf = ligma_pattern_get_mask (text->outline_pattern);
          cairo_surface_t *surface = ligma_temp_buf_create_cairo_surface (tempbuf);

          cairo_set_source_surface (cr, surface, 0.0, 0.0);
          cairo_surface_destroy (surface);

          cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
        }
      else
        {
          cairo_set_source_rgba (cr, col.r, col.g, col.b, col.a);
        }

      cairo_set_line_width (cr, text->outline_width * 2);

      ligma_text_layout_render (layout, cr, text->base_dir, TRUE);
      cairo_clip_preserve (cr);
      cairo_stroke (cr);

      cairo_restore (cr);
    }

  cairo_destroy (cr);

  cairo_surface_flush (surface);

  buffer = ligma_cairo_surface_create_buffer (surface);

  transform = ligma_image_get_color_transform_from_srgb_u8 (image);

  if (transform)
    {
      ligma_color_transform_process_buffer (transform,
                                           buffer,
                                           NULL,
                                           ligma_drawable_get_buffer (drawable),
                                           NULL);
    }
  else
    {
      ligma_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                             ligma_drawable_get_buffer (drawable), NULL);
    }

  g_object_unref (buffer);
  cairo_surface_destroy (surface);

  ligma_drawable_update (drawable, 0, 0, width, height);
}
