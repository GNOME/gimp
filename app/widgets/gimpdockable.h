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


#include <gtk/gtkbin.h>


typedef GtkWidget * (* GimpDockableGetPreviewFunc) (GimpDockable *dockable,
                                                    GimpContext  *context,
						    GtkIconSize   size,
                                                    gpointer      get_preview_data);
typedef void        (* GimpDockableSetContextFunc) (GimpDockable *dockable,
						    GimpContext  *context);


#define GIMP_TYPE_DOCKABLE            (gimp_dockable_get_type ())
#define GIMP_DOCKABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCKABLE, GimpDockable))
#define GIMP_DOCKABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCKABLE, GimpDockableClass))
#define GIMP_IS_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCKABLE))
#define GIMP_IS_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCKABLE))
#define GIMP_DOCKABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DOCKABLE, GimpDockableClass))


typedef struct _GimpDockableClass GimpDockableClass;

struct _GimpDockable
{
  GtkBin        parent_instance;

  gchar        *name;
  gchar        *blurb;
  gchar        *stock_id;
  GimpTabStyle  tab_style;

  GimpDockbook *dockbook;

  GimpContext  *context;

  GimpDockableGetPreviewFunc  get_preview_func;
  gpointer                    get_preview_data;
  GimpDockableSetContextFunc  set_context_func;
};

struct _GimpDockableClass
{
  GtkBinClass  parent_class;

  GtkWidget * (* get_tab_widget) (GimpDockable *dockable,
                                  GimpContext  *context,
                                  GimpTabStyle  tab_style,
				  GtkIconSize   size);
  void        (* set_context)    (GimpDockable *dockable,
				  GimpContext  *context);
};


GType       gimp_dockable_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_dockable_new      (const gchar                *name,
				    const gchar                *blurb,
                                    const gchar                *stock_id,
				    GimpDockableGetPreviewFunc  get_preview_func,
                                    gpointer                    get_preview_data,
				    GimpDockableSetContextFunc  set_context_func);

GtkWidget * gimp_dockable_get_tab_widget (GimpDockable           *dockable,
                                          GimpContext            *context,
                                          GimpTabStyle            tab_style,
					  GtkIconSize             size);
void        gimp_dockable_set_context    (GimpDockable           *dockable,
					  GimpContext            *context);


#endif /* __GIMP_DOCKABLE_H__ */
