/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpchannel-select.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpselection.h"
#include "core/gimptoolinfo.h"

/* FIXME: #include "tools/tools-types.h" */
#include "tools/tools-types.h"
#include "tools/gimpregionselectoptions.h"

#include "gimpselectioneditor.h"
#include "gimpdnd.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void  gimp_selection_editor_docked_iface_init (GimpDockedInterface *iface);

static void   gimp_selection_editor_constructed    (GObject             *object);

static void   gimp_selection_editor_set_image      (GimpImageEditor     *editor,
                                                    GimpImage           *image);

static void   gimp_selection_editor_set_context    (GimpDocked          *docked,
                                                    GimpContext         *context);

static gboolean gimp_selection_view_button_press   (GtkWidget           *widget,
                                                    GdkEventButton      *bevent,
                                                    GimpSelectionEditor *editor);
static void   gimp_selection_editor_drop_color     (GtkWidget           *widget,
                                                    gint                 x,
                                                    gint                 y,
                                                    const GimpRGB       *color,
                                                    gpointer             data);

static void   gimp_selection_editor_mask_changed   (GimpImage           *image,
                                                    GimpSelectionEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpSelectionEditor, gimp_selection_editor,
                         GIMP_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_selection_editor_docked_iface_init))

#define parent_class gimp_selection_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_selection_editor_class_init (GimpSelectionEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = gimp_selection_editor_constructed;

  image_editor_class->set_image = gimp_selection_editor_set_image;
}

static void
gimp_selection_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_selection_editor_set_context;
}

static void
gimp_selection_editor_init (GimpSelectionEditor *editor)
{
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->view = gimp_view_new_by_types (NULL,
                                         GIMP_TYPE_VIEW,
                                         GIMP_TYPE_SELECTION,
                                         GIMP_VIEW_SIZE_HUGE,
                                         0, TRUE);
  gimp_view_renderer_set_background (GIMP_VIEW (editor->view)->renderer,
                                     GIMP_ICON_TEXTURE);
  gtk_widget_set_size_request (editor->view,
                               GIMP_VIEW_SIZE_HUGE, GIMP_VIEW_SIZE_HUGE);
  gimp_view_set_expand (GIMP_VIEW (editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (gimp_selection_view_button_press),
                    editor);

  gimp_dnd_color_dest_add (editor->view,
                           gimp_selection_editor_drop_color,
                           editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
gimp_selection_editor_constructed (GObject *object)
{
  GimpSelectionEditor *editor = GIMP_SELECTION_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->all_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "select",
                                   "select-all", NULL);

  editor->none_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "select",
                                   "select-none", NULL);

  editor->invert_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "select",
                                   "select-invert", NULL);

  editor->save_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "select",
                                   "select-save", NULL);

  editor->path_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "vectors",
                                   "vectors-selection-to-vectors",
                                   "vectors-selection-to-vectors-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);

  editor->stroke_button =
    gimp_editor_add_action_button (GIMP_EDITOR (editor), "select",
                                   "select-stroke",
                                   "select-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
}

static void
gimp_selection_editor_set_image (GimpImageEditor *image_editor,
                                 GimpImage       *image)
{
  GimpSelectionEditor *editor = GIMP_SELECTION_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_selection_editor_mask_changed,
                                            editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect (image, "mask-changed",
                        G_CALLBACK (gimp_selection_editor_mask_changed),
                        editor);

      gimp_view_set_viewable (GIMP_VIEW (editor->view),
                              GIMP_VIEWABLE (gimp_image_get_mask (image)));
    }
  else
    {
      gimp_view_set_viewable (GIMP_VIEW (editor->view), NULL);
    }
}

static void
gimp_selection_editor_set_context (GimpDocked  *docked,
                                   GimpContext *context)
{
  GimpSelectionEditor *editor = GIMP_SELECTION_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
gimp_selection_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_SELECTION_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Selection>",
                       "ui-path",         "/selection-popup",
                       NULL);
}

static gboolean
gimp_selection_view_button_press (GtkWidget           *widget,
                                  GdkEventButton      *bevent,
                                  GimpSelectionEditor *editor)
{
  GimpImageEditor         *image_editor = GIMP_IMAGE_EDITOR (editor);
  GimpViewRenderer        *renderer;
  GimpToolInfo            *tool_info;
  GimpSelectionOptions    *sel_options;
  GimpRegionSelectOptions *options;
  GimpDrawable            *drawable;
  GimpChannelOps           operation;
  gint                     x, y;
  GimpRGB                  color;

  if (! image_editor->image)
    return TRUE;

  renderer = GIMP_VIEW (editor->view)->renderer;

  tool_info = gimp_get_tool_info (image_editor->image->gimp,
                                  "gimp-by-color-select-tool");

  if (! tool_info)
    return TRUE;

  sel_options = GIMP_SELECTION_OPTIONS (tool_info->tool_options);
  options     = GIMP_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawable = gimp_image_get_active_drawable (image_editor->image);

  if (! drawable)
    return TRUE;

  operation = gimp_modifiers_to_channel_op (bevent->state);

  x = gimp_image_get_width  (image_editor->image) * bevent->x / renderer->width;
  y = gimp_image_get_height (image_editor->image) * bevent->y / renderer->height;

  if (gimp_image_pick_color (image_editor->image, drawable, x, y,
                             FALSE, options->sample_merged,
                             FALSE, 0.0,
                             NULL,
                             NULL, &color))
    {
      gimp_channel_select_by_color (gimp_image_get_mask (image_editor->image),
                                    drawable,
                                    options->sample_merged,
                                    &color,
                                    options->threshold,
                                    options->select_transparent,
                                    options->select_criterion,
                                    operation,
                                    sel_options->antialias,
                                    sel_options->feather,
                                    sel_options->feather_radius,
                                    sel_options->feather_radius);
      gimp_image_flush (image_editor->image);
    }

  return TRUE;
}

static void
gimp_selection_editor_drop_color (GtkWidget     *widget,
                                  gint           x,
                                  gint           y,
                                  const GimpRGB *color,
                                  gpointer       data)
{
  GimpImageEditor         *editor = GIMP_IMAGE_EDITOR (data);
  GimpToolInfo            *tool_info;
  GimpSelectionOptions    *sel_options;
  GimpRegionSelectOptions *options;
  GimpDrawable            *drawable;

  if (! editor->image)
    return;

  tool_info = gimp_get_tool_info (editor->image->gimp,
                                  "gimp-by-color-select-tool");
  if (! tool_info)
    return;

  sel_options = GIMP_SELECTION_OPTIONS (tool_info->tool_options);
  options     = GIMP_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawable = gimp_image_get_active_drawable (editor->image);

  if (! drawable)
    return;

  gimp_channel_select_by_color (gimp_image_get_mask (editor->image),
                                drawable,
                                options->sample_merged,
                                color,
                                options->threshold,
                                options->select_transparent,
                                options->select_criterion,
                                sel_options->operation,
                                sel_options->antialias,
                                sel_options->feather,
                                sel_options->feather_radius,
                                sel_options->feather_radius);
  gimp_image_flush (editor->image);
}

static void
gimp_selection_editor_mask_changed (GimpImage           *image,
                                    GimpSelectionEditor *editor)
{
  gimp_view_renderer_invalidate (GIMP_VIEW (editor->view)->renderer);
}
