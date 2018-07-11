/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrowser.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpwidgets.h"
#include "gimpwidgetsmarshal.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpbrowser
 * @title: GimpBrowser
 * @short_description: A base class for a documentation browser.
 *
 * A base class for a documentation browser.
 **/


enum
{
  SEARCH,
  LAST_SIGNAL
};


static void      gimp_browser_dispose          (GObject               *object);

static void      gimp_browser_combo_changed    (GtkComboBox           *combo,
                                                GimpBrowser           *browser);
static void      gimp_browser_entry_changed    (GtkEntry              *entry,
                                                GimpBrowser           *browser);
static void      gimp_browser_entry_icon_press (GtkEntry              *entry,
                                                GtkEntryIconPosition   icon_pos,
                                                GdkEvent              *event,
                                                GimpBrowser           *browser);
static gboolean  gimp_browser_search_timeout   (gpointer               data);


G_DEFINE_TYPE (GimpBrowser, gimp_browser, GTK_TYPE_HPANED)

#define parent_class gimp_browser_parent_class

static guint browser_signals[LAST_SIGNAL] = { 0 };


static void
gimp_browser_class_init (GimpBrowserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  browser_signals[SEARCH] =
    g_signal_new ("search",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpBrowserClass, search),
                  NULL, NULL,
                  _gimp_widgets_marshal_VOID__STRING_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_INT);

  object_class->dispose = gimp_browser_dispose;

  klass->search         = NULL;
}

static void
gimp_browser_init (GimpBrowser *browser)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *scrolled_window;
  GtkWidget *viewport;

  browser->search_type = -1;

  browser->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_paned_pack1 (GTK_PANED (browser), browser->left_vbox, FALSE, TRUE);
  gtk_widget_show (browser->left_vbox);

  /* search entry */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (browser->left_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  browser->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), browser->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (browser->search_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), browser->search_entry);

  g_signal_connect (browser->search_entry, "changed",
                    G_CALLBACK (gimp_browser_entry_changed),
                    browser);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (browser->search_entry),
                                     GTK_ENTRY_ICON_SECONDARY, "edit-clear");
  gtk_entry_set_icon_activatable (GTK_ENTRY (browser->search_entry),
                                  GTK_ENTRY_ICON_SECONDARY, TRUE);
  gtk_entry_set_icon_sensitive (GTK_ENTRY (browser->search_entry),
                                GTK_ENTRY_ICON_SECONDARY, FALSE);

  g_signal_connect (browser->search_entry, "icon-press",
                    G_CALLBACK (gimp_browser_entry_icon_press),
                    browser);

  /* count label */

  browser->count_label = gtk_label_new (_("No matches"));
  gtk_label_set_xalign (GTK_LABEL (browser->count_label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (browser->count_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_end (GTK_BOX (browser->left_vbox), browser->count_label,
                    FALSE, FALSE, 0);
  gtk_widget_show (browser->count_label);

  /* scrolled window */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_paned_pack2 (GTK_PANED (browser), scrolled_window, TRUE, TRUE);
  gtk_widget_show (scrolled_window);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_show (viewport);

  browser->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (browser->right_vbox), 12);
  gtk_container_add (GTK_CONTAINER (viewport), browser->right_vbox);
  gtk_widget_show (browser->right_vbox);

  gtk_widget_grab_focus (browser->search_entry);
}

static void
gimp_browser_dispose (GObject *object)
{
  GimpBrowser *browser = GIMP_BROWSER (object);

  if (browser->search_timeout_id)
    {
      g_source_remove (browser->search_timeout_id);
      browser->search_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */


/**
 * gimp_browser_new:
 *
 * Create a new #GimpBrowser widget.
 *
 * Return Value: a newly created #GimpBrowser.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_browser_new (void)
{
  return g_object_new (GIMP_TYPE_BROWSER, NULL);
}

/**
 * gimp_browser_add_search_types:
 * @browser:          a #GimpBrowser widget
 * @first_type_label: the label of the first search type
 * @first_type_id:    an integer that identifies the first search type
 * @...:              a %NULL-terminated list of more labels and ids.
 *
 * Populates the #GtkComboBox with search types.
 *
 * Since: 2.4
 **/
void
gimp_browser_add_search_types (GimpBrowser *browser,
                               const gchar *first_type_label,
                               gint         first_type_id,
                               ...)
{
  g_return_if_fail (GIMP_IS_BROWSER (browser));
  g_return_if_fail (first_type_label != NULL);

  if (! browser->search_type_combo)
    {
      GtkWidget *combo;
      va_list    args;

      va_start (args, first_type_id);
      combo = gimp_int_combo_box_new_valist (first_type_label,
                                             first_type_id,
                                             args);
      va_end (args);

      gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (combo), FALSE);

      browser->search_type_combo = combo;
      browser->search_type       = first_type_id;

      gtk_box_pack_end (GTK_BOX (gtk_widget_get_parent (browser->search_entry)),
                        combo, FALSE, FALSE, 0);
      gtk_widget_show (combo);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  browser->search_type,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &browser->search_type);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (gimp_browser_combo_changed),
                        browser);
    }
  else
    {
      gimp_int_combo_box_append (GIMP_INT_COMBO_BOX (browser->search_type_combo),
                                 first_type_label, first_type_id,
                                 NULL);
    }
}

