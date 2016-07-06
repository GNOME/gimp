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
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "gegl/gimp-gegl-config.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"
#include "core/gimpparamspecs-duplicate.h"
#include "core/gimppickable.h"
#include "core/gimpsettings.h"

#include "widgets/gimpbuffersourcebox.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimppickablebutton.h"
#include "widgets/gimppropgui.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimpfilteroptions.h"
#include "gimpoperationtool.h"

#include "gimp-intl.h"


typedef struct _AuxInput AuxInput;

struct _AuxInput
{
  GimpOperationTool *tool;
  gchar             *pad;
  GeglNode          *node;
  GtkWidget         *box;
};


/*  local function prototypes  */

static void        gimp_operation_tool_finalize        (GObject           *object);

static gboolean    gimp_operation_tool_initialize      (GimpTool          *tool,
                                                        GimpDisplay       *display,
                                                        GError           **error);
static void        gimp_operation_tool_control         (GimpTool          *tool,
                                                        GimpToolAction     action,
                                                        GimpDisplay       *display);

static gchar     * gimp_operation_tool_get_operation   (GimpFilterTool    *filter_tool,
                                                        gchar            **title,
                                                        gchar            **description,
                                                        gchar            **undo_desc,
                                                        gchar            **icon_name,
                                                        gchar            **help_id);
static void        gimp_operation_tool_dialog          (GimpFilterTool    *filter_tool);
static void        gimp_operation_tool_reset           (GimpFilterTool    *filter_tool);
static GtkWidget * gimp_operation_tool_get_settings_ui (GimpFilterTool    *filter_tool,
                                                        GimpContainer     *settings,
                                                        GFile             *settings_file,
                                                        const gchar       *import_dialog_title,
                                                        const gchar       *export_dialog_title,
                                                        const gchar       *file_dialog_help_id,
                                                        GFile             *default_folder,
                                                        GtkWidget        **settings_box);
static void        gimp_operation_tool_color_picked    (GimpFilterTool    *filter_tool,
                                                        gpointer           identifier,
                                                        gdouble            x,
                                                        gdouble            y,
                                                        const Babl        *sample_format,
                                                        const GimpRGB     *color);

static void        gimp_operation_tool_sync_op         (GimpOperationTool *op_tool,
                                                        GimpDrawable      *drawable);

static AuxInput *  gimp_operation_tool_aux_input_new   (GimpOperationTool *tool,
                                                        GeglNode          *operation,
                                                        const gchar       *input_pad,
                                                        const gchar       *label);
static void        gimp_operation_tool_aux_input_clear (AuxInput          *input);
static void        gimp_operation_tool_aux_input_free  (AuxInput          *input);


G_DEFINE_TYPE (GimpOperationTool, gimp_operation_tool,
               GIMP_TYPE_FILTER_TOOL)

#define parent_class gimp_operation_tool_parent_class


void
gimp_operation_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_OPERATION_TOOL,
                GIMP_TYPE_FILTER_OPTIONS,
                gimp_color_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
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
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  object_class->finalize             = gimp_operation_tool_finalize;

  tool_class->initialize             = gimp_operation_tool_initialize;
  tool_class->control                = gimp_operation_tool_control;

  filter_tool_class->get_operation   = gimp_operation_tool_get_operation;
  filter_tool_class->dialog          = gimp_operation_tool_dialog;
  filter_tool_class->reset           = gimp_operation_tool_reset;
  filter_tool_class->get_settings_ui = gimp_operation_tool_get_settings_ui;
  filter_tool_class->color_picked    = gimp_operation_tool_color_picked;
}

