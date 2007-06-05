/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GIMP Help Browser
 * Copyright (C) 1999-2005 Sven Neumann <sven@gimp.org>
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

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include <libgtkhtml/gtkhtml.h>

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
#include "queue.h"
#include "uri.h"

#include "logo-pixbuf.h"

#include "libgimp/stdplugins-intl.h"

#ifndef _O_BINARY
#define _O_BINARY 0
#endif


#define GIMP_HELP_BROWSER_DIALOG_DATA  "gimp-help-browser-dialog"

typedef struct
{
  gint32  width;
  gint32  height;
  gint32  paned_position;
} DialogData;

enum
{
  HISTORY_TITLE,
  HISTORY_URI
};

/*  local function prototypes  */

static GtkUIManager * ui_manager_new (GtkWidget         *window);
static void       browser_dialog_404 (HtmlDocument      *doc,
                                      const gchar       *uri,
                                      const gchar       *message);

static void       back_callback      (GtkAction         *action,
                                      gpointer           data);
static void       forward_callback   (GtkAction         *action,
                                      gpointer           data);
static void       index_callback     (GtkAction         *action,
                                      gpointer           data);
static void       zoom_in_callback   (GtkAction         *action,
                                      gpointer           data);
static void       zoom_out_callback  (GtkAction         *action,
                                      gpointer           data);
static void       close_callback     (GtkAction         *action,
                                      gpointer           data);
static void       online_callback    (GtkAction         *action,
                                      gpointer           data);

static void       update_toolbar     (void);

static void       combo_changed      (GtkWidget         *widget,
                                      gpointer           data);

static void       drag_begin         (GtkWidget         *widget,
                                      GdkDragContext    *context,
                                      gpointer           data);
static void       drag_data_get      (GtkWidget         *widget,
                                      GdkDragContext    *context,
                                      GtkSelectionData  *selection_data,
                                      guint              info,
                                      guint              time,
                                      gpointer           data);

static void       row_activated      (GtkTreeView       *tree_view,
                                      GtkTreePath       *path,
                                      GtkTreeViewColumn *column);
static void       dialog_unmap       (GtkWidget         *window,
                                      GtkWidget         *paned);

static void       view_realize       (GtkWidget         *widget);
static void       view_unrealize     (GtkWidget         *widget);
static gboolean   view_popup_menu    (GtkWidget         *widget,
                                      GdkEventButton    *event);
static gboolean   view_button_press  (GtkWidget         *widget,
                                      GdkEventButton    *event);

static void       title_changed      (HtmlDocument      *doc,
                                      const gchar       *new_title,
                                      gpointer           data);
static void       link_clicked       (HtmlDocument      *doc,
                                      const gchar       *uri,
                                      gpointer           data);
static gboolean   request_url        (HtmlDocument      *doc,
                                      const gchar       *uri,
                                      HtmlStream        *stream,
                                      GError            **error);
static gboolean   io_handler         (GIOChannel        *io,
                                      GIOCondition       condition,
                                      gpointer           data);
static void       load_remote_page   (const gchar       *uri);

static void       history_add        (GtkComboBox       *combo,
                                      const gchar       *uri,
                                      const gchar       *title);

static gboolean   has_case_prefix    (const gchar       *haystack,
                                      const gchar       *needle);

static gchar    * filename_from_uri  (const gchar       *uri);


/*  private variables  */

static const gchar   *eek_png_tag    = "<h1>Eeek!</h1>";

static Queue         *queue          = NULL;
static gchar         *current_uri    = NULL;
static GHashTable    *uri_hash_table = NULL;

static GtkWidget     *html           = NULL;
static GtkWidget     *tree_view      = NULL;
static GtkUIManager  *ui_manager     = NULL;
static GtkWidget     *button_prev    = NULL;
static GtkWidget     *button_next    = NULL;
static GdkCursor     *busy_cursor    = NULL;

static GtkTargetEntry help_dnd_target_table[] =
{
  { "text/uri-list", 0, 0 },
  { "_NETSCAPE_URL", 0, 0 }
};


/*  public functions  */

