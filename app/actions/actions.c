/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpactionfactory.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpimageeditor.h"
#include "widgets/gimpitemtreeview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gui/dialogs.h"

#include "brushes-actions.h"
#include "buffers-actions.h"
#include "channels-actions.h"
#include "colormap-editor-actions.h"
#include "debug-actions.h"
#include "dialogs-actions.h"
#include "dockable-actions.h"
#include "documents-actions.h"
#include "drawable-actions.h"
#include "edit-actions.h"
#include "error-console-actions.h"
#include "file-actions.h"
#include "file-open-actions.h"
#include "file-save-actions.h"
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
#include "qmask-actions.h"
#include "select-actions.h"
#include "templates-actions.h"
#include "tool-options-actions.h"
#include "tools-actions.h"
#include "vectors-actions.h"
#include "view-actions.h"


/*  global variables  */

GimpActionFactory *global_action_factory = NULL;


/*  private variables  */

static GimpActionFactoryEntry action_groups[] =
{
  { "brushes",
    brushes_actions_setup,
    brushes_actions_update },
  { "buffers",
    buffers_actions_setup,
    buffers_actions_update },
  { "channels",
    channels_actions_setup,
    channels_actions_update },
  { "colormap-editor",
    colormap_editor_actions_setup,
    colormap_editor_actions_update },
  { "debug",
    debug_actions_setup,
    debug_actions_update },
  { "dialogs",
    dialogs_actions_setup,
    dialogs_actions_update },
  { "dockable",
    dockable_actions_setup,
    dockable_actions_update },
  { "documents",
    documents_actions_setup,
    documents_actions_update },
  { "drawable",
    drawable_actions_setup,
    drawable_actions_update },
  { "edit",
    edit_actions_setup,
    edit_actions_update },
  { "error-console",
    error_console_actions_setup,
    error_console_actions_update },
  { "file",
    file_actions_setup,
    file_actions_update },
  { "file-open",
    file_open_actions_setup,
    file_open_actions_update },
  { "file-save",
    file_save_actions_setup,
    file_save_actions_update },
  { "fonts",
    fonts_actions_setup,
    fonts_actions_update },
  { "gradient-editor",
    gradient_editor_actions_setup,
    gradient_editor_actions_update },
  { "gradients",
    gradients_actions_setup,
    gradients_actions_update },
  { "help",
    help_actions_setup,
    help_actions_update },
  { "image",
    image_actions_setup,
    image_actions_update },
  { "images",
    images_actions_setup,
    images_actions_update },
  { "layers",
    layers_actions_setup,
    layers_actions_update },
  { "palette-editor",
    palette_editor_actions_setup,
    palette_editor_actions_update },
  { "palettes",
    palettes_actions_setup,
    palettes_actions_update },
  { "patterns",
    patterns_actions_setup,
    patterns_actions_update },
  { "plug-in",
    plug_in_actions_setup,
    plug_in_actions_update },
  { "qmask",
    qmask_actions_setup,
    qmask_actions_update },
  { "select",
    select_actions_setup,
    select_actions_update },
  { "templates",
    templates_actions_setup,
    templates_actions_update },
  { "tool-options",
    tool_options_actions_setup,
    tool_options_actions_update },
  { "tools",
    tools_actions_setup,
    tools_actions_update },
  { "vectors",
    vectors_actions_setup,
    vectors_actions_update },
  { "view",
    view_actions_setup,
    view_actions_update }
};

static gboolean actions_initialized = FALSE;


/*  public functions  */

void
actions_init (Gimp *gimp)
{
  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (actions_initialized == FALSE);

  actions_initialized = TRUE;

  global_action_factory = gimp_action_factory_new (gimp);

  for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
    gimp_action_factory_group_register (global_action_factory,
                                        action_groups[i].identifier,
                                        action_groups[i].setup_func,
                                        action_groups[i].update_func);
}

void
actions_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_object_unref (global_action_factory);
  global_action_factory = NULL;
}

Gimp *
action_data_get_gimp (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return ((GimpDisplay *) data)->gimage->gimp;
  else if (GIMP_IS_DISPLAY_SHELL (data))
    return ((GimpDisplayShell *) data)->gdisp->gimage->gimp;
  else if (GIMP_IS_ITEM_TREE_VIEW (data))
    return ((GimpItemTreeView *) data)->context->gimp;
  else if (GIMP_IS_IMAGE_EDITOR (data))
    return ((GimpImageEditor *) data)->context->gimp;
  else if (GIMP_IS_GIMP (data))
    return data;
  else if (GIMP_IS_DOCK (data))
    return ((GimpDock *) data)->context->gimp;

  return NULL;
}

GimpContext *
action_data_get_context (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return gimp_get_user_context (((GimpDisplay *) data)->gimage->gimp);
  else if (GIMP_IS_DISPLAY_SHELL (data))
    return gimp_get_user_context (((GimpDisplayShell *) data)->gdisp->gimage->gimp);
  else if (GIMP_IS_ITEM_TREE_VIEW (data))
    return ((GimpItemTreeView *) data)->context;
  else if (GIMP_IS_CONTAINER_VIEW (data))
    return gimp_container_view_get_context ((GimpContainerView *) data);
  else if (GIMP_IS_CONTAINER_EDITOR (data))
    return gimp_container_view_get_context (((GimpContainerEditor *) data)->view);
  else if (GIMP_IS_IMAGE_EDITOR (data))
    return ((GimpImageEditor *) data)->context;
  else if (GIMP_IS_GIMP (data))
    return gimp_get_user_context (data);
  else if (GIMP_IS_DOCK (data))
    return ((GimpDock *) data)->context;

  return NULL;
}

GimpImage *
action_data_get_image (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return ((GimpDisplay *) data)->gimage;
  else if (GIMP_IS_DISPLAY_SHELL (data))
    return ((GimpDisplayShell *) data)->gdisp->gimage;
  else if (GIMP_IS_ITEM_TREE_VIEW (data))
    return ((GimpItemTreeView *) data)->gimage;
  else if (GIMP_IS_IMAGE_EDITOR (data))
    return ((GimpImageEditor *) data)->gimage;
  else if (GIMP_IS_GIMP (data))
    return gimp_context_get_image (gimp_get_user_context (data));
  else if (GIMP_IS_DOCK (data))
    return gimp_context_get_image (((GimpDock *) data)->context);

  return NULL;
}

GimpDisplay *
action_data_get_display (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return data;
  else if (GIMP_IS_DISPLAY_SHELL (data))
    return ((GimpDisplayShell *) data)->gdisp;
  else if (GIMP_IS_GIMP (data))
    return gimp_context_get_display (gimp_get_user_context (data));
  else if (GIMP_IS_DOCK (data))
    return gimp_context_get_display (((GimpDock *) data)->context);

  return NULL;
}

GtkWidget *
action_data_get_widget (gpointer data)
{
  if (! data)
    return NULL;

  if (GIMP_IS_DISPLAY (data))
    return ((GimpDisplay *) data)->shell;
  else if (GIMP_IS_GIMP (data))
    return dialogs_get_toolbox ();
  else if (GTK_IS_WIDGET (data))
    return data;

  return NULL;
}
