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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "drawable.h"
#include "general.h"
#include "gdisplay.h"
#include "histogram.h"
#include "image_map.h"
#include "interface.h"
#include "levels.h"

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

#define TEXT_WIDTH       45
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

typedef struct _Levels Levels;

struct _Levels
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _LevelsDialog LevelsDialog;

struct _LevelsDialog
{
  GtkWidget *    shell;
  GtkWidget *    low_input_text;
  GtkWidget *    gamma_text;
  GtkWidget *    high_input_text;
  GtkWidget *    low_output_text;
  GtkWidget *    high_output_text;
  GtkWidget *    input_levels_da[2];
  GtkWidget *    output_levels_da[2];
  GtkWidget *    channel_menu;
  Histogram *    histogram;

  GimpDrawable * drawable;
  ImageMap       image_map;
  int            color;
  int            channel;
  int            low_input[5];
  double         gamma[5];
  int            high_input[5];
  int            low_output[5];
  int            high_output[5];
  gint           preview;

  int            active_slider;
  int            slider_pos[5];  /*  positions for the five sliders  */

  unsigned char  input[5][256];
  unsigned char  output[5][256];

};

/*  levels action functions  */

static void   levels_button_press   (Tool *, GdkEventButton *, gpointer);
static void   levels_button_release (Tool *, GdkEventButton *, gpointer);
static void   levels_motion         (Tool *, GdkEventMotion *, gpointer);
static void   levels_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   levels_control        (Tool *, int, gpointer);

static LevelsDialog *  levels_new_dialog              (void);
static void            levels_calculate_transfers     (LevelsDialog *);
static void            levels_update                  (LevelsDialog *, int);
static void            levels_preview                 (LevelsDialog *);
static void            levels_value_callback          (GtkWidget *, gpointer);
static void            levels_red_callback            (GtkWidget *, gpointer);
static void            levels_green_callback          (GtkWidget *, gpointer);
static void            levels_blue_callback           (GtkWidget *, gpointer);
static void            levels_alpha_callback          (GtkWidget *, gpointer);
static void            levels_auto_levels_callback    (GtkWidget *, gpointer);
static void            levels_ok_callback             (GtkWidget *, gpointer);
static void            levels_cancel_callback         (GtkWidget *, gpointer);
static gint            levels_delete_callback         (GtkWidget *, GdkEvent *, gpointer);
static void            levels_preview_update          (GtkWidget *, gpointer);
static void            levels_low_input_text_update   (GtkWidget *, gpointer);
static void            levels_gamma_text_update       (GtkWidget *, gpointer);
static void            levels_high_input_text_update  (GtkWidget *, gpointer);
static void            levels_low_output_text_update  (GtkWidget *, gpointer);
static void            levels_high_output_text_update (GtkWidget *, gpointer);
static gint            levels_input_da_events         (GtkWidget *, GdkEvent *, LevelsDialog *);
static gint            levels_output_da_events        (GtkWidget *, GdkEvent *, LevelsDialog *);

static void *levels_options = NULL;
static LevelsDialog *levels_dialog = NULL;

static void       levels (PixelRegion *, PixelRegion *, void *);
static void       levels_histogram_info (PixelRegion *, PixelRegion *, HistogramValues, void *);
static void       levels_histogram_range (int, int, HistogramValues, void *);
static Argument * levels_invoker (Argument *);

/*  levels machinery  */