void
browser_dialog_open (void)
{
  GtkWidget       *window;
  GtkWidget       *vbox;
  GtkWidget       *toolbar;
  GtkWidget       *paned;
  GtkWidget       *hbox;
  GtkWidget       *scroll;
  GtkWidget       *drag_source;
  GtkWidget       *image;
  GtkWidget       *combo;
  GtkWidget       *button;
  GtkToolItem     *item;
  GtkAction       *action;
  GtkListStore    *history;
  GtkCellRenderer *cell;
  GdkPixbuf       *pixbuf;
  gchar           *eek_png_path;
  DialogData       data = { 720, 560, 240 };

  gimp_ui_init ("helpbrowser", TRUE);

  gimp_get_data (GIMP_HELP_BROWSER_DIALOG_DATA, &data);

  eek_png_path = g_build_filename (gimp_data_directory (),
                                   "themes", "Default", "images",
                                   "stock-wilber-eek-64.png", NULL);

  if (g_file_test (eek_png_path, G_FILE_TEST_EXISTS))
    eek_png_tag = g_strdup_printf ("<img src=\"%s\">", eek_png_path);

  g_free (eek_png_path);

  /*  the dialog window  */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), _("GIMP Help Browser"));
  gtk_window_set_role (GTK_WINDOW (window), "helpbrowser");

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
                                      "/help-browser-toolbar/online");
  gimp_throbber_set_image (GIMP_THROBBER (button),
                           gtk_image_new_from_pixbuf (pixbuf));

  /*  the horizontal paned  */
  paned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), paned, TRUE, TRUE, 0);
  gtk_widget_show (paned);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_paned_add1 (GTK_PANED (paned), scroll);
  gtk_paned_set_position (GTK_PANED (paned), data.paned_position);
  gtk_widget_show (scroll);

  tree_view = gtk_tree_view_new ();
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);
  gtk_container_add (GTK_CONTAINER (scroll), tree_view);
  gtk_widget_show (tree_view);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view), -1,
                                               NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);

  g_signal_connect (tree_view, "row-activated",
                    G_CALLBACK (row_activated),
                    NULL);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_paned_add2 (GTK_PANED (paned), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  the drag source  */
  drag_source = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), drag_source, FALSE, FALSE, 4);
  gtk_widget_show (drag_source);

  gimp_help_set_help_data (drag_source,
                           _("Drag and drop this icon to a web browser"),
			   NULL);

  gtk_drag_source_set (GTK_WIDGET (drag_source),
                       GDK_BUTTON1_MASK,
                       help_dnd_target_table,
                       G_N_ELEMENTS (help_dnd_target_table),
                       GDK_ACTION_COPY);

  g_signal_connect (drag_source, "drag-begin",
                    G_CALLBACK (drag_begin),
                    NULL);
  g_signal_connect (drag_source, "drag-data-get",
                    G_CALLBACK (drag_data_get),
                    NULL);

  image = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (drag_source), image);
  gtk_widget_show (image);

  /*  the title combo  */
  history = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (history));
  g_object_unref (history);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
                                  "text", HISTORY_TITLE,
                                  NULL);

  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (combo_changed),
                    NULL);

  /*  HTML view  */
  html  = html_view_new ();
  queue = queue_new ();

  gtk_widget_set_size_request (html, 300, 200);

  scroll =
    gtk_scrolled_window_new (gtk_layout_get_hadjustment (GTK_LAYOUT (html)),
                             gtk_layout_get_vadjustment (GTK_LAYOUT (html)));

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);
  gtk_widget_show (scroll);

  gtk_container_add (GTK_CONTAINER (scroll), html);
  gtk_widget_show (html);

  g_signal_connect (html, "realize",
                    G_CALLBACK (view_realize),
                    NULL);
  g_signal_connect (html, "unrealize",
                    G_CALLBACK (view_unrealize),
                    NULL);

  g_signal_connect (html, "popup-menu",
                    G_CALLBACK (view_popup_menu),
                    NULL);
  g_signal_connect (html, "button-press-event",
                    G_CALLBACK (view_button_press),
                    NULL);

  html_view_set_document (HTML_VIEW (html), html_document_new ());

  g_signal_connect (HTML_VIEW (html)->document, "title-changed",
                    G_CALLBACK (title_changed),
                    combo);
  g_signal_connect (HTML_VIEW (html)->document, "link-clicked",
                    G_CALLBACK (link_clicked),
                    NULL);
  g_signal_connect (HTML_VIEW (html)->document, "request-url",
                    G_CALLBACK (request_url),
                    NULL);

  gtk_widget_grab_focus (html);

  g_signal_connect (window, "unmap",
                    G_CALLBACK (dialog_unmap),
                    paned);
}

