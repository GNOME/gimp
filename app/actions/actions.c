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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpactionfactory.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimpimageeditor.h"
#include "widgets/gimpitemtreeview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"
#include "display/gimpnavigationeditor.h"
#include "display/gimpstatusbar.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "brush-editor-actions.h"
#include "brushes-actions.h"
#include "buffers-actions.h"
#include "channels-actions.h"
#include "colormap-actions.h"
#include "config-actions.h"
#include "context-actions.h"
#include "cursor-info-actions.h"
#include "debug-actions.h"
#include "dialogs-actions.h"
#include "dock-actions.h"
#include "dockable-actions.h"
#include "documents-actions.h"
#include "drawable-actions.h"
#include "dynamics-actions.h"
#include "dynamics-editor-actions.h"
#include "edit-actions.h"
#include "error-console-actions.h"
#include "file-actions.h"
#include "fonts-actions.h"
#include "gradient-editor-actions.h"
#include "gradients-actions.h"
#include "help-actions.h"
#include "image-actions.h"
#include "images-actions.h"
#include "layers-actions.h"
#include "palette-editor-actions.h"
#include "palettes-actions.h"
#include "patterns-actions.h"
#include "plug-in-actions.h"
#include "quick-mask-actions.h"
#include "sample-points-actions.h"
#include "select-actions.h"
#include "templates-actions.h"
#include "text-editor-actions.h"
#include "text-tool-actions.h"
#include "tool-options-actions.h"
#include "tool-presets-actions.h"
#include "tool-preset-editor-actions.h"
#include "tools-actions.h"
#include "vectors-actions.h"
#include "view-actions.h"
#include "windows-actions.h"

#include "gimp-intl.h"


/*  global variables  */

GimpActionFactory *global_action_factory = NULL;


/*  private variables  */

