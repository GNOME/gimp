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
#include "appenv.h"
#include "actionarea.h"
#include "color_select.h"
#include "libgimp/color_selector.h"
#include "colormaps.h"
#include "errors.h"
#include "gimprc.h"
#include "session.h"
#include "color_area.h" /* for color_area_draw_rect */

#include "libgimp/gimpintl.h"

#define XY_DEF_WIDTH       240
#define XY_DEF_HEIGHT      240
#define Z_DEF_WIDTH        15
#define Z_DEF_HEIGHT       240
#define COLOR_AREA_WIDTH   74
#define COLOR_AREA_HEIGHT  20

#define COLOR_AREA_MASK GDK_EXPOSURE_MASK | \
                        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | \
			GDK_BUTTON1_MOTION_MASK | GDK_ENTER_NOTIFY_MASK

typedef enum {
  HUE = 0,
  SATURATION,
  VALUE,
  RED,
  GREEN,
  BLUE,
  HUE_SATURATION,
  HUE_VALUE,
  SATURATION_VALUE,
  RED_GREEN,
  RED_BLUE,
  GREEN_BLUE
} ColorSelectFillType;

typedef enum {
  UPDATE_VALUES = 1 << 0,
  UPDATE_POS = 1 << 1,
  UPDATE_XY_COLOR = 1 << 2,
  UPDATE_Z_COLOR = 1 << 3,
  UPDATE_NEW_COLOR = 1 << 4,
  UPDATE_ORIG_COLOR = 1 << 5,
  UPDATE_CALLER = 1 << 6
} ColorSelectUpdateType;

typedef struct _ColorSelectFill ColorSelectFill;
typedef void (*ColorSelectFillUpdateProc) (ColorSelectFill *);

struct _ColorSelectFill {
  unsigned char *buffer;
  int y;
  int width;
  int height;
  int *values;
  ColorSelectFillUpdateProc update;
};

static GtkWidget * color_select_widget_new (ColorSelectP, int, int, int);

static void color_select_update (ColorSelectP, ColorSelectUpdateType);
static void color_select_update_caller (ColorSelectP);
static void color_select_update_values (ColorSelectP);
static void color_select_update_rgb_values (ColorSelectP);
static void color_select_update_hsv_values (ColorSelectP);
static void color_select_update_pos (ColorSelectP);
static void color_select_update_sliders (ColorSelectP, int);
static void color_select_update_entries (ColorSelectP, int);
static void color_select_update_colors (ColorSelectP, int);

static void color_select_ok_callback (GtkWidget *, gpointer);
static void color_select_cancel_callback (GtkWidget *, gpointer);
static gint color_select_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static gint color_select_xy_expose (GtkWidget *, GdkEventExpose *, ColorSelectP);
static gint color_select_xy_events (GtkWidget *, GdkEvent *, ColorSelectP);
static gint color_select_z_expose (GtkWidget *, GdkEventExpose *, ColorSelectP);
static gint color_select_z_events (GtkWidget *, GdkEvent *, ColorSelectP);
static gint color_select_color_events (GtkWidget *, GdkEvent *);
static void color_select_slider_update (GtkAdjustment *, gpointer);
static void color_select_entry_update (GtkWidget *, gpointer);
static void color_select_toggle_update (GtkWidget *, gpointer);
static gint color_select_hex_entry_leave (GtkWidget *, GdkEvent *, gpointer);

static void color_select_image_fill (GtkWidget *, ColorSelectFillType, int *);

static void color_select_draw_z_marker (ColorSelectP, GdkRectangle *);
static void color_select_draw_xy_marker (ColorSelectP, GdkRectangle *);

static void color_select_update_red (ColorSelectFill *);
static void color_select_update_green (ColorSelectFill *);
static void color_select_update_blue (ColorSelectFill *);
static void color_select_update_hue (ColorSelectFill *);
static void color_select_update_saturation (ColorSelectFill *);
static void color_select_update_value (ColorSelectFill *);
static void color_select_update_red_green (ColorSelectFill *);
static void color_select_update_red_blue (ColorSelectFill *);
static void color_select_update_green_blue (ColorSelectFill *);
static void color_select_update_hue_saturation (ColorSelectFill *);
static void color_select_update_hue_value (ColorSelectFill *);
static void color_select_update_saturation_value (ColorSelectFill *);

static GtkWidget * color_select_notebook_new (int, int, int,
					      GimpColorSelector_Callback,
					      void *, void **);
static void color_select_notebook_free (void *);
static void color_select_notebook_setcolor (void *, int, int, int, int);
static void color_select_notebook_update_callback (int, int, int,
						   ColorSelectState, void *);





static ColorSelectFillUpdateProc update_procs[] =
{
  color_select_update_hue,
  color_select_update_saturation,
  color_select_update_value,
  color_select_update_red,
  color_select_update_green,
  color_select_update_blue,
  color_select_update_hue_saturation,
  color_select_update_hue_value,
  color_select_update_saturation_value,
  color_select_update_red_green,
  color_select_update_red_blue,
  color_select_update_green_blue,
};

static ActionAreaItem action_items[2] =
{
  { N_("OK"), color_select_ok_callback, NULL, NULL },
  { N_("Cancel"), color_select_cancel_callback, NULL, NULL }
};

