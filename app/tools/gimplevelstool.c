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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "appenv.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "gdisplay.h"
#include "histogramwidget.h"
#include "gimphistogram.h"
#include "gimpui.h"
#include "image_map.h"
#include "levels.h"
#include "gimplut.h"
#include "lut_funcs.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimpenv.h"

#define LOW_INPUT          0x1
#define GAMMA              0x2
#define HIGH_INPUT         0x4
#define LOW_OUTPUT         0x8
#define HIGH_OUTPUT        0x10
#define INPUT_LEVELS       0x20
#define OUTPUT_LEVELS      0x40
#define INPUT_SLIDERS      0x80
#define OUTPUT_SLIDERS     0x100
#define DRAW               0x200
#define ALL                0xFFF

#define DA_WIDTH         256
#define DA_HEIGHT        25
#define GRADIENT_HEIGHT  15
#define CONTROL_HEIGHT   DA_HEIGHT - GRADIENT_HEIGHT
#define HISTOGRAM_WIDTH  256
#define HISTOGRAM_HEIGHT 150

#define LEVELS_DA_MASK  GDK_EXPOSURE_MASK | \
                        GDK_ENTER_NOTIFY_MASK | \
			GDK_BUTTON_PRESS_MASK | \
			GDK_BUTTON_RELEASE_MASK | \
			GDK_BUTTON1_MOTION_MASK | \
			GDK_POINTER_MOTION_HINT_MASK

/*  the levels structures  */

typedef struct _Levels Levels;

struct _Levels
{
  gint x, y;    /*  coords for last mouse click  */
};

typedef struct _LevelsDialog LevelsDialog;

struct _LevelsDialog
{
  GtkWidget       *shell;

  GtkAdjustment   *low_input_data;
  GtkAdjustment   *gamma_data;
  GtkAdjustment   *high_input_data;
  GtkAdjustment   *low_output_data;
  GtkAdjustment   *high_output_data;

  GtkWidget       *input_levels_da[2];
  GtkWidget       *output_levels_da[2];
  GtkWidget       *channel_menu;

  HistogramWidget *histogram;
  GimpHistogram   *hist;

  GimpDrawable   *drawable;
  ImageMap        image_map;

  gint            color;
  gint            channel;
  gint            low_input[5];
  gdouble         gamma[5];
  gint            high_input[5];
  gint            low_output[5];
  gint            high_output[5];
  gboolean        preview;

  gint            active_slider;
  gint            slider_pos[5];  /*  positions for the five sliders  */

  guchar          input[5][256]; /* this is used only by the gui */

  GimpLut        *lut;
};

/*  the levels tool options  */
static ToolOptions *levels_options = NULL;

/*  the levels tool dialog  */
static LevelsDialog *levels_dialog = NULL;

/*  the levels file dialog  */
static GtkWidget *file_dlg = NULL;
static gboolean   load_save;

/*  levels action functions  */
static void   levels_control (Tool *, ToolAction, gpointer);

static LevelsDialog * levels_dialog_new (void);

static void   levels_calculate_transfers           (LevelsDialog *);
static void   levels_update                        (LevelsDialog *, gint);
static void   levels_preview                       (LevelsDialog *);
static void   levels_value_callback                (GtkWidget *, gpointer);
static void   levels_red_callback                  (GtkWidget *, gpointer);
static void   levels_green_callback                (GtkWidget *, gpointer);
static void   levels_blue_callback                 (GtkWidget *, gpointer);
static void   levels_alpha_callback                (GtkWidget *, gpointer);
static void   levels_reset_callback                (GtkWidget *, gpointer);
static void   levels_ok_callback                   (GtkWidget *, gpointer);
static void   levels_cancel_callback               (GtkWidget *, gpointer);
static void   levels_auto_callback                 (GtkWidget *, gpointer);
static void   levels_load_callback                 (GtkWidget *, gpointer);
static void   levels_save_callback                 (GtkWidget *, gpointer);
static void   levels_preview_update                (GtkWidget *, gpointer);
static void   levels_low_input_adjustment_update   (GtkAdjustment *, gpointer);
static void   levels_gamma_adjustment_update       (GtkAdjustment *, gpointer);
static void   levels_high_input_adjustment_update  (GtkAdjustment *, gpointer);
static void   levels_low_output_adjustment_update  (GtkAdjustment *, gpointer);
static void   levels_high_output_adjustment_update (GtkAdjustment *, gpointer);
static gint   levels_input_da_events               (GtkWidget *, GdkEvent *,
						    LevelsDialog *);
