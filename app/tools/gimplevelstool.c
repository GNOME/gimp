/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/gimphistogram.h"
#include "base/gimplut.h"
#include "base/levels.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-histogram.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpenummenu.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimphistogramview.h"

#include "display/gimpdisplay.h"

#include "gimphistogramoptions.h"
#include "gimplevelstool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define LOW_INPUT        (1 << 0)
#define GAMMA            (1 << 1)
#define HIGH_INPUT       (1 << 2)
#define LOW_OUTPUT       (1 << 3)
#define HIGH_OUTPUT      (1 << 4)
#define INPUT_LEVELS     (1 << 5)
#define OUTPUT_LEVELS    (1 << 6)
#define INPUT_SLIDERS    (1 << 7)
#define OUTPUT_SLIDERS   (1 << 8)
#define DRAW             (1 << 9)
#define ALL_CHANNELS     (1 << 10)
#define ALL              0xFFF

#define HISTOGRAM_WIDTH   256
#define GRADIENT_HEIGHT   12
#define CONTROL_HEIGHT    8

#define LEVELS_DA_MASK  (GDK_EXPOSURE_MASK       | \
                         GDK_ENTER_NOTIFY_MASK   | \
			 GDK_BUTTON_PRESS_MASK   | \
			 GDK_BUTTON_RELEASE_MASK | \
			 GDK_BUTTON1_MOTION_MASK | \
			 GDK_POINTER_MOTION_HINT_MASK)


/*  local function prototypes  */

static void   gimp_levels_tool_class_init  (GimpLevelsToolClass *klass);
static void   gimp_levels_tool_init        (GimpLevelsTool      *tool);
static void   gimp_levels_tool_finalize    (GObject             *object);

static void   gimp_levels_tool_initialize     (GimpTool         *tool,
					       GimpDisplay      *gdisp);

static void   gimp_levels_tool_color_picked  (GimpColorTool *color_tool,
                                              GimpImageType  sample_type,
                                              GimpRGB       *color,
                                              gint           color_index);
static void   gimp_levels_tool_map        (GimpImageMapTool *image_map_tool);
static void   gimp_levels_tool_dialog     (GimpImageMapTool *image_map_tool);
static void   gimp_levels_tool_reset      (GimpImageMapTool *image_map_tool);
static void   levels_update                        (GimpLevelsTool *l_tool,
						    gint            update);
static void   levels_channel_callback              (GtkWidget      *widget,
						    GimpLevelsTool *l_tool);
static void   levels_channel_reset_callback        (GtkWidget      *widget,
						    GimpLevelsTool *l_tool);
static gboolean levels_set_sensitive_callback      (GimpHistogramChannel channel,
                                                    GimpLevelsTool *l_tool);
static void   levels_auto_callback                 (GtkWidget      *widget,
						    GimpLevelsTool *l_tool);
static void   levels_load_callback                 (GtkWidget      *widget,
						    GimpLevelsTool *l_tool);
static void   levels_save_callback                 (GtkWidget      *widget,
						    GimpLevelsTool *l_tool);
static void   levels_low_input_adjustment_update   (GtkAdjustment  *adjustment,
						    GimpLevelsTool *l_tool);
static void   levels_gamma_adjustment_update       (GtkAdjustment  *adjustment,
						    GimpLevelsTool *l_tool);
static void   levels_high_input_adjustment_update  (GtkAdjustment  *adjustment,
						    GimpLevelsTool *l_tool);
static void   levels_low_output_adjustment_update  (GtkAdjustment  *adjustment,
						    GimpLevelsTool *l_tool);
static void   levels_high_output_adjustment_update (GtkAdjustment  *adjustment,
						    GimpLevelsTool *l_tool);
static void   levels_input_picker_toggled          (GtkWidget      *widget,
                                                    GimpLevelsTool *l_tool);
static gint   levels_input_da_events               (GtkWidget      *widget,
						    GdkEvent       *event,
						    GimpLevelsTool *l_tool);
static gint   levels_output_da_events              (GtkWidget      *widget,
						    GdkEvent       *event,
						    GimpLevelsTool *l_tool);

static void      file_dialog_create                (GimpLevelsTool *l_tool);
static void      file_dialog_response              (GtkWidget      *dialog,
                                                    gint            response_id,
                                                    GimpLevelsTool *l_tool);

static gboolean  levels_read_from_file             (GimpLevelsTool *l_tool,
                                                    FILE           *f);
static void      levels_write_to_file              (GimpLevelsTool *l_tool,
                                                    FILE           *f);


static GimpImageMapToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_levels_tool_register (GimpToolRegisterCallback  callback,
                           gpointer                  data)
{
  (* callback) (GIMP_TYPE_LEVELS_TOOL,
                GIMP_TYPE_HISTOGRAM_OPTIONS,
                gimp_color_options_gui,
                0,
                "gimp-levels-tool",
                _("Levels"),
                _("Adjust color levels"),
                N_("/Tools/Color Tools/_Levels..."), NULL,
                NULL, GIMP_HELP_TOOL_LEVELS,
                GIMP_STOCK_TOOL_LEVELS,
                data);
}

GType
gimp_levels_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpLevelsToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_levels_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpLevelsTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_levels_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_IMAGE_MAP_TOOL,
					  "GimpLevelsTool",
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

