/* Watercolor color selector, Raph Levien <raph@acm.org>, February 1998
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This simple plug-in does an automatic contrast stretch.  For each
   channel in the image, it finds the minimum and maximum values... it
   uses those values to stretch the individual histograms to the full
   contrast range.  For some images it may do just what you want; for
   others it may be total crap :) */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include "libgimp/gimp.h"

/* Declare local functions.
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, [non-interactive]" }
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("extension_waterselect",
			  "Bring up an interactive color selector inspired by watercolors.",
			  "This extension is a prototype of an interactive colorselector that imitates the feel of watercolors.",
			  "Raph Levien",
			  "Raph Levien",
			  "1998",
                          "<Toolbox>/Xtns/Waterselect",
			  NULL,
			  PROC_EXTENSION,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
init_input (void)
{
  GList *tmp_list;
  GdkDeviceInfo *info;

  tmp_list = gdk_input_list_devices();

  info = NULL;
  while (tmp_list)
    {
      info = (GdkDeviceInfo *)tmp_list->data;
#ifdef VERBOSE
      g_print ("device: %s\n", info->name);
#endif
      if (!g_strcasecmp (info->name, "wacom") ||
	  !g_strcasecmp (info->name, "stylus") ||
	  !g_strcasecmp (info->name, "eraser"))
	  {
	    gdk_input_set_mode (info->deviceid, GDK_MODE_SCREEN);
	  }
      tmp_list = tmp_list->next;
    }
  if (!info) return;
}

#define BUCKET_SIZE 20
#define N_BUCKETS 10
#define LAST_BUCKET (N_BUCKETS-1)
#define IMAGE_SIZE 200

gdouble bucket[N_BUCKETS][3];

gboolean need_shift;
gboolean in_bucket;

static void
set_bucket (gint i, gdouble r, gdouble g, gdouble b)
{
  if (i >= 0 && i < N_BUCKETS)
    {
      bucket[i][0] = r;
      bucket[i][1] = g;
      bucket[i][2] = b;
    }
}

static gdouble
calc (gdouble x, gdouble y, gdouble angle)
{
  gdouble s, c;

  s = 1.6 * sin (angle * M_PI / 180) * 256.0 / IMAGE_SIZE;
  c = 1.6 * cos (angle * M_PI / 180) * 256.0 / IMAGE_SIZE;
  return 128 + (x - (IMAGE_SIZE >> 1)) * c - (y - (IMAGE_SIZE >> 1)) * s;
}

static guchar
bucket_to_byte (gdouble val)
{
  return CLAMP ((gint)(val * 280 - 25), 0, 255);
}

GtkWidget *preview;

static void
draw_buckets (GtkWidget *preview, gint start, gint end)
{
  guchar buf[3 * IMAGE_SIZE];
  gint x, y;
  gint i;
  guchar r, g, b;

  for (y = 1; y < BUCKET_SIZE - 1; y++)
    {
      for (i = 0; i < end; i++)
	{
	  r = bucket_to_byte (bucket[i][0]);
	  g = bucket_to_byte (bucket[i][1]);
	  b = bucket_to_byte (bucket[i][2]);
	  buf[i * BUCKET_SIZE * 3] = 0;
	  buf[i * BUCKET_SIZE * 3 + 1] = 0;
	  buf[i * BUCKET_SIZE * 3 + 2] = 0;
	  buf[(i * BUCKET_SIZE + BUCKET_SIZE - 1) * 3] = 0;
	  buf[(i * BUCKET_SIZE + BUCKET_SIZE - 1) * 3 + 1] = 0;
	  buf[(i * BUCKET_SIZE + BUCKET_SIZE - 1) * 3 + 2] = 0;
	  for (x = 1; x < BUCKET_SIZE - 1; x++)
	    {
	      buf[(i * BUCKET_SIZE + x) * 3] = r;
	      buf[(i * BUCKET_SIZE + x) * 3 + 1] = g;
	      buf[(i * BUCKET_SIZE + x) * 3 + 2] = b;
	    }
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y, IMAGE_SIZE);
    }
}

static void
update_buckets (gint start, gint end)
{
  draw_buckets (preview, start, end);

  gtk_widget_draw (preview, NULL);
}

static void
update_gimp_color (void)
{
  gimp_palette_set_foreground (bucket_to_byte (bucket[LAST_BUCKET][0]),
			       bucket_to_byte (bucket[LAST_BUCKET][1]),
			       bucket_to_byte (bucket[LAST_BUCKET][2]));
}

static void
pick_up_bucket (gint i)
{
  bucket[LAST_BUCKET][0] = bucket[i][0];
  bucket[LAST_BUCKET][1] = bucket[i][1];
  bucket[LAST_BUCKET][2] = bucket[i][2];
}

static void
shift_buckets ()
{
  gint i;

  /* to avoid duplication, do nothing if last bucket is already present */
  for (i = 0; i < LAST_BUCKET; i++)
    {
      if (bucket[i][0] == bucket[LAST_BUCKET][0] &&
	  bucket[i][1] == bucket[LAST_BUCKET][1] &&
	  bucket[i][2] == bucket[LAST_BUCKET][2])
	return;
    }

  /* don't bother storing pure white in buckets */
  if (bucket[LAST_BUCKET][0] == 1 &&
      bucket[LAST_BUCKET][1] == 1 &&
      bucket[LAST_BUCKET][2] == 1)
    return;

  /* shift all buckets one to the left */
  for (i = 0; i < LAST_BUCKET; i++)
    {
      bucket[i][0] = bucket[i + 1][0];
      bucket[i][1] = bucket[i + 1][1];
      bucket[i][2] = bucket[i + 1][2];
    }

}