static gint   levels_output_da_events              (GtkWidget *, GdkEvent *,
						    LevelsDialog *);

static void   levels_histogram_range               (HistogramWidget *,
						    gint, gint, void *);
static void   make_file_dlg                        (gpointer);
static void   file_ok_callback                     (GtkWidget *, gpointer);
static void   file_cancel_callback                 (GtkWidget *, gpointer);

static gboolean  read_levels_from_file             (FILE *f);
static void      write_levels_to_file              (FILE *f);

static void
levels_histogram_range (HistogramWidget *hw,
			gint             start,
			gint             end,
			void            *data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  histogram_widget_range (ld->histogram, -1, -1);
}

/*  levels action functions  */

static void
levels_control (Tool       *tool,
		ToolAction  action,
		gpointer    gdisp_ptr)
{
  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      break;

    case HALT:
      if (levels_dialog)
	levels_cancel_callback (NULL, (gpointer) levels_dialog);
      break;

    default:
      break;
    }
}

Tool *
tools_new_levels (void)
{
  Tool * tool;
  Levels * private;

  /*  The tool options  */
  if (! levels_options)
    {
      levels_options = tool_options_new (_("Levels Options"));
      tools_register (LEVELS, levels_options);
    }

  tool = tools_new_tool (LEVELS);
  private = g_new (Levels, 1);

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->control_func = levels_control;

  return tool;
}

void
tools_free_levels (Tool *tool)
{
  Levels * private;

  private = (Levels *) tool->private;

  /*  Close the color select dialog  */
  if (levels_dialog)
    levels_cancel_callback (NULL, (gpointer) levels_dialog);

  g_free (private);
}

static MenuItem color_option_items[] =
{
  { N_("Value"), 0, 0, levels_value_callback, NULL, NULL, NULL },
  { N_("Red"), 0, 0, levels_red_callback, NULL, NULL, NULL },
  { N_("Green"), 0, 0, levels_green_callback, NULL, NULL, NULL },
  { N_("Blue"), 0, 0, levels_blue_callback, NULL, NULL, NULL },
  { N_("Alpha"), 0, 0, levels_alpha_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

void
levels_initialize (GDisplay *gdisp)
{
  gint i;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message (_("Levels for indexed drawables cannot be adjusted."));
      return;
    }

  /*  The levels dialog  */
  if (!levels_dialog)
    levels_dialog = levels_dialog_new ();
  else
    if (!GTK_WIDGET_VISIBLE (levels_dialog->shell))
      gtk_widget_show (levels_dialog->shell);

  /*  Initialize the values  */
  levels_dialog->channel = HISTOGRAM_VALUE;
  for (i = 0; i < 5; i++)
    {
      levels_dialog->low_input[i]   = 0;
      levels_dialog->gamma[i]       = 1.0;
      levels_dialog->high_input[i]  = 255;
      levels_dialog->low_output[i]  = 0;
      levels_dialog->high_output[i] = 255;
    }

  levels_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  levels_dialog->color = drawable_color (levels_dialog->drawable);
  levels_dialog->image_map = image_map_create (gdisp, levels_dialog->drawable);

  /* check for alpha channel */
  if (drawable_has_alpha (levels_dialog->drawable))
    gtk_widget_set_sensitive (color_option_items[4].widget, TRUE);
  else 
    gtk_widget_set_sensitive (color_option_items[4].widget, FALSE);
  
  /*  hide or show the channel menu based on image type  */
  if (levels_dialog->color)
    for (i = 0; i < 4; i++) 
       gtk_widget_set_sensitive (color_option_items[i].widget, TRUE);
  else 
    for (i = 1; i < 4; i++) 
       gtk_widget_set_sensitive (color_option_items[i].widget, FALSE);

  /* set the current selection */
  gtk_option_menu_set_history (GTK_OPTION_MENU (levels_dialog->channel_menu), 0);


  levels_update (levels_dialog, LOW_INPUT | GAMMA | HIGH_INPUT | LOW_OUTPUT | HIGH_OUTPUT | DRAW);
  levels_update (levels_dialog, INPUT_LEVELS | OUTPUT_LEVELS);

  gimp_histogram_calculate_drawable (levels_dialog->hist,
				     levels_dialog->drawable);
  histogram_widget_update (levels_dialog->histogram, levels_dialog->hist);
  histogram_widget_range (levels_dialog->histogram, -1, -1);
}

void
levels_free (void)
{
  if (levels_dialog)
    {
      if (levels_dialog->image_map)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (levels_dialog->image_map);
	  active_tool->preserve = FALSE;

	  levels_dialog->image_map = NULL;
	}
      gtk_widget_destroy (levels_dialog->shell);
    }
}

