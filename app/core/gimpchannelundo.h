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

#ifndef __GIMP_CHANNEL_UNDO_H__
#define __GIMP_CHANNEL_UNDO_H__


#include "gimpitemundo.h"


#define GIMP_TYPE_CHANNEL_UNDO            (gimp_channel_undo_get_type ())
#define GIMP_CHANNEL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHANNEL_UNDO, GimpChannelUndo))
#define GIMP_CHANNEL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL_UNDO, GimpChannelUndoClass))
#define GIMP_IS_CHANNEL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CHANNEL_UNDO))
#define GIMP_IS_CHANNEL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL_UNDO))
#define GIMP_CHANNEL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CHANNEL_UNDO, GimpChannelUndoClass))


typedef struct _GimpChannelUndo      GimpChannelUndo;
typedef struct _GimpChannelUndoClass GimpChannelUndoClass;

struct _GimpChannelUndo
{
  GimpItemUndo  parent_instance;

  GimpChannel  *prev_parent;
  gint          prev_position;   /*  former position in list     */
  GList        *prev_channels;   /*  previous selected channels  */
};

struct _GimpChannelUndoClass
{
  GimpItemUndoClass  parent_class;
};


GType   gimp_channel_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_CHANNEL_UNDO_H__ */
