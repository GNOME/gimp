/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagedock.c
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "gimpdialogfactory.h"
#include "gimpimagedock.h"
#include "gimpcontainermenuimpl.h"
#include "gimpdockbook.h"

#include "gimpdnd.h"


static void   gimp_image_dock_class_init (GimpImageDockClass *klass);
static void   gimp_image_dock_init       (GimpImageDock      *dock);

static void   gimp_image_dock_destroy    (GtkObject          *object);


static GimpDockClass *parent_class = NULL;


GtkType
gimp_image_dock_get_type (void)
{
  static GtkType dock_type = 0;

  if (! dock_type)
    {
      static const GtkTypeInfo dock_info =
      {
	"GimpImageDock",
	sizeof (GimpImageDock),
	sizeof (GimpImageDockClass),
	(GtkClassInitFunc) gimp_image_dock_class_init,
	(GtkObjectInitFunc) gimp_image_dock_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      dock_type = gtk_type_unique (GIMP_TYPE_DOCK, &dock_info);
    }

  return dock_type;
}

static void
gimp_image_dock_class_init (GimpImageDockClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DOCK);

  object_class->destroy = gimp_image_dock_destroy;
}

static void
gimp_image_dock_init (GimpImageDock *dock)
{
  dock->option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (GIMP_DOCK (dock)->main_vbox), dock->option_menu,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (GIMP_DOCK (dock)->main_vbox),
			 dock->option_menu, 0);
  gtk_widget_show (dock->option_menu);

  dock->menu = gimp_container_menu_new (NULL, NULL, 24);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (dock->option_menu), dock->menu);
  gtk_widget_show (dock->menu);
}

static void
gimp_image_dock_destroy (GtkObject *object)
{
  GimpImageDock *dock;

  dock = GIMP_IMAGE_DOCK (object);

  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_image_dock_new (GimpDialogFactory *factory,
		     GimpContainer     *image_container)
{
  GimpImageDock *image_dock;
  GimpDock      *dock;

  g_return_val_if_fail (factory != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (factory), NULL);

  image_dock = gtk_type_new (GIMP_TYPE_IMAGE_DOCK);

  dock = GIMP_DOCK (image_dock);

  dock->factory = factory;

  image_dock->menu = gimp_container_menu_new (image_container,
					      factory->context, 24);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (image_dock->option_menu),
			    image_dock->menu);
  gtk_widget_show (image_dock->menu);

  /*  set the container _after_ adding it to the option menu so it
   *  has a parent and can set the active item correctly
   */
  /*
  gimp_container_menu_set_container (GIMP_CONTAINER_MENU (image_dock->menu),
				     image_container);
  gimp_container_menu_set_context (GIMP_CONTAINER_MENU (image_dock->menu),
				   factory->context);
  */

  return GTK_WIDGET (image_dock);
}
