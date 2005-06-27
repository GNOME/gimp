/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppageselector.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpframe.h"
#include "gimppageselector.h"

#include "libgimp/libgimp-intl.h"


enum
{
  SELECTION_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_N_PAGES
};

enum
{
  COLUMN_PAGE_NO,
  COLUMN_THUMBNAIL,
  COLUMN_TEXT
};


static void   gimp_page_selector_class_init   (GimpPageSelectorClass *klass);
static void   gimp_page_selector_init         (GimpPageSelector      *selector);

static void   gimp_page_selector_finalize          (GObject          *object);
static void   gimp_page_selector_get_property      (GObject          *object,
                                                    guint             property_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);
static void   gimp_page_selector_set_property      (GObject          *object,
                                                    guint             property_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);

static void   gimp_page_selector_selection_changed (GtkIconView      *icon_view,
                                                    GimpPageSelector *selector);
static gboolean gimp_page_selector_range_focus_out (GtkEntry         *entry,
                                                    GdkEventFocus    *fevent,
                                                    GimpPageSelector *selector);
static void   gimp_page_selector_range_activate    (GtkEntry         *entry,
                                                    GimpPageSelector *selector);
static void   gimp_page_selector_print_range       (GString          *string,
                                                    gint              start,
                                                    gint              end);


static guint         selector_signals[LAST_SIGNAL] = { 0 };
static GtkVBoxClass *parent_class                  = NULL;


GType
gimp_page_selector_get_type (void)
{
  static GType selector_type = 0;

  if (!selector_type)
    {
      static const GTypeInfo selector_info =
      {
        sizeof (GimpPageSelectorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_page_selector_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPageSelector),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_page_selector_init,
      };

      selector_type = g_type_register_static (GTK_TYPE_VBOX,
                                              "GimpPageSelector",
                                              &selector_info, 0);
    }

  return selector_type;
}

