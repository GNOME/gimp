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
#include "color_balance.h"
#include "color_transfer.h"
#include "drawable.h"
#include "general.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "image_map.h"
#include "interface.h"

#define TEXT_WIDTH 55

#define SHADOWS    0
#define MIDTONES   1
#define HIGHLIGHTS 2

#define CR_SLIDER 0x1
#define MG_SLIDER 0x2
#define YB_SLIDER 0x4
#define CR_TEXT   0x8
#define MG_TEXT   0x10
#define YB_TEXT   0x20
#define ALL       0xFF

typedef struct _ColorBalance ColorBalance;

struct _ColorBalance
{
  int x, y;    /*  coords for last mouse click  */
};

typedef struct _ColorBalanceDialog ColorBalanceDialog;

struct _ColorBalanceDialog
{
  GtkWidget   *shell;
  GtkWidget   *cyan_red_text;
  GtkWidget   *magenta_green_text;
  GtkWidget   *yellow_blue_text;
  GtkAdjustment  *cyan_red_data;
  GtkAdjustment  *magenta_green_data;
  GtkAdjustment  *yellow_blue_data;

  GimpDrawable *drawable;
  ImageMap     image_map;

  double       cyan_red[3];
  double       magenta_green[3];
  double       yellow_blue[3];

  gint         preserve_luminosity;
  gint         preview;
  gint         application_mode;
};

/*  color balance action functions  */

static void   color_balance_button_press   (Tool *, GdkEventButton *, gpointer);
static void   color_balance_button_release (Tool *, GdkEventButton *, gpointer);
static void   color_balance_motion         (Tool *, GdkEventMotion *, gpointer);
static void   color_balance_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void   color_balance_control        (Tool *, int, gpointer);

static ColorBalanceDialog *  color_balance_new_dialog          (void);
static void                  color_balance_update              (ColorBalanceDialog *, int);
static void                  color_balance_preview             (ColorBalanceDialog *);
static void                  color_balance_ok_callback         (GtkWidget *, gpointer);
static void                  color_balance_cancel_callback     (GtkWidget *, gpointer);
static gint                  color_balance_delete_callback     (GtkWidget *, GdkEvent *, gpointer);
static void                  color_balance_shadows_callback    (GtkWidget *, gpointer);
static void                  color_balance_midtones_callback   (GtkWidget *, gpointer);
static void                  color_balance_highlights_callback (GtkWidget *, gpointer);
static void                  color_balance_preserve_update     (GtkWidget *, gpointer);
static void                  color_balance_preview_update      (GtkWidget *, gpointer);
static void                  color_balance_cr_scale_update     (GtkAdjustment *, gpointer);
static void                  color_balance_mg_scale_update     (GtkAdjustment *, gpointer);
static void                  color_balance_yb_scale_update     (GtkAdjustment *, gpointer);
static void                  color_balance_cr_text_update      (GtkWidget *, gpointer);
static void                  color_balance_mg_text_update      (GtkWidget *, gpointer);
static void                  color_balance_yb_text_update      (GtkWidget *, gpointer);

static void *color_balance_options = NULL;
static ColorBalanceDialog *color_balance_dialog = NULL;

static void       color_balance (PixelRegion *, PixelRegion *, void *);
static Argument * color_balance_invoker (Argument *);

/*  color balance machinery  */