static gboolean
idle_jump_to_anchor (const gchar *anchor)
{
  if (html && anchor)
    html_view_jump_to_anchor (HTML_VIEW (html), anchor);

  return FALSE;
}

static gboolean
idle_scroll_to_pos (gpointer data)
{
  gint offset = GPOINTER_TO_INT (data);

  if (html)
    {
      GtkAdjustment *adj = gtk_layout_get_vadjustment (GTK_LAYOUT (html));

      gtk_adjustment_set_value (adj,
                                (gdouble) offset / (1 << 16) * adj->upper);
    }

  return FALSE;
}

void
browser_dialog_load (const gchar *uri,
                     gboolean     add_to_queue)
{
  HtmlDocument  *doc = HTML_VIEW (html)->document;
  GtkAdjustment *adj = gtk_layout_get_vadjustment (GTK_LAYOUT (html));
  gchar         *abs;
  gchar         *new_uri;
  gchar         *anchor;
  gchar         *tmp;

  g_return_if_fail (uri != NULL);

  if (! current_uri)
    {
      gchar *slash;

      current_uri = g_strdup (uri);

      slash = strrchr (current_uri, '/');

      if (slash)
        *slash = '\0';
    }

  abs = uri_to_abs (uri, current_uri);
  if (! abs)
    return;

  anchor = strchr (uri, '#');
  if (anchor && anchor[0] && anchor[1])
    {
      new_uri = g_strconcat (abs, anchor, NULL);
      anchor += 1;
    }
  else
    {
      new_uri = g_strdup (abs);
      anchor = NULL;
    }

  if (uri_hash_table)
    browser_dialog_goto_index (new_uri);

  if (! has_case_prefix (abs, "file:/"))
    {
      load_remote_page (uri);
      return;
    }

  tmp = uri_to_abs (current_uri, current_uri);
  if (!tmp || strcmp (tmp, abs))
    {
      GError *error = NULL;

      html_document_clear (doc);
      html_document_open_stream (doc, "text/html");
      gtk_adjustment_set_value (adj, 0.0);

      if (anchor)
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) idle_jump_to_anchor,
                         g_strdup (anchor), (GDestroyNotify) g_free);

      if (request_url (doc, abs, doc->current_stream, &error))
        {
          if (html->window)
            gdk_window_set_cursor (html->window, busy_cursor);
        }
      else
        {
          browser_dialog_404 (doc, abs, error->message);
          g_error_free (error);
        }
    }
  else
    {
      if (anchor)
        html_view_jump_to_anchor (HTML_VIEW (html), anchor);
      else
        gtk_adjustment_set_value (adj, 0.0);
     }

  g_free (tmp);

  g_free (current_uri);
  current_uri = new_uri;

  if (add_to_queue)
    queue_add (queue, new_uri);

  update_toolbar ();

  gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (html)));
}

static void
browser_dialog_make_index_foreach (const gchar    *help_id,
                                   GimpHelpItem   *item,
                                   GimpHelpLocale *locale)
{
  gchar **indices;
  gint    i;

#if 0
  g_printerr ("%s: processing %s (parent %s)\n",
              G_STRFUNC, item->title, item->parent ? item->parent : "NULL");
#endif

  indices = g_strsplit (item->title, ".", -1);

  for (i = 0; i < 5; i++)
    {
      if (! indices[i])
        break;

      item->index += atoi (indices[i]) << (8 * (5 - i));
    }

  g_strfreev (indices);

  if (item->parent && strlen (item->parent))
    {
      GimpHelpItem *parent;

      parent = g_hash_table_lookup (locale->help_id_mapping,
                                    item->parent);

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

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view),
                           GTK_TREE_MODEL (store));
  g_object_unref (store);
}

