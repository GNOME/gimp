/*
 * colorsel_triangle module (C) 1999 Simon Budig <Simon.Budig@unix-ag.org>
 *    http://www.home.unix-ag.org/simon/gimp/colorsel.html
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
 *
 * Ported to loadable colour selector interface by Austin Donnelly
 * <austin@gimp.org>
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gimpmodregister.h"

#include <libgimp/gimpcolorselector.h>
#include <libgimp/gimpcolorspace.h>
#include <libgimp/gimpmodule.h>
#include <libgimp/gimpmath.h>

#include "libgimp/gimpintl.h"


/* prototypes */
static GtkWidget * colorsel_triangle_new  (gint              red,
					   gint              green,
					   gint              blue,
					   GimpColorSelector_Callback callback,
					   gpointer          callback_data,
					   gpointer         *selector_data);

static void colorsel_triangle_free        (gpointer          selector_data);

static void colorsel_triangle_setcolor    (gpointer          selector_data,
					   gint              red,
	         			   gint              green,
	         			   gint              blue,
	         			   gint              set_current);

static void colorsel_triangle_drag_begin  (GtkWidget        *widget,
	         			   GdkDragContext   *context,
	         			   gpointer          data);
                                                             
static void colorsel_triangle_drag_end    (GtkWidget        *widget,
	         			   GdkDragContext   *context,
	         			   gpointer          data);
                                                             
static void colorsel_triangle_drop_handle (GtkWidget        *widget, 
	         			   GdkDragContext   *context,
	         			   gint              x,
	         			   gint              y,
	         			   GtkSelectionData *selection_data,
	         			   guint             info,
	         			   guint             time,
	         			   gpointer          data);

static void colorsel_triangle_drag_handle (GtkWidget        *widget, 
					   GdkDragContext   *context,
					   GtkSelectionData *selection_data,
					   guint             info,
					   guint             time,
					   gpointer          data);


/* local methods */
static GimpColorSelectorMethods methods = 
{
  colorsel_triangle_new,
  colorsel_triangle_free,
  colorsel_triangle_setcolor
};


static GimpModuleInfo info = {
    NULL,
    N_("Painter-style color selector as a pluggable color selector"),
    "Simon Budig <Simon.Budig@unix-ag.org>",
    "v0.02",
    "(c) 1999, released under the GPL",
    "17 Jan 1999"
};

static const GtkTargetEntry targets[] = {
  { "application/x-color", 0 }
};


#define COLORWHEELRADIUS 100
#define COLORTRIANGLERADIUS 80
#define PREVIEWSIZE (2 * COLORWHEELRADIUS + 1)

#define BGCOLOR 180

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
                       GDK_BUTTON_RELEASE_MASK | \
                       GDK_BUTTON_MOTION_MASK 

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

struct _ColorSelect {
  gint                        values[6];
  gdouble                     oldsat;
  gdouble                     oldval;
  gint                        mode;
  GtkWidget                  *preview;
  GtkWidget                  *color_preview;
  GimpColorSelector_Callback  callback;
  gpointer                    data;
};

typedef struct _ColorSelect ColorSelect;


static GtkWidget * create_preview                 (ColorSelect *coldata);

static GtkWidget * create_color_preview           (ColorSelect *coldata);

static void        color_select_update_rgb_values (ColorSelect *coldata);

static void        update_previews                (ColorSelect *coldata,
						   gboolean     hue_changed);

static void        color_select_update_hsv_values (ColorSelect *coldata);


/*************************************************************/

/* globaly exported init function */
G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
  {
    GimpColorSelectorID id;

#ifndef __EMX__
    id = gimp_color_selector_register (_("Triangle"), "triangle.html", &methods);
#else
    id = mod_color_selector_register  (_("Triangle"), "triangle.html", &methods);
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
module_unload (gpointer shutdown_data,
	       void (*completed_cb) (gpointer),
	       gpointer completed_data)
{
#ifndef __EMX__
  gimp_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#else
  mod_color_selector_unregister (shutdown_data, completed_cb, completed_data);
#endif
}



/*************************************************************/
/* methods */

static GtkWidget *
colorsel_triangle_new (gint red, gint green, gint blue,
		       GimpColorSelector_Callback callback,
		       gpointer callback_data,
		       /* RETURNS: */
		       gpointer *selector_data)
{
  ColorSelect *coldata;
  GtkWidget   *preview;
  GtkWidget   *color_preview;
  GtkWidget   *frame;
  GtkWidget   *hbox;
  GtkWidget   *vbox;

  coldata = g_malloc (sizeof (ColorSelect));
  coldata->values[RED] = red;
  coldata->values[GREEN] = green;
  coldata->values[BLUE] = blue;
  color_select_update_hsv_values (coldata);

  coldata->oldsat = 0;
  coldata->oldval = 0;

  coldata->mode = 0;

  coldata->callback = callback;
  coldata->data = callback_data;

  preview = create_preview (coldata);
  coldata->preview = preview;

  color_preview = create_color_preview (coldata);
  coldata->color_preview = color_preview;

  update_previews (coldata, TRUE);

  *selector_data = coldata;

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, FALSE, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, FALSE, 0); 
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), color_preview);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, FALSE, 0); 
  gtk_widget_show_all (vbox);

  return hbox;
}


