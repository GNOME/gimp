/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmageglprocedure.c
 * Copyright (C) 2016-2018 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "actions-types.h"

#include "config/ligmaguiconfig.h"

#include "operations/ligma-operation-config.h"
#include "operations/ligmaoperationsettings.h"

#include "core/ligma.h"
#include "core/ligma-memsize.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadisplay.h"
#include "core/ligmadrawable-operation.h"
#include "core/ligmaimage.h"
#include "core/ligmalayermask.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmasettings.h"
#include "core/ligmatoolinfo.h"

#include "tools/ligmaoperationtool.h"
#include "tools/tool_manager.h"

#include "ligmageglprocedure.h"

#include "ligma-intl.h"


static void     ligma_gegl_procedure_finalize            (GObject        *object);

static gint64   ligma_gegl_procedure_get_memsize         (LigmaObject     *object,
                                                         gint64         *gui_size);

static gchar  * ligma_gegl_procedure_get_description     (LigmaViewable   *viewable,
                                                         gchar         **tooltip);

static const gchar * ligma_gegl_procedure_get_menu_label (LigmaProcedure  *procedure);
static gboolean      ligma_gegl_procedure_get_sensitive  (LigmaProcedure  *procedure,
                                                         LigmaObject     *object,
                                                         const gchar   **reason);
static LigmaValueArray * ligma_gegl_procedure_execute     (LigmaProcedure  *procedure,
                                                         Ligma           *ligma,
                                                         LigmaContext    *context,
                                                         LigmaProgress   *progress,
                                                         LigmaValueArray *args,
                                                         GError        **error);
static void     ligma_gegl_procedure_execute_async       (LigmaProcedure  *procedure,
                                                         Ligma           *ligma,
                                                         LigmaContext    *context,
                                                         LigmaProgress   *progress,
                                                         LigmaValueArray *args,
                                                         LigmaDisplay    *display);


G_DEFINE_TYPE (LigmaGeglProcedure, ligma_gegl_procedure,
               LIGMA_TYPE_PROCEDURE)

#define parent_class ligma_gegl_procedure_parent_class


static void
ligma_gegl_procedure_class_init (LigmaGeglProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass    *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass  *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaProcedureClass *proc_class        = LIGMA_PROCEDURE_CLASS (klass);

  object_class->finalize            = ligma_gegl_procedure_finalize;

  ligma_object_class->get_memsize    = ligma_gegl_procedure_get_memsize;

  viewable_class->default_icon_name = "ligma-gegl";
  viewable_class->get_description   = ligma_gegl_procedure_get_description;

  proc_class->get_menu_label        = ligma_gegl_procedure_get_menu_label;
  proc_class->get_sensitive         = ligma_gegl_procedure_get_sensitive;
  proc_class->execute               = ligma_gegl_procedure_execute;
  proc_class->execute_async         = ligma_gegl_procedure_execute_async;
}

static void
ligma_gegl_procedure_init (LigmaGeglProcedure *proc)
{
}