ColorSelectP
color_select_new (int                  r,
		  int                  g,
		  int                  b,
		  ColorSelectCallback  callback,
		  void                *client_data,
		  int                  wants_updates)
{
  ColorSelectP csp;
  GtkWidget *main_vbox;

  csp = g_malloc (sizeof (_ColorSelect));

  csp->callback = callback;
  csp->client_data = client_data;
  csp->z_color_fill = HUE;
  csp->xy_color_fill = SATURATION_VALUE;
  csp->gc = NULL;
  csp->wants_updates = wants_updates;

  csp->values[RED] = csp->orig_values[0] = r;
  csp->values[GREEN] = csp->orig_values[1] = g;
  csp->values[BLUE] = csp->orig_values[2] = b;
  color_select_update_hsv_values (csp);
  color_select_update_pos (csp);

  csp->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (csp->shell), "color_selection", "Gimp");
  gtk_window_set_title (GTK_WINDOW (csp->shell), _("Color Selection"));
  gtk_window_set_policy (GTK_WINDOW (csp->shell), FALSE, FALSE, FALSE);


  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (csp->shell), "delete_event",
		      (GtkSignalFunc) color_select_delete_callback, csp);
  
  main_vbox = color_select_widget_new (csp, r, g, b);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (csp->shell)->vbox), main_vbox, TRUE, TRUE, 0);

  /*  The action area  */
  action_items[0].user_data = csp;
  action_items[1].user_data = csp;
  if (csp->wants_updates)
    {
      action_items[0].label = _("Close");
      action_items[1].label = _("Revert to Old Color");
    }
  else
    {
      action_items[0].label = _("OK");
      action_items[1].label = _("Cancel");
    }
  build_action_area (GTK_DIALOG (csp->shell), action_items, 2, 0);

  color_select_image_fill (csp->z_color, csp->z_color_fill, csp->values);
  color_select_image_fill (csp->xy_color, csp->xy_color_fill, csp->values);

  gtk_widget_show (csp->shell);

  return csp;
}

