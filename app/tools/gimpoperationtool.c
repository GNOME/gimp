/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligmachannel.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmalist.h"
#include "core/ligmapickable.h"
#include "core/ligmasettings.h"

#include "widgets/ligmabuffersourcebox.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmapickablebutton.h"

#include "propgui/ligmapropgui.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolgui.h"

#include "ligmafilteroptions.h"
#include "ligmaoperationtool.h"

#include "ligma-intl.h"


typedef struct _AuxInput AuxInput;

struct _AuxInput
{
  LigmaOperationTool *tool;
  gchar             *pad;
  GeglNode          *node;
  GtkWidget         *box;
};


/*  local function prototypes  */

static void        ligma_operation_tool_finalize        (GObject           *object);

static gboolean    ligma_operation_tool_initialize      (LigmaTool          *tool,
                                                        LigmaDisplay       *display,
                                                        GError           **error);
static void        ligma_operation_tool_control         (LigmaTool          *tool,
                                                        LigmaToolAction     action,
                                                        LigmaDisplay       *display);

static gchar     * ligma_operation_tool_get_operation   (LigmaFilterTool    *filter_tool,
                                                        gchar            **description);
static void        ligma_operation_tool_dialog          (LigmaFilterTool    *filter_tool);
static void        ligma_operation_tool_reset           (LigmaFilterTool    *filter_tool);
static void        ligma_operation_tool_set_config      (LigmaFilterTool    *filter_tool,
                                                        LigmaConfig        *config);
static void        ligma_operation_tool_region_changed  (LigmaFilterTool    *filter_tool);
static void        ligma_operation_tool_color_picked    (LigmaFilterTool    *filter_tool,
                                                        gpointer           identifier,
                                                        gdouble            x,
                                                        gdouble            y,
                                                        const Babl        *sample_format,
                                                        const LigmaRGB     *color);

static void        ligma_operation_tool_halt            (LigmaOperationTool *op_tool);
static void        ligma_operation_tool_commit          (LigmaOperationTool *op_tool);

static void        ligma_operation_tool_sync_op         (LigmaOperationTool *op_tool,
                                                        gboolean           sync_colors);
static void        ligma_operation_tool_create_gui      (LigmaOperationTool *tool);
static void        ligma_operation_tool_add_gui         (LigmaOperationTool *tool);

static AuxInput *  ligma_operation_tool_aux_input_new   (LigmaOperationTool *tool,
                                                        GeglNode          *operation,
                                                        const gchar       *input_pad,
                                                        const gchar       *label);
static void        ligma_operation_tool_aux_input_detach(AuxInput          *input);
static void        ligma_operation_tool_aux_input_clear (AuxInput          *input);
static void        ligma_operation_tool_aux_input_free  (AuxInput          *input);

static void        ligma_operation_tool_unlink_chains   (LigmaOperationTool *op_tool);
static void        ligma_operation_tool_relink_chains   (LigmaOperationTool *op_tool);


