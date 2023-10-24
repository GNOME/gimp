/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP Help Browser
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Róman Joost <romanofski@gimp.org>
 *                         Niels De Graef <nielsdg@redhat.com>
 *
 * dialog.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>

#include <webkit2/webkit2.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "plug-ins/help/gimphelp.h"

#include "dialog.h"
#include "uri.h"

#include "libgimp/stdplugins-intl.h"


#define GIMP_HELP_BROWSER_DIALOG_DATA      "gimp-help-browser-dialog"
#define GIMP_HELP_BROWSER_INDEX_MAX_DEPTH  4


typedef struct
{
  int      width;
  int      height;
  int      paned_position;
  gboolean show_index;
  double   zoom;
} DialogData;

struct _GimpHelpBrowserDialog
{
  GtkApplicationWindow  parent_instance;

  GHashTable           *uri_hash_table; /* (char*) → (GtkTreeIter) */

  GtkWidget            *webview;
  GtkWidget            *paned;
  GtkWidget            *sidebar;
  GtkWidget            *searchbar;
  GtkWidget            *search_entry;
  GtkWidget            *tree_view;
  GtkWidget            *button_prev;
  GtkWidget            *button_next;
  GdkCursor            *busy_cursor;

  GMenuModel           *popup_menu_model;
  GMenuModel           *copy_popup_menu_model;

  GimpProcedureConfig  *config;
};

G_DEFINE_TYPE (GimpHelpBrowserDialog, gimp_help_browser_dialog, GTK_TYPE_APPLICATION_WINDOW)


static void        gimp_help_browser_dialog_finalize (GObject                  *object);

/* Actions. */