static GtkWidget *
color_select_widget_new (ColorSelectP csp, int r, int g, int b)
{
  static char *toggle_titles[6] = { "H", "S", "V", "R", "G", "B" };
  static gfloat slider_max_vals[6] = { 360, 100, 100, 255, 255, 255 };
  static gfloat slider_incs[6] = { 0.1, 0.1, 0.1, 1.0, 1.0, 1.0 };

  GtkWidget *main_vbox;
  GtkWidget *main_hbox;
  GtkWidget *xy_frame;
  GtkWidget *z_frame;
  GtkWidget *colors_frame;
  GtkWidget *colors_hbox;
  GtkWidget *right_vbox;
  GtkWidget *table;
  GtkWidget *slider;
  GtkWidget *hex_hbox;
  GtkWidget *label;
  GSList *group;
  char buffer[16];
  int i;

  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 2);

  main_hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hbox, TRUE, TRUE, 2);
  gtk_widget_show (main_hbox);

  xy_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (xy_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (main_hbox), xy_frame, FALSE, FALSE, 2);
  gtk_widget_show (xy_frame);

  csp->xy_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (csp->xy_color), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (csp->xy_color), XY_DEF_WIDTH, XY_DEF_HEIGHT);
  gtk_widget_set_events (csp->xy_color, COLOR_AREA_MASK);
  gtk_signal_connect_after (GTK_OBJECT (csp->xy_color), "expose_event",
			    (GtkSignalFunc) color_select_xy_expose,
			    csp);
  gtk_signal_connect (GTK_OBJECT (csp->xy_color), "event",
		      (GtkSignalFunc) color_select_xy_events,
		      csp);
  gtk_container_add (GTK_CONTAINER (xy_frame), csp->xy_color);
  gtk_widget_show (csp->xy_color);

  z_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (z_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (main_hbox), z_frame, FALSE, FALSE, 2);
  gtk_widget_show (z_frame);

  csp->z_color = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (csp->z_color), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (csp->z_color), Z_DEF_WIDTH, Z_DEF_HEIGHT);
  gtk_widget_set_events (csp->z_color, COLOR_AREA_MASK);
  gtk_signal_connect_after (GTK_OBJECT (csp->z_color), "expose_event",
			    (GtkSignalFunc) color_select_z_expose,
			    csp);
  gtk_signal_connect (GTK_OBJECT (csp->z_color), "event",
		      (GtkSignalFunc) color_select_z_events,
		      csp);
  gtk_container_add (GTK_CONTAINER (z_frame), csp->z_color);
  gtk_widget_show (csp->z_color);

  /*  The right vertical box with old/new color area and color space sliders  */
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (right_vbox), 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);

  /*  The old/new color area  */
  colors_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (colors_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (right_vbox), colors_frame, FALSE, FALSE, 0);
  gtk_widget_show (colors_frame);

  colors_hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (colors_frame), colors_hbox);
  gtk_widget_show (colors_hbox);

  csp->new_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (csp->new_color), COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (csp->new_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (csp->new_color), "event",
		      (GtkSignalFunc) color_select_color_events,
		      csp);
  gtk_object_set_user_data (GTK_OBJECT (csp->new_color), csp);
  gtk_box_pack_start (GTK_BOX (colors_hbox), csp->new_color, TRUE, TRUE, 0);
  gtk_widget_show (csp->new_color);

  csp->orig_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (csp->orig_color), COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (csp->orig_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (csp->orig_color), "event",
		      (GtkSignalFunc) color_select_color_events,
		      csp);
  gtk_object_set_user_data (GTK_OBJECT (csp->orig_color), csp);
  gtk_box_pack_start (GTK_BOX (colors_hbox), csp->orig_color, TRUE, TRUE, 0);
  gtk_widget_show (csp->orig_color);

  /*  The color space sliders, toggle buttons and entries  */
  table = gtk_table_new (6, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);
  gtk_container_border_width (GTK_CONTAINER (table), 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  group = NULL;
  for (i = 0; i < 6; i++)
    {
      csp->toggles[i] = gtk_radio_button_new_with_label (group, toggle_titles[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (csp->toggles[i]));
      gtk_table_attach (GTK_TABLE (table), csp->toggles[i],
			0, 1, i, i+1, GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_signal_connect (GTK_OBJECT (csp->toggles[i]), "toggled",
			  (GtkSignalFunc) color_select_toggle_update,
			  csp);
      gtk_widget_show (csp->toggles[i]);

      csp->slider_data[i] = GTK_ADJUSTMENT (gtk_adjustment_new (csp->values[i], 0.0,
								slider_max_vals[i],
								slider_incs[i],
								1.0, 0.0));

      slider = gtk_hscale_new (csp->slider_data[i]);
      gtk_table_attach (GTK_TABLE (table), slider, 1, 2, i, i+1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
      gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
      gtk_signal_connect (GTK_OBJECT (csp->slider_data[i]), "value_changed",
			  (GtkSignalFunc) color_select_slider_update,
			  csp);
      gtk_widget_show (slider);

      csp->entries[i] = gtk_entry_new ();
      sprintf (buffer, "%d", csp->values[i]);
      gtk_entry_set_text (GTK_ENTRY (csp->entries[i]), buffer);
      gtk_widget_set_usize (GTK_WIDGET (csp->entries[i]), 40, 0);
      gtk_table_attach (GTK_TABLE (table), csp->entries[i],
			2, 3, i, i+1, GTK_FILL, GTK_EXPAND, 0, 0);
      gtk_signal_connect (GTK_OBJECT (csp->entries[i]), "changed",
			  (GtkSignalFunc) color_select_entry_update,
			  csp);
      gtk_widget_show (csp->entries[i]);
    }

  /* The hex triplet entry */
  hex_hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (right_vbox), hex_hbox, FALSE, FALSE, 0);
  gtk_widget_show (hex_hbox);

  csp->hex_entry = gtk_entry_new ();
  sprintf(buffer, "#%.2x%.2x%.2x", r, g, b);
  gtk_entry_set_text (GTK_ENTRY (csp->hex_entry), buffer);
  gtk_widget_set_usize (GTK_WIDGET (csp->hex_entry), 75, 0);
  gtk_box_pack_end (GTK_BOX (hex_hbox), csp->hex_entry, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (csp->hex_entry), "focus_out_event",
		      (GtkSignalFunc) color_select_hex_entry_leave,
		      csp);
  gtk_widget_show (csp->hex_entry);

  label = gtk_label_new (_("Hex Triplet:"));
  gtk_box_pack_end (GTK_BOX (hex_hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return main_vbox;
}


/* Register the GIMP colour selector with the color notebook */
void
color_select_init (void)
{
  GimpColorSelectorMethods methods =
  {
    color_select_notebook_new,
    color_select_notebook_free,
    color_select_notebook_setcolor
  };

  gimp_color_selector_register ("GIMP", &methods);
}


void
color_select_show (ColorSelectP csp)
{
  if (csp)
    gtk_widget_show (csp->shell);
}

void
color_select_hide (ColorSelectP csp)
{
  if (csp)
    gtk_widget_hide (csp->shell);
}

void
color_select_free (ColorSelectP csp)
{
  if (csp)
    {
      if (csp->shell)
	gtk_widget_destroy (csp->shell);
      gdk_gc_destroy (csp->gc);
      g_free (csp);
    }
}

void
color_select_set_color (ColorSelectP csp,
			int          r,
			int          g,
			int          b,
			int          set_current)
{
  if (csp)
    {
      csp->orig_values[0] = r;
      csp->orig_values[1] = g;
      csp->orig_values[2] = b;

      color_select_update_colors (csp, 1);

      if (set_current)
	{
	  csp->values[RED] = r;
	  csp->values[GREEN] = g;
	  csp->values[BLUE] = b;

	  color_select_update_hsv_values (csp);
	  color_select_update_pos (csp);
	  color_select_update_sliders (csp, -1);
	  color_select_update_entries (csp, -1);
	  color_select_update_colors (csp, 0);

	  color_select_update (csp, UPDATE_Z_COLOR);
	  color_select_update (csp, UPDATE_XY_COLOR);
	}
    }
}

static void
color_select_update (ColorSelectP          csp,
		     ColorSelectUpdateType update)
{
  if (csp)
    {
      if (update & UPDATE_POS)
	color_select_update_pos (csp);

      if (update & UPDATE_VALUES)
	{
	  color_select_update_values (csp);
	  color_select_update_sliders (csp, -1);
	  color_select_update_entries (csp, -1);

	  if (!(update & UPDATE_NEW_COLOR))
	    color_select_update_colors (csp, 0);
	}

      if (update & UPDATE_XY_COLOR)
	{
	  color_select_image_fill (csp->xy_color, csp->xy_color_fill, csp->values);
	  gtk_widget_draw (csp->xy_color, NULL);
	}

      if (update & UPDATE_Z_COLOR)
	{
	  color_select_image_fill (csp->z_color, csp->z_color_fill, csp->values);
	  gtk_widget_draw (csp->z_color, NULL);
	}

      if (update & UPDATE_NEW_COLOR)
	color_select_update_colors (csp, 0);

      if (update & UPDATE_ORIG_COLOR)
	color_select_update_colors (csp, 1);

      /*if (update & UPDATE_CALLER)*/
      color_select_update_caller (csp);
    }
}

static void
color_select_update_caller (ColorSelectP csp)
{
  if (csp && csp->wants_updates && csp->callback)
    {
      (* csp->callback) (csp->values[RED],
			 csp->values[GREEN],
			 csp->values[BLUE],
			 COLOR_SELECT_UPDATE,
			 csp->client_data);
    }
}

static void
color_select_update_values (ColorSelectP csp)
{
  if (csp)
    {
      switch (csp->z_color_fill)
	{
	case RED:
	  csp->values[BLUE] = csp->pos[0];
	  csp->values[GREEN] = csp->pos[1];
	  csp->values[RED] = csp->pos[2];
	  break;
	case GREEN:
	  csp->values[BLUE] = csp->pos[0];
	  csp->values[RED] = csp->pos[1];
	  csp->values[GREEN] = csp->pos[2];
	  break;
	case BLUE:
	  csp->values[GREEN] = csp->pos[0];
	  csp->values[RED] = csp->pos[1];
	  csp->values[BLUE] = csp->pos[2];
	  break;
	case HUE:
	  csp->values[VALUE] = csp->pos[0] * 100 / 255;
	  csp->values[SATURATION] = csp->pos[1] * 100 / 255;
	  csp->values[HUE] = csp->pos[2] * 360 / 255;
	  break;
	case SATURATION:
	  csp->values[VALUE] = csp->pos[0] * 100 / 255;
	  csp->values[HUE] = csp->pos[1] * 360 / 255;
	  csp->values[SATURATION] = csp->pos[2] * 100 / 255;
	  break;
	case VALUE:
	  csp->values[SATURATION] = csp->pos[0] * 100 / 255;
	  csp->values[HUE] = csp->pos[1] * 360 / 255;
	  csp->values[VALUE] = csp->pos[2] * 100 / 255;
	  break;
	}

      switch (csp->z_color_fill)
	{
	case RED:
	case GREEN:
	case BLUE:
	  color_select_update_hsv_values (csp);
	  break;
	case HUE:
	case SATURATION:
	case VALUE:
	  color_select_update_rgb_values (csp);
	  break;
	}
    }
}

static void
color_select_update_rgb_values (ColorSelectP csp)
{
  float h, s, v;
  float f, p, q, t;

  if (csp)
    {
      h = csp->values[HUE];
      s = csp->values[SATURATION] / 100.0;
      v = csp->values[VALUE] / 100.0;

      if (s == 0)
	{
	  csp->values[RED] = v * 255;
	  csp->values[GREEN] = v * 255;
	  csp->values[BLUE] = v * 255;
	}
      else
	{
	  if (h == 360)
	    h = 0;

	  h /= 60;
	  f = h - (int) h;
	  p = v * (1 - s);
	  q = v * (1 - (s * f));
	  t = v * (1 - (s * (1 - f)));

	  switch ((int) h)
	    {
	    case 0:
	      csp->values[RED] = v * 255;
	      csp->values[GREEN] = t * 255;
	      csp->values[BLUE] = p * 255;
	      break;
	    case 1:
	      csp->values[RED] = q * 255;
	      csp->values[GREEN] = v * 255;
	      csp->values[BLUE] = p * 255;
	      break;
	    case 2:
	      csp->values[RED] = p * 255;
	      csp->values[GREEN] = v * 255;
	      csp->values[BLUE] = t * 255;
	      break;
	    case 3:
	      csp->values[RED] = p * 255;
	      csp->values[GREEN] = q * 255;
	      csp->values[BLUE] = v * 255;
	      break;
	    case 4:
	      csp->values[RED] = t * 255;
	      csp->values[GREEN] = p * 255;
	      csp->values[BLUE] = v * 255;
	      break;
	    case 5:
	      csp->values[RED] = v * 255;
	      csp->values[GREEN] = p * 255;
	      csp->values[BLUE] = q * 255;
	      break;
	    }
	}
    }
}

static void
color_select_update_hsv_values (ColorSelectP csp)
{
  int r, g, b;
  float h, s, v;
  int min, max;
  int delta;

  if (csp)
    {
      r = csp->values[RED];
      g = csp->values[GREEN];
      b = csp->values[BLUE];

      if (r > g)
	{
	  if (r > b)
	    max = r;
	  else
	    max = b;

	  if (g < b)
	    min = g;
	  else
	    min = b;
	}
      else
	{
	  if (g > b)
	    max = g;
	  else
	    max = b;

	  if (r < b)
	    min = r;
	  else
	    min = b;
	}

      v = max;

      if (max != 0)
	s = (max - min) / (float) max;
      else
	s = 0;

      if (s == 0)
	h = 0;
      else
	{
	  h = 0;
	  delta = max - min;
	  if (r == max)
	    h = (g - b) / (float) delta;
	  else if (g == max)
	    h = 2 + (b - r) / (float) delta;
	  else if (b == max)
	    h = 4 + (r - g) / (float) delta;
	  h *= 60;

	  if (h < 0)
	    h += 360;
	}

      csp->values[HUE] = h;
      csp->values[SATURATION] = s * 100;
      csp->values[VALUE] = v * 100 / 255;
    }
}

static void
color_select_update_pos (ColorSelectP csp)
{
  if (csp)
    {
      switch (csp->z_color_fill)
	{
	case RED:
	  csp->pos[0] = csp->values[BLUE];
	  csp->pos[1] = csp->values[GREEN];
	  csp->pos[2] = csp->values[RED];
	  break;
	case GREEN:
	  csp->pos[0] = csp->values[BLUE];
	  csp->pos[1] = csp->values[RED];
	  csp->pos[2] = csp->values[GREEN];
	  break;
	case BLUE:
	  csp->pos[0] = csp->values[GREEN];
	  csp->pos[1] = csp->values[RED];
	  csp->pos[2] = csp->values[BLUE];
	  break;
	case HUE:
	  csp->pos[0] = csp->values[VALUE] * 255 / 100;
	  csp->pos[1] = csp->values[SATURATION] * 255 / 100;
	  csp->pos[2] = csp->values[HUE] * 255 / 360;
	  break;
	case SATURATION:
	  csp->pos[0] = csp->values[VALUE] * 255 / 100;
	  csp->pos[1] = csp->values[HUE] * 255 / 360;
	  csp->pos[2] = csp->values[SATURATION] * 255 / 100;
	  break;
	case VALUE:
	  csp->pos[0] = csp->values[SATURATION] * 255 / 100;
	  csp->pos[1] = csp->values[HUE] * 255 / 360;
	  csp->pos[2] = csp->values[VALUE] * 255 / 100;
	  break;
	}
    }
}

static void
color_select_update_sliders (ColorSelectP csp,
			     int          skip)
{
  int i;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (i != skip)
	  {
	    csp->slider_data[i]->value = (gfloat) csp->values[i];

	    gtk_signal_handler_block_by_data (GTK_OBJECT (csp->slider_data[i]), csp);
	    gtk_signal_emit_by_name (GTK_OBJECT (csp->slider_data[i]), "value_changed");
	    gtk_signal_handler_unblock_by_data (GTK_OBJECT (csp->slider_data[i]), csp);
	  }
    }
}

static void
color_select_update_entries (ColorSelectP csp,
			     int          skip)
{
  char buffer[16];
  int i;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (i != skip)
	  {
	    sprintf (buffer, "%d", csp->values[i]);

	    gtk_signal_handler_block_by_data (GTK_OBJECT (csp->entries[i]), csp);
	    gtk_entry_set_text (GTK_ENTRY (csp->entries[i]), buffer);
	    gtk_signal_handler_unblock_by_data (GTK_OBJECT (csp->entries[i]), csp);
	  }

      sprintf(buffer, "#%.2x%.2x%.2x",
	      csp->values[RED],
	      csp->values[GREEN],
	      csp->values[BLUE]);
      gtk_entry_set_text (GTK_ENTRY (csp->hex_entry), buffer);
    }
}

static void
color_select_update_colors (ColorSelectP csp,
			    int          which)
{
  GdkWindow *window;
  GdkColor color;
  int red, green, blue;
  int width, height;

  if (csp)
    {
      if (which)
	{
	  window = csp->orig_color->window;
	  color.pixel = old_color_pixel;
	  red = csp->orig_values[0];
	  green = csp->orig_values[1];
	  blue = csp->orig_values[2];
	}
      else
	{
	  window = csp->new_color->window;
	  color.pixel = new_color_pixel;
	  red = csp->values[RED];
	  green = csp->values[GREEN];
	  blue = csp->values[BLUE];
	}

      /* if we haven't yet been realised, there's no need to redraw
       * anything. */
      if (!window) return;

      store_color (&color.pixel, red, green, blue);

      gdk_window_get_size (window, &width, &height);

      if (csp->gc)
	{
#ifdef OLD_COLOR_AREA
	  gdk_gc_set_foreground (csp->gc, &color);
	  gdk_draw_rectangle (window, csp->gc, 1,
			      0, 0, width, height);
#else
	  color_area_draw_rect (window, csp->gc,
				0, 0, width, height,
				red, green, blue);
#endif
	}
    }
}

static void
color_select_ok_callback (GtkWidget *w,
			  gpointer   client_data)
{
  ColorSelectP csp;

  csp = (ColorSelectP) client_data;
  if (csp)
    {
      if (csp->callback)
	(* csp->callback) (csp->values[RED],
			   csp->values[GREEN],
			   csp->values[BLUE],
			   COLOR_SELECT_OK,
			   csp->client_data);
    }
}

static gint
color_select_delete_callback (GtkWidget *w,
			      GdkEvent  *e,
			      gpointer   client_data)
{
  color_select_cancel_callback (w, client_data);

  return TRUE;
}
  

static void
color_select_cancel_callback (GtkWidget *w,
			      gpointer   client_data)
{
  ColorSelectP csp;

  csp = (ColorSelectP) client_data;
  if (csp)
    {
      if (csp->callback)
	(* csp->callback) (csp->orig_values[0],
			   csp->orig_values[1],
			   csp->orig_values[2],
			   COLOR_SELECT_CANCEL,
			   csp->client_data);
    }
}

static gint
color_select_xy_expose (GtkWidget      *widget,
			GdkEventExpose *event,
			ColorSelectP    csp)
{
  if (!csp->gc)
    csp->gc = gdk_gc_new (widget->window);

  color_select_draw_xy_marker (csp, &event->area);

  return FALSE;
}

static gint
color_select_xy_events (GtkWidget    *widget,
			GdkEvent     *event,
			ColorSelectP  csp)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (bevent->x * 255) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 255 - (bevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 255)
	csp->pos[0] = 255;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 255)
	csp->pos[1] = 255;

      gdk_pointer_grab (csp->xy_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      color_select_draw_xy_marker (csp, NULL);

      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (bevent->x * 255) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 255 - (bevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 255)
	csp->pos[0] = 255;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 255)
	csp->pos[1] = 255;

      gdk_pointer_ungrab (bevent->time);
      color_select_draw_xy_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      color_select_draw_xy_marker (csp, NULL);

      csp->pos[0] = (mevent->x * 255) / (XY_DEF_WIDTH - 1);
      csp->pos[1] = 255 - (mevent->y * 255) / (XY_DEF_HEIGHT - 1);

      if (csp->pos[0] < 0)
	csp->pos[0] = 0;
      if (csp->pos[0] > 255)
	csp->pos[0] = 255;
      if (csp->pos[1] < 0)
	csp->pos[1] = 0;
      if (csp->pos[1] > 255)
	csp->pos[1] = 255;

      color_select_draw_xy_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
color_select_z_expose (GtkWidget      *widget,
		       GdkEventExpose *event,
		       ColorSelectP    csp)
{
  if (!csp->gc)
    csp->gc = gdk_gc_new (widget->window);

  color_select_draw_z_marker (csp, &event->area);

  return FALSE;
}

static gint
color_select_z_events (GtkWidget    *widget,
		       GdkEvent     *event,
		       ColorSelectP  csp)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  int tx, ty;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 255 - (bevent->y * 255) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 255)
	csp->pos[2] = 255;

      gdk_pointer_grab (csp->z_color->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);
      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 255 - (bevent->y * 255) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 255)
	csp->pos[2] = 255;

      gdk_pointer_ungrab (bevent->time);
      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES | UPDATE_XY_COLOR);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      if (mevent->is_hint)
	{
	  gdk_window_get_pointer (widget->window, &tx, &ty, NULL);
	  mevent->x = tx;
	  mevent->y = ty;
	}

      color_select_draw_z_marker (csp, NULL);

      csp->pos[2] = 255 - (mevent->y * 255) / (Z_DEF_HEIGHT - 1);
      if (csp->pos[2] < 0)
	csp->pos[2] = 0;
      if (csp->pos[2] > 255)
	csp->pos[2] = 255;

      color_select_draw_z_marker (csp, NULL);
      color_select_update (csp, UPDATE_VALUES);
      break;

    default:
      break;
    }

  return FALSE;
}

