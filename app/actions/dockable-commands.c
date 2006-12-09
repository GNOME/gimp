/* GIMP - The GNU Image Manipulation Program
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
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpsessioninfo.h"

#include "dialogs/dialogs.h"

#include "dockable-commands.h"


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
              gint               view_size = -1;

              if (view_type == GIMP_VIEW_TYPE_LIST)
                memcpy (substring, "list", 4);
              else if (view_type == GIMP_VIEW_TYPE_GRID)
                memcpy (substring, "grid", 4);

              old_view = gimp_container_view_get_by_dockable (dockable);

              if (old_view)
                view_size = gimp_container_view_get_view_size (old_view, NULL);


              new_dockable =
                gimp_dialog_factory_dockable_new (dockbook->dock->dialog_factory,
                                                  dockbook->dock,
                                                  identifier,
                                                  view_size);

              if (new_dockable)
                {
                  GimpDocked *old = GIMP_DOCKED (GTK_BIN (dockable)->child);
                  GimpDocked *new = GIMP_DOCKED (GTK_BIN (new_dockable)->child);
                  gboolean    show;

                  show = gimp_docked_get_show_button_bar (old);
                  gimp_docked_set_show_button_bar (new, show);
                }

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
dockable_view_size_cmd_callback (GtkAction *action,
                                 GtkAction *current,
                                 gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          view_size;
  gint          page_num;

  view_size = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));

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

          old_size = gimp_container_view_get_view_size (view, &border_width);

          if (old_size != view_size)
            gimp_container_view_set_view_size (view, view_size, border_width);
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
dockable_show_button_bar_cmd_callback (GtkAction *action,
                                       gpointer   data)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      gboolean show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

      gimp_docked_set_show_button_bar (GIMP_DOCKED (GTK_BIN (dockable)->child),
                                       show);
    }
}
