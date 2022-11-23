/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacomponenteditor.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmachannel.h"
#include "core/ligmaimage.h"

#include "ligmacellrendererviewable.h"
#include "ligmacomponenteditor.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmaviewrendererimage.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  COLUMN_CHANNEL,
  COLUMN_VISIBLE,
  COLUMN_RENDERER,
  COLUMN_NAME,
  N_COLUMNS
};


static void ligma_component_editor_docked_iface_init (LigmaDockedInterface *iface);

static void ligma_component_editor_set_context       (LigmaDocked          *docked,
                                                     LigmaContext         *context);

static void ligma_component_editor_set_image         (LigmaImageEditor     *editor,
                                                     LigmaImage           *image);

static void ligma_component_editor_create_components (LigmaComponentEditor *editor);
static void ligma_component_editor_clear_components  (LigmaComponentEditor *editor);
static void ligma_component_editor_clicked         (GtkCellRendererToggle *cellrenderertoggle,
                                                   gchar                 *path,
                                                   GdkModifierType        state,
                                                   LigmaComponentEditor   *editor);
static gboolean ligma_component_editor_select        (GtkTreeSelection    *selection,
                                                     GtkTreeModel        *model,
                                                     GtkTreePath         *path,
                                                     gboolean             path_currently_selected,
                                                     gpointer             data);
static gboolean ligma_component_editor_button_press  (GtkWidget           *widget,
                                                     GdkEventButton      *bevent,
                                                     LigmaComponentEditor *editor);
static void ligma_component_editor_renderer_update   (LigmaViewRenderer    *renderer,
                                                     LigmaComponentEditor *editor);
static void ligma_component_editor_mode_changed      (LigmaImage           *image,
                                                     LigmaComponentEditor *editor);
static void ligma_component_editor_alpha_changed     (LigmaImage           *image,
                                                     LigmaComponentEditor *editor);
static void ligma_component_editor_visibility_changed(LigmaImage           *image,
                                                     LigmaChannelType      channel,
                                                     LigmaComponentEditor *editor);
static void ligma_component_editor_active_changed    (LigmaImage           *image,
                                                     LigmaChannelType      channel,
                                                     LigmaComponentEditor *editor);
static LigmaImage * ligma_component_editor_drag_component (GtkWidget       *widget,
                                                         LigmaContext    **context,
                                                         LigmaChannelType *channel,
                                                         gpointer         data);


G_DEFINE_TYPE_WITH_CODE (LigmaComponentEditor, ligma_component_editor,
                         LIGMA_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_component_editor_docked_iface_init))

#define parent_class ligma_component_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_component_editor_class_init (LigmaComponentEditorClass *klass)
{
  LigmaImageEditorClass *image_editor_class = LIGMA_IMAGE_EDITOR_CLASS (klass);

  image_editor_class->set_image = ligma_component_editor_set_image;
}

static void
ligma_component_editor_init (LigmaComponentEditor *editor)
{
  GtkWidget    *frame;
  GtkListStore *list;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  list = gtk_list_store_new (N_COLUMNS,
                             G_TYPE_INT,
                             G_TYPE_BOOLEAN,
                             LIGMA_TYPE_VIEW_RENDERER,
                             G_TYPE_STRING);
  editor->model = GTK_TREE_MODEL (list);

  editor->view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (editor->model));
  g_object_unref (list);

  gtk_tree_view_set_headers_visible (editor->view, FALSE);

  editor->eye_column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (editor->view, editor->eye_column);

  editor->eye_cell = ligma_cell_renderer_toggle_new (LIGMA_ICON_VISIBLE);
  gtk_tree_view_column_pack_start (editor->eye_column, editor->eye_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (editor->eye_column, editor->eye_cell,
                                       "active", COLUMN_VISIBLE,
                                       NULL);

  g_signal_connect (editor->eye_cell, "clicked",
                    G_CALLBACK (ligma_component_editor_clicked),
                    editor);

  editor->renderer_cell = ligma_cell_renderer_viewable_new ();
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

  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (ligma_component_editor_button_press),
                    editor);

  editor->selection = gtk_tree_view_get_selection (editor->view);
  gtk_tree_selection_set_mode (editor->selection, GTK_SELECTION_MULTIPLE);

  gtk_tree_selection_set_select_function (editor->selection,
                                          ligma_component_editor_select,
                                          editor, NULL);

  ligma_dnd_component_source_add (GTK_WIDGET (editor->view),
                                 ligma_component_editor_drag_component,
                                 editor);
}

