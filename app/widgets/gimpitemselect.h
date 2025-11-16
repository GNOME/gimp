/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemselect.h
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

#include "gimppdbdialog.h"


#define GIMP_TYPE_ITEM_SELECT            (gimp_item_select_get_type ())
#define GIMP_ITEM_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_SELECT, GimpItemSelect))
#define GIMP_ITEM_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_SELECT, GimpItemSelectClass))
#define GIMP_IS_ITEM_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_SELECT))
#define GIMP_IS_ITEM_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_SELECT))
#define GIMP_ITEM_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_SELECT, GimpItemSelectClass))


typedef struct _GimpItemSelectClass  GimpItemSelectClass;

struct _GimpItemSelect
{
  GimpPdbDialog  parent_instance;

  GtkWidget     *chooser;
};

struct _GimpItemSelectClass
{
  GimpPdbDialogClass  parent_class;
};


GType  gimp_item_select_get_type (void) G_GNUC_CONST;
