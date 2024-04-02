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
          GeglColor *color = gegl_color_new ("none");

          gimp_get_color_tag_color (value->value, color, FALSE);
          gimp_action_group_set_action_color (group, action, color, FALSE);
          g_object_unref (color);
        }
    }

  g_type_class_unref (enum_class);
}

void
items_actions_update (GimpActionGroup *group,
                      const gchar     *prefix,
                      GList           *items)
{
  GEnumClass *enum_class;
  GEnumValue *value;
  gchar       action[32];
  gboolean    visible           = FALSE;
  gboolean    has_color_tag     = FALSE;
  gboolean    lock_content      = TRUE;
  gboolean    can_lock_content  = FALSE;
  gboolean    lock_position     = TRUE;
  gboolean    can_lock_position = FALSE;
  GeglColor  *tag_color         = gegl_color_new ("none");
  GList      *iter;

  for (iter = items; iter; iter = iter->next)
    {
      GimpItem *item = iter->data;

      /* With possible multi-selected items, toggle states may be
       * inconsistent (e.g. some items of the selection may be visible,
       * others invisible, yet the toggle action can only be one of 2
       * states). We just choose that if one of the item has the TRUE
       * state, then we give the TRUE (active) state to the action.
       * It's mostly arbitrary and doesn't actually change much to the
       * action.
       */
      visible = (visible || gimp_item_get_visible (item));

      if (gimp_item_can_lock_content (item))
        {
          if (! gimp_item_get_lock_content (item))
            lock_content = FALSE;
          can_lock_content = TRUE;
        }

      if (gimp_item_can_lock_position (item))
        {
          if (! gimp_item_get_lock_position (item))
            lock_position = FALSE;
          can_lock_position = TRUE;
        }

      has_color_tag = (has_color_tag ||
                       gimp_get_color_tag_color (gimp_item_get_color_tag (item),
                                                 tag_color, FALSE));
    }
  g_object_unref (tag_color);

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  g_snprintf (action, sizeof (action), "%s-visible", prefix);
  SET_SENSITIVE (action, items);
  SET_ACTIVE    (action, visible);

  g_snprintf (action, sizeof (action), "%s-lock-content", prefix);
  SET_SENSITIVE (action, can_lock_content);
  SET_ACTIVE    (action, lock_content);

  g_snprintf (action, sizeof (action), "%s-lock-position", prefix);
  SET_SENSITIVE (action, can_lock_position);
  SET_ACTIVE    (action, lock_position);

  enum_class = g_type_class_ref (GIMP_TYPE_COLOR_TAG);

  for (value = enum_class->values; value->value_name; value++)
    {
      g_snprintf (action, sizeof (action),
                  "%s-color-tag-%s", prefix, value->value_nick);

      SET_SENSITIVE (action, items);
    }

  g_type_class_unref (enum_class);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
