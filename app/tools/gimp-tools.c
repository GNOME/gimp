/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "widgets/ligmawidgets-utils.h"

#include "core/ligma.h"
#include "core/ligma-contexts.h"
#include "core/ligma-internal-data.h"
#include "core/ligmacontext.h"
#include "core/ligmalist.h"
#include "core/ligmatoolgroup.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "ligma-tool-options-manager.h"
#include "ligma-tools.h"
#include "ligmatooloptions-gui.h"
#include "tool_manager.h"

#include "ligmaairbrushtool.h"
#include "ligmaaligntool.h"
#include "ligmabrightnesscontrasttool.h"
#include "ligmabucketfilltool.h"
#include "ligmabycolorselecttool.h"
#include "ligmacagetool.h"
#include "ligmaclonetool.h"
#include "ligmacolorpickertool.h"
#include "ligmaconvolvetool.h"
#include "ligmacroptool.h"
#include "ligmacurvestool.h"
#include "ligmadodgeburntool.h"
#include "ligmaellipseselecttool.h"
#include "ligmaerasertool.h"
#include "ligmafliptool.h"
#include "ligmafreeselecttool.h"
#include "ligmaforegroundselecttool.h"
#include "ligmafuzzyselecttool.h"
#include "ligmagegltool.h"
#include "ligmagradienttool.h"
#include "ligmahandletransformtool.h"
#include "ligmahealtool.h"
#include "ligmainktool.h"
#include "ligmaiscissorstool.h"
#include "ligmalevelstool.h"
#include "ligmaoperationtool.h"
#include "ligmamagnifytool.h"
#include "ligmameasuretool.h"
#include "ligmamovetool.h"
#include "ligmamybrushtool.h"
#include "ligmanpointdeformationtool.h"
#include "ligmaoffsettool.h"
#include "ligmapaintbrushtool.h"
#include "ligmapaintselecttool.h"
#include "ligmapenciltool.h"
#include "ligmaperspectiveclonetool.h"
#include "ligmaperspectivetool.h"
#include "ligmathresholdtool.h"
#include "ligmarectangleselecttool.h"
#include "ligmarotatetool.h"
#include "ligmaseamlessclonetool.h"
#include "ligmascaletool.h"
#include "ligmasheartool.h"
#include "ligmasmudgetool.h"
#include "ligmatexttool.h"
#include "ligmatransform3dtool.h"
#include "ligmaunifiedtransformtool.h"
#include "ligmavectortool.h"
#include "ligmawarptool.h"

#include "ligma-intl.h"


#define TOOL_RC_FILE_VERSION 1


/*  local function prototypes  */

static void   ligma_tools_register       (GType                   tool_type,
                                         GType                   tool_options_type,
                                         LigmaToolOptionsGUIFunc  options_gui_func,
                                         LigmaContextPropMask     context_props,
                                         const gchar            *identifier,
                                         const gchar            *label,
                                         const gchar            *tooltip,
                                         const gchar            *menu_label,
                                         const gchar            *menu_accel,
                                         const gchar            *help_domain,
                                         const gchar            *help_data,
                                         const gchar            *icon_name,
                                         gpointer                data);

static void   ligma_tools_copy_structure (Ligma                   *ligma,
                                         LigmaContainer          *src_container,
                                         LigmaContainer          *dest_container,
                                         GHashTable             *tools);

/*  private variables  */

static GBinding *toolbox_groups_binding = NULL;
static gboolean  tool_options_deleted   = FALSE;


/*  public functions  */

