/* Watercolor color_select_module, Raph Levien <raph@acm.org>, February 1998
 *
 * Ported to loadable color-selector, Sven Neumann <sven@gimp.org>, May 1999
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

/* #define VERBOSE 1 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gtk/gtk.h>
#include <libgimp/color_selector.h>
#include <libgimp/gimpmodule.h>
#include "modregister.h"

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif /* M_PI */

/* prototypes */
static GtkWidget * colorsel_water_new         (int, int, int,
				               GimpColorSelector_Callback, 
				               void *,
				               void **);
static void        colorsel_water_free        (void *);
static void        colorsel_water_setcolor    (void *, int, int, int, int);
static void        colorsel_water_update      ();
static void        colorsel_water_drag_begin  (GtkWidget          *widget,
					       GdkDragContext     *context,
					       gpointer            data);
static void        colorsel_water_drag_end    (GtkWidget          *widget,
					       GdkDragContext     *context,
					       gpointer            data);
static void        colorsel_water_drop_handle (GtkWidget          *widget, 
					       GdkDragContext     *context,
					       gint                x,
					       gint                y,
					       GtkSelectionData   *selection_data,
					       guint               info,
					       guint               time,
					       gpointer            data);
static void        colorsel_water_drag_handle (GtkWidget        *widget, 
					       GdkDragContext   *context,
					       GtkSelectionData *selection_data,
					       guint             info,
					       guint             time,
					       gpointer          data);

/* local methods */
static GimpColorSelectorMethods methods = 
{
  colorsel_water_new,
  colorsel_water_free,
  colorsel_water_setcolor
};


static GimpModuleInfo info = {
    NULL,
    "Watercolor style color selector as a pluggable module",
    "Raph Levien <raph@acm.org>, Sven Neumann <sven@gimp.org>",
    "v0.3",
    "(c) 1998-1999, released under the GPL",
    "May, 10 1999"
};


static const GtkTargetEntry targets[] = {
  { "application/x-color", 0 }
};

/*************************************************************/

/* globaly exported init function */
G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
  GimpColorSelectorID id;

#ifndef __EMX__
  id = gimp_color_selector_register ("Watercolor", "watercolor.html",
				     &methods);
#else
   id = mod_color_selector_register ("Watercolor", "watercolor.html",
				     &methods);
#endif
  if (id)
  {
    info.shutdown_data = id;
    *inforet = &info;
    return GIMP_MODULE_OK;
  }
  else
  {
    return GIMP_MODULE_UNLOAD;
  }
}


G_MODULE_EXPORT void
module_unload (void *shutdown_data,
	       void (*completed_cb)(void *),
	       void *completed_data)
{
#ifndef __EMX__
  gimp_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#else
  mod_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#endif
}


/* definitions and variables */

#define N_BUCKETS 10
#define LAST_BUCKET (N_BUCKETS)  /* bucket number 0 is current color */
#define IMAGE_SIZE 200
#define BUCKET_SIZE 20
#define PREVIEW_SIZE 40

typedef struct {
  GimpColorSelector_Callback      callback;
  void                           *data;
} ColorselWater;

static gdouble    bucket[N_BUCKETS + 1][3];
static GtkWidget *color_preview[N_BUCKETS + 1];
static gdouble    last_x, last_y, last_pressure;
static gfloat     pressure_adjust = 1.0;
static guint32    motion_time;
static gint       button_state;
static ColorselWater *coldata;