static gint
color_select_color_events (GtkWidget *widget,
			   GdkEvent  *event)
{
  ColorSelectP csp;

  csp = (ColorSelectP) gtk_object_get_user_data (GTK_OBJECT (widget));
  if (!csp)
    return FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!csp->gc)
	csp->gc = gdk_gc_new (widget->window);

      if (widget == csp->new_color)
	color_select_update (csp, UPDATE_NEW_COLOR);
      else if (widget == csp->orig_color)
	color_select_update (csp, UPDATE_ORIG_COLOR);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
color_select_slider_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  ColorSelectP csp;
  int old_values[6];
  int update_z_marker;
  int update_xy_marker;
  int i, j;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (csp->slider_data[i] == adjustment)
	  break;

      for (j = 0; j < 6; j++)
	old_values[j] = csp->values[j];

      csp->values[i] = (int) adjustment->value;

      if ((i >= HUE) && (i <= VALUE))
	color_select_update_rgb_values (csp);
      else if ((i >= RED) && (i <= BLUE))
	color_select_update_hsv_values (csp);
      color_select_update_sliders (csp, i);
      color_select_update_entries (csp, -1);

      update_z_marker = 0;
      update_xy_marker = 0;
      for (j = 0; j < 6; j++)
	{
	  if (j == csp->z_color_fill)
	    {
	      if (old_values[j] != csp->values[j])
		update_z_marker = 1;
	    }
	  else
	    {
	      if (old_values[j] != csp->values[j])
		update_xy_marker = 1;
	    }
	}

      if (update_z_marker)
	{
	  color_select_draw_z_marker (csp, NULL);
	  color_select_update (csp, UPDATE_POS | UPDATE_XY_COLOR);
	  color_select_draw_z_marker (csp, NULL);
	}
      else
	{
	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);

	  color_select_update (csp, UPDATE_POS);

	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);
	}

      color_select_update (csp, UPDATE_NEW_COLOR);
    }
}

