/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_INK_UNDO_H__
#define __GIMP_INK_UNDO_H__


#include "gimppaintcoreundo.h"


#define GIMP_TYPE_INK_UNDO            (gimp_ink_undo_get_type ())
#define GIMP_INK_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INK_UNDO, GimpInkUndo))
#define GIMP_INK_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INK_UNDO, GimpInkUndoClass))
#define GIMP_IS_INK_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INK_UNDO))
#define GIMP_IS_INK_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INK_UNDO))
#define GIMP_INK_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INK_UNDO, GimpInkUndoClass))


typedef struct _GimpInkUndoClass GimpInkUndoClass;

struct _GimpInkUndo
{
  GimpPaintCoreUndo  parent_instance;

  Blob              *last_blob;

  gdouble            dt_buffer[DIST_SMOOTHER_BUFFER];
  gint               dt_index;

  guint32            ts_buffer[TIME_SMOOTHER_BUFFER];
  gint               ts_index;

  gdouble            last_time;

  gboolean           init_velocity;
};

struct _GimpInkUndoClass
{
  GimpPaintCoreUndoClass  parent_class;
};


GType   gimp_ink_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_INK_UNDO_H__ */
