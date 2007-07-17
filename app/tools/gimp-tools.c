/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimp-contexts.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpresets.h"

#include "gimp-tools.h"
#include "gimptooloptions-gui.h"
#include "tool_manager.h"

#include "gimpairbrushtool.h"
#include "gimpaligntool.h"
#include "gimpblendtool.h"
#include "gimpbrightnesscontrasttool.h"
#include "gimpbucketfilltool.h"
#include "gimpbycolorselecttool.h"
#include "gimpclonetool.h"
#include "gimpcolorbalancetool.h"
#include "gimpcolorizetool.h"
#include "gimpcolorpickertool.h"
#include "gimpconvolvetool.h"
#include "gimpcroptool.h"
#include "gimpcurvestool.h"
#include "gimpdodgeburntool.h"
#include "gimpellipseselecttool.h"
#include "gimperasertool.h"
#include "gimpfliptool.h"
#include "gimpfreeselecttool.h"
#include "gimpforegroundselecttool.h"
#include "gimpfuzzyselecttool.h"
#include "gimphealtool.h"
#include "gimphuesaturationtool.h"
#include "gimpinktool.h"
#include "gimpiscissorstool.h"
#include "gimplevelstool.h"
#include "gimpmagnifytool.h"
#include "gimpmeasuretool.h"
#include "gimpmovetool.h"
#include "gimppaintbrushtool.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpperspectivetool.h"
#include "gimpposterizetool.h"
#include "gimpthresholdtool.h"
#include "gimprectangleselecttool.h"
#include "gimprotatetool.h"
#include "gimpscaletool.h"
#include "gimpsheartool.h"
#include "gimpsmudgetool.h"
#include "gimptexttool.h"
#include "gimpvectortool.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_tools_register (GType                   tool_type,
                                   GType                   tool_options_type,
                                   GimpToolOptionsGUIFunc  options_gui_func,
                                   GimpContextPropMask     context_props,
                                   const gchar            *identifier,
                                   const gchar            *blurb,
                                   const gchar            *help,
                                   const gchar            *menu_path,
                                   const gchar            *menu_accel,
                                   const gchar            *help_domain,
                                   const gchar            *help_data,
                                   const gchar            *stock_id,
                                   gpointer                data);


/*  private variables  */

static gboolean   tool_options_deleted = FALSE;


/*  public functions  */

void
gimp_tools_init (Gimp *gimp)
{
  GimpToolRegisterFunc register_funcs[] =
  {
    /*  register tools in reverse order  */

    /*  color tools  */
    gimp_posterize_tool_register,
    gimp_curves_tool_register,
    gimp_levels_tool_register,
    gimp_threshold_tool_register,
    gimp_brightness_contrast_tool_register,
    gimp_colorize_tool_register,
    gimp_hue_saturation_tool_register,
    gimp_color_balance_tool_register,

    /*  paint tools  */

    gimp_dodge_burn_tool_register,
    gimp_smudge_tool_register,
    gimp_convolve_tool_register,
    gimp_perspective_clone_tool_register,
    gimp_heal_tool_register,
    gimp_clone_tool_register,
    gimp_ink_tool_register,
    gimp_airbrush_tool_register,
    gimp_eraser_tool_register,
    gimp_paintbrush_tool_register,
    gimp_pencil_tool_register,
    gimp_blend_tool_register,
    gimp_bucket_fill_tool_register,
    gimp_text_tool_register,

    /*  transform tools  */

    gimp_flip_tool_register,
    gimp_perspective_tool_register,
    gimp_shear_tool_register,
    gimp_scale_tool_register,
    gimp_rotate_tool_register,
    gimp_crop_tool_register,
    gimp_align_tool_register,
    gimp_move_tool_register,

    /*  non-modifying tools  */

    gimp_measure_tool_register,
    gimp_magnify_tool_register,
    gimp_color_picker_tool_register,

    /*  path tool */

    gimp_vector_tool_register,

    /*  selection tools */

    gimp_foreground_select_tool_register,
    gimp_iscissors_tool_register,
    gimp_by_color_select_tool_register,
    gimp_fuzzy_select_tool_register,
    gimp_free_select_tool_register,
    gimp_ellipse_select_tool_register,
    gimp_rect_select_tool_register
  };

  GList *default_order = NULL;
  GList *list;
  gint   i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager_init (gimp);

  gimp_container_freeze (gimp->tool_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (gimp_tools_register, gimp);
    }

  gimp_container_thaw (gimp->tool_info_list);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      const gchar *identifier = gimp_object_get_name (list->data);

      default_order = g_list_prepend (default_order, g_strdup (identifier));
    }

  default_order = g_list_reverse (default_order);

  g_object_set_data (G_OBJECT (gimp),
                     "gimp-tools-default-order", default_order);
}

