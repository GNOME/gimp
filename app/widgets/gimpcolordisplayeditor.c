/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "widgets/gimpeditor.h"
#include "widgets/gimphelp-ids.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-filter-dialog.h"

#include "gimp-intl.h"


#define LIST_WIDTH  150
#define LIST_HEIGHT 100

#define UPDATE_DISPLAY(cdd) G_STMT_START         \
{	                                         \
  gimp_display_shell_expose_full ((cdd)->shell); \
} G_STMT_END


typedef struct _ColorDisplayDialog ColorDisplayDialog;

struct _ColorDisplayDialog
{
  GimpDisplayShell *shell;

  GtkWidget        *dialog;

  GtkTreeStore     *src;
  GtkTreeStore     *dest;

  GtkTreeSelection *src_sel;
  GtkTreeSelection *dest_sel;

  GimpColorDisplay *selected;

  gboolean          modified;

  GList            *old_nodes;

  GtkWidget        *add_button;
  GtkWidget        *remove_button;
  GtkWidget        *up_button;
  GtkWidget        *down_button;

  GtkWidget        *config_frame;
  GtkWidget        *config_box;
  GtkWidget        *config_widget;

  GtkWidget        *reset_button;
};


static void   make_dialog                      (ColorDisplayDialog *cdd);

static void   color_display_response           (GtkWidget          *widget,
                                                gint                response_id,
                                                ColorDisplayDialog *cdd);
static void   color_display_add_callback       (GtkWidget          *widget,
                                                ColorDisplayDialog *cdd);
static void   color_display_remove_callback    (GtkWidget          *widget,
                                                ColorDisplayDialog *cdd);
static void   color_display_up_callback        (GtkWidget          *widget,
                                                ColorDisplayDialog *cdd);
static void   color_display_down_callback      (GtkWidget          *widget,
                                                ColorDisplayDialog *cdd);

static void   dest_list_populate               (GList              *node_list,
                                                GtkTreeStore       *dest);

static void   src_selection_changed            (GtkTreeSelection   *sel,
                                                ColorDisplayDialog *cdd);
static void   dest_selection_changed           (GtkTreeSelection   *sel,
                                                ColorDisplayDialog *cdd);

static void   selected_filter_changed          (GimpColorDisplay   *filter,
                                                ColorDisplayDialog *cdd);
static void   selected_filter_reset            (GtkWidget          *widget,
                                                ColorDisplayDialog *cdd);

static void   color_display_update_up_and_down (ColorDisplayDialog *cdd);


