/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <time.h>

#include <glib-object.h>

#include "text-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "core/gimpitem.h"
#include "core/gimpitemundo.h"

#include "gimptext.h"
#include "gimptextlayer.h"
#include "gimptextundo.h"


static void    gimp_text_undo_class_init  (GimpTextUndoClass   *klass);
static void    gimp_text_undo_init        (GimpTextUndo        *undo);
static gint64  gimp_text_undo_get_memsize (GimpObject          *object,
                                           gint64              *gui_size);

static void    gimp_text_undo_pop         (GimpUndo            *undo,
                                           GimpUndoMode         undo_mode,
                                           GimpUndoAccumulator *accum);
static void    gimp_text_undo_free        (GimpUndo            *undo,
                                           GimpUndoMode         undo_mode);


static GimpUndoClass *parent_class = NULL;


GType
gimp_text_undo_get_type (void)
{
  static GType undo_type = 0;

  if (! undo_type)
    {
      static const GTypeInfo undo_info =
      {
        sizeof (GimpTextUndoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_undo_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTextUndo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_text_undo_init,
      };

      undo_type = g_type_register_static (GIMP_TYPE_ITEM_UNDO, "GimpTextUndo",
					  &undo_info, 0);
  }

  return undo_type;
}

static void
gimp_text_undo_class_init (GimpTextUndoClass *klass)
{
  GimpObjectClass *object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class   = GIMP_UNDO_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->get_memsize = gimp_text_undo_get_memsize;

  undo_class->pop           = gimp_text_undo_pop;
  undo_class->free          = gimp_text_undo_free;
}

static void
gimp_text_undo_init (GimpTextUndo *undo)
{
  undo->text  = NULL;
  undo->pspec = NULL;
  undo->time  = 0;
}

static gint64
gimp_text_undo_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  GimpTextUndo *undo    = GIMP_TEXT_UNDO (object);
  gint64        memsize = 0;

  if (undo->pspec)
    {
      /*  this is incorrect, but how can it be done better?  */
      memsize = sizeof (GValue);
    }
  else if (undo->text)
    {
      memsize = gimp_object_get_memsize (GIMP_OBJECT (undo->text), NULL);
    }

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

  if (text_undo->pspec)
    {
      GValue *value;

      g_return_if_fail (layer->text != NULL);

      value = g_new0 (GValue, 1);
      g_value_init (value, text_undo->pspec->value_type);

      g_object_get_property (G_OBJECT (layer->text),
                             text_undo->pspec->name, value);

      g_object_set_property (G_OBJECT (layer->text),
                             text_undo->pspec->name, text_undo->value);

      g_value_unset (text_undo->value);
      g_free (text_undo->value);

      text_undo->value = value;
    }
  else
    {
      GimpText *text;

      text = (layer->text ?
              gimp_config_duplicate (GIMP_CONFIG (layer->text)) : NULL);

      if (layer->text && text_undo->text)
        gimp_config_sync (GIMP_CONFIG (text_undo->text),
                          GIMP_CONFIG (layer->text), 0);
      else
        gimp_text_layer_set_text (layer, text_undo->text);

      if (text_undo->text)
        g_object_unref (text_undo->text);

      text_undo->text = text;
    }

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);
}

static void
gimp_text_undo_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  GimpTextUndo *text_undo = GIMP_TEXT_UNDO (undo);

  if (text_undo->text)
    {
      g_object_unref (text_undo->text);
      text_undo->text = NULL;
    }

  if (text_undo->pspec)
    {
      g_value_unset (text_undo->value);
      g_free (text_undo->value);

      text_undo->value = NULL;
      text_undo->pspec = NULL;
    }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}

GimpUndo *
gimp_text_undo_new (GimpTextLayer    *layer,
                    const GParamSpec *pspec,
                    const gchar      *name)
{
  GimpUndo     *undo;
  GimpTextUndo *text_undo;

  g_return_val_if_fail (GIMP_IS_TEXT_LAYER (layer), NULL);
  g_return_val_if_fail (pspec == NULL ||
                        pspec->owner_type == GIMP_TYPE_TEXT, NULL);
  g_return_val_if_fail (pspec == NULL || layer->text != NULL, NULL);

  if (! name)
    name = gimp_undo_type_to_name (GIMP_UNDO_TEXT_LAYER);

  undo = g_object_new (GIMP_TYPE_TEXT_UNDO,
                       "name", name,
                       NULL);

  undo->gimage        = gimp_item_get_image (GIMP_ITEM (layer));
  undo->undo_type     = GIMP_UNDO_TEXT_LAYER;
  undo->size          = 0;
  undo->dirties_image = TRUE;

  GIMP_ITEM_UNDO (undo)->item = g_object_ref (layer);

  text_undo = GIMP_TEXT_UNDO (undo);

  text_undo->time = time (NULL);

  if (pspec)
    {
      text_undo->pspec = pspec;
      text_undo->value = g_new0 (GValue, 1);

      g_value_init (text_undo->value, pspec->value_type);
      g_object_get_property (G_OBJECT (layer->text),
                             pspec->name, text_undo->value);
    }
  else if (layer->text)
    {
      text_undo->text = gimp_config_duplicate (GIMP_CONFIG (layer->text));
    }

  return undo;
}