static void
gimp_operation_tool_init (GimpOperationTool *tool)
{
  GIMP_FILTER_TOOL_GET_CLASS (tool)->settings_name = NULL; /* XXX hack */
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

  if (tool->title)
    {
      g_free (tool->title);
      tool->title = NULL;
    }

  if (tool->description)
    {
      g_free (tool->description);
      tool->description = NULL;
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

  if (tool->help_id)
    {
      g_free (tool->help_id);
      tool->help_id = NULL;
    }

  g_list_free_full (tool->aux_inputs,
                    (GDestroyNotify) gimp_operation_tool_aux_input_free);
  tool->aux_inputs = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_operation_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  if (GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      GimpFilterTool    *filter_tool = GIMP_FILTER_TOOL (tool);
      GimpOperationTool *op_tool     = GIMP_OPERATION_TOOL (tool);
      GimpImage         *image       = gimp_display_get_image (display);
      GimpDrawable      *drawable    = gimp_image_get_active_drawable (image);

      if (filter_tool->config)
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
      g_list_foreach (op_tool->aux_inputs,
                      (GFunc) gimp_operation_tool_aux_input_clear, NULL);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
gimp_operation_tool_get_operation (GimpFilterTool  *filter_tool,
                                   gchar          **title,
                                   gchar          **description,
                                   gchar          **undo_desc,
                                   gchar          **icon_name,
                                   gchar          **help_id)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);

  *title       = g_strdup (tool->title);
  *description = g_strdup (tool->description);
  *undo_desc   = g_strdup (tool->undo_desc);
  *icon_name   = g_strdup (tool->icon_name);
  *help_id     = g_strdup (tool->help_id);

  return g_strdup (tool->operation);
}

static void
gimp_operation_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);
  GtkWidget         *main_vbox;
  GList             *list;

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  /*  The options vbox  */
  tool->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->options_box,
                      TRUE, TRUE, 0);
  gtk_widget_show (tool->options_box);

  for (list = tool->aux_inputs; list; list = g_list_next (list))
    {
      AuxInput *input = list->data;

      gtk_box_pack_start (GTK_BOX (tool->options_box), input->box,
                          FALSE, FALSE, 0);
      gtk_widget_show (input->box);
    }

  if (tool->options_gui)
    {
      gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_gui,
                          TRUE, TRUE, 0);
      gtk_widget_show (tool->options_gui);
    }
}

static void
gimp_operation_tool_reset (GimpFilterTool *filter_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);

  GIMP_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  if (filter_tool->config && GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, GIMP_TOOL (tool)->drawable);
}

static GtkWidget *
gimp_operation_tool_get_settings_ui (GimpFilterTool  *filter_tool,
                                     GimpContainer   *settings,
                                     GFile           *settings_file,
                                     const gchar     *import_dialog_title,
                                     const gchar     *export_dialog_title,
                                     const gchar     *file_dialog_help_id,
                                     GFile           *default_folder,
                                     GtkWidget      **settings_box)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);
  GtkWidget         *widget;
  gchar             *basename;
  GFile             *file;
  gchar             *import_title;
  gchar             *export_title;

  basename = g_strconcat (G_OBJECT_TYPE_NAME (filter_tool->config),
                          ".settings", NULL);
  file = gimp_directory_file ("filters", basename, NULL);
  g_free (basename);

  import_title = g_strdup_printf (_("Import '%s' Settings"), tool->title);
  export_title = g_strdup_printf (_("Export '%s' Settings"), tool->title);

  widget =
    GIMP_FILTER_TOOL_CLASS (parent_class)->get_settings_ui (filter_tool,
                                                            settings,
                                                            file,
                                                            import_title,
                                                            export_title,
                                                            file_dialog_help_id,
                                                            NULL, /* sic */
                                                            settings_box);

  g_free (import_title);
  g_free (export_title);

  g_object_unref (file);

  return widget;
}

