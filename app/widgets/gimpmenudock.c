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

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "gimpdialogfactory.h"
#include "gimpimagedock.h"
#include "gimpcontainermenuimpl.h"
#include "gimpdnd.h"
#include "gimpdockbook.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"

#include "libgimp/gimpintl.h"


static void   gimp_image_dock_class_init    (GimpImageDockClass *klass);
static void   gimp_image_dock_init          (GimpImageDock      *dock);

static void   gimp_image_dock_destroy       (GtkObject          *object);

static void   gimp_image_dock_factory_image_changed (GimpContext        *context,
						     GimpImage          *gimage,
						     GimpDock           *dock);
static void   gimp_image_dock_image_changed         (GimpContext        *context,
						     GimpImage          *gimage,
						     GimpDock           *dock);
static void   gimp_image_dock_auto_clicked          (GtkWidget          *widget,
						     GimpDock           *dock);


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
  GtkWidget *hbox;
  GtkWidget *toggle;

  dock->image_container    = NULL;
  dock->auto_follow_active = TRUE;

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox, 0);
  gtk_widget_show (hbox);

  dock->option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dock->option_menu, TRUE, TRUE, 0);
  gtk_widget_show (dock->option_menu);

  toggle = gtk_toggle_button_new_with_label (_("Auto"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				dock->auto_follow_active);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  gtk_signal_connect (GTK_OBJECT (toggle), "clicked",
		      GTK_SIGNAL_FUNC (gimp_image_dock_auto_clicked),
		      dock);
}

static void
gimp_image_dock_destroy (GtkObject *object)
{
  GimpImageDock *dock;

  dock = GIMP_IMAGE_DOCK (object);

  /*  remove the image menu and the auto button manually here because
   *  of weird cross-connections with GimpDock's context
   */
  gtk_container_remove (GTK_CONTAINER (GIMP_DOCK (dock)->main_vbox),
			dock->option_menu->parent);

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

  g_return_val_if_fail (image_container != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (image_container), NULL);

  image_dock = gtk_type_new (GIMP_TYPE_IMAGE_DOCK);

  dock = GIMP_DOCK (image_dock);

  image_dock->image_container = image_container;

  dock->factory = factory;

  dock->context = gimp_context_new ("Dock Context", NULL);
  gimp_context_define_args (dock->context,
			    GIMP_CONTEXT_ALL_ARGS_MASK &
			    ~(GIMP_CONTEXT_IMAGE_MASK |
			      GIMP_CONTEXT_DISPLAY_MASK),
			    FALSE);
  gimp_context_set_parent (dock->context, factory->context);

  if (image_dock->auto_follow_active)
    gimp_context_copy_arg (factory->context, dock->context,
			   GIMP_CONTEXT_ARG_IMAGE);

  gtk_signal_connect_while_alive
    (GTK_OBJECT (factory->context), "image_changed",
     GTK_SIGNAL_FUNC (gimp_image_dock_factory_image_changed),
     dock,
     GTK_OBJECT (dock));

  gtk_signal_connect_while_alive
    (GTK_OBJECT (dock->context), "image_changed",
     GTK_SIGNAL_FUNC (gimp_image_dock_image_changed),
     dock,
     GTK_OBJECT (dock));

  image_dock->menu = gimp_container_menu_new (image_container,
					      dock->context, 24);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (image_dock->option_menu),
			    image_dock->menu);
  gtk_widget_show (image_dock->menu);

  return GTK_WIDGET (image_dock);
}

void
gimp_image_dock_set_show_image_menu (GimpImageDock *image_dock,
				     gboolean       show)
{
  g_return_if_fail (image_dock != NULL);
  g_return_if_fail (GIMP_IS_IMAGE_DOCK (image_dock));

  if (show)
    {
      gtk_widget_show (image_dock->option_menu->parent);
    }
  else
    {
      gtk_widget_hide (image_dock->option_menu->parent);
    }
}

static void
gimp_image_dock_factory_image_changed (GimpContext *context,
				       GimpImage   *gimage,
				       GimpDock    *dock)
{
  GimpImageDock *image_dock;

  image_dock = GIMP_IMAGE_DOCK (dock);

  if (gimage && image_dock->auto_follow_active)
    {
      gimp_context_set_image (dock->context, gimage);
    }
}

static void
gimp_image_dock_image_changed (GimpContext *context,
			       GimpImage   *gimage,
			       GimpDock    *dock)
{
  GimpImageDock *image_dock;

  image_dock = GIMP_IMAGE_DOCK (dock);

  if (! gimage && image_dock->image_container->num_children > 0)
    {
      gimage = GIMP_IMAGE (gimp_container_get_child_by_index (image_dock->image_container, 0));

      if (gimage)
	{
	  /*  this invokes this function recursively but we don't enter
	   *  the if() branch the second time
	   */
	  gimp_context_set_image (dock->context, gimage);

	  /*  stop the emission of the original signal (the emission of
	   *  the recursive signal is finished)
	   */
	  gtk_signal_emit_stop_by_name (GTK_OBJECT (context), "image_changed");
	}
    }
}

static void
gimp_image_dock_auto_clicked (GtkWidget *widget,
			      GimpDock  *dock)
{
  GimpImageDock *image_dock;

  image_dock = GIMP_IMAGE_DOCK (dock);

  gimp_toggle_button_update (widget, &image_dock->auto_follow_active);

  if (image_dock->auto_follow_active)
    {
      GimpImage *gimage;

      gimage = gimp_context_get_image (dock->factory->context);

      if (gimage)
	gimp_context_set_image (dock->context, gimage);
    }
}
