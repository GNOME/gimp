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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpmarshal.h"
#include "core/gimpviewable.h"

#include "gimpdnd.h"
#include "gimpmenuitem.h"
#include "gimppreview.h"
#include "gimppreviewrenderer.h"


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

  menu_item->preview              = NULL;
  menu_item->name_label           = NULL;

  menu_item->preview_size         = 0;
  menu_item->preview_border_width = 1;
}

GtkWidget *
gimp_menu_item_new (GimpViewable  *viewable,
                    gint           preview_size,
                    gint           preview_border_width)
{
  GimpMenuItem *menu_item;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (preview_size > 0 &&
                        preview_size <= GIMP_VIEWABLE_MAX_MENU_SIZE, NULL);
  g_return_val_if_fail (preview_border_width >= 0 &&
                        preview_border_width <= GIMP_PREVIEW_MAX_BORDER_WIDTH,
                        NULL);

  menu_item = g_object_new (GIMP_TYPE_MENU_ITEM, NULL);

  menu_item->preview_size         = preview_size;
  menu_item->preview_border_width = preview_border_width;

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
  menu_item->preview = gimp_preview_new (viewable,
                                         menu_item->preview_size,
                                         menu_item->preview_border_width,
                                         FALSE);
  gtk_widget_set_size_request (menu_item->preview,
                               menu_item->preview_size +
                               2 * menu_item->preview_border_width,
                               menu_item->preview_size +
                               2 * menu_item->preview_border_width);

  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->preview,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->preview);

  menu_item->name_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (menu_item->hbox), menu_item->name_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (menu_item->name_label);

  gimp_menu_item_name_changed (viewable, menu_item);

  g_signal_connect_object (viewable,
                           GIMP_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                           G_CALLBACK (gimp_menu_item_name_changed),
                           menu_item, 0);

  if (gimp_dnd_drag_source_set_by_type (GTK_WIDGET (menu_item),
                                        GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                                        G_TYPE_FROM_INSTANCE (viewable),
                                        GDK_ACTION_MOVE | GDK_ACTION_COPY))
    {
      gimp_dnd_viewable_source_add (GTK_WIDGET (menu_item),
                                    G_TYPE_FROM_INSTANCE (viewable),
                                    gimp_menu_item_drag_viewable,
                                    NULL);
    }
}

static void
gimp_menu_item_name_changed (GimpViewable *viewable,
                             GimpMenuItem *menu_item)
{
  gchar *name    = NULL;
  gchar *tooltip = NULL;

  name = gimp_viewable_get_description (viewable, &tooltip);

  gtk_label_set_text (GTK_LABEL (menu_item->name_label), name);
  gimp_help_set_help_data (GTK_WIDGET (menu_item), tooltip, NULL);

  g_free (name);
  g_free (tooltip);
}

static GimpViewable *
gimp_menu_item_drag_viewable (GtkWidget *widget,
                              gpointer   data)
{
  return GIMP_PREVIEW (GIMP_MENU_ITEM (widget)->preview)->viewable;
}
