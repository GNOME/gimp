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
 *
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libgimp/color_selector.h>
#include <libgimp/gimpmodule.h>
#include <math.h>


/* prototypes */
static GtkWidget * colorsel_triangle_new (int, int, int,
					  GimpColorSelector_Callback, void *,
					  void **);
static void        colorsel_triangle_free (void *);
static void        colorsel_triangle_setcolor (void *, int, int, int, int);



/* local methods */
static GimpColorSelectorMethods methods = 
{
  colorsel_triangle_new,
  colorsel_triangle_free,
  colorsel_triangle_setcolor
};


static GimpModuleInfo info = {
    NULL,
    "Painter-style colour selector as a pluggable colour selector",
    "Simon Budig <Simon.Budig@unix-ag.org>",
    "v0.01",
    "(c) 1999, released under the GPL",
    "17 Jan 1999"
};



#define COLORWHEELRADIUS 100
#define COLORTRIANGLERADIUS 80
#define PREVIEWSIZE (2*COLORWHEELRADIUS+1)
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)

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

typedef struct _ColorSelect _ColorSelect, *ColorSelectP;

struct _ColorSelect {
  gint                        values[6];
  gfloat                      oldsat;
  gfloat                      oldval;
  gint                        mode;
  GtkWidget                  *preview;
  GimpColorSelector_Callback  callback;
  void                       *data;
};


static GtkWidget * create_color_preview (ColorSelectP);
static void color_select_update_rgb_values (ColorSelectP);
static void update_color_preview (ColorSelectP, GtkWidget *, gint);
static void color_select_update_hsv_values (ColorSelectP);


#ifdef __EMX__
struct main_funcs_struc {
  gchar *name;
  void (*func)();
};
struct main_funcs_struc *gimp_main_funcs = NULL;

static gpointer
get_main_func(gchar *name)
{
  struct main_funcs_struc *x;
  if (gimp_main_funcs == NULL)
    return NULL;
  for (x = gimp_main_funcs; x->name; x++)
  {
    if (!strcmp(x->name, name))
      return (gpointer) x->func;
  }
}
typedef GimpColorSelectorID (*color_reg_func)(const char *,
                                              GimpColorSelectorMethods *);
typedef gboolean (*color_unreg_func) (GimpColorSelectorID,
                                      void (*)(void *),
                                      void *);
#endif


/*************************************************************/

/* globaly exported init function */
G_MODULE_EXPORT GimpModuleStatus
module_init (GimpModuleInfo **inforet)
{
  GimpColorSelectorID id;

#ifndef __EMX__
  id = gimp_color_selector_register ("Triangle", &methods);

  if (id)
#else
  color_reg_func reg_func;
  reg_func = (color_reg_func) get_main_func("gimp_color_selector_register");
  if (reg_func && (id = (*reg_func) ("Triangle", &methods)))
#endif
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
  color_unreg_func unreg_func;
  unreg_func = (color_unreg_func) get_main_func("gimp_color_selector_unregister"
);
  if (unreg_func)
    (*unreg_func) (shutdown_data, completed_cb, completed_data);
#endif
}



/*************************************************************/
/* methods */

static GtkWidget * colorsel_triangle_new (int r, int g, int b,
					  GimpColorSelector_Callback callback,
					  void *callback_data,
					  /* RETURNS: */
					  void **selector_data)
{
  ColorSelectP coldata;
  GtkWidget *preview;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;

  coldata = g_malloc (sizeof (_ColorSelect));
  coldata->values[RED] = r;
  coldata->values[GREEN] = g;
  coldata->values[BLUE] = b;
  color_select_update_hsv_values(coldata);

  coldata->oldsat = 0;
  coldata->oldval = 0;

  coldata->mode = 0;

  coldata->callback = callback;
  coldata->data = callback_data;

  /* gtk_rc_parse ("colorselrc"); */

  preview = create_color_preview (coldata);
  coldata->preview = preview;

  *selector_data = coldata;

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, FALSE, 0); 
  gtk_widget_show_all (hbox);

  return vbox;
}