void
browser_dialog_goto_index (const gchar *ref)
{
  GtkTreeIter *iter;

  iter = g_hash_table_lookup (uri_hash_table, ref);

  if (iter)
    {
      GtkTreeSelection *selection;
      GtkTreeModel     *model;
      GtkTreePath      *path;

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
      model     = gtk_tree_view_get_model (GTK_TREE_VIEW (tree_view));

      path = gtk_tree_model_get_path (model, iter);

      gtk_tree_view_expand_to_path (GTK_TREE_VIEW (tree_view), path);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tree_view), path,
                                    NULL, FALSE, 0.0, 0.0);

      gtk_tree_path_free (path);

      gtk_tree_selection_select_iter (selection, iter);
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
      "index", GTK_STOCK_INDEX,
      NULL, "<alt>Home", N_("Go to the index page"),
      G_CALLBACK (index_callback)
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

  action = gimp_throbber_action_new ("online",
                                     "docs.gimp.org",
                                     _("Visit the GIMP documentation website"),
                                     GIMP_STOCK_WILBER);
  g_signal_connect_closure (action, "activate",
                            g_cclosure_new (G_CALLBACK (online_callback),
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
                                     "    <toolitem action=\"index\" />"
                                     "    <separator name=\"space\" />"
                                     "    <toolitem action=\"online\" />"
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
                                     "    <menuitem action=\"index\" />"
                                     "    <separator />"
                                     "    <menuitem action=\"zoom-in\" />"
                                     "    <menuitem action=\"zoom-out\" />"
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
browser_dialog_404 (HtmlDocument *doc,
                    const gchar  *uri,
                    const gchar  *message)
{
  gchar *msg = g_strdup_printf
    ("<html>"
     "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />"
     "<head><title>%s</title></head>"
     "<body bgcolor=\"white\">"
     "<div align=\"center\">"
     "<div>%s</div>"
     "<h3>%s</h3>"
     "<tt>%s</tt>"
     "<h3>%s</h3>"
     "</div>"
     "</body>"
     "</html>",
     _("Document not found"),
     eek_png_tag,
     _("The requested URL could not be loaded:"),
     uri,
     message);

  html_document_write_stream (doc, msg, strlen (msg));

  g_free (msg);
}

static void
back_callback (GtkAction *action,
               gpointer   data)
{
  gdouble      pos;
  const gchar *uri = queue_prev (queue, GPOINTER_TO_INT (data), &pos);

  if (uri)
    {
      browser_dialog_load (uri, FALSE);

      g_idle_add ((GSourceFunc) idle_scroll_to_pos,
                  GINT_TO_POINTER ((gint) (pos * (1 << 16))));

      queue_move_prev (queue, GPOINTER_TO_INT (data));
    }

  update_toolbar ();
}

static void
forward_callback (GtkAction *action,
                  gpointer   data)
{
  gdouble      pos;
  const gchar *uri = queue_next (queue, GPOINTER_TO_INT (data), &pos);

  if (uri)
    {
      browser_dialog_load (uri, FALSE);

      g_idle_add ((GSourceFunc) idle_scroll_to_pos,
                  GINT_TO_POINTER ((gint) (pos * (1 << 16))));

      queue_move_next (queue, GPOINTER_TO_INT (data));
    }

  update_toolbar ();
}

static void
index_callback (GtkAction *action,
                gpointer   data)
{
  browser_dialog_load ("index.html", TRUE);
}

static void
zoom_in_callback (GtkAction  *action,
                  gpointer  data)
{
  html_view_zoom_in (HTML_VIEW (html));
}

static void
zoom_out_callback (GtkAction *action,
                   gpointer   data)
{
  html_view_zoom_out (HTML_VIEW (html));
}

static void
online_callback (GtkAction *action,
                 gpointer   data)
{
  load_remote_page ("http://docs.gimp.org/");
}

static void
close_callback (GtkAction *action,
                gpointer   data)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (html));
}

