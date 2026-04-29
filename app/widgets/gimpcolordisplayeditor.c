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
#include "gimpcolordisplaytype.h"
#include "gimpeditor.h"
#include "gimprow.h"
#include "gimprow-utils.h"

#include "gimp-intl.h"


static void   gimp_color_display_editor_dispose        (GObject                *object);

static GtkWidget *
              gimp_color_display_editor_create_row     (gpointer                item,
                                                        gpointer                user_data);
static void   gimp_color_display_editor_add_clicked    (GtkWidget              *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_remove_clicked (GtkWidget              *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_up_clicked     (GtkWidget              *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_down_clicked   (GtkWidget              *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_reset_clicked  (GtkWidget              *widget,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_src_selected   (GtkListBox             *list,
                                                        GtkListBoxRow          *row,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_dest_selected  (GtkListBox             *list,
                                                        GtkListBoxRow          *row,
                                                        GimpColorDisplayEditor *editor);
static void   gimp_color_display_editor_items_changed  (GListModel             *list,
                                                        guint                   position,
                                                        guint                   removed,
                                                        guint                   added,
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
  GtkSizeGroup *size_group;
  GtkWidget    *paned;
  GtkWidget    *hbox;
  GtkWidget    *ed;
  GtkWidget    *scrolled_win;
  GtkWidget    *vbox;
  GtkWidget    *label;
  GtkWidget    *image;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  paned = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_wide_handle (GTK_PANED (paned), TRUE);
  gtk_box_pack_start (GTK_BOX (editor), paned, TRUE, TRUE, 0);
  gtk_widget_set_visible (paned, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_paned_pack1 (GTK_PANED (paned), hbox, FALSE, FALSE);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (size_group, vbox);

  label = gtk_label_new (_("Available Filters"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_NEVER);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_win, TRUE);

  editor->src_list = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (editor->src_list),
                                             FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (editor->src_list)),
                               "view");
  gtk_container_add (GTK_CONTAINER (scrolled_win), editor->src_list);
  gtk_widget_set_visible (editor->src_list, TRUE);

  g_signal_connect (editor->src_list, "row-selected",
                    G_CALLBACK (gimp_color_display_editor_src_selected),
                    editor);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  editor->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), editor->add_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (editor->add_button, FALSE);
  gtk_widget_set_visible (editor->add_button, TRUE);

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_NEXT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (editor->add_button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (editor->add_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_add_clicked),
                    editor);

  editor->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), editor->remove_button, TRUE, FALSE, 0);
  gtk_widget_set_sensitive (editor->remove_button, FALSE);
  gtk_widget_set_visible (editor->remove_button, TRUE);

  image = gtk_image_new_from_icon_name (GIMP_ICON_GO_PREVIOUS,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (editor->remove_button), image);
  gtk_widget_set_visible (image, TRUE);

  g_signal_connect (editor->remove_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_remove_clicked),
                    editor);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_visible (vbox, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  gtk_size_group_add_widget (size_group, vbox);
  g_object_unref (size_group);

  label = gtk_label_new (_("Active Filters"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  ed = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (vbox), ed, TRUE, TRUE, 0);
  gtk_widget_set_visible (ed, TRUE);

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
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_NEVER);
  gtk_box_pack_start (GTK_BOX (ed), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_set_visible (scrolled_win, TRUE);

  editor->dest_list = gtk_list_box_new ();
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (editor->dest_list),
                                             FALSE);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (editor->dest_list)),
                               "view");
  gtk_container_add (GTK_CONTAINER (scrolled_win), editor->dest_list);
  gtk_widget_set_visible (editor->dest_list, TRUE);

  g_signal_connect (editor->dest_list, "row-selected",
                    G_CALLBACK (gimp_color_display_editor_dest_selected),
                    editor);

  /*  the config frame  */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_paned_pack2 (GTK_PANED (paned), vbox, TRUE, FALSE);
  gtk_widget_set_visible (vbox, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  editor->config_frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), editor->config_frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (editor->config_frame, TRUE);

  editor->config_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (editor->config_frame), editor->config_box);
  gtk_widget_set_visible (editor->config_box, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_end (GTK_BOX (editor->config_box), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  editor->reset_button = gtk_button_new_with_mnemonic (_("_Reset"));
  gtk_box_pack_end (GTK_BOX (hbox), editor->reset_button, FALSE, FALSE, 0);
  gtk_widget_set_visible (editor->reset_button, TRUE);

  gimp_help_set_help_data (editor->reset_button,
                           _("Reset the selected filter to default values"),
                           NULL);

  g_signal_connect (editor->reset_button, "clicked",
                    G_CALLBACK (gimp_color_display_editor_reset_clicked),
                    editor);
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
  GListModel             *types;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_DISPLAY_STACK (stack), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);

  editor = g_object_new (GIMP_TYPE_COLOR_DISPLAY_EDITOR, NULL);

  editor->gimp    = gimp;
  editor->stack   = g_object_ref (stack);
  editor->config  = g_object_ref (config);
  editor->managed = g_object_ref (managed);

  g_signal_connect_object (stack, "items-changed",
                           G_CALLBACK (gimp_color_display_editor_items_changed),
                           G_OBJECT (editor), 0);

  types = gimp_color_display_type_get_model ();
  gtk_list_box_bind_model (GTK_LIST_BOX (editor->src_list),
                           types,
                           gimp_row_create_for_context,
                           gimp_get_user_context (editor->gimp),
                           NULL);
  g_object_unref (types);

  gtk_list_box_bind_model (GTK_LIST_BOX (editor->dest_list),
                           G_LIST_MODEL (stack),
                           gimp_color_display_editor_create_row,
                           editor,
                           NULL);

  return GTK_WIDGET (editor);
}