static void
colorsel_triangle_free (gpointer selector_data)
{
  /* anything else needed to go? */
  g_free (selector_data);
}


static void
colorsel_triangle_setcolor (gpointer selector_data, 
			    gint red, gint green, gint blue,
			    gint set_current)
{
  ColorSelect *coldata;

  coldata = selector_data;

  coldata->values[RED] = red;
  coldata->values[GREEN] = green;
  coldata->values[BLUE] = blue;
  color_select_update_hsv_values (coldata);
  update_previews (coldata, TRUE);
}


/*************************************************************/
/* helper functions */

static void 
color_select_update_rgb_values (ColorSelect *csp) 
{
  csp->values[RED] = RINT (((gdouble) csp->values[HUE]) / 360.0 * 255);
  csp->values[GREEN] = RINT (((gdouble) csp->values[SATURATION]) / 100.0 * 255);
  csp->values[BLUE] = RINT (((gdouble) csp->values[VALUE]) / 100.0 * 255);

  gimp_hsv_to_rgb (&(csp->values[RED]),
		   &(csp->values[GREEN]),
		   &(csp->values[BLUE]));
}

static void
color_select_update_hsv_values (ColorSelect *csp)
{
  gdouble hue, sat, val;

  hue = (double) csp->values[RED] / 255;
  sat = (double) csp->values[GREEN] / 255;
  val = (double) csp->values[BLUE] / 255;
  
  gimp_rgb_to_hsv_double (&hue, &sat, &val);

  csp->values[HUE] = RINT (hue * 360);
  csp->values[SATURATION] = RINT (sat * 100);
  csp->values[VALUE] = RINT (val * 100);
}


