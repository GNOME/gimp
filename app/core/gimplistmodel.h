/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimplistmodel.h
 * Copyright (C) 2022 Niels De Graef <nielsdegraef@gmail.com>
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

#ifndef __GIMP_LIST_H__
#define __GIMP_LIST_H__


#include "gimpobject.h"


#define GIMP_TYPE_LIST_MODEL (gimp_list_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpListModel, gimp_list_model, GIMP, LIST_MODEL, GimpObject)

struct _GimpListClass
{
  GimpObjectClass parent_class;
};

GimpObject *   gimp_list_model_find             (GimpListModel *self,
                                                 guint         *position);

GimpObject *   gimp_list_model_find_by_name     (GimpListModel *self,
                                                 const gchar *name,
                                                 guint       *position);

#endif  /* __GIMP_LIST_H__ */
