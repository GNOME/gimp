/* scatter_hsv.c -- This is a plug-in for the GIMP (1.0's API)
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 * Time-stamp: <2000-01-08 02:49:39 yasuhiro>
 * Version: 0.42
 *
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
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
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define	PLUG_IN_NAME	"plug_in_scatter_hsv"
#define SHORT_NAME	"scatter_hsv"

static void   query (void);
static void   run   (gchar   *name,
		     gint     nparams,
		     GParam  *param,
		     gint    *nreturn_vals,
		     GParam **return_vals);

static GStatusType scatter_hsv         (gint32  drawable_id);
static void        scatter_hsv_scatter (guchar *r,
					guchar *g,
					guchar *b);
static gint        randomize_value     (gint    now,
					gint    min,
					gint    max,
					gint    mod_p,
					gint    rand_max);

static gint	scatter_hsv_dialog         (void);
static void	scatter_hsv_ok_callback    (GtkWidget     *widget,
					    gpointer       data);
static gint	preview_event_handler      (GtkWidget     *widget,
					    GdkEvent      *event);
static void	scatter_hsv_preview_update (void);
static void     scatter_hsv_iscale_update  (GtkAdjustment *adjustment,
					    gpointer       data);

#define PROGRESS_UPDATE_NUM 100
#define PREVIEW_WIDTH       128
#define PREVIEW_HEIGHT      128
#define SCALE_WIDTH         100

static gint preview_width  = PREVIEW_WIDTH;
static gint preview_height = PREVIEW_HEIGHT;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{				/* gint, gdouble, and so on */
  gint	holdness;
  gint	hue_distance;
  gint	saturation_distance;
  gint	value_distance;
} ValueType;

static ValueType VALS = 
{
  2,
  3,
  10,
  10
};

typedef struct 
{
  gint run;
} Interface;

static Interface INTERFACE =
{
  FALSE
};

static gint      drawable_id;

static GtkWidget *preview;
static gint	  preview_start_x = 0;
static gint	  preview_start_y = 0;
static guchar	 *preview_buffer = NULL;
static gint	  preview_offset_x = 0;
static gint	  preview_offset_y = 0;
static gint	  preview_dragging = FALSE;
static gint	  preview_drag_start_x = 0;
static gint	  preview_drag_start_y = 0;

MAIN ()

