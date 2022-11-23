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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma-gui.h"
#include "core/ligmaasync.h"
#include "core/ligmacancelable.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-histogram.h"
#include "core/ligmahistogram.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatriviallycancelablewaitable.h"
#include "core/ligmawaitable.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmahistogrambox.h"
#include "widgets/ligmahistogramview.h"

#include "display/ligmadisplay.h"

#include "ligmahistogramoptions.h"
#include "ligmathresholdtool.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void       ligma_threshold_tool_finalize        (GObject           *object);

static gboolean   ligma_threshold_tool_initialize      (LigmaTool          *tool,
                                                       LigmaDisplay       *display,
                                                       GError           **error);

static gchar    * ligma_threshold_tool_get_operation   (LigmaFilterTool    *filter_tool,
                                                       gchar            **description);
static void       ligma_threshold_tool_dialog          (LigmaFilterTool    *filter_tool);
static void       ligma_threshold_tool_config_notify   (LigmaFilterTool    *filter_tool,
                                                       LigmaConfig        *config,
                                                       const GParamSpec  *pspec);

static gboolean   ligma_threshold_tool_channel_sensitive
                                                      (gint               value,
                                                       gpointer           data);
static void       ligma_threshold_tool_histogram_range (LigmaHistogramView *view,
                                                       gint               start,
                                                       gint               end,
                                                       LigmaThresholdTool *t_tool);
static void       ligma_threshold_tool_auto_clicked    (GtkWidget         *button,
                                                       LigmaThresholdTool *t_tool);


G_DEFINE_TYPE (LigmaThresholdTool, ligma_threshold_tool,
               LIGMA_TYPE_FILTER_TOOL)

#define parent_class ligma_threshold_tool_parent_class


void
ligma_threshold_tool_register (LigmaToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (LIGMA_TYPE_THRESHOLD_TOOL,
                LIGMA_TYPE_HISTOGRAM_OPTIONS,
                NULL,
                0,
                "ligma-threshold-tool",
                _("Threshold"),
                _("Reduce image to two colors using a threshold"),
                N_("_Threshold..."), NULL,
                NULL, LIGMA_HELP_TOOL_THRESHOLD,
                LIGMA_ICON_TOOL_THRESHOLD,
                data);
}

static void
ligma_threshold_tool_class_init (LigmaThresholdToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  LigmaToolClass       *tool_class        = LIGMA_TOOL_CLASS (klass);
  LigmaFilterToolClass *filter_tool_class = LIGMA_FILTER_TOOL_CLASS (klass);

  object_class->finalize           = ligma_threshold_tool_finalize;

  tool_class->initialize           = ligma_threshold_tool_initialize;

  filter_tool_class->get_operation = ligma_threshold_tool_get_operation;
  filter_tool_class->dialog        = ligma_threshold_tool_dialog;
  filter_tool_class->config_notify = ligma_threshold_tool_config_notify;
}

static void
ligma_threshold_tool_init (LigmaThresholdTool *t_tool)
{
  t_tool->histogram = ligma_histogram_new (FALSE);
}