G_DEFINE_TYPE (LigmaOperationTool, ligma_operation_tool,
               LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_operation_tool_parent_class


void
ligma_operation_tool_register (LigmaToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (LIGMA_TYPE_OPERATION_TOOL,
                LIGMA_TYPE_FILTER_OPTIONS,
                ligma_color_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND,
                "ligma-operation-tool",
                _("GEGL Operation"),
                _("Operation Tool: Use an arbitrary GEGL operation"),
                NULL, NULL,
                NULL, LIGMA_HELP_TOOL_GEGL,
                LIGMA_ICON_GEGL,
                data);
}

static void
ligma_operation_tool_class_init (LigmaOperationToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  object_class->finalize            = ligma_operation_tool_finalize;

  tool_class->initialize            = ligma_operation_tool_initialize;
  tool_class->control               = ligma_operation_tool_control;

  filter_tool_class->get_operation  = ligma_operation_tool_get_operation;
  filter_tool_class->dialog         = ligma_operation_tool_dialog;
  filter_tool_class->reset          = ligma_operation_tool_reset;
  filter_tool_class->set_config     = ligma_operation_tool_set_config;
  filter_tool_class->region_changed = ligma_operation_tool_region_changed;
  filter_tool_class->color_picked   = ligma_operation_tool_color_picked;
}

static void
ligma_operation_tool_init (LigmaOperationTool *op_tool)
{
}

static void
ligma_operation_tool_finalize (GObject *object)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (object);

  g_clear_pointer (&op_tool->operation,   g_free);
  g_clear_pointer (&op_tool->description, g_free);

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) ligma_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_operation_tool_initialize (LigmaTool     *tool,
                                LigmaDisplay  *display,
                                GError      **error)
{
  if (LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      LigmaFilterTool    *filter_tool = LIGMA_FILTER_TOOL (tool);
      LigmaOperationTool *op_tool     = LIGMA_OPERATION_TOOL (tool);

      if (filter_tool->config)
        {
          GtkWidget *options_gui;

          ligma_operation_tool_sync_op (op_tool, TRUE);
          options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
          if (! options_gui)
            {
              ligma_operation_tool_create_gui (op_tool);
              ligma_operation_tool_add_gui (op_tool);
            }
          else
            {
              g_object_unref (options_gui);
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
ligma_operation_tool_control (LigmaTool       *tool,
                             LigmaToolAction  action,
                             LigmaDisplay    *display)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_operation_tool_halt (op_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_operation_tool_commit (op_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
ligma_operation_tool_get_operation (LigmaFilterTool  *filter_tool,
                                   gchar          **description)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (filter_tool);

  *description = g_strdup (op_tool->description);

  return g_strdup (op_tool->operation);
}

static void
ligma_operation_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (filter_tool);
  GtkWidget         *main_vbox;
  GtkWidget         *options_sw;
  GtkWidget         *options_gui;
  GtkWidget         *options_box;

  main_vbox = ligma_filter_tool_dialog_get_vbox (filter_tool);

  /*  The options scrolled window  */
  options_sw = gtk_scrolled_window_new (NULL, NULL);
  g_weak_ref_set (&op_tool->options_sw_ref, options_sw);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (options_sw),
                                             FALSE);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (options_sw),
                                       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (options_sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_box_pack_start (GTK_BOX (main_vbox), options_sw,
                      TRUE, TRUE, 0);
  gtk_widget_show (options_sw);

  /*  The options vbox  */
  options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  g_weak_ref_set (&op_tool->options_box_ref, options_box);
  gtk_container_add (GTK_CONTAINER (options_sw), options_box);
  gtk_widget_show (options_box);

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  if (options_gui)
    {
      ligma_operation_tool_add_gui (op_tool);
      g_object_unref (options_gui);
    }
}

static void
ligma_operation_tool_reset (LigmaFilterTool *filter_tool)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (filter_tool);

  ligma_operation_tool_unlink_chains (op_tool);

  LIGMA_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  if (filter_tool->config && LIGMA_TOOL (op_tool)->drawables)
    ligma_operation_tool_sync_op (op_tool, TRUE);

  ligma_operation_tool_relink_chains (op_tool);
}

static void
ligma_operation_tool_set_config (LigmaFilterTool *filter_tool,
                                LigmaConfig     *config)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (filter_tool);

  ligma_operation_tool_unlink_chains (op_tool);

  LIGMA_FILTER_TOOL_CLASS (parent_class)->set_config (filter_tool, config);

  if (filter_tool->config && LIGMA_TOOL (op_tool)->drawables)
    ligma_operation_tool_sync_op (op_tool, FALSE);

  ligma_operation_tool_relink_chains (op_tool);
}

static void
ligma_operation_tool_region_changed (LigmaFilterTool *filter_tool)
{
  LigmaOperationTool *op_tool = LIGMA_OPERATION_TOOL (filter_tool);

  /* when the region changes, do we want the operation's on-canvas
   * controller to move to a new position, or the operation to
   * change its properties to match the on-canvas controller?
   *
   * decided to leave the on-canvas controller where it is and
   * pretend it has changed, so the operation is updated
   * accordingly...
   */
  if (filter_tool->widget)
    g_signal_emit_by_name (filter_tool->widget, "changed");

  ligma_operation_tool_sync_op (op_tool, FALSE);
}

static void
ligma_operation_tool_color_picked (LigmaFilterTool  *filter_tool,
                                  gpointer         identifier,
                                  gdouble          x,
                                  gdouble          y,
                                  const Babl      *sample_format,
                                  const LigmaRGB   *color)
{
  gchar **pspecs = g_strsplit (identifier, ":", 2);

  if (pspecs[1])
    {
      GObjectClass  *object_class = G_OBJECT_GET_CLASS (filter_tool->config);
      GParamSpec    *pspec_x;
      GParamSpec    *pspec_y;
      gint           off_x, off_y;
      GeglRectangle  area;

      ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

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

#define HAS_KEY(p,k,v) ligma_gegl_param_spec_has_key (p, k, v)

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
ligma_operation_tool_halt (LigmaOperationTool *op_tool)
{
  /*  don't reset op_tool->operation and op_tool->description so the
   *  tool can be properly restarted by clicking on an image
   */

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) ligma_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;
}

static void
ligma_operation_tool_commit (LigmaOperationTool *op_tool)
{
  /*  remove the aux input boxes from the dialog, so they don't get
   *  destroyed when the parent class runs its commit()
   */

  g_list_foreach (op_tool->aux_inputs,
                  (GFunc) ligma_operation_tool_aux_input_detach, NULL);
}

static void
ligma_operation_tool_sync_op (LigmaOperationTool *op_tool,
                             gboolean           sync_colors)
{
  LigmaFilterTool   *filter_tool = LIGMA_FILTER_TOOL (op_tool);
  LigmaToolOptions  *options     = LIGMA_TOOL_GET_OPTIONS (op_tool);
  GParamSpec      **pspecs;
  guint             n_pspecs;
  gint              off_x, off_y;
  GeglRectangle     area;
  gint              i;

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (filter_tool->config),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

#define HAS_KEY(p,k,v) ligma_gegl_param_spec_has_key (p, k, v)

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
              LigmaRGB color;

              ligma_context_get_foreground (LIGMA_CONTEXT (options), &color);
              g_object_set (filter_tool->config, pspec->name, &color, NULL);
            }
          else if (sync_colors && HAS_KEY (pspec, "role", "color-secondary"))
            {
              LigmaRGB color;

              ligma_context_get_background (LIGMA_CONTEXT (options), &color);
              g_object_set (filter_tool->config, pspec->name, &color, NULL);
            }
        }
    }

  g_free (pspecs);
}