static void
gimp_operation_tool_color_picked (GimpFilterTool  *filter_tool,
                                  gpointer         identifier,
                                  gdouble          x,
                                  gdouble          y,
                                  const Babl      *sample_format,
                                  const GimpRGB   *color)
{
  GimpOperationTool  *tool = GIMP_OPERATION_TOOL (filter_tool);
  gchar             **pspecs;

  pspecs = g_strsplit (identifier, ":", 2);

  if (pspecs[1])
    {
      GimpFilterOptions *options      = GIMP_FILTER_TOOL_GET_OPTIONS (tool);
      GimpDrawable      *drawable     = GIMP_TOOL (filter_tool)->drawable;
      GObjectClass      *object_class = G_OBJECT_GET_CLASS (filter_tool->config);
      GParamSpec        *pspec_x;
      GParamSpec        *pspec_y;
      gint               width        = 1;
      gint               height       = 1;

      if (drawable)
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          x -= off_x;
          y -= off_y;

          switch (options->region)
            {
            case GIMP_FILTER_REGION_SELECTION:
              if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                            &off_x, &off_y, &width, &height))
                {
                  x -= off_x;
                  y -= off_y;
                }
              break;

            case GIMP_FILTER_REGION_DRAWABLE:
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

              g_object_set (filter_tool->config,
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

              g_object_set (filter_tool->config,
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
      g_object_set (filter_tool->config,
                    pspecs[0], color,
                    NULL);
    }

  g_strfreev (pspecs);
}

static void
gimp_operation_tool_sync_op (GimpOperationTool *op_tool,
                             GimpDrawable      *drawable)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (op_tool);
  GimpToolOptions  *options     = GIMP_TOOL_GET_OPTIONS (op_tool);
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

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (filter_tool->config),
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
              g_object_set (filter_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-coordinate") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (filter_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "x"))
            {
              g_object_set (filter_tool->config, pspec->name, bounds_width, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (filter_tool->config, pspec->name, bounds_height, NULL);
            }
        }
      else if (HAS_KEY (pspec, "role", "color-primary"))
        {
          GimpRGB color;

          gimp_context_get_foreground (GIMP_CONTEXT (options), &color);
          g_object_set (filter_tool->config, pspec->name, &color, NULL);
        }
      else if (HAS_KEY (pspec, "role", "color-secondary"))
        {
          GimpRGB color;

          gimp_context_get_background (GIMP_CONTEXT (options), &color);
          g_object_set (filter_tool->config, pspec->name, &color, NULL);
        }
    }

  g_free (pspecs);
}


/*  aux input utility functions  */

static void
gimp_operation_tool_aux_input_notify (GimpBufferSourceBox *box,
                                      const GParamSpec    *pspec,
                                      AuxInput            *input)
{
  /* emit "notify" so GimpFilterTool will update its preview
   *
   * FIXME: this is a bad hack that should go away once GimpImageMap
   * and GimpFilterTool are refactored to be more filter-ish.
   */
  g_signal_emit_by_name (GIMP_FILTER_TOOL (input->tool)->config,
                         "notify", NULL);
}

static AuxInput *
gimp_operation_tool_aux_input_new (GimpOperationTool *tool,
                                   GeglNode          *operation,
                                   const gchar       *input_pad,
                                   const gchar       *label)
{
  AuxInput    *input = g_slice_new (AuxInput);
  GimpContext *context;

  input->tool = tool;
  input->pad  = g_strdup (input_pad);
  input->node = gegl_node_new_child (NULL,
                                     "operation", "gegl:buffer-source",
                                     NULL);

  gegl_node_connect_to (input->node, "output",
                        operation,   input_pad);

  context = GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool));

  input->box = gimp_buffer_source_box_new (context, input->node, label);

  g_signal_connect (input->box, "notify::pickable",
                    G_CALLBACK (gimp_operation_tool_aux_input_notify),
                    input);
  g_signal_connect (input->box, "notify::enabled",
                    G_CALLBACK (gimp_operation_tool_aux_input_notify),
                    input);

  return input;
}

static void
gimp_operation_tool_aux_input_clear (AuxInput *input)
{
  g_object_set (input->box,
                "pickable", NULL,
                NULL);
}

static void
gimp_operation_tool_aux_input_free (AuxInput *input)
{
  g_free (input->pad);
  g_object_unref (input->node);
  gtk_widget_destroy (input->box);

  g_slice_free (AuxInput, input);
}


/*  public functions  */