static void
gimp_page_selector_class_init (GimpPageSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  selector_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPageSelectorClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize     = gimp_page_selector_finalize;
  object_class->get_property = gimp_page_selector_get_property;
  object_class->set_property = gimp_page_selector_set_property;

  klass->selection_changed   = NULL;

  /**
   * GimpPageSelector:n_pages:
   *
   * The number of pages of the document to open.
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_N_PAGES,
                                   g_param_spec_int ("n-pages", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_page_selector_init (GimpPageSelector *selector)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *sw;
  GtkWidget *hbox;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *label;

  /* Pages */

  frame = gimp_frame_new (_("Select Pages"));
  gtk_box_pack_start (GTK_BOX (selector), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  selector->store = gtk_list_store_new (3,
                                        G_TYPE_INT,
                                        GDK_TYPE_PIXBUF,
                                        G_TYPE_STRING);

  selector->view =
    gtk_icon_view_new_with_model (GTK_TREE_MODEL (selector->store));
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (selector->view),
                                 COLUMN_TEXT);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (selector->view),
                                   COLUMN_THUMBNAIL);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (selector->view),
                                    GTK_SELECTION_MULTIPLE);
  gtk_container_add (GTK_CONTAINER (sw), selector->view);
  gtk_widget_show (selector->view);

  g_signal_connect (selector->view, "selection-changed",
                    G_CALLBACK (gimp_page_selector_selection_changed),
                    selector);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  hbbox = gtk_hbutton_box_new ();
  gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_mnemonic (_("Select _All"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_page_selector_select_all),
                            selector);

  button = gtk_button_new_with_mnemonic (_("Select _None"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_page_selector_unselect_all),
                            selector);

  selector->range_entry = gtk_entry_new ();
  gtk_widget_set_size_request (selector->range_entry, 80, -1);
  gtk_box_pack_end (GTK_BOX (hbox), selector->range_entry, TRUE, TRUE, 0);
  gtk_widget_show (selector->range_entry);

  g_signal_connect (selector->range_entry, "focus-out-event",
                    G_CALLBACK (gimp_page_selector_range_focus_out),
                    selector);
  g_signal_connect (selector->range_entry, "activate",
                    G_CALLBACK (gimp_page_selector_range_activate),
                    selector);

  label = gtk_label_new_with_mnemonic (_("Select _Range:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), selector->range_entry);

  selector->thumbnail = gtk_widget_render_icon (GTK_WIDGET (selector),
                                                GTK_STOCK_FILE,
                                                GTK_ICON_SIZE_BUTTON,
                                                NULL);
}

static void
gimp_page_selector_finalize (GObject *object)
{
  GimpPageSelector *selector = GIMP_PAGE_SELECTOR (object);

  if (selector->thumbnail)
    g_object_unref (selector->thumbnail);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_page_selector_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPageSelector *selector = GIMP_PAGE_SELECTOR (object);

  switch (property_id)
    {
    case PROP_N_PAGES:
      g_value_set_int (value, selector->n_pages);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_page_selector_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpPageSelector *selector = GIMP_PAGE_SELECTOR (object);

  switch (property_id)
    {
    case PROP_N_PAGES:
      gimp_page_selector_set_n_pages (selector, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * gimp_page_selector_new:
 *
 * Creates a new #GimpPageSelector widget.
 *
 * Returns: Pointer to the new #GimpPageSelector widget.
 **/
GtkWidget *
gimp_page_selector_new (void)
{
  GimpPageSelector *selector;

  selector = g_object_new (GIMP_TYPE_PAGE_SELECTOR, NULL);

  return GTK_WIDGET (selector);
}

/**
 * gimp_page_selector_set_n_pages:
 * @selector: Pointer to a #GimpPageSelector.
 * @n_pages:
 *
 *
 **/
void
gimp_page_selector_set_n_pages (GimpPageSelector *selector,
                                gint              n_pages)
{
  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (n_pages >= 0);

  if (n_pages != selector->n_pages)
    {
      GtkTreeIter iter;
      gint        i;

      if (n_pages < selector->n_pages)
        {
          for (i = n_pages; i < selector->n_pages; i++)
            {
              gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                             &iter, NULL, n_pages);
              gtk_list_store_remove (selector->store, &iter);
            }
        }
      else
        {
          for (i = selector->n_pages; i < n_pages; i++)
            {
              gchar *text;

              text = g_strdup_printf (_("Page %d"), i + 1);

              gtk_list_store_append (selector->store, &iter);
              gtk_list_store_set (selector->store, &iter,
                                  COLUMN_PAGE_NO,   i,
                                  COLUMN_THUMBNAIL, selector->thumbnail,
                                  COLUMN_TEXT,      text,
                                  -1);

              g_free (text);
            }
        }

      selector->n_pages = n_pages;

      g_object_notify (G_OBJECT (selector), "n-pages");
    }
}

/**
 * gimp_page_selector_get_n_pages:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Retrieves the number of pages in the document to open.
 **/
gint
gimp_page_selector_get_n_pages (GimpPageSelector *selector)
{
  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), 0);

  return selector->n_pages;
}

/**
 * gimp_page_selector_set_page_thumbnail:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to set the thumbnail for.
 * @thumbnail: The thumbnail pixbuf.
 *
 **/
void
gimp_page_selector_set_page_thumbnail (GimpPageSelector *selector,
                                       gint              page_no,
                                       GdkPixbuf        *thumbnail)
{
  GtkTreeIter iter;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (page_no >= 0 && page_no < selector->n_pages);
  g_return_if_fail (thumbnail == NULL || GDK_IS_PIXBUF (thumbnail));

  if (! thumbnail)
    thumbnail = selector->thumbnail;

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                 &iter, NULL, page_no);
  gtk_list_store_set (selector->store, &iter,
                      COLUMN_THUMBNAIL, thumbnail,
                      -1);
}

/**
 * gimp_page_selector_get_page_thumbnail:
 * @page_no: The number of the page to get the thumbnail for.
 *
 * Returns: The page's thumbnail, or %NULL if none is set.
 **/
GdkPixbuf *
gimp_page_selector_get_page_thumbnail (GimpPageSelector   *selector,
                                       gint                page_no)
{
  GdkPixbuf   *thumbnail;
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);
  g_return_val_if_fail (page_no >= 0 && page_no < selector->n_pages, NULL);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                 &iter, NULL, page_no);
  gtk_tree_model_get (GTK_TREE_MODEL (selector->store), &iter,
                      COLUMN_THUMBNAIL, &thumbnail,
                      -1);

  if (thumbnail)
    g_object_unref (thumbnail);

  if (thumbnail == selector->thumbnail)
    return NULL;

  return thumbnail;
}

void
gimp_page_selector_select_all (GimpPageSelector *selector)
{
  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  gtk_icon_view_select_all (GTK_ICON_VIEW (selector->view));
}

void
gimp_page_selector_unselect_all (GimpPageSelector *selector)
{
  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  gtk_icon_view_unselect_all (GTK_ICON_VIEW (selector->view));
}

void
gimp_page_selector_select_page (GimpPageSelector *selector,
                                gint              page_no)
{
  GtkTreeIter  iter;
  GtkTreePath *path;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (page_no >= 0 && page_no < selector->n_pages);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (selector->store), &iter);

  gtk_icon_view_select_path (GTK_ICON_VIEW (selector->view), path);

  gtk_tree_path_free (path);
}

void
gimp_page_selector_unselect_page (GimpPageSelector *selector,
                                  gint              page_no)
{
  GtkTreeIter  iter;
  GtkTreePath *path;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (page_no >= 0 && page_no < selector->n_pages);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (selector->store), &iter);

  gtk_icon_view_unselect_path (GTK_ICON_VIEW (selector->view), path);

  gtk_tree_path_free (path);
}

gboolean
gimp_page_selector_page_is_selected (GimpPageSelector *selector,
                                     gint              page_no)
{
  GtkTreeIter  iter;
  GtkTreePath *path;
  gboolean     selected;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), FALSE);
  g_return_val_if_fail (page_no >= 0 && page_no < selector->n_pages, FALSE);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (selector->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (selector->store), &iter);

  selected = gtk_icon_view_path_is_selected (GTK_ICON_VIEW (selector->view),
                                             path);

  gtk_tree_path_free (path);

  return selected;
}

