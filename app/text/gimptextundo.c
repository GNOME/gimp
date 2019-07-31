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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "gegl/gimp-babl.h"

#include "core/gimp-memsize.h"
#include "core/gimpitem.h"
#include "core/gimpitemundo.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextundo.h"


enum
{
  PROP_0,
  PROP_PARAM
};


static void     gimp_text_undo_constructed  (GObject             *object);
static void     gimp_text_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     gimp_text_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   gimp_text_undo_get_memsize  (GimpObject          *object,
                                             gint64              *gui_size);

static void     gimp_text_undo_pop          (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode,
                                             GimpUndoAccumulator *accum);
static void     gimp_text_undo_free         (GimpUndo            *undo,
                                             GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpTextUndo, gimp_text_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_text_undo_parent_class


static void
gimp_text_undo_class_init (GimpTextUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_text_undo_constructed;
  object_class->set_property     = gimp_text_undo_set_property;
  object_class->get_property     = gimp_text_undo_get_property;

  gimp_object_class->get_memsize = gimp_text_undo_get_memsize;

  undo_class->pop                = gimp_text_undo_pop;
  undo_class->free               = gimp_text_undo_free;

  g_object_class_install_property (object_class, PROP_PARAM,
                                   g_param_spec_param ("param", NULL, NULL,
                                                       G_TYPE_PARAM,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_text_undo_init (GimpTextUndo *undo)
{
}

static void
gimp_text_undo_constructed (GObject *object)
{
  GimpTextUndo  *text_undo = GIMP_TEXT_UNDO (object);
  GimpTextLayer *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_TEXT_LAYER (GIMP_ITEM_UNDO (text_undo)->item));

  layer = GIMP_TEXT_LAYER (GIMP_ITEM_UNDO (text_undo)->item);

  switch (GIMP_UNDO (object)->undo_type)
    {
    case GIMP_UNDO_TEXT_LAYER:
      if (text_undo->pspec)
        {
          gimp_assert (text_undo->pspec->owner_type == GIMP_TYPE_TEXT);

          text_undo->value = g_slice_new0 (GValue);

          g_value_init (text_undo->value, text_undo->pspec->value_type);
          g_object_get_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, text_undo->value);
        }
      else if (layer->text)
        {
          text_undo->text = gimp_config_duplicate (GIMP_CONFIG (layer->text));
        }
      break;

    case GIMP_UNDO_TEXT_LAYER_MODIFIED:
      text_undo->modified = layer->modified;
      break;

    case GIMP_UNDO_TEXT_LAYER_CONVERT:
      text_undo->format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
      break;

    default:
      gimp_assert_not_reached ();
    }
}

static void
gimp_text_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpTextUndo *text_undo = GIMP_TEXT_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      text_undo->pspec = g_value_get_param (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpTextUndo *text_undo = GIMP_TEXT_UNDO (object);

  switch (property_id)
    {
    case PROP_PARAM:
      g_value_set_param (value, (GParamSpec *) text_undo->pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_text_undo_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpTextUndo *undo    = GIMP_TEXT_UNDO (object);
  gint64        memsize = 0;

  memsize += gimp_g_value_get_memsize (undo->value);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (undo->text), NULL);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_text_undo_pop (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  GimpTextUndo  *text_undo = GIMP_TEXT_UNDO (undo);
  GimpTextLayer *layer     = GIMP_TEXT_LAYER (GIMP_ITEM_UNDO (undo)->item);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case GIMP_UNDO_TEXT_LAYER:
      if (text_undo->pspec)
        {
          GValue *value;

          g_return_if_fail (layer->text != NULL);

          value = g_slice_new0 (GValue);
          g_value_init (value, text_undo->pspec->value_type);

          g_object_get_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, value);

          g_object_set_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, text_undo->value);

          g_value_unset (text_undo->value);
          g_slice_free (GValue, text_undo->value);

          text_undo->value = value;
        }
      else
        {
          GimpText *text;

          text = (layer->text ?
                  gimp_config_duplicate (GIMP_CONFIG (layer->text)) : NULL);

          if (layer->text && text_undo->text)
            gimp_config_sync (G_OBJECT (text_undo->text),
                              G_OBJECT (layer->text), 0);
          else
            gimp_text_layer_set_text (layer, text_undo->text);

          if (text_undo->text)
            g_object_unref (text_undo->text);

          text_undo->text = text;
        }
      break;

    case GIMP_UNDO_TEXT_LAYER_MODIFIED:
      {
        gboolean modified;

#if 0
        g_print ("setting layer->modified from %s to %s\n",
                 layer->modified ? "TRUE" : "FALSE",
                 text_undo->modified ? "TRUE" : "FALSE");
#endif

        modified = layer->modified;
        g_object_set (layer, "modified", text_undo->modified, NULL);
        text_undo->modified = modified;

        gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
      }
      break;

    case GIMP_UNDO_TEXT_LAYER_CONVERT:
      {
        const Babl *format;

        format = gimp_drawable_get_format (GIMP_DRAWABLE (layer));
        gimp_drawable_convert_type (GIMP_DRAWABLE (layer),
                                    gimp_item_get_image (GIMP_ITEM (layer)),
                                    gimp_babl_format_get_base_type (text_undo->format),
                                    gimp_babl_format_get_precision (text_undo->format),
                                    babl_format_has_alpha (text_undo->format),
                                    NULL, NULL,
                                    GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                    FALSE, NULL);
        text_undo->format = format;
      }
      break;

    default:
      gimp_assert_not_reached ();
    }
}

static void
gimp_text_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpTextUndo *text_undo = GIMP_TEXT_UNDO (undo);

  g_clear_object (&text_undo->text);

  if (text_undo->pspec)
    {
      g_value_unset (text_undo->value);
      g_slice_free (GValue, text_undo->value);

      text_undo->value = NULL;
      text_undo->pspec = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
