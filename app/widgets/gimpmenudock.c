/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagedock.c
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
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


#define DEFAULT_MINIMAL_WIDTH 250


static void   gimp_image_dock_class_init            (GimpImageDockClass *klass);
static void   gimp_image_dock_init                  (GimpImageDock      *dock);

static void   gimp_image_dock_destroy               (GtkObject          *object);

static void   gimp_image_dock_style_set             (GtkWidget          *widget,
                                                     GtkStyle           *prev_style);

static void   gimp_image_dock_factory_image_changed (GimpContext        *context,
						     GimpImage          *gimage,
						     GimpDock           *dock);
static void   gimp_image_dock_image_changed         (GimpContext        *context,
						     GimpImage          *gimage,
						     GimpDock           *dock);
static void   gimp_image_dock_auto_clicked          (GtkWidget          *widget,
						     GimpDock           *dock);


static GimpDockClass *parent_class = NULL;


GType
gimp_image_dock_get_type (void)
{
  static GType dock_type = 0;

  if (! dock_type)
    {
      static const GTypeInfo dock_info =
      {
        sizeof (GimpImageDockClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_image_dock_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImageDock),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_image_dock_init,
      };

      dock_type = g_type_register_static (GIMP_TYPE_DOCK,
                                          "GimpImageDock",
                                          &dock_info, 0);
    }

  return dock_type;
}

static void
gimp_image_dock_class_init (GimpImageDockClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy   = gimp_image_dock_destroy;

  widget_class->style_set = gimp_image_dock_style_set;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal_width",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_WIDTH,
                                                             G_PARAM_READABLE));
}

static void
gimp_image_dock_init (GimpImageDock *dock)
{
  GtkWidget *hbox;

  dock->image_container    = NULL;
  dock->show_image_menu    = FALSE;
  dock->auto_follow_active = TRUE;

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (GIMP_DOCK (dock)->main_vbox), hbox, 0);

  if (dock->show_image_menu)
    gtk_widget_show (hbox);

  dock->option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dock->option_menu, TRUE, TRUE, 0);
  gtk_widget_show (dock->option_menu);

  g_signal_connect (G_OBJECT (dock->option_menu), "destroy",
		    G_CALLBACK (gtk_widget_destroyed),
		    &dock->option_menu);

  dock->auto_button = gtk_toggle_button_new_with_label (_("Auto"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock->auto_button),
				dock->auto_follow_active);
  gtk_box_pack_start (GTK_BOX (hbox), dock->auto_button, FALSE, FALSE, 0);
  gtk_widget_show (dock->auto_button);

  g_signal_connect (G_OBJECT (dock->auto_button), "clicked",
		    G_CALLBACK (gimp_image_dock_auto_clicked),
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
  if (GIMP_DOCK (dock)->main_vbox &&
      dock->option_menu           &&
      dock->option_menu->parent)
    {
      gtk_container_remove (GTK_CONTAINER (GIMP_DOCK (dock)->main_vbox),
			    dock->option_menu->parent);
    }

  if (GTK_OBJECT_CLASS (parent_class))
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_image_dock_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  gint minimal_width;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "minimal_width",  &minimal_width,
			NULL);

  gtk_widget_set_size_request (widget, minimal_width, -1);
}

GtkWidget *
gimp_image_dock_new (GimpDialogFactory *dialog_factory,
		     GimpContainer     *image_container)
{
  GimpImageDock *image_dock;
  GimpContext   *context;
  gchar         *title;

  static gint dock_counter = 1;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER (image_container), NULL);

  image_dock = g_object_new (GIMP_TYPE_IMAGE_DOCK, NULL);

  title = g_strdup_printf (_("Gimp Dock #%d"), dock_counter++);
  gtk_window_set_title (GTK_WINDOW (image_dock), title);
  g_free (title);

  image_dock->image_container = image_container;

  context = gimp_context_new (dialog_factory->context->gimp,
                              "Dock Context", NULL);

  gimp_dock_construct (GIMP_DOCK (image_dock), dialog_factory, context, TRUE);

  gimp_context_define_properties (context,
				  GIMP_CONTEXT_ALL_PROPS_MASK &
				  ~(GIMP_CONTEXT_IMAGE_MASK |
				    GIMP_CONTEXT_DISPLAY_MASK),
				  FALSE);
  gimp_context_set_parent (context, dialog_factory->context);

  if (image_dock->auto_follow_active)
    gimp_context_copy_property (dialog_factory->context, context,
				GIMP_CONTEXT_PROP_IMAGE);

  g_signal_connect_object (G_OBJECT (dialog_factory->context), "image_changed",
			   G_CALLBACK (gimp_image_dock_factory_image_changed),
			   G_OBJECT (image_dock),
			   0);

  g_signal_connect_object (G_OBJECT (context), "image_changed",
			   G_CALLBACK (gimp_image_dock_image_changed),
			   G_OBJECT (image_dock),
			   0);

  image_dock->menu = gimp_container_menu_new (image_container, context, 24);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (image_dock->option_menu),
			    image_dock->menu);
  gtk_widget_show (image_dock->menu);

  g_object_unref (G_OBJECT (context));

  return GTK_WIDGET (image_dock);
}

void
gimp_image_dock_set_auto_follow_active (GimpImageDock *image_dock,
					gboolean       auto_follow_active)
{
  g_return_if_fail (GIMP_IS_IMAGE_DOCK (image_dock));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (image_dock->auto_button),
				auto_follow_active ? TRUE : FALSE);
}

void
gimp_image_dock_set_show_image_menu (GimpImageDock *image_dock,
				     gboolean       show)
{
  g_return_if_fail (GIMP_IS_IMAGE_DOCK (image_dock));

  if (show)
    {
      gtk_widget_show (image_dock->option_menu->parent);
    }
  else
    {
      gtk_widget_hide (image_dock->option_menu->parent);
    }

  image_dock->show_image_menu = show ? TRUE : FALSE;
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
	  g_signal_stop_emission_by_name (G_OBJECT (context), "image_changed");
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

      gimage = gimp_context_get_image (dock->dialog_factory->context);

      if (gimage)
	gimp_context_set_image (dock->context, gimage);
    }
}