static void        back_action                       (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        step_action                       (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        forward_action                    (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        reload_action                     (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        stop_action                       (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        home_action                       (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        find_action                       (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        find_again_action                 (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        search_next_action                (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        search_previous_action            (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        copy_location_action              (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        copy_selection_action             (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        show_index_change_state           (GSimpleAction            *action,
                                                      GVariant                 *new_state,
                                                      gpointer                  user_data);
static void        zoom_in_action                    (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        zoom_out_action                   (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        load_uri_action                   (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);
static void        close_action                      (GSimpleAction            *action,
                                                      GVariant                 *parameter,
                                                      gpointer                  user_data);

/* Callbacks. */

static void        webview_realize                   (GtkWidget                *webview,
                                                      gpointer                  user_data);
static void        webview_unrealize                 (GtkWidget                *widget,
                                                      gpointer                  user_data);
static void        do_popup_menu                     (GimpHelpBrowserDialog    *self,
                                                      GtkWidget                *webview,
                                                      GdkEvent                 *event);
static gboolean    webview_popup_menu                (GtkWidget                *webview,
                                                      gpointer                  user_data);
static gboolean    webview_button_press              (GtkWidget                *webview,
                                                      GdkEventButton           *event,
                                                      gpointer                  user_data);
static gboolean    webview_key_press                 (GtkWidget                *widget,
                                                      GdkEventKey              *event,
                                                      gpointer                  user_data);
static void        webview_title_changed             (WebKitWebView            *webview,
                                                      GParamSpec               *pspec,
                                                      gpointer                  user_data);
static void        select_index                      (GimpHelpBrowserDialog    *self,
                                                      const char               *uri);
static gboolean    webview_decide_policy             (WebKitWebView            *webview,
                                                      WebKitPolicyDecision     *decision,
                                                      WebKitPolicyDecisionType  decision_type,
                                                      gpointer                  user_data);
static gboolean    webview_load_failed               (WebKitWebView            *webview,
                                                      WebKitLoadEvent           load_event,
                                                      char                     *failing_uri,
                                                      GError                   *error,
                                                      gpointer                  user_data);
static void        webview_load_changed              (WebKitWebView            *webview,
                                                      WebKitLoadEvent           event,
                                                      gpointer                  user_data);
static void        row_activated                     (GtkTreeView              *tree_view,
                                                      GtkTreePath              *path,
                                                      GtkTreeViewColumn        *column,
                                                      gpointer                  user_data);
static void        search_close_clicked              (GtkWidget                *button,
                                                      gpointer                  user_data);
static void        search_entry_changed              (GtkWidget                *search_entry,
                                                      gpointer                  user_data);
static gboolean    search_entry_key_press            (GtkWidget                *search_entry,
                                                      GdkEventKey              *event,
                                                      gpointer                  user_data);
static void        dialog_unmap                      (GtkWidget                *window,
                                                      gpointer                  user_data);

/* Utilities. */

static void        search                            (GimpHelpBrowserDialog    *self,
                                                      const char               *text);

static GtkWidget * build_menu                        (const GList              *items,
                                                      gboolean                  back);

static void        update_actions                    (GimpHelpBrowserDialog    *self);

static void        add_tool_button                   (GtkWidget                *toolbar,
                                                      const char               *action,
                                                      const char               *icon,
                                                      const char               *label,
                                                      const char               *tooltip);
static GtkWidget * build_searchbar                   (GimpHelpBrowserDialog    *self);

static void        browser_dialog_make_index_foreach (const gchar              *help_id,
                                                      GimpHelpItem             *item,
                                                      GimpHelpLocale           *locale);
static gint        help_item_compare                 (gconstpointer             a,
                                                      gconstpointer             b);
static void        add_child                         (GimpHelpBrowserDialog    *self,
                                                      GtkTreeStore             *store,
                                                      GimpHelpDomain           *domain,
                                                      GimpHelpLocale           *locale,
                                                      GtkTreeIter              *parent,
                                                      GimpHelpItem             *item,
                                                      int                       depth);


static const GActionEntry ACTIONS[] =
{
  { "back",            back_action },
  { "forward",         forward_action },
  { "step",            step_action, "i" },
  { "reload",          reload_action },
  { "stop",            stop_action },
  { "home",            home_action },
  { "load-uri",        load_uri_action, "s" },
  { "copy-location",   copy_location_action },
  { "copy-selection",  copy_selection_action },
  { "zoom-in",         zoom_in_action },
  { "zoom-out",        zoom_out_action },
  { "find",            find_action },
  { "find-again",      find_again_action },
  { "search-next",     search_next_action },
  { "search-previous", search_previous_action },
  { "close",           close_action },
  { "show-index",      NULL, NULL, "true", show_index_change_state },
};


static void
gimp_help_browser_dialog_class_init (GimpHelpBrowserDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_help_browser_dialog_finalize;
}

static void
gimp_help_browser_dialog_init (GimpHelpBrowserDialog *self)
{
  GtkWindow      *window = GTK_WINDOW (self);
  GtkWidget      *vbox;
  GtkWidget      *toolbar;
  GtkBuilder     *builder;
  GtkToolItem    *item;
  GtkWidget      *main_vbox;
  WebKitSettings *settings;

  /*  the dialog window  */
  gtk_window_set_title (GTK_WINDOW (window), _("GIMP Help Browser"));
  gtk_window_set_icon_name (GTK_WINDOW (window), GIMP_ICON_HELP_USER_MANUAL);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   ACTIONS, G_N_ELEMENTS (ACTIONS),
                                   self);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (vbox);

  /* Toolbar */
  toolbar = gtk_toolbar_new ();

  add_tool_button (toolbar, "win.reload", GIMP_ICON_VIEW_REFRESH, _("_Reload"), _("Reload current page"));
  add_tool_button (toolbar, "win.stop", GIMP_ICON_PROCESS_STOP, _("_Stop"), _("Stop loading this page"));
  add_tool_button (toolbar, "win.home", GIMP_ICON_GO_HOME, NULL, _("Go to the index page"));
  item = gtk_separator_tool_item_new ();
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (item), FALSE);
  gtk_tool_item_set_expand (item, TRUE);
  gtk_widget_show (GTK_WIDGET (item));
  add_tool_button (toolbar, "win.load-uri('https://docs.gimp.org')", GIMP_ICON_HELP_USER_MANUAL, "docs.gimp.org", _("Visit the GIMP documentation website"));

  item = gtk_menu_tool_button_new (gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT, GTK_ICON_SIZE_BUTTON), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  gtk_widget_show (GTK_WIDGET (item));
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), "win.forward");
  self->button_next = GTK_WIDGET (item);

  item = gtk_menu_tool_button_new (gtk_image_new_from_icon_name (GIMP_ICON_GO_PREVIOUS, GTK_ICON_SIZE_BUTTON), NULL);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, 0);
  gtk_widget_show (GTK_WIDGET (item));
  gtk_actionable_set_action_name (GTK_ACTIONABLE (item), "win.back");
  self->button_prev = GTK_WIDGET (item);

  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  /* Context menu */
  builder = gtk_builder_new_from_resource ("/org/gimp/help/help-menu.ui");
  self->popup_menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "help_browser_popup_menu"));
  g_object_ref (self->popup_menu_model);
  self->copy_popup_menu_model = G_MENU_MODEL (gtk_builder_get_object (builder, "help_browser_copy_popup_menu"));
  g_object_ref (self->copy_popup_menu_model);
  g_object_unref (builder);

  /*  the horizontal paned  */
  self->paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (vbox), self->paned, TRUE, TRUE, 0);
  gtk_widget_show (self->paned);

  self->sidebar = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->sidebar),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_add1 (GTK_PANED (self->paned), self->sidebar);

  self->tree_view = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (self->tree_view), FALSE);
  gtk_container_add (GTK_CONTAINER (self->sidebar), self->tree_view);
  gtk_widget_show (self->tree_view);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tree_view), -1,
                                               NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  g_signal_connect (self->tree_view, "row-activated",
                    G_CALLBACK (row_activated),
                    self);

  /*  HTML webview  */
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (main_vbox);
  gtk_paned_pack2 (GTK_PANED (self->paned), main_vbox, TRUE, TRUE);

  settings = webkit_settings_new_with_settings ("default-charset", "utf-8",
                                                NULL);
  self->webview = webkit_web_view_new_with_settings (settings);
  g_object_unref (settings);

  gtk_widget_set_size_request (self->webview, 300, 200);
  gtk_widget_show (self->webview);

  gtk_box_pack_start (GTK_BOX (main_vbox), self->webview, TRUE, TRUE, 0);

  g_signal_connect (self->webview, "realize",
                    G_CALLBACK (webview_realize),
                    self);
  g_signal_connect (self->webview, "unrealize",
                    G_CALLBACK (webview_unrealize),
                    self);
  g_signal_connect (self->webview, "popup-menu",
                    G_CALLBACK (webview_popup_menu),
                    self);
  g_signal_connect (self->webview, "button-press-event",
                    G_CALLBACK (webview_button_press),
                    self);
  g_signal_connect (self->webview, "key-press-event",
                    G_CALLBACK (webview_key_press),
                    self);
  g_signal_connect (self->webview, "notify::title",
                    G_CALLBACK (webview_title_changed),
                    self);
  g_signal_connect (self->webview, "load-changed",
                    G_CALLBACK (webview_load_changed),
                    self);
  g_signal_connect (self->webview, "load-failed",
                    G_CALLBACK (webview_load_failed),
                    self);
  g_signal_connect (self->webview, "decide-policy",
                    G_CALLBACK (webview_decide_policy),
                    self);

  gtk_widget_grab_focus (self->webview);

  g_signal_connect (window, "unmap",
                    G_CALLBACK (dialog_unmap),
                    self);

  update_actions (self);

  /* Searchbar */
  self->searchbar = build_searchbar (self);
  gtk_box_pack_start (GTK_BOX (main_vbox), self->searchbar, FALSE, FALSE, 0);
}

static void
gimp_help_browser_dialog_finalize (GObject *object)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (object);

  g_clear_pointer (&self->uri_hash_table, g_hash_table_unref);
  g_clear_object (&self->popup_menu_model);
  g_clear_object (&self->copy_popup_menu_model);

  G_OBJECT_CLASS (gimp_help_browser_dialog_parent_class)->finalize (object);
}

/* Public functions. */

GimpHelpBrowserDialog *
gimp_help_browser_dialog_new (const gchar         *plug_in_binary,
                              GApplication        *app,
                              GimpProcedureConfig *config)
{
  GimpHelpBrowserDialog *window;
  GBytes                *bytes = NULL;
  DialogData             data  = { 720, 560, 240, TRUE, 1.0 };

  g_object_get (config, "dialog-data", &bytes, NULL);
  if (bytes != NULL && g_bytes_get_size (bytes) == sizeof (DialogData))
    data = *((DialogData *) g_bytes_get_data (bytes, NULL));
  g_bytes_unref (bytes);

  gimp_ui_init (plug_in_binary);

  window = g_object_new (GIMP_TYPE_HELP_BROWSER_DIALOG,
                         "application", app,
                         "role", plug_in_binary,
                         "default-width", data.width,
                         "default-height", data.height,
                         NULL);
  window->config = config;

  gtk_paned_set_position (GTK_PANED (window->paned), data.paned_position);
  if (data.show_index)
    gtk_widget_show (window->sidebar);
  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (window->webview), data.zoom);

  return window;
}

void
gimp_help_browser_dialog_load (GimpHelpBrowserDialog *self,
                               const char            *uri)
{
  g_return_if_fail (uri && *uri);

  webkit_web_view_load_uri (WEBKIT_WEB_VIEW (self->webview), uri);

  select_index (self, uri);

  gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (self->webview)));
}

