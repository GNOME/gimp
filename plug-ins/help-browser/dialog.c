/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP Help Browser
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * dialog.c
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include <webkit/webkit.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "plug-ins/help/gimphelp.h"

#include "gimpthrobber.h"
#include "gimpthrobberaction.h"

#include "dialog.h"
#include "uri.h"

#include "logo-pixbuf.h"

#include "libgimp/stdplugins-intl.h"

#ifndef _O_BINARY
#define _O_BINARY 0
#endif


#define GIMP_HELP_BROWSER_DIALOG_DATA  "gimp-help-browser-dialog"

typedef struct
{
  gint     width;
  gint     height;
  gint     paned_position;
  gdouble  zoom;
} DialogData;

enum
{
  HISTORY_TITLE,
  HISTORY_URI
};

/*  local function prototypes  */

static GtkUIManager * ui_manager_new     (GtkWidget         *window);

static void       back_callback          (GtkAction         *action,
                                          gpointer           data);
static void       forward_callback       (GtkAction         *action,
                                          gpointer           data);
static void       reload_callback        (GtkAction         *action,
                                          gpointer           data);
static void       stop_callback          (GtkAction         *action,
                                          gpointer           data);
static void       home_callback          (GtkAction         *action,
                                          gpointer           data);
static void       zoom_in_callback       (GtkAction         *action,
                                          gpointer           data);
static void       zoom_out_callback      (GtkAction         *action,
                                          gpointer           data);
static void       close_callback         (GtkAction         *action,
                                          gpointer           data);
static void       website_callback       (GtkAction         *action,
                                          gpointer           data);
static void       copy_location_callback (GtkAction         *action,
                                          gpointer           data);

static void       update_actions         (void);

static void       row_activated          (GtkTreeView       *tree_view,
                                          GtkTreePath       *path,
                                          GtkTreeViewColumn *column);
static void       dialog_unmap           (GtkWidget         *window,
                                          GtkWidget         *paned);

static void       view_realize           (GtkWidget         *widget);
static void       view_unrealize         (GtkWidget         *widget);
static gboolean   view_popup_menu        (GtkWidget         *widget,
                                          GdkEventButton    *event);
static gboolean   view_button_press      (GtkWidget         *widget,
                                          GdkEventButton    *event);

static void       title_changed          (GtkWidget         *view,
                                          WebKitWebFrame    *frame,
                                          const gchar       *title,
                                          GtkWidget         *window);
static void       load_started           (GtkWidget         *view,
                                          WebKitWebFrame    *frame);
static void       load_finished          (GtkWidget         *view,
                                          WebKitWebFrame    *frame);

static void       select_index           (const gchar       *uri);



/*  private variables  */

static GHashTable   *uri_hash_table = NULL;

static GtkWidget    *view           = NULL;
static GtkWidget    *tree_view      = NULL;
static GtkUIManager *ui_manager     = NULL;
static GtkWidget    *button_prev    = NULL;
static GtkWidget    *button_next    = NULL;
static GdkCursor    *busy_cursor    = NULL;


/*  public functions  */