/*******************/
/*  Levels dialog  */
/*******************/

static LevelsDialog *
levels_dialog_new (void)
{
  LevelsDialog *ld;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *channel_hbox;
  GtkWidget *menu;
  GtkWidget *hbbox;
  GtkWidget *button;
  GtkWidget *spinbutton;
  GtkObject *data;
  gint i;

  ld = g_new (LevelsDialog, 1);
  ld->preview = TRUE;
  ld->lut     = gimp_lut_new ();
  ld->hist    = gimp_histogram_new ();

  for (i = 0; i < 5; i++)
    color_option_items [i].user_data = (gpointer) ld;

  /*  The shell and main vbox  */
  ld->shell = gimp_dialog_new (_("Levels"), "levels",
			       tools_help_func, NULL,
			       GTK_WIN_POS_NONE,
			       FALSE, TRUE, FALSE,

			       _("OK"), levels_ok_callback,
			       ld, NULL, TRUE, FALSE,
			       _("Reset"), levels_reset_callback,
			       ld, NULL, FALSE, FALSE,
			       _("Cancel"), levels_cancel_callback,
			       ld, NULL, FALSE, TRUE,

			       NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (ld->shell)->vbox), main_vbox);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Modify Levels for Channel:"));
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);

  menu = build_menu (color_option_items, NULL);
  ld->channel_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (channel_hbox), ld->channel_menu, FALSE, FALSE, 0);

  gtk_widget_show (label);
  gtk_widget_show (ld->channel_menu);
  gtk_widget_show (channel_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ld->channel_menu), menu);

  /*  Horizontal box for input levels spinbuttons  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Input Levels:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low input spin  */
  data = gtk_adjustment_new (0, 0, 255, 1, 10, 10);
  ld->low_input_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (ld->low_input_data, 0.5, 0);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->low_input_data), "value_changed",
                      GTK_SIGNAL_FUNC (levels_low_input_adjustment_update),
                      ld);

  gtk_widget_show (spinbutton);

  /*  input gamma spin  */
  data = gtk_adjustment_new (1, 0.1, 10, 0.1, 1, 1);
  ld->gamma_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (ld->gamma_data, 0.5, 2);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->gamma_data), "value_changed",
		      GTK_SIGNAL_FUNC (levels_gamma_adjustment_update),
		      ld);

  gtk_widget_show (spinbutton);

  /*  high input spin  */
  data = gtk_adjustment_new (255, 0, 255, 1, 10, 10);
  ld->high_input_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (ld->high_input_data, 0.5, 0);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->high_input_data), "value_changed",
		      GTK_SIGNAL_FUNC (levels_high_input_adjustment_update),
		      ld);

  gtk_widget_show (spinbutton);
  gtk_widget_show (hbox);

  /*  The levels histogram  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);
  ld->histogram = histogram_widget_new (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (ld->histogram));

  gtk_signal_connect (GTK_OBJECT (ld->histogram), "rangechanged",
		      GTK_SIGNAL_FUNC (levels_histogram_range),
		      ld);

  gtk_widget_show (GTK_WIDGET (ld->histogram));
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  The input levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  ld->input_levels_da[0] = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (ld->input_levels_da[0]),
		    DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[0], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[0], FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[0]), "event",
		      GTK_SIGNAL_FUNC (levels_input_da_events),
		      ld);

  ld->input_levels_da[1] = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (ld->input_levels_da[1]),
			 DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[1], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[1], FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[1]), "event",
		      GTK_SIGNAL_FUNC (levels_input_da_events),
		      ld);

  gtk_widget_show (ld->input_levels_da[0]);
  gtk_widget_show (ld->input_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for levels spin widgets  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Output Levels:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low output spin  */
  data = gtk_adjustment_new (0, 0, 255, 1, 10, 10);
  ld->low_output_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (ld->low_output_data, 0.5, 0);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->low_output_data), "value_changed",
                      GTK_SIGNAL_FUNC (levels_low_output_adjustment_update),
                      ld);

  gtk_widget_show (spinbutton);

  /*  high output spin  */
  data = gtk_adjustment_new (255, 0, 255, 1, 10, 10);
  ld->high_output_data = GTK_ADJUSTMENT (data);

  spinbutton = gtk_spin_button_new (ld->high_output_data, 0.5, 0);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->high_output_data), "value_changed",
                      GTK_SIGNAL_FUNC (levels_high_output_adjustment_update),
                      ld);

  gtk_widget_show (spinbutton);
  gtk_widget_show (hbox);

  /*  The output levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);

  ld->output_levels_da[0] = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[0], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[0], FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[0]), "event",
		      GTK_SIGNAL_FUNC (levels_output_da_events),
		      ld);

  ld->output_levels_da[1] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[1], LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[1], FALSE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[1]), "event",
		      GTK_SIGNAL_FUNC (levels_output_da_events),
		      ld);

  gtk_widget_show (ld->output_levels_da[0]);
  gtk_widget_show (ld->output_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  /*  The preview toggle  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  toggle = gtk_check_button_new_with_label (_("Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), ld->preview);
  gtk_box_pack_end (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      GTK_SIGNAL_FUNC (levels_preview_update),
		      ld);

  gtk_widget_show (toggle);

  gtk_widget_show (hbox);

  /*  Horizontal button box for auto / load / save  */
  hbbox = gtk_hbutton_box_new ();
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbbox), GTK_BUTTONBOX_SPREAD);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Auto"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (levels_auto_callback),
		      ld);

  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Load"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (levels_load_callback),
		      NULL);

  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Save"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (levels_save_callback),
		      NULL);

  gtk_widget_show (button);

  gtk_widget_show (hbbox);

  gtk_widget_show (main_vbox);
  gtk_widget_show (ld->shell);

  return ld;
}

