/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolordisplayeditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "propgui/propgui-types.h"

#include "core/gimp.h"

#include "propgui/gimppropgui.h"

#include "gimpcolordisplayeditor.h"
#include "gimpeditor.h"

#include "gimp-intl.h"


#define LIST_WIDTH  200
#define LIST_HEIGHT 100


enum
{
  SRC_COLUMN_NAME,
  SRC_COLUMN_ICON,
  SRC_COLUMN_TYPE,
  N_SRC_COLUMNS
};

enum
{
  DEST_COLUMN_ENABLED,
  DEST_COLUMN_NAME,
  DEST_COLUMN_ICON,
  DEST_COLUMN_FILTER,
  N_DEST_COLUMNS
};


static void   gimp_color_display_editor_dispose        (GObject               *object);

static void   gimp_color_display_editor_add_clicked    (GtkWidget             *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_remove_clicked (GtkWidget             *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_up_clicked     (GtkWidget             *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_down_clicked   (GtkWidget             *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_reset_clicked  (GtkWidget             *widget,
                                                        GimpColorDisplayEditor *editor);

static void   gimp_color_display_editor_src_changed    (GtkTreeSelection       *sel,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_dest_changed   (GtkTreeSelection       *sel,
                                                        GimpColorDisplayEditor *editor);

static void   gimp_color_display_editor_added          (GimpColorDisplayStack  *stack,
                                                        GimpColorDisplay       *display,
                                                        gint                    position,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_removed        (GimpColorDisplayStack  *stack,
                                                        GimpColorDisplay       *display,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_reordered      (GimpColorDisplayStack  *stack,
                                                        GimpColorDisplay       *display,
                                                        gint                    position,
                                                        GimpColorDisplayEditor *editor);

static void   gimp_color_display_editor_enabled        (GimpColorDisplay       *display,
                                                        GParamSpec             *pspec,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_enable_toggled (GtkCellRendererToggle  *toggle,
                                                        const gchar            *path,
                                                        GimpColorDisplayEditor *editor);

static void   gimp_color_display_editor_update_buttons (GimpColorDisplayEditor *editor);


G_DEFINE_TYPE (GimpColorDisplayEditor, gimp_color_display_editor, GTK_TYPE_BOX)

#define parent_class gimp_color_display_editor_parent_class


static void
gimp_color_display_editor_class_init (GimpColorDisplayEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_color_display_editor_dispose;
}

static void
gimp_color_display_editor_init (GimpColorDisplayEditor *editor)
{
  GtkWidget         *paned;
  GtkWidget         *hbox;
  GtkWidget         *ed;
  GtkWidget         *scrolled_win;
  GtkWidget         *tv;
  GtkWidget         *vbox;
  GtkWidget         *image;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *rend;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_wide_handle (GTK_PANED (paned), TRUE);
  gtk_box_pack_start (GTK_BOX (editor), paned, TRUE, TRUE, 0);
  gtk_widget_show (paned);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_paned_pack1 (GTK_PANED (paned), hbox, FALSE, FALSE);
  gtk_widget_show (hbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  editor->src = gtk_list_store_new (N_SRC_COLUMNS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_GTYPE);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (editor->src),
                                        SRC_COLUMN_NAME, GTK_SORT_ASCENDING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (editor->src));
  g_object_unref (editor->src);

  gtk_widget_set_size_request (tv, LIST_WIDTH, LIST_HEIGHT);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tv), FALSE);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Available Filters"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  rend = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, rend, FALSE);
  gtk_tree_view_column_set_attributes (column, rend,
                                       "icon-name", SRC_COLUMN_ICON,
                                       NULL);

  rend = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, rend, TRUE);
  gtk_tree_view_column_set_attributes (column, rend,
                                       "text", SRC_COLUMN_NAME,
                                       NULL);

  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);

  editor->src_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (editor->src_sel, "changed",
                    G_CALLBACK (gimp_color_display_editor_src_changed),
                    editor);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  editor->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), editor->add_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (editor->add_button, FALSE);
  gtk_widget_show (editor->add_button);

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (editor->add_button), image);
  gtk_widget_show (image);

  g_signal_connect (editor->add_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_add_clicked),
                    editor);

  editor->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), editor->remove_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (editor->remove_button, FALSE);
  gtk_widget_show (editor->remove_button);

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (editor->remove_button), image);
  gtk_widget_show (image);

  g_signal_connect (editor->remove_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_remove_clicked),
                    editor);

  ed = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (hbox), ed, TRUE, TRUE, 0);
  gtk_widget_show (ed);

  editor->up_button =
    gimp_editor_add_button (GIMP_EDITOR (ed),
                            GIMP_ICON_GO_UP,
                            _("Move the selected filter up"),
                            NULL,
                            G_CALLBACK (gimp_color_display_editor_up_clicked),
                            NULL,
                            G_OBJECT (editor));

  editor->down_button =
    gimp_editor_add_button (GIMP_EDITOR (ed),
                            GIMP_ICON_GO_DOWN,
                            _("Move the selected filter down"),
                            NULL,
                            G_CALLBACK (gimp_color_display_editor_down_clicked),
                            NULL,
                            G_OBJECT (editor));

  gtk_widget_set_sensitive (editor->up_button,   FALSE);
  gtk_widget_set_sensitive (editor->down_button, FALSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (ed), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  editor->dest = gtk_list_store_new (N_DEST_COLUMNS,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_STRING,
                                     G_TYPE_STRING,
                                     GIMP_TYPE_COLOR_DISPLAY);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (editor->dest));
  g_object_unref (editor->dest);

  gtk_widget_set_size_request (tv, LIST_WIDTH, LIST_HEIGHT);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (tv), FALSE);

  rend = gtk_cell_renderer_toggle_new ();

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (gimp_color_display_editor_enable_toggled),
                    editor);

  column = gtk_tree_view_column_new_with_attributes (NULL, rend,
                                                     "active",
                                                     DEST_COLUMN_ENABLED,
                                                     NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tv), column, 0);

  image = gtk_image_new_from_icon_name (GIMP_ICON_VISIBLE,
                                        GTK_ICON_SIZE_MENU);
  gtk_tree_view_column_set_widget (column, image);
  gtk_widget_show (image);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Active Filters"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  rend = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, rend, FALSE);
  gtk_tree_view_column_set_attributes (column, rend,
                                       "icon-name", DEST_COLUMN_ICON,
                                       NULL);

  rend = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, rend, TRUE);
  gtk_tree_view_column_set_attributes (column, rend,
                                       "text", DEST_COLUMN_NAME,
                                       NULL);

  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);

  editor->dest_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));

  g_signal_connect (editor->dest_sel, "changed",
                    G_CALLBACK (gimp_color_display_editor_dest_changed),
                    editor);

  /*  the config frame  */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_paned_pack2 (GTK_PANED (paned), vbox, TRUE, FALSE);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->config_frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), editor->config_frame, TRUE, TRUE, 0);
  gtk_widget_show (editor->config_frame);

  editor->config_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (editor->config_frame), editor->config_box);
  gtk_widget_show (editor->config_box);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_end (GTK_BOX (editor->config_box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->reset_button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_end (GTK_BOX (hbox), editor->reset_button, FALSE, FALSE, 0);
  gtk_widget_show (editor->reset_button);

  gimp_help_set_help_data (editor->reset_button,
                           _("Reset the selected filter to default values"),
                           NULL);

  g_signal_connect (editor->reset_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_reset_clicked),
                    editor);

  gimp_color_display_editor_dest_changed (editor->dest_sel, editor);
}