static void
query (void)
{
  static GParamDef args [] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    { PARAM_IMAGE, "image", "Input image (not used)"},
    { PARAM_DRAWABLE, "drawable", "Input drawable"},
    { PARAM_INT32, "holdness", "convolution strength"},
    { PARAM_INT32, "hue_distance", "distribution distance on hue axis [0,255]"},
    { PARAM_INT32, "saturation_distance", "distribution distance on saturation axis [0,255]"},
    { PARAM_INT32, "value_distance", "distribution distance on value axis [0,255]"}
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  INIT_I18N();
  
  gimp_install_procedure (PLUG_IN_NAME,
			  _("Scattering pixel values in HSV space"),
			  _("Scattering pixel values in HSV space"),
			  "Shuji Narazaki (narazaki@InetQ.or.jp)",
			  "Shuji Narazaki",
			  "1997",
			  N_("<Image>/Filters/Noise/Scatter HSV..."),
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam	values[1];
  GStatusType   status = STATUS_EXECUTION_ERROR;
  GRunModeType  run_mode;
  
  run_mode = param[0].data.d_int32;
  drawable_id = param[2].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;
  
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      INIT_I18N_UI();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      if (!gimp_drawable_is_rgb (drawable_id))
	{
	  g_message ("Scatter HSV: RGB drawable is not selected.");
	  return;
	}
      if (! scatter_hsv_dialog ())
	return;
      break;
    case RUN_NONINTERACTIVE:
      INIT_I18N();
      VALS.holdness = param[3].data.d_int32;
      VALS.hue_distance = param[4].data.d_int32;
      VALS.saturation_distance = param[5].data.d_int32;
      VALS.value_distance = param[6].data.d_int32;
      break;
    case RUN_WITH_LAST_VALS:
      INIT_I18N();
      gimp_get_data (PLUG_IN_NAME, &VALS);
      break;
    }
  
  status = scatter_hsv (drawable_id);

  if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();
  if (run_mode == RUN_INTERACTIVE && status == STATUS_SUCCESS )
    gimp_set_data (PLUG_IN_NAME, &VALS, sizeof (ValueType));

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static GStatusType
scatter_hsv (gint32 drawable_id)
{
  GDrawable *drawable;
  GPixelRgn  src_rgn, dest_rgn;
  guchar    *src, *dest;
  gpointer   pr;
  gint       x, y, x1, x2, y1, y2;
  gint       gap, total, processed = 0;
  
  drawable = gimp_drawable_get (drawable_id);
  gap = (gimp_drawable_has_alpha (drawable_id)) ? 1 : 0;
  gimp_drawable_mask_bounds (drawable_id, &x1, &y1, &x2, &y2);
  total = (x2 - x1) * (y2 - y1);

  gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  gimp_pixel_rgn_init (&src_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable,
		       x1, y1, (x2 - x1), (y2 - y1), TRUE, TRUE);

  gimp_progress_init (_("Scatter HSV: Scattering..."));
  srand (time (NULL));
  pr = gimp_pixel_rgns_register (2, &src_rgn, &dest_rgn);
  
  for (; pr != NULL; pr = gimp_pixel_rgns_process (pr))
    {
      int offset;

      for (y = 0; y < src_rgn.h; y++)
	{
	  src = src_rgn.data + y * src_rgn.rowstride;
	  dest = dest_rgn.data + y * dest_rgn.rowstride;
	  offset = 0;

	  for (x = 0; x < src_rgn.w; x++)
	    {
	      guchar	h, s, v;

	      h = *(src + offset);
	      s = *(src + offset + 1);
	      v = *(src + offset + 2);
	      
	      scatter_hsv_scatter (&h, &s, &v);

	      *(dest + offset    ) = (guchar) h;
	      *(dest + offset + 1) = (guchar) s;
	      *(dest + offset + 2) = (guchar) v;

	      offset += 3;
	      if (gap)
		{
		  *(dest + offset) = *(src + offset);
		  offset++;
		}
	      /* the function */
	      if ((++processed % (total / PROGRESS_UPDATE_NUM)) == 0)
		gimp_progress_update ((double) processed /(double) total); 
	    }
	}
  }

  gimp_progress_update (1.0);
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
  gimp_drawable_detach (drawable);

  return STATUS_SUCCESS;
}

static gint
randomize_value (gint now,
		 gint min,
		 gint max,
		 gint mod_p,
		 gint rand_max)
{
  gint    flag, new, steps, index;
  gdouble rand_val;
  
  steps = max - min + 1;
  rand_val = ((double) rand () / (double) G_MAXRAND);
  for (index = 1; index < VALS.holdness; index++)
    {
      double tmp = ((double) rand () / (double) G_MAXRAND);
      if (tmp < rand_val)
	rand_val = tmp;
    }
  
  flag = ((G_MAXRAND / 2) < rand()) ? 1 : -1;
  new = now + flag * ((int) (rand_max * rand_val) % steps);
  
  if (new < min)
    {
      if (mod_p == 1)
	new += steps;
      else
	new = min;
    }
  if (max < new)
    {
      if (mod_p == 1)
	new -= steps;
      else
	new = max;
    }
  return new;
}

void scatter_hsv_scatter (guchar *r,
			  guchar *g,
			  guchar *b)
{
  gint h, s, v;
  gint h1, s1, v1;
  gint h2, s2, v2;
  
  h = *r; s = *g; v = *b;
  
  gimp_rgb_to_hsv (&h, &s, &v);

  if (0 < VALS.hue_distance)
    h = randomize_value (h, 0, 255, 1, VALS.hue_distance);
  if ((0 < VALS.saturation_distance))
    s = randomize_value (s, 0, 255, 0, VALS.saturation_distance);
  if ((0 < VALS.value_distance))
    v = randomize_value (v, 0, 255, 0, VALS.value_distance);

  h1 = h; s1 = s; v1 = v;
	      
  gimp_hsv_to_rgb (&h, &s, &v); /* don't believe ! */

  h2 = h; s2 = s; v2 = v;

  gimp_rgb_to_hsv (&h2, &s2, &v2); /* h2 should be h1. But... */
  
  if ((abs (h1 - h2) <= VALS.hue_distance)
      && (abs (s1 - s2) <= VALS.saturation_distance)
      && (abs (v1 - v2) <= VALS.value_distance))
    {
      *r = h;
      *g = s;
      *b = v;
    }
}

/* dialog stuff */
static gint
scatter_hsv_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *pframe;
  GtkWidget *abox;
  GtkWidget *table;
  GtkObject *adj;
  guchar  *color_cube;
  gchar	 **argv;
  gint	   argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup (SHORT_NAME);

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());
  
  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());

  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());

  dlg = gimp_dialog_new (_("Scatter HSV"), SHORT_NAME,
			 gimp_plugin_help_func, "filters/scatter_hsv.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), scatter_hsv_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dlg)->vbox), vbox);

  frame = gtk_frame_new (_("Preview (1:4) - Right Click to Jump"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), abox);
  gtk_widget_show (abox);

  pframe = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (pframe), 4);
  gtk_container_add (GTK_CONTAINER (abox), pframe);
  gtk_widget_show (pframe);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  {
    gint width  = gimp_drawable_width (drawable_id);
    gint height = gimp_drawable_height (drawable_id);

    preview_width  = (PREVIEW_WIDTH  < width)  ? PREVIEW_WIDTH  : width;
    preview_height = (PREVIEW_HEIGHT < height) ? PREVIEW_HEIGHT : height;
  }
  gtk_preview_size (GTK_PREVIEW (preview), preview_width * 2, preview_height);
  scatter_hsv_preview_update ();
  gtk_container_add (GTK_CONTAINER (pframe), preview);
  gtk_widget_set_events (preview, 
			 GDK_BUTTON_PRESS_MASK |
			 GDK_BUTTON_RELEASE_MASK | 
			 GDK_BUTTON_MOTION_MASK |
			 GDK_POINTER_MOTION_HINT_MASK);
  gtk_signal_connect (GTK_OBJECT (preview), "event",
		      (GtkSignalFunc) preview_event_handler,
		      NULL);
  gtk_widget_show (preview);

  gtk_widget_show (frame);

  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
			      _("Holdness:"), SCALE_WIDTH, 0,
			      VALS.holdness, 1, 8, 1, 2, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (scatter_hsv_iscale_update),
		      &VALS.holdness);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
			      _("Hue:"), SCALE_WIDTH, 0,
			      VALS.hue_distance, 0, 255, 1, 8, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (scatter_hsv_iscale_update),
		      &VALS.hue_distance);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 2,
			      _("Saturation:"), SCALE_WIDTH, 0,
			      VALS.saturation_distance, 0, 255, 1, 8, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (scatter_hsv_iscale_update),
		      &VALS.saturation_distance);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 3,
			      _("Value:"), SCALE_WIDTH, 0,
			      VALS.value_distance, 0, 255, 1, 8, 0,
			      NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (scatter_hsv_iscale_update),
		      &VALS.value_distance);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);
  gtk_widget_show (dlg);
  
  gtk_main ();
  gdk_flush ();

  return INTERFACE.run;
}

