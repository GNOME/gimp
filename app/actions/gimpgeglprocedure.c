/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgeglprocedure.c
 * Copyright (C) 2016-2018 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "actions-types.h"

#include "config/gimpguiconfig.h"

#include "operations/gimp-operation-config.h"
#include "operations/gimpoperationsettings.h"

#include "core/gimp.h"
#include "core/gimp-memsize.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdisplay.h"
#include "core/gimpdrawable-operation.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpimage.h"
#include "core/gimplayermask.h"
#include "core/gimpparamspecs.h"
#include "core/gimpsettings.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpoperationtool.h"
#include "tools/tool_manager.h"

#include "gimpgeglprocedure.h"

#include "gimp-intl.h"


static void     gimp_gegl_procedure_finalize            (GObject        *object);

static gint64   gimp_gegl_procedure_get_memsize         (GimpObject     *object,
                                                         gint64         *gui_size);

static gchar  * gimp_gegl_procedure_get_description     (GimpViewable   *viewable,
                                                         gchar         **tooltip);

static const gchar * gimp_gegl_procedure_get_menu_label (GimpProcedure  *procedure);
static gboolean      gimp_gegl_procedure_get_sensitive  (GimpProcedure  *procedure,
                                                         GimpObject     *object,
                                                         const gchar   **reason);
static GimpValueArray * gimp_gegl_procedure_execute     (GimpProcedure  *procedure,
                                                         Gimp           *gimp,
                                                         GimpContext    *context,
                                                         GimpProgress   *progress,
                                                         GimpValueArray *args,
                                                         GError        **error);
static void     gimp_gegl_procedure_execute_async       (GimpProcedure  *procedure,
                                                         Gimp           *gimp,
                                                         GimpContext    *context,
                                                         GimpProgress   *progress,
                                                         GimpValueArray *args,
                                                         GimpDisplay    *display);


G_DEFINE_TYPE (GimpGeglProcedure, gimp_gegl_procedure,
               GIMP_TYPE_PROCEDURE)

#define parent_class gimp_gegl_procedure_parent_class


static void
gimp_gegl_procedure_class_init (GimpGeglProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass    *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass  *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpProcedureClass *proc_class        = GIMP_PROCEDURE_CLASS (klass);

  object_class->finalize            = gimp_gegl_procedure_finalize;

  gimp_object_class->get_memsize    = gimp_gegl_procedure_get_memsize;

  viewable_class->default_icon_name = "gimp-gegl";
  viewable_class->get_description   = gimp_gegl_procedure_get_description;

  proc_class->get_menu_label        = gimp_gegl_procedure_get_menu_label;
  proc_class->get_sensitive         = gimp_gegl_procedure_get_sensitive;
  proc_class->execute               = gimp_gegl_procedure_execute;
  proc_class->execute_async         = gimp_gegl_procedure_execute_async;
}

static void
gimp_gegl_procedure_init (GimpGeglProcedure *proc)
{
}