static void
gimp_color_display_editor_dispose (GObject *object)
{
  GimpColorDisplayEditor *editor = GIMP_COLOR_DISPLAY_EDITOR (object);

  g_clear_weak_pointer (&editor->selected);

  g_clear_object (&editor->stack);
  g_clear_object (&editor->config);
  g_clear_object (&editor->managed);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
gimp_color_display_editor_new (Gimp                  *gimp,
                               GimpColorDisplayStack *stack,
                               GimpColorConfig       *config,
                               GimpColorManaged      *managed)
{
  GimpColorDisplayEditor *editor;
  GType                  *display_types;
  guint                   n_display_types;
  gint                    i;
  GList                  *list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);

  editor = g_object_new (GIMP_TYPE_COLOR_DISPLAY_EDITOR, NULL);

  editor->gimp    = gimp;
  editor->stack   = g_object_ref (stack);
  editor->config  = g_object_ref (config);
  editor->managed = g_object_ref (managed);

  display_types = g_type_children (GIMP_TYPE_COLOR_DISPLAY, &n_display_types);

  for (i = 0; i < n_display_types; i++)
    {
      GimpColorDisplayClass *display_class;
      GtkTreeIter            iter;

      display_class = g_type_class_ref (display_types[i]);

      gtk_list_store_append (editor->src, &iter);

      gtk_list_store_set (editor->src, &iter,
                          SRC_COLUMN_ICON, display_class->icon_name,
                          SRC_COLUMN_NAME, display_class->name,
                          SRC_COLUMN_TYPE, display_types[i],
                          -1);

      g_type_class_unref (display_class);
    }

  g_free (display_types);

  for (list = gimp_color_display_stack_get_filters (stack);
       list;
       list = g_list_next (list))
    {
      GimpColorDisplay *display = list->data;
      GtkTreeIter       iter;
      gboolean          enabled;
      const gchar      *name;
      const gchar      *icon_name;

      enabled = gimp_color_display_get_enabled (display);

      name      = GIMP_COLOR_DISPLAY_GET_CLASS (display)->name;
      icon_name = GIMP_COLOR_DISPLAY_GET_CLASS (display)->icon_name;

      gtk_list_store_append (editor->dest, &iter);

      gtk_list_store_set (editor->dest, &iter,
                          DEST_COLUMN_ENABLED, enabled,
                          DEST_COLUMN_ICON,    icon_name,
                          DEST_COLUMN_NAME,    name,
                          DEST_COLUMN_FILTER,  display,
                          -1);

      g_signal_connect_object (display, "notify::enabled",
                               G_CALLBACK (gimp_color_display_editor_enabled),
                               G_OBJECT (editor), 0);
    }

  g_signal_connect_object (stack, "added",
                           G_CALLBACK (gimp_color_display_editor_added),
                           G_OBJECT (editor), 0);
  g_signal_connect_object (stack, "removed",
                           G_CALLBACK (gimp_color_display_editor_removed),
                           G_OBJECT (editor), 0);
  g_signal_connect_object (stack, "reordered",
                           G_CALLBACK (gimp_color_display_editor_reordered),
                           G_OBJECT (editor), 0);

  return GTK_WIDGET (editor);
}

