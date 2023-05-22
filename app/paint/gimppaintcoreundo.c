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

#include "paint-types.h"

#include "gimppaintcore.h"
#include "gimppaintcoreundo.h"


enum
{
  PROP_0,
  PROP_PAINT_CORE
};


static void   gimp_paint_core_undo_constructed  (GObject             *object);
static void   gimp_paint_core_undo_set_property (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void   gimp_paint_core_undo_get_property (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void   gimp_paint_core_undo_pop          (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode,
                                                 GimpUndoAccumulator *accum);
static void   gimp_paint_core_undo_free         (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpPaintCoreUndo, gimp_paint_core_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_paint_core_undo_parent_class


static void
gimp_paint_core_undo_class_init (GimpPaintCoreUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_paint_core_undo_constructed;
  object_class->set_property = gimp_paint_core_undo_set_property;
  object_class->get_property = gimp_paint_core_undo_get_property;

  undo_class->pop            = gimp_paint_core_undo_pop;
  undo_class->free           = gimp_paint_core_undo_free;

  g_object_class_install_property (object_class, PROP_PAINT_CORE,
                                   g_param_spec_object ("paint-core", NULL, NULL,
                                                        GIMP_TYPE_PAINT_CORE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_paint_core_undo_init (GimpPaintCoreUndo *undo)
{
}

static void
gimp_paint_core_undo_constructed (GObject *object)
{
  GimpPaintCoreUndo *paint_core_undo = GIMP_PAINT_CORE_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_PAINT_CORE (paint_core_undo->paint_core));

  paint_core_undo->last_coords = paint_core_undo->paint_core->start_coords;
}

static void
gimp_paint_core_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpPaintCoreUndo *paint_core_undo = GIMP_PAINT_CORE_UNDO (object);

  switch (property_id)
    {
    case PROP_PAINT_CORE:
      g_set_weak_pointer (&paint_core_undo->paint_core,
                          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_core_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpPaintCoreUndo *paint_core_undo = GIMP_PAINT_CORE_UNDO (object);

  switch (property_id)
    {
    case PROP_PAINT_CORE:
      g_value_set_object (value, paint_core_undo->paint_core);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_paint_core_undo_pop (GimpUndo              *undo,
                          GimpUndoMode           undo_mode,
                          GimpUndoAccumulator   *accum)
{
  GimpPaintCoreUndo *paint_core_undo = GIMP_PAINT_CORE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  /*  only pop if the core still exists  */
  if (paint_core_undo->paint_core)
    {
      GimpCoords tmp_coords;

      tmp_coords = paint_core_undo->paint_core->last_coords;
      paint_core_undo->paint_core->last_coords = paint_core_undo->last_coords;
      paint_core_undo->last_coords = tmp_coords;
    }
}

static void
gimp_paint_core_undo_free (GimpUndo     *undo,
                           GimpUndoMode  undo_mode)
{
  GimpPaintCoreUndo *paint_core_undo = GIMP_PAINT_CORE_UNDO (undo);

  g_clear_weak_pointer (&paint_core_undo->paint_core);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