static void
ligma_component_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_component_editor_set_context;
}

static void
ligma_component_editor_set_context (LigmaDocked  *docked,
                                   LigmaContext *context)
{
  LigmaComponentEditor *editor = LIGMA_COMPONENT_EDITOR (docked);
  GtkTreeIter          iter;
  gboolean             iter_valid;

  parent_docked_iface->set_context (docked, context);

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      LigmaViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      ligma_view_renderer_set_context (renderer, context);
      g_object_unref (renderer);
    }
}

static void
ligma_component_editor_set_image (LigmaImageEditor *editor,
                                 LigmaImage       *image)
{
  LigmaComponentEditor *component_editor = LIGMA_COMPONENT_EDITOR (editor);

  if (editor->image)
    {
      ligma_component_editor_clear_components (component_editor);

      g_signal_handlers_disconnect_by_func (editor->image,
                                            ligma_component_editor_mode_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            ligma_component_editor_alpha_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            ligma_component_editor_visibility_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            ligma_component_editor_active_changed,
                                            component_editor);
    }

  LIGMA_IMAGE_EDITOR_CLASS (parent_class)->set_image (editor, image);

  if (editor->image)
    {
      ligma_component_editor_create_components (component_editor);

      g_signal_connect (editor->image, "mode-changed",
                        G_CALLBACK (ligma_component_editor_mode_changed),
                        component_editor);
      g_signal_connect (editor->image, "alpha-changed",
                        G_CALLBACK (ligma_component_editor_alpha_changed),
                        component_editor);
      g_signal_connect (editor->image, "component-visibility-changed",
                        G_CALLBACK (ligma_component_editor_visibility_changed),
                        component_editor);
      g_signal_connect (editor->image, "component-active-changed",
                        G_CALLBACK (ligma_component_editor_active_changed),
                        component_editor);
    }
}

GtkWidget *
ligma_component_editor_new (gint             view_size,
                           LigmaMenuFactory *menu_factory)
{
  LigmaComponentEditor *editor;

  g_return_val_if_fail (view_size > 0 &&
                        view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  editor = g_object_new (LIGMA_TYPE_COMPONENT_EDITOR,
                         "menu-factory",    menu_factory,
                         "menu-identifier", "<Channels>",
                         "ui-path",         "/channels-popup",
                         NULL);

  ligma_component_editor_set_view_size (editor, view_size);

  return GTK_WIDGET (editor);
}

void
ligma_component_editor_set_view_size (LigmaComponentEditor *editor,
                                     gint                 view_size)
{
  GtkWidget       *tree_widget;
  GtkStyleContext *tree_style;
  GtkBorder        border;
  GtkTreeIter      iter;
  gboolean         iter_valid;
  gint             icon_size;

  g_return_if_fail (LIGMA_IS_COMPONENT_EDITOR (editor));
  g_return_if_fail (view_size >  0 &&
                    view_size <= LIGMA_VIEWABLE_MAX_PREVIEW_SIZE);

  tree_widget = GTK_WIDGET (editor->view);
  tree_style  = gtk_widget_get_style_context (tree_widget);

  gtk_style_context_save (tree_style);
  gtk_style_context_add_class (tree_style, GTK_STYLE_CLASS_BUTTON);
  gtk_style_context_get_border (tree_style, 0, &border);
  gtk_style_context_restore (tree_style);

  g_object_get (editor->eye_cell, "icon-size", &icon_size, NULL);
  icon_size = MIN (icon_size, MAX (view_size - (border.left + border.right),
                                   view_size - (border.top + border.bottom)));
  g_object_set (editor->eye_cell, "icon-size", icon_size, NULL);

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      LigmaViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      ligma_view_renderer_set_size (renderer, view_size, 1);
      g_object_unref (renderer);
    }

  editor->view_size = view_size;

  gtk_tree_view_columns_autosize (editor->view);
}