static void
set_bucket (gint i, gdouble r, gdouble g, gdouble b)
{
  if (i >= 0 && i <= N_BUCKETS)
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

static void
draw_bucket (gint i)
{
  guchar *buf;
  gint x, y;
  gint width;
  gint height;
  guchar r, g, b;

  g_return_if_fail (i >= 0 && i <= N_BUCKETS);

#ifdef VERBOSE
  g_print ("draw_bucket %d\n", i);  
#endif

  width  = (i == 0 ? PREVIEW_SIZE : BUCKET_SIZE);
  height = width; 
  buf     = g_new (guchar, 3*width);
  
  r = bucket_to_byte (bucket[i][0]);
  g = bucket_to_byte (bucket[i][1]);
  b = bucket_to_byte (bucket[i][2]);
  for (x = 0; x < width; x++)
    {
      buf[x * 3] = r;
      buf[x * 3 + 1] = g;
      buf[x * 3 + 2] = b;
    }
  for (y = 0; y < height; y++) 
    gtk_preview_draw_row (GTK_PREVIEW (color_preview[i]), buf, 0, y, width);

  gtk_widget_draw (color_preview[i], NULL);
  g_free (buf);
}


static void
draw_all_buckets ()
{
  gint i;

#ifdef VERBOSE
  g_print ("drawing all buckets\n");
#endif

  for (i = 1; i <= N_BUCKETS; i++)
    draw_bucket (i);
}

static void
pick_up_bucket_callback (GtkWidget *widget, 
			 gpointer data)
{
  guint i = GPOINTER_TO_UINT (data);

#ifdef VERBOSE
  g_print ("pick up bucket %d\n", i);
#endif

  if (i > 0 && i <= N_BUCKETS)
    {
      bucket[0][0] = bucket[i][0];
      bucket[0][1] = bucket[i][1];
      bucket[0][2] = bucket[i][2];
      colorsel_water_update ();
    }
}

static void
shift_buckets ()
{
  gint i;

#ifdef VERBOSE
  g_print ("shift\n");
#endif

  /* to avoid duplication, do nothing if cuurent bucket is already present */
  for (i = 1; i <= N_BUCKETS; i++)
    {
      if (bucket[i][0] == bucket[0][0] &&
	  bucket[i][1] == bucket[0][1] &&
	  bucket[i][2] == bucket[0][2])
	return;
    }

  /* don't bother storing pure white in buckets */
  if (bucket[0][0] == 1.0 &&
      bucket[0][1] == 1.0 &&
      bucket[0][2] == 1.0)
    return;

  /* shift all buckets one to the right */
  for (i = N_BUCKETS; i > 0; i--)
    {
      bucket[i][0] = bucket[i-1][0];
      bucket[i][1] = bucket[i-1][1];
      bucket[i][2] = bucket[i-1][2];
    }
}

/* Initialize the preview */
static void
select_area_draw (GtkWidget *preview)
{
  guchar buf[3 * IMAGE_SIZE];
  gint x, y;
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
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y, IMAGE_SIZE);
    }
}


static void 
add_pigment (gboolean erase, gdouble x, gdouble y, gdouble much)
{
  gdouble r, g, b;

  much *= (gdouble)pressure_adjust; 

#ifdef VERBOSE
  g_print ("x: %g, y: %g, much: %g\n", x, y, much);
#endif

  if (erase)
    {
      bucket[0][0] = 1 - (1 - bucket[0][0]) * (1 - much);
      bucket[0][1] = 1 - (1 - bucket[0][1]) * (1 - much);
      bucket[0][2] = 1 - (1 - bucket[0][2]) * (1 - much);
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

      bucket[0][0] *= (1 - (1 - r) * much);
      bucket[0][1] *= (1 - (1 - g) * much);
      bucket[0][2] *= (1 - (1 - b) * much);
    }

  colorsel_water_update ();
}

static void
draw_brush (GtkWidget *widget, gboolean erase,
	    gdouble x, gdouble y, gdouble pressure)
{
  gdouble much; /* how much pigment to mix in */

  if (pressure < last_pressure)
    last_pressure = pressure;

  much = sqrt ((x - last_x) * (x - last_x) +
	       (y - last_y) * (y - last_y) +
	       1000 * (pressure - last_pressure) * (pressure - last_pressure));

  much *= pressure * 0.05;

  add_pigment (erase, x, y, much);

  last_x = x;
  last_y = y;
  last_pressure = pressure;
}


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

  erase = (event->button != 1) ||
    (event->source == GDK_SOURCE_ERASER);

  add_pigment (erase, event->x, event->y, 0.05);
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
motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
  GdkTimeCoord *coords;
  int nevents;
  int i;
  gboolean erase;

  if (event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK))
    {
      coords = gdk_input_motion_events (event->window, event->deviceid,
					motion_time, event->time,
					&nevents);
      erase = (event->state & 
	       (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK)) ||
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
#ifdef GTK_HAVE_SIX_VALUATORS
                                          NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#else /* !GTK_HAVE_SIX_VALUATORS */
					  NULL, NULL, NULL, NULL, NULL, NULL);
#endif /* GTK_HAVE_SIX_VALUATORS */
	  draw_brush (widget, erase, event->x, event->y,
		      event->pressure);
	}
    }
  else
    {
      gdk_input_window_get_pointer (event->window, event->deviceid,
				    &event->x, &event->y,
#ifdef GTK_HAVE_SIX_VALUATORS
                                    NULL, NULL, NULL, NULL, NULL);
#else /* !GTK_HAVE_SIX_VALUATORS */
				    NULL, NULL, NULL, NULL);
#endif /* GTK_HAVE_SIX_VALUATORS */
    }

  return TRUE;
}

static gint
proximity_out_event (GtkWidget *widget, GdkEventProximity *event)
{
#ifdef VERBOSE
  g_print ("proximity out\n");
#endif /* VERBOSE */
  return TRUE;
}