static void
levels (PixelRegion *srcPR,
	PixelRegion *destPR,
	void        *user_data)
{
  LevelsDialog *ld;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int has_alpha, alpha;
  int w, h;

  ld = (LevelsDialog *) user_data;

  h = srcPR->h;
  src = srcPR->data;
  dest = destPR->data;
  has_alpha = (srcPR->bytes == 2 || srcPR->bytes == 4);
  alpha = has_alpha ? srcPR->bytes - 1 : srcPR->bytes;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;
      while (w--)
	{
	  if (ld->color)
	    {
	      /*  The contributions from the individual channel level settings  */
	      d[RED_PIX] = ld->output[HISTOGRAM_RED][ld->input[HISTOGRAM_RED][s[RED_PIX]]];
	      d[GREEN_PIX] = ld->output[HISTOGRAM_GREEN][ld->input[HISTOGRAM_GREEN][s[GREEN_PIX]]];
	      d[BLUE_PIX] = ld->output[HISTOGRAM_BLUE][ld->input[HISTOGRAM_BLUE][s[BLUE_PIX]]];

	      /*  The overall changes  */
	      d[RED_PIX] = ld->output[HISTOGRAM_VALUE][ld->input[HISTOGRAM_VALUE][d[RED_PIX]]];
	      d[GREEN_PIX] = ld->output[HISTOGRAM_VALUE][ld->input[HISTOGRAM_VALUE][d[GREEN_PIX]]];
	      d[BLUE_PIX] = ld->output[HISTOGRAM_VALUE][ld->input[HISTOGRAM_VALUE][d[BLUE_PIX]]];
	    }
	  else
	    d[GRAY_PIX] = ld->output[HISTOGRAM_VALUE][ld->input[HISTOGRAM_VALUE][s[GRAY_PIX]]];;

	  if (has_alpha)
	      d[alpha] = ld->output[HISTOGRAM_ALPHA][ld->input[HISTOGRAM_ALPHA][s[alpha]]];
	  /*d[alpha] = s[alpha];*/

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
levels_histogram_info (PixelRegion     *srcPR,
		       PixelRegion     *maskPR,
		       HistogramValues  values,
		       void            *user_data)
{
  LevelsDialog *ld;
  unsigned char *src, *s;
  unsigned char *mask, *m;
  int w, h;
  int value, red, green, blue;
  int has_alpha, alpha;

  mask = NULL;
  m    = NULL;

  ld = (LevelsDialog *) user_data;

  h = srcPR->h;
  src = srcPR->data;
  has_alpha = (srcPR->bytes == 2 || srcPR->bytes == 4);
  alpha = has_alpha ? srcPR->bytes - 1 : srcPR->bytes;

  if (maskPR)
    mask = maskPR->data;

  while (h--)
    {
      w = srcPR->w;
      s = src;

      if (maskPR)
	m = mask;

      while (w--)
	{
	  if (ld->color)
	    {
	      value = MAX (s[RED_PIX], s[GREEN_PIX]);
	      value = MAX (value, s[BLUE_PIX]);
	      red = s[RED_PIX];
	      green = s[GREEN_PIX];
	      blue = s[BLUE_PIX];
	      alpha = s[ALPHA_PIX];
	      if (maskPR)
		{
		  values[HISTOGRAM_VALUE][value] += (double) *m / 255.0;
		  values[HISTOGRAM_RED][red]     += (double) *m / 255.0;
		  values[HISTOGRAM_GREEN][green] += (double) *m / 255.0;
		  values[HISTOGRAM_BLUE][blue]   += (double) *m / 255.0;
		}
	      else
		{
		  values[HISTOGRAM_VALUE][value] += 1.0;
		  values[HISTOGRAM_RED][red]     += 1.0;
		  values[HISTOGRAM_GREEN][green] += 1.0;
		  values[HISTOGRAM_BLUE][blue]   += 1.0;
		}
	    }
	  else
	    {
	      value = s[GRAY_PIX];
	      if (maskPR)
		values[HISTOGRAM_VALUE][value] += (double) *m / 255.0;
	      else
		values[HISTOGRAM_VALUE][value] += 1.0;
	    }

	  if (has_alpha)
	    {
	      if (maskPR)
		values[HISTOGRAM_ALPHA][s[alpha]] += (double) *m / 255.0;
	      else
		values[HISTOGRAM_ALPHA][s[alpha]] += 1.0;
	    }

	  s += srcPR->bytes;

	  if (maskPR)
	    m += maskPR->bytes;
	}

      src += srcPR->rowstride;

      if (maskPR)
	mask += maskPR->rowstride;
    }
}

static void
levels_histogram_range (int              start,
			int              end,
			HistogramValues  values,
			void            *user_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) user_data;

  histogram_range (ld->histogram, -1, -1);
}

/*  levels action functions  */

static void
levels_button_press (Tool           *tool,
		     GdkEventButton *bevent,
		     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
levels_button_release (Tool           *tool,
		       GdkEventButton *bevent,
		       gpointer        gdisp_ptr)
{
}

static void
levels_motion (Tool           *tool,
	       GdkEventMotion *mevent,
	       gpointer        gdisp_ptr)
{
}

static void
levels_cursor_update (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
levels_control (Tool     *tool,
		int       action,
		gpointer  gdisp_ptr)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (levels_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (levels_dialog->image_map);
	  active_tool->preserve = FALSE;
	  levels_dialog->image_map = NULL;
	  levels_cancel_callback (NULL, (gpointer) levels_dialog);
	}
      break;
    }
}

Tool *
tools_new_levels ()
{
  Tool * tool;
  Levels * private;

  /*  The tool options  */
  if (!levels_options)
    levels_options = tools_register_no_options (LEVELS, "Levels Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (Levels *) g_malloc (sizeof (Levels));

  tool->type = LEVELS;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = levels_button_press;
  tool->button_release_func = levels_button_release;
  tool->motion_func = levels_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = levels_cursor_update;
  tool->control_func = levels_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_levels (Tool *tool)
{
  Levels * _levels;

  _levels = (Levels *) tool->private;

  /*  Close the color select dialog  */
  if (levels_dialog)
    levels_cancel_callback (NULL, (gpointer) levels_dialog);

  g_free (_levels);
}

static MenuItem color_option_items[] =
{
  { "Value", 0, 0, levels_value_callback, NULL, NULL, NULL },
  { "Red", 0, 0, levels_red_callback, NULL, NULL, NULL },
  { "Green", 0, 0, levels_green_callback, NULL, NULL, NULL },
  { "Blue", 0, 0, levels_blue_callback, NULL, NULL, NULL },
  { "Alpha", 0, 0, levels_alpha_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL }
};

void
levels_initialize (GDisplay *gdisp)
{
  int i;

  if (drawable_indexed (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("Levels for indexed drawables cannot be adjusted.");
      return;
    }

  /*  The levels dialog  */
  if (!levels_dialog)
    levels_dialog = levels_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (levels_dialog->shell))
      gtk_widget_show (levels_dialog->shell);

  /*  Initialize the values  */
  levels_dialog->channel = HISTOGRAM_VALUE;
  for (i = 0; i < 5; i++)
    {
      levels_dialog->low_input[i] = 0;
      levels_dialog->gamma[i] = 1.0;
      levels_dialog->high_input[i] = 255;
      levels_dialog->low_output[i] = 0;
      levels_dialog->high_output[i] = 255;
    }

  levels_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  levels_dialog->color = drawable_color (levels_dialog->drawable);
  levels_dialog->image_map = image_map_create (gdisp, levels_dialog->drawable);

  /* check for alpha channel */
  if (drawable_has_alpha (levels_dialog->drawable))
    gtk_widget_set_sensitive( color_option_items[4].widget, TRUE);
  else 
    gtk_widget_set_sensitive( color_option_items[4].widget, FALSE);
  
  /*  hide or show the channel menu based on image type  */
  if (levels_dialog->color)
    for (i = 0; i < 4; i++) 
       gtk_widget_set_sensitive( color_option_items[i].widget, TRUE);
  else 
    for (i = 1; i < 4; i++) 
       gtk_widget_set_sensitive( color_option_items[i].widget, FALSE);

  /* set the current selection */
  gtk_option_menu_set_history ( GTK_OPTION_MENU (levels_dialog->channel_menu), 0);


  levels_update (levels_dialog, LOW_INPUT | GAMMA | HIGH_INPUT | LOW_OUTPUT | HIGH_OUTPUT | DRAW);
  levels_update (levels_dialog, INPUT_LEVELS | OUTPUT_LEVELS);

  histogram_update (levels_dialog->histogram,
		    levels_dialog->drawable,
		    levels_histogram_info,
		    (void *) levels_dialog);
  histogram_range (levels_dialog->histogram, -1, -1);
}

void
levels_free ()
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

/****************************/
/*  Select by Color dialog  */
/****************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Auto Levels", levels_auto_levels_callback, NULL, NULL },
  { "OK", levels_ok_callback, NULL, NULL },
  { "Cancel", levels_cancel_callback, NULL, NULL }
};

static LevelsDialog *
levels_new_dialog ()
{
  LevelsDialog *ld;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *channel_hbox;
  GtkWidget *menu;
  int i;

  ld = g_malloc (sizeof (LevelsDialog));
  ld->preview = TRUE;

  for (i = 0; i < 5; i++)
    color_option_items [i].user_data = (gpointer) ld;

  /*  The shell and main vbox  */
  ld->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (ld->shell), "levels", "Gimp");
  gtk_window_set_title (GTK_WINDOW (ld->shell), "Levels");

  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (ld->shell), "delete_event",
		      GTK_SIGNAL_FUNC (levels_delete_callback),
		      ld);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ld->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  The option menu for selecting channels  */
  channel_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), channel_hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Modify Levels for Channel: ");
  gtk_box_pack_start (GTK_BOX (channel_hbox), label, FALSE, FALSE, 0);

  menu = build_menu (color_option_items, NULL);
  ld->channel_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (channel_hbox), ld->channel_menu, FALSE, FALSE, 2);

  gtk_widget_show (label);
  gtk_widget_show (ld->channel_menu);
  gtk_widget_show (channel_hbox);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (ld->channel_menu), menu);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Input Levels: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low input text  */
  ld->low_input_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), "0");
  gtk_widget_set_usize (ld->low_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_input_text), "changed",
		      (GtkSignalFunc) levels_low_input_text_update,
		      ld);
  gtk_widget_show (ld->low_input_text);

  /* input gamma text  */
  ld->gamma_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), "1.0");
  gtk_widget_set_usize (ld->gamma_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->gamma_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->gamma_text), "changed",
		      (GtkSignalFunc) levels_gamma_text_update,
		      ld);
  gtk_widget_show (ld->gamma_text);
  gtk_widget_show (hbox);

  /* high input text  */
  ld->high_input_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), "255");
  gtk_widget_set_usize (ld->high_input_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_input_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_input_text), "changed",
		      (GtkSignalFunc) levels_high_input_text_update,
		      ld);
  gtk_widget_show (ld->high_input_text);
  gtk_widget_show (hbox);

  /*  The levels histogram  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0);

  ld->histogram = histogram_create (HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT,
				    levels_histogram_range, (void *) ld);
  gtk_container_add (GTK_CONTAINER (frame), ld->histogram->histogram_widget);
  gtk_widget_show (ld->histogram->histogram_widget);
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
  ld->input_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->input_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[0]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[0], FALSE, TRUE, 0);
  ld->input_levels_da[1] = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (ld->input_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->input_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->input_levels_da[1]), "event",
		      (GtkSignalFunc) levels_input_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->input_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->input_levels_da[0]);
  gtk_widget_show (ld->input_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for levels text widget  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Output Levels: ");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  low output text  */
  ld->low_output_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), "0");
  gtk_widget_set_usize (ld->low_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->low_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->low_output_text), "changed",
		      (GtkSignalFunc) levels_low_output_text_update,
		      ld);
  gtk_widget_show (ld->low_output_text);

  /*  high output text  */
  ld->high_output_text = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), "255");
  gtk_widget_set_usize (ld->high_output_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), ld->high_output_text, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (ld->high_output_text), "changed",
		      (GtkSignalFunc) levels_high_output_text_update,
		      ld);
  gtk_widget_show (ld->high_output_text);
  gtk_widget_show (hbox);

  /*  The output levels drawing area  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  ld->output_levels_da[0] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[0]), DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[0], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[0]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[0], FALSE, TRUE, 0);
  ld->output_levels_da[1] = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (ld->output_levels_da[1]), DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (ld->output_levels_da[1], LEVELS_DA_MASK);
  gtk_signal_connect (GTK_OBJECT (ld->output_levels_da[1]), "event",
		      (GtkSignalFunc) levels_output_da_events,
		      ld);
  gtk_box_pack_start (GTK_BOX (vbox2), ld->output_levels_da[1], FALSE, TRUE, 0);
  gtk_widget_show (ld->output_levels_da[0]);
  gtk_widget_show (ld->output_levels_da[1]);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  /*  Horizontal box for preview  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), ld->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) levels_preview_update,
		      ld);

  gtk_widget_show (label);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = ld;
  action_items[1].user_data = ld;
  action_items[2].user_data = ld;
  build_action_area (GTK_DIALOG (ld->shell), action_items, 3, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (ld->shell);

  return ld;
}

static void
levels_draw_slider (GdkWindow *window,
		    GdkGC     *border_gc,
		    GdkGC     *fill_gc,
		    int        xpos)
{
  int y;

  for (y = 0; y < CONTROL_HEIGHT; y++)
    gdk_draw_line(window, fill_gc, xpos - y / 2, y,
		  xpos + y / 2, y);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos - (CONTROL_HEIGHT - 1) / 2,  CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos, 0,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);

  gdk_draw_line(window, border_gc, xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1,
		xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
}

static void
levels_erase_slider (GdkWindow *window,
		     int        xpos)
{
  gdk_window_clear_area (window, xpos - (CONTROL_HEIGHT - 1) / 2, 0,
			 CONTROL_HEIGHT - 1, CONTROL_HEIGHT);
}

static void
levels_calculate_transfers (LevelsDialog *ld)
{
  double inten;
  int i, j;

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

	  /*  determine the output intensity  */
	  inten = (double) i / 255.0;
	  if (ld->high_output[j] >= ld->low_output[j])
	    inten = (double) (inten * (ld->high_output[j] - ld->low_output[j]) +
			      ld->low_output[j]);
	  else if (ld->high_output[j] < ld->low_output[j])
	    inten = (double) (ld->low_output[j] - inten *
			      (ld->low_output[j] - ld->high_output[j]));

	  inten = BOUNDS (inten, 0.0, 255.0);
	  ld->output[j][i] = (unsigned char) (inten + 0.5);
	}
    }
}

