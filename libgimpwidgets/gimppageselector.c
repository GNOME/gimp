/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppageselector.c
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpicons.h"
#include "gimppageselector.h"
#include "gimppropwidgets.h"
#include "gimpwidgets.h"
#include "gimp3migration.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimppageselector
 * @title: GimpPageSelector
 * @short_description: A widget to select pages from multi-page things.
 *
 * Use this for example for specifying what pages to import from
 * a PDF or PS document.
 **/


enum
{
  SELECTION_CHANGED,
  ACTIVATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_N_PAGES,
  PROP_TARGET
};

enum
{
  COLUMN_PAGE_NO,
  COLUMN_THUMBNAIL,
  COLUMN_LABEL,
  COLUMN_LABEL_SET
};


typedef struct
{
  gint                    n_pages;
  GimpPageSelectorTarget  target;

  GtkListStore           *store;
  GtkWidget              *view;

  GtkWidget              *count_label;
  GtkWidget              *range_entry;

  GdkPixbuf              *default_thumbnail;
} GimpPageSelectorPrivate;

#define GIMP_PAGE_SELECTOR_GET_PRIVATE(obj) \
  ((GimpPageSelectorPrivate *) ((GimpPageSelector *) (obj))->priv)


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
static void   gimp_page_selector_item_activated    (GtkIconView      *icon_view,
                                                    GtkTreePath      *path,
                                                    GimpPageSelector *selector);
static gboolean gimp_page_selector_range_focus_out (GtkEntry         *entry,
                                                    GdkEventFocus    *fevent,
                                                    GimpPageSelector *selector);
static void   gimp_page_selector_range_activate    (GtkEntry         *entry,
                                                    GimpPageSelector *selector);
static gint   gimp_page_selector_int_compare       (gconstpointer     a,
                                                    gconstpointer     b);
static void   gimp_page_selector_print_range       (GString          *string,
                                                    gint              start,
                                                    gint              end);

static GdkPixbuf * gimp_page_selector_add_frame    (GtkWidget        *widget,
                                                    GdkPixbuf        *pixbuf);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPageSelector, gimp_page_selector, GTK_TYPE_BOX)

#define parent_class gimp_page_selector_parent_class

static guint selector_signals[LAST_SIGNAL] = { 0 };