static void
ligma_gegl_procedure_finalize (GObject *object)
{
  LigmaGeglProcedure *proc = LIGMA_GEGL_PROCEDURE (object);

  g_clear_object (&proc->default_settings);

  g_clear_pointer (&proc->operation,  g_free);
  g_clear_pointer (&proc->menu_label, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_gegl_procedure_get_memsize (LigmaObject *object,
                                 gint64     *gui_size)
{
  LigmaGeglProcedure *proc    = LIGMA_GEGL_PROCEDURE (object);
  gint64             memsize = 0;

  memsize += ligma_string_get_memsize (proc->operation);
  memsize += ligma_string_get_memsize (proc->menu_label);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
ligma_gegl_procedure_get_description (LigmaViewable  *viewable,
                                     gchar        **tooltip)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (viewable);

  if (tooltip)
    *tooltip = g_strdup (ligma_procedure_get_blurb (procedure));

  return g_strdup (ligma_procedure_get_label (procedure));
}

static const gchar *
ligma_gegl_procedure_get_menu_label (LigmaProcedure *procedure)
{
  LigmaGeglProcedure *proc = LIGMA_GEGL_PROCEDURE (procedure);

  if (proc->menu_label)
    return proc->menu_label;

  return LIGMA_PROCEDURE_CLASS (parent_class)->get_menu_label (procedure);
}

static gboolean
ligma_gegl_procedure_get_sensitive (LigmaProcedure  *procedure,
                                   LigmaObject     *object,
                                   const gchar   **reason)
{
  LigmaImage *image     = LIGMA_IMAGE (object);
  GList     *drawables = NULL;
  gboolean   sensitive = FALSE;

  if (image)
    drawables = ligma_image_get_selected_drawables (image);

  if (g_list_length (drawables) == 1)
    {
      LigmaDrawable *drawable  = drawables->data;
      LigmaItem     *item;

      if (LIGMA_IS_LAYER_MASK (drawable))
        item = LIGMA_ITEM (ligma_layer_mask_get_layer (LIGMA_LAYER_MASK (drawable)));
      else
        item = LIGMA_ITEM (drawable);

      sensitive = ! ligma_item_is_content_locked (item, NULL);

      if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
        sensitive = FALSE;
    }
  g_list_free (drawables);

  return sensitive;
}

static LigmaValueArray *
ligma_gegl_procedure_execute (LigmaProcedure   *procedure,
                             Ligma            *ligma,
                             LigmaContext     *context,
                             LigmaProgress    *progress,
                             LigmaValueArray  *args,
                             GError         **error)
{
  LigmaImage    *image;
  LigmaDrawable *drawable;
  GObject      *config;
  GeglNode     *node;

  image    = g_value_get_object (ligma_value_array_index (args, 1));
  drawable = g_value_get_object (ligma_value_array_index (args, 2));
  config   = g_value_get_object (ligma_value_array_index (args, 3));

  node = gegl_node_new_child (NULL,
                              "operation",
                              LIGMA_GEGL_PROCEDURE (procedure)->operation,
                              NULL);

  ligma_drawable_apply_operation_with_config (
    drawable,
    progress, ligma_procedure_get_label (procedure),
    node, config);

  g_object_unref (node);

  ligma_image_flush (image);

  return ligma_procedure_get_return_values (procedure, TRUE, NULL);
}

static void
ligma_gegl_procedure_execute_async (LigmaProcedure  *procedure,
                                   Ligma           *ligma,
                                   LigmaContext    *context,
                                   LigmaProgress   *progress,
                                   LigmaValueArray *args,
                                   LigmaDisplay    *display)
{
  LigmaGeglProcedure *gegl_procedure = LIGMA_GEGL_PROCEDURE (procedure);
  LigmaRunMode        run_mode;
  LigmaObject        *settings;
  LigmaTool          *active_tool;
  const gchar       *tool_name;

  run_mode = g_value_get_enum   (ligma_value_array_index (args, 0));
  settings = g_value_get_object (ligma_value_array_index (args, 3));

  if (! settings &&
      (run_mode != LIGMA_RUN_INTERACTIVE ||
       LIGMA_GUI_CONFIG (ligma->config)->filter_tool_use_last_settings))
    {
      /*  if we didn't get settings passed, get the last used settings  */

      GType          config_type;
      LigmaContainer *container;

      config_type = G_VALUE_TYPE (ligma_value_array_index (args, 3));

      container = ligma_operation_config_get_container (ligma, config_type,
                                                       (GCompareFunc)
                                                       ligma_settings_compare);

      settings = ligma_container_get_child_by_index (container, 0);

      /*  only use the settings if they are automatically created
       *  "last used" values, not if they were saved explicitly and
       *  have a zero timestamp; and if they are not a separator.
       */
      if (settings &&
          (LIGMA_SETTINGS (settings)->time == 0 ||
           ! ligma_object_get_name (settings)))
        {
          settings = NULL;
        }
    }

  if (run_mode == LIGMA_RUN_NONINTERACTIVE ||
      run_mode == LIGMA_RUN_WITH_LAST_VALS)
    {
      if (settings || run_mode == LIGMA_RUN_NONINTERACTIVE)
        {
          LigmaValueArray *return_vals;

          g_value_set_object (ligma_value_array_index (args, 3), settings);
          return_vals = ligma_procedure_execute (procedure, ligma, context, progress,
                                                args, NULL);
          ligma_value_array_unref (return_vals);
          return;
        }

      ligma_message (ligma,
                    G_OBJECT (progress), LIGMA_MESSAGE_WARNING,
                    _("There are no last settings for '%s', "
                      "showing the filter dialog instead."),
                    ligma_procedure_get_label (procedure));
    }

  if (! strcmp (gegl_procedure->operation, "ligma:brightness-contrast"))
    {
      tool_name = "ligma-brightness-contrast-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "ligma:curves"))
    {
      tool_name = "ligma-curves-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "ligma:levels"))
    {
      tool_name = "ligma-levels-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "ligma:threshold"))
    {
      tool_name = "ligma-threshold-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "ligma:offset"))
    {
      tool_name = "ligma-offset-tool";
    }
  else
    {
      tool_name = "ligma-operation-tool";
    }

  active_tool = tool_manager_get_active (ligma);

  /*  do not use the passed context because we need to set the active
   *  tool on the global user context
   */
  context = ligma_get_user_context (ligma);

  if (strcmp (ligma_object_get_name (active_tool->tool_info), tool_name))
    {
      LigmaToolInfo *tool_info = ligma_get_tool_info (ligma, tool_name);

      if (LIGMA_IS_TOOL_INFO (tool_info))
        ligma_context_set_tool (context, tool_info);
    }
  else
    {
      ligma_context_tool_changed (context);
    }

  active_tool = tool_manager_get_active (ligma);

  if (! strcmp (ligma_object_get_name (active_tool->tool_info), tool_name))
    {
      /*  Remember the procedure that created this tool, because
       *  we can't just switch to an operation tool using
       *  ligma_context_set_tool(), we also have to go through the
       *  initialization code below, otherwise we end up with a
       *  dummy tool that does nothing. See bug #776370.
       */
      g_object_set_data_full (G_OBJECT (active_tool), "ligma-gegl-procedure",
                              g_object_ref (procedure),
                              (GDestroyNotify) g_object_unref);

      if (! strcmp (tool_name, "ligma-operation-tool"))
        {
          ligma_operation_tool_set_operation (LIGMA_OPERATION_TOOL (active_tool),
                                             gegl_procedure->operation,
                                             ligma_procedure_get_label (procedure),
                                             ligma_procedure_get_label (procedure),
                                             ligma_procedure_get_label (procedure),
                                             ligma_viewable_get_icon_name (LIGMA_VIEWABLE (procedure)),
                                             ligma_procedure_get_help_id (procedure));
        }

      tool_manager_initialize_active (ligma, display);

      if (settings)
        ligma_filter_tool_set_config (LIGMA_FILTER_TOOL (active_tool),
                                     LIGMA_CONFIG (settings));
    }
}


/*  public functions  */

LigmaProcedure *
ligma_gegl_procedure_new (Ligma        *ligma,
                         LigmaRunMode  default_run_mode,
                         LigmaObject  *default_settings,
                         const gchar *operation,
                         const gchar *name,
                         const gchar *menu_label,
                         const gchar *tooltip,
                         const gchar *icon_name,
                         const gchar *help_id)
{
  LigmaProcedure     *procedure;
  LigmaGeglProcedure *gegl_procedure;
  GType              config_type;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (operation != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (menu_label != NULL, NULL);

  config_type = ligma_operation_config_get_type (ligma, operation, icon_name,
                                                LIGMA_TYPE_OPERATION_SETTINGS);

  procedure = g_object_new (LIGMA_TYPE_GEGL_PROCEDURE, NULL);

  gegl_procedure = LIGMA_GEGL_PROCEDURE (procedure);

  gegl_procedure->operation        = g_strdup (operation);
  gegl_procedure->default_run_mode = default_run_mode;
  gegl_procedure->menu_label       = g_strdup (menu_label);

  if (default_settings)
    gegl_procedure->default_settings = g_object_ref (default_settings);

  ligma_object_set_name (LIGMA_OBJECT (procedure), name);
  ligma_viewable_set_icon_name (LIGMA_VIEWABLE (procedure), icon_name);
  ligma_procedure_set_help (procedure,
                           tooltip,
                           tooltip,
                           help_id);
  ligma_procedure_set_static_attribution (procedure,
                                         "author", "copyright", "date");

  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_enum ("run-mode",
                                                     "Run mode",
                                                     "Run mode",
                                                     LIGMA_TYPE_RUN_MODE,
                                                     LIGMA_RUN_INTERACTIVE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_drawable ("drawable",
                                                         "Drawable",
                                                         "Input drawable",
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_object ("settings",
                                                    "Settings",
                                                    "Settings",
                                                    config_type,
                                                    LIGMA_PARAM_READWRITE));

  return procedure;
}