static void
gimp_color_display_editor_add_clicked (GtkWidget              *widget,
                                       GimpColorDisplayEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (editor->src_sel, &model, &iter))
    {
      GimpColorDisplay *display;
      GType             type;

      gtk_tree_model_get (model, &iter, SRC_COLUMN_TYPE, &type, -1);

      display = g_object_new (type,
                              "color-config",  editor->config,
                              "color-managed", editor->managed,
                              NULL);

      if (display)
        {
          gimp_color_display_stack_add (editor->stack, display);
          g_object_unref (display);
        }
    }
}

static void
gimp_color_display_editor_remove_clicked (GtkWidget             *widget,
                                          GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_stack_remove (editor->stack, editor->selected);
}

static void
gimp_color_display_editor_up_clicked (GtkWidget             *widget,
                                      GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_stack_reorder_up (editor->stack, editor->selected);
}

static void
gimp_color_display_editor_down_clicked (GtkWidget             *widget,
                                        GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_stack_reorder_down (editor->stack, editor->selected);
}

static void
gimp_color_display_editor_reset_clicked (GtkWidget             *widget,
                                         GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_configure_reset (editor->selected);
}

static void
gimp_color_display_editor_src_changed (GtkTreeSelection       *sel,
                                       GimpColorDisplayEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *tip  = NULL;
  const gchar  *name = NULL;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = G_VALUE_INIT;

      gtk_tree_model_get_value (model, &iter, SRC_COLUMN_NAME, &val);

      name = g_value_get_string (&val);

      tip = g_strdup_printf (_("Add '%s' to the list of active filters"), name);

      g_value_unset (&val);
    }

  gtk_widget_set_sensitive (editor->add_button, name != NULL);

  gimp_help_set_help_data (editor->add_button, tip, NULL);
  g_free (tip);
}

