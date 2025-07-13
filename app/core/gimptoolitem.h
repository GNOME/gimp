/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolitem.h
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

#pragma once

#include "gimpviewable.h"


#define GIMP_TYPE_TOOL_ITEM            (gimp_tool_item_get_type ())
#define GIMP_TOOL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_ITEM, GimpToolItem))
#define GIMP_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_ITEM, GimpToolItemClass))
#define GIMP_IS_TOOL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_ITEM))
#define GIMP_IS_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_ITEM))
#define GIMP_TOOL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_ITEM, GimpToolItemClass))


typedef struct _GimpToolItemPrivate GimpToolItemPrivate;
typedef struct _GimpToolItemClass   GimpToolItemClass;

struct _GimpToolItem
{
  GimpViewable         parent_instance;

  GimpToolItemPrivate *priv;
};

struct _GimpToolItemClass
{
  GimpViewableClass  parent_class;

  /*  signals  */
  void (* visible_changed) (GimpToolItem *tool_item);
  void (* shown_changed)   (GimpToolItem *tool_item);
};


GType      gimp_tool_item_get_type      (void) G_GNUC_CONST;

void       gimp_tool_item_set_visible   (GimpToolItem *tool_item,
                                         gboolean      visible);
gboolean   gimp_tool_item_get_visible   (GimpToolItem *tool_item);

gboolean   gimp_tool_item_get_shown     (GimpToolItem *tool_item);


/*  protected  */

void       gimp_tool_item_shown_changed (GimpToolItem *tool_item);
