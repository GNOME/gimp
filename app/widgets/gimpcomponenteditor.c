/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcomponenteditor.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"

#include "gimpcellrendererviewable.h"
#include "gimpcomponenteditor.h"
#include "gimpmenufactory.h"
#include "gimpviewrendererimage.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  COLUMN_CHANNEL,
  COLUMN_VISIBLE,
  COLUMN_RENDERER,
  COLUMN_NAME,
  NUM_COLUMNS
};


static void gimp_component_editor_class_init (GimpComponentEditorClass *klass);
static void gimp_component_editor_init       (GimpComponentEditor      *editor);

static void gimp_component_editor_unrealize         (GtkWidget           *widget);

static void gimp_component_editor_set_image         (GimpImageEditor     *editor,
                                                     GimpImage           *gimage);

static void gimp_component_editor_create_components (GimpComponentEditor *editor);
static void gimp_component_editor_clear_components  (GimpComponentEditor *editor);
static void gimp_component_editor_clicked         (GtkCellRendererToggle *cellrenderertoggle,
                                                   gchar                 *path,
                                                   GdkModifierType        state,
                                                   GimpComponentEditor   *editor);
static gboolean gimp_component_editor_select        (GtkTreeSelection    *selection,
                                                     GtkTreeModel        *model,
                                                     GtkTreePath         *path,
                                                     gboolean             path_currently_selected,
                                                     gpointer             data);
static gboolean gimp_component_editor_button_press  (GtkWidget           *widget,
                                                     GdkEventButton      *bevent,
                                                     GimpComponentEditor *editor);
static void gimp_component_editor_renderer_update   (GimpViewRenderer    *renderer,
                                                     GimpComponentEditor *editor);
static void gimp_component_editor_mode_changed      (GimpImage           *gimage,
                                                     GimpComponentEditor *editor);
static void gimp_component_editor_alpha_changed     (GimpImage           *gimage,
                                                     GimpComponentEditor *editor);
static void gimp_component_editor_visibility_changed(GimpImage           *gimage,
                                                     GimpChannelType      channel,
                                                     GimpComponentEditor *editor);
static void gimp_component_editor_active_changed    (GimpImage           *gimage,
                                                     GimpChannelType      channel,
                                                     GimpComponentEditor *editor);


static GimpImageEditorClass *parent_class = NULL;


GType
gimp_component_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpComponentEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_component_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpComponentEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_component_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_IMAGE_EDITOR,
                                            "GimpComponentEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_component_editor_class_init (GimpComponentEditorClass *klass)
{
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->unrealize = gimp_component_editor_unrealize;

  image_editor_class->set_image = gimp_component_editor_set_image;
}

static void
gimp_component_editor_init (GimpComponentEditor *editor)
{
  GtkWidget    *frame;
  GtkListStore *list;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  list = gtk_list_store_new (NUM_COLUMNS,
                             G_TYPE_INT,
                             G_TYPE_BOOLEAN,
                             GIMP_TYPE_VIEW_RENDERER,
                             G_TYPE_STRING);
  editor->model = GTK_TREE_MODEL (list);

  editor->view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (editor->model));
  g_object_unref (list);

  gtk_tree_view_set_headers_visible (editor->view, FALSE);

  editor->eye_column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (editor->view, editor->eye_column);

  editor->eye_cell = gimp_cell_renderer_toggle_new (GIMP_STOCK_VISIBLE);
  gtk_tree_view_column_pack_start (editor->eye_column, editor->eye_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (editor->eye_column, editor->eye_cell,
                                       "active", COLUMN_VISIBLE,
                                       NULL);

  g_signal_connect (editor->eye_cell, "clicked",
                    G_CALLBACK (gimp_component_editor_clicked),
                    editor);

  editor->renderer_cell = gimp_cell_renderer_viewable_new ();
  gtk_tree_view_insert_column_with_attributes (editor->view,
                                               -1, NULL,
                                               editor->renderer_cell,
                                               "renderer", COLUMN_RENDERER,
                                               NULL);

  gtk_tree_view_insert_column_with_attributes (editor->view,
                                               -1, NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (editor->view));
  gtk_widget_show (GTK_WIDGET (editor->view));

  g_signal_connect (editor->view, "button_press_event",
                    G_CALLBACK (gimp_component_editor_button_press),
                    editor);

  editor->selection = gtk_tree_view_get_selection (editor->view);
  gtk_tree_selection_set_mode (editor->selection, GTK_SELECTION_MULTIPLE);

  gtk_tree_selection_set_select_function (editor->selection,
                                          gimp_component_editor_select,
                                          editor, NULL);
}

