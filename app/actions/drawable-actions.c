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
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayermask.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "drawable-actions.h"
#include "drawable-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry drawable_actions[] =
{
  { "drawable-equalize", NULL,
    NC_("drawable-action", "_Equalize"), NULL,
    NC_("drawable-action", "Automatic contrast enhancement"),
    drawable_equalize_cmd_callback,
    GIMP_HELP_LAYER_EQUALIZE },

  { "drawable-levels-stretch", NULL,
    NC_("drawable-action", "_White Balance"), NULL,
    NC_("drawable-action", "Automatic white balance correction"),
    drawable_levels_stretch_cmd_callback,
    GIMP_HELP_LAYER_WHITE_BALANCE }
};

static const GimpToggleActionEntry drawable_toggle_actions[] =
{
  { "drawable-visible", GIMP_ICON_VISIBLE,
    NC_("drawable-action", "Toggle Drawable _Visibility"), NULL, NULL,
    drawable_visible_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_VISIBLE },

  { "drawable-linked", GIMP_ICON_LINKED,
    NC_("drawable-action", "Toggle Drawable _Linked State"), NULL, NULL,
    drawable_linked_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LINKED },

  { "drawable-lock-content", NULL /* GIMP_ICON_LOCK */,
    NC_("drawable-action", "L_ock Pixels of Drawable"), NULL,
    NC_("drawable-action",
        "Keep the pixels on this drawable from being modified"),
    drawable_lock_content_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LOCK_PIXELS },

  { "drawable-lock-position", GIMP_ICON_TOOL_MOVE,
    NC_("drawable-action", "L_ock Position of Drawable"), NULL,
    NC_("drawable-action",
        "Keep the position on this drawable from being modified"),
    drawable_lock_position_cmd_callback,
    FALSE,
    GIMP_HELP_LAYER_LOCK_POSITION },
};

static const GimpEnumActionEntry drawable_flip_actions[] =
{
  { "drawable-flip-horizontal", GIMP_ICON_OBJECT_FLIP_HORIZONTAL,
    NC_("drawable-action", "Flip _Horizontally"), NULL,
    NC_("drawable-action", "Flip drawable horizontally"),
    GIMP_ORIENTATION_HORIZONTAL, FALSE,
    GIMP_HELP_LAYER_FLIP_HORIZONTAL },

  { "drawable-flip-vertical", GIMP_ICON_OBJECT_FLIP_VERTICAL,
    NC_("drawable-action", "Flip _Vertically"), NULL,
    NC_("drawable-action", "Flip drawable vertically"),
    GIMP_ORIENTATION_VERTICAL, FALSE,
    GIMP_HELP_LAYER_FLIP_VERTICAL }
};

static const GimpEnumActionEntry drawable_rotate_actions[] =
{
  { "drawable-rotate-90", GIMP_ICON_OBJECT_ROTATE_90,
    NC_("drawable-action", "Rotate 90° _clockwise"), NULL,
    NC_("drawable-action", "Rotate drawable 90 degrees to the right"),
    GIMP_ROTATE_90, FALSE,
    GIMP_HELP_LAYER_ROTATE_90 },

  { "drawable-rotate-180", GIMP_ICON_OBJECT_ROTATE_180,
    NC_("drawable-action", "Rotate _180°"), NULL,
    NC_("drawable-action", "Turn drawable upside-down"),
    GIMP_ROTATE_180, FALSE,
    GIMP_HELP_LAYER_ROTATE_180 },

  { "drawable-rotate-270", GIMP_ICON_OBJECT_ROTATE_270,
    NC_("drawable-action", "Rotate 90° counter-clock_wise"), NULL,
    NC_("drawable-action", "Rotate drawable 90 degrees to the left"),
    GIMP_ROTATE_270, FALSE,
    GIMP_HELP_LAYER_ROTATE_270 }
};


void
drawable_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "drawable-action",
                                 drawable_actions,
                                 G_N_ELEMENTS (drawable_actions));

  gimp_action_group_add_toggle_actions (group, "drawable-action",
                                        drawable_toggle_actions,
                                        G_N_ELEMENTS (drawable_toggle_actions));

  gimp_action_group_add_enum_actions (group, "drawable-action",
                                      drawable_flip_actions,
                                      G_N_ELEMENTS (drawable_flip_actions),
                                      drawable_flip_cmd_callback);

  gimp_action_group_add_enum_actions (group, "drawable-action",
                                      drawable_rotate_actions,
                                      G_N_ELEMENTS (drawable_rotate_actions),
                                      drawable_rotate_cmd_callback);

#define SET_ALWAYS_SHOW_IMAGE(action,show) \
        gimp_action_group_set_action_always_show_image (group, action, show)

  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-90",  TRUE);
  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-180", TRUE);
  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-270", TRUE);

#undef SET_ALWAYS_SHOW_IMAGE
}

void
drawable_actions_update (GimpActionGroup *group,
                         gpointer         data)
{
  GimpImage    *image;
  GimpDrawable *drawable     = NULL;
  gboolean      is_rgb       = FALSE;
  gboolean      visible      = FALSE;
  gboolean      linked       = FALSE;
  gboolean      locked       = FALSE;
  gboolean      can_lock     = FALSE;
  gboolean      locked_pos   = FALSE;
  gboolean      can_lock_pos = FALSE;
  gboolean      writable     = FALSE;
  gboolean      movable      = FALSE;
  gboolean      children     = FALSE;

  image = action_data_get_image (data);

  if (image)
    {
      drawable = gimp_image_get_active_drawable (image);

      if (drawable)
        {
          GimpItem *item;

          is_rgb = gimp_drawable_is_rgb (drawable);

          if (GIMP_IS_LAYER_MASK (drawable))
            item = GIMP_ITEM (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));
          else
            item = GIMP_ITEM (drawable);

          visible       = gimp_item_get_visible (item);
          linked        = gimp_item_get_linked (item);
          locked        = gimp_item_get_lock_content (item);
          can_lock      = gimp_item_can_lock_content (item);
          writable      = ! gimp_item_is_content_locked (item);
          locked_pos    = gimp_item_get_lock_position (item);
          can_lock_pos  = gimp_item_can_lock_position (item);
          movable       = ! gimp_item_is_position_locked (item);

          if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
            children = TRUE;
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("drawable-equalize",       writable && !children);
  SET_SENSITIVE ("drawable-levels-stretch", writable && !children && is_rgb);

  SET_SENSITIVE ("drawable-visible",       drawable);
  SET_SENSITIVE ("drawable-linked",        drawable);
  SET_SENSITIVE ("drawable-lock-content",  can_lock);
  SET_SENSITIVE ("drawable-lock-position", can_lock_pos);

  SET_ACTIVE ("drawable-visible",       visible);
  SET_ACTIVE ("drawable-linked",        linked);
  SET_ACTIVE ("drawable-lock-content",  locked);
  SET_ACTIVE ("drawable-lock-position", locked_pos);

  SET_SENSITIVE ("drawable-flip-horizontal", writable && movable);
  SET_SENSITIVE ("drawable-flip-vertical",   writable && movable);

  SET_SENSITIVE ("drawable-rotate-90",  writable && movable);
  SET_SENSITIVE ("drawable-rotate-180", writable && movable);
  SET_SENSITIVE ("drawable-rotate-270", writable && movable);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