static void
color_select_entry_update (GtkWidget *w,
			   gpointer   data)
{
  ColorSelectP csp;
  int old_values[6];
  int update_z_marker;
  int update_xy_marker;
  int i, j;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (csp->entries[i] == w)
	  break;

      for (j = 0; j < 6; j++)
	old_values[j] = csp->values[j];

      csp->values[i] = atoi (gtk_entry_get_text (GTK_ENTRY (csp->entries[i])));
      if (csp->values[i] == old_values[i])
	return;

      if ((i >= HUE) && (i <= VALUE))
	color_select_update_rgb_values (csp);
      else if ((i >= RED) && (i <= BLUE))
	color_select_update_hsv_values (csp);
      color_select_update_entries (csp, i);
      color_select_update_sliders (csp, -1);

      update_z_marker = 0;
      update_xy_marker = 0;
      for (j = 0; j < 6; j++)
	{
	  if (j == csp->z_color_fill)
	    {
	      if (old_values[j] != csp->values[j])
		update_z_marker = 1;
	    }
	  else
	    {
	      if (old_values[j] != csp->values[j])
		update_xy_marker = 1;
	    }
	}

      if (update_z_marker)
	{
	  color_select_draw_z_marker (csp, NULL);
	  color_select_update (csp, UPDATE_POS | UPDATE_XY_COLOR);
	  color_select_draw_z_marker (csp, NULL);
	}
      else
	{
	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);

	  color_select_update (csp, UPDATE_POS);

	  if (update_z_marker)
	    color_select_draw_z_marker (csp, NULL);
	  if (update_xy_marker)
	    color_select_draw_xy_marker (csp, NULL);
	}

      color_select_update (csp, UPDATE_NEW_COLOR);
    }
}