/*  private functions  */

static GtkWidget *
gimp_color_display_editor_create_row (gpointer item,
                                      gpointer user_data)
{
  GimpColorDisplayEditor *editor = user_data;
  GimpColorDisplayType   *model;
  GtkWidget              *row;
  GtkWidget              *hbox;
  GtkWidget              *eye;

  model = gimp_color_display_type_new (G_TYPE_FROM_INSTANCE (item));
  g_object_set_data (G_OBJECT (model), "color-display", item);
  row = gimp_row_create_for_context (model,
                                     gimp_get_user_context (editor->gimp));
  g_object_weak_ref (G_OBJECT (row), (GWeakNotify) g_object_unref, model);

  hbox = _gimp_row_get_box (GIMP_ROW (row));

  eye = gimp_prop_toggle_new (G_OBJECT (item), "enabled",
                              GIMP_ICON_VISIBLE, NULL, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), eye, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), eye, 0);
  gtk_widget_set_visible (eye, TRUE);

  g_object_bind_property (item, "enabled",
                          gtk_bin_get_child (GTK_BIN (eye)), "visible",
                          G_BINDING_SYNC_CREATE);

  return row;
}

static void
gimp_color_display_editor_add_clicked (GtkWidget              *widget,
                                       GimpColorDisplayEditor *editor)
{
  GtkListBoxRow *row;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (editor->src_list));

  if (row)
    {
      GimpColorDisplayType *type;
      GimpColorDisplay     *display;

      type = GIMP_COLOR_DISPLAY_TYPE (gimp_row_get_viewable (GIMP_ROW (row)));

      display = g_object_new (gimp_color_display_type_get_gtype (type),
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
gimp_color_display_editor_remove_clicked (GtkWidget              *widget,
                                          GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_stack_remove (editor->stack, editor->selected);
}

static void
gimp_color_display_editor_up_clicked (GtkWidget              *widget,
                                      GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    {
      GtkListBoxRow *row;
      gint           index;

      row = gtk_list_box_get_selected_row (GTK_LIST_BOX (editor->dest_list));
      index = gtk_list_box_row_get_index (row);

      gimp_color_display_stack_reorder_up (editor->stack, editor->selected);

      row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (editor->dest_list),
                                           index - 1);
      if (row)
        gtk_list_box_select_row (GTK_LIST_BOX (editor->dest_list), row);
    }
}

static void
gimp_color_display_editor_down_clicked (GtkWidget              *widget,
                                        GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    {
      GtkListBoxRow *row;
      gint           index;

      row = gtk_list_box_get_selected_row (GTK_LIST_BOX (editor->dest_list));
      index = gtk_list_box_row_get_index (row);

      gimp_color_display_stack_reorder_down (editor->stack, editor->selected);

      row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (editor->dest_list),
                                           index + 1);
      if (row)
        gtk_list_box_select_row (GTK_LIST_BOX (editor->dest_list), row);
    }
}