static void
ligma_operation_tool_create_gui (LigmaOperationTool *op_tool)
{
  LigmaFilterTool  *filter_tool = LIGMA_FILTER_TOOL (op_tool);
  GtkWidget       *options_gui;
  gint             off_x, off_y;
  GeglRectangle    area;
  gchar          **input_pads;

  ligma_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  options_gui =
    ligma_prop_gui_new (G_OBJECT (filter_tool->config),
                       G_TYPE_FROM_INSTANCE (filter_tool->config), 0,
                       &area,
                       LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (op_tool)),
                       (LigmaCreatePickerFunc) ligma_filter_tool_add_color_picker,
                       (LigmaCreateControllerFunc) ligma_filter_tool_add_controller,
                       filter_tool);
  g_weak_ref_set (&op_tool->options_gui_ref, options_gui);

  input_pads = gegl_node_list_input_pads (filter_tool->operation);

  if (input_pads)
    {
      gint i;

      for (i = 0; input_pads[i]; i++)
        {
          AuxInput *input;
          GRegex   *regex;
          gchar    *label;

          if (! strcmp (input_pads[i], "input"))
            continue;

          regex = g_regex_new ("^aux(\\d*)$", 0, 0, NULL);

          g_return_if_fail (regex != NULL);

          /* Translators: don't translate "Aux" */
          label = g_regex_replace (regex,
                                   input_pads[i], -1, 0,
                                   _("Aux\\1 Input"),
                                   0, NULL);

          input = ligma_operation_tool_aux_input_new (op_tool,
                                                     filter_tool->operation,
                                                     input_pads[i], label);

          op_tool->aux_inputs = g_list_prepend (op_tool->aux_inputs, input);

          g_free (label);

          g_regex_unref (regex);
        }

      g_strfreev (input_pads);
    }
}