static void
color_select_toggle_update (GtkWidget *w,
			    gpointer   data)
{
  ColorSelectP csp;
  ColorSelectFillType type = HUE;
  int i;

  if (!GTK_TOGGLE_BUTTON (w)->active)
    return;

  csp = (ColorSelectP) data;

  if (csp)
    {
      for (i = 0; i < 6; i++)
	if (w == csp->toggles[i])
	  type = (ColorSelectFillType) i;

      switch (type)
	{
	case HUE:
	  csp->z_color_fill = HUE;
	  csp->xy_color_fill = SATURATION_VALUE;
	  break;
	case SATURATION:
	  csp->z_color_fill = SATURATION;
	  csp->xy_color_fill = HUE_VALUE;
	  break;
	case VALUE:
	  csp->z_color_fill = VALUE;
	  csp->xy_color_fill = HUE_SATURATION;
	  break;
	case RED:
	  csp->z_color_fill = RED;
	  csp->xy_color_fill = GREEN_BLUE;
	  break;
	case GREEN:
	  csp->z_color_fill = GREEN;
	  csp->xy_color_fill = RED_BLUE;
	  break;
	case BLUE:
	  csp->z_color_fill = BLUE;
	  csp->xy_color_fill = RED_GREEN;
	  break;
	default:
	  break;
	}

      color_select_update (csp, UPDATE_POS);
      color_select_update (csp, UPDATE_Z_COLOR | UPDATE_XY_COLOR);
    }
}

