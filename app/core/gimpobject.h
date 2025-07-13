/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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


#define GIMP_TYPE_OBJECT (gimp_object_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpObject,
                          gimp_object,
                          GIMP, OBJECT,
                          GObject)


struct _GimpObjectClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void    (* disconnect)   (GimpObject *object);
  void    (* name_changed) (GimpObject *object);

  /*  virtual functions  */
  gint64  (* get_memsize)  (GimpObject *object,
                            gint64     *gui_size);
};


GType         gimp_object_get_type        (void) G_GNUC_CONST;

void          gimp_object_set_name        (GimpObject       *object,
                                           const gchar      *name);
void          gimp_object_set_name_safe   (GimpObject       *object,
                                           const gchar      *name);
void          gimp_object_set_static_name (GimpObject       *object,
                                           const gchar      *name);
void          gimp_object_take_name       (GimpObject       *object,
                                           gchar            *name);
const gchar * gimp_object_get_name        (gconstpointer     object);
void          gimp_object_name_changed    (GimpObject       *object);
void          gimp_object_name_free       (GimpObject       *object);

gint          gimp_object_name_collate    (GimpObject       *object1,
                                           GimpObject       *object2);
gint64        gimp_object_get_memsize     (GimpObject       *object,
                                           gint64           *gui_size);
