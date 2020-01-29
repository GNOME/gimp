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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpdatafactory.h"
#include "gimpfilteredcontainer.h"
#include "gimppaintinfo.h"
#include "gimptoolinfo.h"
#include "gimptooloptions.h"
#include "gimptoolpreset.h"


static void    gimp_tool_info_dispose         (GObject       *object);
static void    gimp_tool_info_finalize        (GObject       *object);

static gchar * gimp_tool_info_get_description (GimpViewable  *viewable,
                                               gchar        **tooltip);


G_DEFINE_TYPE (GimpToolInfo, gimp_tool_info, GIMP_TYPE_TOOL_ITEM)

#define parent_class gimp_tool_info_parent_class


static void
gimp_tool_info_class_init (GimpToolInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->dispose           = gimp_tool_info_dispose;
  object_class->finalize          = gimp_tool_info_finalize;

  viewable_class->get_description = gimp_tool_info_get_description;
}

static void
gimp_tool_info_init (GimpToolInfo *tool_info)
{
}

static void
gimp_tool_info_dispose (GObject *object)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  if (tool_info->tool_options)
    {
      g_object_run_dispose (G_OBJECT (tool_info->tool_options));
      g_clear_object (&tool_info->tool_options);
    }

  g_clear_object (&tool_info->presets);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_info_finalize (GObject *object)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  g_clear_pointer (&tool_info->label,       g_free);
  g_clear_pointer (&tool_info->tooltip,     g_free);
  g_clear_pointer (&tool_info->menu_label,  g_free);
  g_clear_pointer (&tool_info->menu_accel,  g_free);
  g_clear_pointer (&tool_info->help_domain, g_free);
  g_clear_pointer (&tool_info->help_id,     g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
gimp_tool_info_get_description (GimpViewable  *viewable,
                                gchar        **tooltip)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (viewable);

  if (tooltip)
    *tooltip = g_strdup (tool_info->tooltip);

  return g_strdup (tool_info->label);
}

static gboolean
gimp_tool_info_filter_preset (GimpObject *object,
                              gpointer    user_data)
{
  GimpToolPreset *preset    = GIMP_TOOL_PRESET (object);
  GimpToolInfo   *tool_info = user_data;

  return preset->tool_options->tool_info == tool_info;
}

GimpToolInfo *
gimp_tool_info_new (Gimp                *gimp,
                    GType                tool_type,
                    GType                tool_options_type,
                    GimpContextPropMask  context_props,
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
  GimpPaintInfo *paint_info;
  GimpToolInfo  *tool_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);
  g_return_val_if_fail (tooltip != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (paint_core_name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimp->paint_info_list, paint_core_name);

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  tool_info = g_object_new (GIMP_TYPE_TOOL_INFO,
                            "name",      identifier,
                            "icon-name", icon_name,
                            NULL);

  tool_info->gimp              = gimp;
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
      tool_info->tool_options = g_object_ref (GIMP_TOOL_OPTIONS (paint_info->paint_options));
    }
  else
    {
      tool_info->tool_options = g_object_new (tool_info->tool_options_type,
                                              "gimp", gimp,
                                              "name", identifier,
                                              NULL);
    }

  g_object_set (tool_info->tool_options,
                "tool",      tool_info,
                "tool-info", tool_info, NULL);

  gimp_tool_options_set_gui_mode (tool_info->tool_options, TRUE);

  if (tool_info->tool_options_type != GIMP_TYPE_TOOL_OPTIONS)
    {
      GimpContainer *presets;

      presets = gimp_data_factory_get_container (gimp->tool_preset_factory);

      tool_info->presets =
        gimp_filtered_container_new (presets,
                                     gimp_tool_info_filter_preset,
                                     tool_info);
    }

  return tool_info;
}

void
gimp_tool_info_set_standard (Gimp         *gimp,
                             GimpToolInfo *tool_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));

  g_set_object (&gimp->standard_tool_info, tool_info);
}

GimpToolInfo *
gimp_tool_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_tool_info;
}

gchar *
gimp_tool_info_get_action_name (GimpToolInfo *tool_info)
{
  const gchar *identifier;
  gchar       *tmp;
  gchar       *name;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);

  identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

  g_return_val_if_fail (g_str_has_prefix (identifier, "gimp-"), NULL);
  g_return_val_if_fail (g_str_has_suffix (identifier, "-tool"), NULL);

  tmp = g_strndup (identifier + strlen ("gimp-"),
                    strlen (identifier) - strlen ("gimp--tool"));

  name = g_strdup_printf ("tools-%s", tmp);

  g_free (tmp);

  return name;
}

GFile *
gimp_tool_info_get_options_file (GimpToolInfo *tool_info,
                                 const gchar  *suffix)
{
  gchar *basename;
  GFile *file;

  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool_info), NULL);

  /* also works for a NULL suffix */
  basename = g_strconcat (gimp_object_get_name (tool_info), suffix, NULL);

  file = gimp_directory_file ("tool-options", basename, NULL);
  g_free (basename);

  return file;
}
