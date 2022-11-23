/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabrowser.c
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
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

#include "ligmawidgetstypes.h"

#include "ligmawidgets.h"
#include "ligmawidgetsmarshal.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmabrowser
 * @title: LigmaBrowser
 * @short_description: A base class for a documentation browser.
 *
 * A base class for a documentation browser.
 **/


enum
{
  SEARCH,
  LAST_SIGNAL
};


struct _LigmaBrowserPrivate
{
  GtkWidget *left_vbox;

  GtkWidget *search_entry;
  guint      search_timeout_id;

  GtkWidget *search_type_combo;
  gint       search_type;

  GtkWidget *count_label;

  GtkWidget *right_vbox;
  GtkWidget *right_widget;
};

#define GET_PRIVATE(obj) (((LigmaBrowser *) (obj))->priv)


static void      ligma_browser_dispose          (GObject               *object);

static void      ligma_browser_combo_changed    (GtkComboBox           *combo,
                                                LigmaBrowser           *browser);
static void      ligma_browser_entry_changed    (GtkEntry              *entry,
                                                LigmaBrowser           *browser);
static void      ligma_browser_entry_icon_press (GtkEntry              *entry,
                                                GtkEntryIconPosition   icon_pos,
                                                GdkEvent              *event,
                                                LigmaBrowser           *browser);
static gboolean  ligma_browser_search_timeout   (gpointer               data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaBrowser, ligma_browser, GTK_TYPE_PANED)

#define parent_class ligma_browser_parent_class

static guint browser_signals[LAST_SIGNAL] = { 0 };


static void
ligma_browser_class_init (LigmaBrowserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  browser_signals[SEARCH] =
    g_signal_new ("search",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaBrowserClass, search),
                  NULL, NULL,
                  _ligma_widgets_marshal_VOID__STRING_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_INT);

  object_class->dispose = ligma_browser_dispose;

  klass->search         = NULL;
}

static void
ligma_browser_init (LigmaBrowser *browser)
{
  LigmaBrowserPrivate *priv;
  GtkWidget          *hbox;
  GtkWidget          *label;
  GtkWidget          *scrolled_window;
  GtkWidget          *viewport;

  browser->priv = ligma_browser_get_instance_private (browser);

  priv = GET_PRIVATE (browser);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (browser),
                                  GTK_ORIENTATION_HORIZONTAL);

  priv->search_type = -1;

  priv->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_paned_pack1 (GTK_PANED (browser), priv->left_vbox, FALSE, TRUE);
  gtk_widget_show (priv->left_vbox);

  /* search entry */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (priv->left_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  priv->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), priv->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (priv->search_entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->search_entry);

  g_signal_connect (priv->search_entry, "changed",
                    G_CALLBACK (ligma_browser_entry_changed),
                    browser);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (priv->search_entry),
                                     GTK_ENTRY_ICON_SECONDARY, "edit-clear");
  gtk_entry_set_icon_activatable (GTK_ENTRY (priv->search_entry),
                                  GTK_ENTRY_ICON_SECONDARY, TRUE);
  gtk_entry_set_icon_sensitive (GTK_ENTRY (priv->search_entry),
                                GTK_ENTRY_ICON_SECONDARY, FALSE);

  g_signal_connect (priv->search_entry, "icon-press",
                    G_CALLBACK (ligma_browser_entry_icon_press),
                    browser);

  /* count label */

  priv->count_label = gtk_label_new (_("No matches"));
  gtk_label_set_xalign (GTK_LABEL (priv->count_label), 0.0);
  ligma_label_set_attributes (GTK_LABEL (priv->count_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_end (GTK_BOX (priv->left_vbox), priv->count_label,
                    FALSE, FALSE, 0);
  gtk_widget_show (priv->count_label);

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

  priv->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (priv->right_vbox), 12);
  gtk_container_add (GTK_CONTAINER (viewport), priv->right_vbox);
  gtk_widget_show (priv->right_vbox);

  gtk_widget_grab_focus (priv->search_entry);
}

static void
ligma_browser_dispose (GObject *object)
{
  LigmaBrowserPrivate *priv = GET_PRIVATE (object);

  if (priv->search_timeout_id)
    {
      g_source_remove (priv->search_timeout_id);
      priv->search_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */


/**
 * ligma_browser_new:
 *
 * Create a new #LigmaBrowser widget.
 *
 * Returns: a newly created #LigmaBrowser.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_browser_new (void)
{
  return g_object_new (LIGMA_TYPE_BROWSER, NULL);
}

/**
 * ligma_browser_add_search_types: (skip)
 * @browser:          a #LigmaBrowser widget
 * @first_type_label: the label of the first search type
 * @first_type_id:    an integer that identifies the first search type
 * @...:              a %NULL-terminated list of more labels and ids.
 *
 * Populates the #GtkComboBox with search types.
 *
 * Since: 2.4
 **/
void
ligma_browser_add_search_types (LigmaBrowser *browser,
                               const gchar *first_type_label,
                               gint         first_type_id,
                               ...)
{
  LigmaBrowserPrivate *priv;

  g_return_if_fail (LIGMA_IS_BROWSER (browser));
  g_return_if_fail (first_type_label != NULL);

  priv = GET_PRIVATE (browser);

  if (! priv->search_type_combo)
    {
      GtkWidget *combo;
      va_list    args;

      va_start (args, first_type_id);
      combo = ligma_int_combo_box_new_valist (first_type_label,
                                             first_type_id,
                                             args);
      va_end (args);

      gtk_widget_set_focus_on_click (combo, FALSE);

      priv->search_type_combo = combo;
      priv->search_type       = first_type_id;

      gtk_box_pack_end (GTK_BOX (gtk_widget_get_parent (priv->search_entry)),
                        combo, FALSE, FALSE, 0);
      gtk_widget_show (combo);

      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  priv->search_type,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &priv->search_type, NULL);

      g_signal_connect (combo, "changed",
                        G_CALLBACK (ligma_browser_combo_changed),
                        browser);
    }
  else
    {
      ligma_int_combo_box_append (LIGMA_INT_COMBO_BOX (priv->search_type_combo),
                                 first_type_label, first_type_id,
                                 NULL);
    }
}

/**
 * ligma_browser_get_left_vbox:
 * @browser: a #LigmaBrowser widget
 *
 * Returns: (transfer none) (type GtkBox): The left vbox.
 *
 * Since: 3.0
 **/
GtkWidget *
ligma_browser_get_left_vbox (LigmaBrowser *browser)
{
  LigmaBrowserPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_BROWSER (browser), NULL);

  priv = GET_PRIVATE (browser);

  return priv->left_vbox;
}