static void
ligma_operation_tool_add_gui (LigmaOperationTool *op_tool)
{
  GtkSizeGroup   *size_group  = NULL;
  GtkWidget      *options_gui;
  GtkWidget      *options_box;
  GtkWidget      *options_sw;
  GtkWidget      *shell;
  GdkRectangle    workarea;
  GtkRequisition  minimum;
  GList          *list;
  gboolean        scrolling;

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  options_box = g_weak_ref_get (&op_tool->options_box_ref);
  options_sw  = g_weak_ref_get (&op_tool->options_sw_ref);
  g_return_if_fail (options_gui && options_box && options_sw);

  for (list = op_tool->aux_inputs; list; list = g_list_next (list))
    {
      AuxInput  *input = list->data;
      GtkWidget *toggle;

      if (! size_group)
        size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      toggle =
        ligma_buffer_source_box_get_toggle (LIGMA_BUFFER_SOURCE_BOX (input->box));

      gtk_size_group_add_widget (size_group, toggle);

      gtk_box_pack_start (GTK_BOX (options_box), input->box,
                          FALSE, FALSE, 0);
      gtk_widget_show (input->box);
    }

  if (size_group)
    g_object_unref (size_group);

  gtk_box_pack_start (GTK_BOX (options_box), options_gui, TRUE, TRUE, 0);
  gtk_widget_show (options_gui);

  shell = GTK_WIDGET (ligma_display_get_shell (LIGMA_TOOL (op_tool)->display));
  gdk_monitor_get_workarea (ligma_widget_get_monitor (shell), &workarea);
  gtk_widget_get_preferred_size (options_box, &minimum, NULL);

  scrolling = minimum.height > workarea.height / 2;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (options_sw),
                                  GTK_POLICY_NEVER,
                                  scrolling ?
                                  GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER);

  if (scrolling)
    gtk_widget_set_size_request (options_sw, -1, workarea.height / 2);
  else
    gtk_widget_set_size_request (options_sw, -1, -1);

  g_object_unref (options_gui);
  g_object_unref (options_box);
  g_object_unref (options_sw);
}


/*  aux input utility functions  */

static void
ligma_operation_tool_aux_input_notify (LigmaBufferSourceBox *box,
                                      const GParamSpec    *pspec,
                                      AuxInput            *input)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (input->tool);

  /* emit "notify" so LigmaFilterTool will update its preview
   *
   * FIXME: this is a bad hack that should go away once LigmaImageMap
   * and LigmaFilterTool are refactored to be more filter-ish.
   */
  if (filter_tool->config)
    g_signal_emit_by_name (filter_tool->config,
                           "notify", NULL);
}

static AuxInput *
ligma_operation_tool_aux_input_new (LigmaOperationTool *op_tool,
                                   GeglNode          *operation,
                                   const gchar       *input_pad,
                                   const gchar       *label)
{
  AuxInput    *input = g_slice_new (AuxInput);
  LigmaContext *context;

  input->tool = op_tool;
  input->pad  = g_strdup (input_pad);
  input->node = gegl_node_new_child (NULL,
                                     "operation", "gegl:buffer-source",
                                     NULL);

  gegl_node_connect_to (input->node, "output",
                        operation,   input_pad);

  context = LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (op_tool));

  input->box = ligma_buffer_source_box_new (context, input->node, label);

  /* make AuxInput owner of the box */
  g_object_ref_sink (input->box);

  g_signal_connect (input->box, "notify::pickable",
                    G_CALLBACK (ligma_operation_tool_aux_input_notify),
                    input);
  g_signal_connect (input->box, "notify::enabled",
                    G_CALLBACK (ligma_operation_tool_aux_input_notify),
                    input);

  return input;
}

static void
ligma_operation_tool_aux_input_detach (AuxInput *input)
{
  GtkWidget *parent = gtk_widget_get_parent (input->box);

  if (parent)
    gtk_container_remove (GTK_CONTAINER (parent), input->box);
}

static void
ligma_operation_tool_aux_input_clear (AuxInput *input)
{
  ligma_operation_tool_aux_input_detach (input);

  g_object_set (input->box,
                "pickable", NULL,
                NULL);
}

static void
ligma_operation_tool_aux_input_free (AuxInput *input)
{
  ligma_operation_tool_aux_input_clear (input);

  g_free (input->pad);
  g_object_unref (input->node);
  g_object_unref (input->box);

  g_slice_free (AuxInput, input);
}

