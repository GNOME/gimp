/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenuitem.c
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

#include "apptypes.h"

#include "gimpcontainer.h"
#include "gimpdnd.h"
#include "gimpmenuitem.h"
#include "gimpmarshal.h"
#include "gimppreview.h"
#include "gimpviewable.h"


enum
{
  SET_VIEWABLE,
  LAST_SIGNAL
};


static void           gimp_menu_item_class_init    (GimpMenuItemClass *klass);
static void           gimp_menu_item_init          (GimpMenuItem      *menu_item);

static void           gimp_menu_item_set_viewable  (GimpMenuItem      *menu_item,
                                                    GimpViewable      *viewable);

static void           gimp_menu_item_real_set_viewable (GimpMenuItem  *menu_item,
                                                        GimpViewable  *viewable);

static void           gimp_menu_item_name_changed  (GimpViewable      *viewable,
                                                    GtkLabel          *label);
static GimpViewable * gimp_menu_item_drag_viewable (GtkWidget         *widget,
                                                    gpointer           data);


static guint menu_item_signals[LAST_SIGNAL] = { 0 };

static GtkMenuItemClass *parent_class       = NULL;


GtkType
gimp_menu_item_get_type (void)
{
  static GtkType menu_item_type = 0;

  if (!menu_item_type)
    {
      static const GtkTypeInfo menu_item_info =
      {
	"GimpMenuItem",
	sizeof (GimpMenuItem),
	sizeof (GimpMenuItemClass),
	(GtkClassInitFunc) gimp_menu_item_class_init,
	(GtkObjectInitFunc) gimp_menu_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      menu_item_type = gtk_type_unique (GTK_TYPE_MENU_ITEM, &menu_item_info);
    }

  return menu_item_type;
}

static void
gimp_menu_item_class_init (GimpMenuItemClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_MENU_ITEM);

  menu_item_signals[SET_VIEWABLE] = 
    gtk_signal_new ("set_viewable",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpMenuItemClass,
                                       set_viewable),
                    gtk_marshal_NONE__OBJECT,
                    GTK_TYPE_NONE, 1,
                    GIMP_TYPE_VIEWABLE);

  klass->set_viewable = gimp_menu_item_real_set_viewable;
}

static void
gimp_menu_item_init (GimpMenuItem *menu_item)
{
  menu_item->hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (menu_item), menu_item->hbox);
  gtk_widget_show (menu_item->hbox);

  menu_item->preview      = NULL;
  menu_item->name_label   = NULL;

  menu_item->preview_size = 0;
}

GtkWidget *
gimp_menu_item_new (GimpViewable  *viewable,
                    gint           preview_size)
{
  GimpMenuItem *menu_item;

  g_return_val_if_fail (viewable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 256, NULL);

  menu_item = gtk_type_new (GIMP_TYPE_MENU_ITEM);

  menu_item->preview_size = preview_size;

  gimp_menu_item_set_viewable (menu_item, viewable);

  return GTK_WIDGET (menu_item);
}

static void
gimp_menu_item_set_viewable (GimpMenuItem *menu_item,
                             GimpViewable *viewable)
{
  gtk_signal_emit (GTK_OBJECT (menu_item), menu_item_signals[SET_VIEWABLE],
                   viewable);
}

static void
gimp_menu_item_real_set_viewable (GimpMenuItem *menu_item,
                                  GimpViewable *viewable)
{
  menu_item->preview = gimp_preview_new (viewable, menu_item->preview_size,
                                         1, FALSE);
  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->preview,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->preview);

  menu_item->name_label =
    gtk_label_new (gimp_object_get_name (GIMP_OBJECT (viewable)));
  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->name_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->name_label);

  gtk_signal_connect_while_alive (GTK_OBJECT (viewable), "name_changed",
                                  GTK_SIGNAL_FUNC (gimp_menu_item_name_changed),
                                  menu_item->name_label,
                                  GTK_OBJECT (menu_item->name_label));

  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (menu_item),
				    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				    GTK_OBJECT (viewable)->klass->type,
				    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gimp_dnd_viewable_source_set (GTK_WIDGET (menu_item),
                                GTK_OBJECT (viewable)->klass->type,
				gimp_menu_item_drag_viewable,
				NULL);
}

static void
gimp_menu_item_name_changed (GimpViewable *viewable,
                             GtkLabel     *label)
{
  gtk_label_set_text (label, gimp_object_get_name (GIMP_OBJECT (viewable)));
}

static GimpViewable *
gimp_menu_item_drag_viewable (GtkWidget *widget,
                              gpointer   data)
{
  return GIMP_PREVIEW (GIMP_MENU_ITEM (widget)->preview)->viewable;
}
