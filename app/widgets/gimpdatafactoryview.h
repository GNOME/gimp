/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.h
 * Copyright (C) 2001 Michael Natterer
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

#ifndef __GIMP_DATA_FACTORY_VIEW_H__
#define __GIMP_DATA_FACTORY_VIEW_H__


#include "gimpcontainereditor.h"


typedef void (* GimpDataEditFunc) (GimpData *data);


#define GIMP_TYPE_DATA_FACTORY_VIEW            (gimp_data_factory_view_get_type ())
#define GIMP_DATA_FACTORY_VIEW(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DATA_FACTORY_VIEW, GimpDataFactoryView))
#define GIMP_DATA_FACTORY_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_FACTORY_VIEW, GimpDataFactoryViewClass))
#define GIMP_IS_DATA_FACTORY_VIEW(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DATA_FACTORY_VIEW))
#define GIMP_IS_DATA_FACTORY_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_FACTORY_VIEW))


typedef struct _GimpDataFactoryViewClass  GimpDataFactoryViewClass;

struct _GimpDataFactoryView
{
  GimpContainerEditor  parent_instance;

  GimpDataFactory     *factory;
  GimpDataEditFunc     data_edit_func;

  GtkWidget           *new_button;
  GtkWidget           *duplicate_button;
  GtkWidget           *edit_button;
  GtkWidget           *delete_button;

  GtkWidget           *refresh_button;
};

struct _GimpDataFactoryViewClass
{
  GimpContainerEditorClass  parent_class;
};


GtkType     gimp_data_factory_view_get_type (void);

GtkWidget * gimp_data_factory_view_new      (GimpViewType      view_type,
					     GimpDataFactory  *factory,
					     GimpDataEditFunc  edit_func,
					     GimpContext      *context,
					     gint              preview_size,
					     gint              min_items_x,
					     gint              min_items_y);


#endif  /*  __GIMP_DATA_FACTORY_VIEW_H__  */