static const GimpActionFactoryEntry action_groups[] =
{
  { "brush-editor", N_("Brush Editor"), GIMP_STOCK_BRUSH,
    brush_editor_actions_setup,
    brush_editor_actions_update },
  { "brushes", N_("Brushes"), GIMP_STOCK_BRUSH,
    brushes_actions_setup,
    brushes_actions_update },
  { "buffers", N_("Buffers"), GIMP_STOCK_BUFFER,
    buffers_actions_setup,
    buffers_actions_update },
  { "channels", N_("Channels"), GIMP_STOCK_CHANNEL,
    channels_actions_setup,
    channels_actions_update },
  { "colormap", N_("Colormap"), GIMP_STOCK_COLORMAP,
    colormap_actions_setup,
    colormap_actions_update },
  { "config", N_("Configuration"), GTK_STOCK_PREFERENCES,
    config_actions_setup,
    config_actions_update },
  { "context", N_("Context"), GIMP_STOCK_TOOL_OPTIONS /* well... */,
    context_actions_setup,
    context_actions_update },
  { "cursor-info", N_("Pointer Information"), NULL,
    cursor_info_actions_setup,
    cursor_info_actions_update },
  { "debug", N_("Debug"), NULL,
    debug_actions_setup,
    debug_actions_update },
  { "dialogs", N_("Dialogs"), NULL,
    dialogs_actions_setup,
    dialogs_actions_update },
  { "dock", N_("Dock"), NULL,
    dock_actions_setup,
    dock_actions_update },
  { "dockable", N_("Dockable"), NULL,
    dockable_actions_setup,
    dockable_actions_update },
  { "documents", N_("Document History"), NULL,
    documents_actions_setup,
    documents_actions_update },
  { "drawable", N_("Drawable"), GIMP_STOCK_LAYER,
    drawable_actions_setup,
    drawable_actions_update },
  { "dynamics", N_("Paint Dynamics"), GIMP_STOCK_DYNAMICS,
    dynamics_actions_setup,
    dynamics_actions_update },
  { "dynamics-editor", N_("Paint Dynamics Editor"), GIMP_STOCK_DYNAMICS,
    dynamics_editor_actions_setup,
    dynamics_editor_actions_update },
  { "edit", N_("Edit"), GTK_STOCK_EDIT,
    edit_actions_setup,
    edit_actions_update },
  { "error-console", N_("Error Console"), GIMP_STOCK_WARNING,
    error_console_actions_setup,
    error_console_actions_update },
  { "file", N_("File"), GTK_STOCK_FILE,
    file_actions_setup,
    file_actions_update },
  { "fonts", N_("Fonts"), GIMP_STOCK_FONT,
    fonts_actions_setup,
    fonts_actions_update },
  { "gradient-editor", N_("Gradient Editor"), GIMP_STOCK_GRADIENT,
    gradient_editor_actions_setup,
    gradient_editor_actions_update },
  { "gradients", N_("Gradients"), GIMP_STOCK_GRADIENT,
    gradients_actions_setup,
    gradients_actions_update },
  { "tool-presets", N_("Tool Presets"), GIMP_STOCK_TOOL_PRESET,
    tool_presets_actions_setup,
    tool_presets_actions_update },
  { "tool-preset-editor", N_("Tool Preset Editor"), GIMP_STOCK_TOOL_PRESET,
    tool_preset_editor_actions_setup,
    tool_preset_editor_actions_update },
  { "help", N_("Help"), GTK_STOCK_HELP,
    help_actions_setup,
    help_actions_update },
  { "image", N_("Image"), GIMP_STOCK_IMAGE,
    image_actions_setup,
    image_actions_update },
  { "images", N_("Images"), GIMP_STOCK_IMAGE,
    images_actions_setup,
    images_actions_update },
  { "layers", N_("Layers"), GIMP_STOCK_LAYER,
    layers_actions_setup,
    layers_actions_update },
  { "palette-editor", N_("Palette Editor"), GIMP_STOCK_PALETTE,
    palette_editor_actions_setup,
    palette_editor_actions_update },
  { "palettes", N_("Palettes"), GIMP_STOCK_PALETTE,
    palettes_actions_setup,
    palettes_actions_update },
  { "patterns", N_("Patterns"), GIMP_STOCK_PATTERN,
    patterns_actions_setup,
    patterns_actions_update },
  { "plug-in", N_("Plug-Ins"), GIMP_STOCK_PLUGIN,
    plug_in_actions_setup,
    plug_in_actions_update },
  { "quick-mask", N_("Quick Mask"), GIMP_STOCK_QUICK_MASK_ON,
    quick_mask_actions_setup,
    quick_mask_actions_update },
  { "sample-points", N_("Sample Points"), GIMP_STOCK_SAMPLE_POINT,
    sample_points_actions_setup,
    sample_points_actions_update },
  { "select", N_("Select"), GIMP_STOCK_SELECTION,
    select_actions_setup,
    select_actions_update },
  { "templates", N_("Templates"), GIMP_STOCK_TEMPLATE,
    templates_actions_setup,
    templates_actions_update },
  { "text-tool", N_("Text Tool"), GTK_STOCK_EDIT,
    text_tool_actions_setup,
    text_tool_actions_update },
  { "text-editor", N_("Text Editor"), GTK_STOCK_EDIT,
    text_editor_actions_setup,
    text_editor_actions_update },
  { "tool-options", N_("Tool Options"), GIMP_STOCK_TOOL_OPTIONS,
    tool_options_actions_setup,
    tool_options_actions_update },
  { "tools", N_("Tools"), GIMP_STOCK_TOOLS,
    tools_actions_setup,
    tools_actions_update },
  { "vectors", N_("Paths"), GIMP_STOCK_PATH,
    vectors_actions_setup,
    vectors_actions_update },
  { "view", N_("View"), GIMP_STOCK_VISIBLE,
    view_actions_setup,
    view_actions_update },
  { "windows", N_("Windows"), NULL,
    windows_actions_setup,
    windows_actions_update }
};


/*  public functions  */

void
actions_init (Gimp *gimp)
{
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (global_action_factory == NULL);

  global_action_factory = gimp_action_factory_new (gimp);

  for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
    gimp_action_factory_group_register (global_action_factory,
                                        action_groups[i].identifier,
                                        gettext (action_groups[i].label),
                                        action_groups[i].stock_id,
                                        action_groups[i].setup_func,
                                        action_groups[i].update_func);
}