void
gimp_tools_exit (Gimp *gimp)
{
  GList *default_order;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  default_order = g_object_get_data (G_OBJECT (gimp),
                                     "gimp-tools-default-order");

  g_list_foreach (default_order, (GFunc) g_free, NULL);
  g_list_free (default_order);

  g_object_set_data (G_OBJECT (gimp), "gimp-tools-default-order", NULL);

  tool_manager_exit (gimp);
}

void
gimp_tools_restore (Gimp *gimp)
{
  GimpContainer *gimp_list;
  gchar         *filename;
  GList         *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_list = gimp_list_new (GIMP_TYPE_TOOL_INFO, FALSE);

  filename = gimp_personal_rc_file ("toolrc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  if (gimp_config_deserialize_file (GIMP_CONFIG (gimp_list), filename,
                                    NULL, NULL))
    {
      gint n = gimp_container_num_children (gimp->tool_info_list);
      gint i;

      gimp_list_reverse (GIMP_LIST (gimp_list));

      for (list = GIMP_LIST (gimp_list)->list, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          const gchar *name;
          GimpObject  *object;

          name = gimp_object_get_name (list->data);

          object = gimp_container_get_child_by_name (gimp->tool_info_list,
                                                     name);

          if (object)
            {
              g_object_set (object,
                            "visible", GIMP_TOOL_INFO (list->data)->visible,
                            NULL);

              gimp_container_reorder (gimp->tool_info_list,
                                      object, MIN (i, n - 1));
            }
        }
    }

  g_free (filename);
  g_object_unref (gimp_list);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

      /*  get default values from prefs (see bug #120832)  */
      gimp_tool_options_reset (tool_info->tool_options);
    }

  gimp_contexts_load (gimp);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list;
       list = g_list_next (list))
    {
      GimpToolInfo           *tool_info = GIMP_TOOL_INFO (list->data);
      GimpToolOptionsGUIFunc  options_gui_func;
      GtkWidget              *options_gui;

      gimp_context_copy_properties (gimp_get_user_context (gimp),
                                    GIMP_CONTEXT (tool_info->tool_options),
                                    GIMP_CONTEXT_ALL_PROPS_MASK);

      gimp_tool_options_deserialize (tool_info->tool_options, NULL);

      options_gui_func = g_object_get_data (G_OBJECT (tool_info),
                                            "gimp-tool-options-gui-func");

      if (options_gui_func)
        {
          options_gui = (* options_gui_func) (tool_info->tool_options);
        }
      else
        {
          GtkWidget *label;

          options_gui = gimp_tool_options_gui (tool_info->tool_options);

          label = gtk_label_new (_("This tool has\nno options."));
          gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
          gimp_label_set_attributes (GTK_LABEL (label),
                                     PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                     -1);
          gtk_box_pack_start (GTK_BOX (options_gui), label, FALSE, FALSE, 6);
          gtk_widget_show (label);
        }

      g_object_set_data (G_OBJECT (tool_info->tool_options),
                         "gimp-tool-options-gui", options_gui);

      if (tool_info->presets)
        gimp_tool_presets_load (tool_info->presets, NULL);
    }
}

