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

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpreset.h"

#include "gimpdocked.h"
#include "gimptoolpreseteditor.h"
#include "gimpmenufactory.h"
#include "gimppropwidgets.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_tool_preset_editor_constructed  (GObject              *object);
static void   gimp_tool_preset_editor_finalize     (GObject              *object);

static void   gimp_tool_preset_editor_set_data     (GimpDataEditor       *editor,
                                                    GimpData             *data);

static void   gimp_tool_preset_editor_notify_model (GimpToolPreset       *options,
                                                    const GParamSpec     *pspec,
                                                    GimpToolPresetEditor *editor);
static void   gimp_tool_preset_editor_notify_data  (GimpToolPreset       *options,
                                                    const GParamSpec     *pspec,
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

  object_class->constructed = gimp_tool_preset_editor_constructed;
  object_class->finalize    = gimp_tool_preset_editor_finalize;

  editor_class->set_data    = gimp_tool_preset_editor_set_data;
  editor_class->title       = _("Tool Preset Editor");
}

static void
gimp_tool_preset_editor_init (GimpToolPresetEditor *editor)
{
}

static void
gimp_tool_preset_editor_constructed (GObject *object)
{
  GimpToolPresetEditor *editor      = GIMP_TOOL_PRESET_EDITOR (object);
  GimpDataEditor       *data_editor = GIMP_DATA_EDITOR (editor);
  GimpToolPreset       *preset;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *button;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  preset = editor->tool_preset_model =
    g_object_new (GIMP_TYPE_TOOL_PRESET,
                  "gimp", data_editor->context->gimp,
                  NULL);

  g_signal_connect (preset, "notify",
                    G_CALLBACK (gimp_tool_preset_editor_notify_model),
                    editor);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->tool_icon = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->tool_icon,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->tool_icon);

  editor->tool_label = gtk_label_new ("");
  gimp_label_set_attributes (GTK_LABEL (editor->tool_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), editor->tool_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->tool_label);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Icon:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = gimp_prop_icon_picker_new (G_OBJECT (preset), "stock-id",
                                      data_editor->context->gimp);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-fg-bg",
                                       _("Apply stored FG/BG"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-brush",
                                       _("Apply stored brush"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-dynamics",
                                       _("Apply stored dynamics"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-gradient",
                                       _("Apply stored gradient"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-pattern",
                                       _("Apply stored pattern"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-palette",
                                       _("Apply stored palette"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (G_OBJECT (preset), "use-font",
                                       _("Apply stored font"));
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
gimp_tool_preset_editor_finalize (GObject *object)
{
  GimpToolPresetEditor *editor = GIMP_TOOL_PRESET_EDITOR (object);

  if (editor->tool_preset_model)
    {
      g_object_unref (editor->tool_preset_model);
      editor->tool_preset_model = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_preset_editor_set_data (GimpDataEditor *editor,
                                  GimpData       *data)
{
  GimpToolPresetEditor *preset_editor = GIMP_TOOL_PRESET_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          gimp_tool_preset_editor_notify_data,
                                          editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data && preset_editor->tool_preset_model)
    {
      GimpToolInfo *tool_info;
      const gchar  *stock_id;
      gchar        *label;

      g_signal_handlers_block_by_func (preset_editor->tool_preset_model,
                                       gimp_tool_preset_editor_notify_model,
                                       editor);

      gimp_config_copy (GIMP_CONFIG (editor->data),
                        GIMP_CONFIG (preset_editor->tool_preset_model),
                        GIMP_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (preset_editor->tool_preset_model,
                                         gimp_tool_preset_editor_notify_model,
                                         editor);

      g_signal_connect (editor->data, "notify",
                        G_CALLBACK (gimp_tool_preset_editor_notify_data),
                        editor);

      tool_info = preset_editor->tool_preset_model->tool_options->tool_info;

      stock_id = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
      label    = g_strdup_printf (_("%s Preset"), tool_info->blurb);

      gtk_image_set_from_stock (GTK_IMAGE (preset_editor->tool_icon),
                                stock_id, GTK_ICON_SIZE_MENU);
      gtk_label_set_text (GTK_LABEL (preset_editor->tool_label), label);

      g_free (label);
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
                       "ui-path",         "/tool-preset-editor-popup",
                       "data-factory",    context->gimp->tool_preset_factory,
                       "context",         context,
                       "data",            gimp_context_get_tool_preset (context),
                       NULL);
}


/*  private functions  */

static void
gimp_tool_preset_editor_notify_model (GimpToolPreset       *options,
                                      const GParamSpec     *pspec,
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
                                     const GParamSpec     *pspec,
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