static void
levels_draw_slider (GdkWindow *window,
		    GdkGC     *border_gc,
		    GdkGC     *fill_gc,
		    gint       xpos)
{
  int y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line(window, fill_gc, xpos - y / 2, y,
		  xpos + y / 2, y);

  gdk_draw_line (window, border_gc, xpos, 0,
		 xpos - (CONTROL_HEIGHT - 1) / 2,  CONTROL_HEIGHT - 1);

  gdk_draw_line (window, border_gc, xpos, 0,
		 xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);

  gdk_draw_line (window, border_gc, xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1,
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
levels_calculate_transfers (LevelsDialog *ld)
{
  gdouble inten;
  gint i, j;

  /*  Recalculate the levels arrays  */
  for (j = 0; j < 5; j++)
    {
      for (i = 0; i < 256; i++)
	{
	  /*  determine input intensity  */
	  if (ld->high_input[j] != ld->low_input[j])
	    inten = (double) (i - ld->low_input[j]) /
	      (double) (ld->high_input[j] - ld->low_input[j]);
	  else
	    inten = (double) (i - ld->low_input[j]);

	  inten = BOUNDS (inten, 0.0, 1.0);
	  if (ld->gamma[j] != 0.0)
	    inten = pow (inten, (1.0 / ld->gamma[j]));
	  ld->input[j][i] = (unsigned char) (inten * 255.0 + 0.5);
	}
    }
}

static void
levels_update (LevelsDialog *ld,
	       gint          update)
{
  gint i;

  /*  Recalculate the transfer arrays  */
  levels_calculate_transfers (ld);
  /* set up the lut */
  levels_lut_setup (ld->lut, ld->gamma, ld->low_input, ld->high_input,
		    ld->low_output, ld->high_output,
		    gimp_drawable_bytes(ld->drawable));

  if (update & LOW_INPUT)
    {
      gtk_adjustment_set_value (ld->low_input_data,
				ld->low_input[ld->channel]);
    }
  if (update & GAMMA)
    {
      gtk_adjustment_set_value (ld->gamma_data,
				ld->gamma[ld->channel]);
    }
  if (update & HIGH_INPUT)
    {
      gtk_adjustment_set_value (ld->high_input_data,
				ld->high_input[ld->channel]);
    }
  if (update & LOW_OUTPUT)
    {
      gtk_adjustment_set_value (ld->low_output_data,
				ld->low_output[ld->channel]);
    }
  if (update & HIGH_OUTPUT)
    {
      gtk_adjustment_set_value (ld->high_output_data,
				ld->high_output[ld->channel]);
    }
  if (update & INPUT_LEVELS)
    {
      guchar buf[DA_WIDTH*3];

      switch (ld->channel)
	{
	default:
	  g_warning ("unknown channel type, can't happen\n");
	  /* fall through */
	case HISTOGRAM_VALUE:
	case HISTOGRAM_ALPHA:
	  for (i = 0; i < DA_WIDTH; i++)
	    {
	      buf[3*i+0] = ld->input[ld->channel][i];
	      buf[3*i+1] = ld->input[ld->channel][i];
	      buf[3*i+2] = ld->input[ld->channel][i];
	    }
	  break;

	case HISTOGRAM_RED:
	case HISTOGRAM_GREEN:
	case HISTOGRAM_BLUE:	  
	  for (i = 0; i < DA_WIDTH; i++)
	    {
	      buf[3*i+0] = ld->input[HISTOGRAM_RED][i];
	      buf[3*i+1] = ld->input[HISTOGRAM_GREEN][i];
	      buf[3*i+2] = ld->input[HISTOGRAM_BLUE][i];
	    }
	  break;
	}

      for (i = 0; i < GRADIENT_HEIGHT/2; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->input_levels_da[0]),
			      buf, 0, i, DA_WIDTH);


      for (i = 0; i < DA_WIDTH; i++)
	{
	  buf[3*i+0] = i;
	  buf[3*i+1] = i;
	  buf[3*i+2] = i;
	}

      for (i = GRADIENT_HEIGHT/2; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->input_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->input_levels_da[0], NULL);
    }
  if (update & OUTPUT_LEVELS)
    {
      guchar buf[DA_WIDTH*3];
      guchar r, g, b;

      r = g = b = 0;
      switch (ld->channel)
	{
	default:
	  g_warning ("unknown channel type, can't happen\n");
	  /* fall through */
	case HISTOGRAM_VALUE:
	case HISTOGRAM_ALPHA:  r = g = b = 1; break;
	case HISTOGRAM_RED:    r = 1;         break;
	case HISTOGRAM_GREEN:  g = 1;         break;
	case HISTOGRAM_BLUE:   b = 1;         break;
	}

      for (i = 0; i < DA_WIDTH; i++)
	{
	  buf[3*i+0] = i*r;
	  buf[3*i+1] = i*g;
	  buf[3*i+2] = i*b;
	}

      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->output_levels_da[0]),
			      buf, 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->output_levels_da[0], NULL);
    }
  if (update & INPUT_SLIDERS)
    {
      double width, mid, tmp;

      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[0]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[1]);
      levels_erase_slider (ld->input_levels_da[1]->window, ld->slider_pos[2]);

      ld->slider_pos[0] = DA_WIDTH * ((double) ld->low_input[ld->channel] / 255.0);
      ld->slider_pos[2] = DA_WIDTH * ((double) ld->high_input[ld->channel] / 255.0);

      width = (double) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
      mid = ld->slider_pos[0] + width;
      tmp = log10 (1.0 / ld->gamma[ld->channel]);
      ld->slider_pos[1] = (int) (mid + width * tmp + 0.5);

      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->dark_gc[GTK_STATE_NORMAL],
			  ld->slider_pos[1]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->slider_pos[0]);
      levels_draw_slider (ld->input_levels_da[1]->window,
			  ld->input_levels_da[1]->style->black_gc,
			  ld->input_levels_da[1]->style->white_gc,
			  ld->slider_pos[2]);
    }
  if (update & OUTPUT_SLIDERS)
    {
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[3]);
      levels_erase_slider (ld->output_levels_da[1]->window, ld->slider_pos[4]);

      ld->slider_pos[3] = DA_WIDTH * ((double) ld->low_output[ld->channel] / 255.0);
      ld->slider_pos[4] = DA_WIDTH * ((double) ld->high_output[ld->channel] / 255.0);

      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->slider_pos[3]);
      levels_draw_slider (ld->output_levels_da[1]->window,
			  ld->output_levels_da[1]->style->black_gc,
			  ld->output_levels_da[1]->style->white_gc,
			  ld->slider_pos[4]);
    }
}