static void
ligma_component_editor_create_components (LigmaComponentEditor *editor)
{
  LigmaImage       *image        = LIGMA_IMAGE_EDITOR (editor)->image;
  gint             n_components = 0;
  LigmaChannelType  components[MAX_CHANNELS];
  GEnumClass      *enum_class;
  gint             i;

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
      n_components  = 3;
      components[0] = LIGMA_CHANNEL_RED;
      components[1] = LIGMA_CHANNEL_GREEN;
      components[2] = LIGMA_CHANNEL_BLUE;
      break;

    case LIGMA_GRAY:
      n_components  = 1;
      components[0] = LIGMA_CHANNEL_GRAY;
      break;

    case LIGMA_INDEXED:
      n_components  = 1;
      components[0] = LIGMA_CHANNEL_INDEXED;
      break;
    }

  if (ligma_image_has_alpha (image))
    components[n_components++] = LIGMA_CHANNEL_ALPHA;

  enum_class = g_type_class_ref (LIGMA_TYPE_CHANNEL_TYPE);

  for (i = 0; i < n_components; i++)
    {
      LigmaViewRenderer *renderer;
      GtkTreeIter       iter;
      GEnumValue       *enum_value;
      const gchar      *desc;
      gboolean          visible;

      visible = ligma_image_get_component_visible (image, components[i]);

      renderer = ligma_view_renderer_new (LIGMA_IMAGE_EDITOR (editor)->context,
                                         G_TYPE_FROM_INSTANCE (image),
                                         editor->view_size, 1, FALSE);
      ligma_view_renderer_set_viewable (renderer, LIGMA_VIEWABLE (image));
      ligma_view_renderer_remove_idle (renderer);

      LIGMA_VIEW_RENDERER_IMAGE (renderer)->channel = components[i];

      g_signal_connect (renderer, "update",
                        G_CALLBACK (ligma_component_editor_renderer_update),
                        editor);

      enum_value = g_enum_get_value (enum_class, components[i]);
      desc = ligma_enum_value_get_desc (enum_class, enum_value);

      gtk_list_store_append (GTK_LIST_STORE (editor->model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_CHANNEL,  components[i],
                          COLUMN_VISIBLE,  visible,
                          COLUMN_RENDERER, renderer,
                          COLUMN_NAME,     desc,
                          -1);

      g_object_unref (renderer);

      if (ligma_image_get_component_active (image, components[i]))
        gtk_tree_selection_select_iter (editor->selection, &iter);
    }

  g_type_class_unref (enum_class);
}

static void
ligma_component_editor_clear_components (LigmaComponentEditor *editor)
{
  gtk_list_store_clear (GTK_LIST_STORE (editor->model));

  /*  Clear the renderer so that it don't reference the viewable.
   *  See bug #149906.
   */
  g_object_set (editor->renderer_cell, "renderer", NULL, NULL);
}

static void
ligma_component_editor_clicked (GtkCellRendererToggle *cellrenderertoggle,
                               gchar                 *path_str,
                               GdkModifierType        state,
                               LigmaComponentEditor   *editor)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (editor->model, &iter, path))
    {
      LigmaImage       *image = LIGMA_IMAGE_EDITOR (editor)->image;
      LigmaChannelType  channel;
      gboolean         active;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);
      g_object_get (cellrenderertoggle,
                    "active", &active,
                    NULL);

      ligma_image_set_component_visible (image, channel, !active);
      ligma_image_flush (image);
    }

  gtk_tree_path_free (path);
}