static GtkWidget *
build_menu (GList     *list,
            GCallback  callback)
{
  GtkWidget *menu;
  gint       i;

  if (! list)
    return NULL;

  menu = gtk_menu_new ();

  for (i = 0; list && i < 15; list = g_list_next (list), i++)
    {
      GtkWidget *menu_item = gtk_menu_item_new_with_label (list->data);

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);

      g_signal_connect (menu_item, "activate",
                        G_CALLBACK (callback),
                        GINT_TO_POINTER (i));
    }

  g_list_free (list);

  return menu;
}

static void
update_toolbar (void)
{
  GtkAction *action;

  /*  update the back button and its menu  */

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/back");
  gtk_action_set_sensitive (action, queue_has_prev (queue));

  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_prev),
                                 build_menu (queue_list_prev (queue),
                                             G_CALLBACK (back_callback)));

  /*  update the forward button and its menu  */

  action = gtk_ui_manager_get_action (ui_manager,
                                      "/ui/help-browser-popup/forward");
  gtk_action_set_sensitive (action, queue_has_next (queue));

  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button_next),
                                 build_menu (queue_list_next (queue),
                                             G_CALLBACK (forward_callback)));
}

static void
combo_changed (GtkWidget *widget,
               gpointer   data)
{
  GtkComboBox *combo = GTK_COMBO_BOX (widget);
  GtkTreeIter  iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      GValue  value = { 0, };

      gtk_tree_model_get_value (gtk_combo_box_get_model (combo),
                                &iter, HISTORY_URI, &value);

      browser_dialog_load (g_value_get_string (&value), TRUE);

      g_value_unset (&value);
    }
}

static void
drag_begin (GtkWidget      *widget,
            GdkDragContext *context,
            gpointer        data)
{
  gtk_drag_set_icon_stock (context, GIMP_STOCK_WEB, -8, -8);
}

static void
drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *selection_data,
               guint             info,
               guint             time,
               gpointer          data)
{
  if (! current_uri)
    return;

  if (selection_data->target ==
      gdk_atom_intern_static_string ("text/uri-list"))
    {
      gchar *uris[2];

      uris[0] = current_uri;
      uris[1] = NULL;

      gtk_selection_data_set_uris (selection_data, uris);
    }
  else if (selection_data->target ==
           gdk_atom_intern_static_string ("_NETSCAPE_URL"))
    {
      gtk_selection_data_set (selection_data,
                              selection_data->target,
                              8, (guchar *) current_uri, strlen (current_uri));
    }
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

  browser_dialog_load (uri, TRUE);

  g_free (uri);
}

