/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbrushmenu.c
 * Copyright (C) 1998 Andy Thomas                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


/* Idea is to have a function to call that returns a widget that 
 * completely controls the selection of a brush.
 * you get a widget returned that you can use in a table say.
 * In:- Initial brush name. Null means use current selection.
 *      pointer to func to call when brush changes (GimpRunBrushCallback).
 * Returned:- Pointer to a widget that you can use in UI.
 * 
 * Widget simply made up of a preview widget (20x20) containing the brush
 * mask and a button that can be clicked on to change the brush.
 */


#define BSEL_DATA_KEY     "__bsel_data"
#define CELL_SIZE         20
#define BRUSH_EVENT_MASK  GDK_EXPOSURE_MASK       | \
                          GDK_BUTTON_PRESS_MASK   | \
			  GDK_BUTTON_RELEASE_MASK | \
                          GDK_BUTTON1_MOTION_MASK 

struct __brushes_sel 
{
  gchar                *title;
  GimpRunBrushCallback  callback;
  GtkWidget            *brush_preview;
  GtkWidget            *device_brushpopup; 
  GtkWidget            *device_brushpreview;
  GtkWidget            *button;
  GtkWidget            *top_hbox;
  gchar                *brush_name;       /* Local copy */
  gdouble               opacity;
  gint                  spacing;
  GimpLayerModeEffects  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *mask_data;        /* local copy */
  gchar                *brush_popup_pnt;  /* used to control the popup */
  gpointer              data;
};

typedef struct __brushes_sel BSelect;


static void  brush_popup_close (BSelect *bsel);


static void
brush_popup_open (BSelect  *bsel,
		  gint      x,
		  gint      y)
{
  GtkWidget    *frame;
  const guchar *src;
  const guchar *s;
  guchar       *buf;
  guchar       *b;
  gint          x_org;
  gint          y_org;
  gint          scr_w;
  gint          scr_h;

  if (bsel->device_brushpopup)
    brush_popup_close (bsel);

  if (bsel->width <= CELL_SIZE && bsel->height <= CELL_SIZE)
    return;

  bsel->device_brushpopup = gtk_window_new (GTK_WINDOW_POPUP);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (bsel->device_brushpopup), frame);
  gtk_widget_show (frame);
  
  bsel->device_brushpreview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_container_add (GTK_CONTAINER (frame), bsel->device_brushpreview);
  gtk_widget_show (bsel->device_brushpreview);

  /* decide where to put the popup */
  gdk_window_get_origin (bsel->brush_preview->window, &x_org, &y_org);
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();

  x = x_org + x - (bsel->width  / 2);
  y = y_org + y - (bsel->height / 2);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + bsel->width  > scr_w) ? scr_w - bsel->width  : x;
  y = (y + bsel->height > scr_h) ? scr_h - bsel->height : y;

  gtk_preview_size (GTK_PREVIEW (bsel->device_brushpreview),
                    bsel->width, bsel->height);

  gtk_window_move (GTK_WINDOW (bsel->device_brushpopup), x, y);
  
  /*  Draw the brush  */
  buf = g_new (guchar, bsel->width);
  memset (buf, 255, bsel->width);
  
  src = bsel->mask_data;

  for (y = 0; y < bsel->height; y++)
    {
      int j;

      s = src;
      b = buf;

      for (j = 0; j < bsel->width ; j++)
	*b++ = 255 - *s++;

      gtk_preview_draw_row (GTK_PREVIEW (bsel->device_brushpreview), 
			    buf, 0, y, bsel->width);
      src += bsel->width;
    }

  g_free (buf);

  gtk_widget_show (bsel->device_brushpopup);
}

static void
brush_popup_close (BSelect *bsel)
{
  if (bsel->device_brushpopup)
    {
      gtk_widget_destroy (bsel->device_brushpopup);
      bsel->device_brushpopup = NULL;
    }
}

static gint
brush_preview_events (GtkWidget    *widget,
		      GdkEvent     *event,
		      gpointer      data)
{
  GdkEventButton *bevent;
  BSelect        *bsel = (BSelect*)data;

  if (bsel->mask_data)
    {
      switch (event->type)
	{
	case GDK_EXPOSE:
	  break;
	  
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;
	  
	  if (bevent->button == 1)
	    {
	      gtk_grab_add (widget);
	      brush_popup_open (bsel, bevent->x, bevent->y);
	    }
	  break;
	  
	case GDK_BUTTON_RELEASE:
	  bevent = (GdkEventButton *) event;
	  
	  if (bevent->button == 1)
	    {
	      gtk_grab_remove (widget);
	      brush_popup_close (bsel);
	    }
	  break;
	case GDK_DELETE:
	  break;
	  
	default:
	  break;
	}
    }

  return FALSE;
}