static void 
update_previews (ColorSelect *coldata,
		 gint         hue_changed) 
{
  GtkWidget *preview;
  guchar buf[3*PREVIEWSIZE];
  gint x, y, k, r2, dx, col;
  gint x0, y0;
  gdouble hue, sat, val, s, v, atn;
  gint hx,hy, sx,sy, vx,vy;

  hue = (gdouble) coldata->values[HUE] * G_PI / 180;

  hx = sin (hue) * COLORTRIANGLERADIUS;
  hy = cos (hue) * COLORTRIANGLERADIUS;

  sx = sin (hue - 2 * G_PI/3) * COLORTRIANGLERADIUS;
  sy = cos (hue - 2 * G_PI/3) * COLORTRIANGLERADIUS;

  vx = sin (hue + 2 * G_PI/3) * COLORTRIANGLERADIUS;
  vy = cos (hue + 2 * G_PI/3) * COLORTRIANGLERADIUS;

  hue = (gdouble) coldata->values[HUE];
  preview = coldata->preview;

  if (hue_changed) {
    for (y = COLORWHEELRADIUS; y > -COLORWHEELRADIUS; y--) {
      dx = RINT (sqrt (fabs ((COLORWHEELRADIUS) * (COLORWHEELRADIUS) - y * y)));
      for (x = -dx, k = 0; x <= dx; x++) {
        buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
        r2 = (x * x) + (y * y);
        if ( r2 <= COLORWHEELRADIUS * COLORWHEELRADIUS) {
          if (r2 > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
	    atn = atan2 (x, y);
	    if (atn < 0)
	      atn = atn + 2 * G_PI;
            gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
          } else {
            val = (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx)) /
	          (gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));
	    
            if (val >= 0 && val<= 1) {   /* normally val>=0, but this results in
					   graphics errors... */
              sat = (val == 0 ? 0: ((gdouble) (y - sy - val * (vy - sy)) /
				    (val * (gdouble) (hy - vy))));
              if (sat >= 0 && sat <= 1) 
		gimp_hsv_to_rgb4 (buf + k, hue / 360, sat, val);
            } 
          }
        }
        k += 3;
      }
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, COLORWHEELRADIUS - dx,
			    COLORWHEELRADIUS - y - 1, 2 * dx + 1);
    }
  
    /* marker in outer ring */
  
    x0 = RINT (sin (hue * G_PI/180) *
	       ((gdouble) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1) / 2 +
		COLORTRIANGLERADIUS) + 0.5);
    y0 = RINT (cos (hue * G_PI/180) *
	       ((gdouble) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1) / 2 +
		  COLORTRIANGLERADIUS) + 0.5);

    atn = atan2 (x0, y0);
    if (atn < 0)
      atn = atn + 2 * G_PI;
    gimp_hsv_to_rgb4 (buf, atn / (2 * G_PI), 1, 1);

    col = INTENSITY (buf[0], buf[1], buf[2]) > 127 ? 0 : 255 ;
  
    for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
      for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
        r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
        if (r2 <= 20 && r2 >= 6) {
          buf[k] = buf[k+1] = buf[k+2] = col;
	} else {
	  atn = atan2 (x, y);
	  if (atn < 0)
	    atn = atn + 2 * G_PI;
	  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
	}
        k += 3;
      }
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf,
			    COLORWHEELRADIUS + x0-4,
			    COLORWHEELRADIUS - 1 - y, 9);
    }
  
  
  } else {
    /* delete marker in triangle */
  
    s = coldata->oldsat;
    v = coldata->oldval;
    x0 = RINT (sx + (vx - sx) * v + (hx - vx) * s * v);
    y0 = RINT (sy + (vy - sy) * v + (hy - vy) * s * v);
    for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
      for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
        buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
        r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
        if (x * x + y * y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
	  atn = atan2 (x, y);
	  if (atn < 0)
	    atn = atn + 2 * G_PI;
	  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
        } else {
          val = (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx)) /
	        (gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));
          if (val > 0 && val <= 1) {   /* eigentlich val>=0, aber dann Grafikfehler... */
            sat = (val == 0 ? 0 : ((gdouble) (y - sy - val * (vy - sy)) /
				           (val * (gdouble) (hy - vy))));
            if (sat >= 0 && sat <= 1) 
              gimp_hsv_to_rgb4 (buf + k, hue / 360, sat, val);
          } 
        }
        k += 3;
      }
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf,
			    COLORWHEELRADIUS + x0 - 4,
			    COLORWHEELRADIUS - 1 - y, 9);
    }
  
    coldata->oldsat = coldata->values[SATURATION] / 100.0;
    coldata->oldval = coldata->values[VALUE] / 100.0;
  }
  
  /* marker in triangle */

  col = INTENSITY (coldata->values[RED], coldata->values[GREEN],
                   coldata->values[BLUE]) > 127 ? 0 : 255 ;

  s = coldata->values[SATURATION] / 100.0;
  v = coldata->values[VALUE] / 100.0;
  coldata->oldsat = s;
  coldata->oldval = v;
  x0 = RINT (sx + (vx - sx) * v + (hx - vx) * s * v);
  y0 = RINT (sy + (vy - sy) * v + (hy - vy) * s * v);
  for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
    for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
      buf[k] = buf[k+1] = buf[k+2] = BGCOLOR;
      r2 = (x - x0) * (x - x0) + (y - y0) * (y - y0);
      if (r2 <= 20 && r2 >= 6) {
        buf[k] = buf[k+1] = buf[k+2] = col;
      } else {
        if (x * x + y * y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
	  atn = atan2 (x, y);
	  if (atn < 0)
	    atn = atn + 2 * G_PI;
	  gimp_hsv_to_rgb4 (buf + k, atn / (2 * G_PI), 1, 1);
        } else {
          val = (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx)) /
	        (gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));
          if (val > 0 && val <= 1) {   /* eigentlich val>=0, aber dann Grafikfehler... */
            sat = (val == 0 ? 0 : ((gdouble) (y - sy - val * (vy - sy)) /
				           (val * (gdouble) (hy - vy))));
            if (sat >= 0 && sat <= 1) 
              gimp_hsv_to_rgb4 (buf + k, hue / 360, sat, val);
          } 
        }
      } 
      k += 3;
    }
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf,
			  COLORWHEELRADIUS + x0 - 4,
			  COLORWHEELRADIUS - 1 - y, 9);
  }
  gtk_widget_draw (preview, NULL);

  preview = coldata->color_preview;
  for (k = 0; k < (PREVIEWSIZE * 3); k+=3) {
    buf[k] = coldata->values[RED];
    buf[k+1] = coldata->values[GREEN];
    buf[k+2] = coldata->values[BLUE];
  }
  for (y = 0; y < 30; y++) {
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y, PREVIEWSIZE);
  }
  gtk_widget_draw (preview, NULL);
}


/*
 * Color Preview
 */