static void
dialog_unmap (GtkWidget *window,
              GtkWidget *paned)
{
  DialogData  data;
  gint        width;
  gint        height;

  gtk_window_get_size (GTK_WINDOW (window), &width, &height);

  data.width          = width;
  data.height         = height;
  data.paned_position = gtk_paned_get_position (GTK_PANED (paned));

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
title_changed (HtmlDocument *doc,
               const gchar  *new_title,
               gpointer      data)
{
  gchar *title = NULL;

  if (new_title)
    {
      gchar *c;

      title = g_strdup (new_title);

      for (c = title; *c; c++)
        {
          if (*c == '\n' || *c == '\r')
            *c = ' ';
        }

      title = g_strstrip (title);
      if (! strlen (title))
        {
          g_free (title);
          title = NULL;
        }
    }

  history_add (GTK_COMBO_BOX (data),
               current_uri, title ? title : _("Untitled"));

  if (title)
    queue_set_title (queue, title);

  g_free (title);
}

static void
link_clicked (HtmlDocument *doc,
              const gchar  *uri,
              gpointer      data)
{
  GtkAdjustment *adj = gtk_layout_get_vadjustment (GTK_LAYOUT (html));

  if (adj->upper > 0.0)
    queue_set_scroll_offset (queue, adj->value / adj->upper);

  browser_dialog_load (uri, TRUE);
}

static gboolean
request_url (HtmlDocument *doc,
             const gchar  *uri,
             HtmlStream   *stream,
             GError      **error)
{
  gchar *abs;
  gchar *filename;

  g_return_val_if_fail (uri != NULL, TRUE);
  g_return_val_if_fail (stream != NULL, TRUE);

  abs = uri_to_abs (uri, current_uri);
  if (! abs)
    return TRUE;

  filename = filename_from_uri (abs);
  g_free (abs);

  if (filename)
    {
      gint fd = g_open (filename, O_RDONLY | _O_BINARY, 0);

      if (fd != -1)
        {
          GIOChannel *io = g_io_channel_unix_new (fd);

          g_io_channel_set_close_on_unref (io, TRUE);
          g_io_channel_set_encoding (io, NULL, NULL);

          g_io_add_watch (io, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                          io_handler, stream);
        }

      g_free (filename);

      if (fd == -1)
        {
          g_set_error (error,
                       G_FILE_ERROR, g_file_error_from_errno (errno),
                       g_strerror (errno));
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
io_handler (GIOChannel   *io,
            GIOCondition  condition,
            gpointer      data)
{
  HtmlStream *stream = data;
  gchar       buffer[8192];
  gsize       bytes;

  if (condition & G_IO_IN)
    {
      if (g_io_channel_read_chars (io, buffer, sizeof (buffer),
                                   &bytes, NULL) != G_IO_STATUS_ERROR
          && bytes > 0)
        {
          html_stream_write (stream, buffer, bytes);
        }
      else
	{
          goto error;
	}

      if (condition & G_IO_HUP)
        {
          while (g_io_channel_read_chars (io, buffer, sizeof (buffer),
                                          &bytes, NULL) != G_IO_STATUS_ERROR
                 && bytes > 0)
            {
              html_stream_write (stream, buffer, bytes);
            }
        }
    }

  if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
    {
    error:
      html_stream_close (stream);
      g_io_channel_unref (io);

      if (html->window)
        gdk_window_set_cursor (html->window, NULL);

      return FALSE;
    }

  return TRUE;
}

static void
load_remote_page (const gchar *uri)
{
  GimpParam *return_vals;
  gint       nreturn_vals;

  /*  try to call the user specified web browser */
  return_vals = gimp_run_procedure ("plug-in-web-browser",
                                    &nreturn_vals,
                                    GIMP_PDB_STRING, uri,
                                    GIMP_PDB_END);
  gimp_destroy_params (return_vals, nreturn_vals);
}

static void
history_add (GtkComboBox *combo,
             const gchar *uri,
	     const gchar *title)
{
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  GtkTreeIter   iter;
  gboolean      iter_valid;
  GValue        value = { 0, };

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gtk_tree_model_get_value (model, &iter, HISTORY_URI, &value);

      if (strcmp (g_value_get_string (&value), uri) == 0)
        {
          gtk_list_store_move_after (GTK_LIST_STORE (model), &iter, NULL);
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  if (! iter_valid)
    {
      gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          HISTORY_TITLE, title,
                          HISTORY_URI,   uri,
                          -1);
    }

  g_signal_handlers_block_by_func (combo, combo_changed, NULL);
  gtk_combo_box_set_active_iter (combo, &iter);
  g_signal_handlers_unblock_by_func (combo, combo_changed, NULL);
}

/* Taken from glib/gconvert.c:
 * Test of haystack has the needle prefix, comparing case
 * insensitive. haystack may be UTF-8, but needle must
 * contain only ascii.
 */
static gboolean
has_case_prefix (const gchar *haystack, const gchar *needle)
{
  const gchar *h = haystack;
  const gchar *n = needle;

  while (*n && *h && g_ascii_tolower (*n) == g_ascii_tolower (*h))
    {
      n++;
      h++;
    }

  return (*n == '\0');
}

static gchar *
filename_from_uri (const gchar *uri)
{
  gchar *filename;
  gchar *hostname;

  g_return_val_if_fail (uri != NULL, NULL);

  filename = g_filename_from_uri (uri, &hostname, NULL);

  if (!filename)
    return NULL;

  if (hostname)
    {
      /*  we have a file: URI with a hostname                           */
#ifdef G_OS_WIN32
      /*  on Win32, create a valid UNC path and use it as the filename  */

      gchar *tmp = g_build_filename ("//", hostname, filename, NULL);

      g_free (filename);
      filename = tmp;
#else
      /*  otherwise return NULL, caller should use URI then             */
      g_free (filename);
      filename = NULL;
#endif

      g_free (hostname);
    }

  return filename;
}
