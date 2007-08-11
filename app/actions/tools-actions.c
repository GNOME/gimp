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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "tools-actions.h"
#include "tools-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry tools_actions[] =
{
  { "tools-popup", GIMP_STOCK_TOOLS,
    N_("Tools Menu"), NULL, NULL, NULL,
    GIMP_HELP_TOOLS_DIALOG },

  { "tools-menu",           NULL, N_("_Tools")           },
  { "tools-select-menu",    NULL, N_("_Selection Tools") },
  { "tools-paint-menu",     NULL, N_("_Paint Tools")     },
  { "tools-transform-menu", NULL, N_("_Transform Tools") },
  { "tools-color-menu",     NULL, N_("_Color Tools")     },

  { "tools-raise", GTK_STOCK_GO_UP,
    N_("R_aise Tool"), NULL,
    N_("Raise tool"),
    G_CALLBACK (tools_raise_cmd_callback),
    NULL },

  { "tools-raise-to-top", GTK_STOCK_GOTO_TOP,
    N_("Ra_ise to Top"), NULL,
    N_("Raise tool to top"),
    G_CALLBACK (tools_raise_to_top_cmd_callback),
    NULL },

  { "tools-lower", GTK_STOCK_GO_DOWN,
    N_("L_ower Tool"), NULL,
    N_("Lower tool"),
    G_CALLBACK (tools_lower_cmd_callback),
    NULL },

  { "tools-lower-to-bottom", GTK_STOCK_GOTO_BOTTOM,
    N_("Lo_wer to Bottom"), NULL,
    N_("Lower tool to bottom"),
    G_CALLBACK (tools_lower_to_bottom_cmd_callback),
    NULL },

  { "tools-reset", GIMP_STOCK_RESET,
    N_("_Reset Order & Visibility"), NULL,
    N_("Reset tool order and visibility"),
    G_CALLBACK (tools_reset_cmd_callback),
    NULL }
};

static const GimpToggleActionEntry tools_toggle_actions[] =
{
  { "tools-visibility", GIMP_STOCK_VISIBLE,
    N_("_Show in Toolbox"), NULL, NULL,
    G_CALLBACK (tools_toggle_visibility_cmd_callback),
    TRUE,
    NULL /* FIXME */ }
};

static const GimpStringActionEntry tools_alternative_actions[] =
{
  { "tools-by-color-select-short", GIMP_STOCK_TOOL_BY_COLOR_SELECT,
    N_("_By Color"), NULL,
    N_("Select regions with similar colors"),
    "gimp-by-color-select-tool",
    GIMP_HELP_TOOL_BY_COLOR_SELECT },

  { "tools-rotate-arbitrary", GIMP_STOCK_TOOL_ROTATE,
    N_("_Arbitrary Rotation..."), "",
    N_("Rotate by an arbitrary angle"),
    "gimp-rotate-layer",
    GIMP_HELP_TOOL_ROTATE }
};

