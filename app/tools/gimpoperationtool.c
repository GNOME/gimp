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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gegl/gimp-gegl-config-proxy.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplist.h"
#include "core/gimpparamspecs-duplicate.h"
#include "core/gimppickable.h"
#include "core/gimpsettings.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppickablebutton.h"
#include "widgets/gimppropgui.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimpcoloroptions.h"
#include "gimpoperationtool.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void        gimp_operation_tool_finalize        (GObject           *object);

static gboolean    gimp_operation_tool_initialize      (GimpTool          *tool,
                                                        GimpDisplay       *display,
                                                        GError           **error);
static void        gimp_operation_tool_control         (GimpTool          *tool,
                                                        GimpToolAction     action,
                                                        GimpDisplay       *display);

static GeglNode  * gimp_operation_tool_get_operation   (GimpImageMapTool  *im_tool,
                                                        GObject          **config,
                                                        gchar            **undo_desc);
static void        gimp_operation_tool_map             (GimpImageMapTool  *im_tool);
static void        gimp_operation_tool_dialog          (GimpImageMapTool  *im_tool);
static void        gimp_operation_tool_reset           (GimpImageMapTool  *im_tool);
static GtkWidget * gimp_operation_tool_get_settings_ui (GimpImageMapTool  *image_map_tool,
                                                        GimpContainer     *settings,
                                                        const gchar       *settings_filename,
                                                        const gchar       *import_dialog_title,
                                                        const gchar       *export_dialog_title,
                                                        const gchar       *file_dialog_help_id,
                                                        const gchar       *default_folder,
                                                        GtkWidget        **settings_box);
static void        gimp_operation_tool_color_picked    (GimpImageMapTool  *im_tool,
                                                        gpointer           identifier,
                                                        gdouble            x,
                                                        gdouble            y,
                                                        const Babl        *sample_format,
                                                        const GimpRGB     *color);

static void        gimp_operation_tool_sync_op         (GimpOperationTool *op_tool,
                                                        GimpDrawable      *drawable);


G_DEFINE_TYPE (GimpOperationTool, gimp_operation_tool,
               GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_operation_tool_parent_class


void
gimp_operation_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_OPERATION_TOOL,
                GIMP_TYPE_COLOR_OPTIONS,
                gimp_color_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-operation-tool",
                _("GEGL Operation"),
                _("Operation Tool: Use an arbitrary GEGL operation"),
                N_("_GEGL Operation..."), NULL,
                NULL, GIMP_HELP_TOOL_GEGL,
                GIMP_STOCK_GEGL,
                data);
}

static void
gimp_operation_tool_class_init (GimpOperationToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize         = gimp_operation_tool_finalize;

  tool_class->initialize         = gimp_operation_tool_initialize;
  tool_class->control            = gimp_operation_tool_control;

  im_tool_class->dialog_desc     = _("GEGL Operation");

  im_tool_class->get_operation   = gimp_operation_tool_get_operation;
  im_tool_class->map             = gimp_operation_tool_map;
  im_tool_class->dialog          = gimp_operation_tool_dialog;
  im_tool_class->reset           = gimp_operation_tool_reset;
  im_tool_class->get_settings_ui = gimp_operation_tool_get_settings_ui;
  im_tool_class->color_picked    = gimp_operation_tool_color_picked;
}

static void
gimp_operation_tool_init (GimpOperationTool *tool)
{
  GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->settings_name = NULL; /* XXX hack */
}