static void
ligma_threshold_tool_finalize (GObject *object)
{
  LigmaThresholdTool *t_tool = LIGMA_THRESHOLD_TOOL (object);

  g_clear_object (&t_tool->histogram);
  g_clear_object (&t_tool->histogram_async);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_threshold_tool_initialize (LigmaTool     *tool,
                                LigmaDisplay  *display,
                                GError      **error)
{
  LigmaThresholdTool *t_tool      = LIGMA_THRESHOLD_TOOL (tool);
  LigmaFilterTool    *filter_tool = LIGMA_FILTER_TOOL (tool);
  LigmaImage         *image       = ligma_display_get_image (display);
  GList             *drawables;
  LigmaDrawable      *drawable;
  gdouble            low;
  gdouble            high;
  gint               n_bins;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        ligma_tool_message_literal (tool, display,
                                   _("Cannot modify multiple drawables. Select only one."));
      else
        ligma_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  g_clear_object (&t_tool->histogram_async);

  g_object_get (filter_tool->config,
                "low",  &low,
                "high", &high,
                NULL);

  /* this is a hack to make sure that
   * 'ligma_histogram_n_bins (t_tool->histogram)' returns the correct value for
   * 'drawable' before the asynchronous calculation of its histogram is
   * finished.
   */
  {
    GeglBuffer *temp;

    temp = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 1, 1),
                            ligma_drawable_get_format (drawable));

    ligma_histogram_calculate (t_tool->histogram,
                              temp, GEGL_RECTANGLE (0, 0, 1, 1),
                              NULL, NULL);

    g_object_unref (temp);
  }

  n_bins = ligma_histogram_n_bins (t_tool->histogram);

  t_tool->histogram_async = ligma_drawable_calculate_histogram_async (
    drawable, t_tool->histogram, FALSE);
  ligma_histogram_view_set_histogram (t_tool->histogram_box->view,
                                     t_tool->histogram);

  ligma_histogram_view_set_range (t_tool->histogram_box->view,
                                 low  * (n_bins - 0.0001),
                                 high * (n_bins - 0.0001));

  return TRUE;
}

static gchar *
ligma_threshold_tool_get_operation (LigmaFilterTool  *filter_tool,
                                   gchar          **description)
{
  *description = g_strdup (_("Apply Threshold"));

  return g_strdup ("ligma:threshold");
}


/**********************/
/*  Threshold dialog  */
/**********************/