void
browser_dialog_open (void)
{
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *toolbar;
  GtkWidget       *paned;
  GtkWidget       *scrolled;
  GtkWidget       *button;
  GtkToolItem     *item;
  GtkAction       *action;
  GdkPixbuf       *pixbuf;
  DialogData       data = { 720, 560, 240, 1.0 };

  gimp_ui_init ("helpbrowser", TRUE);

  gimp_get_data (GIMP_HELP_BROWSER_DIALOG_DATA, &data);

  /*  the dialog window  */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _("GIMP Help Browser"));
  gtk_window_set_role (GTK_WINDOW (window), "help-browser");

  gtk_window_set_default_size (GTK_WINDOW (window), data.width, data.height);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  pixbuf = gdk_pixbuf_new_from_inline (-1, logo_data, FALSE, NULL);
  gtk_window_set_icon (GTK_WINDOW (window), pixbuf);
  g_object_unref (pixbuf);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  ui_manager = ui_manager_new (window);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/help-browser-toolbar");
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  item = g_object_new (GTK_TYPE_MENU_TOOL_BUTTON, NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  gtk_widget_show (GTK_WIDGET (item));

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/forward");
  gtk_action_connect_proxy (action, GTK_WIDGET (item));
  g_object_notify (G_OBJECT (action), "tooltip");
  button_next = GTK_WIDGET (item);

  item = g_object_new (GTK_TYPE_MENU_TOOL_BUTTON, NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  gtk_widget_show (GTK_WIDGET (item));

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/back");
  gtk_action_connect_proxy (action, GTK_WIDGET (item));
  g_object_notify (G_OBJECT (action), "tooltip");
  button_prev = GTK_WIDGET (item);

  item =
    GTK_TOOL_ITEM (gtk_ui_manager_get_widget (ui_manager,
                                              "/help-browser-toolbar/space"));
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, TRUE);

  button = gtk_ui_manager_get_widget (ui_manager,
                                      "/help-browser-toolbar/website");
  gimp_throbber_set_image (GIMP_THROBBER (button),
                           gtk_image_new_from_pixbuf (pixbuf));

  /*  the horizontal paned  */
  paned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);
  gtk_widget_show (paned);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_add1 (GTK_PANED (paned), scrolled);
  gtk_paned_set_position (GTK_PANED (paned), data.paned_position);
  gtk_widget_show (scrolled);

  tree_view = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled), tree_view);
  gtk_widget_show (tree_view);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view), -1,
                                               NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  g_signal_connect (tree_view, "row-activated",
                    G_CALLBACK (row_activated),
                    NULL);

  /*  HTML view  */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_set_size_request (scrolled, 300, 200);
  gtk_paned_add2 (GTK_PANED (paned), scrolled);
  gtk_widget_show (scrolled);

  view = webkit_web_view_new ();
  webkit_web_view_set_maintains_back_forward_list (WEBKIT_WEB_VIEW (view),
                                                   TRUE);
  gtk_container_add (GTK_CONTAINER (scrolled), view);
  gtk_widget_show (view);

  g_signal_connect (view, "realize",
                    G_CALLBACK (view_realize),
                    NULL);
  g_signal_connect (view, "unrealize",
                    G_CALLBACK (view_unrealize),
                    NULL);

  g_signal_connect (view, "popup-menu",
                    G_CALLBACK (view_popup_menu),
                    NULL);
  g_signal_connect (view, "button-press-event",
                    G_CALLBACK (view_button_press),
                    NULL);

#if HAVE_WEBKIT_ZOOM_API
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (view), data.zoom);
#endif

  g_signal_connect (view, "title-changed",
                    G_CALLBACK (title_changed),
                    window);

  g_signal_connect (view, "load-started",
                    G_CALLBACK (load_started),
                    NULL);
  g_signal_connect (view, "load-finished",
                    G_CALLBACK (load_finished),
                    NULL);

  gtk_widget_grab_focus (view);

  g_signal_connect (window, "unmap",
                    G_CALLBACK (dialog_unmap),
                    paned);

  update_actions ();
}

void
browser_dialog_load (const gchar *uri)
{
  g_return_if_fail (uri != NULL);

  webkit_web_view_open (WEBKIT_WEB_VIEW (view), uri);

  select_index (uri);

  gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (view)));
}

static void
browser_dialog_make_index_foreach (const gchar    *help_id,
                                   GimpHelpItem   *item,
                                   GimpHelpLocale *locale)
{

#if 0
  g_printerr ("%s: processing %s (parent %s)\n",
              G_STRFUNC,
              item->title  ? item->title  : "NULL",
              item->parent ? item->parent : "NULL");
#endif

  if (item->title)
    {
      gchar **indices = g_strsplit (item->title, ".", -1);
      gint    i;

      for (i = 0; i < 5; i++)
        {
          if (! indices[i])
            break;

          item->index += atoi (indices[i]) << (8 * (5 - i));
        }

      g_strfreev (indices);
    }

  if (item->parent && strlen (item->parent))
    {
      GimpHelpItem *parent;

      parent = g_hash_table_lookup (locale->help_id_mapping, item->parent);

      if (parent)
        parent->children = g_list_prepend (parent->children, item);
    }
  else
    {
      locale->toplevel_items = g_list_prepend (locale->toplevel_items, item);
    }
}