static void
gimp_page_selector_class_init (GimpPageSelectorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize     = gimp_page_selector_finalize;
  object_class->get_property = gimp_page_selector_get_property;
  object_class->set_property = gimp_page_selector_set_property;

  klass->selection_changed   = NULL;
  klass->activate            = NULL;

  /**
   * GimpPageSelector::selection-changed:
   * @widget: the object which received the signal.
   *
   * This signal is emitted whenever the set of selected pages changes.
   *
   * Since: 2.4
   **/
  selector_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPageSelectorClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * GimpPageSelector::activate:
   * @widget: the object which received the signal.
   *
   * The "activate" signal on GimpPageSelector is an action signal. It
   * is emitted when a user double-clicks an item in the page selection.
   *
   * Since: 2.4
   */
  selector_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpPageSelectorClass, activate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  widget_class->activate_signal = selector_signals[ACTIVATE];

  /**
   * GimpPageSelector:n-pages:
   *
   * The number of pages of the document to open.
   *
   * Since: 2.4
   **/
  g_object_class_install_property (object_class, PROP_N_PAGES,
                                   g_param_spec_int ("n-pages",
                                                     "N Pages",
                                                     "The number of pages to open",
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READWRITE));

  /**
   * GimpPageSelector:target:
   *
   * The target to open the document to.
   *
   * Since: 2.4
   **/
  g_object_class_install_property (object_class, PROP_TARGET,
                                   g_param_spec_enum ("target",
                                                      "Target",
                                                      "the target to open to",
                                                      GIMP_TYPE_PAGE_SELECTOR_TARGET,
                                                      GIMP_PAGE_SELECTOR_TARGET_LAYERS,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_page_selector_init (GimpPageSelector *selector)
{
  GimpPageSelectorPrivate *priv;
  GtkWidget               *vbox;
  GtkWidget               *sw;
  GtkWidget               *hbox;
  GtkWidget               *hbbox;
  GtkWidget               *button;
  GtkWidget               *label;
  GtkWidget               *combo;

  selector->priv = gimp_page_selector_get_instance_private (selector);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  priv->n_pages = 0;
  priv->target  = GIMP_PAGE_SELECTOR_TARGET_LAYERS;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selector),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (selector), 12);

  /*  Pages  */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (selector), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  priv->store = gtk_list_store_new (4,
                                    G_TYPE_INT,
                                    GDK_TYPE_PIXBUF,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN);

  priv->view = gtk_icon_view_new_with_model (GTK_TREE_MODEL (priv->store));
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (priv->view),
                                 COLUMN_LABEL);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (priv->view),
                                   COLUMN_THUMBNAIL);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (priv->view),
                                    GTK_SELECTION_MULTIPLE);
  gtk_container_add (GTK_CONTAINER (sw), priv->view);
  gtk_widget_show (priv->view);

  g_signal_connect (priv->view, "selection-changed",
                    G_CALLBACK (gimp_page_selector_selection_changed),
                    selector);
  g_signal_connect (priv->view, "item-activated",
                    G_CALLBACK (gimp_page_selector_item_activated),
                    selector);

  /*  Count label  */

  priv->count_label = gtk_label_new (_("Nothing selected"));
  gtk_label_set_xalign (GTK_LABEL (priv->count_label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (priv->count_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), priv->count_label, FALSE, FALSE, 0);
  gtk_widget_show (priv->count_label);

  /*  Select all button & range entry  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selector), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  hbbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_mnemonic (_("Select _All"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_page_selector_select_all),
                            selector);

  priv->range_entry = gtk_entry_new ();
  gtk_widget_set_size_request (priv->range_entry, 80, -1);
  gtk_box_pack_end (GTK_BOX (hbox), priv->range_entry, TRUE, TRUE, 0);
  gtk_widget_show (priv->range_entry);

  g_signal_connect (priv->range_entry, "focus-out-event",
                    G_CALLBACK (gimp_page_selector_range_focus_out),
                    selector);
  g_signal_connect (priv->range_entry, "activate",
                    G_CALLBACK (gimp_page_selector_range_activate),
                    selector);

  label = gtk_label_new_with_mnemonic (_("Select _range:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->range_entry);

  /*  Target combo  */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selector), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Open _pages as"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (selector), "target", -1, -1);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  priv->default_thumbnail =
    gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                              "text-x-generic", 32, 0, NULL);
}