static void
brush_preview_update (GtkWidget   *brush_preview,
		      gint         brush_width,
		      gint         brush_height,
		      const gchar *mask_data)
{
  guchar       *buf;
  guchar       *b;
  const guchar *src;
  const guchar *s;
  gint          i, y;
  gint          offset_x, offset_y;
  gint          ystart, yend;
  gint          width, height;

  /*  Draw the brush  */
  buf = g_new (guchar, CELL_SIZE); 

  /* Set buffer to white */  
  memset (buf, 255, CELL_SIZE);
  for (i = 0; i < CELL_SIZE; i++)
    gtk_preview_draw_row (GTK_PREVIEW (brush_preview),
			  buf, 0, i, CELL_SIZE);

 /* Limit to cell size */
  width  = (brush_width  > CELL_SIZE) ? CELL_SIZE: brush_width;
  height = (brush_height > CELL_SIZE) ? CELL_SIZE: brush_height;

  offset_x = ((CELL_SIZE - width)  / 2);
  offset_y = ((CELL_SIZE - height) / 2);

  ystart = CLAMP (offset_y, 0, CELL_SIZE);
  yend   = CLAMP (offset_y + height, 0, CELL_SIZE);

  src = mask_data;

  for (y = ystart; y < yend; y++)
    {
      gint j;

      s = src;
      b = buf;
      for (j = 0; j < width ; j++)
	*b++ = 255 - *s++;

      gtk_preview_draw_row (GTK_PREVIEW (brush_preview),
			    buf, offset_x, y, width);
      src += brush_width;
    }

  g_free (buf);

  gtk_widget_queue_draw (brush_preview);
}

static void
brush_select_invoker (const gchar          *name,
		      gdouble               opacity,
		      gint                  spacing,
		      GimpLayerModeEffects  paint_mode,
		      gint                  width,
		      gint                  height,
		      const guchar         *mask_data,
		      gboolean              closing,
		      gpointer              data)
{
  BSelect *bsel = (BSelect *) data;

  g_free (bsel->brush_name);
  g_free (bsel->mask_data);

  bsel->brush_name = g_strdup (name);
  bsel->width      = width;
  bsel->height     = height;
  bsel->mask_data  = g_memdup (mask_data, width * height);
  bsel->opacity    = opacity;
  bsel->spacing    = spacing;
  bsel->paint_mode = paint_mode;

  brush_preview_update (bsel->brush_preview, width, height, mask_data);

  if (bsel->callback)
    bsel->callback (name, opacity, spacing, paint_mode, 
		    width, height, mask_data, closing, bsel->data);

  if (closing)
    bsel->brush_popup_pnt = NULL;
}

static void
brush_select_callback (GtkWidget *widget,
		       gpointer   data)
{
  BSelect *bsel = (BSelect*)data;

  if (bsel->brush_popup_pnt)
    {
      /*  calling gimp_brushes_set_popup() raises the dialog  */
      gimp_brushes_set_popup (bsel->brush_popup_pnt, 
			      bsel->brush_name,
			      bsel->opacity,
			      bsel->spacing,
			      bsel->paint_mode);
    }
  else
    {
      bsel->brush_popup_pnt = 
	gimp_interactive_selection_brush (bsel->title ?
					  bsel->title : _("Brush Selection"),
					  bsel->brush_name,
					  bsel->opacity,
					  bsel->spacing,
					  bsel->paint_mode,
					  brush_select_invoker,
					  bsel);
    }
}