static void
new_color_callback (GtkWidget *widget, gpointer data)
{
#ifdef VERBOSE
  g_print ("new color\n");
#endif
  shift_buckets ();
  
  /* current bucket becomes white */
  bucket[0][0] = 1.0;
  bucket[0][1] = 1.0;
  bucket[0][2] = 1.0;

  draw_all_buckets ();
  colorsel_water_update ();

  return;
}

static void
reset_color_callback (GtkWidget *widget, gpointer data)
{
#ifdef VERBOSE
  g_print ("reset color\n");
#endif

  /* current bucket becomes white */
  bucket[0][0] = 1.0;
  bucket[0][1] = 1.0;
  bucket[0][2] = 1.0;

  colorsel_water_update ();

  return;
}

static void
pressure_adjust_update (GtkAdjustment *adj, gpointer data)
{
  pressure_adjust = adj->value / 100;
}


/*************************************************************/
/* methods */


static GtkWidget*
colorsel_water_new (int r, int g, int b,
		    GimpColorSelector_Callback callback,
		    void *callback_data,
		    /* RETURNS: */
		    void **selector_data)
{
  GtkWidget *preview;
  GtkWidget *event_box;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *hbox3;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *bbox;
  GtkWidget *label;
  GtkObject *adj;
  GtkWidget *scale;
  guint      i;

  coldata = g_malloc (sizeof (ColorselWater));

  coldata->callback = callback;
  coldata->data = callback_data;

  *selector_data = coldata;

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 4);

  /* the event box */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0); 

  event_box = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (frame), event_box);

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), IMAGE_SIZE, IMAGE_SIZE);
  gtk_container_add (GTK_CONTAINER (event_box), preview);
  select_area_draw (preview);

  /* Event signals */
  gtk_signal_connect (GTK_OBJECT (event_box), "motion_notify_event",
		      (GtkSignalFunc) motion_notify_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_press_event",
		      (GtkSignalFunc) button_press_event, NULL);
  gtk_signal_connect (GTK_OBJECT (event_box), "button_release_event",
		      (GtkSignalFunc) button_release_event, NULL);
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
  
  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX (hbox), vbox2, TRUE, FALSE, 0);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, FALSE, 4);

  vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, FALSE, 4);
  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox3, FALSE, FALSE, 4);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox3), frame, TRUE, FALSE, 0); 
  color_preview[0] = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (color_preview[0]), PREVIEW_SIZE, PREVIEW_SIZE);
  gtk_drag_dest_set (color_preview[0],
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     GDK_ACTION_COPY);
  gtk_drag_source_set (color_preview[0], 
		       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
		       targets, 1,
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (color_preview[0]),
		      "drag_begin",
		      GTK_SIGNAL_FUNC (colorsel_water_drag_begin),
		      bucket[0]);
  gtk_signal_connect (GTK_OBJECT (color_preview[0]),
		      "drag_end",
		      GTK_SIGNAL_FUNC (colorsel_water_drag_end),
		      bucket[0]);
  gtk_signal_connect (GTK_OBJECT (color_preview[0]),
		      "drag_data_get",
		      GTK_SIGNAL_FUNC (colorsel_water_drag_handle),
		      bucket[0]);
  gtk_signal_connect (GTK_OBJECT (color_preview[0]),
		      "drag_data_received",
		      GTK_SIGNAL_FUNC (colorsel_water_drop_handle),
		      bucket[0]);
  gtk_container_add (GTK_CONTAINER (frame), color_preview[0]);

  bbox = gtk_vbutton_box_new ();
  gtk_box_pack_end (GTK_BOX (hbox2), bbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("New");
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) new_color_callback,
		      NULL);
  button = gtk_button_new_with_label ("Reset");
  gtk_container_add (GTK_CONTAINER (bbox), button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) reset_color_callback,
		      NULL);

  frame = gtk_frame_new ("Color history");
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, FALSE, 0); 

  table = gtk_table_new (2, 5, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  
  for (i = 0; i < N_BUCKETS; i++)
    {
      button = gtk_button_new ();
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (pick_up_bucket_callback), 
			  (gpointer) GUINT_TO_POINTER (i+1));	  
      gtk_drag_dest_set (button,
			 GTK_DEST_DEFAULT_HIGHLIGHT |
			 GTK_DEST_DEFAULT_MOTION |
			 GTK_DEST_DEFAULT_DROP,
			 targets, 1,
			 GDK_ACTION_COPY);
      gtk_drag_source_set (button, 
			   GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
			   targets, 1,
			   GDK_ACTION_COPY | GDK_ACTION_MOVE);
      gtk_signal_connect (GTK_OBJECT (button),
			  "drag_begin",
			  GTK_SIGNAL_FUNC (colorsel_water_drag_begin),
			  bucket[i+1]);
      gtk_signal_connect (GTK_OBJECT (button),
			  "drag_end",
			  GTK_SIGNAL_FUNC (colorsel_water_drag_end),
			  bucket[i+1]);
      gtk_signal_connect (GTK_OBJECT (button),
			  "drag_data_get",
			  GTK_SIGNAL_FUNC (colorsel_water_drag_handle),
			  bucket[i+1]);
      gtk_signal_connect (GTK_OBJECT (button),
			  "drag_data_received",
			  GTK_SIGNAL_FUNC (colorsel_water_drop_handle),
			  bucket[i+1]);
      gtk_table_attach_defaults (GTK_TABLE (table),
				 button, 
				 i % 5, (i % 5) + 1,
				 i / 5, (i/ 5) + 1);
      color_preview[i+1] = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (color_preview[i+1]), BUCKET_SIZE, BUCKET_SIZE);
      gtk_container_add (GTK_CONTAINER (button), color_preview[i+1]);
      set_bucket (i+1, 1.0, 1.0, 1.0);
    }

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, FALSE, 0);  
  label = gtk_label_new ("Pressure:");
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);

  adj = gtk_adjustment_new (100.0, 0.0, 200.0, 1.0, 1.0, 0.0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (pressure_adjust_update), NULL);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, TRUE, TRUE, 0);

  gtk_widget_show_all (hbox);
  
  colorsel_water_setcolor (coldata, r, g, b, 0);
  draw_all_buckets ();
  
   return (vbox);
}