void
gimp_help_browser_dialog_make_index (GimpHelpBrowserDialog *self,
                                     GimpHelpDomain        *domain,
                                     GimpHelpLocale        *locale)
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

  if (self->uri_hash_table)
    g_hash_table_unref (self->uri_hash_table);

  self->uri_hash_table = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                (GDestroyNotify) g_free,
                                                (GDestroyNotify) gtk_tree_iter_free);

  for (list = locale->toplevel_items; list; list = g_list_next (list))
    {
      GimpHelpItem *item = list->data;

      add_child (self, store, domain, locale, NULL, item, 0);
    }

  gtk_tree_view_set_model (GTK_TREE_VIEW (self->tree_view), GTK_TREE_MODEL (store));
  g_object_unref (store);
}

/* Private functions. */

static void
back_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  webkit_web_view_go_back (WEBKIT_WEB_VIEW (self->webview));
}

static void
step_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GimpHelpBrowserDialog     *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  gint                       steps;
  WebKitBackForwardList     *back_fw_list;
  WebKitBackForwardListItem *back_fw_list_item;

  g_return_if_fail (parameter);

  steps = g_variant_get_int32 (parameter);

  back_fw_list =
    webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (self->webview));
  back_fw_list_item = webkit_back_forward_list_get_nth_item (back_fw_list, steps);
  if (back_fw_list_item)
    webkit_web_view_go_to_back_forward_list_item (WEBKIT_WEB_VIEW (self->webview),
                                                  back_fw_list_item);
}

