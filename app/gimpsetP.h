/* The GIMP -- an image manipulation program
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
#ifndef __GIMP_SET_P_H__
#define __GIMP_SET_P_H__

#include "gimpobjectP.h"
#include "gimpset.h"

#define GIMP_SET_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_set_get_type(), GimpSetClass)

typedef struct _GimpSetClass GimpSetClass;

struct _GimpSet
{
  GimpObject  gobject;

  GtkType     type;
  GSList     *list;
  GArray     *handlers;
  gboolean    weak;
  gpointer    active_element;
};

struct _GimpSetClass
{
  GimpObjectClass parent_class;

  void (* add)            (GimpSet *gimpset, gpointer object);
  void (* remove)         (GimpSet *gimpset, gpointer object);
  void (* active_changed) (GimpSet *gimpset, gpointer object);
};

#endif /* __GIMP_SET_P_H__ */