static void
gimp_component_editor_unrealize (GtkWidget *widget)
{
  GimpComponentEditor *editor = GIMP_COMPONENT_EDITOR (widget);
  GtkTreeIter          iter;
  gboolean             iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      gimp_view_renderer_unrealize (renderer);
      g_object_unref (renderer);
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_component_editor_set_image (GimpImageEditor *editor,
                                 GimpImage       *gimage)
{
  GimpComponentEditor *component_editor;

  component_editor = GIMP_COMPONENT_EDITOR (editor);

  if (editor->gimage)
    {
      gimp_component_editor_clear_components (component_editor);

      g_signal_handlers_disconnect_by_func (editor->gimage,
                                            gimp_component_editor_mode_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->gimage,
                                            gimp_component_editor_alpha_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->gimage,
                                            gimp_component_editor_visibility_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->gimage,
                                            gimp_component_editor_active_changed,
                                            component_editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (editor, gimage);

  if (editor->gimage)
    {
      gimp_component_editor_create_components (component_editor);

      g_signal_connect (editor->gimage, "mode_changed",
                        G_CALLBACK (gimp_component_editor_mode_changed),
                        component_editor);
      g_signal_connect (editor->gimage, "alpha_changed",
                        G_CALLBACK (gimp_component_editor_alpha_changed),
                        component_editor);
      g_signal_connect (editor->gimage, "component_visibility_changed",
                        G_CALLBACK (gimp_component_editor_visibility_changed),
                        component_editor);
      g_signal_connect (editor->gimage, "component_active_changed",
                        G_CALLBACK (gimp_component_editor_active_changed),
                        component_editor);
    }
}

GtkWidget *
gimp_component_editor_new (gint             view_size,
                           GimpMenuFactory *menu_factory)
{
  GimpComponentEditor *editor;

  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  editor = g_object_new (GIMP_TYPE_COMPONENT_EDITOR,
                         "menu-factory",    menu_factory,
                         "menu-identifier", "<Channels>",
                         "ui-path",         "/channels-popup",
                         NULL);

  gimp_component_editor_set_preview_size (editor, view_size);

  return GTK_WIDGET (editor);
}

void
gimp_component_editor_set_preview_size (GimpComponentEditor *editor,
                                        gint                 view_size)
{
  GtkWidget   *tree_widget;
  GtkIconSize  icon_size;
  GtkTreeIter  iter;
  gboolean     iter_valid;

  g_return_if_fail (GIMP_IS_COMPONENT_EDITOR (editor));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE);

  tree_widget = GTK_WIDGET (editor->view);

  icon_size = gimp_get_icon_size (tree_widget,
                                  GIMP_STOCK_VISIBLE,
                                  GTK_ICON_SIZE_BUTTON,
                                  view_size -
                                  2 * tree_widget->style->xthickness,
                                  view_size -
                                  2 * tree_widget->style->ythickness);

  g_object_set (editor->eye_cell,
                "stock-size", icon_size,
                NULL);

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      GimpViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      gimp_view_renderer_set_size (renderer, view_size, 1);
      g_object_unref (renderer);
    }

  editor->preview_size = view_size;

  gtk_tree_view_columns_autosize (editor->view);
}

static void
gimp_component_editor_create_components (GimpComponentEditor *editor)
{
  GimpImage       *gimage;
  gint             n_components = 0;
  GimpChannelType  components[MAX_CHANNELS];
  GEnumClass      *enum_class;
  gint             i;

  gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

  switch (gimp_image_base_type (gimage))
    {
    case GIMP_RGB:
      n_components  = 3;
      components[0] = GIMP_RED_CHANNEL;
      components[1] = GIMP_GREEN_CHANNEL;
      components[2] = GIMP_BLUE_CHANNEL;
      break;

    case GIMP_GRAY:
      n_components  = 1;
      components[0] = GIMP_GRAY_CHANNEL;
      break;

    case GIMP_INDEXED:
      n_components  = 1;
      components[0] = GIMP_INDEXED_CHANNEL;
      break;
    }

  if (gimp_image_has_alpha (gimage))
    components[n_components++] = GIMP_ALPHA_CHANNEL;

  enum_class = g_type_class_ref (GIMP_TYPE_CHANNEL_TYPE);

  for (i = 0; i < n_components; i++)
    {
      GimpViewRenderer *renderer;
      GtkTreeIter       iter;
      GEnumValue       *enum_value;
      const gchar      *desc;
      gboolean          visible;

      visible = gimp_image_get_component_visible (gimage, components[i]);

      renderer = gimp_view_renderer_new (G_TYPE_FROM_INSTANCE (gimage),
                                         editor->preview_size, 1, FALSE);
      gimp_view_renderer_set_viewable (renderer, GIMP_VIEWABLE (gimage));
      gimp_view_renderer_remove_idle (renderer);

      GIMP_VIEW_RENDERER_IMAGE (renderer)->channel = components[i];

      g_signal_connect (renderer, "update",
                        G_CALLBACK (gimp_component_editor_renderer_update),
                        editor);

      enum_value = g_enum_get_value (enum_class, components[i]);
      desc = gimp_enum_value_get_desc (enum_class, enum_value);

      gtk_list_store_append (GTK_LIST_STORE (editor->model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_CHANNEL,  components[i],
                          COLUMN_VISIBLE,  visible,
                          COLUMN_RENDERER, renderer,
                          COLUMN_NAME,     desc,
                          -1);

      g_object_unref (renderer);

      if (gimp_image_get_component_active (gimage, components[i]))
        gtk_tree_selection_select_iter (editor->selection, &iter);
    }

  g_type_class_unref (enum_class);
}

static void
gimp_component_editor_clear_components (GimpComponentEditor *editor)
{
  gtk_list_store_clear (GTK_LIST_STORE (editor->model));

#ifdef __GNUC__
#warning FIXME: remove this hack as soon as bug #149906 is fixed
#endif
  g_object_set (editor->renderer_cell, "renderer", NULL, NULL);
}

static void
gimp_component_editor_clicked (GtkCellRendererToggle *cellrenderertoggle,
                               gchar                 *path_str,
                               GdkModifierType        state,
                               GimpComponentEditor   *editor)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (editor->model, &iter, path))
    {
      GimpImage       *gimage;
      GimpChannelType  channel;
      gboolean         active;

      gimage = GIMP_IMAGE_EDITOR (editor)->gimage;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);
      g_object_get (cellrenderertoggle,
                    "active", &active,
                    NULL);

      gimp_image_set_component_visible (gimage, channel, !active);
      gimp_image_flush (gimage);
    }

  gtk_tree_path_free (path);
}