static const GimpEnumActionEntry tools_color_average_radius_actions[] =
{
  { "tools-color-average-radius-set", GIMP_STOCK_TOOL_COLOR_PICKER,
    "Set Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-color-average-radius-minimum", GIMP_STOCK_TOOL_COLOR_PICKER,
    "M Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-color-average-radius-maximum", GIMP_STOCK_TOOL_COLOR_PICKER,
    "Maximum Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-color-average-radius-decrease", GIMP_STOCK_TOOL_COLOR_PICKER,
    "Decrease Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-color-average-radius-increase", GIMP_STOCK_TOOL_COLOR_PICKER,
    "Increase Color Picker Radius", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-color-average-radius-decrease-skip",
    GIMP_STOCK_TOOL_COLOR_PICKER,
    "Decrease Color Picker Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-color-average-radius-increase-skip",
    GIMP_STOCK_TOOL_COLOR_PICKER,
    "Increase Color Picker Radius More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_paint_brush_scale_actions[] =
{
  { "tools-paint-brush-scale-set", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Set Brush Scale", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-paint-brush-scale-minimum", GIMP_STOCK_TOOL_PAINTBRUSH,
    "M Brush Scale", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-paint-brush-scale-maximum", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Maximum Brush Scale", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-paint-brush-scale-decrease", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Decrease Brush Scale", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-paint-brush-scale-increase", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Increase Brush Scale", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-paint-brush-scale-decrease-skip", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Decrease Brush Scale More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-paint-brush-scale-increase-skip", GIMP_STOCK_TOOL_PAINTBRUSH,
    "Increase Brush Scale More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_ink_blob_size_actions[] =
{
  { "tools-ink-blob-size-set", GIMP_STOCK_TOOL_INK,
    "Set Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-ink-blob-size-minimum", GIMP_STOCK_TOOL_INK,
    "M Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-ink-blob-size-maximum", GIMP_STOCK_TOOL_INK,
    "Maximum Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-ink-blob-size-decrease", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-size-increase", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Size", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-ink-blob-size-decrease-skip", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Size More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-size-increase-skip", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Size More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_ink_blob_aspect_actions[] =
{
  { "tools-ink-blob-aspect-set", GIMP_STOCK_TOOL_INK,
    "Set Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-ink-blob-aspect-minimum", GIMP_STOCK_TOOL_INK,
    "M Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-ink-blob-aspect-maximum", GIMP_STOCK_TOOL_INK,
    "Maximum Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-ink-blob-aspect-decrease", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-aspect-increase", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Aspect", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-ink-blob-aspect-decrease-skip", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-aspect-increase-skip", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Aspect More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_ink_blob_angle_actions[] =
{
  { "tools-ink-blob-angle-set", GIMP_STOCK_TOOL_INK,
    "Set Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-ink-blob-angle-minimum", GIMP_STOCK_TOOL_INK,
    "M Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-ink-blob-angle-maximum", GIMP_STOCK_TOOL_INK,
    "Maximum Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-ink-blob-angle-decrease", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-angle-increase", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Angle", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-ink-blob-angle-decrease-skip", GIMP_STOCK_TOOL_INK,
    "Decrease Ink Blob Angle More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-ink-blob-angle-increase-skip", GIMP_STOCK_TOOL_INK,
    "Increase Ink Blob Angle More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_foreground_select_brush_size_actions[] =
{
  { "tools-foreground-select-brush-size-set",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Set Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-foreground-select-brush-size-minimum",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "M Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-foreground-select-brush-size-maximum",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Maximum Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-foreground-select-brush-size-decrease",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Decrease Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-foreground-select-brush-size-increase",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Increase Foreground Select Brush Size", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-foreground-select-brush-size-decrease-skip",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Decrease Foreground Select Brush Size More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-foreground-select-brush-size-increase-skip",
    GIMP_STOCK_TOOL_FOREGROUND_SELECT,
    "Increase Foreground Select Brush Size More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_value_1_actions[] =
{
  { "tools-value-1-set", GIMP_STOCK_TOOL_OPTIONS,
    "Set Value 1", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-value-1-minimum", GIMP_STOCK_TOOL_OPTIONS,
    "Minimize Value 1", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-value-1-maximum", GIMP_STOCK_TOOL_OPTIONS,
    "Maximize Value 1", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-value-1-decrease", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 1", "less", NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-value-1-increase", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 1", "greater", NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-value-1-decrease-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 1 More", "<control>less", NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-value-1-increase-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 1 More", "<control>greater", NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_value_2_actions[] =
{
  { "tools-value-2-set", GIMP_STOCK_TOOL_OPTIONS,
    "Set Value 2", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-value-2-minimum", GIMP_STOCK_TOOL_OPTIONS,
    "Minimize Value 2", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-value-2-maximum", GIMP_STOCK_TOOL_OPTIONS,
    "Maximize Value 2", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-value-2-decrease", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 2", "bracketleft", NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-value-2-increase", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 2", "bracketright", NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-value-2-decrease-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 2 More", "<shift>bracketleft", NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-value-2-increase-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 2 More", "<shift>bracketright", NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_value_3_actions[] =
{
  { "tools-value-3-set", GIMP_STOCK_TOOL_OPTIONS,
    "Set Value 3", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-value-3-minimum", GIMP_STOCK_TOOL_OPTIONS,
    "Minimize Value 3", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-value-3-maximum", GIMP_STOCK_TOOL_OPTIONS,
    "Maximize Value 3", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-value-3-decrease", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 3", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-value-3-increase", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 3", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-value-3-decrease-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 3 More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-value-3-increase-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 3 More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_value_4_actions[] =
{
  { "tools-value-4-set", GIMP_STOCK_TOOL_OPTIONS,
    "Set Value 4", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-value-4-minimum", GIMP_STOCK_TOOL_OPTIONS,
    "Minimize Value 4", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-value-4-maximum", GIMP_STOCK_TOOL_OPTIONS,
    "Maximize Value 4", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-value-4-decrease", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 4", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-value-4-increase", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 4", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "tools-value-4-decrease-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Decrease Value 4 More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "tools-value-4-increase-skip", GIMP_STOCK_TOOL_OPTIONS,
    "Increase Value 4 More", NULL, NULL,
    GIMP_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
};

static const GimpEnumActionEntry tools_object_1_actions[] =
{
  { "tools-object-1-set", GIMP_STOCK_TOOL_OPTIONS,
    "Select Object 1 by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-1-first", GIMP_STOCK_TOOL_OPTIONS,
    "First Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-1-last", GIMP_STOCK_TOOL_OPTIONS,
    "Last Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-1-previous", GIMP_STOCK_TOOL_OPTIONS,
    "Previous Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-1-next", GIMP_STOCK_TOOL_OPTIONS,
    "Next Object 1", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const GimpEnumActionEntry tools_object_2_actions[] =
{
  { "tools-object-2-set", GIMP_STOCK_TOOL_OPTIONS,
    "Select Object 2 by Index", NULL, NULL,
    GIMP_ACTION_SELECT_SET, TRUE,
    NULL },
  { "tools-object-2-first", GIMP_STOCK_TOOL_OPTIONS,
    "First Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "tools-object-2-last", GIMP_STOCK_TOOL_OPTIONS,
    "Last Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "tools-object-2-previous", GIMP_STOCK_TOOL_OPTIONS,
    "Previous Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "tools-object-2-next", GIMP_STOCK_TOOL_OPTIONS,
    "Next Object 2", NULL, NULL,
    GIMP_ACTION_SELECT_NEXT, FALSE,
    NULL }
};


void
tools_actions_setup (GimpActionGroup *group)
{
  GtkAction *action;
  GList     *list;

  gimp_action_group_add_actions (group,
                                 tools_actions,
                                 G_N_ELEMENTS (tools_actions));

  gimp_action_group_add_toggle_actions (group,
                                        tools_toggle_actions,
                                        G_N_ELEMENTS (tools_toggle_actions));

  gimp_action_group_add_string_actions (group,
                                        tools_alternative_actions,
                                        G_N_ELEMENTS (tools_alternative_actions),
                                        G_CALLBACK (tools_select_cmd_callback));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        "tools-by-color-select-short");
  gtk_action_set_accel_path (action, "<Actions>/tools/tools-by-color-select");

  gimp_action_group_add_enum_actions (group,
                                      tools_color_average_radius_actions,
                                      G_N_ELEMENTS (tools_color_average_radius_actions),
                                      G_CALLBACK (tools_color_average_radius_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      tools_paint_brush_scale_actions,
                                      G_N_ELEMENTS (tools_paint_brush_scale_actions),
                                      G_CALLBACK (tools_paint_brush_scale_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      tools_ink_blob_size_actions,
                                      G_N_ELEMENTS (tools_ink_blob_size_actions),
                                      G_CALLBACK (tools_ink_blob_size_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_ink_blob_aspect_actions,
                                      G_N_ELEMENTS (tools_ink_blob_aspect_actions),
                                      G_CALLBACK (tools_ink_blob_aspect_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_ink_blob_angle_actions,
                                      G_N_ELEMENTS (tools_ink_blob_angle_actions),
                                      G_CALLBACK (tools_ink_blob_angle_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      tools_foreground_select_brush_size_actions,
                                      G_N_ELEMENTS (tools_foreground_select_brush_size_actions),
                                      G_CALLBACK (tools_fg_select_brush_size_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      tools_value_1_actions,
                                      G_N_ELEMENTS (tools_value_1_actions),
                                      G_CALLBACK (tools_value_1_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_value_2_actions,
                                      G_N_ELEMENTS (tools_value_2_actions),
                                      G_CALLBACK (tools_value_2_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_value_3_actions,
                                      G_N_ELEMENTS (tools_value_3_actions),
                                      G_CALLBACK (tools_value_3_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_value_4_actions,
                                      G_N_ELEMENTS (tools_value_4_actions),
                                      G_CALLBACK (tools_value_4_cmd_callback));

  gimp_action_group_add_enum_actions (group,
                                      tools_object_1_actions,
                                      G_N_ELEMENTS (tools_object_1_actions),
                                      G_CALLBACK (tools_object_1_cmd_callback));
  gimp_action_group_add_enum_actions (group,
                                      tools_object_2_actions,
                                      G_N_ELEMENTS (tools_object_2_actions),
                                      G_CALLBACK (tools_object_2_cmd_callback));

  for (list = GIMP_LIST (group->gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      if (tool_info->menu_path)
        {
          GimpStringActionEntry  entry;
          const gchar           *stock_id;
          const gchar           *identifier;
          gchar                 *tmp;
          gchar                 *name;

          stock_id   = gimp_viewable_get_stock_id (GIMP_VIEWABLE (tool_info));
          identifier = gimp_object_get_name (GIMP_OBJECT (tool_info));

          tmp = g_strndup (identifier + strlen ("gimp-"),
                           strlen (identifier) - strlen ("gimp--tool"));
          name = g_strdup_printf ("tools-%s", tmp);
          g_free (tmp);

          entry.name        = name;
          entry.stock_id    = stock_id;
          entry.label       = tool_info->menu_path;
          entry.accelerator = tool_info->menu_accel;
          entry.tooltip     = tool_info->help;
          entry.help_id     = tool_info->help_id;
          entry.value       = identifier;

          gimp_action_group_add_string_actions (group,
                                                &entry, 1,
                                                G_CALLBACK (tools_select_cmd_callback));

          g_free (name);
        }
    }
}

void
tools_actions_update (GimpActionGroup *group,
                      gpointer         data)
{
  GimpContext   *context   = gimp_get_user_context (group->gimp);
  GimpToolInfo  *tool_info = gimp_context_get_tool (context);
  GimpContainer *container = context->gimp->tool_info_list;
  gboolean       raise     = FALSE;
  gboolean       lower     = FALSE;

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#define SET_ACTIVE(action,condition) \
        gimp_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("tools-visibility", tool_info);

  if (tool_info)
    {
      gint last_index;
      gint index;

      SET_ACTIVE ("tools-visibility", tool_info->visible);

      last_index = gimp_container_num_children (container) -1;
      index      = gimp_container_get_child_index   (container,
                                                     GIMP_OBJECT (tool_info));

      raise = index != 0;
      lower = index != last_index;
    }

  SET_SENSITIVE ("tools-raise",           raise);
  SET_SENSITIVE ("tools-raise-to-top",    raise);
  SET_SENSITIVE ("tools-lower",           lower);
  SET_SENSITIVE ("tools-lower-to-bottom", lower);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
