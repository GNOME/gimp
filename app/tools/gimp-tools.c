/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "core/gimp.h"
#include "core/gimp-contexts.h"
#include "core/gimp-internal-data.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "gimp-tool-options-manager.h"
#include "gimp-tools.h"
#include "gimptooloptions-gui.h"
#include "tool_manager.h"

#include "gimpairbrushtool.h"
#include "gimpaligntool.h"
#include "gimpbrightnesscontrasttool.h"
#include "gimpbucketfilltool.h"
#include "gimpbycolorselecttool.h"
#include "gimpcagetool.h"
#include "gimpclonetool.h"
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
#include "gimpgegltool.h"
#include "gimpgradienttool.h"
#include "gimphandletransformtool.h"
#include "gimphealtool.h"
#include "gimpinktool.h"
#include "gimpiscissorstool.h"
#include "gimplevelstool.h"
#include "gimpoperationtool.h"
#include "gimpmagnifytool.h"
#include "gimpmeasuretool.h"
#include "gimpmovetool.h"
#include "gimpmybrushtool.h"
#include "gimpnpointdeformationtool.h"
#include "gimpoffsettool.h"
#include "gimppaintbrushtool.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpperspectivetool.h"
#include "gimpthresholdtool.h"
#include "gimprectangleselecttool.h"
#include "gimprotatetool.h"
#include "gimpseamlessclonetool.h"
#include "gimpscaletool.h"
#include "gimpsheartool.h"
#include "gimpsmudgetool.h"
#include "gimptexttool.h"
#include "gimpunifiedtransformtool.h"
#include "gimpvectortool.h"
#include "gimpwarptool.h"


/*  local function prototypes  */

static void   gimp_tools_register (GType                   tool_type,
                                   GType                   tool_options_type,
                                   GimpToolOptionsGUIFunc  options_gui_func,
                                   GimpContextPropMask     context_props,
                                   const gchar            *identifier,
                                   const gchar            *label,
                                   const gchar            *tooltip,
                                   const gchar            *menu_label,
                                   const gchar            *menu_accel,
                                   const gchar            *help_domain,
                                   const gchar            *help_data,
                                   const gchar            *icon_name,
                                   gpointer                data);


/*  private variables  */

static gboolean   tool_options_deleted = FALSE;


/*  public functions  */

void
gimp_tools_init (Gimp *gimp)
{
  GimpToolRegisterFunc register_funcs[] =
  {
    /*  selection tools */

    gimp_rectangle_select_tool_register,
    gimp_ellipse_select_tool_register,
    gimp_free_select_tool_register,
    gimp_fuzzy_select_tool_register,
    gimp_by_color_select_tool_register,
    gimp_iscissors_tool_register,
    gimp_foreground_select_tool_register,

    /*  path tool */

    gimp_vector_tool_register,

    /*  non-modifying tools  */

    gimp_color_picker_tool_register,
    gimp_magnify_tool_register,
    gimp_measure_tool_register,

    /*  transform tools  */

    gimp_move_tool_register,
    gimp_align_tool_register,
    gimp_crop_tool_register,
    gimp_unified_transform_tool_register,
    gimp_rotate_tool_register,
    gimp_scale_tool_register,
    gimp_shear_tool_register,
    gimp_handle_transform_tool_register,
    gimp_perspective_tool_register,
    gimp_flip_tool_register,
    gimp_cage_tool_register,
    gimp_warp_tool_register,
    gimp_n_point_deformation_tool_register,

    /*  paint tools  */

    gimp_seamless_clone_tool_register,
    gimp_text_tool_register,
    gimp_bucket_fill_tool_register,
    gimp_gradient_tool_register,
    gimp_pencil_tool_register,
    gimp_paintbrush_tool_register,
    gimp_eraser_tool_register,
    gimp_airbrush_tool_register,
    gimp_ink_tool_register,
    gimp_mybrush_tool_register,
    gimp_clone_tool_register,
    gimp_heal_tool_register,
    gimp_perspective_clone_tool_register,
    gimp_convolve_tool_register,
    gimp_smudge_tool_register,
    gimp_dodge_burn_tool_register,

    /*  filter tools  */

    gimp_brightness_contrast_tool_register,
    gimp_threshold_tool_register,
    gimp_levels_tool_register,
    gimp_curves_tool_register,
    gimp_offset_tool_register,
    gimp_gegl_tool_register,
    gimp_operation_tool_register
  };

  GList *default_order = NULL;
  GList *list;
  gint   i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_tool_options_create_folder ();

  gimp_container_freeze (gimp->tool_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (gimp_tools_register, gimp);
    }

  gimp_container_thaw (gimp->tool_info_list);

  gimp_tool_options_manager_init (gimp);

  tool_manager_init (gimp);

  for (list = gimp_get_tool_info_iter (gimp);
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
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  default_order = g_object_get_data (G_OBJECT (gimp),
                                     "gimp-tools-default-order");

  g_list_free_full (default_order, (GDestroyNotify) g_free);

  g_object_set_data (G_OBJECT (gimp), "gimp-tools-default-order", NULL);

  tool_manager_exit (gimp);

  gimp_tool_options_manager_exit (gimp);

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      gimp_tools_set_tool_options_gui (tool_info->tool_options, NULL);
    }
}

