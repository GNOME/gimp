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
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimptoolgroup.h"
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
#include "gimptransform3dtool.h"
#include "gimpunifiedtransformtool.h"
#include "gimpvectortool.h"
#include "gimpwarptool.h"

#include "gimp-intl.h"


#define TOOL_RC_FILE_VERSION 1


/*  local function prototypes  */

static void   gimp_tools_register       (GType                   tool_type,
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

static void   gimp_tools_copy_structure (Gimp                   *gimp,
                                         GimpContainer          *src_container,
                                         GimpContainer          *dest_container,
                                         GHashTable             *tools);

/*  private variables  */

static GBinding *toolbox_groups_binding = NULL;
static gboolean  tool_options_deleted   = FALSE;


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
    gimp_transform_3d_tool_register,
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

  gint i;

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

  toolbox_groups_binding = g_object_bind_property (
    gimp->config,            "toolbox-groups",
    gimp->tool_item_ui_list, "flat",
    G_BINDING_INVERT_BOOLEAN |
    G_BINDING_SYNC_CREATE);
}

void
gimp_tools_exit (Gimp *gimp)
{
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_clear_object (&toolbox_groups_binding);

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
  GimpObject *object;
  GList      *list;
  GError     *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* restore tool order */
  gimp_tools_reset (gimp, gimp->tool_item_list, TRUE);

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
  GimpConfigWriter *writer;
  GFile            *file;

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

  writer = gimp_config_writer_new_gfile (file, TRUE, "GIMP toolrc", NULL);

  if (writer)
    {
      gimp_tools_serialize (gimp, gimp->tool_item_list, writer);

      gimp_config_writer_finish (writer, "end of toolrc", NULL);
    }

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

gboolean
gimp_tools_serialize (Gimp             *gimp,
                      GimpContainer    *container,
                      GimpConfigWriter *writer)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), FALSE);

  gimp_config_writer_open (writer, "file-version");
  gimp_config_writer_printf (writer, "%d", TOOL_RC_FILE_VERSION);
  gimp_config_writer_close (writer);

  gimp_config_writer_linefeed (writer);

  return gimp_config_serialize (GIMP_CONFIG (container), writer, NULL);
}

gboolean
gimp_tools_deserialize (Gimp          *gimp,
                        GimpContainer *container,
                        GScanner      *scanner)
{
  enum
  {
    FILE_VERSION = 1
  };

  GimpContainer *src_container;
  GTokenType     token;
  guint          scope_id;
  guint          old_scope_id;
  gint           file_version = 0;
  gboolean       result       = FALSE;

  scope_id     = g_type_qname (GIMP_TYPE_TOOL_GROUP);
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id,
                              "file-version",
                              GINT_TO_POINTER (FILE_VERSION));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token &&
         (token != G_TOKEN_LEFT_PAREN                 ||
          ! file_version))
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case FILE_VERSION:
              token = G_TOKEN_INT;
              if (gimp_scanner_parse_int (scanner, &file_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  g_scanner_set_scope (scanner, old_scope_id);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);

      return FALSE;
    }
  else if (file_version != TOOL_RC_FILE_VERSION)
    {
      g_scanner_error (scanner, "wrong toolrc file format version");

      return FALSE;
    }

  gimp_container_freeze (container);

  /* make sure the various GimpToolItem types are registered */
  g_type_class_unref (g_type_class_ref (GIMP_TYPE_TOOL_GROUP));
  g_type_class_unref (g_type_class_ref (GIMP_TYPE_TOOL_INFO));

  gimp_container_clear (container);

  src_container = g_object_new (GIMP_TYPE_LIST,
                                "children-type", GIMP_TYPE_TOOL_ITEM,
                                "append",        TRUE,
                                NULL);

  if (gimp_config_deserialize (GIMP_CONFIG (src_container),
                               scanner, 0, NULL))
    {
      GHashTable *tools;
      GList      *list;

      result = TRUE;

      tools = g_hash_table_new (g_direct_hash, g_direct_equal);

      gimp_tools_copy_structure (gimp, src_container, container, tools);

      for (list = gimp_get_tool_info_iter (gimp);
           list;
           list = g_list_next (list))
        {
          GimpToolInfo *tool_info = list->data;

          if (! tool_info->hidden && ! g_hash_table_contains (tools, tool_info))
            {
              if (tool_info->experimental)
                {
                  /* if an experimental tool is not in the file, just add it to
                   * the tool-item list.
                   */
                  gimp_container_add (container, GIMP_OBJECT (tool_info));
                }
              else
                {
                  /* otherwise, it means we added a new stable tool.  this must
                   * be the user toolrc file; rejct it, so that we fall back to
                   * the default toolrc file, which should contain the missing
                   * tool.
                   */
                  g_scanner_error (scanner, "missing tools in toolrc file");

                  result = FALSE;

                  break;
                }
            }
        }

      g_hash_table_unref (tools);
    }

  g_object_unref (src_container);

  gimp_container_thaw (container);

  return result;
}