static void
make_dialog (ColorDisplayDialog *cdd)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *editor;
  GtkWidget *scrolled_win;
  GtkWidget *tv;
  GtkWidget *vbox;
  GtkWidget *image;

  cdd->dialog = gimp_dialog_new (_("Color Display Filters"), "display_filters",
                                 NULL, 0,
                                 gimp_standard_help_func,
                                 GIMP_HELP_DISPLAY_FILTER_DIALOG,

                                 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                 GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                 NULL);

  g_signal_connect (cdd->dialog, "response",
                    G_CALLBACK (color_display_response),
                    cdd);

  main_vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cdd->dialog)->vbox), main_vbox,
		      TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  cdd->src = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (cdd->src));
  g_object_unref (cdd->src);

  gtk_widget_set_size_request (tv, LIST_WIDTH, LIST_HEIGHT);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tv), FALSE);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
                                               0, _("Available Filters"),
                                               gtk_cell_renderer_text_new (),
                                               "text", 0, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);

  cdd->src_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (cdd->src_sel, "changed",
                    G_CALLBACK (src_selection_changed),
                    cdd);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  cdd->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), cdd->add_button, FALSE, FALSE, 16);
  gtk_widget_set_sensitive (cdd->add_button, FALSE);
  gtk_widget_show (cdd->add_button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (cdd->add_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (cdd->add_button,
                           _("Add the selected filter to the list of "
                             "active filters."), NULL);

  g_signal_connect (cdd->add_button, "clicked",
                    G_CALLBACK (color_display_add_callback),
                    cdd);

  cdd->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), cdd->remove_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (cdd->remove_button, FALSE);
  gtk_widget_show (cdd->remove_button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (cdd->remove_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (cdd->remove_button,
                           _("Remove the selected filter from the list of "
                             "active filters."), NULL);

  g_signal_connect (cdd->remove_button, "clicked",
                    G_CALLBACK (color_display_remove_callback),
                    cdd);

  editor = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  cdd->up_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_GO_UP,
                            _("Move the selected filter up"),
                            NULL,
                            G_CALLBACK (color_display_up_callback),
                            NULL,
                            cdd);

  cdd->down_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_GO_DOWN,
                            _("Move the selected filter down"),
                            NULL,
                            G_CALLBACK (color_display_down_callback),
                            NULL,
                            cdd);

  gtk_widget_set_sensitive (cdd->up_button,        FALSE);
  gtk_widget_set_sensitive (cdd->down_button,      FALSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (editor), scrolled_win);
  gtk_widget_show (scrolled_win);

  cdd->dest = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (cdd->dest));
  g_object_unref (cdd->dest);

  gtk_widget_set_size_request (tv, LIST_WIDTH, LIST_HEIGHT);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tv), FALSE);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
                                               0, _("Active Filters"),
                                               gtk_cell_renderer_text_new (),
                                               "text", 0, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);

  cdd->dest_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (cdd->dest_sel, "changed",
                    G_CALLBACK (dest_selection_changed),
                    cdd);

  /*  the config frame  */

  cdd->config_frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), cdd->config_frame, FALSE, FALSE, 0);
  gtk_widget_show (cdd->config_frame);

  cdd->config_box = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (cdd->config_box), 4);
  gtk_container_add (GTK_CONTAINER (cdd->config_frame), cdd->config_box);
  gtk_widget_show (cdd->config_box);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (cdd->config_box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  cdd->reset_button = gtk_button_new_from_stock (GIMP_STOCK_RESET);
  gtk_box_pack_end (GTK_BOX (hbox), cdd->reset_button, FALSE, FALSE, 0);
  gtk_widget_show (cdd->reset_button);

  gimp_help_set_help_data (cdd->reset_button,
                           _("Reset the selected filter to default values"),
                           NULL);

  g_signal_connect (cdd->reset_button, "clicked",
                    G_CALLBACK (selected_filter_reset),
                    cdd);

  dest_selection_changed (cdd->dest_sel, cdd);

  gtk_widget_show (main_vbox);
}

static void
color_display_response (GtkWidget          *widget,
                        gint                response_id,
                        ColorDisplayDialog *cdd)
{

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
  cdd->shell->filters_dialog = NULL;

  if (cdd->modified)
    {
      GList *list;

      if (response_id == GTK_RESPONSE_OK)
        {
          for (list = cdd->old_nodes; list; list = g_list_next (list))
            {
              if (! g_list_find (cdd->shell->filters, list->data))
                gimp_display_shell_filter_detach_destroy (cdd->shell,
                                                          list->data);
            }

          g_list_free (cdd->old_nodes);
        }
      else
        {
          list = cdd->shell->filters;
          cdd->shell->filters = cdd->old_nodes;

          while (list)
            {
              GList *next = list->next;

              if (! g_list_find (cdd->old_nodes, list->data))
                gimp_display_shell_filter_detach_destroy (cdd->shell,
                                                          list->data);

              list = next;
            }
        }

      UPDATE_DISPLAY (cdd);
    }
}

static void
color_display_update_up_and_down (ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      up_sensitive   = FALSE;
  gboolean      down_sensitive = FALSE;

  if (gtk_tree_selection_get_selected (cdd->dest_sel, &model, &iter))
    {
      GtkTreePath *path;
      gint        *indices;

      path    = gtk_tree_model_get_path (model, &iter);
      indices = gtk_tree_path_get_indices (path);

      up_sensitive   = indices[0] > 0;
      down_sensitive = indices[0] < (g_list_length (cdd->shell->filters) - 1);

      gtk_tree_path_free (path);
    }

  gtk_widget_set_sensitive (cdd->up_button,   up_sensitive);
  gtk_widget_set_sensitive (cdd->down_button, down_sensitive);
}

