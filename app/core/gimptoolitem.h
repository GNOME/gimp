/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolitem.h
 * Copyright (C) 2020 Ell
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

#ifndef __LIGMA_TOOL_ITEM_H__
#define __LIGMA_TOOL_ITEM_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_TOOL_ITEM            (ligma_tool_item_get_type ())
#define LIGMA_TOOL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_ITEM, LigmaToolItem))
#define LIGMA_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_ITEM, LigmaToolItemClass))
#define LIGMA_IS_TOOL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_ITEM))
#define LIGMA_IS_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_ITEM))
#define LIGMA_TOOL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_ITEM, LigmaToolItemClass))


typedef struct _LigmaToolItemPrivate LigmaToolItemPrivate;
typedef struct _LigmaToolItemClass   LigmaToolItemClass;

struct _LigmaToolItem
{
  LigmaViewable         parent_instance;

  LigmaToolItemPrivate *priv;
};

struct _LigmaToolItemClass
{
  LigmaViewableClass  parent_class;

  /*  signals  */
  void (* visible_changed) (LigmaToolItem *tool_item);
  void (* shown_changed)   (LigmaToolItem *tool_item);
};


GType      ligma_tool_item_get_type      (void) G_GNUC_CONST;
                                        
void       ligma_tool_item_set_visible   (LigmaToolItem *tool_item,
                                         gboolean      visible);
gboolean   ligma_tool_item_get_visible   (LigmaToolItem *tool_item);
                                        
gboolean   ligma_tool_item_get_shown     (LigmaToolItem *tool_item);


/*  protected  */

void       ligma_tool_item_shown_changed (LigmaToolItem *tool_item);


#endif  /*  __LIGMA_TOOL_ITEM_H__  */