void
gimp_tools_restore (Gimp *gimp)
{
  GimpContainer *gimp_list;
  GimpObject    *object;
  GFile         *file;
  GList         *list;
  GError        *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_list = g_object_new (GIMP_TYPE_LIST,
                            "children-type", GIMP_TYPE_TOOL_INFO,
                            "append",        TRUE,
                            NULL);

  file = gimp_directory_file ("toolrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (gimp_config_deserialize_gfile (GIMP_CONFIG (gimp_list), file,
                                     NULL, &error))
    {
      gint n = gimp_container_get_n_children (gimp->tool_info_list);
      gint i;

      for (list = GIMP_LIST (gimp_list)->queue->head, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          const gchar *name = gimp_object_get_name (list->data);

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
  else
    {
      if (error->code != G_IO_ERROR_NOT_FOUND)
        gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING, error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);
  g_object_unref (gimp_list);

  /* make the generic operation tool invisible by default */
  object = gimp_container_get_child_by_name (gimp->tool_info_list,
                                             "gimp-operation-tool");
  if (object)
    g_object_set (object, "visible", FALSE, NULL);

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

      /*  get default values from prefs (see bug #120832)  */
      gimp_config_reset (GIMP_CONFIG (tool_info->tool_options));
    }

  if (! gimp_contexts_load (gimp, &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  if (! gimp_internal_data_load (gimp, &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  /*  make sure there is always a tool active, so broken config files
   *  can't leave us with no initial tool
   */
  if (! gimp_context_get_tool (gimp_get_user_context (gimp)))
    {
      gimp_context_set_tool (gimp_get_user_context (gimp),
                             gimp_get_tool_info_iter (gimp)->data);
    }

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo           *tool_info = GIMP_TOOL_INFO (list->data);
      GimpToolOptionsGUIFunc  options_gui_func;

      /*  copy all context properties except those the tool actually
       *  uses, because the subsequent deserialize() on the tool
       *  options will only set the properties that were set to
       *  non-default values at the time of saving, and we want to
       *  keep these default values as if they have been saved.
       * (see bug #541586).
       */
      gimp_context_copy_properties (gimp_get_user_context (gimp),
                                    GIMP_CONTEXT (tool_info->tool_options),
                                    GIMP_CONTEXT_PROP_MASK_ALL &~
                                    (tool_info->context_props    |
                                     GIMP_CONTEXT_PROP_MASK_TOOL |
                                     GIMP_CONTEXT_PROP_MASK_PAINT_INFO));

      gimp_tool_options_deserialize (tool_info->tool_options, NULL);

      options_gui_func = g_object_get_data (G_OBJECT (tool_info),
                                            "gimp-tool-options-gui-func");

      if (! options_gui_func)
        options_gui_func = gimp_tool_options_empty_gui;

      gimp_tools_set_tool_options_gui_func (tool_info->tool_options,
                                            options_gui_func);
    }
}

void
gimp_tools_save (Gimp     *gimp,
                 gboolean  save_tool_options,
                 gboolean  always_save)
{
  GFile *file;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (save_tool_options && (! tool_options_deleted || always_save))
    {
      GList  *list;
      GError *error = NULL;

      if (! gimp_contexts_save (gimp, &error))
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      if (! gimp_internal_data_save (gimp, &error))
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      gimp_tool_options_create_folder ();

      for (list = gimp_get_tool_info_iter (gimp);
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

          gimp_tool_options_serialize (tool_info->tool_options, NULL);
        }
    }

  file = gimp_directory_file ("toolrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_config_serialize_to_gfile (GIMP_CONFIG (gimp->tool_info_list),
                                  file,
                                  "GIMP toolrc",
                                  "end of toolrc",
                                  NULL, NULL);
  g_object_unref (file);
}

gboolean
gimp_tools_clear (Gimp    *gimp,
                  GError **error)
{
  GList    *list;
  gboolean  success = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  for (list = gimp_get_tool_info_iter (gimp);
       list && success;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = GIMP_TOOL_INFO (list->data);

      success = gimp_tool_options_delete (tool_info->tool_options, NULL);
    }

  if (success)
    success = gimp_contexts_clear (gimp, error);

  if (success)
    success = gimp_internal_data_clear (gimp, error);

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
                     const gchar            *label,
                     const gchar            *tooltip,
                     const gchar            *menu_label,
                     const gchar            *menu_accel,
                     const gchar            *help_domain,
                     const gchar            *help_data,
                     const gchar            *icon_name,
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
  else if (tool_type == GIMP_TYPE_MYBRUSH_TOOL)
    {
      paint_core_name = "gimp-mybrush";
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
                                  label,
                                  tooltip,
                                  menu_label,
                                  menu_accel,
                                  help_domain,
                                  help_data,
                                  paint_core_name,
                                  icon_name);

  visible = (! g_type_is_a (tool_type, GIMP_TYPE_FILTER_TOOL));

  g_object_set (tool_info, "visible", visible, NULL);

  /* hack to make the operation tools always invisible */
  if (tool_type == GIMP_TYPE_OPERATION_TOOL)
    tool_info->hidden = TRUE;

  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-default-visible",
                     GINT_TO_POINTER (visible));

  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-options-gui-func",
                     options_gui_func);

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
  g_object_unref (tool_info);

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    gimp_tool_info_set_standard (gimp, tool_info);
}