void
ligma_tools_init (Ligma *ligma)
{
  LigmaToolRegisterFunc register_funcs[] =
  {
    /*  selection tools */

    ligma_rectangle_select_tool_register,
    ligma_ellipse_select_tool_register,
    ligma_free_select_tool_register,
    ligma_fuzzy_select_tool_register,
    ligma_by_color_select_tool_register,
    ligma_iscissors_tool_register,
    ligma_foreground_select_tool_register,
    ligma_paint_select_tool_register,

    /*  path tool */

    ligma_vector_tool_register,

    /*  non-modifying tools  */

    ligma_color_picker_tool_register,
    ligma_magnify_tool_register,
    ligma_measure_tool_register,

    /*  transform tools  */

    ligma_move_tool_register,
    ligma_align_tool_register,
    ligma_crop_tool_register,
    ligma_unified_transform_tool_register,
    ligma_rotate_tool_register,
    ligma_scale_tool_register,
    ligma_shear_tool_register,
    ligma_handle_transform_tool_register,
    ligma_perspective_tool_register,
    ligma_transform_3d_tool_register,
    ligma_flip_tool_register,
    ligma_cage_tool_register,
    ligma_warp_tool_register,
    ligma_n_point_deformation_tool_register,

    /*  paint tools  */

    ligma_seamless_clone_tool_register,
    ligma_text_tool_register,
    ligma_bucket_fill_tool_register,
    ligma_gradient_tool_register,
    ligma_pencil_tool_register,
    ligma_paintbrush_tool_register,
    ligma_eraser_tool_register,
    ligma_airbrush_tool_register,
    ligma_ink_tool_register,
    ligma_mybrush_tool_register,
    ligma_clone_tool_register,
    ligma_heal_tool_register,
    ligma_perspective_clone_tool_register,
    ligma_convolve_tool_register,
    ligma_smudge_tool_register,
    ligma_dodge_burn_tool_register,

    /*  filter tools  */

    ligma_brightness_contrast_tool_register,
    ligma_threshold_tool_register,
    ligma_levels_tool_register,
    ligma_curves_tool_register,
    ligma_offset_tool_register,
    ligma_gegl_tool_register,
    ligma_operation_tool_register
  };

  gint i;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_tool_options_create_folder ();

  ligma_container_freeze (ligma->tool_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (ligma_tools_register, ligma);
    }

  ligma_container_thaw (ligma->tool_info_list);

  ligma_tool_options_manager_init (ligma);

  tool_manager_init (ligma);

  toolbox_groups_binding = g_object_bind_property (
    ligma->config,            "toolbox-groups",
    ligma->tool_item_ui_list, "flat",
    G_BINDING_INVERT_BOOLEAN |
    G_BINDING_SYNC_CREATE);
}

void
ligma_tools_exit (Ligma *ligma)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_clear_object (&toolbox_groups_binding);

  tool_manager_exit (ligma);

  ligma_tool_options_manager_exit (ligma);

  for (list = ligma_get_tool_info_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaToolInfo *tool_info = list->data;

      ligma_tools_set_tool_options_gui (tool_info->tool_options, NULL);
    }
}

