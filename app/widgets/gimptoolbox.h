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

#ifndef __GIMP_TOOLBOX_H__
#define __GIMP_TOOLBOX_H__


#include "gimpimagedock.h"


#define GIMP_TYPE_TOOLBOX            (gimp_toolbox_get_type ())
#define GIMP_TOOLBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOLBOX, GimpToolbox))
#define GIMP_TOOLBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOLBOX, GimpToolboxClass))
#define GIMP_IS_TOOLBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOLBOX))
#define GIMP_IS_TOOLBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOLBOX))
#define GIMP_TOOLBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOLBOX, GimpToolboxClass))


typedef struct _GimpToolboxClass GimpToolboxClass;

struct _GimpToolbox
{
  GimpImageDock  parent_instance;

  GtkWidget     *menu_bar;
  GtkWidget     *tool_wbox;
  GtkWidget     *area_wbox;
  GtkWidget     *color_area;
  GtkWidget     *foo_area;
  GtkWidget     *image_area;

  gint           tool_rows;
  gint           tool_columns;
  gint           area_rows;
  gint           area_columns;
};

struct _GimpToolboxClass
{
  GimpImageDockClass  parent_class;
};


GType       gimp_toolbox_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_toolbox_new      (GimpDialogFactory *factory,
                                   GimpContext       *context);


#endif /* __GIMP_TOOLBOX_H__ */