static void
levels_update (LevelsDialog *ld,
	       int           update)
{
  char text[12];
  int i;

  /*  Recalculate the transfer arrays  */
  levels_calculate_transfers (ld);

  if (update & LOW_INPUT)
    {
      sprintf (text, "%d", ld->low_input[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->low_input_text), text);
    }
  if (update & GAMMA)
    {
      sprintf (text, "%2.2f", ld->gamma[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->gamma_text), text);
    }
  if (update & HIGH_INPUT)
    {
      sprintf (text, "%d", ld->high_input[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->high_input_text), text);
    }
  if (update & LOW_OUTPUT)
    {
      sprintf (text, "%d", ld->low_output[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->low_output_text), text);
    }
  if (update & HIGH_OUTPUT)
    {
      sprintf (text, "%d", ld->high_output[ld->channel]);
      gtk_entry_set_text (GTK_ENTRY (ld->high_output_text), text);
    }
  if (update & INPUT_LEVELS)
    {
      for (i = 0; i < GRADIENT_HEIGHT; i++)
	gtk_preview_draw_row (GTK_PREVIEW (ld->input_levels_da[0]),
			      ld->input[ld->channel], 0, i, DA_WIDTH);

      if (update & DRAW)
	gtk_widget_draw (ld->input_levels_da[0], NULL);
    }
  if (update & OUTPUT_LEVELS)
    {
      unsigned char buf[DA_WIDTH];

      for (i = 0; i < DA_WIDTH; i++)
	buf[i] = i;

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
    g_warning ("No image map");
  active_tool->preserve = TRUE;
  image_map_apply (ld->image_map, levels, (void *) ld);
  active_tool->preserve = FALSE;
}

static void
levels_value_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_VALUE)
    {
      ld->channel = HISTOGRAM_VALUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_red_callback (GtkWidget *w,
		     gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_RED)
    {
      ld->channel = HISTOGRAM_RED;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_green_callback (GtkWidget *w,
		       gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_GREEN)
    {
      ld->channel = HISTOGRAM_GREEN;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_blue_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_BLUE)
    {
      ld->channel = HISTOGRAM_BLUE;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_alpha_callback (GtkWidget *w,
		      gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (ld->channel != HISTOGRAM_ALPHA)
    {
      ld->channel = HISTOGRAM_ALPHA;
      histogram_channel (ld->histogram, ld->channel);
      levels_update (ld, ALL);
    }
}

static void
levels_adjust_channel (LevelsDialog    *ld,
		       HistogramValues *values,
		       int              channel)
{
  int i;
  double count, new_count, percentage, next_percentage;

  ld->gamma[channel]       = 1.0;
  ld->low_output[channel]  = 0;
  ld->high_output[channel] = 255;

  count = 0.0;
  for (i = 0; i < 256; i++)
    count += (*values)[channel][i];

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
	  new_count += (*values)[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + (*values)[channel][i + 1]) / count;

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
	  new_count += (*values)[channel][i];
	  percentage = new_count / count;
	  next_percentage = (new_count + (*values)[channel][i - 1]) / count;

	  if (fabs (percentage - 0.006) < fabs (next_percentage - 0.006))
	    {
	      ld->high_input[channel] = i - 1;
	      break;
	    }
	}
    }
}

static void
levels_auto_levels_callback (GtkWidget *widget,
			     gpointer   client_data)
{
  LevelsDialog *ld;
  HistogramValues *values;
  int channel;

  ld = (LevelsDialog *) client_data;
  values = histogram_values (ld->histogram);

  if (ld->color)
    {
      /*  Set the overall value to defaults  */
      ld->low_input[HISTOGRAM_VALUE]   = 0;
      ld->gamma[HISTOGRAM_VALUE]       = 1.0;
      ld->high_input[HISTOGRAM_VALUE]  = 255;
      ld->low_output[HISTOGRAM_VALUE]  = 0;
      ld->high_output[HISTOGRAM_VALUE] = 255;

      for (channel = 0; channel < 3; channel ++)
	levels_adjust_channel (ld, values, channel + 1);
    }
  else
    levels_adjust_channel (ld, values, HISTOGRAM_VALUE);

  levels_update (ld, ALL);
  if (ld->preview)
    levels_preview (ld);
}

static void
levels_ok_callback (GtkWidget *widget,
		    gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  active_tool->preserve = TRUE;

  if (!ld->preview)
    image_map_apply (ld->image_map, levels, (void *) ld);

  if (ld->image_map)
    image_map_commit (ld->image_map);

  active_tool->preserve = FALSE;

  ld->image_map = NULL;
}

static gint
levels_delete_callback (GtkWidget *w,
			GdkEvent *e,
			gpointer client_data)
{
  levels_cancel_callback (w, client_data);

  return TRUE;
}

static void
levels_cancel_callback (GtkWidget *widget,
			gpointer   client_data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (ld->shell))
    gtk_widget_hide (ld->shell);

  if (ld->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (ld->image_map);
      active_tool->preserve = TRUE;
      gdisplays_flush ();
    }

  ld->image_map = NULL;
}

