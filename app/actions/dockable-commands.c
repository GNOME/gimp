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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpsessioninfo.h"

#include "dialogs/dialogs.h"

#include "dockable-commands.h"


/*  local function prototypes  */

static void   dockable_change_screen_confirm_callback (GtkWidget *dialog,
                                                       gint       value,
                                                       gpointer   data);
static void   dockable_change_screen_destroy_callback (GtkWidget *dialog,
                                                       GtkWidget *dock);


/*  public functions  */

void
dockable_add_tab_cmd_callback (GtkAction   *action,
                               const gchar *value,
                               gpointer     data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  if (value)
    {
      GtkWidget *dockable;
      gchar     *identifier;
      gchar     *p;

      identifier = g_strdup (value);

      p = strchr (identifier, '|');

      if (p)
        *p = '\0';

      dockable =
        gimp_dialog_factory_dockable_new (dockbook->dock->dialog_factory,
                                          dockbook->dock,
                                          identifier, -1);

      g_free (identifier);

      /*  Maybe gimp_dialog_factory_dockable_new() returned an already
       *  existing singleton dockable, so check if it already is
       *  attached to a dockbook.
       */
      if (dockable && ! GIMP_DOCKABLE (dockable)->dockbook)
        gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), -1);
    }
}

void
dockable_close_tab_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    gimp_dockbook_remove (dockbook, dockable);
}

void
dockable_detach_tab_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    gimp_dockable_detach (dockable);
}

void
dockable_toggle_view_cmd_callback (GtkAction *action,
                                   GtkAction *current,
                                   gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  GimpViewType  view_type;
  gint          page_num;

  view_type = (GimpViewType)
    gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      GimpDialogFactoryEntry *entry;

      gimp_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

      if (entry)
        {
          gchar *identifier;
          gchar *substring = NULL;

          identifier = g_strdup (entry->identifier);

          substring = strstr (identifier, "grid");

          if (substring && view_type == GIMP_VIEW_TYPE_GRID)
            return;

          if (! substring)
            {
              substring = strstr (identifier, "list");

              if (substring && view_type == GIMP_VIEW_TYPE_LIST)
                return;
            }

          if (substring)
            {
              GimpContainerView *old_view;
              GtkWidget         *new_dockable;
              gint               preview_size = -1;

              if (view_type == GIMP_VIEW_TYPE_LIST)
                memcpy (substring, "list", 4);
              else if (view_type == GIMP_VIEW_TYPE_GRID)
                memcpy (substring, "grid", 4);

              old_view = gimp_container_view_get_by_dockable (dockable);

              if (old_view)
                preview_size = gimp_container_view_get_preview_size (old_view,
                                                                     NULL);

              new_dockable =
                gimp_dialog_factory_dockable_new (dockbook->dock->dialog_factory,
                                                  dockbook->dock,
                                                  identifier,
                                                  preview_size);

              /*  Maybe gimp_dialog_factory_dockable_new() returned
               *  an already existing singleton dockable, so check
               *  if it already is attached to a dockbook.
               */
              if (new_dockable && ! GIMP_DOCKABLE (new_dockable)->dockbook)
                {
                  gimp_dockbook_add (dockbook, GIMP_DOCKABLE (new_dockable),
                                     page_num);

                  gimp_dockbook_remove (dockbook, dockable);

                  gtk_notebook_set_current_page (GTK_NOTEBOOK (dockbook),
                                                 page_num);
                }
            }

          g_free (identifier);
        }
    }
}

void
dockable_preview_size_cmd_callback (GtkAction *action,
                                    GtkAction *current,
                                    gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          preview_size;
  gint          page_num;

  preview_size = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      GimpContainerView *view = gimp_container_view_get_by_dockable (dockable);

      if (view)
        {
          gint old_size;
          gint border_width;

          old_size = gimp_container_view_get_preview_size (view,
                                                           &border_width);

          if (old_size != preview_size)
            gimp_container_view_set_preview_size (view, preview_size,
                                                  border_width);
        }
    }
}

void
dockable_tab_style_cmd_callback (GtkAction *action,
                                 GtkAction *current,
                                 gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  GimpTabStyle  tab_style;
  gint          page_num;

  tab_style = (GimpTabStyle)
    gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable && dockable->tab_style != tab_style)
    {
      GtkWidget *tab_widget;

      gimp_dockable_set_tab_style (dockable, tab_style);

      tab_widget = gimp_dockbook_get_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
}

void
dockable_toggle_image_menu_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  gboolean      active;

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_IMAGE_DOCK (dockbook->dock))
    gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (dockbook->dock),
                                         active);
}

void
dockable_toggle_auto_cmd_callback (GtkAction *action,
                                   gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  gboolean      active;

  active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

  if (GIMP_IS_IMAGE_DOCK (dockbook->dock))
    gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (dockbook->dock),
                                            active);
}

void
dockable_change_screen_cmd_callback (GtkAction *action,
                                     gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GtkWidget    *dock;
  GdkScreen    *screen;
  GdkDisplay   *display;
  gint          cur_screen;
  gint          num_screens;
  GtkWidget    *dialog;

  dock = GTK_WIDGET (dockbook->dock);

  dialog = g_object_get_data (G_OBJECT (dock), "gimp-change-screen-dialog");

  if (dialog)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }

  screen  = gtk_widget_get_screen (dock);
  display = gtk_widget_get_display (dock);

  cur_screen  = gdk_screen_get_number (screen);
  num_screens = gdk_display_get_n_screens (display);

  dialog = gimp_query_int_box ("Move Dock to Screen",
                               dock,
                               NULL, NULL,
                               "Enter destination screen",
                               cur_screen, 0, num_screens - 1,
                               G_OBJECT (dock), "destroy",
                               dockable_change_screen_confirm_callback,
                               dock);

  g_object_set_data (G_OBJECT (dock), "gimp-change-screen-dialog", dialog);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (dockable_change_screen_destroy_callback),
                    dock);

  gtk_widget_show (dialog);
}


/*  private functions  */

static void
dockable_change_screen_confirm_callback (GtkWidget *dialog,
                                         gint       value,
                                         gpointer   data)
{
  GdkScreen *screen;

  screen = gdk_display_get_screen (gtk_widget_get_display (GTK_WIDGET (data)),
                                   value);

  if (screen)
    gtk_window_set_screen (GTK_WINDOW (data), screen);
}

static void
dockable_change_screen_destroy_callback (GtkWidget *dialog,
                                         GtkWidget *dock)
{
  g_object_set_data (G_OBJECT (dock), "gimp-change-screen-dialog", NULL);
}