static void
forward_action (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  webkit_web_view_go_forward (WEBKIT_WEB_VIEW (self->webview));
}

static void
reload_action (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  webkit_web_view_reload (WEBKIT_WEB_VIEW (self->webview));
}

static void
stop_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  webkit_web_view_stop_loading (WEBKIT_WEB_VIEW (self->webview));
}

static void
home_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  GtkTreeModel          *model;
  GimpHelpDomain        *domain;
  GimpHelpLocale        *locale;

  model  = gtk_tree_view_get_model (GTK_TREE_VIEW (self->tree_view));
  domain = g_object_get_data (G_OBJECT (model), "domain");
  locale = g_object_get_data (G_OBJECT (model), "locale");
  if (domain && locale)
    {
      gchar *uri = g_strconcat (domain->help_uri,  "/",
                                locale->locale_id, "/",
                                gimp_help_locale_map (locale,
                                                      GIMP_HELP_DEFAULT_ID),
                                NULL);
      gimp_help_browser_dialog_load (self, uri);
      g_free (uri);
    }
}

static void
find_action (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  gtk_widget_show (self->searchbar);
  gtk_widget_grab_focus (self->search_entry);
}

static void
find_again_action (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  gtk_widget_show (self->searchbar);
  gtk_widget_grab_focus (self->search_entry);

  search (self, gtk_entry_get_text (GTK_ENTRY (self->search_entry)));
}

static void
search_next_action (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  WebKitFindController *find_controller;

  find_controller =
    webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (self->webview));
  webkit_find_controller_search_next (find_controller);
}

