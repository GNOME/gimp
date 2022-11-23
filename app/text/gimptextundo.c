/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "text-types.h"

#include "gegl/ligma-babl.h"

#include "core/ligma-memsize.h"
#include "core/ligmaitem.h"
#include "core/ligmaitemundo.h"

#include "ligmatext.h"
#include "ligmatextlayer.h"
#include "ligmatextundo.h"


enum
{
  PROP_0,
  PROP_PARAM
};


static void     ligma_text_undo_constructed  (GObject             *object);
static void     ligma_text_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     ligma_text_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   ligma_text_undo_get_memsize  (LigmaObject          *object,
                                             gint64              *gui_size);

static void     ligma_text_undo_pop          (LigmaUndo            *undo,
                                             LigmaUndoMode         undo_mode,
                                             LigmaUndoAccumulator *accum);
static void     ligma_text_undo_free         (LigmaUndo            *undo,
                                             LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaTextUndo, ligma_text_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_text_undo_parent_class


static void
ligma_text_undo_class_init (LigmaTextUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_text_undo_constructed;
  object_class->set_property     = ligma_text_undo_set_property;
  object_class->get_property     = ligma_text_undo_get_property;

  ligma_object_class->get_memsize = ligma_text_undo_get_memsize;

  undo_class->pop                = ligma_text_undo_pop;
  undo_class->free               = ligma_text_undo_free;

  g_object_class_install_property (object_class, PROP_PARAM,
                                   g_param_spec_param ("param", NULL, NULL,
                                                       G_TYPE_PARAM,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_text_undo_init (LigmaTextUndo *undo)
{
}

static void
ligma_text_undo_constructed (GObject *object)
{
  LigmaTextUndo  *text_undo = LIGMA_TEXT_UNDO (object);
  LigmaTextLayer *layer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_TEXT_LAYER (LIGMA_ITEM_UNDO (text_undo)->item));

  layer = LIGMA_TEXT_LAYER (LIGMA_ITEM_UNDO (text_undo)->item);

  switch (LIGMA_UNDO (object)->undo_type)
    {
    case LIGMA_UNDO_TEXT_LAYER:
      if (text_undo->pspec)
        {
          ligma_assert (text_undo->pspec->owner_type == LIGMA_TYPE_TEXT);

          text_undo->value = g_slice_new0 (GValue);

          g_value_init (text_undo->value, text_undo->pspec->value_type);
          g_object_get_property (G_OBJECT (layer->text),
                                 text_undo->pspec->name, text_undo->value);
        }
      else if (layer->text)
        {
          text_undo->text = ligma_config_duplicate (LIGMA_CONFIG (layer->text));
        }
      break;

    case LIGMA_UNDO_TEXT_LAYER_MODIFIED:
      text_undo->modified = layer->modified;
      break;

    case LIGMA_UNDO_TEXT_LAYER_CONVERT:
      text_undo->format = ligma_drawable_get_format (LIGMA_DRAWABLE (layer));
      break;

    default:
      ligma_assert_not_reached ();
    }
}

static void
ligma_text_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaTextUndo *text_undo = LIGMA_TEXT_UNDO (object);

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
ligma_text_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaTextUndo *text_undo = LIGMA_TEXT_UNDO (object);

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
ligma_text_undo_get_memsize (LigmaObject *object,
                            gint64     *gui_size)
{
  LigmaTextUndo *undo    = LIGMA_TEXT_UNDO (object);
  gint64        memsize = 0;

  memsize += ligma_g_value_get_memsize (undo->value);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (undo->text), NULL);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_text_undo_pop (LigmaUndo            *undo,
                    LigmaUndoMode         undo_mode,
                    LigmaUndoAccumulator *accum)
{
  LigmaTextUndo  *text_undo = LIGMA_TEXT_UNDO (undo);
  LigmaTextLayer *layer     = LIGMA_TEXT_LAYER (LIGMA_ITEM_UNDO (undo)->item);

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_TEXT_LAYER:
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
          LigmaText *text;

          text = (layer->text ?
                  ligma_config_duplicate (LIGMA_CONFIG (layer->text)) : NULL);

          if (layer->text && text_undo->text)
            ligma_config_sync (G_OBJECT (text_undo->text),
                              G_OBJECT (layer->text), 0);
          else
            ligma_text_layer_set_text (layer, text_undo->text);

          if (text_undo->text)
            g_object_unref (text_undo->text);

          text_undo->text = text;
        }
      break;

    case LIGMA_UNDO_TEXT_LAYER_MODIFIED:
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

        ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (layer));
      }
      break;

    case LIGMA_UNDO_TEXT_LAYER_CONVERT:
      {
        const Babl *format;

        format = ligma_drawable_get_format (LIGMA_DRAWABLE (layer));
        ligma_drawable_convert_type (LIGMA_DRAWABLE (layer),
                                    ligma_item_get_image (LIGMA_ITEM (layer)),
                                    ligma_babl_format_get_base_type (text_undo->format),
                                    ligma_babl_format_get_precision (text_undo->format),
                                    babl_format_has_alpha (text_undo->format),
                                    NULL, NULL,
                                    GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                    FALSE, NULL);
        text_undo->format = format;
      }
      break;

    default:
      ligma_assert_not_reached ();
    }
}

static void
ligma_text_undo_free (LigmaUndo     *undo,
                     LigmaUndoMode  undo_mode)
{
  LigmaTextUndo *text_undo = LIGMA_TEXT_UNDO (undo);

  g_clear_object (&text_undo->text);

  if (text_undo->pspec)
    {
      g_value_unset (text_undo->value);
      g_slice_free (GValue, text_undo->value);

      text_undo->value = NULL;
      text_undo->pspec = NULL;
    }

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