void
actions_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (global_action_factory != NULL);
  g_return_if_fail (global_action_factory->gimp == gimp);

  g_object_unref (global_action_factory);
  global_action_factory = NULL;
}

Gimp *
action_data_get_gimp (gpointer data)
{
  GimpContext *context = NULL;

  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return ((GimpDisplay *) data)->gimp;
  else if (GIMP_IS_IMAGE_WINDOW (data))
    {
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (data);
      return shell ? shell->display->gimp : NULL;
    }
  else if (GIMP_IS_GIMP (data))
    return data;
  else if (GIMP_IS_DOCK (data))
    context = gimp_dock_get_context (((GimpDock *) data));
  else if (GIMP_IS_DOCK_WINDOW (data))
    context = gimp_dock_window_get_context (((GimpDockWindow *) data));
  else if (GIMP_IS_CONTAINER_VIEW (data))
    context = gimp_container_view_get_context ((GimpContainerView *) data);
  else if (GIMP_IS_CONTAINER_EDITOR (data))
    context = gimp_container_view_get_context (((GimpContainerEditor *) data)->view);
  else if (GIMP_IS_IMAGE_EDITOR (data))
    context = ((GimpImageEditor *) data)->context;
  else if (GIMP_IS_NAVIGATION_EDITOR (data))
    context = ((GimpNavigationEditor *) data)->context;

  if (context)
    return context->gimp;

  return NULL;
}

GimpContext *
action_data_get_context (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return gimp_get_user_context (((GimpDisplay *) data)->gimp);
  else if (GIMP_IS_IMAGE_WINDOW (data))
    {
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (data);
      return shell ? gimp_get_user_context (shell->display->gimp) : NULL;
    }
  else if (GIMP_IS_GIMP (data))
    return gimp_get_user_context (data);
  else if (GIMP_IS_DOCK (data))
    return gimp_dock_get_context ((GimpDock *) data);
  else if (GIMP_IS_DOCK_WINDOW (data))
    return gimp_dock_window_get_context (((GimpDockWindow *) data));
  else if (GIMP_IS_CONTAINER_VIEW (data))
    return gimp_container_view_get_context ((GimpContainerView *) data);
  else if (GIMP_IS_CONTAINER_EDITOR (data))
    return gimp_container_view_get_context (((GimpContainerEditor *) data)->view);
  else if (GIMP_IS_IMAGE_EDITOR (data))
    return ((GimpImageEditor *) data)->context;
  else if (GIMP_IS_NAVIGATION_EDITOR (data))
    return ((GimpNavigationEditor *) data)->context;

  return NULL;
}

GimpImage *
action_data_get_image (gpointer data)
{
  GimpContext *context = NULL;
  GimpDisplay *display = NULL;

  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    display = (GimpDisplay *) data;
  else if (GIMP_IS_IMAGE_WINDOW (data))
    {
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (data);
      display = shell ? shell->display : NULL;
    }
  else if (GIMP_IS_GIMP (data))
    context = gimp_get_user_context (data);
  else if (GIMP_IS_DOCK (data))
    context = gimp_dock_get_context ((GimpDock *) data);
  else if (GIMP_IS_DOCK_WINDOW (data))
    context = gimp_dock_window_get_context (((GimpDockWindow *) data));
  else if (GIMP_IS_ITEM_TREE_VIEW (data))
    return gimp_item_tree_view_get_image ((GimpItemTreeView *) data);
  else if (GIMP_IS_IMAGE_EDITOR (data))
    return ((GimpImageEditor *) data)->image;
  else if (GIMP_IS_NAVIGATION_EDITOR (data))
    context = ((GimpNavigationEditor *) data)->context;

  if (context)
    return gimp_context_get_image (context);
  else if (display)
    return gimp_display_get_image (display);

  return NULL;
}