static gint
color_select_hex_entry_leave (GtkWidget *w,
			      GdkEvent  *event,
			      gpointer   data)
{
  ColorSelectP csp;
  gchar buffer[8];
  gchar *hex_color;
  guint hex_rgb;

  csp = (ColorSelectP) data;

  if (csp)
    {
      hex_color = g_strdup (gtk_entry_get_text (GTK_ENTRY (csp->hex_entry)));

      sprintf(buffer, "#%.2x%.2x%.2x",
	      csp->values[RED],
	      csp->values[GREEN],
	      csp->values[BLUE]);

      if ((strlen (hex_color) == 7) &&
	  (g_strcasecmp (buffer, hex_color) != 0))
	{
	  if ((sscanf (hex_color, "#%x", &hex_rgb) == 1) &&
	      (hex_rgb < (1 << 24)))
	    color_select_set_color (csp,
				    (hex_rgb & 0xff0000) >> 16,
				    (hex_rgb & 0x00ff00) >> 8,
				    hex_rgb & 0x0000ff,
				    TRUE);
	}

      g_free (hex_color);
    }

  return FALSE;
}

static void
color_select_image_fill (GtkWidget           *preview,
			 ColorSelectFillType  type,
			 int                 *values)
{
  ColorSelectFill csf;
  int height;

  csf.buffer = g_malloc (preview->requisition.width * 3);

  csf.update = update_procs[type];

  csf.y = -1;
  csf.width = preview->requisition.width;
  csf.height = preview->requisition.height;
  csf.values = values;

  height = csf.height;
  if (height > 0)
    while (height--)
      {
	(* csf.update) (&csf);
	gtk_preview_draw_row (GTK_PREVIEW (preview), csf.buffer, 0, csf.y, csf.width);
      }

  g_free (csf.buffer);
}

static void
color_select_draw_z_marker (ColorSelectP csp,
			    GdkRectangle *clip)
{
  int width;
  int height;
  int y;
  int minx;
  int miny;

  if (csp->gc)
    {
      y = (Z_DEF_HEIGHT - 1) - ((Z_DEF_HEIGHT - 1) * csp->pos[2]) / 255;
      width = csp->z_color->requisition.width;
      height = csp->z_color->requisition.height;
      minx = 0;
      miny = 0;
      if (width <= 0)
	return;

      if (clip)
        {
	  width  = MIN(width,  clip->x + clip->width);
	  height = MIN(height, clip->y + clip->height);
	  minx   = MAX(0, clip->x);
	  miny   = MAX(0, clip->y);
	}

      if (y >= miny && y < height)
        {
	  gdk_gc_set_function (csp->gc, GDK_INVERT);
	  gdk_draw_line (csp->z_color->window, csp->gc, minx, y, width - 1, y);
	  gdk_gc_set_function (csp->gc, GDK_COPY);
	}
    }
}

static void
color_select_draw_xy_marker (ColorSelectP csp,
			     GdkRectangle *clip)
{
  int width;
  int height;
  int x, y;
  int minx, miny;

  if (csp->gc)
    {
      x = ((XY_DEF_WIDTH - 1) * csp->pos[0]) / 255;
      y = (XY_DEF_HEIGHT - 1) - ((XY_DEF_HEIGHT - 1) * csp->pos[1]) / 255;
      width = csp->xy_color->requisition.width;
      height = csp->xy_color->requisition.height;
      minx = 0;
      miny = 0;
      if ((width <= 0) || (height <= 0))
	return;

      gdk_gc_set_function (csp->gc, GDK_INVERT);

      if (clip)
        {
	  width  = MIN(width,  clip->x + clip->width);
	  height = MIN(height, clip->y + clip->height);
	  minx   = MAX(0, clip->x);
	  miny   = MAX(0, clip->y);
	}

      if (y >= miny && y < height)
	gdk_draw_line (csp->xy_color->window, csp->gc, minx, y, width - 1, y);

      if (x >= minx && x < width)
	gdk_draw_line (csp->xy_color->window, csp->gc, x, miny, x, height - 1);

      gdk_gc_set_function (csp->gc, GDK_COPY);
    }
}

static void
color_select_update_red (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, r;

  p = csf->buffer;

  csf->y += 1;
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = 0;
      *p++ = 0;
    }
}

static void
color_select_update_green (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, g;

  p = csf->buffer;

  csf->y += 1;
  g = (csf->height - csf->y + 1) * 255 / csf->height;

  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = g;
      *p++ = 0;
    }
}

static void
color_select_update_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, b;

  p = csf->buffer;

  csf->y += 1;
  b = (csf->height - csf->y + 1) * 255 / csf->height;

  if (b < 0)
    b = 0;
  if (b > 255)
    b = 255;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = 0;
      *p++ = 0;
      *p++ = b;
    }
}

static void
color_select_update_hue (ColorSelectFill *csf)
{
  unsigned char *p;
  float h, f;
  int r, g, b;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = csf->y * 360 / csf->height;

  h = 360 - h;

  if (h < 0)
    h = 0;
  if (h >= 360)
    h = 0;

  h /= 60;
  f = (h - (int) h) * 255;

  r = g = b = 0;

  switch ((int) h)
    {
    case 0:
      r = 255;
      g = f;
      b = 0;
      break;
    case 1:
      r = 255 - f;
      g = 255;
      b = 0;
      break;
    case 2:
      r = 0;
      g = 255;
      b = f;
      break;
    case 3:
      r = 0;
      g = 255 - f;
      b = 255;
      break;
    case 4:
      r = f;
      g = 0;
      b = 255;
      break;
    case 5:
      r = 255;
      g = 0;
      b = 255 - f;
      break;
    }

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;
    }
}

static void
color_select_update_saturation (ColorSelectFill *csf)
{
  unsigned char *p;
  int s;
  int i;

  p = csf->buffer;

  csf->y += 1;
  s = csf->y * 255 / csf->height;

  if (s < 0)
    s = 0;
  if (s > 255)
    s = 255;

  s = 255 - s;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = s;
      *p++ = s;
      *p++ = s;
    }
}

