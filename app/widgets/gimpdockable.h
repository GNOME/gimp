/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdockable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DOCKABLE_H__
#define __GIMP_DOCKABLE_H__


#include <gtk/gtkvbox.h>


typedef GtkWidget * (* GimpDockableGetTabFunc) (GimpDockable *dockable,
						gint          size);


#define GIMP_TYPE_DOCKABLE            (gimp_dockable_get_type ())
#define GIMP_DOCKABLE(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_DOCKABLE, GimpDockable))
#define GIMP_DOCKABLE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCKABLE, GimpDockableClass))
#define GIMP_IS_DOCKABLE(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DOCKABLE))
#define GIMP_IS_DOCKABLE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCKABLE))


typedef struct _GimpDockableClass   GimpDockableClass;


struct _GimpDockable
{
  GtkVBox       parent_instance;

  gchar        *name;
  gchar        *short_name;

  GimpDockbook *dockbook;

  GimpDockableGetTabFunc  get_tab_func;
};

struct _GimpDockableClass
{
  GtkVBoxClass  parent_class;
};


GtkType     gimp_dockable_get_type       (void);
GtkWidget * gimp_dockable_new            (const gchar            *name,
					  const gchar            *short_name,
					  GimpDockableGetTabFunc  get_tab_func);

GtkWidget * gimp_dockable_get_tab_widget (GimpDockable           *dockable,
					  gint                    size);


#endif /* __GIMP_DOCKABLE_H__ */