/* Initialize the preview */
static void
preview_init (GtkWidget *preview)
{
  guchar buf[3 * IMAGE_SIZE];
  gint x, y;
  gint i;
  gdouble r, g, b;
  gdouble dr, dg, db;

  for (y = 0; y < IMAGE_SIZE; y++)
    {
      r = calc (0, y, 0);
      g = calc (0, y, 120);
      b = calc (0, y, 240);

      dr = calc (1, y, 0) - r;
      dg = calc (1, y, 120) - g;
      db = calc (1, y, 240) - b;

      for (x = 0; x < IMAGE_SIZE; x++)
	{
	  buf[x * 3] = CLAMP ((gint)r, 0, 255);
	  buf[x * 3 + 1] = CLAMP ((gint)g, 0, 255);
	  buf[x * 3 + 2] = CLAMP ((gint)b, 0, 255);
	  r += dr;
	  g += dg;
	  b += db;
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0,
			    BUCKET_SIZE + y,
			    IMAGE_SIZE);
    }

  for (i = 0; i < N_BUCKETS; i++)
    set_bucket (i, 1, 1, 1);

  draw_buckets (preview, 0, N_BUCKETS);
}

static void
waterselect_destroy_callback (GtkWidget *widget, void *dummy)
{
  gtk_main_quit ();
}

static gboolean
waterselect_delete_callback (GtkWidget *widget, void *dummy)
{
  return TRUE;
}

static void
waterselect_ok_callback (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

/* todo: cancel callback with appropriate logic */

static GdkPixmap *pixmap = NULL;

/* Create a new backing pixmap of the appropriate size - obsolete */
static gint
configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
  if (pixmap)
    gdk_pixmap_unref (pixmap);
  pixmap = gdk_pixmap_new(widget->window,
			  widget->allocation.width,
			  widget->allocation.height,
			  -1);
  gdk_draw_rectangle (pixmap,
		      widget->style->white_gc,
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);
  gdk_draw_rectangle (pixmap,
		      widget->style->black_gc,
		      TRUE,
		      0, 0,
		      10,
		      10);

  return TRUE;

}