static void
gimp_levels_tool_class_init (GimpLevelsToolClass *klass)
{
  GObjectClass          *object_class;
  GimpToolClass         *tool_class;
  GimpColorToolClass    *color_tool_class;
  GimpImageMapToolClass *image_map_tool_class;

  object_class         = G_OBJECT_CLASS (klass);
  tool_class           = GIMP_TOOL_CLASS (klass);
  color_tool_class     = GIMP_COLOR_TOOL_CLASS (klass);
  image_map_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize       = gimp_levels_tool_finalize;

  tool_class->initialize       = gimp_levels_tool_initialize;

  color_tool_class->picked     = gimp_levels_tool_color_picked;

  image_map_tool_class->map    = gimp_levels_tool_map;
  image_map_tool_class->dialog = gimp_levels_tool_dialog;
  image_map_tool_class->reset  = gimp_levels_tool_reset;
}

static void
gimp_levels_tool_init (GimpLevelsTool *l_tool)
{
  GimpImageMapTool *image_map_tool = GIMP_IMAGE_MAP_TOOL (l_tool);

  image_map_tool->shell_desc = _("Adjust Color Levels");

  l_tool->lut           = gimp_lut_new ();
  l_tool->levels        = g_new0 (Levels, 1);
  l_tool->hist          = NULL;
  l_tool->channel       = GIMP_HISTOGRAM_VALUE;
  l_tool->active_picker = NULL;

  levels_init (l_tool->levels);
}

static void
gimp_levels_tool_finalize (GObject *object)
{
  GimpLevelsTool *l_tool = GIMP_LEVELS_TOOL (object);

  if (l_tool->lut)
    {
      gimp_lut_free (l_tool->lut);
      l_tool->lut = NULL;
    }

  if (l_tool->levels)
    {
      g_free (l_tool->levels);
      l_tool->levels = NULL;
    }

  if (l_tool->hist)
    {
      gimp_histogram_free (l_tool->hist);
      l_tool->hist = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_levels_tool_initialize (GimpTool    *tool,
			     GimpDisplay *gdisp)
{
  GimpLevelsTool *l_tool = GIMP_LEVELS_TOOL (tool);
  GimpDrawable   *drawable;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! drawable)
    return;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Levels for indexed layers cannot be adjusted."));
      return;
    }

  if (!l_tool->hist)
    {
      Gimp *gimp = GIMP_TOOL (l_tool)->tool_info->gimp;

      l_tool->hist = gimp_histogram_new (GIMP_BASE_CONFIG (gimp->config));
    }

  levels_init (l_tool->levels);

  l_tool->channel = GIMP_HISTOGRAM_VALUE;
  l_tool->color   = gimp_drawable_is_rgb (drawable);

  if (l_tool->active_picker)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (l_tool->active_picker),
				  FALSE);

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, gdisp);

  /* set the sensitivity of the channel menu based on the drawable type */
  gimp_int_option_menu_set_sensitive (GTK_OPTION_MENU (l_tool->channel_menu),
                                      (GimpIntOptionMenuSensitivityCallback) levels_set_sensitive_callback,
                                      l_tool);

  /* set the current selection */
  gtk_option_menu_set_history (GTK_OPTION_MENU (l_tool->channel_menu),
			       l_tool->channel);

  levels_update (l_tool, (LOW_INPUT | GAMMA | HIGH_INPUT |
                          LOW_OUTPUT | HIGH_OUTPUT |
                          DRAW | INPUT_LEVELS | OUTPUT_LEVELS));

  gimp_drawable_calculate_histogram (drawable, l_tool->hist);
  gimp_histogram_view_set_histogram (GIMP_HISTOGRAM_VIEW (l_tool->hist_view),
                                     l_tool->hist);
}

static void
gimp_levels_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool *l_tool = GIMP_LEVELS_TOOL (image_map_tool);

  gimp_image_map_apply (image_map_tool->image_map,
                        (GimpImageMapApplyFunc) gimp_lut_process_2,
                        l_tool->lut);
}


/*******************/
/*  Levels dialog  */
/*******************/


static GtkWidget *
gimp_levels_tool_color_picker_new (GimpLevelsTool *tool,
				   guint           value)
{
  GtkWidget   *button;
  const gchar *stock_id;
  const gchar *help;

  switch (value & 0xF)
    {
    case LOW_INPUT:
      stock_id = GIMP_STOCK_COLOR_PICKER_BLACK;
      help     = _("Pick Black Point");
      break;
    case GAMMA:
      stock_id = GIMP_STOCK_COLOR_PICKER_GRAY;
      help     = _("Pick Gray Point");
      break;
    case HIGH_INPUT:
      stock_id = GIMP_STOCK_COLOR_PICKER_WHITE;
      help     = _("Pick White Point");
      break;
    default:
      return NULL;
    }

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "label",          stock_id,
                         "use_stock",      TRUE,
                         "draw_indicator", FALSE,
                         NULL);

  gimp_help_set_help_data (button, help, NULL);

  g_object_set_data (G_OBJECT (button), "pick_value", GUINT_TO_POINTER (value));
  g_signal_connect (button, "toggled",
                    G_CALLBACK (levels_input_picker_toggled),
                    tool);

  return button;
}