static gint
color_selection_callback (GtkWidget *widget, 
			  GdkEvent  *event)
{
  ColorSelect *coldata;
  gint x,y, angle, mousex, mousey;
  gdouble r;
  gdouble  hue, sat, val;
  gint hx,hy, sx,sy, vx,vy;

  coldata = gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type) {
    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);
      x = event->button.x - COLORWHEELRADIUS - 1;
      y = - event->button.y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      if ( /* r <= COLORWHEELRADIUS  && */ r > COLORTRIANGLERADIUS) 
        coldata->mode = 1;  /* Dragging in the Ring */
      else
        coldata->mode = 2;  /* Dragging in the Triangle */
      break;

    case GDK_MOTION_NOTIFY:
      x = event->motion.x - COLORWHEELRADIUS - 1;
      y = - event->motion.y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      break;

    case GDK_BUTTON_RELEASE:
      coldata->mode = 0;
      gtk_grab_remove (widget);

      /* callback the user */
      (*coldata->callback) (coldata->data,
			    coldata->values[RED],
			    coldata->values[GREEN],
			    coldata->values[BLUE]);
      
      return FALSE;
      break;

    default:
      gtk_widget_get_pointer (widget, &x, &y);
      x = x - COLORWHEELRADIUS - 1;
      y = - y + COLORWHEELRADIUS + 1;
      r = sqrt ((gdouble) (x * x + y * y));
      angle = ((gint) RINT (atan2 (x, y) / G_PI * 180) + 360 ) % 360;
      break;
  }

  gtk_widget_get_pointer (widget, &mousex, &mousey);
  if ((event->type == GDK_MOTION_NOTIFY &&
      (mousex != event->motion.x || mousey != event->motion.y)))
     return FALSE;

  if (coldata->mode == 1 ||
      (r > COLORWHEELRADIUS &&
       (abs (angle - coldata->values[HUE]) < 30 ||
	abs (abs (angle - coldata->values[HUE]) - 360) < 30))) {
    coldata->values[HUE] = angle;
    color_select_update_rgb_values (coldata);
    update_previews (coldata, TRUE);
  } else {
    hue = (gdouble) coldata->values[HUE] * G_PI / 180;
    hx = sin (hue) * COLORTRIANGLERADIUS;
    hy = cos (hue) * COLORTRIANGLERADIUS;
    sx = sin (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS;
    sy = cos (hue - 2 * G_PI / 3) * COLORTRIANGLERADIUS;
    vx = sin (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS;
    vy = cos (hue + 2 * G_PI / 3) * COLORTRIANGLERADIUS;
    hue = (gdouble) coldata->values[HUE];
    if ((x - sx) * vx + (y - sy) * vy < 0) {
      sat = 1;
      val = ((gdouble) ( (x - sx) * (hx - sx) +  (y - sy) * (hy - sy)))
		     / ((hx - sx) * (hx - sx) + (hy - sy) * (hy - sy));
      if (val < 0)
	val = 0;
      else if (val > 1)
	val = 1;
    } else
      if ((x - sx) * hx + (y - sy) * hy < 0) {
	sat = 0;
	val = ((gdouble) ( (x - sx) * (vx - sx) +  (y - sy) * (vy - sy)))
		       / ((vx - sx) * (vx - sx) + (vy - sy) * (vy - sy));
        if (val < 0)
	  val = 0;
       	else if (val > 1)
	  val = 1;
    } else if ((x - hx) * sx + (y - hy) * sy < 0) {
      val = 1;
      sat = ((gdouble) ( (x - vx) * (hx - vx) +  (y - vy) * (hy - vy)))
		     / ((hx - vx) * (hx - vx) + (hy - vy) * (hy - vy));
      if (sat < 0)
       	sat = 0;
      else if (sat > 1)
       	sat = 1;
    } else {
      val =   (gdouble) ( (x - sx) * (hy - vy) -  (y - sy) * (hx - vx))
	    / (gdouble) ((vx - sx) * (hy - vy) - (vy - sy) * (hx - vx));
      if (val <= 0) {
        val = 0;
        sat = 0;
      } else {
        if (val > 1)
	  val = 1;
        sat = (gdouble) (y - sy - val * (vy - sy)) / (val * (gdouble) (hy - vy));
        if (sat < 0)
	  sat = 0;
       	else if (sat > 1)
	  sat = 1;
      }
    }

    coldata->values[SATURATION] = 100 * sat + 0.5;
    coldata->values[VALUE] = 100 * val + 0.5;
    color_select_update_rgb_values (coldata);
    update_previews (coldata, FALSE);
  }

  /* callback the user */
  (*coldata->callback) (coldata->data,
			coldata->values[RED],
			coldata->values[GREEN],
			coldata->values[BLUE]);

  return FALSE;
}

static GtkWidget *
create_preview (ColorSelect *coldata)
{
  GtkWidget *preview;
  guchar buf[3 * PREVIEWSIZE];
  gint i;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (preview), GDK_RGB_DITHER_MAX);
  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_MASK );
  gtk_preview_size (GTK_PREVIEW (preview), PREVIEWSIZE, PREVIEWSIZE);

  gtk_object_set_user_data (GTK_OBJECT (preview), coldata);

  gtk_signal_connect (GTK_OBJECT (preview), "motion_notify_event",
		      GTK_SIGNAL_FUNC (color_selection_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_press_event",
		      GTK_SIGNAL_FUNC (color_selection_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (preview), "button_release_event",
		      GTK_SIGNAL_FUNC (color_selection_callback), NULL);

  for (i=0; i < 3 * PREVIEWSIZE; i += 3)
    buf[i] = buf[i+1] = buf[i+2] = BGCOLOR;
  for (i=0; i < PREVIEWSIZE; i++) 
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, PREVIEWSIZE);
  gtk_widget_draw (preview, NULL);

  return preview;
}