static void add_pigment (gboolean erase, gdouble x, gdouble y, gdouble much)
{
  gdouble r, g, b;

  y -= BUCKET_SIZE;

#ifdef VERBOSE
  g_print ("%g->%g %g->%g %g->%g => %g\n",
	   last_x, x, last_y, y, last_pressure, pressure, much);
#endif
  if (erase)
    {
      bucket[LAST_BUCKET][0] = 1 - (1 - bucket[LAST_BUCKET][0]) * (1 - much);
      bucket[LAST_BUCKET][1] = 1 - (1 - bucket[LAST_BUCKET][1]) * (1 - much);
      bucket[LAST_BUCKET][2] = 1 - (1 - bucket[LAST_BUCKET][2]) * (1 - much);
    }
  else
    {
      r = calc (x, y, 0) / 255.0;
      if (r < 0) r = 0;
      if (r > 1) r = 1;

      g = calc (x, y, 120) / 255.0;
      if (g < 0) g = 0;
      if (g > 1) g = 1;

      b = calc (x, y, 240) / 255.0;
      if (b < 0) b = 0;
      if (b > 1) b = 1;

      bucket[LAST_BUCKET][0] *= (1 - (1 - r) * much);
      bucket[LAST_BUCKET][1] *= (1 - (1 - g) * much);
      bucket[LAST_BUCKET][2] *= (1 - (1 - b) * much);
    }
}

static gdouble last_x, last_y, last_pressure;

static void
draw_brush (GtkWidget *widget, gboolean erase,
	    gdouble x, gdouble y, gdouble pressure)
{
  gdouble much; /* how much pigment to mix in */

  much = sqrt ((x - last_x) * (x - last_x) +
	       (y - last_y) * (y - last_y) +
	       1000 * (pressure - last_pressure) * (pressure - last_pressure));

  much *= pressure * 0.05;

  add_pigment (erase, x, y, much);

  last_x = x;
  last_y = y;
  last_pressure = pressure;
}

static guint32 motion_time;

static gint button_state;

static gint
button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  gboolean erase;

#ifdef VERBOSE
  g_print ("button press\n");
#endif
  last_x = event->x;
  last_y = event->y;
  last_pressure = event->pressure;

  button_state |= 1 << event->button;

  if (event->y < BUCKET_SIZE)
    {
      in_bucket = TRUE;
      
      pick_up_bucket (event->x / BUCKET_SIZE);
      update_buckets (LAST_BUCKET, N_BUCKETS);
      update_gimp_color ();
    }
  else
    {
      in_bucket = FALSE;

      erase = (event->button != 1) ||
	(event->source == GDK_SOURCE_ERASER);

      add_pigment (erase, event->x, event->y, 0.05);
      update_buckets (LAST_BUCKET, N_BUCKETS);
      update_gimp_color ();
    }
  motion_time = event->time;

  return FALSE;
}

static gint
button_release_event (GtkWidget *widget, GdkEventButton *event)
{
  button_state &= ~(1 << event->button);

  return TRUE;
}

static gint
key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  if ((event->keyval >= 0x20) && (event->keyval <= 0xFF))
    printf("I got a %c\n", event->keyval);
  else
    printf("I got some other key\n");

  return TRUE;
}

static gint
motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
  GdkTimeCoord *coords;
  int nevents;
  int i;
  gboolean erase;

  if (event->state & (GDK_BUTTON1_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK))
    {
      if (in_bucket)
	return FALSE;
      coords = gdk_input_motion_events (event->window, event->deviceid,
					motion_time, event->time,
					&nevents);
      erase = (!(event->state & GDK_BUTTON1_MASK)) ||
	(event->source == GDK_SOURCE_ERASER);
      motion_time = event->time;
      if (coords)
	{
	  for (i=0; i<nevents; i++)
	    draw_brush (widget, erase, coords[i].x, coords[i].y,
			coords[i].pressure);
	  g_free (coords);
	}
      else
	{
	  if (event->is_hint)
	    gdk_input_window_get_pointer (event->window, event->deviceid,
					  NULL, NULL, NULL, NULL, NULL, NULL);
	  draw_brush (widget, erase, event->x, event->y,
		      event->pressure);
	}
      update_buckets (LAST_BUCKET, N_BUCKETS);
      update_gimp_color ();
    }
  else
    {
      gdk_input_window_get_pointer (event->window, event->deviceid,
				    &event->x, &event->y,
				    NULL, NULL, NULL, NULL);
    }


  return TRUE;
}

/* We track the next two events to know when we need to draw a
   cursor */

