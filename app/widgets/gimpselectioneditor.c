/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmachannel-select.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-color.h"
#include "core/ligmaselection.h"
#include "core/ligmatoolinfo.h"

/* FIXME: #include "tools/tools-types.h" */
#include "tools/tools-types.h"
#include "tools/ligmaregionselectoptions.h"

#include "ligmaselectioneditor.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmamenufactory.h"
#include "ligmaview.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


static void  ligma_selection_editor_docked_iface_init (LigmaDockedInterface *iface);

static void   ligma_selection_editor_constructed    (GObject             *object);

static void   ligma_selection_editor_set_image      (LigmaImageEditor     *editor,
                                                    LigmaImage           *image);

static void   ligma_selection_editor_set_context    (LigmaDocked          *docked,
                                                    LigmaContext         *context);

static gboolean ligma_selection_view_button_press   (GtkWidget           *widget,
                                                    GdkEventButton      *bevent,
                                                    LigmaSelectionEditor *editor);
static void   ligma_selection_editor_drop_color     (GtkWidget           *widget,
                                                    gint                 x,
                                                    gint                 y,
                                                    const LigmaRGB       *color,
                                                    gpointer             data);

static void   ligma_selection_editor_mask_changed   (LigmaImage           *image,
                                                    LigmaSelectionEditor *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaSelectionEditor, ligma_selection_editor,
                         LIGMA_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_selection_editor_docked_iface_init))

#define parent_class ligma_selection_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_selection_editor_class_init (LigmaSelectionEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  LigmaImageEditorClass *image_editor_class = LIGMA_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = ligma_selection_editor_constructed;

  image_editor_class->set_image = ligma_selection_editor_set_image;
}

static void
ligma_selection_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_selection_editor_set_context;
}

static void
ligma_selection_editor_init (LigmaSelectionEditor *editor)
{
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->view = ligma_view_new_by_types (NULL,
                                         LIGMA_TYPE_VIEW,
                                         LIGMA_TYPE_SELECTION,
                                         LIGMA_VIEW_SIZE_HUGE,
                                         0, TRUE);
  ligma_view_renderer_set_background (LIGMA_VIEW (editor->view)->renderer,
                                     LIGMA_ICON_TEXTURE);
  gtk_widget_set_size_request (editor->view,
                               LIGMA_VIEW_SIZE_HUGE, LIGMA_VIEW_SIZE_HUGE);
  ligma_view_set_expand (LIGMA_VIEW (editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (ligma_selection_view_button_press),
                    editor);

  ligma_dnd_color_dest_add (editor->view,
                           ligma_selection_editor_drop_color,
                           editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
ligma_selection_editor_constructed (GObject *object)
{
  LigmaSelectionEditor *editor = LIGMA_SELECTION_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->all_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "select",
                                   "select-all", NULL);

  editor->none_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "select",
                                   "select-none", NULL);

  editor->invert_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "select",
                                   "select-invert", NULL);

  editor->save_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "select",
                                   "select-save", NULL);

  editor->path_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);

  editor->stroke_button =
    ligma_editor_add_action_button (LIGMA_EDITOR (editor), "select",
                                   "select-stroke",
                                   "select-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
}

static void
ligma_selection_editor_set_image (LigmaImageEditor *image_editor,
                                 LigmaImage       *image)
{
  LigmaSelectionEditor *editor = LIGMA_SELECTION_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_selection_editor_mask_changed,
                                            editor);
    }

  LIGMA_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect (image, "mask-changed",
                        G_CALLBACK (ligma_selection_editor_mask_changed),
                        editor);

      ligma_view_set_viewable (LIGMA_VIEW (editor->view),
                              LIGMA_VIEWABLE (ligma_image_get_mask (image)));
    }
  else
    {
      ligma_view_set_viewable (LIGMA_VIEW (editor->view), NULL);
    }
}