static void
gimp_page_selector_finalize (GObject *object)
{
  GimpPageSelectorPrivate *priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (object);

  g_clear_object (&priv->default_thumbnail);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_page_selector_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPageSelectorPrivate *priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_N_PAGES:
      g_value_set_int (value, priv->n_pages);
      break;
    case PROP_TARGET:
      g_value_set_enum (value, priv->target);
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
  GimpPageSelector        *selector = GIMP_PAGE_SELECTOR (object);
  GimpPageSelectorPrivate *priv     = GIMP_PAGE_SELECTOR_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_N_PAGES:
      gimp_page_selector_set_n_pages (selector, g_value_get_int (value));
      break;
    case PROP_TARGET:
      priv->target = g_value_get_enum (value);
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
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_page_selector_new (void)
{
  return g_object_new (GIMP_TYPE_PAGE_SELECTOR, NULL);
}

/**
 * gimp_page_selector_set_n_pages:
 * @selector: Pointer to a #GimpPageSelector.
 * @n_pages:  The number of pages.
 *
 * Sets the number of pages in the document to open.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_set_n_pages (GimpPageSelector *selector,
                                gint              n_pages)
{
  GimpPageSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (n_pages >= 0);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  if (n_pages != priv->n_pages)
    {
      GtkTreeIter iter;
      gint        i;

      if (n_pages < priv->n_pages)
        {
          for (i = n_pages; i < priv->n_pages; i++)
            {
              gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                             &iter, NULL, n_pages);
              gtk_list_store_remove (priv->store, &iter);
            }
        }
      else
        {
          for (i = priv->n_pages; i < n_pages; i++)
            {
              gchar *text;

              text = g_strdup_printf (_("Page %d"), i + 1);

              gtk_list_store_append (priv->store, &iter);
              gtk_list_store_set (priv->store, &iter,
                                  COLUMN_PAGE_NO,   i,
                                  COLUMN_THUMBNAIL, priv->default_thumbnail,
                                  COLUMN_LABEL,     text,
                                  COLUMN_LABEL_SET, FALSE,
                                  -1);

              g_free (text);
            }
        }

      priv->n_pages = n_pages;

      g_object_notify (G_OBJECT (selector), "n-pages");
    }
}

/**
 * gimp_page_selector_get_n_pages:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Returns: the number of pages in the document to open.
 *
 * Since: 2.4
 **/
gint
gimp_page_selector_get_n_pages (GimpPageSelector *selector)
{
  GimpPageSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), 0);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  return priv->n_pages;
}

/**
 * gimp_page_selector_set_target:
 * @selector: Pointer to a #GimpPageSelector.
 * @target:   How to open the selected pages.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_set_target (GimpPageSelector       *selector,
                               GimpPageSelectorTarget  target)
{
  GimpPageSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (target <= GIMP_PAGE_SELECTOR_TARGET_IMAGES);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  if (target != priv->target)
    {
      priv->target = target;

      g_object_notify (G_OBJECT (selector), "target");
    }
}

/**
 * gimp_page_selector_get_target:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Returns: How the selected pages should be opened.
 *
 * Since: 2.4
 **/
GimpPageSelectorTarget
gimp_page_selector_get_target (GimpPageSelector *selector)
{
  GimpPageSelectorPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector),
                        GIMP_PAGE_SELECTOR_TARGET_LAYERS);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  return priv->target;
}

/**
 * gimp_page_selector_set_page_thumbnail:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to set the thumbnail for.
 * @thumbnail: The thumbnail pixbuf.
 *
 * Sets the thumbnail for given @page_no. A default "page" icon will
 * be used if no page thumbnail is set.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_set_page_thumbnail (GimpPageSelector *selector,
                                       gint              page_no,
                                       GdkPixbuf        *thumbnail)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));
  g_return_if_fail (thumbnail == NULL || GDK_IS_PIXBUF (thumbnail));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_if_fail (page_no >= 0 && page_no < priv->n_pages);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);

  if (! thumbnail)
    {
      thumbnail = g_object_ref (priv->default_thumbnail);
    }
  else
    {
      thumbnail = gimp_page_selector_add_frame (GTK_WIDGET (selector),
                                                thumbnail);
    }

  gtk_list_store_set (priv->store, &iter,
                      COLUMN_THUMBNAIL, thumbnail,
                      -1);
  g_clear_object (&thumbnail);
}

/**
 * gimp_page_selector_get_page_thumbnail:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to get the thumbnail for.
 *
 * Returns: The page's thumbnail, or %NULL if none is set. The returned
 *          pixbuf is owned by #GimpPageSelector and must not be
 *          unref'ed when no longer needed.
 *
 * Since: 2.4
 **/
GdkPixbuf *
gimp_page_selector_get_page_thumbnail (GimpPageSelector *selector,
                                       gint              page_no)
{
  GimpPageSelectorPrivate *priv;
  GdkPixbuf               *thumbnail;
  GtkTreeIter              iter;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_val_if_fail (page_no >= 0 && page_no < priv->n_pages, NULL);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                      COLUMN_THUMBNAIL, &thumbnail,
                      -1);

  if (thumbnail)
    g_object_unref (thumbnail);

  if (thumbnail == priv->default_thumbnail)
    return NULL;

  return thumbnail;
}