void
gimp_tools_save (Gimp     *gimp,
                 gboolean  save_tool_options,
                 gboolean  always_save)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (save_tool_options && (! tool_options_deleted || always_save))
    {
      GList *list;

      gimp_contexts_save (gimp);

      gimp_tool_options_create_folder ();

      for (list = GIMP_LIST (gimp->tool_info_list)->list;
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

          gimp_tool_options_serialize (tool_info->tool_options, NULL);
        }
    }

  filename = gimp_personal_rc_file ("toolrc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  gimp_config_serialize_to_file (GIMP_CONFIG (gimp->tool_info_list),
                                 filename,
                                 "GIMP toolrc",
                                 "end of toolrc",
                                 NULL, NULL);
  g_free (filename);
}

gboolean
gimp_tools_clear (Gimp    *gimp,
                  GError **error)
{
  GList    *list;
  gboolean  success = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  for (list = GIMP_LIST (gimp->tool_info_list)->list;
       list && success;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

      success = gimp_tool_options_delete (tool_info->tool_options, NULL);
    }

  if (success)
    success = gimp_contexts_clear (gimp, error);

  if (success)
    tool_options_deleted = TRUE;

  return success;
}

GList *
gimp_tools_get_default_order (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_get_data (G_OBJECT (gimp),
                            "gimp-tools-default-order");
}


/*  private functions  */

static void
gimp_tools_register (GType                   tool_type,
                     GType                   tool_options_type,
                     GimpToolOptionsGUIFunc  options_gui_func,
                     GimpContextPropMask     context_props,
                     const gchar            *identifier,
                     const gchar            *blurb,
                     const gchar            *help,
                     const gchar            *menu_path,
                     const gchar            *menu_accel,
                     const gchar            *help_domain,
                     const gchar            *help_data,
                     const gchar            *stock_id,
                     gpointer                data)
{
  Gimp         *gimp = (Gimp *) data;
  GimpToolInfo *tool_info;
  const gchar  *paint_core_name;
  gboolean      visible;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (g_type_is_a (tool_type, GIMP_TYPE_TOOL));
  g_return_if_fail (tool_options_type == G_TYPE_NONE ||
                    g_type_is_a (tool_options_type, GIMP_TYPE_TOOL_OPTIONS));

  if (tool_options_type == G_TYPE_NONE)
    tool_options_type = GIMP_TYPE_TOOL_OPTIONS;

  if (tool_type == GIMP_TYPE_PENCIL_TOOL)
    {
      paint_core_name = "gimp-pencil";
    }
  else if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    {
      paint_core_name = "gimp-paintbrush";
    }
  else if (tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      paint_core_name = "gimp-eraser";
    }
  else if (tool_type == GIMP_TYPE_AIRBRUSH_TOOL)
    {
      paint_core_name = "gimp-airbrush";
    }
  else if (tool_type == GIMP_TYPE_CLONE_TOOL)
    {
      paint_core_name = "gimp-clone";
    }
  else if (tool_type == GIMP_TYPE_HEAL_TOOL)
    {
      paint_core_name = "gimp-heal";
    }
  else if (tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL)
    {
      paint_core_name = "gimp-perspective-clone";
    }
  else if (tool_type == GIMP_TYPE_CONVOLVE_TOOL)
    {
      paint_core_name = "gimp-convolve";
    }
  else if (tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      paint_core_name = "gimp-smudge";
    }
  else if (tool_type == GIMP_TYPE_DODGE_BURN_TOOL)
    {
      paint_core_name = "gimp-dodge-burn";
    }
  else if (tool_type == GIMP_TYPE_INK_TOOL)
    {
      paint_core_name = "gimp-ink";
    }
  else
    {
      paint_core_name = "gimp-paintbrush";
    }

  tool_info = gimp_tool_info_new (gimp,
                                  tool_type,
                                  tool_options_type,
                                  context_props,
                                  identifier,
                                  blurb,
                                  help,
                                  menu_path,
                                  menu_accel,
                                  help_domain,
                                  help_data,
                                  paint_core_name,
                                  stock_id);

  visible = (! g_type_is_a (tool_type, GIMP_TYPE_IMAGE_MAP_TOOL));

  g_object_set (tool_info, "visible", visible, NULL);
  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-default-visible",
                     GINT_TO_POINTER (visible));

  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-options-gui-func",
                     options_gui_func);

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
  g_object_unref (tool_info);

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    gimp_tool_info_set_standard (gimp, tool_info);
}
