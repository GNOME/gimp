/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppickableselect.h
 * Copyright (C) 2023 Jehan
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


#define GIMP_TYPE_PICKABLE_SELECT            (gimp_pickable_select_get_type ())
#define GIMP_PICKABLE_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PICKABLE_SELECT, GimpPickableSelect))
#define GIMP_PICKABLE_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PICKABLE_SELECT, GimpPickableSelectClass))
#define GIMP_IS_PICKABLE_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PICKABLE_SELECT))
#define GIMP_IS_PICKABLE_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PICKABLE_SELECT))
#define GIMP_PICKABLE_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PICKABLE_SELECT, GimpPickableSelectClass))


typedef struct _GimpPickableSelectClass  GimpPickableSelectClass;

struct _GimpPickableSelect
{
  GimpPdbDialog  parent_instance;

  GtkWidget     *chooser;
};

struct _GimpPickableSelectClass
{
  GimpPdbDialogClass  parent_class;
};


GType  gimp_pickable_select_get_type (void) G_GNUC_CONST;