/**
 * gimp_browser_set_widget:
 * @browser: a #GimpBrowser widget
 * @widget:  a #GtkWidget
 *
 * Sets the widget to appear on the right side of the @browser.
 *
 * Since: 2.4
 **/
void
gimp_browser_set_widget (GimpBrowser *browser,
                         GtkWidget   *widget)
{
  g_return_if_fail (GIMP_IS_BROWSER (browser));
  g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

  if (widget == browser->right_widget)
    return;

  if (browser->right_widget)
    gtk_container_remove (GTK_CONTAINER (browser->right_vbox),
                          browser->right_widget);

  browser->right_widget = widget;

  if (widget)
    {
      gtk_box_pack_start (GTK_BOX (browser->right_vbox), widget,
                          FALSE, FALSE, 0);
      gtk_widget_show (widget);
    }
}

/**
 * gimp_browser_show_message:
 * @browser: a #GimpBrowser widget
 * @message: text message
 *
 * Displays @message in the right side of the @browser. Unless the right
 * side already contains a #GtkLabel, the widget previously added with
 * gimp_browser_set_widget() is removed and replaced by a #GtkLabel.
 *
 * Since: 2.4
 **/
void
gimp_browser_show_message (GimpBrowser *browser,
                           const gchar *message)
{
  g_return_if_fail (GIMP_IS_BROWSER (browser));
  g_return_if_fail (message != NULL);

  if (GTK_IS_LABEL (browser->right_widget))
    {
      gtk_label_set_text (GTK_LABEL (browser->right_widget), message);
    }
  else
    {
      GtkWidget *label = gtk_label_new (message);

      gimp_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      gimp_browser_set_widget (browser, label);
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

static void
gimp_browser_queue_search (GimpBrowser *browser)
{
  if (browser->search_timeout_id)
    g_source_remove (browser->search_timeout_id);

  browser->search_timeout_id =
    g_timeout_add (100, gimp_browser_search_timeout, browser);
}

static void
gimp_browser_combo_changed (GtkComboBox *combo,
                            GimpBrowser *browser)
{
  gimp_browser_queue_search (browser);
}

static void
gimp_browser_entry_changed (GtkEntry    *entry,
                            GimpBrowser *browser)
{
  gimp_browser_queue_search (browser);

  gtk_entry_set_icon_sensitive (entry,
                                GTK_ENTRY_ICON_SECONDARY,
                                gtk_entry_get_text_length (entry) > 0);
}

static void
gimp_browser_entry_icon_press (GtkEntry              *entry,
                               GtkEntryIconPosition   icon_pos,
                               GdkEvent              *event,
                               GimpBrowser           *browser)
{
  GdkEventButton *bevent = (GdkEventButton *) event;

  if (icon_pos == GTK_ENTRY_ICON_SECONDARY && bevent->button == 1)
    {
      gtk_entry_set_text (entry, "");
    }
}

static gboolean
gimp_browser_search_timeout (gpointer data)
{
  GimpBrowser *browser = GIMP_BROWSER (data);
  const gchar *search_string;

  GDK_THREADS_ENTER();

  search_string = gtk_entry_get_text (GTK_ENTRY (browser->search_entry));

  if (! search_string)
    search_string = "";

  g_signal_emit (browser, browser_signals[SEARCH], 0,
                 search_string, browser->search_type);

  browser->search_timeout_id = 0;

  GDK_THREADS_LEAVE();

  return FALSE;
}