static void
color_balance (PixelRegion *srcPR,
	       PixelRegion *destPR,
	       void        *user_data)
{
  ColorBalanceDialog *cbd;
  unsigned char *src, *s;
  unsigned char *dest, *d;
  int alpha;
  int r, g, b;
  int r_n, g_n, b_n;
  int w, h;
  double *cyan_red_transfer[3];
  double *magenta_green_transfer[3];
  double *yellow_blue_transfer[3];

  cbd = (ColorBalanceDialog *) user_data;

  /*  Set the transfer arrays  (for speed)  */
  cyan_red_transfer[SHADOWS] = (cbd->cyan_red[SHADOWS] > 0) ? shadows_add : shadows_sub;
  cyan_red_transfer[MIDTONES] = (cbd->cyan_red[MIDTONES] > 0) ? midtones_add : midtones_sub;
  cyan_red_transfer[HIGHLIGHTS] = (cbd->cyan_red[HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;
  magenta_green_transfer[SHADOWS] = (cbd->magenta_green[SHADOWS] > 0) ? shadows_add : shadows_sub;
  magenta_green_transfer[MIDTONES] = (cbd->magenta_green[MIDTONES] > 0) ? midtones_add : midtones_sub;
  magenta_green_transfer[HIGHLIGHTS] = (cbd->magenta_green[HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;
  yellow_blue_transfer[SHADOWS] = (cbd->yellow_blue[SHADOWS] > 0) ? shadows_add : shadows_sub;
  yellow_blue_transfer[MIDTONES] = (cbd->yellow_blue[MIDTONES] > 0) ? midtones_add : midtones_sub;
  yellow_blue_transfer[HIGHLIGHTS] = (cbd->yellow_blue[HIGHLIGHTS] > 0) ? highlights_add : highlights_sub;

  h = srcPR->h;
  src = srcPR->data;
  dest = destPR->data;
  alpha = (srcPR->bytes == 4) ? TRUE : FALSE;

  while (h--)
    {
      w = srcPR->w;
      s = src;
      d = dest;
      while (w--)
	{
	  r = r_n = s[RED_PIX];
	  g = g_n = s[GREEN_PIX];
	  b = b_n = s[BLUE_PIX];

	  r_n += cbd->cyan_red[SHADOWS] * cyan_red_transfer[SHADOWS][r_n];
	  r_n = CLAMP0255 (r_n);
	  r_n += cbd->cyan_red[MIDTONES] * cyan_red_transfer[MIDTONES][r_n];
	  r_n = CLAMP0255 (r_n);
	  r_n += cbd->cyan_red[HIGHLIGHTS] * cyan_red_transfer[HIGHLIGHTS][r_n];
	  r_n = CLAMP0255 (r_n);

	  g_n += cbd->magenta_green[SHADOWS] * magenta_green_transfer[SHADOWS][g_n];
	  g_n = CLAMP0255 (g_n);
	  g_n += cbd->magenta_green[MIDTONES] * magenta_green_transfer[MIDTONES][g_n];
	  g_n = CLAMP0255 (g_n);
	  g_n += cbd->magenta_green[HIGHLIGHTS] * magenta_green_transfer[HIGHLIGHTS][g_n];
	  g_n = CLAMP0255 (g_n);

	  b_n += cbd->yellow_blue[SHADOWS] * yellow_blue_transfer[SHADOWS][b_n];
	  b_n = CLAMP0255 (b_n);
	  b_n += cbd->yellow_blue[MIDTONES] * yellow_blue_transfer[MIDTONES][b_n];
	  b_n = CLAMP0255 (b_n);
	  b_n += cbd->yellow_blue[HIGHLIGHTS] * yellow_blue_transfer[HIGHLIGHTS][b_n];
	  b_n = CLAMP0255 (b_n);

	  if (cbd->preserve_luminosity)
	    {
	      rgb_to_hls (&r, &g, &b);
	      rgb_to_hls (&r_n, &g_n, &b_n);
	      g_n = g;
	      hls_to_rgb (&r_n, &g_n, &b_n);
	    }

	  d[RED_PIX] = r_n;
	  d[GREEN_PIX] = g_n;
 	  d[BLUE_PIX] = b_n;

	  if (alpha)
	    d[ALPHA_PIX] = s[ALPHA_PIX];

	  s += srcPR->bytes;
	  d += destPR->bytes;
	}

      src += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}


/*  by_color select action functions  */

static void
color_balance_button_press (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
color_balance_button_release (Tool           *tool,
			      GdkEventButton *bevent,
			      gpointer        gdisp_ptr)
{
}

static void
color_balance_motion (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
}

static void
color_balance_cursor_update (Tool           *tool,
			     GdkEventMotion *mevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
color_balance_control (Tool     *tool,
		       int       action,
		       gpointer  gdisp_ptr)
{
  ColorBalance * color_bal;

  color_bal = (ColorBalance *) tool->private;

  switch (action)
    {
    case PAUSE :
      break;
    case RESUME :
      break;
    case HALT :
      if (color_balance_dialog)
	{
	  active_tool->preserve = TRUE;
	  image_map_abort (color_balance_dialog->image_map);
	  active_tool->preserve = FALSE;
	  color_balance_dialog->image_map = NULL;
	  color_balance_cancel_callback (NULL, (gpointer) color_balance_dialog);
	}
      break;
    }
}

Tool *
tools_new_color_balance ()
{
  Tool * tool;
  ColorBalance * private;

  /*  The tool options  */
  if (!color_balance_options)
    color_balance_options = tools_register_no_options (COLOR_BALANCE, "Color Balance Options");

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (ColorBalance *) g_malloc (sizeof (ColorBalance));

  tool->type = COLOR_BALANCE;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;  /*  Disallow scrolling  */
  tool->private = (void *) private;
  tool->auto_snap_to = TRUE;
  tool->button_press_func = color_balance_button_press;
  tool->button_release_func = color_balance_button_release;
  tool->motion_func = color_balance_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = color_balance_cursor_update;
  tool->control_func = color_balance_control;
  tool->preserve = FALSE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;

  return tool;
}

void
tools_free_color_balance (Tool *tool)
{
  ColorBalance * color_bal;

  color_bal = (ColorBalance *) tool->private;

  /*  Close the color select dialog  */
  if (color_balance_dialog)
    color_balance_cancel_callback (NULL, (gpointer) color_balance_dialog);

  g_free (color_bal);
}

void
color_balance_initialize (GDisplay *gdisp)
{
  int i;

  if (! drawable_color (gimage_active_drawable (gdisp->gimage)))
    {
      g_message ("Color balance operates only on RGB color drawables.");
      return;
    }

  /*  The color balance dialog  */
  if (!color_balance_dialog)
    color_balance_dialog = color_balance_new_dialog ();
  else
    if (!GTK_WIDGET_VISIBLE (color_balance_dialog->shell))
      gtk_widget_show (color_balance_dialog->shell);

  /*  Initialize dialog fields  */
  color_balance_dialog->image_map = NULL;
  for (i = 0; i < 3; i++)
    {
      color_balance_dialog->cyan_red[i] = 0.0;
      color_balance_dialog->magenta_green[i] = 0.0;
      color_balance_dialog->yellow_blue[i] = 0.0;
    }
  color_balance_dialog->drawable = gimage_active_drawable (gdisp->gimage);
  color_balance_dialog->image_map = image_map_create (gdisp, color_balance_dialog->drawable);
  color_balance_update (color_balance_dialog, ALL);
}


/****************************/
/*  Select by Color dialog  */
/****************************/

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "OK", color_balance_ok_callback, NULL, NULL },
  { "Cancel", color_balance_cancel_callback, NULL, NULL }
};

static ColorBalanceDialog *
color_balance_new_dialog ()
{
  ColorBalanceDialog *cbd;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *start_label;
  GtkWidget *end_label;
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *toggle;
  GtkWidget *radio_button;
  GtkObject *data;
  GSList *group = NULL;
  int i;
  char *appl_mode_names[3] =
  {
    "Shadows",
    "Midtones",
    "Highlights"
  };
  ActionCallback appl_mode_callbacks[3] =
  {
    color_balance_shadows_callback,
    color_balance_midtones_callback,
    color_balance_highlights_callback
  };

  cbd = g_malloc (sizeof (ColorBalanceDialog));
  cbd->preserve_luminosity = TRUE;
  cbd->preview = TRUE;
  cbd->application_mode = SHADOWS;

  /*  The shell and main vbox  */
  cbd->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (cbd->shell), "color_balance", "Gimp");
  gtk_window_set_title (GTK_WINDOW (cbd->shell), "Color Balance");
  
  /* handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (cbd->shell), "delete_event",
		      GTK_SIGNAL_FUNC (color_balance_delete_callback),
		      cbd);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cbd->shell)->vbox), vbox, TRUE, TRUE, 0);

  /*  Horizontal box for application mode  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Color Levels: ");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  /*  cyan-red text  */
  cbd->cyan_red_text = gtk_entry_new ();
  gtk_widget_set_usize (cbd->cyan_red_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), cbd->cyan_red_text, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cbd->cyan_red_text), "changed",
		      (GtkSignalFunc) color_balance_cr_text_update,
		      cbd);
  gtk_widget_show (cbd->cyan_red_text);

  /*  magenta-green text  */
  cbd->magenta_green_text = gtk_entry_new ();
  gtk_widget_set_usize (cbd->magenta_green_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), cbd->magenta_green_text, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cbd->magenta_green_text), "changed",
		      (GtkSignalFunc) color_balance_mg_text_update,
		      cbd);
  gtk_widget_show (cbd->magenta_green_text);

  /*  yellow-blue text  */
  cbd->yellow_blue_text = gtk_entry_new ();
  gtk_widget_set_usize (cbd->yellow_blue_text, TEXT_WIDTH, 25);
  gtk_box_pack_start (GTK_BOX (hbox), cbd->yellow_blue_text, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (cbd->yellow_blue_text), "changed",
		      (GtkSignalFunc) color_balance_yb_text_update,
		      cbd);
  gtk_widget_show (cbd->yellow_blue_text);
  gtk_widget_show (hbox);


  /*  The table containing sliders  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  Create the cyan-red scale widget  */
  start_label = gtk_label_new ("Cyan");
  gtk_misc_set_alignment (GTK_MISC (start_label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), start_label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  cbd->cyan_red_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) color_balance_cr_scale_update,
		      cbd);
  end_label = gtk_label_new ("Red");
  gtk_misc_set_alignment (GTK_MISC (end_label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), end_label, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  gtk_widget_show (start_label);
  gtk_widget_show (end_label);
  gtk_widget_show (slider);

  /*  Create the magenta-green scale widget  */
  start_label = gtk_label_new ("Magenta");
  gtk_misc_set_alignment (GTK_MISC (start_label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), start_label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  cbd->magenta_green_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) color_balance_mg_scale_update,
		      cbd);
  end_label = gtk_label_new ("Green");
  gtk_misc_set_alignment (GTK_MISC (end_label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), end_label, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  gtk_widget_show (start_label);
  gtk_widget_show (end_label);
  gtk_widget_show (slider);

  /*  Create the yellow-blue scale widget  */
  start_label = gtk_label_new ("Yellow");
  gtk_misc_set_alignment (GTK_MISC (start_label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), start_label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);
  data = gtk_adjustment_new (0, -100.0, 100.0, 1.0, 1.0, 0.0);
  cbd->yellow_blue_data = GTK_ADJUSTMENT (data);
  slider = gtk_hscale_new (GTK_ADJUSTMENT (data));
  gtk_scale_set_digits (GTK_SCALE (slider), 0);
  gtk_table_attach (GTK_TABLE (table), slider, 1, 2, 2, 3,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    2, 2);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (data), "value_changed",
		      (GtkSignalFunc) color_balance_yb_scale_update,
		      cbd);
  end_label = gtk_label_new ("Blue");
  gtk_misc_set_alignment (GTK_MISC (end_label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), end_label, 2, 3, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 2, 2);

  gtk_widget_show (start_label);
  gtk_widget_show (end_label);
  gtk_widget_show (slider);

  /*  Horizontal box for preview and preserve luminosity toggle buttons  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  The preview toggle  */
  toggle = gtk_check_button_new_with_label ("Preview");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), cbd->preview);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) color_balance_preview_update,
		      cbd);
  gtk_widget_show (toggle);

  /*  The preserve luminosity toggle  */
  toggle = gtk_check_button_new_with_label ("Preserve Luminosity");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), cbd->preserve_luminosity);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) color_balance_preserve_update,
		      cbd);
  gtk_widget_show (toggle);
  gtk_widget_show (hbox);

  /*  Horizontal box for application mode  */
  hbox = gtk_hbox_new (TRUE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /*  the radio buttons for application mode  */
  for (i = 0; i < 3; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, appl_mode_names[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_box_pack_start (GTK_BOX (hbox), radio_button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) appl_mode_callbacks[i],
			  cbd);
      gtk_widget_show (radio_button);
    }
  gtk_widget_show (hbox);

  /*  The action area  */
  action_items[0].user_data = cbd;
  action_items[1].user_data = cbd;
  build_action_area (GTK_DIALOG (cbd->shell), action_items, 2, 0);

  gtk_widget_show (table);
  gtk_widget_show (vbox);
  gtk_widget_show (cbd->shell);

  return cbd;
}