static void
color_display_add_callback (GtkWidget          *widget,
			    ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (cdd->src_sel, &model, &iter))
    {
      GimpColorDisplay *filter;
      GValue            val = { 0, };

      gtk_tree_model_get_value (model, &iter, 1, &val);

      filter = gimp_display_shell_filter_attach (cdd->shell,
                                                 (GType) g_value_get_pointer (&val));

      g_value_unset (&val);

      gtk_tree_store_append (cdd->dest, &iter, NULL);

      gtk_tree_store_set (cdd->dest, &iter,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter)->name,
                          1, filter,
                          -1);

      cdd->modified = TRUE;

      color_display_update_up_and_down (cdd);

      UPDATE_DISPLAY (cdd);
    }
}

static void
color_display_remove_callback (GtkWidget          *widget,
			       ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (cdd->dest_sel, &model, &iter))
    {
      GimpColorDisplay *filter;
      GValue            val = { 0, };

      gtk_tree_model_get_value (model, &iter, 1, &val);

      filter = g_value_get_pointer (&val);

      g_value_unset (&val);

      gtk_tree_store_remove (cdd->dest, &iter);

      cdd->modified = TRUE;

      if (g_list_find (cdd->old_nodes, filter))
        gimp_display_shell_filter_detach (cdd->shell, filter);
      else
        gimp_display_shell_filter_detach_destroy (cdd->shell, filter);

      color_display_update_up_and_down (cdd);

      UPDATE_DISPLAY (cdd);
    }
}

static void
color_display_up_callback (GtkWidget          *widget,
			   ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter1;
  GtkTreeIter   iter2;

  if (gtk_tree_selection_get_selected (cdd->dest_sel, &model, &iter1))
    {
      GtkTreePath      *path;
      GimpColorDisplay *filter1;
      GimpColorDisplay *filter2;

      path = gtk_tree_model_get_path (model, &iter1);
      gtk_tree_path_prev (path);
      gtk_tree_model_get_iter (model, &iter2, path);
      gtk_tree_path_free (path);

      gtk_tree_model_get (model, &iter1, 1, &filter1, -1);
      gtk_tree_model_get (model, &iter2, 1, &filter2, -1);

      gtk_tree_store_set (GTK_TREE_STORE (model), &iter1,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter2)->name,
                          1, filter2,
                          -1);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter2,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter1)->name,
                          1, filter1,
                          -1);

      gimp_display_shell_filter_reorder_up (cdd->shell, filter1);

      cdd->modified = TRUE;

      gtk_tree_selection_select_iter (cdd->dest_sel, &iter2);

      UPDATE_DISPLAY (cdd);
    }
}

static void
color_display_down_callback (GtkWidget          *widget,
			     ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter1;
  GtkTreeIter   iter2;

  if (gtk_tree_selection_get_selected (cdd->dest_sel, &model, &iter1))
    {
      GtkTreePath      *path;
      GimpColorDisplay *filter1;
      GimpColorDisplay *filter2;

      path = gtk_tree_model_get_path (model, &iter1);
      gtk_tree_path_next (path);
      gtk_tree_model_get_iter (model, &iter2, path);
      gtk_tree_path_free (path);

      gtk_tree_model_get (model, &iter1, 1, &filter1, -1);
      gtk_tree_model_get (model, &iter2, 1, &filter2, -1);

      gtk_tree_store_set (GTK_TREE_STORE (model), &iter1,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter2)->name,
                          1, filter2,
                          -1);
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter2,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter1)->name,
                          1, filter1,
                          -1);

      gimp_display_shell_filter_reorder_down (cdd->shell, filter1);

      cdd->modified = TRUE;

      gtk_tree_selection_select_iter (cdd->dest_sel, &iter2);

      UPDATE_DISPLAY (cdd);
    }
}