gint
gimp_page_selector_int_compare (gconstpointer a,
                                gconstpointer b)
{
  return *(gint*)a - *(gint*)b;
}

gint *
gimp_page_selector_get_selected_pages (GimpPageSelector *selector,
                                       gint             *n_selected_pages)
{
  GList *selected;
  GList *list;
  gint  *array;
  gint   i;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);
  g_return_val_if_fail (n_selected_pages != NULL, NULL);

  selected = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (selector->view));

  *n_selected_pages = g_list_length (selected);
  array = g_new0 (gint, *n_selected_pages);

  for (list = selected, i = 0; list; list = g_list_next (list), i++)
    {
      gint *indices = gtk_tree_path_get_indices (list->data);

      array[i] = indices[0];
    }

  qsort (array, *n_selected_pages, sizeof (gint),
         gimp_page_selector_int_compare);

  g_list_foreach (selected, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (selected);

  return array;
}

void
gimp_page_selector_select_range (GimpPageSelector *selector,
                                 const gchar      *range_string)
{
  gchar **ranges;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  if (! range_string)
    range_string = "";

  g_signal_handlers_block_by_func (selector->view,
                                   gimp_page_selector_selection_changed,
                                   selector);

  gimp_page_selector_unselect_all (selector);

  ranges = g_strsplit (range_string, ",", -1);

  if (ranges)
    {
      gint i;

      for (i = 0; ranges[i] != NULL; i++)
        {
          gchar *range = g_strstrip (ranges[i]);
          gchar *dash;

          dash = strchr (range, '-');

          if (dash)
            {
              gint page_from;
              gint page_to;

              if (sscanf (range,    "%i", &page_from) == 1 &&
                  sscanf (dash + 1, "%i", &page_to)   == 1 &&
                  page_from <= page_to                     &&
                  page_from <= selector->n_pages)
                {
                  gint page_no;

                  page_from = MAX (page_from, 1) - 1;
                  page_to   = MIN (page_to, selector->n_pages) - 1;

                  for (page_no = page_from; page_no <= page_to; page_no++)
                    gimp_page_selector_select_page (selector, page_no);
                }
            }
          else
            {
              gint page_no;

              if (sscanf (range, "%i", &page_no) == 1 &&
                  page_no >= 1                        &&
                  page_no <= selector->n_pages)
                {
                  gimp_page_selector_select_page (selector, page_no - 1);
                }
            }
        }

      g_strfreev (ranges);
    }

  g_signal_handlers_unblock_by_func (selector->view,
                                     gimp_page_selector_selection_changed,
                                     selector);

  gimp_page_selector_selection_changed (GTK_ICON_VIEW (selector->view),
                                        selector);
}

gchar *
gimp_page_selector_get_selected_range (GimpPageSelector *selector)
{
  gint    *pages;
  gint     n_pages;
  GString *string;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);

  string = g_string_new ("");

  pages = gimp_page_selector_get_selected_pages (selector, &n_pages);

  if (pages)
    {
      gint range_start, range_end;
      gint last_printed;
      gint i;

      range_start  = pages[0];
      range_end    = pages[0];
      last_printed = -1;

      for (i = 1; i < n_pages; i++)
        {
          if (pages[i] > range_end + 1)
            {
              gimp_page_selector_print_range (string,
                                              range_start, range_end);

              last_printed = range_end;
              range_start = pages[i];
            }

          range_end = pages[i];
        }

      if (range_end != last_printed)
        gimp_page_selector_print_range (string, range_start, range_end);

      g_free (pages);
    }

  return g_string_free (string, FALSE);
}

/*  private functions  */

static void
gimp_page_selector_selection_changed (GtkIconView      *icon_view,
                                      GimpPageSelector *selector)
{
  gchar *range;

  range = gimp_page_selector_get_selected_range (selector);
  gtk_entry_set_text (GTK_ENTRY (selector->range_entry), range);
  g_free (range);

  gtk_editable_set_position (GTK_EDITABLE (selector->range_entry), -1);

  g_signal_emit (selector, selector_signals[SELECTION_CHANGED], 0);
}

static gboolean
gimp_page_selector_range_focus_out (GtkEntry         *entry,
                                    GdkEventFocus    *fevent,
                                    GimpPageSelector *selector)
{
  gimp_page_selector_range_activate (entry, selector);

  return FALSE;
}

static void
gimp_page_selector_range_activate (GtkEntry         *entry,
                                   GimpPageSelector *selector)
{
  gimp_page_selector_select_range (selector, gtk_entry_get_text (entry));
}

static void
gimp_page_selector_print_range (GString *string,
                                gint     start,
                                gint     end)
{
  if (string->len != 0)
    g_string_append_c (string, ',');

  if (start == end)
    g_string_append_printf (string, "%d", start + 1);
  else
    g_string_append_printf (string, "%d-%d", start + 1, end + 1);
}