/**
 * gimp_page_selector_set_page_label:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no:  The number of the page to set the label for.
 * @label:    The label.
 *
 * Sets the label of the specified page.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_set_page_label (GimpPageSelector *selector,
                                   gint              page_no,
                                   const gchar      *label)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;
  gchar                   *tmp;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_if_fail (page_no >= 0 && page_no < priv->n_pages);

  if (! label)
    tmp = g_strdup_printf (_("Page %d"), page_no + 1);
  else
    tmp = (gchar *) label;

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  gtk_list_store_set (priv->store, &iter,
                      COLUMN_LABEL,     tmp,
                      COLUMN_LABEL_SET, label != NULL,
                      -1);

  if (! label)
    g_free (tmp);
}

/**
 * gimp_page_selector_get_page_label:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to get the thumbnail for.
 *
 * Returns: The page's label, or %NULL if none is set. This is a newly
 *          allocated string that should be g_free()'d when no longer
 *          needed.
 *
 * Since: 2.4
 **/
gchar *
gimp_page_selector_get_page_label (GimpPageSelector *selector,
                                   gint              page_no)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;
  gchar                   *label;
  gboolean                 label_set;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_val_if_fail (page_no >= 0 && page_no < priv->n_pages, NULL);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                      COLUMN_LABEL,     &label,
                      COLUMN_LABEL_SET, &label_set,
                      -1);

  if (! label_set)
    {
      g_free (label);
      label = NULL;
    }

  return label;
}

/**
 * gimp_page_selector_select_all:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Selects all pages.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_select_all (GimpPageSelector *selector)
{
  GimpPageSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  gtk_icon_view_select_all (GTK_ICON_VIEW (priv->view));
}

/**
 * gimp_page_selector_unselect_all:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Unselects all pages.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_unselect_all (GimpPageSelector *selector)
{
  GimpPageSelectorPrivate *priv;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  gtk_icon_view_unselect_all (GTK_ICON_VIEW (priv->view));
}

/**
 * gimp_page_selector_select_page:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to select.
 *
 * Adds a page to the selection.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_select_page (GimpPageSelector *selector,
                                gint              page_no)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;
  GtkTreePath             *path;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_if_fail (page_no >= 0 && page_no < priv->n_pages);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);

  gtk_icon_view_select_path (GTK_ICON_VIEW (priv->view), path);

  gtk_tree_path_free (path);
}

/**
 * gimp_page_selector_unselect_page:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to unselect.
 *
 * Removes a page from the selection.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_unselect_page (GimpPageSelector *selector,
                                  gint              page_no)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;
  GtkTreePath             *path;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_if_fail (page_no >= 0 && page_no < priv->n_pages);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);

  gtk_icon_view_unselect_path (GTK_ICON_VIEW (priv->view), path);

  gtk_tree_path_free (path);
}

/**
 * gimp_page_selector_page_is_selected:
 * @selector: Pointer to a #GimpPageSelector.
 * @page_no: The number of the page to check.
 *
 * Returns: %TRUE if the page is selected, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
gimp_page_selector_page_is_selected (GimpPageSelector *selector,
                                     gint              page_no)
{
  GimpPageSelectorPrivate *priv;
  GtkTreeIter              iter;
  GtkTreePath             *path;
  gboolean                 selected;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), FALSE);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  g_return_val_if_fail (page_no >= 0 && page_no < priv->n_pages, FALSE);

  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (priv->store),
                                 &iter, NULL, page_no);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);

  selected = gtk_icon_view_path_is_selected (GTK_ICON_VIEW (priv->view),
                                             path);

  gtk_tree_path_free (path);

  return selected;
}

/**
 * gimp_page_selector_get_selected_pages:
 * @selector: Pointer to a #GimpPageSelector.
 * @n_selected_pages: Returns the number of selected pages.
 *
 * Returns: A sorted array of page numbers of selected pages. Use g_free() if
 *          you don't need the array any longer.
 *
 * Since: 2.4
 **/