static void
colorsel_water_free (void *selector_data)
{
  g_free (selector_data);
}

static void
colorsel_water_setcolor (void *data, int r, int g, int b,
			 int set_current)
{
  set_bucket (0, 
	      ((gdouble) r) / 255.999, 
	      ((gdouble) g) / 255.999,
	      ((gdouble) b) / 255.999);
  draw_bucket (0);
}

static void
colorsel_water_update ()
{
  int r;
  int g;
  int b;

  r = (int) (bucket[0][0] * 255.999);
  g = (int) (bucket[0][1] * 255.999);
  b = (int) (bucket[0][2] * 255.999);

  draw_bucket (0);

  coldata->callback (coldata->data, r, g, b);
}

static void        
colorsel_water_drag_begin (GtkWidget      *widget,
			   GdkDragContext *context,
			   gpointer        data)
{
  GtkWidget *window;
  gdouble *colors;
  GdkColor bg;

  colors = (gdouble *)data;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, 32, 32);
  gtk_widget_realize (window);
  gtk_object_set_data_full (GTK_OBJECT (widget),
			    "gimp-color-drag-window",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  bg.red = 0xffff * colors[0];
  bg.green = 0xffff * colors[1];
  bg.blue = 0xffff * colors[2];

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  gtk_drag_set_icon_widget (context, window, -2, -2);
}


static void        
colorsel_water_drag_end (GtkWidget      *widget,
			 GdkDragContext *context,
			 gpointer        data)
{
  gtk_object_set_data (GTK_OBJECT (widget),
		       "gimp-color-drag-window", NULL);
}

static void        
colorsel_water_drop_handle (GtkWidget        *widget, 
			    GdkDragContext   *context,
			    gint              x,
			    gint              y,
			    GtkSelectionData *selection_data,
			    guint             info,
			    guint             time,
			    gpointer          data)
{
  guint16 *vals;
  gdouble *colors;

  colors = (gdouble*) data;

  if (selection_data->length < 0)
    return;

  if ((selection_data->format != 16) || 
      (selection_data->length != 8))
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *)selection_data->data;

  colors[0] = (gdouble)vals[0] / 0xffff;
  colors[1] = (gdouble)vals[1] / 0xffff;
  colors[2] = (gdouble)vals[2] / 0xffff;
  
  draw_all_buckets ();
  colorsel_water_update ();
}

static void        
colorsel_water_drag_handle (GtkWidget        *widget, 
			    GdkDragContext   *context,
			    GtkSelectionData *selection_data,
			    guint             info,
			    guint             time,
			    gpointer          data)
{
  guint16 vals[4];
  gdouble *colors;

  colors = (gdouble*) data;

  vals[0] = colors[0] * 0xffff;
  vals[1] = colors[1] * 0xffff;
  vals[2] = colors[2] * 0xffff;
  vals[3] = 0xffff;

  gtk_selection_data_set (selection_data,
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *)vals, 8);
}

/* End of colorsel_gtk.c */
  