static GtkWidget *
create_color_preview (ColorSelect *coldata)
{
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (preview), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (preview), PREVIEWSIZE, 30 /* BAD! */);

  gtk_drag_dest_set (preview,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     GDK_ACTION_COPY);
  gtk_drag_source_set (preview, 
		       GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
		       targets, 1,
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_signal_connect (GTK_OBJECT (preview),
		      "drag_begin",
		      GTK_SIGNAL_FUNC (colorsel_triangle_drag_begin),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (preview),
		      "drag_end",
		      GTK_SIGNAL_FUNC (colorsel_triangle_drag_end),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (preview),
		      "drag_data_get",
		      GTK_SIGNAL_FUNC (colorsel_triangle_drag_handle),
		      coldata);
  gtk_signal_connect (GTK_OBJECT (preview),
		      "drag_data_received",
		      GTK_SIGNAL_FUNC (colorsel_triangle_drop_handle),
		      coldata);  
  return preview;
}


static void        
colorsel_triangle_drag_begin (GtkWidget      *widget,
			      GdkDragContext *context,
			      gpointer        data)
{
  GtkWidget *window;
  GdkColor bg;
  ColorSelect *coldata;

  coldata = (ColorSelect *) data;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
  gtk_widget_set_usize (window, 32, 32);
  gtk_widget_realize (window);
  gtk_object_set_data_full (GTK_OBJECT (widget),
			    "gimp-color-drag-window",
			    window,
			    (GtkDestroyNotify) gtk_widget_destroy);

  bg.red = 256 * coldata->values[RED];
  bg.green = 256 * coldata->values[GREEN];
  bg.blue = 256 * coldata->values[BLUE];

  gdk_color_alloc (gtk_widget_get_colormap (window), &bg);
  gdk_window_set_background (window->window, &bg);

  gtk_drag_set_icon_widget (context, window, -2, -2);
}


static void        
colorsel_triangle_drag_end (GtkWidget      *widget,
			    GdkDragContext *context,
			    gpointer        data)
{
  gtk_object_set_data (GTK_OBJECT (widget),
		       "gimp-color-drag-window", NULL);
}

static void        
colorsel_triangle_drop_handle (GtkWidget        *widget, 
			       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       gpointer          data)
{
  guint16 *vals;
  ColorSelect *coldata;

  coldata = (ColorSelect *) data;

  if (selection_data->length < 0)
    return;

  if ((selection_data->format != 16) || 
      (selection_data->length != 8))
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *) selection_data->data;

  coldata->values[RED]   = vals[0] / 256;
  coldata->values[GREEN] = vals[1] / 256;
  coldata->values[BLUE]  = vals[2] / 256;
  
  color_select_update_hsv_values (coldata);
  update_previews (coldata, TRUE);
}

static void        
colorsel_triangle_drag_handle (GtkWidget        *widget, 
			       GdkDragContext   *context,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             time,
			       gpointer          data)
{
  guint16 vals[4];
  ColorSelect *coldata;

  coldata = (ColorSelect *) data;

  vals[0] = coldata->values[RED] * 256;
  vals[1] = coldata->values[GREEN] * 256;
  vals[2] = coldata->values[BLUE] * 256;
  vals[3] = 0xffff;

  gtk_selection_data_set (selection_data,
			  gdk_atom_intern ("application/x-color", FALSE),
			  16, (guchar *) vals, 8);
}