static void
colorsel_triangle_free (void *selector_data)
{
  /* anything else needed to go? */
  g_free (selector_data);
}


static void
colorsel_triangle_setcolor (void *selector_data, int r, int g, int b,
			    int set_current)
{
  ColorSelectP coldata;

  coldata = selector_data;

  coldata->values[RED] = r;
  coldata->values[GREEN] = g;
  coldata->values[BLUE] = b;
  color_select_update_hsv_values(coldata);
  update_color_preview(coldata, coldata->preview, 1);
}




/*************************************************************/
/* helper functions */


/* Conversion hsv->rgb */

static void color_hsv_to_rgb (gfloat hue, gfloat sat, gfloat val, guchar* red, guchar *green, guchar *blue) {
  gfloat f, p, q, t;

  if (sat == 0) {
      *red = val * 255;
      *green = val * 255;
      *blue = val * 255;
  } else {
      while (hue < 0)
        hue += 360;
      while (hue >= 360)
        hue -= 360;
      
      hue /= 60;
      f = hue - (int) hue;
      p = val * (1 - sat);
      q = val * (1 - (sat * f));
      t = val * (1 - (sat * (1 - f)));

      switch ((int) hue) {
        case 0:
          *red = val * 255;
          *green = t * 255;
          *blue = p * 255;
          break;
        case 1:
          *red = q * 255;
          *green = val * 255;
          *blue = p * 255;
          break;
        case 2:
          *red = p * 255;
          *green = val * 255;
          *blue = t * 255;
          break;
        case 3:
          *red = p * 255;
          *green = q * 255;
          *blue = val * 255;
          break;
        case 4:
          *red = t * 255;
          *green = p * 255;
          *blue = val * 255;
          break;
        case 5:
          *red = val * 255;
          *green = p * 255;
          *blue = q * 255;
          break;
        default:
          break;
      }
    }
}