static gint
proximity_out_event (GtkWidget *widget, GdkEventProximity *event)
{
#ifdef VERBOSE
  g_print ("proximity out\n");
#endif VERBOSE
  return TRUE;
}

static gint
enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
#ifdef VERBOSE
  g_print ("enter\n");
#endif
  if (need_shift)
    {
      shift_buckets ();

      /* last bucket becomes white */
      bucket[LAST_BUCKET][0] = 1;
      bucket[LAST_BUCKET][1] = 1;
      bucket[LAST_BUCKET][2] = 1;

      update_buckets (0, N_BUCKETS);
      /* don't update gimp color here... */
      need_shift = FALSE;
    }
  return TRUE;
}

static gint
leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
#ifdef VERBOSE
  g_print ("leave\n");
#endif
  /* The goal is to detect the mouse leaving the eventbox window with
     the button not pressed. It seems to work, but is a bit of a hack. */
  if (button_state == 0 && event->detail != GDK_NOTIFY_INFERIOR)
    need_shift = TRUE;
  return TRUE;
}

static gint
waterselect_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *event_box;

  guchar *color_cube;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("Film");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  gdk_set_use_xshm(gimp_use_xshm());

  gtk_preview_set_gamma(gimp_gamma());
  gtk_preview_set_install_cmap(gimp_install_cmap());
  color_cube = gimp_color_cube();
  gtk_preview_set_color_cube(color_cube[0], color_cube[1], color_cube[2],
                             color_cube[3]);
  gtk_widget_set_default_visual(gtk_preview_get_visual());
  gtk_widget_set_default_colormap(gtk_preview_get_cmap());

  init_input ();

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Waterselect");
  /* set window position to mouse? */
  gtk_signal_connect (GTK_OBJECT (dlg), "delete_event",
                      (GtkSignalFunc) waterselect_delete_callback,
                      NULL);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      (GtkSignalFunc) waterselect_destroy_callback,
                      NULL);
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) waterselect_ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             (GtkSignalFunc) gtk_widget_destroy,
                             GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button,
                      TRUE, TRUE, 0);
  gtk_widget_show (button);

  event_box = gtk_event_box_new ();

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), IMAGE_SIZE, BUCKET_SIZE + IMAGE_SIZE);

  preview_init (preview);

  gtk_container_add (GTK_CONTAINER (event_box), preview);
  gtk_widget_show (preview);

  /* Signals used to handle backing pixmap */

  gtk_signal_connect (GTK_OBJECT(event_box),"configure_event",
		      (GtkSignalFunc) configure_event, NULL);

  /* Event signals */

  gtk_signal_connect (GTK_OBJECT (event_box), "motion_notify_event",
		      (GtkSignalFunc) motion_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_press_event",
		      (GtkSignalFunc) button_press_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_release_event",
		      (GtkSignalFunc) button_release_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "key_press_event",
		      (GtkSignalFunc) key_press_event, NULL);

  gtk_signal_connect (GTK_OBJECT (event_box), "enter_notify_event",
		      (GtkSignalFunc) enter_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "leave_notify_event",
		      (GtkSignalFunc) leave_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "proximity_out_event",
		      (GtkSignalFunc) proximity_out_event, NULL);

  gtk_widget_set_events (event_box, GDK_EXPOSURE_MASK
			 | GDK_LEAVE_NOTIFY_MASK
			 | GDK_BUTTON_PRESS_MASK
			 | GDK_KEY_PRESS_MASK
			 | GDK_POINTER_MOTION_MASK
			 | GDK_POINTER_MOTION_HINT_MASK
			 | GDK_PROXIMITY_OUT_MASK);

  /* The following call enables tracking and processing of extension
     events for the drawing area */
  gtk_widget_set_extension_events (event_box, GDK_EXTENSION_EVENTS_ALL);

  gtk_widget_grab_focus (event_box);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), event_box, TRUE, TRUE, 0);
  gtk_widget_show (event_box);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return TRUE;
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  switch (run_mode) 
    {
    case RUN_INTERACTIVE:
      if (!waterselect_dialog ())
	return;
      break;
    default:
      g_warning ("waterselect allows only interactive invocation");
      values[0].data.d_status = STATUS_CALLING_ERROR;
      break;
    }

}