static void
gimp_operation_tool_finalize (GObject *object)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (object);

  if (tool->operation)
    {
      g_free (tool->operation);
      tool->operation = NULL;
    }

  if (tool->undo_desc)
    {
      g_free (tool->undo_desc);
      tool->undo_desc = NULL;
    }

  if (tool->icon_name)
    {
      g_free (tool->icon_name);
      tool->icon_name = NULL;
    }

  if (tool->config)
    {
      g_object_unref (tool->config);
      tool->config = NULL;
    }

  if (tool->aux_input)
    {
      g_object_unref (tool->aux_input);
      tool->aux_input = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_operation_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  if (GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      GimpOperationTool *op_tool  = GIMP_OPERATION_TOOL (tool);
      GimpImage         *image    = gimp_display_get_image (display);
      GimpDrawable      *drawable = gimp_image_get_active_drawable (image);

      if (op_tool->config)
        gimp_operation_tool_sync_op (op_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

static void
gimp_operation_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpOperationTool *op_tool = GIMP_OPERATION_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      if (op_tool->aux_input_button)
        gimp_pickable_button_set_pickable (GIMP_PICKABLE_BUTTON (op_tool->aux_input_button),
                                           NULL);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static GeglNode *
gimp_operation_tool_get_operation (GimpImageMapTool  *im_tool,
                                   GObject          **config,
                                   gchar            **undo_desc)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (im_tool);

  if (tool->config)
    *config = g_object_ref (tool->config);

  if (tool->undo_desc)
    *undo_desc = g_strdup (tool->undo_desc);

  if (tool->operation)
    return gegl_node_new_child (NULL,
                                "operation", tool->operation,
                                NULL);

  return gegl_node_new_child (NULL,
                              "operation", "gegl:nop",
                              NULL);
}

static void
gimp_operation_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (image_map_tool);

  if (tool->config)
    gimp_gegl_config_proxy_sync (tool->config, image_map_tool->operation);
}

static void
gimp_operation_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (image_map_tool);
  GtkWidget         *main_vbox;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  /*  The options vbox  */
  tool->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->options_box,
                      FALSE, FALSE, 0);
  gtk_widget_show (tool->options_box);

  if (tool->aux_input_box)
    {
      gtk_box_pack_start (GTK_BOX (tool->options_box), tool->aux_input_box,
                          FALSE, FALSE, 0);
      gtk_widget_show (tool->aux_input_box);
    }

  if (tool->options_gui)
    {
      gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_gui,
                          FALSE, FALSE, 0);
      gtk_widget_show (tool->options_gui);
    }

  if (tool->undo_desc)
    gimp_tool_gui_set_description (GIMP_IMAGE_MAP_TOOL (tool)->gui,
                                   tool->undo_desc);

  if (tool->icon_name)
    gimp_tool_gui_set_icon_name (GIMP_IMAGE_MAP_TOOL (tool)->gui,
                                 tool->icon_name);
}

static void
gimp_operation_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (image_map_tool);

  GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->reset (image_map_tool);

  if (tool->config && GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, GIMP_TOOL (tool)->drawable);
}

static GtkWidget *
gimp_operation_tool_get_settings_ui (GimpImageMapTool  *image_map_tool,
                                     GimpContainer     *settings,
                                     const gchar       *settings_filename,
                                     const gchar       *import_dialog_title,
                                     const gchar       *export_dialog_title,
                                     const gchar       *file_dialog_help_id,
                                     const gchar       *default_folder,
                                     GtkWidget        **settings_box)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (image_map_tool);
  GType              type = G_TYPE_FROM_INSTANCE (tool->config);
  GtkWidget         *widget;
  gchar             *basename;
  gchar             *filename;
  gchar             *import_title;
  gchar             *export_title;

  settings = gimp_gegl_get_config_container (type);
  if (! gimp_list_get_sort_func (GIMP_LIST (settings)))
    gimp_list_set_sort_func (GIMP_LIST (settings),
                             (GCompareFunc) gimp_settings_compare);

  basename = g_strconcat (G_OBJECT_TYPE_NAME (tool->config), ".settings", NULL);
  filename = g_build_filename (gimp_directory (), "filters", basename, NULL);
  g_free (basename);

  import_title = g_strdup_printf (_("Import '%s' Settings"), tool->undo_desc);
  export_title = g_strdup_printf (_("Export '%s' Settings"), tool->undo_desc);

  widget =
    GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->get_settings_ui (image_map_tool,
                                                               settings,
                                                               filename,
                                                               import_title,
                                                               export_title,
                                                               "help-foo",
                                                               g_get_home_dir (),
                                                               settings_box);

  g_free (filename);
  g_free (import_title);
  g_free (export_title);

  return widget;
}