static void
color_balance_update (ColorBalanceDialog *cbd,
		      int                 update)
{
  char text[12];

  if (update & CR_SLIDER)
    {
      cbd->cyan_red_data->value = cbd->cyan_red[cbd->application_mode];
      gtk_signal_emit_by_name (GTK_OBJECT (cbd->cyan_red_data), "value_changed");
    }
  if (update & MG_SLIDER)
    {
      cbd->magenta_green_data->value = cbd->magenta_green[cbd->application_mode];
      gtk_signal_emit_by_name (GTK_OBJECT (cbd->magenta_green_data), "value_changed");
    }
  if (update & YB_SLIDER)
    {
      cbd->yellow_blue_data->value = cbd->yellow_blue[cbd->application_mode];
      gtk_signal_emit_by_name (GTK_OBJECT (cbd->yellow_blue_data), "value_changed");
    }
  if (update & CR_TEXT)
    {
      sprintf (text, "%0.0f", cbd->cyan_red[cbd->application_mode]);
      gtk_entry_set_text (GTK_ENTRY (cbd->cyan_red_text), text);
    }
  if (update & MG_TEXT)
    {
      sprintf (text, "%0.0f", cbd->magenta_green[cbd->application_mode]);
      gtk_entry_set_text (GTK_ENTRY (cbd->magenta_green_text), text);
    }
  if (update & YB_TEXT)
    {
      sprintf (text, "%0.0f", cbd->yellow_blue[cbd->application_mode]);
      gtk_entry_set_text (GTK_ENTRY (cbd->yellow_blue_text), text);
    }
}