static void
gimp_color_display_editor_reset_clicked (GtkWidget              *widget,
                                         GimpColorDisplayEditor *editor)
{
  if (editor->selected)
    gimp_color_display_configure_reset (editor->selected);
}

static void
gimp_color_display_editor_src_selected (GtkListBox             *list,
                                        GtkListBoxRow          *row,
                                        GimpColorDisplayEditor *editor)
{
  gchar *tip = NULL;

  if (row)
    {
      GimpViewable *viewable = gimp_row_get_viewable (GIMP_ROW (row));

      tip = g_strdup_printf (_("Add '%s' to the list of active filters"),
                             gimp_object_get_name (viewable));
    }

  gtk_widget_set_sensitive (editor->add_button, row != NULL);

  gimp_help_set_help_data (editor->add_button, tip, NULL);
  g_free (tip);
}

static void
gimp_color_display_editor_dest_selected (GtkListBox             *list,
                                         GtkListBoxRow          *row,
                                         GimpColorDisplayEditor *editor)
{
  GimpColorDisplay *display = NULL;
  gchar            *tip     = NULL;

  g_clear_weak_pointer (&editor->selected);

  if (row)
    {
      GimpViewable *viewable = gimp_row_get_viewable (GIMP_ROW (row));

      display = g_object_get_data (G_OBJECT (viewable), "color-display");

      tip = g_strdup_printf (_("Remove '%s' from the list of active filters"),
                             gimp_object_get_name (viewable));
    }

  gtk_widget_set_sensitive (editor->remove_button, display != NULL);
  gtk_widget_set_sensitive (editor->reset_button,  display != NULL);

  gimp_help_set_help_data (editor->remove_button, tip, NULL);
  g_free (tip);

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
      gtk_widget_set_visible (editor->config_widget, TRUE);

      g_object_add_weak_pointer (G_OBJECT (editor->config_widget),
                                 (gpointer) &editor->config_widget);
    }

  gimp_color_display_editor_update_buttons (editor);
}

static void
gimp_color_display_editor_items_changed (GListModel             *list,
                                         guint                   position,
                                         guint                   removed,
                                         guint                   added,
                                         GimpColorDisplayEditor *editor)
{
  gimp_color_display_editor_update_buttons (editor);
}

static void
gimp_color_display_editor_update_buttons (GimpColorDisplayEditor *editor)
{
  GtkListBoxRow *row;
  gboolean       up_sensitive   = FALSE;
  gboolean       down_sensitive = FALSE;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (editor->dest_list));

  if (row)
    {
      gint index = gtk_list_box_row_get_index (row);
      gint rows;

      rows = g_list_model_get_n_items (G_LIST_MODEL (editor->stack));

      up_sensitive   = index > 0;
      down_sensitive = index < rows - 1;
    }

  gtk_widget_set_sensitive (editor->up_button,   up_sensitive);
  gtk_widget_set_sensitive (editor->down_button, down_sensitive);
}
