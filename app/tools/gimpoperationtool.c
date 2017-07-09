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

#include "propgui/gimppropgui.h"

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
static void        gimp_operation_tool_options_notify  (GimpTool          *tool,
                                                        GimpToolOptions   *options,
                                                        const GParamSpec  *pspec);

static gchar     * gimp_operation_tool_get_operation   (GimpFilterTool    *filter_tool,
                                                        gchar            **description,
                                                        gboolean          *has_settings);
static void        gimp_operation_tool_dialog          (GimpFilterTool    *filter_tool);
static void        gimp_operation_tool_reset           (GimpFilterTool    *filter_tool);
static void        gimp_operation_tool_set_config      (GimpFilterTool    *filter_tool,
                                                        GimpConfig        *config);
static void        gimp_operation_tool_color_picked    (GimpFilterTool    *filter_tool,
                                                        gpointer           identifier,
                                                        gdouble            x,
                                                        gdouble            y,
                                                        const Babl        *sample_format,
                                                        const GimpRGB     *color);

static void        gimp_operation_tool_halt            (GimpOperationTool *op_tool);
static void        gimp_operation_tool_commit          (GimpOperationTool *op_tool);

static void        gimp_operation_tool_sync_op         (GimpOperationTool *op_tool,
                                                        gboolean           sync_colors);
static void        gimp_operation_tool_create_gui      (GimpOperationTool *tool);
static void        gimp_operation_tool_add_gui         (GimpOperationTool *tool);

static AuxInput *  gimp_operation_tool_aux_input_new   (GimpOperationTool *tool,
                                                        GeglNode          *operation,
                                                        const gchar       *input_pad,
                                                        const gchar       *label);
static void        gimp_operation_tool_aux_input_detach(AuxInput          *input);
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
                GIMP_ICON_GEGL,
                data);
}

static void
gimp_operation_tool_class_init (GimpOperationToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  GimpToolClass       *tool_class        = GIMP_TOOL_CLASS (klass);
  GimpFilterToolClass *filter_tool_class = GIMP_FILTER_TOOL_CLASS (klass);

  object_class->finalize           = gimp_operation_tool_finalize;

  tool_class->initialize           = gimp_operation_tool_initialize;
  tool_class->control              = gimp_operation_tool_control;
  tool_class->options_notify       = gimp_operation_tool_options_notify;

  filter_tool_class->get_operation = gimp_operation_tool_get_operation;
  filter_tool_class->dialog        = gimp_operation_tool_dialog;
  filter_tool_class->reset         = gimp_operation_tool_reset;
  filter_tool_class->set_config    = gimp_operation_tool_set_config;
  filter_tool_class->color_picked  = gimp_operation_tool_color_picked;
}

static void
gimp_operation_tool_init (GimpOperationTool *tool)
{
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

  if (tool->description)
    {
      g_free (tool->description);
      tool->description = NULL;
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

      if (filter_tool->config)
        {
          gimp_operation_tool_sync_op (op_tool, TRUE);

          if (! op_tool->options_gui)
            {
              gimp_operation_tool_create_gui (op_tool);
              gimp_operation_tool_add_gui (op_tool);
            }
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
      gimp_operation_tool_halt (op_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_operation_tool_commit (op_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_operation_tool_options_notify (GimpTool         *tool,
                                    GimpToolOptions  *options,
                                    const GParamSpec *pspec)
{
  GimpOperationTool *op_tool = GIMP_OPERATION_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "region"))
    {
      GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);

      /* when the region changes, do we want the operation's on-canvas
       * controller to move to a new position, or the operation to
       * change its properties to match the on-canvas controller?
       *
       * decided to leave the on-canvas controler where it is and
       * pretend it has changed, so the operation is updated
       * accordingly...
       */
      if (filter_tool->widget)
        g_signal_emit_by_name (filter_tool->widget, "changed");

      if (filter_tool->config && tool->drawable)
        gimp_operation_tool_sync_op (op_tool, FALSE);
    }
}

static gchar *
gimp_operation_tool_get_operation (GimpFilterTool  *filter_tool,
                                   gchar          **description,
                                   gboolean        *has_settings)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);

  *description  = g_strdup (tool->description);
  *has_settings = FALSE;

  return g_strdup (tool->operation);
}