GimpDisplay *
action_data_get_display (gpointer data)
{
  GimpContext *context = NULL;

  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return data;
  else if (GIMP_IS_IMAGE_WINDOW (data))
    {
      GimpDisplayShell *shell = gimp_image_window_get_active_shell (data);
      return shell ? shell->display : NULL;
    }
  else if (GIMP_IS_GIMP (data))
    context = gimp_get_user_context (data);
  else if (GIMP_IS_DOCK (data))
    context = gimp_dock_get_context ((GimpDock *) data);
  else if (GIMP_IS_DOCK_WINDOW (data))
    context = gimp_dock_window_get_context (((GimpDockWindow *) data));
  else if (GIMP_IS_NAVIGATION_EDITOR (data))
    context = ((GimpNavigationEditor *) data)->context;

  if (context)
    return gimp_context_get_display (context);

  return NULL;
}

GimpDisplayShell *
action_data_get_shell (gpointer data)
{
  GimpDisplay      *display = NULL;
  GimpDisplayShell *shell   = NULL;

  display = action_data_get_display (data);

  if (display)
    shell = gimp_display_get_shell (display);

  return shell;
}

GtkWidget *
action_data_get_widget (gpointer data)
{
  GimpDisplay *display = NULL;

  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    display = data;
  else if (GIMP_IS_GIMP (data))
    display = gimp_context_get_display (gimp_get_user_context (data));
  else if (GTK_IS_WIDGET (data))
    return data;

  if (display)
    return GTK_WIDGET (gimp_display_get_shell (display));

  return dialogs_get_toolbox ();
}

gint
action_data_sel_count (gpointer data)
{
  if (GIMP_IS_CONTAINER_EDITOR (data))
    {
      GimpContainerEditor  *editor;

      editor = GIMP_CONTAINER_EDITOR (data);
      return gimp_container_view_get_selected (editor->view, NULL);
    }
  else
    {
      return 0;
    }
}

gdouble
action_select_value (GimpActionSelectType  select_type,
                     gdouble               value,
                     gdouble               min,
                     gdouble               max,
                     gdouble               def,
                     gdouble               small_inc,
                     gdouble               inc,
                     gdouble               skip_inc,
                     gdouble               delta_factor,
                     gboolean              wrap)
{
  switch (select_type)
    {
    case GIMP_ACTION_SELECT_SET_TO_DEFAULT:
      value = def;
      break;

    case GIMP_ACTION_SELECT_FIRST:
      value = min;
      break;

    case GIMP_ACTION_SELECT_LAST:
      value = max;
      break;

    case GIMP_ACTION_SELECT_SMALL_PREVIOUS:
      value -= small_inc;
      break;

    case GIMP_ACTION_SELECT_SMALL_NEXT:
      value += small_inc;
      break;

    case GIMP_ACTION_SELECT_PREVIOUS:
      value -= inc;
      break;

    case GIMP_ACTION_SELECT_NEXT:
      value += inc;
      break;

    case GIMP_ACTION_SELECT_SKIP_PREVIOUS:
      value -= skip_inc;
      break;

    case GIMP_ACTION_SELECT_SKIP_NEXT:
      value += skip_inc;
      break;

    case GIMP_ACTION_SELECT_PERCENT_PREVIOUS:
      g_return_val_if_fail (delta_factor >= 0.0, value);
      value /= (1.0 + delta_factor);
      break;

    case GIMP_ACTION_SELECT_PERCENT_NEXT:
      g_return_val_if_fail (delta_factor >= 0.0, value);
      value *= (1.0 + delta_factor);
      break;

    default:
      if ((gint) select_type >= 0)
        value = (gdouble) select_type * (max - min) / 1000.0 + min;
      else
        g_return_val_if_reached (value);
      break;
    }

  if (wrap)
    {
      while (value < min)
        value = max - (min - value);

      while (value > max)
        value = min + (value - max);
    }
  else
    {
      value = CLAMP (value, min, max);
    }

  return value;
}

