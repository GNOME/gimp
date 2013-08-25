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

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimplist.h"
#include "core/gimpparamspecs-duplicate.h"
#include "core/gimpsettings.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpproptable.h"

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
                0,
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
  GimpOperationTool *op_tool = GIMP_OPERATION_TOOL (tool);

  if (GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      if (op_tool->aux_input_combo)
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (tool->drawable));

          gimp_container_view_set_container (GIMP_CONTAINER_VIEW (op_tool->aux_input_combo),
                                             gimp_image_get_channels (image));
        }

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
      if (op_tool->aux_input_combo)
        gimp_container_view_set_container (GIMP_CONTAINER_VIEW (op_tool->aux_input_combo),
                                           NULL);
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

  return g_object_new (GEGL_TYPE_NODE, NULL);
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

  if (tool->aux_input_combo)
    {
      gtk_box_pack_start (GTK_BOX (tool->options_box), tool->aux_input_combo,
                          FALSE, FALSE, 0);
      gtk_widget_show (tool->aux_input_combo);
    }

  if (tool->options_table)
    {
      gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_table,
                          FALSE, FALSE, 0);
      gtk_widget_show (tool->options_table);
    }

  if (tool->undo_desc)
    gimp_tool_gui_set_description (GIMP_IMAGE_MAP_TOOL (tool)->gui,
                                   tool->undo_desc);
}

static void
gimp_operation_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (image_map_tool);

  if (tool->config)
    gimp_config_reset (GIMP_CONFIG (tool->config));
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
      GimpDrawable *drawable     = GIMP_TOOL (im_tool)->drawable;
      GObjectClass *object_class = G_OBJECT_GET_CLASS (tool->config);
      GParamSpec   *pspec_x;
      GParamSpec   *pspec_y;
      gint          width        = 1;
      gint          height       = 1;

      /* the operation's coordinate system is the selection bounds of
       * the drawable
       */
      if (drawable)
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                        &off_x, &off_y, &width, &height))
            {
              x -= off_x;
              y -= off_y;
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
              GParamSpecDouble *dspec_x = G_PARAM_SPEC_DOUBLE (pspec_x);
              GParamSpecDouble *dspec_y = G_PARAM_SPEC_DOUBLE (pspec_y);

              /* handle what certain well known ops use as extent,
               * this is by no measure proper code...
               */
              if (dspec_x->minimum == 0.0 && dspec_x->maximum == 1.0 &&
                  dspec_y->minimum == 0.0 && dspec_y->maximum == 1.0)
                {
                  x /= width;
                  y /= height;
                }
              else if (dspec_x->minimum == -1.0 && dspec_x->maximum == 2.0 &&
                       dspec_y->minimum == -1.0 && dspec_y->maximum == 2.0)
                {
                  x /= width;
                  y /= height;
                }
              else if (dspec_x->minimum == -1.0 && dspec_x->maximum == 1.0 &&
                       dspec_y->minimum == -1.0 && dspec_y->maximum == 1.0)
                {
                  x = x * 2.0 / width  - 1.0;
                  y = y * 2.0 / height - 1.0;
                }

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

static gboolean
gimp_operation_tool_aux_selected (GimpContainerView  *view,
                                  GimpViewable       *viewable,
                                  gpointer            insert_data,
                                  GeglNode           *aux_input)
{
  GeglBuffer *buffer = NULL;

  if (viewable)
    buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (viewable));

  gegl_node_set (aux_input,
                 "buffer", buffer,
                 NULL);

  return TRUE;
}

void
gimp_operation_tool_set_operation (GimpOperationTool *tool,
                                   const gchar       *operation,
                                   const gchar       *undo_desc)
{
  GimpImageMapTool *im_tool;

  g_return_if_fail (GIMP_IS_OPERATION_TOOL (tool));
  g_return_if_fail (operation != NULL);

  im_tool = GIMP_IMAGE_MAP_TOOL (tool);

  if (tool->operation)
    g_free (tool->operation);

  if (tool->undo_desc)
    g_free (tool->undo_desc);

  tool->operation = g_strdup (operation);
  tool->undo_desc = g_strdup (undo_desc);

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

  if (tool->aux_input_combo)
    {
      gtk_widget_destroy (tool->aux_input_combo);
      tool->aux_input_combo = NULL;
    }

  if (tool->options_table)
    {
      gtk_widget_destroy (tool->options_table);
      tool->options_table = NULL;

      if (im_tool->active_picker)
        {
          im_tool->active_picker = NULL;
          gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
        }
    }

  if (gegl_node_has_pad (im_tool->operation, "aux"))
    {
      GimpImage     *image;
      GimpContext   *context;
      GimpContainer *channels;
      GimpChannel   *channel;

      tool->aux_input = gegl_node_new_child (NULL,
                                             "operation", "gegl:buffer-source",
                                             NULL);

      gegl_node_connect_to (tool->aux_input,    "output",
                            im_tool->operation, "aux");

      image = gimp_item_get_image (GIMP_ITEM (GIMP_TOOL (tool)->drawable));

      context  = GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool));
      channels = gimp_image_get_channels (image);

      tool->aux_input_combo =
        gimp_container_combo_box_new (channels, context,
                                      GIMP_VIEW_SIZE_SMALL, 1);

      if (tool->options_box)
        {
          gtk_box_pack_start (GTK_BOX (tool->options_box), tool->aux_input_combo,
                              FALSE, FALSE, 0);
          gtk_widget_show (tool->aux_input_combo);
        }

      g_signal_connect_object (tool->aux_input_combo, "select-item",
                               G_CALLBACK (gimp_operation_tool_aux_selected),
                               tool->aux_input, 0);

      channel = gimp_image_get_active_channel (image);

      if (! channel)
        channel = GIMP_CHANNEL (gimp_container_get_first_child (channels));

      gimp_container_view_select_item (GIMP_CONTAINER_VIEW (tool->aux_input_combo),
                                       GIMP_VIEWABLE (channel));
    }

  if (tool->config)
    {
      tool->options_table =
        gimp_prop_table_new (G_OBJECT (tool->config),
                             G_TYPE_FROM_INSTANCE (tool->config),
                             GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool)),
                             (GimpCreatePickerFunc) gimp_image_map_tool_add_color_picker,
                             tool);

      if (tool->options_box)
        {
          gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_table,
                              FALSE, FALSE, 0);
          gtk_widget_show (tool->options_table);
        }
    }

  if (undo_desc && im_tool->gui)
    gimp_tool_gui_set_description (im_tool->gui, undo_desc);

  if (GIMP_TOOL (tool)->drawable)
    gimp_image_map_tool_preview (im_tool);
}
