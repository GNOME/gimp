/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmenuitem.c
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

#include "widgets-types.h"

#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpdnd.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"


static void           gimp_menu_item_class_init    (GimpMenuItemClass *klass);
static void           gimp_menu_item_init          (GimpMenuItem      *menu_item);

static void           gimp_menu_item_set_viewable  (GimpMenuItem      *menu_item,
                                                    GimpViewable      *viewable);

static void           gimp_menu_item_real_set_viewable (GimpMenuItem  *menu_item,
                                                        GimpViewable  *viewable);

static void           gimp_menu_item_name_changed  (GimpViewable      *viewable,
                                                    GimpMenuItem      *menu_item);
static GimpViewable * gimp_menu_item_drag_viewable (GtkWidget         *widget,
                                                    gpointer           data);


static GtkMenuItemClass *parent_class = NULL;


GType
gimp_menu_item_get_type (void)
{
  static GType menu_item_type = 0;

  if (!menu_item_type)
    {
      static const GTypeInfo menu_item_info =
      {
        sizeof (GimpMenuItemClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_menu_item_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpMenuItem),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_menu_item_init,
      };

      menu_item_type = g_type_register_static (GTK_TYPE_MENU_ITEM,
                                               "GimpMenuItem",
                                               &menu_item_info, 0);
    }

  return menu_item_type;
}

static void
gimp_menu_item_class_init (GimpMenuItemClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  klass->set_viewable = gimp_menu_item_real_set_viewable;
}

static void
gimp_menu_item_init (GimpMenuItem *menu_item)
{
  menu_item->hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (menu_item), menu_item->hbox);
  gtk_widget_show (menu_item->hbox);

  menu_item->preview       = NULL;
  menu_item->name_label    = NULL;

  menu_item->preview_size  = 0;
  menu_item->get_name_func = NULL;
}

GtkWidget *
gimp_menu_item_new (GimpViewable  *viewable,
                    gint           preview_size)
{
  GimpMenuItem *menu_item;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (preview_size > 0 && preview_size <= 256, NULL);

  menu_item = g_object_new (GIMP_TYPE_MENU_ITEM, NULL);

  menu_item->preview_size = preview_size;

  gimp_menu_item_set_viewable (menu_item, viewable);

  return GTK_WIDGET (menu_item);
}

static void
gimp_menu_item_set_viewable (GimpMenuItem *menu_item,
                             GimpViewable *viewable)
{
  GIMP_MENU_ITEM_GET_CLASS (menu_item)->set_viewable (menu_item, viewable);
}

static void
gimp_menu_item_real_set_viewable (GimpMenuItem *menu_item,
                                  GimpViewable *viewable)
{
  menu_item->preview = gimp_preview_new (viewable, menu_item->preview_size,
                                         1, FALSE);
  gtk_widget_set_usize (menu_item->preview, menu_item->preview_size, -1);
  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->preview,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->preview);

  menu_item->name_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->name_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->name_label);

  gimp_menu_item_name_changed (viewable, menu_item);

  g_signal_connect_object (G_OBJECT (viewable), "name_changed",
                           G_CALLBACK (gimp_menu_item_name_changed),
                           menu_item, 0);

  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (menu_item),
				    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
				    G_TYPE_FROM_INSTANCE (viewable),
				    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gimp_dnd_viewable_source_set (GTK_WIDGET (menu_item),
                                G_TYPE_FROM_INSTANCE (viewable),
				gimp_menu_item_drag_viewable,
				NULL);
}

void
gimp_menu_item_set_name_func (GimpMenuItem        *menu_item,
			      GimpItemGetNameFunc  get_name_func)
{
  g_return_if_fail (GIMP_IS_MENU_ITEM (menu_item));

  if (menu_item->get_name_func != get_name_func)
    {
      GimpViewable *viewable;

      menu_item->get_name_func = get_name_func;

      viewable = GIMP_PREVIEW (menu_item->preview)->viewable;

      if (viewable)
	gimp_menu_item_name_changed (viewable, menu_item);
    }
}

static void
gimp_menu_item_name_changed (GimpViewable *viewable,
                             GimpMenuItem *menu_item)
{
  if (menu_item->get_name_func)
    {
      gchar *name;

      name = menu_item->get_name_func (GTK_WIDGET (menu_item));

      gtk_label_set_text (GTK_LABEL (menu_item->name_label), name);

      g_free (name);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (menu_item->name_label),
			  gimp_object_get_name (GIMP_OBJECT (viewable)));
    }
}

static GimpViewable *
gimp_menu_item_drag_viewable (GtkWidget *widget,
                              gpointer   data)
{
  return GIMP_PREVIEW (GIMP_MENU_ITEM (widget)->preview)->viewable;
}