static void color_select_update_rgb_values (ColorSelectP csp) {
  gfloat h, s, v;
  gfloat f, p, q, t;

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



static void update_color_preview (ColorSelectP coldata,
				  GtkWidget *preview, gint hue_changed) {
  guchar buf[3*PREVIEWSIZE];
  gint x, y, k, r2, dx, col;
  gint x0, y0;
  gfloat hue, sat, val, s, v;
  gint hx,hy, sx,sy, vx,vy;

  hue = (float) coldata->values[HUE] * M_PI / 180;

  hx = sin(hue) * COLORTRIANGLERADIUS;
  hy = cos(hue) * COLORTRIANGLERADIUS;

  sx = sin(hue - 2*M_PI/3) * COLORTRIANGLERADIUS;
  sy = cos(hue - 2*M_PI/3) * COLORTRIANGLERADIUS;

  vx = sin(hue + 2*M_PI/3) * COLORTRIANGLERADIUS;
  vy = cos(hue + 2*M_PI/3) * COLORTRIANGLERADIUS;

  hue = (float) coldata->values[HUE];

  if (hue_changed) {
    for (y = COLORWHEELRADIUS; y > -COLORWHEELRADIUS; y--) {
      dx = (int) sqrt((float) abs((COLORWHEELRADIUS)*(COLORWHEELRADIUS)-y*y)); 
      for (x = -dx, k=0; x <= dx; x++) {
        buf[k]=buf[k+1]=buf[k+2]=BGCOLOR;
        r2 = (x*x)+(y*y);
        if ( r2 <= COLORWHEELRADIUS * COLORWHEELRADIUS) {
          if (r2 > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
            color_hsv_to_rgb (atan2 (x,y) / M_PI * 180, 1, 1, &buf[k], &buf[k+1], &buf[k+2]);
          } else {
            val = (float) ((x-sx)*(hy-vy)-(y-sy)*(hx-vx)) / (float) ((vx-sx)*(hy-vy)-(vy-sy)*(hx-vx));
            if (val>0 && val<=1) {   /* eigentlich val>=0, aber dann Grafikfehler... */
              sat = (val==0?0: ((float) (y-sy-val*(vy-sy)) / (val * (float) (hy-vy))));
              if (sat >= 0 && sat <= 1) 
                color_hsv_to_rgb (hue, sat, val, &buf[k], &buf[k+1], &buf[k+2]);
            } 
          }
        }
        k += 3;
      }
      /* gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, COLORWHEELRADIUS - y - 1, PREVIEWSIZE); */
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, COLORWHEELRADIUS - dx, COLORWHEELRADIUS - y - 1, 2*dx+1);
    }
  
    /* Marker im aeusseren Ring */
  
    x0 = (gint) (sin(hue*M_PI/180) * ((float) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1)/2 + COLORTRIANGLERADIUS) + 0.5);
    y0 = (gint) (cos(hue*M_PI/180) * ((float) (COLORWHEELRADIUS - COLORTRIANGLERADIUS + 1)/2 + COLORTRIANGLERADIUS) + 0.5);
    color_hsv_to_rgb (atan2 (x0,y0) / M_PI * 180, 1, 1, &buf[0], &buf[1], &buf[2]);
    col = INTENSITY(buf[0], buf[1], buf[2]) > 127 ? 0 : 255 ;
  
    for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
      for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
        r2 = (x-x0)*(x-x0)+(y-y0)*(y-y0);
        if (r2 <= 20 && r2 >= 6)
          buf[k]=buf[k+1]=buf[k+2]=col;
        else
          color_hsv_to_rgb (atan2 (x,y) / M_PI * 180, 1, 1, &buf[k], &buf[k+1], &buf[k+2]);
        k += 3;
      }
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, COLORWHEELRADIUS + x0-4, COLORWHEELRADIUS - 1 - y, 9);
    }
  
  
  } else {
    /* Marker im Dreieck loeschen */
  
    s = coldata->oldsat;
    v = coldata->oldval;
    x0 = (gint) (sx + (vx - sx)*v + (hx - vx) * s * v);
    y0 = (gint) (sy + (vy - sy)*v + (hy - vy) * s * v);
    for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
      for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
        buf[k]=buf[k+1]=buf[k+2]=BGCOLOR;
        r2 = (x-x0)*(x-x0)+(y-y0)*(y-y0);
        if (x*x+y*y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
          color_hsv_to_rgb (atan2 (x,y) / M_PI * 180, 1, 1, &buf[k], &buf[k+1], &buf[k+2]);
        } else {
          val = (float) ((x-sx)*(hy-vy)-(y-sy)*(hx-vx)) / (float) ((vx-sx)*(hy-vy)-(vy-sy)*(hx-vx));
          if (val>0 && val<=1) {   /* eigentlich val>=0, aber dann Grafikfehler... */
            sat = (val==0?0: ((float) (y-sy-val*(vy-sy)) / (val * (float) (hy-vy))));
            if (sat >= 0 && sat <= 1) 
              color_hsv_to_rgb (hue, sat, val, &buf[k], &buf[k+1], &buf[k+2]);
          } 
        }
        k += 3;
      }
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, COLORWHEELRADIUS + x0-4, COLORWHEELRADIUS - 1 - y, 9);
    }
  
    coldata->oldsat = coldata->values[SATURATION] / 100.0;
    coldata->oldval = coldata->values[VALUE] / 100.0;
  }
  
  /* Marker im Dreieck */

  col = INTENSITY(coldata->values[RED], coldata->values[GREEN],
                  coldata->values[BLUE]) > 127 ? 0 : 255 ;

  s = coldata->values[SATURATION] / 100.0;
  v = coldata->values[VALUE] / 100.0;
  coldata->oldsat=s;
  coldata->oldval=v;
  x0 = (gint) (sx + (vx - sx)*v + (hx - vx) * s * v);
  y0 = (gint) (sy + (vy - sy)*v + (hy - vy) * s * v);
  for (y = y0 - 4 ; y <= y0 + 4 ; y++) {
    for (x = x0 - 4, k=0 ; x <= x0 + 4 ; x++) {
      buf[k]=buf[k+1]=buf[k+2]=BGCOLOR;
      r2 = (x-x0)*(x-x0)+(y-y0)*(y-y0);
      if (r2 <= 20 && r2 >= 6)
        buf[k]=buf[k+1]=buf[k+2]=col;
      else {
        if (x*x+y*y > COLORTRIANGLERADIUS * COLORTRIANGLERADIUS) { 
          color_hsv_to_rgb (atan2 (x,y) / M_PI * 180, 1, 1, &buf[k], &buf[k+1], &buf[k+2]);
        } else {
          val = (float) ((x-sx)*(hy-vy)-(y-sy)*(hx-vx)) / (float) ((vx-sx)*(hy-vy)-(vy-sy)*(hx-vx));
          if (val>0 && val<=1) {   /* eigentlich val>=0, aber dann Grafikfehler... */
            sat = (val==0?0: ((float) (y-sy-val*(vy-sy)) / (val * (float) (hy-vy))));
            if (sat >= 0 && sat <= 1) 
              color_hsv_to_rgb (hue, sat, val, &buf[k], &buf[k+1], &buf[k+2]);
          } 
        }
      } 
      k += 3;
    }
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, COLORWHEELRADIUS + x0-4, COLORWHEELRADIUS - 1 - y, 9);
  }

  for (k=0; k < (PREVIEWSIZE * 3); k+=3) {
    buf[k]=coldata->values[RED];
    buf[k+1]=coldata->values[GREEN];
    buf[k+2]=coldata->values[BLUE];
  }
  for (y=0; y < 30; y++) {
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, y+PREVIEWSIZE, PREVIEWSIZE);
  }

  gtk_widget_draw (preview, NULL);

}

