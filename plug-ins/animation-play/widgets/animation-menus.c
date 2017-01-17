/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-menus.c
 * Copyright (C) 2016 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "core/animation-celanimation.h"
#include "core/animation-playback.h"

#include "animation-xsheet.h"

#include "animation-menus.h"

typedef struct
{
  AnimationCelAnimation *animation;
  gint                   level;
  gint                   position;
  gboolean               dup_previous;
}
CellData;

static void on_add_cell    (GtkMenuItem *menuitem,
                            gpointer     user_data);
static void on_delete_cell (GtkMenuItem *menuitem,
                            gpointer     user_data);


static void
on_add_cell (GtkMenuItem *menuitem,
             gpointer     user_data)
{
  CellData *data = user_data;

  animation_cel_animation_cel_add (data->animation,
                                   data->level,
                                   data->position,
                                   data->dup_previous);
}

static void
on_delete_cell (GtkMenuItem *menuitem,
                gpointer     user_data)
{
  CellData *data = user_data;

  animation_cel_animation_cel_delete (data->animation,
                                      data->level,
                                      data->position);
}

void
animation_menu_cell (AnimationCelAnimation *animation,
                     GdkEventButton        *event,
                     gint                   frame,
                     gint                   track)
{
  GtkWidget *menu;
  GtkWidget *item;
  CellData  *data;

  menu = gtk_menu_new ();

  /* Duplicate cell. */
  item = gtk_menu_item_new_with_label (_("Duplicate cel"));
  data = g_new0 (CellData, 1);
  data->animation    = animation;
  data->position     = frame + 1;
  data->level        = track;
  data->dup_previous = TRUE;
  g_signal_connect_data (item, "activate",
                         G_CALLBACK (on_add_cell), data,
                         (GClosureNotify) g_free, 0);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* Add empty cell. */
  item = gtk_menu_item_new_with_label (_("Push cels down"));
  data = g_new0 (CellData, 1);
  data->animation    = animation;
  data->position     = frame;
  data->level        = track;
  data->dup_previous = FALSE;
  g_signal_connect_data (item, "activate",
                         G_CALLBACK (on_add_cell), data,
                         (GClosureNotify) g_free, 0);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* Add empty cell. */
  item = gtk_menu_item_new_with_label (_("Add empty cel after"));
  data = g_new0 (CellData, 1);
  data->animation    = animation;
  data->position     = frame + 1;
  data->level        = track;
  data->dup_previous = FALSE;
  g_signal_connect_data (item, "activate",
                         G_CALLBACK (on_add_cell), data,
                         (GClosureNotify) g_free, 0);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* Delete cell. */
  item = gtk_menu_item_new_with_label (_("Delete cell"));
  data = g_new0 (CellData, 1);
  data->animation = animation;
  data->position  = frame;
  data->level     = track;
  g_signal_connect_data (item, "activate",
                         G_CALLBACK (on_delete_cell), data,
                         (GClosureNotify) g_free, 0);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  gtk_widget_show (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  event->button, event->time);
}