static void
gimp_operation_tool_dialog (GimpFilterTool *filter_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);
  GtkWidget         *main_vbox;

  main_vbox = gimp_filter_tool_dialog_get_vbox (filter_tool);

  /*  The options vbox  */
  tool->options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->options_box,
                      TRUE, TRUE, 0);
  gtk_widget_show (tool->options_box);

  g_object_add_weak_pointer (G_OBJECT (tool->options_box),
                             (gpointer) &tool->options_box);

  if (tool->options_gui)
    gimp_operation_tool_add_gui (tool);
}

static void
gimp_operation_tool_reset (GimpFilterTool *filter_tool)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);

  GIMP_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  if (filter_tool->config && GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, TRUE);
}

static void
gimp_operation_tool_set_config (GimpFilterTool *filter_tool,
                                GimpConfig     *config)
{
  GimpOperationTool *tool = GIMP_OPERATION_TOOL (filter_tool);

  GIMP_FILTER_TOOL_CLASS (parent_class)->set_config (filter_tool, config);

  if (filter_tool->config && GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, FALSE);
}

static void
gimp_operation_tool_color_picked (GimpFilterTool  *filter_tool,
                                  gpointer         identifier,
                                  gdouble          x,
                                  gdouble          y,
                                  const Babl      *sample_format,
                                  const GimpRGB   *color)
{
  gchar **pspecs = g_strsplit (identifier, ":", 2);

  if (pspecs[1])
    {
      GObjectClass  *object_class = G_OBJECT_GET_CLASS (filter_tool->config);
      GParamSpec    *pspec_x;
      GParamSpec    *pspec_y;
      gint           off_x, off_y;
      GeglRectangle  area;

      gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

      x -= off_x + area.x;
      y -= off_y + area.y;

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
              x /= (gdouble) area.width;
              y /= (gdouble) area.height;
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
gimp_operation_tool_halt (GimpOperationTool *op_tool)
{
  /*  don't reset op_tool->operation and op_tool->description so the
   *  tool can be properly restarted by clicking on an image
   */

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) gimp_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;
}

static void
gimp_operation_tool_commit (GimpOperationTool *op_tool)
{
  /*  remove the aux input boxes from the dialog, so they don't get
   *  destroyed when the parent class runs its commit()
   */

  g_list_foreach (op_tool->aux_inputs,
                  (GFunc) gimp_operation_tool_aux_input_detach, NULL);
}

static void
gimp_operation_tool_sync_op (GimpOperationTool *op_tool,
                             gboolean           sync_colors)
{
  GimpFilterTool   *filter_tool = GIMP_FILTER_TOOL (op_tool);
  GimpToolOptions  *options     = GIMP_TOOL_GET_OPTIONS (op_tool);
  GParamSpec      **pspecs;
  guint             n_pspecs;
  gint              off_x, off_y;
  GeglRectangle     area;
  gint              i;

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

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
              g_object_set (filter_tool->config, pspec->name, area.width, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (filter_tool->config, pspec->name, area.height, NULL);
            }
        }
      else if (sync_colors)
        {
          if (HAS_KEY (pspec, "role", "color-primary"))
            {
              GimpRGB color;

              gimp_context_get_foreground (GIMP_CONTEXT (options), &color);
              g_object_set (filter_tool->config, pspec->name, &color, NULL);
            }
          else if (sync_colors && HAS_KEY (pspec, "role", "color-secondary"))
            {
              GimpRGB color;

              gimp_context_get_background (GIMP_CONTEXT (options), &color);
              g_object_set (filter_tool->config, pspec->name, &color, NULL);
            }
        }
    }

  g_free (pspecs);
}

static void
gimp_operation_tool_create_gui (GimpOperationTool *tool)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (tool);
  gint            off_x, off_y;
  GeglRectangle   area;
  gint            aux;

  gimp_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  tool->options_gui =
    gimp_prop_gui_new (G_OBJECT (filter_tool->config),
                       G_TYPE_FROM_INSTANCE (filter_tool->config), 0,
                       &area,
                       GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool)),
                       (GimpCreatePickerFunc) gimp_filter_tool_add_color_picker,
                       (GimpCreateControllerFunc) gimp_filter_tool_add_controller,
                       tool);

  g_object_add_weak_pointer (G_OBJECT (tool->options_gui),
                             (gpointer) &tool->options_gui);

  /*  ugly, see comment in the function  */
  gimp_filter_tool_set_has_settings (filter_tool,
                                     ! GTK_IS_LABEL (tool->options_gui));

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
          AuxInput *input;

          input = gimp_operation_tool_aux_input_new (tool,
                                                     filter_tool->operation, pad,
                                                     label);

          tool->aux_inputs = g_list_append (tool->aux_inputs, input);
        }
      else
        {
          break;
        }
    }
}