static gint
help_item_compare (gconstpointer a,
                   gconstpointer b)
{
  GimpHelpItem *item_a = (GimpHelpItem *) a;
  GimpHelpItem *item_b = (GimpHelpItem *) b;

  if (item_a->index > item_b->index)
    return 1;
  else if (item_a->index < item_b->index)
    return -1;

  return 0;
}

static void
add_child (GtkTreeStore   *store,
           GimpHelpDomain *domain,
           GimpHelpLocale *locale,
           GtkTreeIter    *parent,
           GimpHelpItem   *item)
{
  GtkTreeIter  iter;
  GList       *list;
  gchar       *uri;

  gtk_tree_store_append (store, &iter, parent);

  gtk_tree_store_set (store, &iter,
                      0, item,
                      1, item->title,
                      -1);

  uri = g_strconcat (domain->help_uri,  "/",
                     locale->locale_id, "/",
                     item->ref,
                     NULL);

  g_hash_table_insert (uri_hash_table,
                       uri,
                       gtk_tree_iter_copy (&iter));

  item->children = g_list_sort (item->children, help_item_compare);

  for (list = item->children; list; list = g_list_next (list))
    {
      GimpHelpItem *item = list->data;

      add_child (store, domain, locale, &iter, item);
    }
}

void
browser_dialog_make_index (GimpHelpDomain *domain,
                           GimpHelpLocale *locale)
{
  GtkTreeStore *store;
  GList        *list;

  if (! locale->toplevel_items)
    {
      g_hash_table_foreach (locale->help_id_mapping,
                            (GHFunc) browser_dialog_make_index_foreach,
                            locale);

      locale->toplevel_items = g_list_sort (locale->toplevel_items,
                                            help_item_compare);
    }

  store = gtk_tree_store_new (2,
                              G_TYPE_POINTER,
                              G_TYPE_STRING);

  g_object_set_data (G_OBJECT (store), "domain", domain);
  g_object_set_data (G_OBJECT (store), "locale", locale);

  if (uri_hash_table)
    g_hash_table_unref (uri_hash_table);

  uri_hash_table = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          (GDestroyNotify) g_free,
                                          (GDestroyNotify) gtk_tree_iter_free);

  for (list = locale->toplevel_items; list; list = g_list_next (list))
    {
      GimpHelpItem *item = list->data;

      add_child (store, domain, locale, NULL, item);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
select_index (const gchar *uri)
{
  GtkTreeSelection *selection;
  GtkTreeIter      *iter;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  iter = g_hash_table_lookup (uri_hash_table, uri);

  if (iter)
    {
      GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
      GtkTreePath  *path;
      GtkTreePath  *scroll_path;

      path = gtk_tree_model_get_path (model, iter);
      scroll_path = gtk_tree_path_copy (path);

      gtk_tree_path_up (path);
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree_view), path);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree_view), scroll_path,
                                    NULL, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);
      gtk_tree_path_free (scroll_path);

      gtk_tree_selection_select_iter (selection, iter);
    }
  else
    {
      gtk_tree_selection_unselect_all (selection);
    }
}


/*  private functions  */

