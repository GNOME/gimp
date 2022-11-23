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

#ifndef __LIGMA_ITEM_PROP_UNDO_H__
#define __LIGMA_ITEM_PROP_UNDO_H__


#include "ligmaitemundo.h"


#define LIGMA_TYPE_ITEM_PROP_UNDO            (ligma_item_prop_undo_get_type ())
#define LIGMA_ITEM_PROP_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ITEM_PROP_UNDO, LigmaItemPropUndo))
#define LIGMA_ITEM_PROP_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ITEM_PROP_UNDO, LigmaItemPropUndoClass))
#define LIGMA_IS_ITEM_PROP_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ITEM_PROP_UNDO))
#define LIGMA_IS_ITEM_PROP_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ITEM_PROP_UNDO))
#define LIGMA_ITEM_PROP_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ITEM_PROP_UNDO, LigmaItemPropUndoClass))


typedef struct _LigmaItemPropUndo      LigmaItemPropUndo;
typedef struct _LigmaItemPropUndoClass LigmaItemPropUndoClass;

struct _LigmaItemPropUndo
{
  LigmaItemUndo  parent_instance;

  LigmaItem     *parent;
  gint          position;
  gchar        *name;
  gint          offset_x;
  gint          offset_y;
  guint         visible         : 1;
  guint         linked          : 1;
  guint         lock_content    : 1;
  guint         lock_position   : 1;
  guint         lock_visibility : 1;
  LigmaColorTag  color_tag;
  gchar        *parasite_name;
  LigmaParasite *parasite;
};

struct _LigmaItemPropUndoClass
{
  LigmaItemUndoClass  parent_class;
};


GType   ligma_item_prop_undo_get_type (void) G_GNUC_CONST;


#endif /* __LIGMA_ITEM_PROP_UNDO_H__ */