static void
search_previous_action (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  WebKitFindController *find_controller;

  find_controller =
    webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (self->webview));
  webkit_find_controller_search_previous (find_controller);
}

static void
copy_location_action (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  const char *uri;

  uri = webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self->webview));
  if (uri)
    {
      GtkClipboard *clipboard;

      clipboard = gtk_clipboard_get_for_display (gtk_widget_get_display (self->webview),
                                                 GDK_SELECTION_CLIPBOARD);
      gtk_clipboard_set_text (clipboard, uri, -1);
    }
}

static void
copy_selection_action (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  WebKitEditorState     *editor_state;

  editor_state = webkit_web_view_get_editor_state (WEBKIT_WEB_VIEW (self->webview));
  if (webkit_editor_state_is_copy_available (editor_state))
    {
      webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self->webview),
                                               WEBKIT_EDITING_COMMAND_COPY);
    }
}

static void
show_index_change_state (GSimpleAction *action,
                         GVariant      *new_state,
                         gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  gboolean               show_index;

  show_index = g_variant_get_boolean (new_state);
  g_warning ("NEW STATE %s", show_index? "true" : "false");
  gtk_widget_set_visible (self->sidebar, show_index);
  g_simple_action_set_state (action, new_state);
}

static void
zoom_in_action (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  double                 zoom_level;

  zoom_level = webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (self->webview));
  if (zoom_level < 10.0)
    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (self->webview), zoom_level + 0.1);
}

static void
zoom_out_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  double                 zoom_level;

  zoom_level = webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (self->webview));
  if (zoom_level > 0.1)
    webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (self->webview), zoom_level - 0.1);
}

static void
load_uri_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  const char            *uri;

  uri = g_variant_get_string (parameter, NULL);
  gimp_help_browser_dialog_load (self, uri);
}

static void
close_action (GSimpleAction *action,
              GVariant      *parameter,
              gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
webview_realize (GtkWidget *webview,
                 gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  g_return_if_fail (self->busy_cursor == NULL);

  self->busy_cursor = gdk_cursor_new_for_display (gtk_widget_get_display (webview),
                                                  GDK_WATCH);
}

static void
webview_unrealize (GtkWidget *widget,
                   gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  g_clear_object (&self->busy_cursor);
}

static void
do_popup_menu (GimpHelpBrowserDialog *self,
               GtkWidget             *webview,
               GdkEvent              *event)
{
  WebKitEditorState *editor_state;
  GMenuModel        *menu_model;
  GtkWidget         *menu;

  editor_state = webkit_web_view_get_editor_state (WEBKIT_WEB_VIEW (webview));
  if (webkit_editor_state_is_copy_available (editor_state))
    menu_model = self->copy_popup_menu_model;
  else
    menu_model = self->popup_menu_model;

  menu = gtk_menu_new_from_model (menu_model);
  g_signal_connect (menu, "deactivate",
                    G_CALLBACK (gtk_widget_destroy), NULL);

  gtk_menu_attach_to_widget (GTK_MENU (menu), webview, NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), event);
}

static gboolean
webview_popup_menu (GtkWidget *webview,
                    gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  do_popup_menu (self, webview, NULL);
  return TRUE;
}

static gboolean
webview_button_press (GtkWidget      *webview,
                      GdkEventButton *event,
                      gpointer        user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  if (gdk_event_triggers_context_menu ((GdkEvent *) event))
    {
      do_popup_menu (self, webview, (GdkEvent *) event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
webview_key_press (GtkWidget   *widget,
                   GdkEventKey *event,
                   gpointer     user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  if (event->keyval == GDK_KEY_slash)
    {
      g_action_group_activate_action (G_ACTION_GROUP (self),
                                      "find",
                                      NULL);
      return TRUE;
    }

  return FALSE;
}

static void
webview_title_changed (WebKitWebView *webview,
                       GParamSpec    *pspec,
                       gpointer       user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  const char            *title;
  char                  *full_title;

  title = webkit_web_view_get_title (webview);
  full_title = g_strdup_printf ("%s - %s",
                                title ? title : _("Untitled"),
                                _("GIMP Help Browser"));

  gtk_window_set_title (GTK_WINDOW (self), full_title);
  g_free (full_title);

  update_actions (self);
}

static void
select_index (GimpHelpBrowserDialog *self,
              const char            *uri)
{
  GtkTreeSelection *selection;
  GtkTreeIter      *iter = NULL;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->tree_view));

  if (uri)
    iter = g_hash_table_lookup (self->uri_hash_table, uri);

  if (iter)
    {
      GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self->tree_view));
      GtkTreePath  *path;
      GtkTreePath  *scroll_path;

      path = gtk_tree_model_get_path (model, iter);
      scroll_path = gtk_tree_path_copy (path);

      gtk_tree_path_up (path);
      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (self->tree_view), path);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self->tree_view), scroll_path,
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

