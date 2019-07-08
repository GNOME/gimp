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

#ifndef __GIMP_TEXT_UNDO_H__
#define __GIMP_TEXT_UNDO_H__


#include "core/gimpitemundo.h"


#define GIMP_TYPE_TEXT_UNDO            (gimp_text_undo_get_type ())
#define GIMP_TEXT_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_UNDO, GimpTextUndo))
#define GIMP_TEXT_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_UNDO, GimpTextUndoClass))
#define GIMP_IS_TEXT_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_UNDO))
#define GIMP_IS_TEXT_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_UNDO))
#define GIMP_TEXT_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEXT_UNDO, GimpTextUndoClass))


typedef struct _GimpTextUndoClass GimpTextUndoClass;

struct _GimpTextUndo
{
  GimpItemUndo      parent_instance;

  GimpText         *text;
  const GParamSpec *pspec;
  GValue           *value;
  gboolean          modified;
  const Babl       *format;
};

struct _GimpTextUndoClass
{
  GimpItemClass  parent_class;
};


GType      gimp_text_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_TEXT_UNDO_H__ */
