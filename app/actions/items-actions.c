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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmaitem.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmawidgets-utils.h"

#include "items-actions.h"


void
items_actions_setup (LigmaActionGroup *group,
                     const gchar     *prefix)
{
  GEnumClass *enum_class;
  GEnumValue *value;

  enum_class = g_type_class_ref (LIGMA_TYPE_COLOR_TAG);

  for (value = enum_class->values; value->value_name; value++)
    {
      gchar action[32];

      g_snprintf (action, sizeof (action),
                  "%s-color-tag-%s", prefix, value->value_nick);

      if (value->value == LIGMA_COLOR_TAG_NONE)
        {
          ligma_action_group_set_action_always_show_image (group, action, TRUE);
        }
      else
        {
          LigmaRGB color;

          ligma_action_group_set_action_context (group, action,
                                                ligma_get_user_context (group->ligma));

          ligma_get_color_tag_color (value->value, &color, FALSE);
          ligma_action_group_set_action_color (group, action, &color, FALSE);
        }
    }

  g_type_class_unref (enum_class);
}

void
items_actions_update (LigmaActionGroup *group,
                      const gchar     *prefix,
                      GList           *items)
{
  GEnumClass *enum_class;
  GEnumValue *value;
  gchar       action[32];
  gboolean    visible       = FALSE;
  gboolean    has_color_tag = FALSE;
  gboolean    lock_content      = TRUE;
  gboolean    can_lock_content  = FALSE;
  gboolean    lock_position     = TRUE;
  gboolean    can_lock_position = FALSE;
  LigmaRGB     tag_color;
  GList       *iter;

  for (iter = items; iter; iter = iter->next)
    {
      LigmaItem *item = iter->data;

      /* With possible multi-selected items, toggle states may be
       * inconsistent (e.g. some items of the selection may be visible,
       * others invisible, yet the toggle action can only be one of 2
       * states). We just choose that if one of the item has the TRUE
       * state, then we give the TRUE (active) state to the action.
       * It's mostly arbitrary and doesn't actually change much to the
       * action.
       */
      visible = (visible || ligma_item_get_visible (item));

      if (ligma_item_can_lock_content (item))
        {
          if (! ligma_item_get_lock_content (item))
            lock_content = FALSE;
          can_lock_content = TRUE;
        }

      if (ligma_item_can_lock_position (item))
        {
          if (! ligma_item_get_lock_position (item))
            lock_position = FALSE;
          can_lock_position = TRUE;
        }

      has_color_tag = (has_color_tag ||
                       ligma_get_color_tag_color (ligma_item_get_color_tag (item),
                                                 &tag_color, FALSE));
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)
#define SET_COLOR(action,color) \
        ligma_action_group_set_action_color (group, action, color, FALSE)

  g_snprintf (action, sizeof (action), "%s-visible", prefix);
  SET_SENSITIVE (action, items);
  SET_ACTIVE    (action, visible);

  g_snprintf (action, sizeof (action), "%s-lock-content", prefix);
  SET_SENSITIVE (action, can_lock_content);
  SET_ACTIVE    (action, lock_content);

  g_snprintf (action, sizeof (action), "%s-lock-position", prefix);
  SET_SENSITIVE (action, can_lock_position);
  SET_ACTIVE    (action, lock_position);

  g_snprintf (action, sizeof (action), "%s-color-tag-menu", prefix);
  SET_COLOR (action, has_color_tag ? &tag_color : NULL);

  enum_class = g_type_class_ref (LIGMA_TYPE_COLOR_TAG);

  for (value = enum_class->values; value->value_name; value++)
    {
      g_snprintf (action, sizeof (action),
                  "%s-color-tag-%s", prefix, value->value_nick);

      SET_SENSITIVE (action, items);
    }

  g_type_class_unref (enum_class);

#undef SET_SENSITIVE
#undef SET_ACTIVE
#undef SET_COLOR
}