static gboolean
webview_decide_policy (WebKitWebView            *webview,
                       WebKitPolicyDecision     *decision,
                       WebKitPolicyDecisionType  decision_type,
                       gpointer                  user_data)
{
  /* Some files return mime types like application/x-extension-html,
   * which is not supported by default */
  webkit_policy_decision_use (decision);
  return FALSE;
}

static gboolean
webview_load_failed (WebKitWebView  *webview,
                     WebKitLoadEvent load_event,
                     char           *failing_uri,
                     GError         *error,
                     gpointer        user_data)
{
  g_warning ("Failed to load page: %s", error->message);
  return TRUE;
}

static void
webview_load_changed (WebKitWebView   *webview,
                      WebKitLoadEvent  event,
                      gpointer         user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  GAction               *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (self), "back");
  switch (event)
    {
    case WEBKIT_LOAD_STARTED:
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
      break;

    case WEBKIT_LOAD_FINISHED:
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
      update_actions (self);
      select_index (self, webkit_web_view_get_uri (webview));
      break;

    case WEBKIT_LOAD_REDIRECTED:
    case WEBKIT_LOAD_COMMITTED:
      break;
    }
}

static void
row_activated (GtkTreeView       *tree_view,
               GtkTreePath       *path,
               GtkTreeViewColumn *column,
               gpointer           user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  GtkTreeModel          *model;
  GtkTreeIter            iter;
  GimpHelpDomain        *domain;
  GimpHelpLocale        *locale;
  GimpHelpItem          *item;
  char                  *uri;

  model = gtk_tree_view_get_model (tree_view);
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

  gimp_help_browser_dialog_load (self, uri);

  g_free (uri);
}

static void
search_close_clicked (GtkWidget *button,
                      gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  WebKitFindController  *find_controller =
    webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (self->webview));

  gtk_widget_hide (self->searchbar);

  webkit_find_controller_search_finish (find_controller);
}

static void
search_entry_changed (GtkWidget *search_entry,
                      gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);

  search (self, gtk_entry_get_text (GTK_ENTRY (search_entry)));
}

static gboolean
search_entry_key_press (GtkWidget   *search_entry,
                        GdkEventKey *event,
                        gpointer     user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  WebKitFindController  *find_controller;

  find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (self->webview));
  switch (event->keyval)
    {
    case GDK_KEY_Escape:
      gtk_widget_hide (self->searchbar);
      webkit_find_controller_search_finish (find_controller);
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      search (self, gtk_entry_get_text (GTK_ENTRY (search_entry)));
      return TRUE;
    }

  return FALSE;
}

static void
dialog_unmap (GtkWidget *window,
              gpointer   user_data)
{
  GimpHelpBrowserDialog *self = GIMP_HELP_BROWSER_DIALOG (user_data);
  GBytes                *bytes;
  DialogData             data = { 720, 560, 240, TRUE, 1.0 };

  gtk_window_get_size (GTK_WINDOW (window), &data.width, &data.height);

  data.paned_position = gtk_paned_get_position (GTK_PANED (self->paned));
  data.show_index     = gtk_widget_get_visible (self->sidebar);

  data.zoom = (self->webview ?
               webkit_web_view_get_zoom_level (WEBKIT_WEB_VIEW (self->webview)) : 1.0);

  bytes = g_bytes_new (&data, sizeof (DialogData));
  g_object_set (self->config, "dialog-data", bytes, NULL);
  g_bytes_unref (bytes);
}