void
gimp_operation_tool_set_operation (GimpOperationTool *tool,
                                   const gchar       *operation,
                                   const gchar       *title,
                                   const gchar       *description,
                                   const gchar       *undo_desc,
                                   const gchar       *icon_name,
                                   const gchar       *help_id)
{
  GimpFilterTool *filter_tool;
  GtkSizeGroup   *size_group = NULL;
  gint            aux;

  g_return_if_fail (GIMP_IS_OPERATION_TOOL (tool));
  g_return_if_fail (operation != NULL);

  filter_tool = GIMP_FILTER_TOOL (tool);

  if (tool->operation)
    g_free (tool->operation);

  if (tool->title)
    g_free (tool->title);

  if (tool->description)
    g_free (tool->description);

  if (tool->undo_desc)
    g_free (tool->undo_desc);

  if (tool->icon_name)
    g_free (tool->icon_name);

  if (tool->help_id)
    g_free (tool->help_id);

  tool->operation   = g_strdup (operation);
  tool->title       = g_strdup (title);
  tool->description = g_strdup (description);
  tool->undo_desc   = g_strdup (undo_desc);
  tool->icon_name   = g_strdup (icon_name);
  tool->help_id     = g_strdup (help_id);

  g_list_free_full (tool->aux_inputs,
                    (GDestroyNotify) gimp_operation_tool_aux_input_free);
  tool->aux_inputs = NULL;

  gimp_filter_tool_get_operation (filter_tool);

  if (undo_desc)
    GIMP_FILTER_TOOL_GET_CLASS (tool)->settings_name = "yes"; /* XXX hack */
  else
    GIMP_FILTER_TOOL_GET_CLASS (tool)->settings_name = NULL; /* XXX hack */

  if (tool->options_gui)
    {
      gtk_widget_destroy (tool->options_gui);
      tool->options_gui = NULL;

      if (filter_tool->active_picker)
        {
          filter_tool->active_picker = NULL;
          gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
        }
    }

  for (aux = 1; ; aux++)
    {
      gchar pad[32];
      gchar label[32];

      if (aux == 1)
        {
          g_snprintf (pad,   sizeof (pad),   "aux");
          /* don't translate "Aux" */
          g_snprintf (label, sizeof (label), _("Aux Input"));
        }
      else
        {
          g_snprintf (pad,   sizeof (pad),   "aux%d", aux);
          /* don't translate "Aux" */
          g_snprintf (label, sizeof (label), _("Aux%d Input"), aux);
        }

      if (gegl_node_has_pad (filter_tool->operation, pad))
        {
          AuxInput  *input;
          GtkWidget *toggle;

          if (! size_group)
            size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

          input = gimp_operation_tool_aux_input_new (tool,
                                                     filter_tool->operation, pad,
                                                     label);

          tool->aux_inputs = g_list_append (tool->aux_inputs, input);

          toggle = gimp_buffer_source_box_get_toggle (GIMP_BUFFER_SOURCE_BOX (input->box));
          gtk_size_group_add_widget (size_group, toggle);

          if (tool->options_box)
            {
              gtk_box_pack_start (GTK_BOX (tool->options_box), input->box,
                                  FALSE, FALSE, 0);
              gtk_widget_show (input->box);
            }
        }
      else
        {
          break;
        }
    }

  if (size_group)
    g_object_unref (size_group);

  if (filter_tool->config)
    {
      GeglRectangle *area = NULL;
      GeglRectangle  tmp  = { 0, };

      if (GIMP_TOOL (tool)->drawable)
        {
          GimpDrawable *drawable = GIMP_TOOL (tool)->drawable;

          tmp.width  = gimp_item_get_width  (GIMP_ITEM (drawable));
          tmp.height = gimp_item_get_height (GIMP_ITEM (drawable));

          area = &tmp;
        }

      tool->options_gui =
        gimp_prop_gui_new (G_OBJECT (filter_tool->config),
                           G_TYPE_FROM_INSTANCE (filter_tool->config), 0,
                           area,
                           GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool)),
                           (GimpCreatePickerFunc) gimp_filter_tool_add_color_picker,
                           tool);

      if (tool->options_box)
        {
          gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_gui,
                              TRUE, TRUE, 0);
          gtk_widget_show (tool->options_gui);
        }
    }

  if (GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, GIMP_TOOL (tool)->drawable);
}