gint *
gimp_page_selector_get_selected_pages (GimpPageSelector *selector,
                                       gint             *n_selected_pages)
{
  GimpPageSelectorPrivate *priv;
  GList                   *selected;
  GList                   *list;
  gint                    *array;
  gint                     i;

  g_return_val_if_fail (GIMP_IS_PAGE_SELECTOR (selector), NULL);
  g_return_val_if_fail (n_selected_pages != NULL, NULL);

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  selected = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->view));

  *n_selected_pages = g_list_length (selected);
  array = g_new0 (gint, *n_selected_pages);

  for (list = selected, i = 0; list; list = g_list_next (list), i++)
    {
      gint *indices = gtk_tree_path_get_indices (list->data);

      array[i] = indices[0];
    }

  qsort (array, *n_selected_pages, sizeof (gint),
         gimp_page_selector_int_compare);

  g_list_free_full (selected, (GDestroyNotify) gtk_tree_path_free);

  return array;
}

/**
 * gimp_page_selector_select_range:
 * @selector: Pointer to a #GimpPageSelector.
 * @range: A string representing the set of selected pages.
 *
 * Selects the pages described by @range. The range string is a
 * user-editable list of pages and ranges, e.g. "1,3,5-7,9-12,14".
 * Note that the page numbering in the range string starts with 1,
 * not 0.
 *
 * Invalid pages and ranges will be silently ignored, duplicate and
 * overlapping pages and ranges will be merged.
 *
 * Since: 2.4
 **/
void
gimp_page_selector_select_range (GimpPageSelector *selector,
                                 const gchar      *range)
{
  GimpPageSelectorPrivate  *priv;
  gchar                   **ranges;

  g_return_if_fail (GIMP_IS_PAGE_SELECTOR (selector));

  priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);

  if (! range)
    range = "";

  g_signal_handlers_block_by_func (priv->view,
                                   gimp_page_selector_selection_changed,
                                   selector);

  gimp_page_selector_unselect_all (selector);

  ranges = g_strsplit (range, ",", -1);

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
              gchar *from;
              gchar *to;
              gint   page_from = -1;
              gint   page_to   = -1;

              *dash = '\0';

              from = g_strstrip (range);
              to   = g_strstrip (dash + 1);

              if (sscanf (from, "%i", &page_from) != 1 && strlen (from) == 0)
                page_from = 1;

              if (sscanf (to, "%i", &page_to) != 1 && strlen (to) == 0)
                page_to = priv->n_pages;

              if (page_from > 0        &&
                  page_to   > 0        &&
                  page_from <= page_to &&
                  page_from <= priv->n_pages)
                {
                  gint page_no;

                  page_from = MAX (page_from, 1) - 1;
                  page_to   = MIN (page_to, priv->n_pages) - 1;

                  for (page_no = page_from; page_no <= page_to; page_no++)
                    gimp_page_selector_select_page (selector, page_no);
                }
            }
          else
            {
              gint page_no;

              if (sscanf (range, "%i", &page_no) == 1 &&
                  page_no >= 1                        &&
                  page_no <= priv->n_pages)
                {
                  gimp_page_selector_select_page (selector, page_no - 1);
                }
            }
        }

      g_strfreev (ranges);
    }

  g_signal_handlers_unblock_by_func (priv->view,
                                     gimp_page_selector_selection_changed,
                                     selector);

  gimp_page_selector_selection_changed (GTK_ICON_VIEW (priv->view), selector);
}

