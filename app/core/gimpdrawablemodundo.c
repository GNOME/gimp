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

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp-memsize.h"
#include "gimpimage.h"
#include "gimpdrawable.h"
#include "gimpdrawablemodundo.h"


enum
{
  PROP_0,
  PROP_COPY_BUFFER
};


static void     gimp_drawable_mod_undo_constructed  (GObject             *object);
static void     gimp_drawable_mod_undo_set_property (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     gimp_drawable_mod_undo_get_property (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static gint64   gimp_drawable_mod_undo_get_memsize  (GimpObject          *object,
                                                     gint64              *gui_size);

static void     gimp_drawable_mod_undo_pop          (GimpUndo            *undo,
                                                     GimpUndoMode         undo_mode,
                                                     GimpUndoAccumulator *accum);
static void     gimp_drawable_mod_undo_free         (GimpUndo            *undo,
                                                     GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpDrawableModUndo, gimp_drawable_mod_undo, GIMP_TYPE_ITEM_UNDO)

#define parent_class gimp_drawable_mod_undo_parent_class


static void
gimp_drawable_mod_undo_class_init (GimpDrawableModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed      = gimp_drawable_mod_undo_constructed;
  object_class->set_property     = gimp_drawable_mod_undo_set_property;
  object_class->get_property     = gimp_drawable_mod_undo_get_property;

  gimp_object_class->get_memsize = gimp_drawable_mod_undo_get_memsize;

  undo_class->pop                = gimp_drawable_mod_undo_pop;
  undo_class->free               = gimp_drawable_mod_undo_free;

  g_object_class_install_property (object_class, PROP_COPY_BUFFER,
                                   g_param_spec_boolean ("copy-buffer",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_drawable_mod_undo_init (GimpDrawableModUndo *undo)
{
}

static void
gimp_drawable_mod_undo_constructed (GObject *object)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);
  GimpItem            *item;
  GimpDrawable        *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_DRAWABLE (GIMP_ITEM_UNDO (object)->item));

  item     = GIMP_ITEM_UNDO (object)->item;
  drawable = GIMP_DRAWABLE (item);

  if (drawable_mod_undo->copy_buffer)
    {
      drawable_mod_undo->buffer =
        gimp_gegl_buffer_dup (gimp_drawable_get_buffer (drawable));
    }
  else
    {
      drawable_mod_undo->buffer =
        g_object_ref (gimp_drawable_get_buffer (drawable));
    }

  gimp_item_get_offset (item,
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);
}

static void
gimp_drawable_mod_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      drawable_mod_undo->copy_buffer = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_mod_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      g_value_set_boolean (value, drawable_mod_undo->copy_buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_drawable_mod_undo_get_memsize (GimpObject *object,
                                    gint64     *gui_size)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (object);
  gint64               memsize           = 0;

  memsize += gimp_gegl_buffer_get_memsize (drawable_mod_undo->buffer);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_drawable_mod_undo_pop (GimpUndo            *undo,
                            GimpUndoMode         undo_mode,
                            GimpUndoAccumulator *accum)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (undo);
  GimpDrawable        *drawable          = GIMP_DRAWABLE (GIMP_ITEM_UNDO (undo)->item);
  GeglBuffer          *buffer;
  gint                 offset_x;
  gint                 offset_y;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  buffer   = drawable_mod_undo->buffer;
  offset_x = drawable_mod_undo->offset_x;
  offset_y = drawable_mod_undo->offset_y;

  drawable_mod_undo->buffer = g_object_ref (gimp_drawable_get_buffer (drawable));

  gimp_item_get_offset (GIMP_ITEM (drawable),
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);

  gimp_drawable_set_buffer_full (drawable, FALSE, NULL,
                                 buffer,
                                 GEGL_RECTANGLE (offset_x, offset_y, 0, 0),
                                 TRUE);
  g_object_unref (buffer);
}

static void
gimp_drawable_mod_undo_free (GimpUndo     *undo,
                             GimpUndoMode  undo_mode)
{
  GimpDrawableModUndo *drawable_mod_undo = GIMP_DRAWABLE_MOD_UNDO (undo);

  g_clear_object (&drawable_mod_undo->buffer);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