static void
gimp_operation_tool_color_picked (GimpImageMapTool  *im_tool,
                                  gpointer           identifier,
                                  gdouble            x,
                                  gdouble            y,
                                  const Babl        *sample_format,
                                  const GimpRGB     *color)
{
  GimpOperationTool  *tool = GIMP_OPERATION_TOOL (im_tool);
  gchar             **pspecs;

  pspecs = g_strsplit (identifier, ":", 2);

  if (pspecs[1])
    {
      GimpImageMapOptions *options      = GIMP_IMAGE_MAP_TOOL_GET_OPTIONS (tool);
      GimpDrawable        *drawable     = GIMP_TOOL (im_tool)->drawable;
      GObjectClass        *object_class = G_OBJECT_GET_CLASS (tool->config);
      GParamSpec          *pspec_x;
      GParamSpec          *pspec_y;
      gint                 width        = 1;
      gint                 height       = 1;

      if (drawable)
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          switch (options->region)
            {
            case GIMP_IMAGE_MAP_REGION_SELECTION:
              if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                            &off_x, &off_y, &width, &height))
                {
                  x -= off_x;
                  y -= off_y;
                }
              break;

            case GIMP_IMAGE_MAP_REGION_DRAWABLE:
              width  = gimp_item_get_width  (GIMP_ITEM (drawable));
              height = gimp_item_get_height (GIMP_ITEM (drawable));
              break;
            }
        }

      pspec_x = g_object_class_find_property (object_class, pspecs[0]);
      pspec_y = g_object_class_find_property (object_class, pspecs[1]);

      if (pspec_x && pspec_y &&
          G_PARAM_SPEC_TYPE (pspec_x) == G_PARAM_SPEC_TYPE (pspec_y))
        {
          GValue value_x = G_VALUE_INIT;
          GValue value_y = G_VALUE_INIT;

          g_value_init (&value_x, G_PARAM_SPEC_VALUE_TYPE (pspec_x));
          g_value_init (&value_y, G_PARAM_SPEC_VALUE_TYPE (pspec_y));

#define HAS_KEY(p,k,v) gimp_gegl_param_spec_has_key (p, k, v)

          if (HAS_KEY (pspec_x, "unit", "relative-coordinate") &&
              HAS_KEY (pspec_y, "unit", "relative-coordinate"))
            {
              x /= (gdouble) width;
              y /= (gdouble) height;
            }

          if (G_IS_PARAM_SPEC_INT (pspec_x))
            {
              g_value_set_int (&value_x, x);
              g_value_set_int (&value_y, y);

              g_param_value_validate (pspec_x, &value_x);
              g_param_value_validate (pspec_y, &value_y);

              g_object_set (tool->config,
                            pspecs[0], g_value_get_int (&value_x),
                            pspecs[1], g_value_get_int (&value_y),
                            NULL);
            }
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec_x))
            {
              g_value_set_double (&value_x, x);
              g_value_set_double (&value_y, y);

              g_param_value_validate (pspec_x, &value_x);
              g_param_value_validate (pspec_y, &value_y);

              g_object_set (tool->config,
                            pspecs[0], g_value_get_double (&value_x),
                            pspecs[1], g_value_get_double (&value_y),
                            NULL);
            }
          else
            {
              g_warning ("%s: unhandled param spec of type %s",
                         G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (pspec_x));
            }

          g_value_unset (&value_x);
          g_value_unset (&value_y);
        }
    }
  else
    {
      g_object_set (tool->config,
                    pspecs[0], color,
                    NULL);
    }

  g_strfreev (pspecs);
}