static GtkUIManager *
ui_manager_new (GtkWidget *window)
{
  static const GtkActionEntry actions[] =
  {
    {
      "back", GTK_STOCK_GO_BACK,
      NULL, "<alt>Left", N_("Go back one page"),
      G_CALLBACK (back_callback)
    },
    {
      "forward", GTK_STOCK_GO_FORWARD,
      NULL, "<alt>Right", N_("Go forward one page"),
      G_CALLBACK (forward_callback)
    },
    {
      "reload", GTK_STOCK_REFRESH,
       N_("Reload"), "<control>R", N_("Reload current page"),
      G_CALLBACK (reload_callback)
    },
    {
      "stop", GTK_STOCK_CANCEL,
       N_("Stop"), "Escape", N_("Stop loading this page"),
      G_CALLBACK (stop_callback)
    },
    {
      "home", GTK_STOCK_HOME,
      NULL, "<alt>Home", N_("Go to the index page"),
      G_CALLBACK (home_callback)
    },
    {
      "copy-location", GTK_STOCK_COPY,
      N_("Copy location"), "",
      N_("Copy the location of this page to the clipboard"),
      G_CALLBACK (copy_location_callback)
    },
    {
      "zoom-in", GTK_STOCK_ZOOM_IN,
      NULL, "<control>plus", NULL,
      G_CALLBACK (zoom_in_callback)
    },
    {
      "zoom-out", GTK_STOCK_ZOOM_OUT,
      NULL, "<control>minus", NULL,
      G_CALLBACK (zoom_out_callback)
    },
    {
      "close", GTK_STOCK_CLOSE,
      NULL, "<control>W", NULL,
      G_CALLBACK (close_callback)
    },
    {
      "quit", GTK_STOCK_QUIT,
      NULL, "<control>Q", NULL,
      G_CALLBACK (close_callback)
    }
  };

  GtkUIManager   *ui_manager = gtk_ui_manager_new ();
  GtkActionGroup *group      = gtk_action_group_new ("Actions");
  GtkAction      *action;
  GError         *error      = NULL;

  gtk_action_group_set_translation_domain (group, NULL);
  gtk_action_group_add_actions (group, actions, G_N_ELEMENTS (actions), NULL);

  action = gimp_throbber_action_new ("website",
                                     "docs.gimp.org",
                                     _("Visit the GIMP documentation website"),
                                     GIMP_STOCK_WILBER);
  g_signal_connect_closure (action, "activate",
                            g_cclosure_new (G_CALLBACK (website_callback),
                                            NULL, NULL),
                            FALSE);
  gtk_action_group_add_action (group, action);
  g_object_unref (action);

  gtk_window_add_accel_group (GTK_WINDOW (window),
                              gtk_ui_manager_get_accel_group (ui_manager));
  gtk_accel_group_lock (gtk_ui_manager_get_accel_group (ui_manager));

  gtk_ui_manager_insert_action_group (ui_manager, group, -1);
  g_object_unref (group);

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <toolbar name=\"help-browser-toolbar\">"
                                     "    <toolitem action=\"reload\" />"
                                     "    <toolitem action=\"stop\" />"
                                     "    <toolitem action=\"home\" />"
                                     "    <separator name=\"space\" />"
                                     "    <toolitem action=\"website\" />"
                                     "  </toolbar>"
                                     "  <accelerator action=\"close\" />"
                                     "  <accelerator action=\"quit\" />"
                                     "</ui>",
                                     -1, &error);

  if (error)
    {
      g_warning ("error parsing ui: %s", error->message);
      g_clear_error (&error);
    }

  gtk_ui_manager_add_ui_from_string (ui_manager,
                                     "<ui>"
                                     "  <popup name=\"help-browser-popup\">"
                                     "    <menuitem action=\"back\" />"
                                     "    <menuitem action=\"forward\" />"
                                     "    <menuitem action=\"reload\" />"
                                     "    <menuitem action=\"stop\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"home\" />"
                                     "    <menuitem action=\"copy-location\" />"
#ifdef HAVE_WEBKIT_ZOOM_API
                                     "    <separator />"
                                     "    <menuitem action=\"zoom-in\" />"
                                     "    <menuitem action=\"zoom-out\" />"
#endif
                                     "    <separator />"
                                     "    <menuitem action=\"close\" />"
                                     "  </popup>"
                                     "</ui>",
                                     -1, &error);

  if (error)
    {
      g_warning ("error parsing ui: %s", error->message);
      g_clear_error (&error);
    }

  return ui_manager;
}

