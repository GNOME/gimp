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

#ifndef __GIMP_DATA_H__
#define __GIMP_DATA_H__


#include "gimpviewable.h"


#define GIMP_TYPE_DATA            (gimp_data_get_type ())
#define GIMP_DATA(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DATA, GimpData))
#define GIMP_DATA_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA, GimpDataClass))
#define GIMP_IS_DATA(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DATA))
#define GIMP_IS_DATA_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA))


typedef struct _GimpDataClass GimpDataClass;

struct _GimpData
{
  GimpViewable  parent_instance;

  gchar        *filename;
  gboolean      dirty;
};

struct _GimpDataClass
{
  GimpViewableClass  parent_class;

  void       (* dirty)         (GimpData *data);
  gboolean   (* save)          (GimpData *data);
  gchar    * (* get_extension) (GimpData *data);
};


GtkType       gimp_data_get_type         (void);

gboolean      gimp_data_save             (GimpData    *data);

void          gimp_data_dirty            (GimpData    *data);
gboolean      gimp_data_delete_from_disk (GimpData    *data);

const gchar * gimp_data_get_extension    (GimpData    *data);

void          gimp_data_set_filename     (GimpData    *data,
					  const gchar *filename);
void          gimp_data_create_filename  (GimpData    *data,
					  const gchar *filename,
					  const gchar *data_path);


#endif /* __GIMP_DATA_H__ */