static void
gimp_levels_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool  *l_tool = GIMP_LEVELS_TOOL (image_map_tool);
  GimpToolOptions *tool_options;
  GtkWidget       *vbox;
  GtkWidget       *vbox2;
  GtkWidget       *vbox3;
  GtkWidget       *hbox;
  GtkWidget       *hbox2;
  GtkWidget       *label;
  GtkWidget       *menu;
  GtkWidget       *frame;
  GtkWidget       *channel_hbox;
  GtkWidget       *hbbox;
  GtkWidget       *button;
  GtkWidget       *spinbutton;
  GtkObject       *data;

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);
  gtk_widget_show (channel_hbox);

  label = gtk_label_new (_("Modify Levels for Channel:"));
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  menu = gimp_enum_option_menu_new (GIMP_TYPE_HISTOGRAM_CHANNEL,
                                    G_CALLBACK (levels_channel_callback),
                                    l_tool);
  gimp_enum_option_menu_set_stock_prefix (GTK_OPTION_MENU (menu),
                                          "gimp-channel");
  gtk_box_pack_start (GTK_BOX (channel_hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  l_tool->channel_menu = menu;

  button = gtk_button_new_with_mnemonic (_("R_eset Channel"));
  gtk_box_pack_start (GTK_BOX (channel_hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_channel_reset_callback),
                    l_tool);


  /*  Input levels frame  */
  frame = gtk_frame_new (_("Input Levels"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, FALSE, 0);
  gtk_widget_show (vbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  l_tool->hist_view = gimp_histogram_view_new (FALSE);
  gtk_container_add (GTK_CONTAINER (frame), l_tool->hist_view);
  gtk_widget_show (GTK_WIDGET (l_tool->hist_view));

  tool_options = GIMP_TOOL (l_tool)->tool_info->tool_options;
  gimp_histogram_options_connect_view (GIMP_HISTOGRAM_OPTIONS (tool_options),
                                       GIMP_HISTOGRAM_VIEW (l_tool->hist_view));

  /*  The input levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox3 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show (vbox3);

  l_tool->input_levels_da[0] = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (l_tool->input_levels_da[0]),
		    HISTOGRAM_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (l_tool->input_levels_da[0], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox3), l_tool->input_levels_da[0],
                      FALSE, TRUE, 0);
  gtk_widget_show (l_tool->input_levels_da[0]);

  g_signal_connect (l_tool->input_levels_da[0], "event",
                    G_CALLBACK (levels_input_da_events),
                    l_tool);

  l_tool->input_levels_da[1] = gtk_drawing_area_new ();
  gtk_widget_set_size_request (l_tool->input_levels_da[1],
                               HISTOGRAM_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (l_tool->input_levels_da[1], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox3), l_tool->input_levels_da[1],
                      FALSE, TRUE, 0);
  gtk_widget_show (l_tool->input_levels_da[1]);

  g_signal_connect (l_tool->input_levels_da[1], "event",
                    G_CALLBACK (levels_input_da_events),
                    l_tool);

  /*  Horizontal box for input levels spinbuttons  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low input spin  */
  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (l_tool, LOW_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  data = gtk_adjustment_new (0, 0, 255, 1, 10, 10);
  l_tool->low_input_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (l_tool->low_input_data, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (l_tool->low_input_data, "value_changed",
                    G_CALLBACK (levels_low_input_adjustment_update),
                    l_tool);

  /*  input gamma spin  */
  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (l_tool, GAMMA);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  data = gtk_adjustment_new (1, 0.1, 10, 0.1, 1, 1);
  l_tool->gamma_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (l_tool->gamma_data, 0.5, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gimp_help_set_help_data (spinbutton, _("Gamma"), NULL);
  gtk_widget_show (spinbutton);

  g_signal_connect (l_tool->gamma_data, "value_changed",
                    G_CALLBACK (levels_gamma_adjustment_update),
                    l_tool);

  /*  high input spin  */
  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  button = gimp_levels_tool_color_picker_new (l_tool, HIGH_INPUT);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  data = gtk_adjustment_new (255, 0, 255, 1, 10, 10);
  l_tool->high_input_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (l_tool->high_input_data, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox2), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (l_tool->high_input_data, "value_changed",
                    G_CALLBACK (levels_high_input_adjustment_update),
                    l_tool);


  /*  Output levels frame  */
  frame = gtk_frame_new (_("Output Levels"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, FALSE, 0);
  gtk_widget_show (vbox2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox3 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show (vbox3);

  l_tool->output_levels_da[0] = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (l_tool->output_levels_da[0]),
                    HISTOGRAM_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (l_tool->output_levels_da[0], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox3), l_tool->output_levels_da[0],
                      FALSE, TRUE, 0);
  gtk_widget_show (l_tool->output_levels_da[0]);

  g_signal_connect (l_tool->output_levels_da[0], "event",
                    G_CALLBACK (levels_output_da_events),
                    l_tool);

  l_tool->output_levels_da[1] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (l_tool->output_levels_da[1]),
                    HISTOGRAM_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (l_tool->output_levels_da[1], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox3), l_tool->output_levels_da[1],
                      FALSE, TRUE, 0);
  gtk_widget_show (l_tool->output_levels_da[1]);

  g_signal_connect (l_tool->output_levels_da[1], "event",
                    G_CALLBACK (levels_output_da_events),
                    l_tool);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  low output spin  */
  data = gtk_adjustment_new (0, 0, 255, 1, 10, 10);
  l_tool->low_output_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (l_tool->low_output_data, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (l_tool->low_output_data, "value_changed",
                    G_CALLBACK (levels_low_output_adjustment_update),
                    l_tool);

  /*  high output spin  */
  data = gtk_adjustment_new (255, 0, 255, 1, 10, 10);
  l_tool->high_output_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (l_tool->high_output_data, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (l_tool->high_output_data, "value_changed",
                    G_CALLBACK (levels_high_output_adjustment_update),
                    l_tool);


  frame = gtk_frame_new (_("All Channels"));
  gtk_box_pack_end (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  hbbox = gtk_hbutton_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (hbbox), 2);
  gtk_box_set_spacing (GTK_BOX (hbbox), 4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OPEN);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Read levels settings from file"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_load_callback),
                    l_tool);

  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gimp_help_set_help_data (button, _("Save levels settings to file"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_save_callback),
                    l_tool);

  hbbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbbox), 2);
  gtk_box_pack_start (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);
  gtk_widget_show (hbbox);

  button = gtk_button_new_with_mnemonic (_("_Auto"));
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 2);
  gimp_help_set_help_data (button, _("Adjust levels automatically"), NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (levels_auto_callback),
                    l_tool);

  button = gimp_levels_tool_color_picker_new (l_tool,
                                              LOW_INPUT | ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (l_tool, GAMMA | ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_levels_tool_color_picker_new (l_tool,
                                              HIGH_INPUT | ALL_CHANNELS);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
gimp_levels_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpLevelsTool *l_tool = GIMP_LEVELS_TOOL (image_map_tool);

  levels_init (l_tool->levels);
  levels_update (l_tool, ALL);
}

static void
levels_draw_slider (GdkWindow *window,
		    GdkGC     *border_gc,
		    GdkGC     *fill_gc,
		    gint       xpos)
{
  int y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line (window, fill_gc,
                   xpos - y / 2, y,
                   xpos + y / 2, y);

  gdk_draw_line (window, border_gc,
                 xpos, 0,
		 xpos - (CONTROL_HEIGHT - 1) / 2,  CONTROL_HEIGHT - 1);

  gdk_draw_line (window, border_gc,
                 xpos, 0,
		 xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);

  gdk_draw_line (window, border_gc,
                 xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1,
		 xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
}

static void
levels_erase_slider (GdkWindow *window,
		     gint       xpos)
{
  gdk_window_clear_area (window, xpos - (CONTROL_HEIGHT - 1) / 2, 0,
			 CONTROL_HEIGHT - 1, CONTROL_HEIGHT);
}

static void
levels_update (GimpLevelsTool *l_tool,
	       gint            update)
{
  gint i;
  gint sel_channel;

  if (l_tool->color)
    {
      sel_channel = l_tool->channel;
    }
  else
    {
      if (l_tool->channel == 2)
	sel_channel = GIMP_HISTOGRAM_ALPHA;
      else
	sel_channel = GIMP_HISTOGRAM_VALUE;
    }

  /*  Recalculate the transfer arrays  */
  levels_calculate_transfers (l_tool->levels);

  /* set up the lut */
  gimp_lut_setup (l_tool->lut,
                  (GimpLutFunc) levels_lut_func,
                  l_tool->levels,
                  gimp_drawable_bytes (GIMP_IMAGE_MAP_TOOL (l_tool)->drawable));

  if (update & LOW_INPUT)
    gtk_adjustment_set_value (l_tool->low_input_data,
                              l_tool->levels->low_input[l_tool->channel]);

  if (update & GAMMA)
    gtk_adjustment_set_value (l_tool->gamma_data,
                              l_tool->levels->gamma[l_tool->channel]);

  if (update & HIGH_INPUT)
    gtk_adjustment_set_value (l_tool->high_input_data,
                              l_tool->levels->high_input[l_tool->channel]);

  if (update & LOW_OUTPUT)
    gtk_adjustment_set_value (l_tool->low_output_data,
                              l_tool->levels->low_output[l_tool->channel]);

  if (update & HIGH_OUTPUT)
    gtk_adjustment_set_value (l_tool->high_output_data,
                              l_tool->levels->high_output[l_tool->channel]);

  if (update & INPUT_LEVELS)
    {
      guchar buf[HISTOGRAM_WIDTH*3];

      switch (sel_channel)
	{
	default:
	  g_warning ("unknown channel type, can't happen\n");
	  /* fall through */
	case GIMP_HISTOGRAM_VALUE:
	case GIMP_HISTOGRAM_ALPHA:
	  for (i = 0; i < HISTOGRAM_WIDTH; i++)
	    {
	      buf[3 * i + 0] = l_tool->levels->input[sel_channel][i];
	      buf[3 * i + 1] = l_tool->levels->input[sel_channel][i];
	      buf[3 * i + 2] = l_tool->levels->input[sel_channel][i];
	    }
	  break;

	case GIMP_HISTOGRAM_RED:
	case GIMP_HISTOGRAM_GREEN:
	case GIMP_HISTOGRAM_BLUE:
	  for (i = 0; i < HISTOGRAM_WIDTH; i++)
	    {
	      buf[3 * i + 0] = l_tool->levels->input[GIMP_HISTOGRAM_RED][i];
	      buf[3 * i + 1] = l_tool->levels->input[GIMP_HISTOGRAM_GREEN][i];
	      buf[3 * i + 2] = l_tool->levels->input[GIMP_HISTOGRAM_BLUE][i];
	    }
	  break;
	}

      for (i = 0; i < GRADIENT_HEIGHT/2; i++)
	gtk_preview_draw_row (GTK_PREVIEW (l_tool->input_levels_da[0]),
			      buf, 0, i, HISTOGRAM_WIDTH);

      for (i = 0; i < HISTOGRAM_WIDTH; i++)
	{
	  buf[3 * i + 0] = i;
	  buf[3 * i + 1] = i;
	  buf[3 * i + 2] = i;
	}

      for (i = GRADIENT_HEIGHT/2; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (l_tool->input_levels_da[0]),
			      buf, 0, i, HISTOGRAM_WIDTH);

      if (update & DRAW)
	gtk_widget_queue_draw (l_tool->input_levels_da[0]);
    }

  if (update & OUTPUT_LEVELS)
    {
      guchar buf[HISTOGRAM_WIDTH*3];
      guchar r, g, b;

      r = g = b = 0;
      switch (sel_channel)
	{
	default:
	  g_warning ("unknown channel type, can't happen\n");
	  /* fall through */
	case GIMP_HISTOGRAM_VALUE:
	case GIMP_HISTOGRAM_ALPHA:  r = g = b = 1; break;
	case GIMP_HISTOGRAM_RED:    r = 1;         break;
	case GIMP_HISTOGRAM_GREEN:  g = 1;         break;
	case GIMP_HISTOGRAM_BLUE:   b = 1;         break;
	}

      for (i = 0; i < HISTOGRAM_WIDTH; i++)
	{
	  buf[3 * i + 0] = i * r;
	  buf[3 * i + 1] = i * g;
	  buf[3 * i + 2] = i * b;
	}

      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (l_tool->output_levels_da[0]),
			      buf, 0, i, HISTOGRAM_WIDTH);

      if (update & DRAW)
	gtk_widget_queue_draw (l_tool->output_levels_da[0]);
    }

  if (update & INPUT_SLIDERS)
    {
      gdouble width, mid, tmp;

      levels_erase_slider (l_tool->input_levels_da[1]->window,
                           l_tool->slider_pos[0]);
      levels_erase_slider (l_tool->input_levels_da[1]->window,
                           l_tool->slider_pos[1]);
      levels_erase_slider (l_tool->input_levels_da[1]->window,
                           l_tool->slider_pos[2]);

      l_tool->slider_pos[0] =
        HISTOGRAM_WIDTH * ((gdouble) l_tool->levels->low_input[l_tool->channel] / 255.0);

      l_tool->slider_pos[2] =
        HISTOGRAM_WIDTH * ((gdouble) l_tool->levels->high_input[l_tool->channel] / 255.0);

      width = (gdouble) (l_tool->slider_pos[2] - l_tool->slider_pos[0]) / 2.0;
      mid   = l_tool->slider_pos[0] + width;
      tmp   = log10 (1.0 / l_tool->levels->gamma[l_tool->channel]);

      l_tool->slider_pos[1] = (gint) (mid + width * tmp + 0.5);

      levels_draw_slider (l_tool->input_levels_da[1]->window,
			  l_tool->input_levels_da[1]->style->black_gc,
			  l_tool->input_levels_da[1]->style->dark_gc[GTK_STATE_NORMAL],
			  l_tool->slider_pos[1]);
      levels_draw_slider (l_tool->input_levels_da[1]->window,
			  l_tool->input_levels_da[1]->style->black_gc,
			  l_tool->input_levels_da[1]->style->black_gc,
			  l_tool->slider_pos[0]);
      levels_draw_slider (l_tool->input_levels_da[1]->window,
			  l_tool->input_levels_da[1]->style->black_gc,
			  l_tool->input_levels_da[1]->style->white_gc,
			  l_tool->slider_pos[2]);
    }

  if (update & OUTPUT_SLIDERS)
    {
      levels_erase_slider (l_tool->output_levels_da[1]->window,
                           l_tool->slider_pos[3]);
      levels_erase_slider (l_tool->output_levels_da[1]->window,
                           l_tool->slider_pos[4]);

      l_tool->slider_pos[3] =
        HISTOGRAM_WIDTH * ((gdouble) l_tool->levels->low_output[l_tool->channel] / 255.0);

      l_tool->slider_pos[4] =
        HISTOGRAM_WIDTH * ((gdouble) l_tool->levels->high_output[l_tool->channel] / 255.0);

      levels_draw_slider (l_tool->output_levels_da[1]->window,
			  l_tool->output_levels_da[1]->style->black_gc,
			  l_tool->output_levels_da[1]->style->black_gc,
			  l_tool->slider_pos[3]);
      levels_draw_slider (l_tool->output_levels_da[1]->window,
			  l_tool->output_levels_da[1]->style->black_gc,
			  l_tool->output_levels_da[1]->style->white_gc,
			  l_tool->slider_pos[4]);
    }
}

static void
levels_channel_callback (GtkWidget      *widget,
			 GimpLevelsTool *l_tool)
{
  gimp_menu_item_update (widget, &l_tool->channel);

  gimp_histogram_view_set_channel (GIMP_HISTOGRAM_VIEW (l_tool->hist_view),
                                   l_tool->channel);

  /* FIXME: hack */
  if (! l_tool->color)
    l_tool->channel = (l_tool->channel > 1) ? 2 : 1;

  levels_update (l_tool, ALL);
}

static void
levels_channel_reset_callback (GtkWidget      *widget,
                               GimpLevelsTool *l_tool)
{
  levels_channel_reset (l_tool->levels, l_tool->channel);
  levels_update (l_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
}

static gboolean
levels_set_sensitive_callback (GimpHistogramChannel  channel,
                               GimpLevelsTool       *l_tool)
{
  switch (channel)
    {
    case GIMP_HISTOGRAM_VALUE:
      return TRUE;

    case GIMP_HISTOGRAM_RED:
    case GIMP_HISTOGRAM_GREEN:
    case GIMP_HISTOGRAM_BLUE:
      return l_tool->color;

    case GIMP_HISTOGRAM_ALPHA:
      return gimp_drawable_has_alpha (GIMP_IMAGE_MAP_TOOL (l_tool)->drawable);
    }

  return FALSE;
}

static void
levels_auto_callback (GtkWidget      *widget,
		      GimpLevelsTool *l_tool)
{
  levels_auto (l_tool->levels, l_tool->hist, l_tool->color);
  levels_update (l_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
}

static void
levels_low_input_adjustment_update (GtkAdjustment  *adjustment,
				    GimpLevelsTool *l_tool)
{
  gint value;

  value = (gint) (adjustment->value + 0.5);
  value = CLAMP (value, 0, l_tool->levels->high_input[l_tool->channel]);

  /*  enforce a consistent displayed value (low_input <= high_input)  */
  gtk_adjustment_set_value (adjustment, value);

  if (l_tool->levels->low_input[l_tool->channel] != value)
    {
      l_tool->levels->low_input[l_tool->channel] = value;
      levels_update (l_tool, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
    }
}

static void
levels_gamma_adjustment_update (GtkAdjustment  *adjustment,
				GimpLevelsTool *l_tool)
{
  if (l_tool->levels->gamma[l_tool->channel] != adjustment->value)
    {
      l_tool->levels->gamma[l_tool->channel] = adjustment->value;
      levels_update (l_tool, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
    }
}

static void
levels_high_input_adjustment_update (GtkAdjustment  *adjustment,
				     GimpLevelsTool *l_tool)
{
  gint value;

  value = (gint) (adjustment->value + 0.5);
  value = CLAMP (value, l_tool->levels->low_input[l_tool->channel], 255);

  /*  enforce a consistent displayed value (high_input >= low_input)  */
  gtk_adjustment_set_value (adjustment, value);

  if (l_tool->levels->high_input[l_tool->channel] != value)
    {
      l_tool->levels->high_input[l_tool->channel] = value;
      levels_update (l_tool, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
    }
}

static void
levels_low_output_adjustment_update (GtkAdjustment  *adjustment,
				     GimpLevelsTool *l_tool)
{
  gint value;

  value = (gint) (adjustment->value + 0.5);

  if (l_tool->levels->low_output[l_tool->channel] != value)
    {
      l_tool->levels->low_output[l_tool->channel] = value;
      levels_update (l_tool, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
    }
}

static void
levels_high_output_adjustment_update (GtkAdjustment  *adjustment,
				      GimpLevelsTool *l_tool)
{
  gint value;

  value = (gint) (adjustment->value + 0.5);

  if (l_tool->levels->high_output[l_tool->channel] != value)
    {
      l_tool->levels->high_output[l_tool->channel] = value;
      levels_update (l_tool, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
    }
}

static void
levels_input_picker_toggled (GtkWidget      *widget,
                             GimpLevelsTool *tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      if (tool->active_picker == widget)
        return;

      if (tool->active_picker)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tool->active_picker),
                                      FALSE);

      tool->active_picker = widget;

      gimp_color_tool_enable (GIMP_COLOR_TOOL (tool),
                              GIMP_COLOR_OPTIONS (GIMP_TOOL (tool)->tool_info->tool_options));
    }
  else if (tool->active_picker == widget)
    {
      tool->active_picker = NULL;
      gimp_color_tool_disable (GIMP_COLOR_TOOL (tool));
    }
}

static gboolean
levels_input_da_events (GtkWidget      *widget,
			GdkEvent       *event,
			GimpLevelsTool *l_tool)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gchar           text[12];
  gdouble         width, mid, tmp;
  gint            x, distance;
  gint            i;
  gboolean        update = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == l_tool->input_levels_da[1])
	levels_update (l_tool, INPUT_SLIDERS);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 3; i++)
	if (fabs (bevent->x - l_tool->slider_pos[i]) < distance)
	  {
	    l_tool->active_slider = i;
	    distance = fabs (bevent->x - l_tool->slider_pos[i]);
	  }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);
      switch (l_tool->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (l_tool, LOW_INPUT | GAMMA | DRAW);
	  break;
	case 1:  /*  gamma  */
	  levels_update (l_tool, GAMMA);
	  break;
	case 2:  /*  high input  */
	  levels_update (l_tool, HIGH_INPUT | GAMMA | DRAW);
	  break;
	}

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      update = TRUE;
      break;

    default:
      break;
    }

  if (update)
    {
      switch (l_tool->active_slider)
	{
	case 0:  /*  low input  */
	  l_tool->levels->low_input[l_tool->channel] =
            ((gdouble) x / (gdouble) HISTOGRAM_WIDTH) * 255.0;

	  l_tool->levels->low_input[l_tool->channel] =
            CLAMP (l_tool->levels->low_input[l_tool->channel],
                   0, l_tool->levels->high_input[l_tool->channel]);
	  break;

	case 1:  /*  gamma  */
	  width = (double) (l_tool->slider_pos[2] - l_tool->slider_pos[0]) / 2.0;
	  mid   = l_tool->slider_pos[0] + width;

	  x   = CLAMP (x, l_tool->slider_pos[0], l_tool->slider_pos[2]);
	  tmp = (double) (x - mid) / width;

	  l_tool->levels->gamma[l_tool->channel] = 1.0 / pow (10, tmp);

	  /*  round the gamma value to the nearest 1/100th  */
	  sprintf (text, "%2.2f", l_tool->levels->gamma[l_tool->channel]);
	  l_tool->levels->gamma[l_tool->channel] = atof (text);
	  break;

	case 2:  /*  high input  */
	  l_tool->levels->high_input[l_tool->channel] =
            ((gdouble) x / (gdouble) HISTOGRAM_WIDTH) * 255.0;

	  l_tool->levels->high_input[l_tool->channel] =
            CLAMP (l_tool->levels->high_input[l_tool->channel],
                   l_tool->levels->low_input[l_tool->channel], 255);
	  break;
	}

      levels_update (l_tool, INPUT_SLIDERS | INPUT_LEVELS | DRAW);
    }

  return FALSE;
}

static gboolean
levels_output_da_events (GtkWidget      *widget,
			 GdkEvent       *event,
			 GimpLevelsTool *l_tool)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            x, distance;
  gint            i;
  gboolean        update = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == l_tool->output_levels_da[1])
	levels_update (l_tool, OUTPUT_SLIDERS);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 3; i < 5; i++)
	if (fabs (bevent->x - l_tool->slider_pos[i]) < distance)
	  {
	    l_tool->active_slider = i;
	    distance = fabs (bevent->x - l_tool->slider_pos[i]);
	  }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      switch (l_tool->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (l_tool, LOW_OUTPUT | DRAW);
	  break;
	case 4:  /*  high output  */
	  levels_update (l_tool, HIGH_OUTPUT | DRAW);
	  break;
	}

      gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      gdk_window_get_pointer (widget->window, &x, NULL, NULL);
      update = TRUE;
      break;

    default:
      break;
    }

  if (update)
    {
      switch (l_tool->active_slider)
	{
	case 3:  /*  low output  */
	  l_tool->levels->low_output[l_tool->channel] =
            ((gdouble) x / (gdouble) HISTOGRAM_WIDTH) * 255.0;

	  l_tool->levels->low_output[l_tool->channel] =
            CLAMP (l_tool->levels->low_output[l_tool->channel], 0, 255);
	  break;

	case 4:  /*  high output  */
	  l_tool->levels->high_output[l_tool->channel] =
            ((gdouble) x / (gdouble) HISTOGRAM_WIDTH) * 255.0;

	  l_tool->levels->high_output[l_tool->channel] =
            CLAMP (l_tool->levels->high_output[l_tool->channel], 0, 255);
	  break;
	}

      levels_update (l_tool, OUTPUT_SLIDERS | DRAW);
    }

  return FALSE;
}

static void
levels_input_adjust_by_color (Levels               *levels,
			      guint                 value,
			      GimpHistogramChannel  channel,
			      guchar               *color)
{
  switch (value & 0xF)
    {
    case LOW_INPUT:
      levels_adjust_by_colors (levels, channel, color, NULL, NULL);
      break;
    case GAMMA:
      levels_adjust_by_colors (levels, channel, NULL, color, NULL);
      break;
    case HIGH_INPUT:
      levels_adjust_by_colors (levels, channel, NULL, NULL, color);
      break;
    default:
      break;
    }
}

static void
gimp_levels_tool_color_picked (GimpColorTool *color_tool,
                               GimpImageType  sample_type,
                               GimpRGB       *color,
                               gint           color_index)
{
  GimpLevelsTool *l_tool;
  guchar          col[5];
  guint           value;

  l_tool = GIMP_LEVELS_TOOL (color_tool);

  gimp_rgba_get_uchar (color,
                       col + RED_PIX,
                       col + GREEN_PIX,
                       col + BLUE_PIX,
                       col + ALPHA_PIX);

  value = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (l_tool->active_picker),
					       "pick_value"));

  if (value & ALL_CHANNELS && GIMP_IMAGE_TYPE_IS_RGB (sample_type))
    {
      GimpHistogramChannel  channel;

      /*  first reset the value channel  */
      switch (value & 0xF)
	{
	case LOW_INPUT:
	  l_tool->levels->low_input[GIMP_HISTOGRAM_VALUE] = 0;
	  break;
	case GAMMA:
	  l_tool->levels->gamma[GIMP_HISTOGRAM_VALUE] = 1.0;
	  break;
	case HIGH_INPUT:
	  l_tool->levels->high_input[GIMP_HISTOGRAM_VALUE] = 255;
	  break;
	default:
	  break;
	}

      /*  then adjust all color channels  */
      for (channel = GIMP_HISTOGRAM_RED;
	   channel <= GIMP_HISTOGRAM_BLUE;
	   channel++)
	{
	  levels_input_adjust_by_color (l_tool->levels,
                                        value, channel, col);
	}
    }
  else
    {
      levels_input_adjust_by_color (l_tool->levels,
                                    value, l_tool->channel, col);
    }

  levels_update (l_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));
}