static void
levels_preview (LevelsDialog *ld)
{
  if (!ld->image_map)
    {
      g_warning ("No image map");
      return;
    }

  active_tool->preserve = TRUE;
  image_map_apply (ld->image_map,  (ImageMapApplyFunc)gimp_lut_process_2,
		   (void *) ld->lut);
  active_tool->preserve = FALSE;
}

static void
levels_value_callback (GtkWidget *widget,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->channel != HISTOGRAM_VALUE)
    {
      ld->channel = HISTOGRAM_VALUE;
      histogram_widget_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_red_callback (GtkWidget *widget,
		     gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->channel != HISTOGRAM_RED)
    {
      ld->channel = HISTOGRAM_RED;
      histogram_widget_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_green_callback (GtkWidget *widget,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->channel != HISTOGRAM_GREEN)
    {
      ld->channel = HISTOGRAM_GREEN;
      histogram_widget_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_blue_callback (GtkWidget *widget,
		      gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->channel != HISTOGRAM_BLUE)
    {
      ld->channel = HISTOGRAM_BLUE;
      histogram_widget_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_alpha_callback (GtkWidget *widget,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->channel != HISTOGRAM_ALPHA)
    {
      ld->channel = HISTOGRAM_ALPHA;
      histogram_widget_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_adjust_channel (LevelsDialog    *ld,
		       GimpHistogram   *hist,
		       int              channel)
{
  int i;
  double count, new_count, percentage, next_percentage;

  ld->gamma[channel]       = 1.0;
  ld->low_output[channel]  = 0;
  ld->high_output[channel] = 255;

  count = gimp_histogram_get_count (hist, 0, 255);

  if (count == 0.0)
    {
      ld->low_input[channel] = 0;
      ld->high_input[channel] = 0;
    }
  else
    {
      /*  Set the low input  */
      new_count = 0.0;
      for (i = 0; i < 255; i++)
	{
	  new_count += gimp_histogram_get_value(hist, channel, i);
	  percentage = new_count / count;
	  next_percentage =
	    (new_count + gimp_histogram_get_value (hist,
						   channel,
						   i + 1)) / count;
	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      ld->low_input[channel] = i + 1;
	      break;
	    }
	}
      /*  Set the high input  */
      new_count = 0.0;
      for (i = 255; i > 0; i--)
	{
	  new_count += gimp_histogram_get_value(hist, channel, i);
	  percentage = new_count / count;
	  next_percentage =
	    (new_count + gimp_histogram_get_value (hist,
						   channel,
						   i - 1)) / count;
	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      ld->high_input[channel] = i - 1;
	      break;
	    }
	}
    }
}

static void
levels_reset_callback (GtkWidget *widget,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  ld->low_input[ld->channel]   = 0;
  ld->gamma[ld->channel]       = 1.0;
  ld->high_input[ld->channel]  = 255;
  ld->low_output[ld->channel]  = 0;
  ld->high_output[ld->channel] = 255;

  levels_update (ld, ALL);

  if (ld->preview)
    levels_preview (ld);
}

static void
levels_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  active_tool->preserve = TRUE;

  if (!ld->preview)
    {
      levels_lut_setup (ld->lut, ld->gamma, ld->low_input, ld->high_input,
			ld->low_output, ld->high_output,
			gimp_drawable_bytes (ld->drawable));
      image_map_apply (ld->image_map, (ImageMapApplyFunc) gimp_lut_process_2,
		       (void *) ld->lut);
    }

  if (ld->image_map)
    image_map_commit (ld->image_map);

  active_tool->preserve = FALSE;

  ld->image_map = NULL;

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
levels_cancel_callback (GtkWidget *widget,
			gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  if (ld->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (ld->image_map);
      active_tool->preserve = FALSE;

      gdisplays_flush ();
      ld->image_map = NULL;
    }

  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
}

static void
levels_auto_callback (GtkWidget *widget,
		      gpointer   data)
{
  LevelsDialog *ld;
  int channel;

  ld = (LevelsDialog *) data;

  if (ld->color)
    {
      /*  Set the overall value to defaults  */
      ld->low_input[HISTOGRAM_VALUE]   = 0;
      ld->gamma[HISTOGRAM_VALUE]       = 1.0;
      ld->high_input[HISTOGRAM_VALUE]  = 255;
      ld->low_output[HISTOGRAM_VALUE]  = 0;
      ld->high_output[HISTOGRAM_VALUE] = 255;

      for (channel = 0; channel < 3; channel ++)
	levels_adjust_channel (ld, ld->hist, channel + 1);
    }
  else
    levels_adjust_channel (ld, ld->hist, HISTOGRAM_VALUE);

  levels_update (ld, ALL);
  if (ld->preview)
    levels_preview (ld);
}

static void
levels_load_callback (GtkWidget *widget,
		      gpointer   data)
{
  if (!file_dlg)
    make_file_dlg (NULL);
  else if (GTK_WIDGET_VISIBLE (file_dlg)) 
    return;

  load_save = TRUE;

  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Load Levels"));
  gtk_widget_show (file_dlg);
}

static void
levels_save_callback (GtkWidget *widget,
		      gpointer   data)
{
  if (!file_dlg)
    make_file_dlg (NULL);
  else if (GTK_WIDGET_VISIBLE (file_dlg)) 
    return;

  load_save = FALSE;

  gtk_window_set_title (GTK_WINDOW (file_dlg), _("Save Levels"));
  gtk_widget_show (file_dlg);
}

static void
levels_preview_update (GtkWidget *widget,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      ld->preview = TRUE;
      levels_preview (ld);
    }
  else
    ld->preview = FALSE;
}

static void
levels_low_input_adjustment_update (GtkAdjustment *adjustment,
				    gpointer       data)
{
  LevelsDialog *ld;
  gint value;

  ld = (LevelsDialog *) data;

  value = (gint) (adjustment->value + 0.5);
  value = BOUNDS (value, 0, ld->high_input[ld->channel]);

  /*  enforce a consistent displayed value (low_input <= high_input)  */
  gtk_adjustment_set_value (adjustment, value);

  if (ld->low_input[ld->channel] != value)
    {
      ld->low_input[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_gamma_adjustment_update (GtkAdjustment *adjustment,
				gpointer       data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (ld->gamma[ld->channel] != adjustment->value)
    {
      ld->gamma[ld->channel] = adjustment->value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_input_adjustment_update (GtkAdjustment *adjustment,
				     gpointer       data)
{
  LevelsDialog *ld;
  gint value;

  ld = (LevelsDialog *) data;

  value = (gint) (adjustment->value + 0.5);
  value = BOUNDS (value, ld->low_input[ld->channel], 255);

  /*  enforce a consistent displayed value (high_input >= low_input)  */
  gtk_adjustment_set_value (adjustment, value);

  if (ld->high_input[ld->channel] != value)
    {
      ld->high_input[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_low_output_adjustment_update (GtkAdjustment *adjustment,
				     gpointer       data)
{
  LevelsDialog *ld;
  gint value;

  ld = (LevelsDialog *) data;

  value = (gint) (adjustment->value + 0.5);

  if (ld->low_output[ld->channel] != value)
    {
      ld->low_output[ld->channel] = value;
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_output_adjustment_update (GtkAdjustment *adjustment,
				      gpointer       data)
{
  LevelsDialog *ld;
  gint value;

  ld = (LevelsDialog *) data;

  value = (gint) (adjustment->value + 0.5);

  if (ld->high_output[ld->channel] != value)
    {
      ld->high_output[ld->channel] = value;
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static gint
levels_input_da_events (GtkWidget    *widget,
			GdkEvent     *event,
			LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gchar text[12];
  gdouble width, mid, tmp;
  gint x, distance;
  gint i;
  gint update = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->input_levels_da[1])
	levels_update (ld, INPUT_SLIDERS);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 3; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  levels_update (ld, LOW_INPUT | GAMMA | DRAW);
	  break;
	case 1:  /*  gamma  */
	  levels_update (ld, GAMMA);
	  break;
	case 2:  /*  high input  */
	  levels_update (ld, HIGH_INPUT | GAMMA | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
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
      switch (ld->active_slider)
	{
	case 0:  /*  low input  */
	  ld->low_input[ld->channel] = ((double) x / (double) DA_WIDTH) * 255.0;
	  ld->low_input[ld->channel] = BOUNDS (ld->low_input[ld->channel], 0,
					       ld->high_input[ld->channel]);
	  break;

	case 1:  /*  gamma  */
	  width = (double) (ld->slider_pos[2] - ld->slider_pos[0]) / 2.0;
	  mid = ld->slider_pos[0] + width;

	  x = BOUNDS (x, ld->slider_pos[0], ld->slider_pos[2]);
	  tmp = (double) (x - mid) / width;
	  ld->gamma[ld->channel] = 1.0 / pow (10, tmp);

	  /*  round the gamma value to the nearest 1/100th  */
	  sprintf (text, "%2.2f", ld->gamma[ld->channel]);
	  ld->gamma[ld->channel] = atof (text);
	  break;

	case 2:  /*  high input  */
	  ld->high_input[ld->channel] = ((double) x / (double) DA_WIDTH) * 255.0;
	  ld->high_input[ld->channel] = BOUNDS (ld->high_input[ld->channel],
						ld->low_input[ld->channel], 255);
	  break;
	}

      levels_update (ld, INPUT_SLIDERS | INPUT_LEVELS | DRAW);
    }

  return FALSE;
}

static gint
levels_output_da_events (GtkWidget    *widget,
			 GdkEvent     *event,
			 LevelsDialog *ld)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int x, distance;
  int i;
  int update = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (widget == ld->output_levels_da[1])
	levels_update (ld, OUTPUT_SLIDERS);
      break;


    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 3; i < 5; i++)
	if (fabs (bevent->x - ld->slider_pos[i]) < distance)
	  {
	    ld->active_slider = i;
	    distance = fabs (bevent->x - ld->slider_pos[i]);
	  }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  levels_update (ld, LOW_OUTPUT | DRAW);
	  break;
	case 4:  /*  high output  */
	  levels_update (ld, HIGH_OUTPUT | DRAW);
	  break;
	}

      if (ld->preview)
	levels_preview (ld);
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
      switch (ld->active_slider)
	{
	case 3:  /*  low output  */
	  ld->low_output[ld->channel] = ((double) x / (double) DA_WIDTH) * 255.0;
	  ld->low_output[ld->channel] = BOUNDS (ld->low_output[ld->channel], 0, 255);
	  break;

	case 4:  /*  high output  */
	  ld->high_output[ld->channel] = ((double) x / (double) DA_WIDTH) * 255.0;
	  ld->high_output[ld->channel] = BOUNDS (ld->high_output[ld->channel], 0, 255);
	  break;
	}

      levels_update (ld, OUTPUT_SLIDERS | DRAW);
    }

  return FALSE;
}

static void
make_file_dlg (gpointer data)
{
  gchar *temp;

  file_dlg = gtk_file_selection_new (_("Load/Save Levels"));
  gtk_window_set_wmclass (GTK_WINDOW (file_dlg), "load_save_levels", "Gimp");
  gtk_window_set_position (GTK_WINDOW (file_dlg), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (file_dlg), 2);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (file_dlg)->button_area), 2);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->cancel_button),
		      "clicked", GTK_SIGNAL_FUNC (file_cancel_callback),
		      data);
  gtk_signal_connect (GTK_OBJECT (file_dlg), "delete_event",
		      GTK_SIGNAL_FUNC (file_cancel_callback),
		      data);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_dlg)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC (file_ok_callback),
		      data);

  temp = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "levels" G_DIR_SEPARATOR_S,
      			  gimp_directory ());
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_dlg), temp);
  g_free (temp);
}