static void
color_balance_preview (ColorBalanceDialog *cbd)
{
  if (!cbd->image_map)
    g_message ("color_balance_preview(): No image map");
  active_tool->preserve = TRUE;
  image_map_apply (cbd->image_map, color_balance, (void *) cbd);
  active_tool->preserve = FALSE;
}

static void
color_balance_ok_callback (GtkWidget *widget,
			   gpointer   client_data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) client_data;

  if (GTK_WIDGET_VISIBLE (cbd->shell))
    gtk_widget_hide (cbd->shell);
  
  active_tool->preserve = TRUE;

  if (!cbd->preview)
    image_map_apply (cbd->image_map, color_balance, (void *) cbd);

  if (cbd->image_map)
    image_map_commit (cbd->image_map);

  active_tool->preserve = FALSE;

  cbd->image_map = NULL;
}

static gint
color_balance_delete_callback (GtkWidget *w,
			       GdkEvent *e,
			       gpointer client_data)
{
  color_balance_cancel_callback (w, client_data);

  return TRUE;
}

static void
color_balance_cancel_callback (GtkWidget *widget,
			       gpointer   client_data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) client_data;
  if (GTK_WIDGET_VISIBLE (cbd->shell))
    gtk_widget_hide (cbd->shell);

  if (cbd->image_map)
    {
      active_tool->preserve = TRUE;
      image_map_abort (cbd->image_map);
      active_tool->preserve = FALSE;
      gdisplays_flush ();
    }

  cbd->image_map = NULL;
}