void
ligma_tools_restore (Ligma *ligma)
{
  LigmaObject *object;
  GList      *list;
  GError     *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /* restore tool order */
  ligma_tools_reset (ligma, ligma->tool_item_list, TRUE);

  /* make the generic operation tool invisible by default */
  object = ligma_container_get_child_by_name (ligma->tool_info_list,
                                             "ligma-operation-tool");
  if (object)
    g_object_set (object, "visible", FALSE, NULL);

  for (list = ligma_get_tool_info_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (list->data);

      /*  get default values from prefs (see bug #120832)  */
      ligma_config_reset (LIGMA_CONFIG (tool_info->tool_options));
    }

  if (! ligma_contexts_load (ligma, &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  if (! ligma_internal_data_load (ligma, &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  /*  make sure there is always a tool active, so broken config files
   *  can't leave us with no initial tool
   */
  if (! ligma_context_get_tool (ligma_get_user_context (ligma)))
    {
      ligma_context_set_tool (ligma_get_user_context (ligma),
                             ligma_get_tool_info_iter (ligma)->data);
    }

  for (list = ligma_get_tool_info_iter (ligma);
       list;
       list = g_list_next (list))
    {
      LigmaToolInfo           *tool_info = LIGMA_TOOL_INFO (list->data);
      LigmaToolOptionsGUIFunc  options_gui_func;

      /*  copy all context properties except those the tool actually
       *  uses, because the subsequent deserialize() on the tool
       *  options will only set the properties that were set to
       *  non-default values at the time of saving, and we want to
       *  keep these default values as if they have been saved.
       * (see bug #541586).
       */
      ligma_context_copy_properties (ligma_get_user_context (ligma),
                                    LIGMA_CONTEXT (tool_info->tool_options),
                                    LIGMA_CONTEXT_PROP_MASK_ALL &~
                                    (tool_info->context_props    |
                                     LIGMA_CONTEXT_PROP_MASK_TOOL |
                                     LIGMA_CONTEXT_PROP_MASK_PAINT_INFO));

      ligma_tool_options_deserialize (tool_info->tool_options, NULL);

      options_gui_func = g_object_get_data (G_OBJECT (tool_info),
                                            "ligma-tool-options-gui-func");

      if (! options_gui_func)
        options_gui_func = ligma_tool_options_empty_gui;

      ligma_tools_set_tool_options_gui_func (tool_info->tool_options,
                                            options_gui_func);
    }
}

void
ligma_tools_save (Ligma     *ligma,
                 gboolean  save_tool_options,
                 gboolean  always_save)
{
  LigmaConfigWriter *writer;
  GFile            *file;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (save_tool_options && (! tool_options_deleted || always_save))
    {
      GList  *list;
      GError *error = NULL;

      if (! ligma_contexts_save (ligma, &error))
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      if (! ligma_internal_data_save (ligma, &error))
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      ligma_tool_options_create_folder ();

      for (list = ligma_get_tool_info_iter (ligma);
           list;
           list = g_list_next (list))
        {
          LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (list->data);

          ligma_tool_options_serialize (tool_info->tool_options, NULL);
        }
    }

  file = ligma_directory_file ("toolrc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  writer = ligma_config_writer_new_from_file (file, TRUE, "LIGMA toolrc", NULL);

  if (writer)
    {
      ligma_tools_serialize (ligma, ligma->tool_item_list, writer);

      ligma_config_writer_finish (writer, "end of toolrc", NULL);
    }

  g_object_unref (file);
}

gboolean
ligma_tools_clear (Ligma    *ligma,
                  GError **error)
{
  GList    *list;
  gboolean  success = TRUE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  for (list = ligma_get_tool_info_iter (ligma);
       list && success;
       list = g_list_next (list))
    {
      LigmaToolInfo *tool_info = LIGMA_TOOL_INFO (list->data);

      success = ligma_tool_options_delete (tool_info->tool_options, NULL);
    }

  if (success)
    success = ligma_contexts_clear (ligma, error);

  if (success)
    success = ligma_internal_data_clear (ligma, error);

  if (success)
    tool_options_deleted = TRUE;

  return success;
}

gboolean
ligma_tools_serialize (Ligma             *ligma,
                      LigmaContainer    *container,
                      LigmaConfigWriter *writer)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), FALSE);

  ligma_config_writer_open (writer, "file-version");
  ligma_config_writer_printf (writer, "%d", TOOL_RC_FILE_VERSION);
  ligma_config_writer_close (writer);

  ligma_config_writer_linefeed (writer);

  return ligma_config_serialize (LIGMA_CONFIG (container), writer, NULL);
}

gboolean
ligma_tools_deserialize (Ligma          *ligma,
                        LigmaContainer *container,
                        GScanner      *scanner)
{
  enum
  {
    FILE_VERSION = 1
  };

  LigmaContainer *src_container;
  GTokenType     token;
  guint          scope_id;
  guint          old_scope_id;
  gint           file_version = 0;
  gboolean       result       = FALSE;

  scope_id     = g_type_qname (LIGMA_TYPE_TOOL_GROUP);
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
              if (ligma_scanner_parse_int (scanner, &file_version))
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

  ligma_container_freeze (container);

  /* make sure the various LigmaToolItem types are registered */
  g_type_class_unref (g_type_class_ref (LIGMA_TYPE_TOOL_GROUP));
  g_type_class_unref (g_type_class_ref (LIGMA_TYPE_TOOL_INFO));

  ligma_container_clear (container);

  src_container = g_object_new (LIGMA_TYPE_LIST,
                                "children-type", LIGMA_TYPE_TOOL_ITEM,
                                "append",        TRUE,
                                NULL);

  if (ligma_config_deserialize (LIGMA_CONFIG (src_container),
                               scanner, 0, NULL))
    {
      GHashTable *tools;
      GList      *list;

      result = TRUE;

      tools = g_hash_table_new (g_direct_hash, g_direct_equal);

      ligma_tools_copy_structure (ligma, src_container, container, tools);

      for (list = ligma_get_tool_info_iter (ligma);
           list;
           list = g_list_next (list))
        {
          LigmaToolInfo *tool_info = list->data;

          if (! tool_info->hidden && ! g_hash_table_contains (tools, tool_info))
            {
              if (tool_info->experimental)
                {
                  /* if an experimental tool is not in the file, just add it to
                   * the tool-item list.
                   */
                  ligma_container_add (container, LIGMA_OBJECT (tool_info));
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

  ligma_container_thaw (container);

  return result;
}

void
ligma_tools_reset (Ligma          *ligma,
                  LigmaContainer *container,
                  gboolean       user_toolrc)
{
  GList *files = NULL;
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_CONTAINER (container));

  if (user_toolrc)
    files = g_list_prepend (files, ligma_directory_file       ("toolrc", NULL));
  files = g_list_prepend (files, ligma_sysconf_directory_file ("toolrc", NULL));

  files = g_list_reverse (files);

  ligma_container_freeze (container);

  ligma_container_clear (container);

  for (list = files; list; list = g_list_next (list))
    {
      GScanner *scanner;
      GFile    *file  = list->data;
      GError   *error = NULL;

      if (ligma->be_verbose)
        g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

      scanner = ligma_scanner_new_file (file, &error);

      if (scanner && ligma_tools_deserialize (ligma, container, scanner))
        {
          ligma_scanner_unref (scanner);

          break;
        }
      else
        {
          if (error->code != G_IO_ERROR_NOT_FOUND)
            {
              ligma_message_literal (ligma, NULL,
                                    LIGMA_MESSAGE_WARNING, error->message);
            }

          g_clear_error (&error);

          ligma_container_clear (container);
        }

      g_clear_pointer (&scanner, ligma_scanner_unref);
    }

  g_list_free_full (files, g_object_unref);

  if (ligma_container_is_empty (container))
    {
      if (ligma->be_verbose)
        g_print ("Using default tool order\n");

      ligma_tools_copy_structure (ligma, ligma->tool_info_list, container, NULL);
    }

  ligma_container_thaw (container);
}


/*  private functions  */

static void
ligma_tools_register (GType                   tool_type,
                     GType                   tool_options_type,
                     LigmaToolOptionsGUIFunc  options_gui_func,
                     LigmaContextPropMask     context_props,
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
  Ligma         *ligma = (Ligma *) data;
  LigmaToolInfo *tool_info;
  const gchar  *paint_core_name;
  gboolean      visible;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (g_type_is_a (tool_type, LIGMA_TYPE_TOOL));
  g_return_if_fail (tool_options_type == G_TYPE_NONE ||
                    g_type_is_a (tool_options_type, LIGMA_TYPE_TOOL_OPTIONS));

  if (tool_options_type == G_TYPE_NONE)
    tool_options_type = LIGMA_TYPE_TOOL_OPTIONS;

  if (tool_type == LIGMA_TYPE_PENCIL_TOOL)
    {
      paint_core_name = "ligma-pencil";
    }
  else if (tool_type == LIGMA_TYPE_PAINTBRUSH_TOOL)
    {
      paint_core_name = "ligma-paintbrush";
    }
  else if (tool_type == LIGMA_TYPE_ERASER_TOOL)
    {
      paint_core_name = "ligma-eraser";
    }
  else if (tool_type == LIGMA_TYPE_AIRBRUSH_TOOL)
    {
      paint_core_name = "ligma-airbrush";
    }
  else if (tool_type == LIGMA_TYPE_CLONE_TOOL)
    {
      paint_core_name = "ligma-clone";
    }
  else if (tool_type == LIGMA_TYPE_HEAL_TOOL)
    {
      paint_core_name = "ligma-heal";
    }
  else if (tool_type == LIGMA_TYPE_PERSPECTIVE_CLONE_TOOL)
    {
      paint_core_name = "ligma-perspective-clone";
    }
  else if (tool_type == LIGMA_TYPE_CONVOLVE_TOOL)
    {
      paint_core_name = "ligma-convolve";
    }
  else if (tool_type == LIGMA_TYPE_SMUDGE_TOOL)
    {
      paint_core_name = "ligma-smudge";
    }
  else if (tool_type == LIGMA_TYPE_DODGE_BURN_TOOL)
    {
      paint_core_name = "ligma-dodge-burn";
    }
  else if (tool_type == LIGMA_TYPE_INK_TOOL)
    {
      paint_core_name = "ligma-ink";
    }
  else if (tool_type == LIGMA_TYPE_MYBRUSH_TOOL)
    {
      paint_core_name = "ligma-mybrush";
    }
  else
    {
      paint_core_name = "ligma-paintbrush";
    }

  tool_info = ligma_tool_info_new (ligma,
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

  visible = (! g_type_is_a (tool_type, LIGMA_TYPE_FILTER_TOOL));

  ligma_tool_item_set_visible (LIGMA_TOOL_ITEM (tool_info), visible);

  /* hack to hide the operation tool entirely */
  if (tool_type == LIGMA_TYPE_OPERATION_TOOL)
    tool_info->hidden = TRUE;

  /* hack to not require experimental tools to be present in toolrc */
  if (tool_type == LIGMA_TYPE_N_POINT_DEFORMATION_TOOL ||
      tool_type == LIGMA_TYPE_SEAMLESS_CLONE_TOOL      ||
      tool_type == LIGMA_TYPE_PAINT_SELECT_TOOL)
    {
      tool_info->experimental = TRUE;
    }

  g_object_set_data (G_OBJECT (tool_info), "ligma-tool-options-gui-func",
                     options_gui_func);

  ligma_container_add (ligma->tool_info_list, LIGMA_OBJECT (tool_info));
  g_object_unref (tool_info);

  if (tool_type == LIGMA_TYPE_PAINTBRUSH_TOOL)
    ligma_tool_info_set_standard (ligma, tool_info);
}

static void
ligma_tools_copy_structure (Ligma          *ligma,
                           LigmaContainer *src_container,
                           LigmaContainer *dest_container,
                           GHashTable    *tools)
{
  GList *list;

  for (list = LIGMA_LIST (src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaToolItem *src_tool_item  = list->data;
      LigmaToolItem *dest_tool_item = NULL;

      if (LIGMA_IS_TOOL_GROUP (src_tool_item))
        {
          dest_tool_item = LIGMA_TOOL_ITEM (ligma_tool_group_new ());

          ligma_tools_copy_structure (
            ligma,
            ligma_viewable_get_children (LIGMA_VIEWABLE (src_tool_item)),
            ligma_viewable_get_children (LIGMA_VIEWABLE (dest_tool_item)),
            tools);

          ligma_tool_group_set_active_tool (
            LIGMA_TOOL_GROUP (dest_tool_item),
            ligma_tool_group_get_active_tool (LIGMA_TOOL_GROUP (src_tool_item)));
        }
      else
        {
          dest_tool_item = LIGMA_TOOL_ITEM (
            ligma_get_tool_info (ligma, ligma_object_get_name (src_tool_item)));

          if (dest_tool_item)
            {
              if (! LIGMA_TOOL_INFO (dest_tool_item)->hidden)
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
          ligma_tool_item_set_visible (
            dest_tool_item,
            ligma_tool_item_get_visible (src_tool_item));

          ligma_container_add (dest_container,
                              LIGMA_OBJECT (dest_tool_item));

          g_object_unref (dest_tool_item);
        }
    }
}