static gboolean
gimp_component_editor_select (GtkTreeSelection *selection,
                              GtkTreeModel     *model,
                              GtkTreePath      *path,
                              gboolean          path_currently_selected,
                              gpointer          data)
{
  GimpComponentEditor *editor;
  GtkTreeIter          iter;
  GimpChannelType      channel;
  gboolean             active;

  editor = GIMP_COMPONENT_EDITOR (data);

  gtk_tree_model_get_iter (editor->model, &iter, path);
  gtk_tree_model_get (editor->model, &iter,
                      COLUMN_CHANNEL, &channel,
                      -1);

  active = gimp_image_get_component_active (GIMP_IMAGE_EDITOR (editor)->gimage,
                                            channel);

  return active != path_currently_selected;
}

static gboolean
gimp_component_editor_button_press (GtkWidget           *widget,
                                    GdkEventButton      *bevent,
                                    GimpComponentEditor *editor)
{
  GtkTreeViewColumn *column;
  GtkTreePath       *path;

  editor->clicked_component = -1;

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                     bevent->x,
                                     bevent->y,
                                     &path, &column, NULL, NULL))
    {
      GtkTreeIter     iter;
      GimpChannelType channel;
      gboolean        active;

      active = gtk_tree_selection_path_is_selected (editor->selection, path);

      gtk_tree_model_get_iter (editor->model, &iter, path);

      gtk_tree_path_free (path);

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);

      editor->clicked_component = channel;

      switch (bevent->button)
        {
        case 1:
          if (column != editor->eye_column && bevent->type == GDK_BUTTON_PRESS)
            {
              GimpImage *image = GIMP_IMAGE_EDITOR (editor)->gimage;

              gimp_image_set_component_active (image, channel, ! active);
              gimp_image_flush (image);
            }
          break;

        case 2:
          break;

        case 3:
          gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static gboolean
gimp_component_editor_get_iter (GimpComponentEditor *editor,
                                GimpChannelType      channel,
                                GtkTreeIter         *iter)
{
  gint index;

  index = gimp_image_get_component_index (GIMP_IMAGE_EDITOR (editor)->gimage,
                                          channel);

  if (index != -1)
    return gtk_tree_model_iter_nth_child (editor->model, iter, NULL, index);

  return FALSE;
}

static void
gimp_component_editor_renderer_update (GimpViewRenderer    *renderer,
                                       GimpComponentEditor *editor)
{
  GimpChannelType channel = GIMP_VIEW_RENDERER_IMAGE (renderer)->channel;
  GtkTreeIter     iter;

  if (gimp_component_editor_get_iter (editor, channel, &iter))
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (editor->model, &iter);
      gtk_tree_model_row_changed (editor->model, path, &iter);
      gtk_tree_path_free (path);
    }
}

static void
gimp_component_editor_mode_changed (GimpImage           *gimage,
                                    GimpComponentEditor *editor)
{
  gimp_component_editor_clear_components (editor);
  gimp_component_editor_create_components (editor);
}

static void
gimp_component_editor_alpha_changed (GimpImage           *gimage,
                                     GimpComponentEditor *editor)
{
  gimp_component_editor_clear_components (editor);
  gimp_component_editor_create_components (editor);
}

static void
gimp_component_editor_visibility_changed (GimpImage           *gimage,
                                          GimpChannelType      channel,
                                          GimpComponentEditor *editor)
{
  GtkTreeIter iter;

  if (gimp_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean visible;

      visible = gimp_image_get_component_visible (gimage, channel);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_VISIBLE, visible,
                          -1);
    }
}

static void
gimp_component_editor_active_changed (GimpImage           *gimage,
                                      GimpChannelType      channel,
                                      GimpComponentEditor *editor)
{
  GtkTreeIter iter;

  if (gimp_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean active;

      active = gimp_image_get_component_active (gimage, channel);

      if (gtk_tree_selection_iter_is_selected (editor->selection, &iter) !=
          active)
        {
          if (active)
            gtk_tree_selection_select_iter (editor->selection, &iter);
          else
            gtk_tree_selection_unselect_iter (editor->selection, &iter);
        }
    }
}