static void
levels_load_callback (GtkWidget      *widget,
		      GimpLevelsTool *l_tool)
{
  if (! l_tool->file_dialog)
    file_dialog_create (l_tool);

  if (GTK_WIDGET_VISIBLE (l_tool->file_dialog))
    {
      gtk_window_present (GTK_WINDOW (l_tool->file_dialog));
      return;
    }

  l_tool->is_save = FALSE;

  gtk_window_set_title (GTK_WINDOW (l_tool->file_dialog), _("Load Levels"));
  gtk_widget_show (l_tool->file_dialog);
}

static void
levels_save_callback (GtkWidget      *widget,
		      GimpLevelsTool *l_tool)
{
  if (! l_tool->file_dialog)
    file_dialog_create (l_tool);

  if (GTK_WIDGET_VISIBLE (l_tool->file_dialog))
    {
      gtk_window_present (GTK_WINDOW (l_tool->file_dialog));
      return;
    }

  l_tool->is_save = TRUE;

  gtk_window_set_title (GTK_WINDOW (l_tool->file_dialog), _("Save Levels"));
  gtk_widget_show (l_tool->file_dialog);
}

static void
file_dialog_create (GimpLevelsTool *l_tool)
{
  GtkFileSelection *file_dlg;
  gchar            *temp;

  l_tool->file_dialog = gtk_file_selection_new ("");

  file_dlg = GTK_FILE_SELECTION (l_tool->file_dialog);

  gtk_window_set_role (GTK_WINDOW (file_dlg), "gimp-load-save-levels");
  gtk_window_set_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (file_dlg), 6);
  gtk_container_set_border_width (GTK_CONTAINER (file_dlg->button_area), 4);

  g_object_add_weak_pointer (G_OBJECT (file_dlg),
                             (gpointer) &l_tool->file_dialog);

  gtk_window_set_transient_for (GTK_WINDOW (file_dlg),
                                GTK_WINDOW (GIMP_IMAGE_MAP_TOOL (l_tool)->shell));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (file_dlg), TRUE);

  g_signal_connect (file_dlg, "response",
                    G_CALLBACK (file_dialog_response),
                    l_tool);

  temp = g_build_filename (gimp_directory (), "levels", ".", NULL);
  gtk_file_selection_set_filename (file_dlg, temp);
  g_free (temp);

  gimp_help_connect (l_tool->file_dialog, gimp_standard_help_func,
                     GIMP_TOOL (l_tool)->tool_info->help_id, NULL);
}