static void
color_balance_shadows_callback (GtkWidget *widget,
				gpointer   client_data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) client_data;
  cbd->application_mode = SHADOWS;
  color_balance_update (cbd, ALL);
}

static void
color_balance_midtones_callback (GtkWidget *widget,
				 gpointer   client_data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) client_data;
  cbd->application_mode = MIDTONES;
  color_balance_update (cbd, ALL);
}

static void
color_balance_highlights_callback (GtkWidget *widget,
				   gpointer   client_data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) client_data;
  cbd->application_mode = HIGHLIGHTS;
  color_balance_update (cbd, ALL);
}

static void
color_balance_preserve_update (GtkWidget *w,
			       gpointer   data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      cbd->preserve_luminosity = TRUE;
      if (cbd->preview)
	color_balance_preview (cbd);
    }
  else
    {
      cbd->preserve_luminosity = FALSE;
      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_preview_update (GtkWidget *w,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    {
      cbd->preview = TRUE;
      color_balance_preview (cbd);
    }
  else
    cbd->preview = FALSE;
}

static void
color_balance_cr_scale_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->cyan_red[cbd->application_mode] != adjustment->value)
    {
      cbd->cyan_red[cbd->application_mode] = adjustment->value;
      color_balance_update (cbd, CR_TEXT);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_mg_scale_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->magenta_green[cbd->application_mode] != adjustment->value)
    {
      cbd->magenta_green[cbd->application_mode] = adjustment->value;
      color_balance_update (cbd, MG_TEXT);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_yb_scale_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  ColorBalanceDialog *cbd;

  cbd = (ColorBalanceDialog *) data;

  if (cbd->yellow_blue[cbd->application_mode] != adjustment->value)
    {
      cbd->yellow_blue[cbd->application_mode] = adjustment->value;
      color_balance_update (cbd, YB_TEXT);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_cr_text_update (GtkWidget *w,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  cbd = (ColorBalanceDialog *) data;
  value = BOUNDS (((int) atof (str)), -100, 100);

  if ((int) cbd->cyan_red[cbd->application_mode] != value)
    {
      cbd->cyan_red[cbd->application_mode] = value;
      color_balance_update (cbd, CR_SLIDER);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_mg_text_update (GtkWidget *w,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  cbd = (ColorBalanceDialog *) data;
  value = BOUNDS (((int) atof (str)), -100, 100);

  if ((int) cbd->magenta_green[cbd->application_mode] != value)
    {
      cbd->magenta_green[cbd->application_mode] = value;
      color_balance_update (cbd, MG_SLIDER);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}

static void
color_balance_yb_text_update (GtkWidget *w,
			      gpointer   data)
{
  ColorBalanceDialog *cbd;
  char *str;
  int value;

  str = gtk_entry_get_text (GTK_ENTRY (w));
  cbd = (ColorBalanceDialog *) data;
  value = BOUNDS (((int) atof (str)), -100, 100);

  if ((int) cbd->yellow_blue[cbd->application_mode] != value)
    {
      cbd->yellow_blue[cbd->application_mode] = value;
      color_balance_update (cbd, YB_SLIDER);

      if (cbd->preview)
	color_balance_preview (cbd);
    }
}


/*  The color_balance procedure definition  */
ProcArg color_balance_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_INT32,
    "transfer_mode",
    "Transfer mode: { SHADOWS (0), MIDTONES (1), HIGHLIGHTS (2) }"
  },
  { PDB_INT32,
    "preserve_lum",
    "Preserve luminosity values at each pixel"
  },
  { PDB_FLOAT,
    "cyan_red",
    "Cyan-Red color balance: (-100 <= cyan_red <= 100)"
  },
  { PDB_FLOAT,
    "magenta_green",
    "Magenta-Green color balance: (-100 <= magenta_green <= 100)"
  },
  { PDB_FLOAT,
    "yellow_blue",
    "Yellow-Blue color balance: (-100 <= yellow_blue <= 100)"
  }
};

ProcRecord color_balance_proc =
{
  "gimp_color_balance",
  "Modify the color balance of the specified drawable",
  "Modify the color balance of the specified drawable.  There are three axis which can be modified: cyan-red, magenta-green, and yellow-blue.  Negative values increase the amount of the former, positive values increase the amount of the latter.  Color balance can be controlled with the 'transfer_mode' setting, which allows shadows, midtones, and highlights in an image to be affected differently.  The 'preserve_lum' parameter, if non-zero, ensures that the luminosity of each pixel remains fixed.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1997",
  PDB_INTERNAL,

  /*  Input arguments  */
  7,
  color_balance_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { color_balance_invoker } },
};


static Argument *
color_balance_invoker (Argument *args)
{
  PixelRegion srcPR, destPR;
  int success = TRUE;
  int int_value;
  ColorBalanceDialog cbd;
  GImage *gimage;
  int transfer_mode;
  int preserve_lum;
  double cyan_red;
  double magenta_green;
  double yellow_blue;
  double fp_value;
  int x1, y1, x2, y2;
  int i;
  void *pr;
  GimpDrawable *drawable;

  drawable      = NULL;
  transfer_mode = MIDTONES;
  cyan_red      = 0;
  magenta_green = 0;
  yellow_blue   = 0;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  make sure the drawable is not indexed color  */
  if (success)
    success = ! drawable_indexed (drawable);

  /*  transfer_mode  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      switch (int_value)
	{
	case 0: transfer_mode = SHADOWS; break;
	case 1: transfer_mode = MIDTONES; break;
	case 2: transfer_mode = HIGHLIGHTS; break;
	default: success = FALSE; break;
	}
    }
  /*  preserve_lum  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      preserve_lum = (int_value) ? TRUE : FALSE;
    }
  /*  cyan_red  */
  if (success)
    {
      fp_value = args[4].value.pdb_float;
      if (fp_value >= -100 && fp_value <= 100)
	cyan_red = fp_value;
      else
	success = FALSE;
    }
  /*  magenta_green  */
  if (success)
    {
      fp_value = args[5].value.pdb_float;
      if (fp_value >= -100 && fp_value <= 100)
	magenta_green = fp_value;
      else
	success = FALSE;
    }
  /*  yellow_blue  */
  if (success)
    {
      fp_value = args[6].value.pdb_float;
      if (fp_value >= -100 && fp_value <= 100)
	yellow_blue = fp_value;
      else
	success = FALSE;
    }

  /*  arrange to modify the color balance  */
  if (success)
    {
      for (i = 0; i < 3; i++)
	{
	  cbd.cyan_red[i] = 0.0;
	  cbd.magenta_green[i] = 0.0;
	  cbd.yellow_blue[i] = 0.0;
	}

      cbd.preserve_luminosity = preserve_lum;
      cbd.cyan_red[transfer_mode] = cyan_red;
      cbd.magenta_green[transfer_mode] = magenta_green;
      cbd.yellow_blue[transfer_mode] = yellow_blue;

      /*  The application should occur only within selection bounds  */
      drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, drawable_shadow (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      for (pr = pixel_regions_register (2, &srcPR, &destPR); pr != NULL; pr = pixel_regions_process (pr))
	color_balance (&srcPR, &destPR, (void *) &cbd);

      drawable_merge_shadow (drawable, TRUE);
      drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));
    }

  return procedural_db_return_args (&color_balance_proc, success);
}