static void
ligma_selection_editor_set_context (LigmaDocked  *docked,
                                   LigmaContext *context)
{
  LigmaSelectionEditor *editor = LIGMA_SELECTION_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  ligma_view_renderer_set_context (LIGMA_VIEW (editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
ligma_selection_editor_new (LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_SELECTION_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Selection>",
                       "ui-path",         "/selection-popup",
                       NULL);
}

static gboolean
ligma_selection_view_button_press (GtkWidget           *widget,
                                  GdkEventButton      *bevent,
                                  LigmaSelectionEditor *editor)
{
  LigmaImageEditor         *image_editor = LIGMA_IMAGE_EDITOR (editor);
  LigmaViewRenderer        *renderer;
  LigmaToolInfo            *tool_info;
  LigmaSelectionOptions    *sel_options;
  LigmaRegionSelectOptions *options;
  LigmaChannelOps           operation;
  GList                   *drawables;
  gint                     x, y;
  LigmaRGB                  color;

  if (! image_editor->image)
    return TRUE;

  renderer = LIGMA_VIEW (editor->view)->renderer;

  tool_info = ligma_get_tool_info (image_editor->image->ligma,
                                  "ligma-by-color-select-tool");

  if (! tool_info)
    return TRUE;

  sel_options = LIGMA_SELECTION_OPTIONS (tool_info->tool_options);
  options     = LIGMA_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawables = ligma_image_get_selected_drawables (image_editor->image);

  if (! drawables)
    return TRUE;

  operation = ligma_modifiers_to_channel_op (bevent->state);

  x = ligma_image_get_width  (image_editor->image) * bevent->x / renderer->width;
  y = ligma_image_get_height (image_editor->image) * bevent->y / renderer->height;

  if (ligma_image_pick_color (image_editor->image, drawables, x, y,
                             FALSE, options->sample_merged,
                             FALSE, 0.0,
                             NULL,
                             NULL, &color))
    {
      ligma_channel_select_by_color (ligma_image_get_mask (image_editor->image),
                                    drawables,
                                    options->sample_merged,
                                    &color,
                                    options->threshold / 255.0,
                                    options->select_transparent,
                                    options->select_criterion,
                                    operation,
                                    sel_options->antialias,
                                    sel_options->feather,
                                    sel_options->feather_radius,
                                    sel_options->feather_radius);
      ligma_image_flush (image_editor->image);
    }
  g_list_free (drawables);

  return TRUE;
}

static void
ligma_selection_editor_drop_color (GtkWidget     *widget,
                                  gint           x,
                                  gint           y,
                                  const LigmaRGB *color,
                                  gpointer       data)
{
  LigmaImageEditor         *editor = LIGMA_IMAGE_EDITOR (data);
  LigmaToolInfo            *tool_info;
  LigmaSelectionOptions    *sel_options;
  LigmaRegionSelectOptions *options;
  GList                   *drawables;

  if (! editor->image)
    return;

  tool_info = ligma_get_tool_info (editor->image->ligma,
                                  "ligma-by-color-select-tool");
  if (! tool_info)
    return;

  sel_options = LIGMA_SELECTION_OPTIONS (tool_info->tool_options);
  options     = LIGMA_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawables = ligma_image_get_selected_drawables (editor->image);

  if (! drawables)
    return;

  ligma_channel_select_by_color (ligma_image_get_mask (editor->image),
                                drawables,
                                options->sample_merged,
                                color,
                                options->threshold / 255.0,
                                options->select_transparent,
                                options->select_criterion,
                                sel_options->operation,
                                sel_options->antialias,
                                sel_options->feather,
                                sel_options->feather_radius,
                                sel_options->feather_radius);
  ligma_image_flush (editor->image);
  g_list_free (drawables);
}

static void
ligma_selection_editor_mask_changed (LigmaImage           *image,
                                    LigmaSelectionEditor *editor)
{
  ligma_view_renderer_invalidate (LIGMA_VIEW (editor->view)->renderer);
}