static void
init_color_preview (GtkWidget *preview)
{
  guchar buf[3*PREVIEWSIZE];
  gint i;
  for (i=0; i < 3*PREVIEWSIZE; i+=3)
    buf[i]=buf[i+1]=buf[i+2]=BGCOLOR;
  for (i=0; i < PREVIEWSIZE; i++) 
    gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, PREVIEWSIZE);
  gtk_widget_draw (preview, NULL);
}

/*
 * Color Preview
 */

static gint
color_selection_callback(GtkWidget *widget, GdkEvent *event)
{
  ColorSelectP coldata;
  gint x,y, mousex, mousey;
  gfloat r;
  gfloat hue, sat, val;
  gint hx,hy, sx,sy, vx,vy;

  coldata = gtk_object_get_user_data (GTK_OBJECT (widget));

  switch (event->type) {
    case GDK_BUTTON_PRESS:
      x = event->button.x - COLORWHEELRADIUS - 1;
      y = - event->button.y + COLORWHEELRADIUS + 1;
      r = sqrt((float) (x*x+y*y));
      if ( /* r <= COLORWHEELRADIUS  && */ r > COLORTRIANGLERADIUS) 
        coldata->mode = 1;  /* Dragging in the Ring */
      else
        coldata->mode = 2;  /* Dragging in the Triangle */
      break;

    case GDK_MOTION_NOTIFY:
      x = event->motion.x - COLORWHEELRADIUS - 1;
      y = - event->motion.y + COLORWHEELRADIUS + 1;
      break;

    case GDK_BUTTON_RELEASE:
      coldata->mode = 0;
      break;

    default:
      gtk_widget_get_pointer(widget, &x, &y);
      x = x - COLORWHEELRADIUS - 1;
      y = - y + COLORWHEELRADIUS + 1;
      break;
  }

  gtk_widget_get_pointer(widget, &mousex, &mousey);
  if ((event->type == GDK_MOTION_NOTIFY &&
        (mousex != event->motion.x || mousey != event->motion.y)))
     return FALSE;

  if (coldata->mode == 1) {
    coldata->values[HUE] = ( (int) (atan2 (x, y) / M_PI * 180) + 360 ) %360;
    color_select_update_rgb_values(coldata);
    update_color_preview(coldata, widget, 1);
  }
  if (coldata->mode == 2) {
    hue = (float) coldata->values[HUE] * M_PI / 180;
    hx = sin(hue) * COLORTRIANGLERADIUS;
    hy = cos(hue) * COLORTRIANGLERADIUS;
    sx = sin(hue - 2*M_PI/3) * COLORTRIANGLERADIUS;
    sy = cos(hue - 2*M_PI/3) * COLORTRIANGLERADIUS;
    vx = sin(hue + 2*M_PI/3) * COLORTRIANGLERADIUS;
    vy = cos(hue + 2*M_PI/3) * COLORTRIANGLERADIUS;
    hue = (float) coldata->values[HUE];
    if ((x-sx)*vx+(y-sy)*vy < 0) {
      sat = 1;
      val = ((float) ((x-sx)*(hx-sx)+(y-sy)*(hy-sy)))/((hx-sx)*(hx-sx)+(hy-sy)*(hy-sy));
      if (val<0) val=0; else if (val>1) val=1;
    } else if ((x-sx)*hx+(y-sy)*hy < 0) {
      sat = 0;
      val = ((float) ((x-sx)*(vx-sx)+(y-sy)*(vy-sy)))/((vx-sx)*(vx-sx)+(vy-sy)*(vy-sy));
      if (val<0) val=0; else if (val>1) val=1;
    } else if ((x-hx)*sx+(y-hy)*sy < 0) {
      val = 1;
      sat = ((float) ((x-vx)*(hx-vx)+(y-vy)*(hy-vy)))/((hx-vx)*(hx-vx)+(hy-vy)*(hy-vy));
      if (sat<0) sat=0; else if (sat>1) sat=1;
    } else {
      val = (float) ((x-sx)*(hy-vy)-(y-sy)*(hx-vx)) / (float) ((vx-sx)*(hy-vy)-(vy-sy)*(hx-vx));
      if (val<=0) {
        val=0;
        sat=0;
      } else {
        if (val>1) val=1;
        sat = (float) (y-sy-val*(vy-sy)) / (val * (float) (hy-vy));
        if (sat<0) sat=0; else if (sat>1) sat=1;
      }
    }

    coldata->values[SATURATION]=100*sat+0.5;
    coldata->values[VALUE]=100*val+0.5;
    color_select_update_rgb_values(coldata);
    update_color_preview(coldata, widget, 0);

  }

  /* callback the user */
  (*coldata->callback) (coldata->data,
			coldata->values[RED],
			coldata->values[GREEN],
			coldata->values[BLUE]);

  return FALSE;
}

static GtkWidget *
create_color_preview (ColorSelectP coldata)
{
  GtkWidget *preview;

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (preview), GDK_RGB_DITHER_MAX);
  gtk_widget_set_events( GTK_WIDGET(preview), PREVIEW_MASK );
  gtk_preview_size (GTK_PREVIEW (preview), PREVIEWSIZE, PREVIEWSIZE + 30 /* BAD! */);

  gtk_object_set_user_data (GTK_OBJECT (preview), coldata);

  gtk_signal_connect (GTK_OBJECT(preview), "motion_notify_event",
		      GTK_SIGNAL_FUNC(color_selection_callback), NULL);
  gtk_signal_connect (GTK_OBJECT(preview), "button_press_event",
		      GTK_SIGNAL_FUNC(color_selection_callback), NULL);
  gtk_signal_connect (GTK_OBJECT(preview), "button_release_event",
		      GTK_SIGNAL_FUNC(color_selection_callback), NULL);
  init_color_preview (preview);
  update_color_preview (coldata, preview, 1);

  return preview;
}


