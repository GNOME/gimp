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

#include "gui-types.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpimagedock.h"
#include "widgets/gimpsessioninfo.h"

#include "dialogs.h"
#include "dialogs-commands.h"


/*  local function prototypes  */

static void   dialogs_create_dock (GdkScreen   *screen,
                                   const gchar *tabs[],
                                   gint         n_tabs);


/*  public functions  */

void
dialogs_show_toolbox_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  dialogs_show_toolbox ();
}

void
dialogs_create_toplevel_cmd_callback (GtkWidget *widget,
				      gpointer   data,
				      guint      action)
{
  if (action)
    {
      const gchar *identifier = g_quark_to_string ((GQuark) action);

      if (identifier)
	gimp_dialog_factory_dialog_new (global_dialog_factory,
                                        gtk_widget_get_screen (widget),
                                        identifier, -1);
    }
}

void
dialogs_create_dockable_cmd_callback (GtkWidget *widget,
				      gpointer   data,
				      guint      action)
{
  if (action)
    {
      const gchar *identifier = g_quark_to_string ((GQuark) action);

      if (!identifier)
        return;

      gimp_dialog_factory_dialog_raise (global_dock_factory,
                                        gtk_widget_get_screen (widget),
                                        identifier, -1);
    }
}

void
dialogs_add_tab_cmd_callback (GtkWidget *widget,
			      gpointer   data,
			      guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  if (action)
    {
      GtkWidget   *dockable;
      const gchar *identifier;

      identifier = g_quark_to_string ((GQuark) action);

      if (identifier)
	{
	  dockable =
            gimp_dialog_factory_dockable_new (dockbook->dock->dialog_factory,
                                              dockbook->dock,
                                              identifier,
                                              -1);

	  /*  Maybe gimp_dialog_factory_dockable_new() returned an already
	   *  existing singleton dockable, so check if it already is
	   *  attached to a dockbook.
	   */
	  if (dockable && ! GIMP_DOCKABLE (dockable)->dockbook)
	    gimp_dockbook_add (dockbook, GIMP_DOCKABLE (dockable), -1);
	}
    }
}

void
dialogs_close_tab_cmd_callback (GtkWidget *widget,
                                gpointer   data,
                                guint      action)
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
dialogs_detach_tab_cmd_callback (GtkWidget *widget,
                                 gpointer   data,
                                 guint      action)
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
dialogs_toggle_view_cmd_callback (GtkWidget *widget,
                                  gpointer   data,
                                  guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  GimpViewType  view_type;
  gint          page_num;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  view_type = (GimpViewType) action;

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
                preview_size = old_view->preview_size;

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
dialogs_preview_size_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  gint          preview_size;
  gint          page_num;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  preview_size = (gint) action;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable)
    {
      GimpContainerView *view;

      view = gimp_container_view_get_by_dockable (dockable);

      if (view && view->preview_size != preview_size)
        gimp_container_view_set_preview_size (view, preview_size,
                                              view->preview_border_width);
    }
}

void
dialogs_tab_style_cmd_callback (GtkWidget *widget,
                                gpointer   data,
                                guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GimpDockable *dockable;
  GimpTabStyle  tab_style;
  gint          page_num;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  tab_style = (gint) action;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = (GimpDockable *)
    gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (dockable && dockable->tab_style != tab_style)
    {
      GtkWidget *tab_widget;

      dockable->tab_style = tab_style;

      tab_widget = gimp_dockbook_get_tab_widget (dockbook, dockable);

      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
}

void
dialogs_toggle_image_menu_cmd_callback (GtkWidget *widget,
					gpointer   data,
					guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  if (GIMP_IS_IMAGE_DOCK (dockbook->dock))
    gimp_image_dock_set_show_image_menu (GIMP_IMAGE_DOCK (dockbook->dock),
                                         GTK_CHECK_MENU_ITEM (widget)->active);
}

void
dialogs_toggle_auto_cmd_callback (GtkWidget *widget,
				  gpointer   data,
				  guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);

  if (GIMP_IS_IMAGE_DOCK (dockbook->dock))
    gimp_image_dock_set_auto_follow_active (GIMP_IMAGE_DOCK (dockbook->dock),
                                            GTK_CHECK_MENU_ITEM (widget)->active);
}