static gboolean
ligma_component_editor_select (GtkTreeSelection *selection,
                              GtkTreeModel     *model,
                              GtkTreePath      *path,
                              gboolean          path_currently_selected,
                              gpointer          data)
{
  LigmaComponentEditor *editor = LIGMA_COMPONENT_EDITOR (data);
  GtkTreeIter          iter;
  LigmaChannelType      channel;
  gboolean             active;

  gtk_tree_model_get_iter (editor->model, &iter, path);
  gtk_tree_model_get (editor->model, &iter,
                      COLUMN_CHANNEL, &channel,
                      -1);

  active = ligma_image_get_component_active (LIGMA_IMAGE_EDITOR (editor)->image,
                                            channel);

  return active != path_currently_selected;
}

static gboolean
ligma_component_editor_button_press (GtkWidget           *widget,
                                    GdkEventButton      *bevent,
                                    LigmaComponentEditor *editor)
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
      LigmaChannelType channel;
      gboolean        active;

      active = gtk_tree_selection_path_is_selected (editor->selection, path);

      gtk_tree_model_get_iter (editor->model, &iter, path);

      gtk_tree_path_free (path);

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);

      editor->clicked_component = channel;

      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (editor), (GdkEvent *) bevent);
        }
      else if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1 &&
               column != editor->eye_column)
        {
          LigmaImage *image = LIGMA_IMAGE_EDITOR (editor)->image;

          ligma_image_set_component_active (image, channel, ! active);
          ligma_image_flush (image);
        }
    }

  return FALSE;
}

static gboolean
ligma_component_editor_get_iter (LigmaComponentEditor *editor,
                                LigmaChannelType      channel,
                                GtkTreeIter         *iter)
{
  gint index;

  index = ligma_image_get_component_index (LIGMA_IMAGE_EDITOR (editor)->image,
                                          channel);

  if (index != -1)
    return gtk_tree_model_iter_nth_child (editor->model, iter, NULL, index);

  return FALSE;
}

static void
ligma_component_editor_renderer_update (LigmaViewRenderer    *renderer,
                                       LigmaComponentEditor *editor)
{
  LigmaChannelType channel = LIGMA_VIEW_RENDERER_IMAGE (renderer)->channel;
  GtkTreeIter     iter;

  if (ligma_component_editor_get_iter (editor, channel, &iter))
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (editor->model, &iter);
      gtk_tree_model_row_changed (editor->model, path, &iter);
      gtk_tree_path_free (path);
    }
}

static void
ligma_component_editor_mode_changed (LigmaImage           *image,
                                    LigmaComponentEditor *editor)
{
  ligma_component_editor_clear_components (editor);
  ligma_component_editor_create_components (editor);
}

static void
ligma_component_editor_alpha_changed (LigmaImage           *image,
                                     LigmaComponentEditor *editor)
{
  ligma_component_editor_clear_components (editor);
  ligma_component_editor_create_components (editor);
}

static void
ligma_component_editor_visibility_changed (LigmaImage           *image,
                                          LigmaChannelType      channel,
                                          LigmaComponentEditor *editor)
{
  GtkTreeIter iter;

  if (ligma_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean visible = ligma_image_get_component_visible (image, channel);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_VISIBLE, visible,
                          -1);
    }
}

static void
ligma_component_editor_active_changed (LigmaImage           *image,
                                      LigmaChannelType      channel,
                                      LigmaComponentEditor *editor)
{
  GtkTreeIter iter;

  if (ligma_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean active = ligma_image_get_component_active (image, channel);

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

static LigmaImage *
ligma_component_editor_drag_component (GtkWidget        *widget,
                                      LigmaContext     **context,
                                      LigmaChannelType  *channel,
                                      gpointer          data)
{
  LigmaComponentEditor *editor = LIGMA_COMPONENT_EDITOR (data);

  if (LIGMA_IMAGE_EDITOR (editor)->image &&
      editor->clicked_component != -1)
    {
      if (channel)
        *channel = editor->clicked_component;

      if (context)
        *context = LIGMA_IMAGE_EDITOR (editor)->context;

      return LIGMA_IMAGE_EDITOR (editor)->image;
    }

  return NULL;
}