static void
search (GimpHelpBrowserDialog *self,
        const char            *text)
{
  WebKitFindController *find_controller;

  find_controller = webkit_web_view_get_find_controller (WEBKIT_WEB_VIEW (self->webview));
  if (text)
    {
      const char *prev_text =
        webkit_find_controller_get_search_text (find_controller);

      /* The previous search, if any, may need to be canceled. */
      if (prev_text && strcmp (text, prev_text) != 0)
        webkit_find_controller_search_finish (find_controller);

      webkit_find_controller_search (find_controller,
                                     text,
                                     WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                                     WEBKIT_FIND_OPTIONS_WRAP_AROUND,
                                     G_MAXUINT);
    }
  else
    {
      webkit_find_controller_search_finish (find_controller);
    }
}

static GtkWidget *
build_menu (const GList *items,
            gboolean     back)
{
  GMenu       *menu;
  const GList *iter;
  int          steps;

  if (!items)
    return NULL;

  menu = g_menu_new ();

  /* Go over every item in the back_fw list and add it to the menu */
  for (iter = items, steps = 1; iter; iter = g_list_next (iter), steps++)
    {
      WebKitBackForwardListItem *item = iter->data;
      const char                *title;
      char                      *action;

      title = webkit_back_forward_list_item_get_title (item);
      if (title == NULL)
        continue;

      action = g_strdup_printf ("steps(%d)", steps);
      g_menu_insert (menu, steps - 1, title, action);
      g_free (action);
    }

  return gtk_menu_new_from_model (G_MENU_MODEL (menu));
}

static void
update_actions (GimpHelpBrowserDialog *self)
{
  GActionMap            *action_map = G_ACTION_MAP (self);
  GAction               *action;
  WebKitBackForwardList *back_forward_list;

  back_forward_list =
    webkit_web_view_get_back_forward_list (WEBKIT_WEB_VIEW (self->webview));

  /*  update the back button and its menu  */
  action = g_action_map_lookup_action (action_map, "back");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_back (WEBKIT_WEB_VIEW (self->webview)));

  if (back_forward_list)
    {
      const GList *list;

      list = webkit_back_forward_list_get_back_list_with_limit (back_forward_list,
                                                                12);
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (self->button_prev),
                                     build_menu (list, TRUE));
    }
  else
    {
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (self->button_prev), NULL);
    }

  /*  update the forward button and its menu  */
  action = g_action_map_lookup_action (action_map, "forward");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_can_go_forward (WEBKIT_WEB_VIEW (self->webview)));

  if (back_forward_list)
    {
      const GList *list;

      list = webkit_back_forward_list_get_forward_list_with_limit (back_forward_list,
                                                                   12);
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (self->button_next),
                                     build_menu (list, FALSE));
    }
  else
    {
      gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (self->button_next), NULL);
    }

  /*  update the copy-location action  */
  action = g_action_map_lookup_action (action_map, "copy-location");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
                               webkit_web_view_get_uri (WEBKIT_WEB_VIEW (self->webview)) != NULL);

  /*  update the show-index action  */
  action = g_action_map_lookup_action (action_map, "show-index");
  g_simple_action_set_state (G_SIMPLE_ACTION (action),
                             g_variant_new_boolean (gtk_widget_get_visible (self->sidebar)));
}

static void
add_tool_button (GtkWidget  *toolbar,
                 const char *action,
                 const char *icon,
                 const char *label,
                 const char *tooltip)
{
  GtkWidget   *tool_icon;
  GtkToolItem *tool_button;

  tool_icon = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (GTK_WIDGET (tool_icon));
  tool_button = gtk_tool_button_new (tool_icon, label);
  gtk_widget_show (GTK_WIDGET (tool_button));
  gtk_tool_item_set_tooltip_text (tool_button, tooltip);
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (tool_button), action);

  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_button, -1);
}