/**
 * gimp_page_selector_get_selected_range:
 * @selector: Pointer to a #GimpPageSelector.
 *
 * Returns: A newly allocated string representing the set of selected
 *          pages. See gimp_page_selector_select_range() for the
 *          format of the string.
 *
 * Since: 2.4
 **/
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
  GimpPageSelectorPrivate *priv = GIMP_PAGE_SELECTOR_GET_PRIVATE (selector);
  GList                   *selected;
  gint                     n_selected;
  gchar                   *range;

  selected = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (priv->view));
  n_selected = g_list_length (selected);
  g_list_free_full (selected, (GDestroyNotify) gtk_tree_path_free);

  if (n_selected == 0)
    {
      gtk_label_set_text (GTK_LABEL (priv->count_label),
                          _("Nothing selected"));
    }
  else if (n_selected == 1)
    {
      gtk_label_set_text (GTK_LABEL (priv->count_label),
                          _("One page selected"));
    }
  else
    {
      gchar *text;

      if (n_selected == priv->n_pages)
        text = g_strdup_printf (ngettext ("%d page selected",
                                          "All %d pages selected", n_selected),
                                n_selected);
      else
        text = g_strdup_printf (ngettext ("%d page selected",
                                          "%d pages selected",
                                          n_selected),
                                n_selected);

      gtk_label_set_text (GTK_LABEL (priv->count_label), text);
      g_free (text);
    }

  range = gimp_page_selector_get_selected_range (selector);
  gtk_entry_set_text (GTK_ENTRY (priv->range_entry), range);
  g_free (range);

  gtk_editable_set_position (GTK_EDITABLE (priv->range_entry), -1);

  g_signal_emit (selector, selector_signals[SELECTION_CHANGED], 0);
}