void
gimp_display_shell_filter_dialog_new (GimpDisplayShell *shell)
{
  ColorDisplayDialog *cdd;
  GType              *filter_types;
  guint               n_filter_types;
  gint                i;

  cdd = g_new0 (ColorDisplayDialog, 1);

  make_dialog (cdd);

  filter_types = g_type_children (GIMP_TYPE_COLOR_DISPLAY, &n_filter_types);

  for (i = 0; i < n_filter_types; i++)
    {
      GimpColorDisplayClass *filter_class;
      GtkTreeIter            iter;

      filter_class = g_type_class_ref (filter_types[i]);

      gtk_tree_store_append (cdd->src, &iter, NULL);

      gtk_tree_store_set (cdd->src, &iter,
                          0, filter_class->name,
                          1, filter_types[i],
                          -1);

      g_type_class_unref (filter_class);
    }

  g_free (filter_types);

  cdd->old_nodes = shell->filters;
  dest_list_populate (shell->filters, cdd->dest);
  shell->filters = g_list_copy (cdd->old_nodes);

  cdd->shell = shell;

  shell->filters_dialog = cdd->dialog;
}

static void
dest_list_populate (GList        *node_list,
    		    GtkTreeStore *dest)
{
  GimpColorDisplay *filter;
  GList            *list;
  GtkTreeIter       iter;

  for (list = node_list; list; list = g_list_next (list))
    {
      filter = (GimpColorDisplay *) list->data;

      gtk_tree_store_append (dest, &iter, NULL);

      gtk_tree_store_set (dest, &iter,
                          0, GIMP_COLOR_DISPLAY_GET_CLASS (filter)->name,
                          1, filter,
                          -1);
    }
}

static void
src_selection_changed (GtkTreeSelection   *sel,
                       ColorDisplayDialog *cdd)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  const gchar  *name = NULL;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = { 0, };

      gtk_tree_model_get_value (model, &iter, 0, &val);

      name = g_value_get_string (&val);

      g_value_unset (&val);
    }

  gtk_widget_set_sensitive (cdd->add_button, (name != NULL));
}

static void
dest_selection_changed (GtkTreeSelection   *sel,
                        ColorDisplayDialog *cdd)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GimpColorDisplay *filter = NULL;

  if (cdd->selected)
    {
      g_signal_handlers_disconnect_by_func (cdd->selected,
                                            selected_filter_changed,
                                            cdd);
      g_object_remove_weak_pointer (G_OBJECT (cdd->selected),
                                    (gpointer) &cdd->selected);
      cdd->selected = NULL;
    }

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = { 0, };

      gtk_tree_model_get_value (model, &iter, 1, &val);

      filter = g_value_get_pointer (&val);

      g_value_unset (&val);
    }

  gtk_widget_set_sensitive (cdd->remove_button, (filter != NULL));
  gtk_widget_set_sensitive (cdd->reset_button,  (filter != NULL));

  if (cdd->config_widget)
    gtk_container_remove (GTK_CONTAINER (cdd->config_box), cdd->config_widget);

  if (filter)
    {
      gchar *str;

      cdd->selected = filter;

      g_object_add_weak_pointer (G_OBJECT (filter), (gpointer) &cdd->selected);
      g_signal_connect (cdd->selected, "changed",
                        G_CALLBACK (selected_filter_changed),
                        cdd);

      cdd->config_widget = gimp_color_display_configure (filter);

      str = g_strdup_printf (_("Configure Selected Filter: %s"),
                             GIMP_COLOR_DISPLAY_GET_CLASS (filter)->name);
      gtk_frame_set_label (GTK_FRAME (cdd->config_frame), str);
      g_free (str);
    }
  else
    {
      cdd->config_widget = gtk_label_new (_("No Filter Selected"));
      gtk_widget_set_sensitive (cdd->config_widget, FALSE);

      gtk_frame_set_label (GTK_FRAME (cdd->config_frame),
                           _("Configure Selected Filter"));
    }

  if (cdd->config_widget)
    {
      gtk_box_pack_start (GTK_BOX (cdd->config_box), cdd->config_widget,
                          FALSE, FALSE, 0);
      gtk_widget_show (cdd->config_widget);

      g_object_add_weak_pointer (G_OBJECT (cdd->config_widget),
                                 (gpointer) &cdd->config_widget);
    }

  color_display_update_up_and_down (cdd);
}

static void
selected_filter_changed (GimpColorDisplay   *filter,
                         ColorDisplayDialog *cdd)
{
  UPDATE_DISPLAY (cdd);
}

static void
selected_filter_reset (GtkWidget          *widget,
                       ColorDisplayDialog *cdd)
{
  if (cdd->selected)
    gimp_color_display_configure_reset (cdd->selected);
}
