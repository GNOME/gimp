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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmadatafactory.h"
#include "ligmafilteredcontainer.h"
#include "ligmapaintinfo.h"
#include "ligmatoolinfo.h"
#include "ligmatooloptions.h"
#include "ligmatoolpreset.h"


static void    ligma_tool_info_dispose         (GObject       *object);
static void    ligma_tool_info_finalize        (GObject       *object);

static gchar * ligma_tool_info_get_description (LigmaViewable  *viewable,
                                               gchar        **tooltip);


G_DEFINE_TYPE (LigmaToolInfo, ligma_tool_info, LIGMA_TYPE_TOOL_ITEM)

#define parent_class ligma_tool_info_parent_class


static void
ligma_tool_info_class_init (LigmaToolInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class = LIGMA_VIEWABLE_CLASS (klass);

  object_class->dispose           = ligma_tool_info_dispose;
  object_class->finalize          = ligma_tool_info_finalize;

  viewable_class->get_description = ligma_tool_info_get_description;
}

static void
ligma_tool_info_init (LigmaToolInfo *tool_info)
{
}

static void
ligma_tool_info_dispose (GObject *object)
{
  LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (object);

  if (tool_info->tool_options)
    {
      g_object_run_dispose (G_OBJECT (tool_info->tool_options));
      g_clear_object (&tool_info->tool_options);
    }

  g_clear_object (&tool_info->presets);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_tool_info_finalize (GObject *object)
{
  LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (object);

  g_clear_pointer (&tool_info->label,       g_free);
  g_clear_pointer (&tool_info->tooltip,     g_free);
  g_clear_pointer (&tool_info->menu_label,  g_free);
  g_clear_pointer (&tool_info->menu_accel,  g_free);
  g_clear_pointer (&tool_info->help_domain, g_free);
  g_clear_pointer (&tool_info->help_id,     g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
ligma_tool_info_get_description (LigmaViewable  *viewable,
                                gchar        **tooltip)
{
  LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (viewable);

  if (tooltip)
    *tooltip = g_strdup (tool_info->tooltip);

  return g_strdup (tool_info->label);
}

static gboolean
ligma_tool_info_filter_preset (LigmaObject *object,
                              gpointer    user_data)
{
  LigmaToolPreset *preset    = LIGMA_TOOL_PRESET (object);
  LigmaToolInfo   *tool_info = user_data;

  return preset->tool_options->tool_info == tool_info;
}

LigmaToolInfo *
ligma_tool_info_new (Ligma                *ligma,
                    GType                tool_type,
                    GType                tool_options_type,
                    LigmaContextPropMask  context_props,
                    const gchar         *identifier,
                    const gchar         *label,
                    const gchar         *tooltip,
                    const gchar         *menu_label,
                    const gchar         *menu_accel,
                    const gchar         *help_domain,
                    const gchar         *help_id,
                    const gchar         *paint_core_name,
                    const gchar         *icon_name)
{
  LigmaPaintInfo *paint_info;
  LigmaToolInfo  *tool_info;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);
  g_return_val_if_fail (tooltip != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (paint_core_name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  paint_info = (LigmaPaintInfo *)
    ligma_container_get_child_by_name (ligma->paint_info_list, paint_core_name);

  g_return_val_if_fail (LIGMA_IS_PAINT_INFO (paint_info), NULL);

  tool_info = g_object_new (LIGMA_TYPE_TOOL_INFO,
                            "name",      identifier,
                            "icon-name", icon_name,
                            NULL);

  tool_info->ligma              = ligma;
  tool_info->tool_type         = tool_type;
  tool_info->tool_options_type = tool_options_type;
  tool_info->context_props     = context_props;

  tool_info->label             = g_strdup (label);
  tool_info->tooltip           = g_strdup (tooltip);

  tool_info->menu_label        = g_strdup (menu_label);
  tool_info->menu_accel        = g_strdup (menu_accel);

  tool_info->help_domain       = g_strdup (help_domain);
  tool_info->help_id           = g_strdup (help_id);

  tool_info->paint_info        = paint_info;

  if (tool_info->tool_options_type == paint_info->paint_options_type)
    {
      tool_info->tool_options = g_object_ref (LIGMA_TOOL_OPTIONS (paint_info->paint_options));
    }
  else
    {
      tool_info->tool_options = g_object_new (tool_info->tool_options_type,
                                              "ligma", ligma,
                                              "name", identifier,
                                              NULL);
    }

  g_object_set (tool_info->tool_options,
                "tool",      tool_info,
                "tool-info", tool_info, NULL);

  ligma_tool_options_set_gui_mode (tool_info->tool_options, TRUE);

  if (tool_info->tool_options_type != LIGMA_TYPE_TOOL_OPTIONS)
    {
      LigmaContainer *presets;

      presets = ligma_data_factory_get_container (ligma->tool_preset_factory);

      tool_info->presets =
        ligma_filtered_container_new (presets,
                                     ligma_tool_info_filter_preset,
                                     tool_info);
    }

  return tool_info;
}

void
ligma_tool_info_set_standard (Ligma         *ligma,
                             LigmaToolInfo *tool_info)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (! tool_info || LIGMA_IS_TOOL_INFO (tool_info));

  g_set_object (&ligma->standard_tool_info, tool_info);
}

LigmaToolInfo *
ligma_tool_info_get_standard (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return ligma->standard_tool_info;
}

gchar *
ligma_tool_info_get_action_name (LigmaToolInfo *tool_info)
{
  const gchar *identifier;
  gchar       *tmp;
  gchar       *name;

  g_return_val_if_fail (LIGMA_IS_TOOL_INFO (tool_info), NULL);

  identifier = ligma_object_get_name (LIGMA_OBJECT (tool_info));

  g_return_val_if_fail (g_str_has_prefix (identifier, "ligma-"), NULL);
  g_return_val_if_fail (g_str_has_suffix (identifier, "-tool"), NULL);

  tmp = g_strndup (identifier + strlen ("ligma-"),
                    strlen (identifier) - strlen ("ligma--tool"));

  name = g_strdup_printf ("tools-%s", tmp);

  g_free (tmp);

  return name;
}

GFile *
ligma_tool_info_get_options_file (LigmaToolInfo *tool_info,
                                 const gchar  *suffix)
{
  gchar *basename;
  GFile *file;

  g_return_val_if_fail (LIGMA_IS_TOOL_INFO (tool_info), NULL);

  /* also works for a NULL suffix */
  basename = g_strconcat (ligma_object_get_name (tool_info), suffix, NULL);

  file = ligma_directory_file ("tool-options", basename, NULL);
  g_free (basename);

  return file;
}