static void
gimp_gegl_procedure_finalize (GObject *object)
{
  GimpGeglProcedure *proc = GIMP_GEGL_PROCEDURE (object);

  g_clear_object (&proc->default_settings);

  g_clear_pointer (&proc->operation,  g_free);
  g_clear_pointer (&proc->menu_label, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_gegl_procedure_get_memsize (GimpObject *object,
                                 gint64     *gui_size)
{
  GimpGeglProcedure *proc    = GIMP_GEGL_PROCEDURE (object);
  gint64             memsize = 0;

  memsize += gimp_string_get_memsize (proc->operation);
  memsize += gimp_string_get_memsize (proc->menu_label);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
gimp_gegl_procedure_get_description (GimpViewable  *viewable,
                                     gchar        **tooltip)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (viewable);

  if (tooltip)
    *tooltip = g_strdup (gimp_procedure_get_blurb (procedure));

  return g_strdup (gimp_procedure_get_label (procedure));
}

static const gchar *
gimp_gegl_procedure_get_menu_label (GimpProcedure *procedure)
{
  GimpGeglProcedure *proc = GIMP_GEGL_PROCEDURE (procedure);

  if (proc->menu_label)
    return proc->menu_label;

  return GIMP_PROCEDURE_CLASS (parent_class)->get_menu_label (procedure);
}

static gboolean
gimp_gegl_procedure_get_sensitive (GimpProcedure  *procedure,
                                   GimpObject     *object,
                                   const gchar   **reason)
{
  GimpImage *image     = GIMP_IMAGE (object);
  GList     *drawables = NULL;
  gboolean   sensitive = FALSE;

  if (image)
    drawables = gimp_image_get_selected_drawables (image);

  if (g_list_length (drawables) == 1)
    {
      GimpDrawable *drawable  = drawables->data;
      GimpItem     *item;

      if (GIMP_IS_LAYER_MASK (drawable))
        item = GIMP_ITEM (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (drawable)));
      else
        item = GIMP_ITEM (drawable);

      sensitive = ! gimp_item_is_content_locked (item, NULL);

      if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
        sensitive = FALSE;
    }

  g_list_free (drawables);

  return sensitive;
}

static GimpValueArray *
gimp_gegl_procedure_execute (GimpProcedure   *procedure,
                             Gimp            *gimp,
                             GimpContext     *context,
                             GimpProgress    *progress,
                             GimpValueArray  *args,
                             GError         **error)
{
  GimpImage  *image;
  gint        n_drawables;
  GObject   **drawables;
  GObject    *config;

  image       = g_value_get_object          (gimp_value_array_index (args, 1));
  n_drawables = g_value_get_int             (gimp_value_array_index (args, 2));
  drawables   = gimp_value_get_object_array (gimp_value_array_index (args, 3));
  config      = g_value_get_object          (gimp_value_array_index (args, 4));

  if (n_drawables == 1)
    {
      GeglNode *node;

      node = gegl_node_new_child (NULL,
                                  "operation",
                                  GIMP_GEGL_PROCEDURE (procedure)->operation,
                                  NULL);

      gimp_drawable_apply_operation_with_config (GIMP_DRAWABLE (drawables[0]),
                                                 progress,
                                                 gimp_procedure_get_label (procedure),
                                                 node, config);

      g_object_unref (node);

      gimp_image_flush (image);

      return gimp_procedure_get_return_values (procedure, TRUE, NULL);
    }

  return gimp_procedure_get_return_values (procedure, FALSE, NULL);
}

static void
gimp_gegl_procedure_execute_async (GimpProcedure  *procedure,
                                   Gimp           *gimp,
                                   GimpContext    *context,
                                   GimpProgress   *progress,
                                   GimpValueArray *args,
                                   GimpDisplay    *display)
{
  GimpGeglProcedure *gegl_procedure = GIMP_GEGL_PROCEDURE (procedure);
  GimpRunMode        run_mode;
  GimpObject        *settings;
  GimpTool          *active_tool;
  const gchar       *tool_name;

  run_mode = g_value_get_enum   (gimp_value_array_index (args, 0));
  settings = g_value_get_object (gimp_value_array_index (args, 4));

  if (! settings &&
      (run_mode != GIMP_RUN_INTERACTIVE ||
       GIMP_GUI_CONFIG (gimp->config)->filter_tool_use_last_settings))
    {
      /*  if we didn't get settings passed, get the last used settings  */

      GType          config_type;
      GimpContainer *container;

      config_type = G_VALUE_TYPE (gimp_value_array_index (args, 4));

      container = gimp_operation_config_get_container (gimp, config_type,
                                                       (GCompareFunc)
                                                       gimp_settings_compare);

      settings = gimp_container_get_child_by_index (container, 0);

      /*  only use the settings if they are automatically created
       *  "last used" values, not if they were saved explicitly and
       *  have a zero timestamp; and if they are not a separator.
       */
      if (settings &&
          (GIMP_SETTINGS (settings)->time == 0 ||
           ! gimp_object_get_name (settings)))
        {
          settings = NULL;
        }
    }

  if (run_mode == GIMP_RUN_NONINTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    {
      if (settings || run_mode == GIMP_RUN_NONINTERACTIVE)
        {
          GimpValueArray *return_vals;

          g_value_set_object (gimp_value_array_index (args, 4), settings);
          return_vals = gimp_procedure_execute (procedure, gimp, context, progress,
                                                args, NULL);
          gimp_value_array_unref (return_vals);
          return;
        }

      gimp_message (gimp,
                    G_OBJECT (progress), GIMP_MESSAGE_WARNING,
                    _("There are no last settings for '%s', "
                      "showing the filter dialog instead."),
                    gimp_procedure_get_label (procedure));
    }

  if (! strcmp (gegl_procedure->operation, "gimp:brightness-contrast"))
    {
      tool_name = "gimp-brightness-contrast-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "gimp:curves"))
    {
      tool_name = "gimp-curves-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "gimp:levels"))
    {
      tool_name = "gimp-levels-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "gimp:threshold"))
    {
      tool_name = "gimp-threshold-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "gimp:offset"))
    {
      tool_name = "gimp-offset-tool";
    }
  else
    {
      tool_name = "gimp-operation-tool";
    }

  active_tool = tool_manager_get_active (gimp);

  /*  do not use the passed context because we need to set the active
   *  tool on the global user context
   */
  context = gimp_get_user_context (gimp);

  if (strcmp (gimp_object_get_name (active_tool->tool_info), tool_name))
    {
      GimpToolInfo *tool_info = gimp_get_tool_info (gimp, tool_name);

      if (GIMP_IS_TOOL_INFO (tool_info))
        gimp_context_set_tool (context, tool_info);
    }
  else
    {
      gimp_context_tool_changed (context);
    }

  active_tool = tool_manager_get_active (gimp);

  if (! strcmp (gimp_object_get_name (active_tool->tool_info), tool_name))
    {
      /*  Remember the procedure that created this tool, because
       *  we can't just switch to an operation tool using
       *  gimp_context_set_tool(), we also have to go through the
       *  initialization code below, otherwise we end up with a
       *  dummy tool that does nothing. See bug #776370.
       */
      g_object_set_data_full (G_OBJECT (active_tool), "gimp-gegl-procedure",
                              g_object_ref (procedure),
                              (GDestroyNotify) g_object_unref);

      if (! strcmp (tool_name, "gimp-operation-tool"))
        {
          gimp_operation_tool_set_operation (GIMP_OPERATION_TOOL (active_tool),
                                             gegl_procedure->filter,
                                             gegl_procedure->operation,
                                             gimp_procedure_get_label (procedure),
                                             gimp_procedure_get_label (procedure),
                                             gimp_procedure_get_label (procedure),
                                             gimp_viewable_get_icon_name (GIMP_VIEWABLE (procedure)),
                                             gimp_procedure_get_help_id (procedure));
        }

      tool_manager_initialize_active (gimp, display);

      /* For GIMP-specific GEGL operations, we need to copy over the
       * config object stored in the GeglNode */
      if (gegl_procedure->filter)
        {
          GeglNode *node;

          GIMP_FILTER_TOOL (active_tool)->existing_filter = gegl_procedure->filter;
          gimp_filter_set_active (GIMP_FILTER (gegl_procedure->filter), FALSE);

          node = gimp_drawable_filter_get_operation (gegl_procedure->filter);

          if (gimp_operation_config_is_custom (gimp, gegl_procedure->operation))
            {
              gegl_node_get (node,
                             "config", &settings,
                             NULL);
            }
          else
            {
              GParamSpec       **pspecs;
              guint              n_pspecs;
              gdouble            opacity;
              GimpLayerMode      paint_mode;
              GimpFilterRegion   region;

              opacity    = gimp_drawable_filter_get_opacity (gegl_procedure->filter);
              paint_mode = gimp_drawable_filter_get_paint_mode (gegl_procedure->filter);
              region     = gimp_drawable_filter_get_region (gegl_procedure->filter);

              settings =
                g_object_new (gimp_operation_config_get_type (active_tool->tool_info->gimp,
                                                              gegl_procedure->operation,
                                                              gimp_tool_get_icon_name (active_tool),
                                                              GIMP_TYPE_OPERATION_SETTINGS),
                              NULL);

              pspecs = gegl_operation_list_properties (gegl_procedure->operation, &n_pspecs);

              for (gint i = 0; i < n_pspecs; i++)
                {
                  GValue      value      = G_VALUE_INIT;
                  GParamSpec *pspec      = pspecs[i];
                  GParamSpec *gimp_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings),
                                                                         pspec->name);

                  g_value_init (&value, pspec->value_type);
                  gegl_node_get_property (node, pspec->name,
                                          &value);

                  if (gimp_pspec)
                    g_object_set_property (G_OBJECT (settings), gimp_pspec->name,
                                           &value);
                  else
                    g_critical ("%s: property '%s' of operation '%s' doesn't exist in config %s",
                                G_STRFUNC, pspec->name, gegl_procedure->operation,
                                g_type_name (G_TYPE_FROM_INSTANCE (settings)));
                  g_value_unset (&value);
                }
              g_free (pspecs);

              g_object_set (settings,
                            "gimp-opacity", opacity,
                            "gimp-mode",    paint_mode,
                            "gimp-region",  region,
                            NULL);
            }
        }

      if (settings)
        gimp_filter_tool_set_config (GIMP_FILTER_TOOL (active_tool),
                                     GIMP_CONFIG (settings));
    }
}


/*  public functions  */

GimpProcedure *
gimp_gegl_procedure_new (Gimp               *gimp,
                         GimpDrawableFilter *filter,
                         GimpRunMode         default_run_mode,
                         GimpObject         *default_settings,
                         const gchar        *operation,
                         const gchar        *name,
                         const gchar        *menu_label,
                         const gchar        *tooltip,
                         const gchar        *icon_name,
                         const gchar        *help_id)
{
  GimpProcedure     *procedure;
  GimpGeglProcedure *gegl_procedure;
  GType              config_type;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (operation != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (menu_label != NULL, NULL);

  config_type = gimp_operation_config_get_type (gimp, operation, icon_name,
                                                GIMP_TYPE_OPERATION_SETTINGS);

  procedure = g_object_new (GIMP_TYPE_GEGL_PROCEDURE, NULL);

  gegl_procedure = GIMP_GEGL_PROCEDURE (procedure);

  gegl_procedure->filter           = filter;
  gegl_procedure->operation        = g_strdup (operation);
  gegl_procedure->default_run_mode = default_run_mode;
  gegl_procedure->menu_label       = g_strdup (menu_label);

  if (default_settings)
    gegl_procedure->default_settings = g_object_ref (default_settings);

  gimp_object_set_name (GIMP_OBJECT (procedure), name);
  gimp_viewable_set_icon_name (GIMP_VIEWABLE (procedure), icon_name);
  gimp_procedure_set_help (procedure,
                           tooltip,
                           tooltip,
                           help_id);
  gimp_procedure_set_static_attribution (procedure,
                                         "author", "copyright", "date");

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_enum ("run-mode",
                                                     "Run mode",
                                                     "Run mode",
                                                     GIMP_TYPE_RUN_MODE,
                                                     GIMP_RUN_INTERACTIVE,
                                                     GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_int ("num-drawables",
                                                 "N drawables",
                                                 "The number of drawables",
                                                 0, G_MAXINT32, 0,
                                                 GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_object_array ("drawables",
                                                             "Drawables",
                                                             "Input drawables",
                                                             GIMP_TYPE_DRAWABLE,
                                                             GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               g_param_spec_object ("settings",
                                                    "Settings",
                                                    "Settings",
                                                    config_type,
                                                    GIMP_PARAM_READWRITE));

  return procedure;
}