void
action_select_property (GimpActionSelectType  select_type,
                        GimpDisplay          *display,
                        GObject              *object,
                        const gchar          *property_name,
                        gdouble               small_inc,
                        gdouble               inc,
                        gdouble               skip_inc,
                        gdouble               delta_factor,
                        gboolean              wrap)
{
  GParamSpec *pspec;

  g_return_if_fail (display == NULL || GIMP_IS_DISPLAY (display));
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                        property_name);

  if (G_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      gdouble value;

      g_object_get (object, property_name, &value, NULL);

      value = action_select_value (select_type,
                                   value,
                                   G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                                   G_PARAM_SPEC_DOUBLE (pspec)->maximum,
                                   G_PARAM_SPEC_DOUBLE (pspec)->default_value,
                                   small_inc, inc, skip_inc, delta_factor, wrap);

      g_object_set (object, property_name, value, NULL);

      if (display)
        {
          const gchar *blurb = g_param_spec_get_blurb (pspec);

          if (blurb)
            {
              /*  value description and new value shown in the status bar  */
              action_message (display, object, _("%s: %.2f"), blurb, value);
            }
        }
    }
  else if (G_IS_PARAM_SPEC_INT (pspec))
    {
      gint value;

      g_object_get (object, property_name, &value, NULL);

      value = action_select_value (select_type,
                                   value,
                                   G_PARAM_SPEC_INT (pspec)->minimum,
                                   G_PARAM_SPEC_INT (pspec)->maximum,
                                   G_PARAM_SPEC_INT (pspec)->default_value,
                                   small_inc, inc, skip_inc, delta_factor, wrap);

      g_object_set (object, property_name, value, NULL);

      if (display)
        {
          const gchar *blurb = g_param_spec_get_blurb (pspec);

          if (blurb)
            {
              /*  value description and new value shown in the status bar  */
              action_message (display, object, _("%s: %d"), blurb, value);
            }
        }
    }
  else
    {
      g_return_if_reached ();
    }
}

GimpObject *
action_select_object (GimpActionSelectType  select_type,
                      GimpContainer        *container,
                      GimpObject           *current)
{
  gint select_index;
  gint n_children;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (current == NULL || GIMP_IS_OBJECT (current), NULL);

  if (! current)
    return NULL;

  n_children = gimp_container_get_n_children (container);

  if (n_children == 0)
    return NULL;

  switch (select_type)
    {
    case GIMP_ACTION_SELECT_FIRST:
      select_index = 0;
      break;

    case GIMP_ACTION_SELECT_LAST:
      select_index = n_children - 1;
      break;

    case GIMP_ACTION_SELECT_PREVIOUS:
      select_index = gimp_container_get_child_index (container, current) - 1;
      break;

    case GIMP_ACTION_SELECT_NEXT:
      select_index = gimp_container_get_child_index (container, current) + 1;
      break;

    case GIMP_ACTION_SELECT_SKIP_PREVIOUS:
      select_index = gimp_container_get_child_index (container, current) - 10;
      break;

    case GIMP_ACTION_SELECT_SKIP_NEXT:
      select_index = gimp_container_get_child_index (container, current) + 10;
      break;

    default:
      if ((gint) select_type >= 0)
        select_index = (gint) select_type;
      else
        g_return_val_if_reached (current);
      break;
    }

  select_index = CLAMP (select_index, 0, n_children - 1);

  return gimp_container_get_child_by_index (container, select_index);
}

void
action_message (GimpDisplay *display,
                GObject     *object,
                const gchar *format,
                ...)
{
  GimpDisplayShell *shell     = gimp_display_get_shell (display);
  GimpStatusbar    *statusbar = gimp_display_shell_get_statusbar (shell);
  const gchar      *stock_id  = NULL;
  va_list           args;

  if (GIMP_IS_TOOL_OPTIONS (object))
    {
      GimpToolInfo *tool_info = GIMP_TOOL_OPTIONS (object)->tool_info;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
    }
  else if (GIMP_IS_VIEWABLE (object))
    {
      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (object));
    }

  va_start (args, format);
  gimp_statusbar_push_temp_valist (statusbar, GIMP_MESSAGE_INFO,
                                   stock_id, format, args);
  va_end (args);
}
