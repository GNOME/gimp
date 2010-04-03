/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
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

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimptoolpreset.h"

#include "gimpdocked.h"
#include "gimptoolpreseteditor.h"
#include "gimpmenufactory.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GObject * gimp_tool_preset_editor_constructor  (GType              type,
                                                       guint              n_params,
                                                       GObjectConstructParam *params);

static void   gimp_tool_preset_editor_set_data        (GimpDataEditor     *editor,
                                                       GimpData           *data);

static void   gimp_tool_preset_editor_notify_model    (GimpToolPreset       *options,
                                                       const GParamSpec     *pspec,
                                                       GimpToolPresetEditor *editor);
static void   gimp_tool_preset_editor_notify_data     (GimpToolPreset       *options,
                                                       const GParamSpec   *pspec,
                                                       GimpToolPresetEditor *editor);



G_DEFINE_TYPE_WITH_CODE (GimpToolPresetEditor, gimp_tool_preset_editor,
                         GIMP_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED, NULL))

#define parent_class gimp_tool_preset_editor_parent_class


static void
gimp_tool_preset_editor_class_init (GimpToolPresetEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructor = gimp_tool_preset_editor_constructor;

  editor_class->set_data    = gimp_tool_preset_editor_set_data;
  editor_class->title       = _("Tool Preset Editor");
}

static void
gimp_tool_preset_editor_init (GimpToolPresetEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  /*Nuffink*/
}

static GObject *
gimp_tool_preset_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_docked_set_show_button_bar (GIMP_DOCKED (object), FALSE);

  return object;
}

static void
gimp_tool_preset_editor_set_data (GimpDataEditor *editor,
                               GimpData       *data)
{
  GimpToolPresetEditor *tool_preset_editor = GIMP_TOOL_PRESET_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          gimp_tool_preset_editor_notify_data,
                                          editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      g_signal_handlers_block_by_func (tool_preset_editor->tool_preset_model,
                                       gimp_tool_preset_editor_notify_model,
                                       editor);

      gimp_config_copy (GIMP_CONFIG (editor->data),
                        GIMP_CONFIG (tool_preset_editor->tool_preset_model),
                        GIMP_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (tool_preset_editor->tool_preset_model,
                                         gimp_tool_preset_editor_notify_model,
                                         editor);

      g_signal_connect (editor->data, "notify",
                        G_CALLBACK (gimp_tool_preset_editor_notify_data),
                        editor);
    }
}


/*  public functions  */

GtkWidget *
gimp_tool_preset_editor_new (GimpContext     *context,
                          GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_TOOL_PRESET_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolPresetEditor>",
                       "ui-path",         "/tool_preset-editor-popup",
                       "data-factory",    context->gimp->tool_preset_factory,
                       "context",         context,
                       "data",            gimp_context_get_tool_preset (context),
                       NULL);
}


/*  private functions  */

static void
gimp_tool_preset_editor_notify_model (GimpToolPreset       *options,
                                   const GParamSpec   *pspec,
                                   GimpToolPresetEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

  if (data_editor->data)
    {
      g_signal_handlers_block_by_func (data_editor->data,
                                       gimp_tool_preset_editor_notify_data,
                                       editor);

      gimp_config_copy (GIMP_CONFIG (editor->tool_preset_model),
                        GIMP_CONFIG (data_editor->data),
                        GIMP_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (data_editor->data,
                                         gimp_tool_preset_editor_notify_data,
                                         editor);
    }
}

static void
gimp_tool_preset_editor_notify_data (GimpToolPreset       *options,
                                  const GParamSpec   *pspec,
                                  GimpToolPresetEditor *editor)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

  g_signal_handlers_block_by_func (editor->tool_preset_model,
                                   gimp_tool_preset_editor_notify_model,
                                   editor);

  gimp_config_copy (GIMP_CONFIG (data_editor->data),
                    GIMP_CONFIG (editor->tool_preset_model),
                    GIMP_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (editor->tool_preset_model,
                                     gimp_tool_preset_editor_notify_model,
                                     editor);
}