static GtkWidget *
build_searchbar (GimpHelpBrowserDialog *self)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label = gtk_label_new (_("Find:"));
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  self->search_entry = gtk_entry_new ();
  gtk_widget_show (self->search_entry);
  gtk_box_pack_start (GTK_BOX (hbox), self->search_entry, TRUE, TRUE, 0);

  g_signal_connect (self->search_entry, "changed",
                    G_CALLBACK (search_entry_changed),
                    self);

  g_signal_connect (self->search_entry, "key-press-event",
                    G_CALLBACK (search_entry_key_press),
                    self);

  button = gtk_button_new_with_mnemonic (C_("search", "_Previous"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (GIMP_ICON_GO_PREVIOUS,
                                                      GTK_ICON_SIZE_BUTTON));
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.search-previous");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (C_("search", "_Next"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT,
                                                      GTK_ICON_SIZE_BUTTON));
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.search-next");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_mnemonic (C_("search", "_Close"));
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (search_close_clicked),
                    self);

  return hbox;
}

static void
browser_dialog_make_index_foreach (const gchar    *help_id,
                                   GimpHelpItem   *item,
                                   GimpHelpLocale *locale)
{
  gchar *sort_key = item->title;

#if DEBUG_SORT_HELP_ITEMS
  g_printerr ("%s: processing %s (parent %s)\n",
              G_STRFUNC,
              item->title  ? item->title  : "NULL",
              item->parent ? item->parent : "NULL");
#endif

  if (item->sort &&
      g_regex_match_simple ("^[0-9]+([.][0-9]+)*$", item->sort, 0, 0))
    {
      sort_key = item->sort;

#if DEBUG_SORT_HELP_ITEMS
      g_printerr ("%s: sort key = %s\n", G_STRFUNC, sort_key);
#endif
    }

  item->index = 0;

  if (sort_key)
    {
      int    max_tokens = GIMP_HELP_BROWSER_INDEX_MAX_DEPTH;
      char **indices = g_strsplit (sort_key, ".", max_tokens + 1);
      int    i;

      for (i = 0; i < max_tokens; i++)
        {
          gunichar c;

          if (! indices[i])
            {
              /* make sure that all item->index's are comparable */
              item->index <<= (8 * (max_tokens - i));
              break;
            }

          item->index <<= 8;  /* NOP if i = 0 */
          c = g_utf8_get_char (indices[i]);
          if (g_unichar_isdigit (c))
            {
              item->index += atoi (indices[i]);
            }
          else if (g_utf8_strlen (indices[i], -1) == 1)
            {
              item->index += (c & 0xFF);
            }
        }

      g_strfreev (indices);

#if DEBUG_SORT_HELP_ITEMS
      g_printerr ("%s: index = %lu\n", G_STRFUNC, item->index);
#endif
    }

  if (item->parent && strlen (item->parent))
    {
      GimpHelpItem *parent;

      parent = g_hash_table_lookup (locale->help_id_mapping, item->parent);

      if (parent)
        {
          parent->children = g_list_prepend (parent->children, item);
        }
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
  const GimpHelpItem *item_a = a;
  const GimpHelpItem *item_b = b;

  if (item_a->index > item_b->index)
    return 1;
  else if (item_a->index < item_b->index)
    return -1;

  return 0;
}

static void
add_child (GimpHelpBrowserDialog *self,
           GtkTreeStore          *store,
           GimpHelpDomain        *domain,
           GimpHelpLocale        *locale,
           GtkTreeIter           *parent,
           GimpHelpItem          *item,
           int                    depth)
{
  GtkTreeIter iter;
  char       *uri;

  gtk_tree_store_append (store, &iter, parent);

  gtk_tree_store_set (store, &iter,
                      0, item,
                      1, item->title,
                      -1);

  uri = g_strconcat (domain->help_uri,  "/",
                     locale->locale_id, "/",
                     item->ref,
                     NULL);

  g_hash_table_insert (self->uri_hash_table,
                       uri,
                       gtk_tree_iter_copy (&iter));

  if (depth + 1 == GIMP_HELP_BROWSER_INDEX_MAX_DEPTH)
    return;

  item->children = g_list_sort (item->children, help_item_compare);

  for (GList *list = item->children; list; list = g_list_next (list))
    {
      GimpHelpItem *item = list->data;

      add_child (self, store, domain, locale, &iter, item, depth + 1);
    }
}