static gint
preview_event_handler (GtkWidget *widget,
		       GdkEvent  *event)
{
  gint            x, y;
  gint            dx, dy;
  GdkEventButton *bevent;
	
  gtk_widget_get_pointer (widget, &x, &y);

  bevent = (GdkEventButton *) event;

  switch (event->type) 
    {
    case GDK_BUTTON_PRESS:
      if (x < preview_width)
	{
	  if (bevent->button == 3)
	    {
	      preview_offset_x = - x;
	      preview_offset_y = - y;
	      scatter_hsv_preview_update ();
	    }
	  else
	    {
	      preview_dragging = TRUE;
	      preview_drag_start_x = x;
	      preview_drag_start_y = y;
	      gtk_grab_add (widget);
	    }
	}
      break;
    case GDK_BUTTON_RELEASE:
      if (preview_dragging)
	{
	  gtk_grab_remove (widget);
	  preview_dragging = FALSE;
	  scatter_hsv_preview_update ();
	}
      break;
    case GDK_MOTION_NOTIFY:
      if (preview_dragging)
	{
	  dx = x - preview_drag_start_x;
	  dy = y - preview_drag_start_y;

	  preview_drag_start_x = x;
	  preview_drag_start_y = y;

	  if ((dx == 0) && (dy == 0))
	    break;

	  preview_offset_x = MAX (preview_offset_x - dx, 0);
	  preview_offset_y = MAX (preview_offset_y - dy, 0);
	  scatter_hsv_preview_update ();
	}
      break; 
    default:
      break;
    }
  return FALSE;
}

