/* GIMP - The GNU Image Manipulation Program
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

#include <string.h>

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimplist.h"
#include "gimppaintinfo.h"
#include "gimptoolinfo.h"
#include "gimptooloptions.h"
#include "gimptoolpresets.h"


enum
{
  PROP_0,
  PROP_VISIBLE
};


static void    gimp_tool_info_finalize        (GObject       *object);
static void    gimp_tool_info_get_property    (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
static void    gimp_tool_info_set_property    (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);
static gchar * gimp_tool_info_get_description (GimpViewable  *viewable,
                                               gchar        **tooltip);


G_DEFINE_TYPE (GimpToolInfo, gimp_tool_info, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_tool_info_parent_class


static void
gimp_tool_info_class_init (GimpToolInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->finalize          = gimp_tool_info_finalize;
  object_class->get_property      = gimp_tool_info_get_property;
  object_class->set_property      = gimp_tool_info_set_property;

  viewable_class->get_description = gimp_tool_info_get_description;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VISIBLE, "visible",
                                    NULL, TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_tool_info_init (GimpToolInfo *tool_info)
{
  tool_info->gimp              = NULL;

  tool_info->tool_type         = G_TYPE_NONE;
  tool_info->tool_options_type = G_TYPE_NONE;
  tool_info->context_props     = 0;

  tool_info->blurb             = NULL;
  tool_info->help              = NULL;

  tool_info->menu_path         = NULL;
  tool_info->menu_accel        = NULL;

  tool_info->help_domain       = NULL;
  tool_info->help_id           = NULL;

  tool_info->visible           = TRUE;
  tool_info->tool_options      = NULL;
  tool_info->paint_info        = NULL;
}

static void
gimp_tool_info_finalize (GObject *object)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  if (tool_info->blurb)
    {
      g_free (tool_info->blurb);
      tool_info->blurb = NULL;
    }
  if (tool_info->help)
    {
      g_free (tool_info->help);
      tool_info->blurb = NULL;
    }

  if (tool_info->menu_path)
    {
      g_free (tool_info->menu_path);
      tool_info->menu_path = NULL;
    }
  if (tool_info->menu_accel)
    {
      g_free (tool_info->menu_accel);
      tool_info->menu_accel = NULL;
    }

  if (tool_info->help_domain)
    {
      g_free (tool_info->help_domain);
      tool_info->help_domain = NULL;
    }
  if (tool_info->help_id)
    {
      g_free (tool_info->help_id);
      tool_info->help_id = NULL;
    }

  if (tool_info->tool_options)
    {
      g_object_unref (tool_info->tool_options);
      tool_info->tool_options = NULL;
    }

  if (tool_info->presets)
    {
      g_object_unref (tool_info->presets);
      tool_info->presets = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_info_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value, tool_info->visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_info_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (object);

  switch (property_id)
    {
    case PROP_VISIBLE:
      tool_info->visible = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gchar *
gimp_tool_info_get_description (GimpViewable  *viewable,
                                gchar        **tooltip)
{
  GimpToolInfo *tool_info = GIMP_TOOL_INFO (viewable);

  return g_strdup (tool_info->blurb);
}

GimpToolInfo *
gimp_tool_info_new (Gimp                *gimp,
                    GType                tool_type,
                    GType                tool_options_type,
                    GimpContextPropMask  context_props,
                    const gchar         *identifier,
                    const gchar         *blurb,
                    const gchar         *help,
                    const gchar         *menu_path,
                    const gchar         *menu_accel,
                    const gchar         *help_domain,
                    const gchar         *help_id,
                    const gchar         *paint_core_name,
                    const gchar         *stock_id)
{
  GimpPaintInfo *paint_info;
  GimpToolInfo  *tool_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (help != NULL, NULL);
  g_return_val_if_fail (menu_path != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (paint_core_name != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  paint_info = (GimpPaintInfo *)
    gimp_container_get_child_by_name (gimp->paint_info_list, paint_core_name);

  g_return_val_if_fail (GIMP_IS_PAINT_INFO (paint_info), NULL);

  tool_info = g_object_new (GIMP_TYPE_TOOL_INFO,
                            "name",     identifier,
                            "stock-id", stock_id,
                            NULL);

  tool_info->gimp              = gimp;
  tool_info->tool_type         = tool_type;
  tool_info->tool_options_type = tool_options_type;
  tool_info->context_props     = context_props;

  tool_info->blurb             = g_strdup (blurb);
  tool_info->help              = g_strdup (help);

  tool_info->menu_path         = g_strdup (menu_path);
  tool_info->menu_accel        = g_strdup (menu_accel);

  tool_info->help_domain       = g_strdup (help_domain);
  tool_info->help_id           = g_strdup (help_id);

  tool_info->paint_info        = paint_info;

  if (tool_info->tool_options_type == paint_info->paint_options_type)
    {
      tool_info->tool_options = g_object_ref (paint_info->paint_options);
    }
  else
    {
      tool_info->tool_options = g_object_new (tool_info->tool_options_type,
                                              "gimp", gimp,
                                              "name", identifier,
                                              NULL);
    }

  g_object_set (tool_info->tool_options, "tool-info", tool_info, NULL);

  if (tool_info->context_props)
    {
      gimp_context_define_properties (GIMP_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props, FALSE);
    }

  gimp_context_set_serialize_properties (GIMP_CONTEXT (tool_info->tool_options),
                                         tool_info->context_props);

  if (tool_info->tool_options_type != GIMP_TYPE_TOOL_OPTIONS)
    {
      tool_info->presets = gimp_tool_presets_new (tool_info);
    }

  return tool_info;
}

void
gimp_tool_info_set_standard (Gimp         *gimp,
                             GimpToolInfo *tool_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! tool_info || GIMP_IS_TOOL_INFO (tool_info));

  if (tool_info != gimp->standard_tool_info)
    {
      if (gimp->standard_tool_info)
        g_object_unref (gimp->standard_tool_info);

      gimp->standard_tool_info = tool_info;

      if (gimp->standard_tool_info)
        g_object_ref (gimp->standard_tool_info);
    }
}

GimpToolInfo *
gimp_tool_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_tool_info;
}