static void
gimp_page_selector_item_activated (GtkIconView      *icon_view,
                                   GtkTreePath      *path,
                                   GimpPageSelector *selector)
{
  g_signal_emit (selector, selector_signals[ACTIVATE], 0);
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

static gint
gimp_page_selector_int_compare (gconstpointer a,
                                gconstpointer b)
{
  return *(gint*)a - *(gint*)b;
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

static void
draw_frame_row (GdkPixbuf *frame_image,
                gint       target_width,
                gint       source_width,
                gint       source_v_position,
                gint       dest_v_position,
                GdkPixbuf *result_pixbuf,
                gint       left_offset,
                gint       height)
{
  gint remaining_width = target_width;
  gint h_offset        = 0;

  while (remaining_width > 0)
    {
      gint slab_width = (remaining_width > source_width ?
                         source_width : remaining_width);

      gdk_pixbuf_copy_area (frame_image,
                            left_offset, source_v_position,
                            slab_width, height,
                            result_pixbuf,
                            left_offset + h_offset, dest_v_position);

      remaining_width -= slab_width;
      h_offset += slab_width;
    }
}

/* utility to draw the middle section of the frame in a loop */
static void
draw_frame_column (GdkPixbuf *frame_image,
                   gint       target_height,
                   gint       source_height,
                   gint       source_h_position,
                   gint       dest_h_position,
                   GdkPixbuf *result_pixbuf,
                   gint       top_offset, int width)
{
  gint remaining_height = target_height;
  gint v_offset         = 0;

  while (remaining_height > 0)
    {
      gint slab_height = (remaining_height > source_height ?
                          source_height : remaining_height);

      gdk_pixbuf_copy_area (frame_image,
                            source_h_position, top_offset,
                            width, slab_height,
                            result_pixbuf,
                            dest_h_position, top_offset + v_offset);

      remaining_height -= slab_height;
      v_offset += slab_height;
    }
}

static GdkPixbuf *
stretch_frame_image (GdkPixbuf *frame_image,
                     gint       left_offset,
                     gint       top_offset,
                     gint       right_offset,
                     gint       bottom_offset,
                     gint       dest_width,
                     gint       dest_height)
{
  GdkPixbuf *pixbuf;
  gint       frame_width, frame_height;
  gint       target_width,  target_frame_width;
  gint       target_height, target_frame_height;

  frame_width  = gdk_pixbuf_get_width  (frame_image);
  frame_height = gdk_pixbuf_get_height (frame_image );

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                           dest_width, dest_height);
  gdk_pixbuf_fill (pixbuf, 0);

  target_width = dest_width - left_offset - right_offset;
  target_height = dest_height - top_offset - bottom_offset;
  target_frame_width  = frame_width - left_offset - right_offset;
  target_frame_height = frame_height - top_offset - bottom_offset;

  left_offset   += MIN (target_width / 4, target_frame_width / 4);
  right_offset  += MIN (target_width / 4, target_frame_width / 4);
  top_offset    += MIN (target_height / 4, target_frame_height / 4);
  bottom_offset += MIN (target_height / 4, target_frame_height / 4);

  target_width = dest_width - left_offset - right_offset;
  target_height = dest_height - top_offset - bottom_offset;
  target_frame_width  = frame_width - left_offset - right_offset;
  target_frame_height = frame_height - top_offset - bottom_offset;

  /* draw the left top corner  and top row */
  gdk_pixbuf_copy_area (frame_image,
                        0, 0, left_offset, top_offset,
                        pixbuf, 0,  0);
  draw_frame_row (frame_image, target_width, target_frame_width,
                  0, 0,
                  pixbuf,
                  left_offset, top_offset);

  /* draw the right top corner and left column */
  gdk_pixbuf_copy_area (frame_image,
                        frame_width - right_offset, 0,
                        right_offset, top_offset,

                        pixbuf,
                        dest_width - right_offset,  0);
  draw_frame_column (frame_image, target_height, target_frame_height, 0, 0,
                     pixbuf, top_offset, left_offset);

  /* draw the bottom right corner and bottom row */
  gdk_pixbuf_copy_area (frame_image,
                        frame_width - right_offset, frame_height - bottom_offset,
                        right_offset, bottom_offset,
                        pixbuf,
                        dest_width - right_offset, dest_height - bottom_offset);
  draw_frame_row (frame_image, target_width, target_frame_width,
                  frame_height - bottom_offset, dest_height - bottom_offset,
                  pixbuf, left_offset, bottom_offset);

  /* draw the bottom left corner and the right column */
  gdk_pixbuf_copy_area (frame_image,
                        0, frame_height - bottom_offset,
                        left_offset, bottom_offset,
                        pixbuf,
                        0,  dest_height - bottom_offset);
  draw_frame_column (frame_image, target_height, target_frame_height,
                     frame_width - right_offset, dest_width - right_offset,
                     pixbuf, top_offset, right_offset);

  return pixbuf;
}

#define FRAME_LEFT   2
#define FRAME_TOP    2
#define FRAME_RIGHT  4
#define FRAME_BOTTOM 4

static GdkPixbuf *
gimp_page_selector_add_frame (GtkWidget *widget,
                              GdkPixbuf *pixbuf)
{
  GdkPixbuf *frame;
  gint       width, height;

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  frame = g_object_get_data (G_OBJECT (widget), "frame");

  if (! frame)
    {
      GError *error = NULL;

      frame = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                        GIMP_ICON_FRAME, 64, 0, &error);
      if (error)
        {
          g_printerr ("%s: %s\n", G_STRFUNC, error->message);
          g_error_free (error);
        }
      g_return_val_if_fail (frame, NULL);
      g_object_set_data_full (G_OBJECT (widget), "frame", frame,
                              (GDestroyNotify) g_object_unref);
    }

  frame = stretch_frame_image (frame,
                               FRAME_LEFT,
                               FRAME_TOP,
                               FRAME_RIGHT,
                               FRAME_BOTTOM,
                               width  + FRAME_LEFT + FRAME_RIGHT,
                               height + FRAME_TOP  + FRAME_BOTTOM);

  gdk_pixbuf_copy_area (pixbuf, 0, 0, width, height,
                        frame, FRAME_LEFT, FRAME_TOP);

  return frame;
}