static void
levels_preview_update (GtkWidget *w,
		       gpointer   data)
{
  LevelsDialog *ld;

  ld = (LevelsDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      ld->preview = TRUE;
      levels_preview (ld);
    }
  else
    ld->preview = FALSE;
}

static void
levels_low_input_text_update (GtkWidget *w,
			      gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS (((int) atof (str)), 0, ld->high_input[ld->channel]);

  if (value != ld->low_input[ld->channel])
    {
      ld->low_input[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_gamma_text_update (GtkWidget *w,
			  gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  double value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS ((atof (str)), 0.1, 10.0);

  if (value != ld->gamma[ld->channel])
    {
      ld->gamma[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_input_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS (((int) atof (str)), ld->low_input[ld->channel], 255);

  if (value != ld->high_input[ld->channel])
    {
      ld->high_input[ld->channel] = value;
      levels_update (ld, INPUT_LEVELS | INPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_low_output_text_update (GtkWidget *w,
			       gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS (((int) atof (str)), 0, 255);

  if (value != ld->low_output[ld->channel])
    {
      ld->low_output[ld->channel] = value;
      levels_update (ld, OUTPUT_LEVELS | OUTPUT_SLIDERS | DRAW);

      if (ld->preview)
	levels_preview (ld);
    }
}

static void
levels_high_output_text_update (GtkWidget *w,
				gpointer   data)
{
  LevelsDialog *ld;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  ld = (LevelsDialog *) data;
  value = BOUNDS (((int) atof (str)), 0, 255);

  if (value != ld->high_output[ld->channel])
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
  char text[12];
  double width, mid, tmp;
  int x, distance;
  int i;
  int update = FALSE;

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


/*  The levels procedure definition  */
ProcArg levels_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "channel",
    "the channel to modify: { VALUE (0), RED (1), GREEN (2), BLUE (3), GRAY (0) }"
  },
  { PDB_INT32,
    "low_input",
    "intensity of lowest input: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_input",
    "intensity of highest input: (0 <= high_input <= 255)"
  },
  { PDB_FLOAT,
    "gamma",
    "gamma correction factor: (0.1 <= gamma <= 10)"
  },
  { PDB_INT32,
    "low_output",
    "intensity of lowest output: (0 <= low_input <= 255)"
  },
  { PDB_INT32,
    "high_output",
    "intensity of highest output: (0 <= high_input <= 255)"
  }
};

ProcRecord levels_proc =
{
  "gimp_levels",
  "Modifies intensity levels in the specified drawable",
  "This tool allows intensity levels in the specified drawable to be remapped according to a set of parameters.  The low/high input levels specify an initial mapping from the source intensities.  The gamma value determines how intensities between the low and high input intensities are interpolated.  A gamma value of 1.0 results in a linear interpolation.  Higher gamma values result in more high-level intensities.  Lower gamma values result in more low-level intensities.  The low/high output levels constrain the final intensity mapping--that is, no final intensity will be lower than the low output level and no final intensity will be higher than the high output level.  This tool is only valid on RGB color and grayscale images.  It will not operate on indexed drawables.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  levels_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { levels_invoker } },
};


static Argument *
levels_invoker (Argument *args)
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  LevelsDialog ld;
  GImage *gimage;
  GimpDrawable *drawable;
  int channel;
  int low_input;
  int high_input;
  double gamma;
  int low_output;
  int high_output;
  int int_value;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  void *pr;

  drawable = NULL;
  low_input   = 0;
  high_input  = 0;
  gamma       = 1.0;
  low_output  = 0;
  high_output = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);
   
  /*  channel  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      if (success)
	{
	  if (drawable_gray (drawable))
	    {
	      if (int_value != 0)
		success = FALSE;
	    }
	  else if (drawable_color (drawable))
	    {
	      if (int_value < 0 || int_value > 3)
		success = FALSE;
	    }
	  else
	    success = FALSE;
	}
      channel = int_value;
    }
  /*  low input  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_input = int_value;
      else
	success = FALSE;
    }
  /*  high input  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_input = int_value;
      else
	success = FALSE;
    }
  /*  gamma  */
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value >= 0.1 && fp_value <= 10.0)
	gamma = fp_value;
      else
	success = FALSE;
    }
  /*  low output  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	low_output = int_value;
      else
	success = FALSE;
    }
  /*  high output  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      if (int_value >= 0 && int_value < 256)
	high_output = int_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the levels  */
  if (success)
    {
      for (i = 0; i < 5; i++)
	{
	  ld.low_input[i] = 0;
	  ld.gamma[i] = 1.0;
	  ld.high_input[i] = 255;
	  ld.low_output[i] = 0;
	  ld.high_output[i] = 255;
	}

      ld.channel = channel;
      ld.color = drawable_color (drawable);
      ld.low_input[channel] = low_input;
      ld.high_input[channel] = high_input;
      ld.gamma[channel] = gamma;
      ld.low_output[channel] = low_output;
      ld.high_output[channel] = high_output;

      /*  calculate the transfer arrays  */
      levels_calculate_transfers (&ld);

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
	levels (&srcPR, &destPR, (void *) &ld);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&levels_proc, success);
}