static void
back_callback (GtkAction *action,
               gpointer   data)
{
  webkit_web_view_go_back (WEBKIT_WEB_VIEW (view));
}

static void
forward_callback (GtkAction *action,
                  gpointer   data)
{
  webkit_web_view_go_forward (WEBKIT_WEB_VIEW (view));
}

static void
reload_callback (GtkAction *action,
                 gpointer   data)
{
  webkit_web_view_reload (WEBKIT_WEB_VIEW (view));
}

static void
stop_callback (GtkAction *action,
               gpointer   data)
{
  webkit_web_view_stop_loading (WEBKIT_WEB_VIEW (view));
}

static void
home_callback (GtkAction *action,
               gpointer   data)
{
  GtkTreeModel   *model  = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));
  GimpHelpDomain *domain = g_object_get_data (G_OBJECT (model), "domain");
  GimpHelpLocale *locale = g_object_get_data (G_OBJECT (model), "locale");

  if (domain && locale)
    {
      gchar *uri = g_strconcat (domain->help_uri,  "/",
                                locale->locale_id, "/",
                                gimp_help_locale_map (locale,
                                                      GIMP_HELP_DEFAULT_ID),
                                NULL);
      browser_dialog_load (uri);
      g_free (uri);
    }
}

static void
copy_location_callback (GtkAction *action,
                        gpointer   data)
{
  WebKitWebFrame *frame = webkit_web_view_get_main_frame (WEBKIT_WEB_VIEW (view));
  const gchar    *uri;

  uri = webkit_web_frame_get_uri (frame);

  if (uri)
    {
      GtkClipboard *clipboard;

      clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (view),
                                                 GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_text (clipboard, uri, -1);
    }
}

static void
zoom_in_callback (GtkAction *action,
                  gpointer   data)
{
#ifdef HAVE_WEBKIT_ZOOM_API
  webkit_web_view_zoom_in (WEBKIT_WEB_VIEW (view));
#endif
}

static void
zoom_out_callback (GtkAction *action,
                   gpointer   data)
{
#ifdef HAVE_WEBKIT_ZOOM_API
  webkit_web_view_zoom_out (WEBKIT_WEB_VIEW (view));
#endif
}

static void
website_callback (GtkAction *action,
                  gpointer   data)
{
  browser_dialog_load ("http://docs.gimp.org/");
}

static void
close_callback (GtkAction *action,
                gpointer   data)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (view));
}

static void
menu_callback (GtkWidget            *menu,
               WebKitWebHistoryItem *item)
{
  browser_dialog_load (webkit_web_history_item_get_uri (item));
}

/*  this function unrefs the items and frees the list  */
static GtkWidget *
build_menu (GList *list)
{
  GtkWidget *menu;

  if (! list)
    return NULL;

  menu = gtk_menu_new ();

  do
    {
      WebKitWebHistoryItem *item = list->data;
      GtkWidget            *menu_item;

      menu_item = gtk_menu_item_new_with_label (webkit_web_history_item_get_title (item));

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);

      g_signal_connect_object (menu_item, "activate",
                               G_CALLBACK (menu_callback),
                               item, 0);

      g_object_unref (item);
    }
  while ((list = g_list_next (list)));

  g_list_free (list);

  return menu;
}

static void
update_actions (void)
{
  GtkAction                *action;
  WebKitWebBackForwardList *back_forward_list;
  WebKitWebFrame           *frame;

  back_forward_list =
    webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (view));

  /*  update the back button and its menu  */

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/back");
  gtk_action_set_sensitive (action,
                            webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (view)));

  if (back_forward_list)
    {
      GList *list;

      list = webkit_web_back_forward_list_get_back_list_with_limit (back_forward_list,
                                                                    12);
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_prev),
                                     build_menu (list));
    }
  else
    {
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_prev), NULL);
    }

  /*  update the forward button and its menu  */

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/forward");
  gtk_action_set_sensitive (action,
                            webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (view)));

  if (back_forward_list)
    {
      GList *list;

      list = webkit_web_back_forward_list_get_forward_list_with_limit (back_forward_list,
                                                                       12);
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_next),
                                     build_menu (list));
    }
  else
    {
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_next), NULL);
    }

  /*  update the copy-location action  */
  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/copy-location");

  frame = webkit_web_view_get_main_frame (WEBKIT_WEB_VIEW (view));
  gtk_action_set_sensitive (action, webkit_web_frame_get_uri (frame) != NULL);
}