/**
 * ligma_browser_get_right_vbox:
 * @browser: a #LigmaBrowser widget
 *
 * Returns: (transfer none) (type GtkBox): The right vbox.
 *
 * Since: 3.0
 **/
GtkWidget *
ligma_browser_get_right_vbox (LigmaBrowser *browser)
{
  LigmaBrowserPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_BROWSER (browser), NULL);

  priv = GET_PRIVATE (browser);

  return priv->right_vbox;
}

/**
 * ligma_browser_set_search_summary:
 * @browser: a #LigmaBrowser widget
 * @summary: a string describing the search result
 *
 * Sets the search summary text.
 *
 * Since: 3.0
 **/
void
ligma_browser_set_search_summary (LigmaBrowser *browser,
                                 const gchar *summary)
{
  LigmaBrowserPrivate *priv;

  g_return_if_fail (LIGMA_IS_BROWSER (browser));
  g_return_if_fail (summary != NULL);

  priv = GET_PRIVATE (browser);

  gtk_label_set_text (GTK_LABEL (priv->count_label), summary);
}

/**
 * ligma_browser_set_widget:
 * @browser: a #LigmaBrowser widget
 * @widget:  a #GtkWidget
 *
 * Sets the widget to appear on the right side of the @browser.
 *
 * Since: 2.4
 **/
void
ligma_browser_set_widget (LigmaBrowser *browser,
                         GtkWidget   *widget)
{
  LigmaBrowserPrivate *priv;

  g_return_if_fail (LIGMA_IS_BROWSER (browser));
  g_return_if_fail (widget == NULL || GTK_IS_WIDGET (widget));

  priv = GET_PRIVATE (browser);

  if (widget == priv->right_widget)
    return;

  if (priv->right_widget)
    gtk_container_remove (GTK_CONTAINER (priv->right_vbox),
                          priv->right_widget);

  priv->right_widget = widget;

  if (widget)
    {
      gtk_box_pack_start (GTK_BOX (priv->right_vbox), widget,
                          FALSE, FALSE, 0);
      gtk_widget_show (widget);
    }
}

/**
 * ligma_browser_show_message:
 * @browser: a #LigmaBrowser widget
 * @message: text message
 *
 * Displays @message in the right side of the @browser. Unless the right
 * side already contains a #GtkLabel, the widget previously added with
 * ligma_browser_set_widget() is removed and replaced by a #GtkLabel.
 *
 * Since: 2.4
 **/
void
ligma_browser_show_message (LigmaBrowser *browser,
                           const gchar *message)
{
  LigmaBrowserPrivate *priv;

  g_return_if_fail (LIGMA_IS_BROWSER (browser));
  g_return_if_fail (message != NULL);

  priv = GET_PRIVATE (browser);

  if (GTK_IS_LABEL (priv->right_widget))
    {
      gtk_label_set_text (GTK_LABEL (priv->right_widget), message);
    }
  else
    {
      GtkWidget *label = gtk_label_new (message);

      ligma_label_set_attributes (GTK_LABEL (label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);
      ligma_browser_set_widget (browser, label);
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


/*  private functions  */

static void
ligma_browser_queue_search (LigmaBrowser *browser)
{
  LigmaBrowserPrivate *priv = GET_PRIVATE (browser);

  if (priv->search_timeout_id)
    g_source_remove (priv->search_timeout_id);

  priv->search_timeout_id =
    g_timeout_add (100, ligma_browser_search_timeout, browser);
}

static void
ligma_browser_combo_changed (GtkComboBox *combo,
                            LigmaBrowser *browser)
{
  ligma_browser_queue_search (browser);
}

static void
ligma_browser_entry_changed (GtkEntry    *entry,
                            LigmaBrowser *browser)
{
  ligma_browser_queue_search (browser);

  gtk_entry_set_icon_sensitive (entry,
                                GTK_ENTRY_ICON_SECONDARY,
                                gtk_entry_get_text_length (entry) > 0);
}

static void
ligma_browser_entry_icon_press (GtkEntry              *entry,
                               GtkEntryIconPosition   icon_pos,
                               GdkEvent              *event,
                               LigmaBrowser           *browser)
{
  GdkEventButton *bevent = (GdkEventButton *) event;

  if (icon_pos == GTK_ENTRY_ICON_SECONDARY && bevent->button == 1)
    {
      gtk_entry_set_text (entry, "");
    }
}

static gboolean
ligma_browser_search_timeout (gpointer data)
{
  LigmaBrowserPrivate *priv = GET_PRIVATE (data);
  const gchar        *search_string;

  search_string = gtk_entry_get_text (GTK_ENTRY (priv->search_entry));

  if (! search_string)
    search_string = "";

  g_signal_emit (data, browser_signals[SEARCH], 0,
                 search_string, priv->search_type);

  priv->search_timeout_id = 0;

  return FALSE;
}