static void
gimp_operation_tool_sync_op (GimpOperationTool *op_tool,
                             GimpDrawable      *drawable)
{
  GimpToolOptions  *options = GIMP_TOOL_GET_OPTIONS (op_tool);
  GParamSpec      **pspecs;
  guint             n_pspecs;
  gint              bounds_x;
  gint              bounds_y;
  gint              bounds_width;
  gint              bounds_height;
  gint              i;

  gimp_item_mask_intersect (GIMP_ITEM (drawable),
                            &bounds_x, &bounds_y,
                            &bounds_width, &bounds_height);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (op_tool->config),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

#define HAS_KEY(p,k,v) gimp_gegl_param_spec_has_key (p, k, v)

      if (HAS_KEY (pspec, "role", "output-extent"))
        {
          if (HAS_KEY (pspec, "unit", "pixel-coordinate") &&
              HAS_KEY (pspec, "axis", "x"))
            {
              g_object_set (op_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-coordinate") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (op_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "x"))
            {
              g_object_set (op_tool->config, pspec->name, bounds_width, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (op_tool->config, pspec->name, bounds_height, NULL);
            }
        }
      else if (HAS_KEY (pspec, "role", "color-primary"))
        {
          GimpRGB color;

          gimp_context_get_foreground (GIMP_CONTEXT (options), &color);
          g_object_set (op_tool->config, pspec->name, &color, NULL);
        }
      else if (HAS_KEY (pspec, "role", "color-secondary"))
        {
          GimpRGB color;

          gimp_context_get_background (GIMP_CONTEXT (options), &color);
          g_object_set (op_tool->config, pspec->name, &color, NULL);
        }
    }

  g_free (pspecs);
}

static gboolean
gimp_operation_tool_aux_notify (GimpPickableButton *button,
                                const GParamSpec   *pspec,
                                GeglNode           *aux_input)
{
  GimpPickable *pickable = gimp_pickable_button_get_pickable (button);
  GeglBuffer   *buffer   = NULL;

  if (pickable)
    buffer = gimp_pickable_get_buffer (pickable);

  gegl_node_set (aux_input,
                 "buffer", buffer,
                 NULL);

  return TRUE;
}

void
gimp_operation_tool_set_operation (GimpOperationTool *tool,
                                   const gchar       *operation,
                                   const gchar       *undo_desc,
                                   const gchar       *icon_name)
{
  GimpImageMapTool *im_tool;

  g_return_if_fail (GIMP_IS_OPERATION_TOOL (tool));
  g_return_if_fail (operation != NULL);

  im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (tool->operation)
    g_free (tool->operation);

  if (tool->undo_desc)
    g_free (tool->undo_desc);

  if (tool->icon_name)
    g_free (tool->icon_name);

  tool->operation = g_strdup (operation);
  tool->undo_desc = g_strdup (undo_desc);
  tool->icon_name = g_strdup (icon_name);

  if (tool->config)
    g_object_unref (tool->config);

  tool->config = gimp_gegl_get_config_proxy (tool->operation,
                                             GIMP_TYPE_SETTINGS);

  if (tool->aux_input)
    {
      g_object_unref (tool->aux_input);
      tool->aux_input = NULL;
    }

  gimp_image_map_tool_get_operation (im_tool);

  if (undo_desc)
    GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->settings_name = "yes"; /* XXX hack */
  else
    GIMP_IMAGE_MAP_TOOL_GET_CLASS (tool)->settings_name = NULL; /* XXX hack */

  if (tool->aux_input_box)
    {
      gtk_widget_destroy (tool->aux_input_box);
      tool->aux_input_button = NULL;
      tool->aux_input_box    = NULL;
    }

  if (tool->options_gui)
    {
      gtk_widget_destroy (tool->options_gui);
      tool->options_gui = NULL;

      if (im_tool->active_picker)
        {
          im_tool->active_picker = NULL;
          gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
        }
    }

  if (gegl_node_has_pad (im_tool->operation, "aux"))
    {
      GimpContext *context;
      GtkWidget   *label;

      tool->aux_input = gegl_node_new_child (NULL,
                                             "operation", "gegl:buffer-source",
                                             NULL);

      gegl_node_connect_to (tool->aux_input,    "output",
                            im_tool->operation, "aux");

      context = GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool));

      tool->aux_input_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

      label = gtk_label_new_with_mnemonic (_("_Aux Input"));
      gtk_box_pack_start (GTK_BOX (tool->aux_input_box), label,
                          FALSE, FALSE, 0);
      gtk_widget_show (label);

      tool->aux_input_button =
        gimp_pickable_button_new (context, GIMP_VIEW_SIZE_LARGE, 1);
      gtk_box_pack_start (GTK_BOX (tool->aux_input_box),
                          tool->aux_input_button, FALSE, FALSE, 0);
      gtk_widget_show (tool->aux_input_button);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                     tool->aux_input_button);

      if (tool->options_box)
        {
          gtk_box_pack_start (GTK_BOX (tool->options_box), tool->aux_input_box,
                              FALSE, FALSE, 0);
          gtk_widget_show (tool->aux_input_box);
        }

      g_signal_connect_object (tool->aux_input_button, "notify::pickable",
                               G_CALLBACK (gimp_operation_tool_aux_notify),
                               tool->aux_input, 0);
    }

  if (tool->config)
    {
      tool->options_gui =
        gimp_prop_gui_new (G_OBJECT (tool->config),
                           G_TYPE_FROM_INSTANCE (tool->config),
                           GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool)),
                           (GimpCreatePickerFunc) gimp_image_map_tool_add_color_picker,
                           tool);

      if (tool->options_box)
        {
          gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_gui,
                              FALSE, FALSE, 0);
          gtk_widget_show (tool->options_gui);
        }
    }

  if (im_tool->gui)
    {
      if (undo_desc)
        gimp_tool_gui_set_description (im_tool->gui, undo_desc);

      if (icon_name)
        gimp_tool_gui_set_icon_name (im_tool->gui, icon_name);
    }

  if (GIMP_TOOL (tool)->drawable)
    {
      gimp_operation_tool_sync_op (tool, GIMP_TOOL (tool)->drawable);
      gimp_image_map_tool_preview (im_tool);
    }
}