static void
color_select_update_value (ColorSelectFill *csf)
{
  unsigned char *p;
  int v;
  int i;

  p = csf->buffer;

  csf->y += 1;
  v = csf->y * 255 / csf->height;

  if (v < 0)
    v = 0;
  if (v > 255)
    v = 255;

  v = 255 - v;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = v;
      *p++ = v;
      *p++ = v;
    }
}

static void
color_select_update_red_green (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, r, b;
  float g, dg;

  p = csf->buffer;

  csf->y += 1;
  b = csf->values[BLUE];
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  g = 0;
  dg = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      g += dg;
    }
}

static void
color_select_update_red_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, r, g;
  float b, db;

  p = csf->buffer;

  csf->y += 1;
  g = csf->values[GREEN];
  r = (csf->height - csf->y + 1) * 255 / csf->height;

  if (r < 0)
    r = 0;
  if (r > 255)
    r = 255;

  b = 0;
  db = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      b += db;
    }
}

static void
color_select_update_green_blue (ColorSelectFill *csf)
{
  unsigned char *p;
  int i, g, r;
  float b, db;

  p = csf->buffer;

  csf->y += 1;
  r = csf->values[RED];
  g = (csf->height - csf->y + 1) * 255 / csf->height;

  if (g < 0)
    g = 0;
  if (g > 255)
    g = 255;

  b = 0;
  db = 255.0 / csf->width;

  for (i = 0; i < csf->width; i++)
    {
      *p++ = r;
      *p++ = g;
      *p++ = b;

      b += db;
    }
}

static void
color_select_update_hue_saturation (ColorSelectFill *csf)
{
  unsigned char *p;
  float h, v, s, ds;
  int f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h) * 255;

  s = 0;
  ds = 1.0 / csf->width;

  v = csf->values[VALUE] / 100.0;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  s += ds;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  s += ds;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  s += ds;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  s += ds;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  s += ds;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  s += ds;
	}
      break;
    }
}

static void
color_select_update_hue_value (ColorSelectFill *csf)
{
  unsigned char *p;
  float h, v, dv, s;
  int f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  h = 360 - (csf->y * 360 / csf->height);

  if (h < 0)
    h = 0;
  if (h > 359)
    h = 359;

  h /= 60;
  f = (h - (int) h) * 255;

  v = 0;
  dv = 1.0 / csf->width;

  s = csf->values[SATURATION] / 100.0;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  v += dv;
	}
      break;
    }
}

static void
color_select_update_saturation_value (ColorSelectFill *csf)
{
  unsigned char *p;
  float h, v, dv, s;
  int f;
  int i;

  p = csf->buffer;

  csf->y += 1;
  s = (float) csf->y / csf->height;

  if (s < 0)
    s = 0;
  if (s > 1)
    s = 1;

  s = 1 - s;

  h = (float) csf->values[HUE];
  if (h >= 360)
    h -= 360;
  h /= 60;
  f = (h - (int) h) * 255;

  v = 0;
  dv = 1.0 / csf->width;

  switch ((int) h)
    {
    case 0:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 1:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);

	  v += dv;
	}
      break;
    case 2:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v *255;
	  *p++ = v * (255 - (s * (255 - f)));

	  v += dv;
	}
      break;
    case 3:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 4:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * (255 - (s * (255 - f)));
	  *p++ = v * (255 * (1 - s));
	  *p++ = v * 255;

	  v += dv;
	}
      break;
    case 5:
      for (i = 0; i < csf->width; i++)
	{
	  *p++ = v * 255;
	  *p++ = v * 255 * (1 - s);
	  *p++ = v * (255 - s * f);

	  v += dv;
	}
      break;
    }
}


/*****************************/
/* Colour notebook glue      */

typedef struct {
  GimpColorSelector_Callback      callback;
  void                           *client_data;
  ColorSelectP                    csp;
  GtkWidget                      *main_vbox;
} notebook_glue;


static GtkWidget *
color_select_notebook_new (int r, int g, int b,
	      GimpColorSelector_Callback callback,  void *client_data,
	      /* RETURNS: */
	      void **selector_data)
{
  ColorSelectP csp;
  notebook_glue *glue;

  glue = g_malloc (sizeof (notebook_glue));

  csp = g_malloc (sizeof (_ColorSelect));

  glue->csp = csp;
  glue->callback = callback;
  glue->client_data = client_data;

  csp->callback = color_select_notebook_update_callback;
  csp->client_data = glue;
  csp->z_color_fill = HUE;
  csp->xy_color_fill = SATURATION_VALUE;
  csp->gc = NULL;
  csp->wants_updates = TRUE;

  csp->values[RED] = csp->orig_values[0] = r;
  csp->values[GREEN] = csp->orig_values[1] = g;
  csp->values[BLUE] = csp->orig_values[2] = b;
  color_select_update_hsv_values (csp);
  color_select_update_pos (csp);

  glue->main_vbox = color_select_widget_new (csp, r, g, b);

  /* the shell is provided by the notebook */
  csp->shell = NULL;

  color_select_image_fill (csp->z_color, csp->z_color_fill, csp->values);
  color_select_image_fill (csp->xy_color, csp->xy_color_fill, csp->values);

  (*selector_data) = glue;
  return glue->main_vbox;
}


static void
color_select_notebook_free (void *data)
{
  notebook_glue *glue = data;

  color_select_free (glue->csp);
  /* don't need to destroy the widget, since it's done by the caller
   * of this function */
  g_free (glue);
}


static void
color_select_notebook_setcolor (void *data,
				int r, int g, int b, int set_current)
{
  notebook_glue *glue = data;

  color_select_set_color (glue->csp, r, g, b, set_current);
}

static void
color_select_notebook_update_callback (int r, int g, int b,
				       ColorSelectState state, void *data)
{
  notebook_glue *glue = data;

  switch (state) { 
  case COLOR_SELECT_UPDATE:
    glue->callback (glue->client_data, r, g, b);
    break;

  default:
    g_warning ("state %d can't happen!", state);
    break;
  }
}