static void
ligma_operation_tool_unlink_chains (LigmaOperationTool *op_tool)
{
  GObject *options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  GList   *chains;

  g_return_if_fail (options_gui != NULL);

  chains = g_object_get_data (options_gui, "chains");

  while (chains)
    {
      LigmaChainButton *chain = chains->data;
      gboolean         active;

      active = ligma_chain_button_get_active (chain);

      g_object_set_data (G_OBJECT (chain), "was-active",
                         GINT_TO_POINTER (active));

      if (active)
        {
          ligma_chain_button_set_active (chain, FALSE);
        }

      chains = chains->next;
    }

  g_object_unref (options_gui);
}

static void
ligma_operation_tool_relink_chains (LigmaOperationTool *op_tool)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (op_tool);
  GObject        *options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  GList          *chains;

  g_return_if_fail (options_gui != NULL);

  chains = g_object_get_data (options_gui, "chains");

  while (chains)
    {
      LigmaChainButton *chain = chains->data;

      if (g_object_get_data (G_OBJECT (chain), "was-active"))
        {
          const gchar *name_x    = g_object_get_data (chains->data, "x-property");
          const gchar *name_y    = g_object_get_data (chains->data, "y-property");
          const gchar *names[2]  = { name_x, name_y };
          GValue       values[2] = { G_VALUE_INIT, G_VALUE_INIT };
          GValue       double_x  = G_VALUE_INIT;
          GValue       double_y  = G_VALUE_INIT;

          g_object_getv (filter_tool->config, 2, names, values);

          g_value_init (&double_x, G_TYPE_DOUBLE);
          g_value_init (&double_y, G_TYPE_DOUBLE);

          if (g_value_transform (&values[0], &double_x) &&
              g_value_transform (&values[1], &double_y) &&
              g_value_get_double (&double_x) ==
              g_value_get_double (&double_y))
            {
              ligma_chain_button_set_active (chain, TRUE);
            }

          g_value_unset (&double_x);
          g_value_unset (&double_y);
          g_value_unset (&values[0]);
          g_value_unset (&values[1]);

          g_object_set_data (G_OBJECT (chain), "was-active", NULL);
        }

      chains = chains->next;
    }

  g_object_unref (options_gui);
}


/*  public functions  */

void
ligma_operation_tool_set_operation (LigmaOperationTool *op_tool,
                                   const gchar       *operation,
                                   const gchar       *title,
                                   const gchar       *description,
                                   const gchar       *undo_desc,
                                   const gchar       *icon_name,
                                   const gchar       *help_id)
{
  LigmaTool       *tool;
  LigmaFilterTool *filter_tool;
  GtkWidget      *options_gui;

  g_return_if_fail (LIGMA_IS_OPERATION_TOOL (op_tool));

  tool        = LIGMA_TOOL (op_tool);
  filter_tool = LIGMA_FILTER_TOOL (op_tool);

  g_free (op_tool->operation);
  g_free (op_tool->description);

  op_tool->operation   = g_strdup (operation);
  op_tool->description = g_strdup (description);

  ligma_tool_set_label     (tool, title);
  ligma_tool_set_undo_desc (tool, undo_desc);
  ligma_tool_set_icon_name (tool, icon_name);
  ligma_tool_set_help_id   (tool, help_id);

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) ligma_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;

  ligma_filter_tool_set_widget (filter_tool, NULL);

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  if (options_gui)
    {
      ligma_filter_tool_disable_color_picking (filter_tool);
      g_object_unref (options_gui);
      gtk_widget_destroy (options_gui);
    }

  if (! operation)
    return;

  ligma_filter_tool_get_operation (filter_tool);

  if (tool->drawables)
    ligma_operation_tool_sync_op (op_tool, TRUE);

  if (filter_tool->config && tool->display)
    {
      GtkWidget *options_box;

      ligma_operation_tool_create_gui (op_tool);

      options_box = g_weak_ref_get (&op_tool->options_box_ref);
      if (options_box)
        {
          ligma_operation_tool_add_gui (op_tool);
          g_object_unref (options_box);
        }
    }
}