static void
file_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  FILE  *f;
  gchar *filename;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_dlg));

  if (load_save)
    {
      f = fopen (filename, "rt");

      if (!f)
	{
	  g_message (_("Unable to open file %s"), filename);
	  return;
	}

      if (!read_levels_from_file (f))
	{
	  g_message (("Error in reading file %s"), filename);
	  return;
	}

      fclose (f);
    }
  else
    {
      f = fopen(filename, "wt");

      if (!f)
	{
	  g_message (_("Unable to open file %s"), filename);
	  return;
	}

      write_levels_to_file (f);

      fclose (f);
    }

  gtk_widget_hide (file_dlg);
}

static void
file_cancel_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_widget_hide (file_dlg);
}

static gboolean
read_levels_from_file (FILE *f)
{
  int low_input[5];
  int high_input[5];
  int low_output[5];
  int high_output[5];
  double gamma[5];
  int i, fields;
  char buf[50], *nptr;
  
  if (!fgets (buf, 50, f))
    return FALSE;

  if (strcmp (buf, "# GIMP Levels File\n") != 0)
    return FALSE;

  for (i = 0; i < 5; i++)
    {
      fields = fscanf (f, "%d %d %d %d ",
		       &low_input[i],
		       &high_input[i],
		       &low_output[i],
		       &high_output[i]);

      if (fields != 4)
	return FALSE;

      if (!fgets (buf, 50, f))
	return FALSE;

      gamma[i] = strtod(buf, &nptr);

      if (buf == nptr || errno == ERANGE)
	return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      levels_dialog->low_input[i] = low_input[i];
      levels_dialog->high_input[i] = high_input[i];
      levels_dialog->low_output[i] = low_output[i];
      levels_dialog->high_output[i] = high_output[i];
      levels_dialog->gamma[i] = gamma[i];
    }

  levels_update (levels_dialog, ALL);

  if (levels_dialog->preview)
    levels_preview (levels_dialog);

  return TRUE;
}

static void
write_levels_to_file (FILE *f)
{
  int i;

  fprintf (f, "# GIMP Levels File\n");

  for (i = 0; i < 5; i++)
    {
      fprintf (f, "%d %d %d %d %f\n",
	       levels_dialog->low_input[i],
	       levels_dialog->high_input[i],
	       levels_dialog->low_output[i],
	       levels_dialog->high_output[i],
	       levels_dialog->gamma[i]);
    }
}