static void
file_dialog_response (GtkWidget      *dialog,
                      gint            response_id,
                      GimpLevelsTool *l_tool)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *filename;
      FILE        *file;

      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      file = fopen (filename, l_tool->is_save ? "wt" : "rt");

      if (! file)
        {
          g_message (l_tool->is_save ?
                     _("Could not open '%s' for writing: %s") :
                     _("Could not open '%s' for reading: %s"),
                     filename, g_strerror (errno));
          return;
        }

      if (l_tool->is_save)
        {
          levels_write_to_file (l_tool, file);
        }
      else if (! levels_read_from_file (l_tool, file))
        {
          g_message (("Error in reading file '%s'."), filename);
        }

      fclose (file);
    }

  gtk_widget_destroy (dialog);
}

static gboolean
levels_read_from_file (GimpLevelsTool *l_tool,
                       FILE           *file)
{
  gint    low_input[5];
  gint    high_input[5];
  gint    low_output[5];
  gint    high_output[5];
  gdouble gamma[5];
  gint    i, fields;
  gchar   buf[50], *nptr;

  if (! fgets (buf, 50, file))
    return FALSE;

  if (strcmp (buf, "# GIMP Levels File\n") != 0)
    return FALSE;

  for (i = 0; i < 5; i++)
    {
      fields = fscanf (file, "%d %d %d %d ",
		       &low_input[i],
		       &high_input[i],
		       &low_output[i],
		       &high_output[i]);

      if (fields != 4)
	return FALSE;

      if (! fgets (buf, 50, file))
	return FALSE;

      gamma[i] = g_ascii_strtod (buf, &nptr);

      if (buf == nptr || errno == ERANGE)
	return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      l_tool->levels->low_input[i]   = low_input[i];
      l_tool->levels->high_input[i]  = high_input[i];
      l_tool->levels->low_output[i]  = low_output[i];
      l_tool->levels->high_output[i] = high_output[i];
      l_tool->levels->gamma[i]       = gamma[i];
    }

  levels_update (l_tool, ALL);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (l_tool));

  return TRUE;
}

static void
levels_write_to_file (GimpLevelsTool *l_tool,
                      FILE           *file)
{
  gint  i;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

  fprintf (file, "# GIMP Levels File\n");

  for (i = 0; i < 5; i++)
    {
      fprintf (file, "%d %d %d %d %s\n",
	       l_tool->levels->low_input[i],
	       l_tool->levels->high_input[i],
	       l_tool->levels->low_output[i],
	       l_tool->levels->high_output[i],
               g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%f",
                                l_tool->levels->gamma[i]));
    }
}