static void
dialogs_change_screen_confirm_callback (GtkWidget *query_box,
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
dialogs_change_screen_destroy_callback (GtkWidget *query_box,
                                        GtkWidget *dock)
{
  g_object_set_data (G_OBJECT (dock), "gimp-change-screen-dialog", NULL);
}

void
dialogs_change_screen_cmd_callback (GtkWidget *widget,
                                    gpointer   data,
                                    guint      action)
{
  GimpDockbook *dockbook = GIMP_DOCKBOOK (data);
  GtkWidget    *dock;
  GdkScreen    *screen;
  GdkDisplay   *display;
  gint          cur_screen;
  gint          num_screens;
  GtkWidget    *qbox;

  dock = GTK_WIDGET (dockbook->dock);

  qbox = g_object_get_data (G_OBJECT (dock), "gimp-change-screen-dialog");

  if (qbox)
    {
      gtk_window_present (GTK_WINDOW (qbox));
      return;
    }

  screen  = gtk_widget_get_screen (dock);
  display = gtk_widget_get_display (dock);

  cur_screen  = gdk_screen_get_number (screen);
  num_screens = gdk_display_get_n_screens (display);

  qbox = gimp_query_int_box ("Move Dock to Screen",
                             dock,
                             NULL, 0,
                             "Enter destination screen",
                             cur_screen, 0, num_screens - 1,
                             G_OBJECT (dock), "destroy",
                             dialogs_change_screen_confirm_callback,
                             dock);

  g_object_set_data (G_OBJECT (dock), "gimp-change-screen-dialog", qbox);

  g_signal_connect (qbox, "destroy",
                    G_CALLBACK (dialogs_change_screen_destroy_callback),
                    dock);

  gtk_widget_show (qbox);
}

void
dialogs_create_lc_cmd_callback (GtkWidget *widget,
                                gpointer   data,
                                guint      action)
{
  static const gchar *tabs[] =
  {
    "gimp-layer-list",
    "gimp-channel-list",
    "gimp-vectors-list",
    "gimp-indexed-palette"
  };

  dialogs_create_dock (gtk_widget_get_screen (widget),
                       tabs, G_N_ELEMENTS (tabs));
}

void
dialogs_create_data_cmd_callback (GtkWidget *widget,
                                  gpointer   data,
                                  guint      action)
{
  static const gchar *tabs[] =
  {
    "gimp-brush-grid",
    "gimp-pattern-grid",
    "gimp-gradient-list",
    "gimp-palette-list",
    "gimp-font-list"
  };

  dialogs_create_dock (gtk_widget_get_screen (widget),
                       tabs, G_N_ELEMENTS (tabs));
}

void
dialogs_create_stuff_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  static const gchar *tabs[] =
  {
    "gimp-buffer-list",
    "gimp-image-list",
    "gimp-document-list",
    "gimp-template-list"
  };

  dialogs_create_dock (gtk_widget_get_screen (widget),
                       tabs, G_N_ELEMENTS (tabs));
}

void
dialogs_show_toolbox (void)
{
  if (! global_toolbox_factory->open_dialogs)
    {
      GtkWidget *toolbox;

      toolbox = gimp_dialog_factory_dock_new (global_toolbox_factory,
                                              gdk_screen_get_default ());

      gtk_widget_show (toolbox);
    }
  else
    {
      GList *list;

      for (list = global_toolbox_factory->open_dialogs;
           list;
           list = g_list_next (list))
        {
          if (GTK_WIDGET_TOPLEVEL (list->data))
            {
              gtk_window_present (GTK_WINDOW (list->data));
              break;
            }
        }
    }
}


/*  private functions  */

static void
dialogs_create_dock (GdkScreen   *screen,
                     const gchar *tabs[],
                     gint         n_tabs)
{
  GtkWidget *dock;
  GtkWidget *dockbook;
  GtkWidget *dockable;
  gint       i;

  dock = gimp_dialog_factory_dock_new (global_dock_factory, screen);

  dockbook = gimp_dockbook_new (global_dock_factory->menu_factory);

  gimp_dock_add_book (GIMP_DOCK (dock), GIMP_DOCKBOOK (dockbook), 0);

  for (i = 0; i < n_tabs; i++)
    {
      dockable = gimp_dialog_factory_dialog_new (global_dock_factory,
                                                 screen,
                                                 tabs[i], -1);

      if (dockable && ! GIMP_DOCKABLE (dockable)->dockbook)
        gimp_dock_add (GIMP_DOCK (dock), GIMP_DOCKABLE (dockable), -1, -1);
    }

  gtk_widget_show (dock);
}
