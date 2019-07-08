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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpitem.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpwidgets-utils.h"

#include "items-actions.h"


void
items_actions_setup (GimpActionGroup *group,
                     const gchar     *prefix)
{
  GEnumClass *enum_class;
  GEnumValue *value;

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_TAG);

  for (value = enum_class->values; value->value_name; value++)
    {
      gchar action[32];

      g_snprintf (action, sizeof (action),
                  "%s-color-tag-%s", prefix, value->value_nick);

      if (value->value == GIMP_COLOR_TAG_NONE)
        {
          gimp_action_group_set_action_always_show_image (group, action, TRUE);
        }
      else
        {
          GimpRGB color;

          gimp_action_group_set_action_context (group, action,
                                                gimp_get_user_context (group->gimp));

          gimp_get_color_tag_color (value->value, &color, FALSE);
          gimp_action_group_set_action_color (group, action, &color, FALSE);
        }
    }

  g_type_class_unref (enum_class);
}

void
items_actions_update (GimpActionGroup *group,
                      const gchar     *prefix,
                      GimpItem        *item)
{
  GEnumClass *enum_class;
  GEnumValue *value;
  gchar       action[32];
  gboolean    visible       = FALSE;
  gboolean    linked        = FALSE;
  gboolean    has_color_tag = FALSE;
  gboolean    locked        = FALSE;
  gboolean    can_lock      = FALSE;
  gboolean    locked_pos    = FALSE;
  gboolean    can_lock_pos  = FALSE;
  GimpRGB     tag_color;

  if (item)
    {
      visible      = gimp_item_get_visible (item);
      linked       = gimp_item_get_linked (item);
      locked       = gimp_item_get_lock_content (item);
      can_lock     = gimp_item_can_lock_content (item);
      locked_pos   = gimp_item_get_lock_position (item);
      can_lock_pos = gimp_item_can_lock_position (item);

      has_color_tag = gimp_get_color_tag_color (gimp_item_get_color_tag (item),
                                                &tag_color, FALSE);
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        gimp_action_group_set_action_color (group, action, color, FALSE)

  g_snprintf (action, sizeof (action), "%s-visible", prefix);
  SET_SENSITIVE (action, item);
  SET_ACTIVE    (action, visible);

  g_snprintf (action, sizeof (action), "%s-linked", prefix);
  SET_SENSITIVE (action, item);
  SET_ACTIVE    (action, linked);

  g_snprintf (action, sizeof (action), "%s-lock-content", prefix);
  SET_SENSITIVE (action, can_lock);
  SET_ACTIVE    (action, locked);

  g_snprintf (action, sizeof (action), "%s-lock-position", prefix);
  SET_SENSITIVE (action, can_lock_pos);
  SET_ACTIVE    (action, locked_pos);

  g_snprintf (action, sizeof (action), "%s-color-tag-menu", prefix);
  SET_COLOR (action, has_color_tag ? &tag_color : NULL);

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_TAG);

  for (value = enum_class->values; value->value_name; value++)
    {
      g_snprintf (action, sizeof (action),
                  "%s-color-tag-%s", prefix, value->value_nick);

      SET_SENSITIVE (action, item);
    }

  g_type_class_unref (enum_class);

#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_COLOR
}