static void
row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *path,
               GtkTreeViewColumn *column)
{
  GtkTreeModel   *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter     iter;
  GimpHelpDomain *domain;
  GimpHelpLocale *locale;
  GimpHelpItem   *item;
  gchar          *uri;

  gtk_tree_model_get_iter (model, &iter, path);

  gtk_tree_model_get (model, &iter,
                      0, &item,
                      -1);

  domain = g_object_get_data (G_OBJECT (model), "domain");
  locale = g_object_get_data (G_OBJECT (model), "locale");

  uri = g_strconcat (domain->help_uri,  "/",
                     locale->locale_id, "/",
                     item->ref,
                     NULL);

  browser_dialog_load (uri);

  g_free (uri);
}

static void
dialog_unmap (GtkWidget *window,
              GtkWidget *paned)
{
  DialogData  data;

  gtk_window_get_size (GTK_WINDOW (window), &data.width, &data.height);

  data.paned_position = gtk_paned_get_position (GTK_PANED (paned));

#ifdef HAVE_WEBKIT_ZOOM_API
  data.zoom = (view ?
               webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (view)) : 1.0);
#else
  data.zoom = 1.0;
#endif

  gimp_set_data (GIMP_HELP_BROWSER_DIALOG_DATA, &data, sizeof (data));

  gtk_main_quit ();
}

static void
view_realize (GtkWidget *widget)
{
  g_return_if_fail (busy_cursor == NULL);

  busy_cursor = gdk_cursor_new_for_display (gtk_widget_get_display (widget),
                                            GDK_WATCH);
}

static void
view_unrealize (GtkWidget *widget)
{
  if (busy_cursor)
    {
      gdk_cursor_unref (busy_cursor);
      busy_cursor = NULL;
    }
}

static gboolean
view_popup_menu (GtkWidget      *widget,
                 GdkEventButton *event)
{
  GtkWidget *menu = gtk_ui_manager_get_widget (ui_manager,
                                               "/help-browser-popup");

  gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));
  gtk_menu_popup (GTK_MENU (menu),
                  NULL, NULL, NULL, NULL,
                  event ? event->button : 0,
                  event ? event->time   : gtk_get_current_event_time ());

  return TRUE;
}

static gboolean
view_button_press (GtkWidget      *widget,
                   GdkEventButton *event)
{
  if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    return view_popup_menu (widget, event);

  return FALSE;
}

static void
title_changed (GtkWidget      *view,
               WebKitWebFrame *frame,
               const gchar    *title,
               GtkWidget      *window)
{
  gchar *full_title;

  full_title = g_strdup_printf ("%s - %s",
                                title ? title : _("Untitled"),
                                _("GIMP Help Browser"));

  gtk_window_set_title (GTK_WINDOW (window), full_title);
  g_free (full_title);

  update_actions ();
}

static void
load_started (GtkWidget      *view,
              WebKitWebFrame *frame)
{
  GtkAction *action = gtk_ui_manager_get_action (ui_manager,
                                                 "/ui/help-browser-popup/stop");
  gtk_action_set_sensitive (action, TRUE);
}

static void
load_finished (GtkWidget      *view,
               WebKitWebFrame *frame)
{
  GtkAction *action = gtk_ui_manager_get_action (ui_manager,
                                                 "/ui/help-browser-popup/stop");
  gtk_action_set_sensitive (action, FALSE);

  update_actions ();
}