/**
 * gimp_brush_select_widget:
 * @title: Title of the dialog to use or %NULL to use the default title.
 * @brush_name: Initial brush name or %NULL to use current selection. 
 * @opacity: Initial opacity. -1 means to use current opacity.
 * @spacing: Initial spacing. -1 means to use current spacing.
 * @paint_mode: Initial paint mode.  -1 means to use current paint mode.
 * @callback: a function to call when the selected brush changes.
 * @data: a pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of a 
 * #GimpBrush.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget * 
gimp_brush_select_widget (const gchar          *title,
			  const gchar          *brush_name, 
			  gdouble               opacity,
			  gint                  spacing,
			  GimpLayerModeEffects  paint_mode,
			  GimpRunBrushCallback  callback,
			  gpointer              data)
{
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *brush;
  GtkWidget *button;
  gint       width;
  gint       height;
  gint       init_spacing;
  GimpLayerModeEffects  init_paint_mode;
  gdouble    init_opacity;
  gint       mask_data_size;
  guint8    *mask_data;
  gchar     *name;
  BSelect   *bsel;

  bsel = g_new0 (BSelect, 1);

  hbox = gtk_hbox_new (FALSE, 3);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_OUT);
  gtk_widget_show (frame);

  brush = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (brush), CELL_SIZE, CELL_SIZE); 
  gtk_widget_show (brush);
  gtk_container_add (GTK_CONTAINER (frame), brush); 

  gtk_widget_set_events (brush, BRUSH_EVENT_MASK);

  g_signal_connect (brush, "event",
                    G_CALLBACK (brush_preview_events),
                    bsel);

  bsel->callback          = callback;
  bsel->data              = data;
  bsel->device_brushpopup = bsel->device_brushpreview = NULL;
  bsel->brush_preview     = brush;
  bsel->title             = g_strdup (title);

  /* Do initial brush setup */
  name = gimp_brushes_get_brush_data ((gchar *) brush_name,
				      &init_opacity,
				      &init_spacing,
				      &init_paint_mode,
				      &width,
				      &height,
				      &mask_data_size,
				      &mask_data);

  if (name)
    {
      bsel->brush_name = name;
      bsel->opacity    = (opacity == -1.0)  ? init_opacity    : opacity;
      bsel->spacing    = (spacing == -1)    ? init_spacing    : spacing;
      bsel->paint_mode = (paint_mode == -1) ? init_paint_mode : paint_mode;
      bsel->width      = width;
      bsel->height     = height;
      bsel->mask_data  = mask_data;

      brush_preview_update (bsel->brush_preview, width, height, mask_data);
    }

  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("... ");
  gtk_container_add (GTK_CONTAINER (hbox), button); 
  gtk_widget_show (button);

  bsel->button = button;
  g_signal_connect (button, "clicked",
                    G_CALLBACK (brush_select_callback),
                    bsel);

  g_object_set_data (G_OBJECT (hbox), BSEL_DATA_KEY, bsel);

  return hbox;
}


/**
 * gimp_brush_select_widget_close_popup:
 * @widget: A brush select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_brush_select_widget_close_popup (GtkWidget *widget)
{
  BSelect  *bsel;

  bsel = (BSelect *) g_object_get_data (G_OBJECT (widget), BSEL_DATA_KEY);

  if (bsel && bsel->brush_popup_pnt)
    {
      gimp_brushes_close_popup (bsel->brush_popup_pnt);
      bsel->brush_popup_pnt = NULL;
    }
}

/**
 * gimp_brush_select_widget_set_popup:
 * @widget: A brush select widget.
 * @brush_name: Brush name to set; %NULL means no change. 
 * @opacity: Opacity to set. -1 means no change.
 * @spacing: Spacing to set. -1 means no change.
 * @paint_mode: Paint mode to set.  -1 means no change.
 *
 * Sets the current brush and other values for the brush
 * select widget.  Calls the callback function if one was
 * supplied in the call to gimp_brush_select_widget().
 */
void
gimp_brush_select_widget_set_popup (GtkWidget            *widget,
				    const gchar          *brush_name,
				    gdouble               opacity,
				    gint                  spacing,
				    GimpLayerModeEffects  paint_mode)
{
  gint     width;
  gint     height;
  gint     init_spacing;
  GimpLayerModeEffects init_paint_mode;
  gdouble  init_opacity;
  gint     mask_data_size;
  guint8  *mask_data;
  gchar   *name;
  BSelect *bsel;
  
  bsel = (BSelect *) g_object_get_data (G_OBJECT (widget), BSEL_DATA_KEY);

  if (bsel)
    {
      name = gimp_brushes_get_brush_data ((gchar *) brush_name,
					  &init_opacity,
					  &init_spacing,
					  &init_paint_mode,
					  &width,
					  &height,
					  &mask_data_size,
					  &mask_data);

      if (opacity == -1.0)  opacity    = init_opacity;
      if (spacing == -1)    spacing    = init_spacing;
      if (paint_mode == -1) paint_mode = init_paint_mode;
  
      brush_select_invoker (brush_name, opacity, spacing, paint_mode,
			    width, height, mask_data, 0, bsel);

      if (bsel->brush_popup_pnt)
	gimp_brushes_set_popup (bsel->brush_popup_pnt, 
				name, opacity, spacing, paint_mode);

      g_free (name);
      g_free (mask_data);
    }
}