static void
scatter_hsv_preview_update (void)
{
  GDrawable	*drawable;
  GPixelRgn	src_rgn;
  gint	scale;
  gint	x, y, dx, dy;
  gint	bound_start_x, bound_start_y, bound_end_x, bound_end_y;
  gint	src_has_alpha = FALSE;
  gint	src_is_gray = FALSE;
  gint	src_bpp, src_bpl;
  guchar	data[3];
  gdouble	shift_rate;
    
  drawable = gimp_drawable_get (drawable_id);
  gimp_drawable_mask_bounds (drawable_id,
			     &bound_start_x, &bound_start_y,
			     &bound_end_x, &bound_end_y);
  src_has_alpha  = gimp_drawable_has_alpha (drawable_id);
  src_is_gray =  gimp_drawable_is_gray (drawable_id);
  src_bpp = (src_is_gray ? 1 : 3) + (src_has_alpha ? 1 : 0);
  src_bpl = preview_width * src_bpp;

  if (! preview_buffer)
    preview_buffer
      = (guchar *) g_malloc (src_bpl * preview_height * sizeof (guchar));

  if (preview_offset_x < 0)
    preview_offset_x = (bound_end_x - bound_start_x) * (- preview_offset_x) /  preview_width;
  if (preview_offset_y < 0)
    preview_offset_y = (bound_end_y - bound_start_y) * (- preview_offset_y) /  preview_height;
  preview_start_x = CLAMP (bound_start_x + preview_offset_x,
			   bound_start_x, MAX (bound_end_x - preview_width, 0));
  preview_start_y = CLAMP (bound_start_y + preview_offset_y,
			   bound_start_y, MAX (bound_end_y - preview_height, 0));
  if (preview_start_x == bound_start_x)
    preview_offset_x = 0;
  if (preview_start_y == bound_start_y)
    preview_offset_y =0;

  gimp_pixel_rgn_init (&src_rgn, drawable, preview_start_x, preview_start_y,
		       preview_width, preview_height,
		       FALSE, FALSE);

  /* Since it's small, get whole data before processing. */
  gimp_pixel_rgn_get_rect (&src_rgn, preview_buffer,
			   preview_start_x, preview_start_y,
			   preview_width, preview_height);

  scale = 4;
  shift_rate = (gdouble) (scale - 1) / ( 2 * scale);
  for (y = 0; y < preview_height/4; y++)
    {
      for (x = 0; x < preview_width/4; x++)
	{
	  gint pos;
	  gint	i;

	  pos = (gint)(y + preview_height * shift_rate) * src_bpl
	        + (gint)(x + preview_width * shift_rate) * src_bpp;

	  for (i = 0; i < src_bpp; i++)
	    data[i] = preview_buffer[pos + i];

	  scatter_hsv_scatter (data+0, data+1, data+2);
	  for (dy = 0; dy < scale; dy++)
	    for (dx = 0; dx < scale; dx++)
	      gtk_preview_draw_row (GTK_PREVIEW (preview), data,
				    preview_width + x * scale + dx,
				    y * scale + dy, 1);
	}
    }
  for (y = 0; y < preview_height; y ++)
    for (x = 0; x < preview_width; x++)
      {
	gint	i;

	for (i = 0; i < src_bpp; i++)
	  data[i] = preview_buffer[y * src_bpl + x * src_bpp + i];

	scatter_hsv_scatter (data+0, data+1, data+2);
	gtk_preview_draw_row (GTK_PREVIEW (preview), data, x, y, 1);
      }
  gtk_widget_draw (preview, NULL);
  gdk_flush ();
}

static void
scatter_hsv_ok_callback (GtkWidget *widget,
			 gpointer   data)
{
  INTERFACE.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
scatter_hsv_iscale_update (GtkAdjustment *adjustment,
			   gpointer       data)
{
  gimp_int_adjustment_update (adjustment, data);

  scatter_hsv_preview_update ();
}
