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

#ifndef __GIMP_DATA_LIST_H__
#define __GIMP_DATA_LIST_H__


#include "gimplist.h"


#define GIMP_TYPE_DATA_LIST            (gimp_data_list_get_type ())
#define GIMP_DATA_LIST(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DATA_LIST, GimpDataList))
#define GIMP_DATA_LIST_CLASS(klass)    (GTK_CHECK_CLASS_CAST (klass, GIMP_TYPE_DATA_LIST, GimpDataListClass))
#define GIMP_IS_DATA_LIST(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DATA_LIST))
#define GIMP_IS_DATA_LIST_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_LIST))


typedef struct _GimpDataListClass GimpDataListClass;

struct _GimpDataList
{
  GimpList  parent_instance;
};

struct _GimpDataListClass
{
  GimpListClass parent_class;
};


GtkType        gimp_data_list_get_type (void);
GimpDataList * gimp_data_list_new      (GtkType  children_type);


#endif  /*  __GIMP_DATA_LIST_H__  */