static void
gimp_operation_tool_add_gui (GimpOperationTool *tool)
{
  GtkSizeGroup *size_group  = NULL;
  GList        *list;

  for (list = tool->aux_inputs; list; list = g_list_next (list))
    {
      AuxInput  *input = list->data;
      GtkWidget *toggle;

      if (! size_group)
        size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      toggle =
        gimp_buffer_source_box_get_toggle (GIMP_BUFFER_SOURCE_BOX (input->box));

      gtk_size_group_add_widget (size_group, toggle);

      gtk_box_pack_start (GTK_BOX (tool->options_box), input->box,
                          FALSE, FALSE, 0);
      gtk_widget_show (input->box);
    }

  if (size_group)
    g_object_unref (size_group);

  gtk_box_pack_start (GTK_BOX (tool->options_box), tool->options_gui,
                      TRUE, TRUE, 0);
  gtk_widget_show (tool->options_gui);
}


/*  aux input utility functions  */

static void
gimp_operation_tool_aux_input_notify (GimpBufferSourceBox *box,
                                      const GParamSpec    *pspec,
                                      AuxInput            *input)
{
  GimpFilterTool *filter_tool = GIMP_FILTER_TOOL (input->tool);

  /* emit "notify" so GimpFilterTool will update its preview
   *
   * FIXME: this is a bad hack that should go away once GimpImageMap
   * and GimpFilterTool are refactored to be more filter-ish.
   */
  if (filter_tool->config)
    g_signal_emit_by_name (filter_tool->config,
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

  /* make AuxInput owner of the box */
  g_object_ref_sink (input->box);

  g_signal_connect (input->box, "notify::pickable",
                    G_CALLBACK (gimp_operation_tool_aux_input_notify),
                    input);
  g_signal_connect (input->box, "notify::enabled",
                    G_CALLBACK (gimp_operation_tool_aux_input_notify),
                    input);

  return input;
}

static void
gimp_operation_tool_aux_input_detach (AuxInput *input)
{
  GtkWidget *parent = gtk_widget_get_parent (input->box);

  if (parent)
    gtk_container_remove (GTK_CONTAINER (parent), input->box);
}

static void
gimp_operation_tool_aux_input_clear (AuxInput *input)
{
  gimp_operation_tool_aux_input_detach (input);

  g_object_set (input->box,
                "pickable", NULL,
                NULL);
}

static void
gimp_operation_tool_aux_input_free (AuxInput *input)
{
  gimp_operation_tool_aux_input_clear (input);

  g_free (input->pad);
  g_object_unref (input->node);
  g_object_unref (input->box);

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

  g_return_if_fail (GIMP_IS_OPERATION_TOOL (tool));

  filter_tool = GIMP_FILTER_TOOL (tool);

  if (tool->operation)
    g_free (tool->operation);

  if (tool->description)
    g_free (tool->description);

  tool->operation   = g_strdup (operation);
  tool->description = g_strdup (description);

  gimp_tool_set_label     (GIMP_TOOL (tool), title);
  gimp_tool_set_undo_desc (GIMP_TOOL (tool), undo_desc);
  gimp_tool_set_icon_name (GIMP_TOOL (tool), icon_name);
  gimp_tool_set_help_id   (GIMP_TOOL (tool), help_id);

  g_list_free_full (tool->aux_inputs,
                    (GDestroyNotify) gimp_operation_tool_aux_input_free);
  tool->aux_inputs = NULL;

  gimp_filter_tool_set_widget (filter_tool, NULL);

  if (tool->options_gui)
    {
      gimp_filter_tool_disable_color_picking (filter_tool);

      gtk_widget_destroy (tool->options_gui);
    }

  if (! operation)
    return;

  gimp_filter_tool_get_operation (filter_tool);

  if (GIMP_TOOL (tool)->drawable)
    gimp_operation_tool_sync_op (tool, TRUE);

  if (filter_tool->config && GIMP_TOOL (tool)->display)
    {
      gimp_operation_tool_create_gui (tool);

      if (tool->options_box)
        gimp_operation_tool_add_gui (tool);
    }
}