static void
gimp_color_display_editor_dest_changed (GtkTreeSelection       *sel,
                                        GimpColorDisplayEditor *editor)
{
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GimpColorDisplay *display = NULL;
  gchar            *tip     = NULL;

  g_clear_weak_pointer (&editor->selected);

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = G_VALUE_INIT;

      gtk_tree_model_get_value (model, &iter, DEST_COLUMN_FILTER, &val);

      display = g_value_get_object (&val);

      g_value_unset (&val);

      tip = g_strdup_printf (_("Remove '%s' from the list of active filters"),
                             GIMP_COLOR_DISPLAY_GET_CLASS (display)->name);
    }

  gimp_help_set_help_data (editor->remove_button, tip, NULL);
  g_free (tip);

  gtk_widget_set_sensitive (editor->remove_button, display != NULL);
  gtk_widget_set_sensitive (editor->reset_button,  display != NULL);

  if (editor->config_widget)
    gtk_container_remove (GTK_CONTAINER (editor->config_box),
                          editor->config_widget);

  if (display)
    {
      g_set_weak_pointer (&editor->selected, display);

      editor->config_widget = gimp_color_display_configure (display);

      if (! editor->config_widget)
        {
          editor->config_widget =
            gimp_prop_gui_new (G_OBJECT (display),
                               G_TYPE_FROM_INSTANCE (display), 0,
                               NULL,
                               gimp_get_user_context (editor->gimp),
                               NULL, NULL, NULL);
        }

      gtk_frame_set_label (GTK_FRAME (editor->config_frame),
                           GIMP_COLOR_DISPLAY_GET_CLASS (display)->name);
    }
  else
    {
      editor->config_widget = NULL;

      gtk_frame_set_label (GTK_FRAME (editor->config_frame),
                           _("No filter selected"));
    }

  if (editor->config_widget)
    {
      gtk_box_pack_start (GTK_BOX (editor->config_box), editor->config_widget,
                          FALSE, FALSE, 0);
      gtk_widget_show (editor->config_widget);

      g_object_add_weak_pointer (G_OBJECT (editor->config_widget),
                                 (gpointer) &editor->config_widget);
    }

  gimp_color_display_editor_update_buttons (editor);
}

static void
gimp_color_display_editor_added (GimpColorDisplayStack  *stack,
                                 GimpColorDisplay       *display,
                                 gint                    position,
                                 GimpColorDisplayEditor *editor)
{
  GtkTreeIter  iter;
  gboolean     enabled;
  const gchar *name;
  const gchar *icon_name;

  enabled = gimp_color_display_get_enabled (display);

  name      = GIMP_COLOR_DISPLAY_GET_CLASS (display)->name;
  icon_name = GIMP_COLOR_DISPLAY_GET_CLASS (display)->icon_name;

  gtk_list_store_insert (editor->dest, &iter, position);

  gtk_list_store_set (editor->dest, &iter,
                      DEST_COLUMN_ENABLED, enabled,
                      DEST_COLUMN_ICON,    icon_name,
                      DEST_COLUMN_NAME,    name,
                      DEST_COLUMN_FILTER,  display,
                      -1);

  g_signal_connect_object (display, "notify::enabled",
                           G_CALLBACK (gimp_color_display_editor_enabled),
                           G_OBJECT (editor), 0);

  gimp_color_display_editor_update_buttons (editor);
}

static void
gimp_color_display_editor_removed (GimpColorDisplayStack  *stack,
                                   GimpColorDisplay       *display,
                                   GimpColorDisplayEditor *editor)
{
  GtkTreeIter iter;
  gboolean    iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (editor->dest),
                                                   &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (editor->dest),
                                              &iter))
    {
      GimpColorDisplay *display2;

      gtk_tree_model_get (GTK_TREE_MODEL (editor->dest), &iter,
                          DEST_COLUMN_FILTER, &display2,
                          -1);

      g_object_unref (display2);

      if (display == display2)
        {
          g_signal_handlers_disconnect_by_func (display,
                                                gimp_color_display_editor_enabled,
                                                editor);

          gtk_list_store_remove (editor->dest, &iter);

          gimp_color_display_editor_update_buttons (editor);
          break;
        }
    }
}

