/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_H__
#define __GIMP_H__


#include "gimpobject.h"


#define GIMP_TYPE_GIMP            (gimp_get_type ())
#define GIMP(obj)                 (GTK_CHECK_CAST ((obj), GIMP_TYPE_GIMP, Gimp))
#define GIMP_CLASS(klass)         (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GIMP, GimpClass))
#define GIMP_IS_GIMP(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_GIMP))
#define GIMP_IS_GIMP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GIMP))


typedef struct _GimpClass GimpClass;

struct _Gimp
{
  GimpObject        parent_instance;

  GimpContainer    *images;

  TileManager      *global_buffer;
  GimpContainer    *named_buffers;

  GimpParasiteList *parasites;

  GimpDataFactory  *brush_factory;
  GimpDataFactory  *pattern_factory;
  GimpDataFactory  *gradient_factory;
  GimpDataFactory  *palette_factory;

  GHashTable       *procedural_ht;

  GimpContainer    *tool_info_list;
};

struct _GimpClass
{
  GimpObjectClass  parent_class;
};


GtkType   gimp_get_type   (void);
Gimp    * gimp_new        (void);

void      gimp_restore    (Gimp *gimp);
void      gimp_shutdown   (Gimp *gimp);


#endif  /* __GIMP_H__ */