static void
ligma_threshold_tool_dialog (LigmaFilterTool *filter_tool)
{
  LigmaThresholdTool    *t_tool       = LIGMA_THRESHOLD_TOOL (filter_tool);
  LigmaToolOptions      *tool_options = LIGMA_TOOL_GET_OPTIONS (filter_tool);
  GtkWidget            *main_vbox;
  GtkWidget            *main_frame;
  GtkWidget            *frame_vbox;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *hbox2;
  GtkWidget            *box;
  GtkWidget            *button;
  LigmaHistogramChannel  channel;

  main_vbox = ligma_filter_tool_dialog_get_vbox (filter_tool);

  main_frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_frame, TRUE, TRUE, 0);
  gtk_widget_show (main_frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_frame_set_label_widget (GTK_FRAME (main_frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("Cha_nnel:"));
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  t_tool->channel_menu = ligma_prop_enum_combo_box_new (filter_tool->config,
                                                       "channel", -1, -1);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (t_tool->channel_menu),
                                       "ligma-channel");
  ligma_int_combo_box_set_sensitivity (LIGMA_INT_COMBO_BOX (t_tool->channel_menu),
                                      ligma_threshold_tool_channel_sensitive,
                                      filter_tool, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), t_tool->channel_menu, FALSE, FALSE, 0);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), t_tool->channel_menu);

  hbox2 = ligma_prop_enum_icon_box_new (G_OBJECT (tool_options),
                                       "histogram-scale", "ligma-histogram",
                                       0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  frame_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (main_frame), frame_vbox);
  gtk_widget_show (frame_vbox);

  box = ligma_histogram_box_new ();
  gtk_box_pack_start (GTK_BOX (frame_vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  t_tool->histogram_box = LIGMA_HISTOGRAM_BOX (box);

  g_object_get (filter_tool->config,
                "channel", &channel,
                NULL);

  ligma_histogram_view_set_channel (t_tool->histogram_box->view, channel);

  g_signal_connect (t_tool->histogram_box->view, "range-changed",
                    G_CALLBACK (ligma_threshold_tool_histogram_range),
                    t_tool);

  g_object_bind_property (G_OBJECT (tool_options),                "histogram-scale",
                          G_OBJECT (t_tool->histogram_box->view), "histogram-scale",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (frame_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Auto"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  ligma_help_set_help_data (button, _("Automatically adjust to optimal "
                                     "binarization threshold"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_threshold_tool_auto_clicked),
                    t_tool);
}

static void
ligma_threshold_tool_config_notify (LigmaFilterTool   *filter_tool,
                                   LigmaConfig       *config,
                                   const GParamSpec *pspec)
{
  LigmaThresholdTool *t_tool = LIGMA_THRESHOLD_TOOL (filter_tool);

  LIGMA_FILTER_TOOL_CLASS (parent_class)->config_notify (filter_tool,
                                                        config, pspec);

  if (! t_tool->histogram_box)
    return;

  if (! strcmp (pspec->name, "channel"))
    {
      LigmaHistogramChannel channel;

      g_object_get (config,
                    "channel", &channel,
                    NULL);

      ligma_histogram_view_set_channel (t_tool->histogram_box->view,
                                       channel);
    }
  else if (! strcmp (pspec->name, "low") ||
           ! strcmp (pspec->name, "high"))
    {
      gdouble low;
      gdouble high;
      gint    n_bins;

      g_object_get (config,
                    "low",  &low,
                    "high", &high,
                    NULL);

      n_bins = ligma_histogram_n_bins (t_tool->histogram);

      ligma_histogram_view_set_range (t_tool->histogram_box->view,
                                     low  * (n_bins - 0.0001),
                                     high * (n_bins - 0.0001));
    }
}

static gboolean
ligma_threshold_tool_channel_sensitive (gint     value,
                                       gpointer data)
{
  GList                *drawables = LIGMA_TOOL (data)->drawables;
  LigmaDrawable         *drawable;
  LigmaHistogramChannel  channel   = value;

  if (!drawables)
    return FALSE;

  g_return_val_if_fail (g_list_length (drawables) == 1, FALSE);
  drawable = drawables->data;

  switch (channel)
    {
    case LIGMA_HISTOGRAM_VALUE:
      return TRUE;

    case LIGMA_HISTOGRAM_RED:
    case LIGMA_HISTOGRAM_GREEN:
    case LIGMA_HISTOGRAM_BLUE:
      return ligma_drawable_is_rgb (drawable);

    case LIGMA_HISTOGRAM_ALPHA:
      return ligma_drawable_has_alpha (drawable);

    case LIGMA_HISTOGRAM_RGB:
      return ligma_drawable_is_rgb (drawable);

    case LIGMA_HISTOGRAM_LUMINANCE:
      return ligma_drawable_is_rgb (drawable);
    }

  return FALSE;
}

static void
ligma_threshold_tool_histogram_range (LigmaHistogramView *widget,
                                     gint               start,
                                     gint               end,
                                     LigmaThresholdTool *t_tool)
{
  LigmaFilterTool *filter_tool = LIGMA_FILTER_TOOL (t_tool);
  gint            n_bins      = ligma_histogram_n_bins (t_tool->histogram);
  gdouble         low         = (gdouble) start / (n_bins - 1);
  gdouble         high        = (gdouble) end   / (n_bins - 1);
  gdouble         config_low;
  gdouble         config_high;

  g_object_get (filter_tool->config,
                "low",  &config_low,
                "high", &config_high,
                NULL);

  if (low  != config_low ||
      high != config_high)
    {
      g_object_set (filter_tool->config,
                    "low",  low,
                    "high", high,
                    NULL);
    }
}

static void
ligma_threshold_tool_auto_clicked (GtkWidget         *button,
                                  LigmaThresholdTool *t_tool)
{
  LigmaTool     *tool = LIGMA_TOOL (t_tool);
  LigmaWaitable *waitable;

  waitable = ligma_trivially_cancelable_waitable_new (
    LIGMA_WAITABLE (t_tool->histogram_async));

  ligma_wait (tool->tool_info->ligma, waitable, _("Calculating histogram..."));

  g_object_unref (waitable);

  if (ligma_async_is_synced   (t_tool->histogram_async) &&
      ligma_async_is_finished (t_tool->histogram_async))
    {
      LigmaHistogramChannel channel;
      gint                 n_bins;
      gdouble              low;

      g_object_get (LIGMA_FILTER_TOOL (t_tool)->config,
                    "channel", &channel,
                    NULL);

      n_bins = ligma_histogram_n_bins (t_tool->histogram);

      low = ligma_histogram_get_threshold (t_tool->histogram,
                                          channel,
                                          0, n_bins - 1);

      ligma_histogram_view_set_range (t_tool->histogram_box->view,
                                     low, n_bins - 1);
    }
}