void
gimp_tools_reset (Gimp          *gimp,
                  GimpContainer *container,
                  gboolean       user_toolrc)
{
  GList *files = NULL;
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_CONTAINER (container));

  if (user_toolrc)
    files = g_list_prepend (files, gimp_directory_file       ("toolrc", NULL));
  files = g_list_prepend (files, gimp_sysconf_directory_file ("toolrc", NULL));

  files = g_list_reverse (files);

  gimp_container_freeze (container);

  gimp_container_clear (container);

  for (list = files; list; list = g_list_next (list))
    {
      GScanner *scanner;
      GFile    *file  = list->data;
      GError   *error = NULL;

      if (gimp->be_verbose)
        g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

      scanner = gimp_scanner_new_gfile (file, &error);

      if (scanner && gimp_tools_deserialize (gimp, container, scanner))
        {
          gimp_scanner_destroy (scanner);

          break;
        }
      else
        {
          if (error->code != G_IO_ERROR_NOT_FOUND)
            {
              gimp_message_literal (gimp, NULL,
                                    GIMP_MESSAGE_WARNING, error->message);
            }

          g_clear_error (&error);

          gimp_container_clear (container);
        }

      g_clear_pointer (&scanner, gimp_scanner_destroy);
    }

  g_list_free_full (files, g_object_unref);

  if (gimp_container_is_empty (container))
    {
      if (gimp->be_verbose)
        g_print ("Using default tool order\n");

      gimp_tools_copy_structure (gimp, gimp->tool_info_list, container, NULL);
    }

  gimp_container_thaw (container);
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

  gimp_tool_item_set_visible (GIMP_TOOL_ITEM (tool_info), visible);

  /* hack to hide the operation tool entirely */
  if (tool_type == GIMP_TYPE_OPERATION_TOOL)
    tool_info->hidden = TRUE;

  /* hack to not require experimental tools to be present in toolrc */
  if (tool_type == GIMP_TYPE_N_POINT_DEFORMATION_TOOL ||
      tool_type == GIMP_TYPE_SEAMLESS_CLONE_TOOL)
    {
      tool_info->experimental = TRUE;
    }

  g_object_set_data (G_OBJECT (tool_info), "gimp-tool-options-gui-func",
                     options_gui_func);

  gimp_container_add (gimp->tool_info_list, GIMP_OBJECT (tool_info));
  g_object_unref (tool_info);

  if (tool_type == GIMP_TYPE_PAINTBRUSH_TOOL)
    gimp_tool_info_set_standard (gimp, tool_info);
}

static void
gimp_tools_copy_structure (Gimp          *gimp,
                           GimpContainer *src_container,
                           GimpContainer *dest_container,
                           GHashTable    *tools)
{
  GList *list;

  for (list = GIMP_LIST (src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpToolItem *src_tool_item  = list->data;
      GimpToolItem *dest_tool_item = NULL;

      if (GIMP_IS_TOOL_GROUP (src_tool_item))
        {
          dest_tool_item = GIMP_TOOL_ITEM (gimp_tool_group_new ());

          gimp_tools_copy_structure (
            gimp,
            gimp_viewable_get_children (GIMP_VIEWABLE (src_tool_item)),
            gimp_viewable_get_children (GIMP_VIEWABLE (dest_tool_item)),
            tools);

          gimp_tool_group_set_active_tool (
            GIMP_TOOL_GROUP (dest_tool_item),
            gimp_tool_group_get_active_tool (GIMP_TOOL_GROUP (src_tool_item)));
        }
      else
        {
          dest_tool_item = GIMP_TOOL_ITEM (
            gimp_get_tool_info (gimp, gimp_object_get_name (src_tool_item)));

          if (dest_tool_item)
            {
              if (! GIMP_TOOL_INFO (dest_tool_item)->hidden)
                {
                  g_object_ref (dest_tool_item);

                  if (tools)
                    g_hash_table_add (tools, dest_tool_item);
                }
              else
                {
                  dest_tool_item = NULL;
                }
            }
        }

      if (dest_tool_item)
        {
          gimp_tool_item_set_visible (
            dest_tool_item,
            gimp_tool_item_get_visible (src_tool_item));

          gimp_container_add (dest_container,
                              GIMP_OBJECT (dest_tool_item));

          g_object_unref (dest_tool_item);
        }
    }
}