static void
gimp_color_display_editor_reordered (GimpColorDisplayStack  *stack,
                                     GimpColorDisplay       *display,
                                     gint                    position,
                                     GimpColorDisplayEditor *editor)
{
  GtkTreeIter iter;
  gboolean    iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (editor->dest),
                                                   &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (editor->dest),
                                              &iter))
    {
      GimpColorDisplay *display2;

      gtk_tree_model_get (GTK_TREE_MODEL (editor->dest), &iter,
                          DEST_COLUMN_FILTER, &display2,
                          -1);

      g_object_unref (display2);

      if (display == display2)
        {
          GList       *filters = gimp_color_display_stack_get_filters (stack);
          GtkTreePath *path;
          gint         old_position;

          path = gtk_tree_model_get_path (GTK_TREE_MODEL (editor->dest), &iter);
          old_position = gtk_tree_path_get_indices (path)[0];
          gtk_tree_path_free (path);

          if (position == old_position)
            return;

          if (position == -1 || position == g_list_length (filters) - 1)
            {
              gtk_list_store_move_before (editor->dest, &iter, NULL);
            }
          else if (position == 0)
            {
              gtk_list_store_move_after (editor->dest, &iter, NULL);
            }
          else
            {
              GtkTreeIter place_iter;

              path = gtk_tree_path_new_from_indices (position, -1);
              gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->dest),
                                       &place_iter, path);
              gtk_tree_path_free (path);

              if (position > old_position)
                gtk_list_store_move_after (editor->dest, &iter, &place_iter);
              else
                gtk_list_store_move_before (editor->dest, &iter, &place_iter);
            }

          gimp_color_display_editor_update_buttons (editor);

          return;
        }
    }
}

static void
gimp_color_display_editor_enabled (GimpColorDisplay       *display,
                                   GParamSpec             *pspec,
                                   GimpColorDisplayEditor *editor)
{
  GtkTreeIter iter;
  gboolean    iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (editor->dest),
                                                   &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (editor->dest),
                                              &iter))
    {
      GimpColorDisplay *display2;

      gtk_tree_model_get (GTK_TREE_MODEL (editor->dest), &iter,
                          DEST_COLUMN_FILTER,  &display2,
                          -1);

      g_object_unref (display2);

      if (display == display2)
        {
          gboolean enabled = gimp_color_display_get_enabled (display);

          gtk_list_store_set (editor->dest, &iter,
                              DEST_COLUMN_ENABLED, enabled,
                              -1);

          break;
        }
    }
}

static void
gimp_color_display_editor_enable_toggled (GtkCellRendererToggle  *toggle,
                                          const gchar            *path_str,
                                          GimpColorDisplayEditor *editor)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter  iter;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->dest), &iter, path))
    {
      GimpColorDisplay *display;
      gboolean          enabled;

      gtk_tree_model_get (GTK_TREE_MODEL (editor->dest), &iter,
                          DEST_COLUMN_FILTER,  &display,
                          DEST_COLUMN_ENABLED, &enabled,
                          -1);

      gimp_color_display_set_enabled (display, ! enabled);

      g_object_unref (display);
    }

  gtk_tree_path_free (path);
}

static void
gimp_color_display_editor_update_buttons (GimpColorDisplayEditor *editor)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      up_sensitive   = FALSE;
  gboolean      down_sensitive = FALSE;

  if (gtk_tree_selection_get_selected (editor->dest_sel, &model, &iter))
    {
      GList       *filters = gimp_color_display_stack_get_filters (editor->stack);
      GtkTreePath *path    = gtk_tree_model_get_path (model, &iter);
      gint        *indices = gtk_tree_path_get_indices (path);

      up_sensitive   = indices[0] > 0;
      down_sensitive = indices[0] < (g_list_length (filters) - 1);

      gtk_tree_path_free (path);
    }

  gtk_widget_set_sensitive (editor->up_button,   up_sensitive);
  gtk_widget_set_sensitive (editor->down_button, down_sensitive);
}
