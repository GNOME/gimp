#include <stdio.h>

#include "FractalExplorer.h"
#include "Events.h"
#include "Dialogs.h"

/**********************************************************************
 FUNCTION: preview_button_press_event
 *********************************************************************/

gint
preview_button_press_event (GtkWidget      *widget,
			    GdkEventButton *event)
{
  gdouble  left;
  gdouble  right;
  gdouble  bottom;
  gdouble  top;
  gdouble  dx;
  gdouble  dy;
  gint     px;
  gint     py;
  gint     x;
  gint     y;
  gushort  r;
  gushort  g;
  gushort  b;
  guchar  *p_ul;
  guchar  *i;
  guchar  *p;

  if (event->button == 1)
    {
      x_press = event->x;
      y_press = event->y;
      left = sel_x1;
      right = sel_x2 - 1;
      bottom = sel_y2 - 1;
      top = sel_y1;
      dx = (right - left) / (preview_width - 1);
      dy = (bottom - top) / (preview_height - 1);
      xbild = preview_width;
      ybild = preview_height;
      xdiff = (xmax - xmin) / xbild;
      ydiff = (ymax - ymin) / ybild;

      py = y_press;
      px = 0;
      p_ul = wint.wimage + 3 * (preview_width * py + 0);

      for (x = 0; x < preview_width; x++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3;
	  px += 1;
	}

      py = 0;
      p_ul = wint.wimage + 3 * (preview_width * 0 + (int) x_press);
      px = x_press;

      for (y = 0; y < preview_height; y++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3 * (preview_width);
	  py += 1;
	}

      p = wint.wimage;

      for (y = 0; y < preview_height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (wint.preview), p,
				0, y, preview_width);
	  p += preview_width * 3;
	}

      gtk_widget_draw (wint.preview, NULL);
      gdk_flush ();
    }
  return TRUE;
}

/**********************************************************************
 FUNCTION: preview_motion_notify_event
 *********************************************************************/

gint
preview_motion_notify_event (GtkWidget      *widget,
			     GdkEventButton *event)
{
  gint     px;
  gint     py;
  gint     x;
  gint     y;
  gushort  r;
  gushort  g;
  gushort  b;
  guchar  *p_ul;
  guchar  *i;
  guchar  *p;

  ypos = oldypos;
  xpos = oldxpos;

  if (oldypos != -1)
    {
      py = ypos;
      px = 0;
      p_ul = wint.wimage + 3 * (preview_width * py + 0);

      for (x = 0; x < preview_width; x++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3;
	  px += 1;
	}

      py = 0;
      p_ul = wint.wimage + 3 * (preview_width * 0 + (int) xpos);
      px = xpos;

      for (y = 0; y < preview_height; y++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3 * (preview_width);
	  py += 1;
	}
    }
  oldxpos = xpos = event->x;
  oldypos = ypos = event->y;

  if ((xpos >= 0.0) &&
      (ypos >= 0.0) &&
      (xpos < preview_width) &&
      (ypos < preview_height))
    {
      py = ypos;
      px = 0;
      p_ul = wint.wimage + 3 * (preview_width * py + 0);

      for (x = 0; x < preview_width; x++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3;
	  px += 1;
	}

      py = 0;
      p_ul = wint.wimage + 3 * (preview_width * 0 + (int) xpos);
      px = xpos;

      for (y = 0; y < preview_height; y++)
	{
	    i = wint.wimage + 3 * (preview_width * py + px);
	    r = (*i++) ^ 254;
	    g = (*i++) ^ 254;
	    b = (*i) ^ 254;
	    p_ul[0] = r;
	    p_ul[1] = g;
	    p_ul[2] = b;
	    p_ul += 3 * (preview_width);
	    py += 1;
	}			/* for */
    }
  else
    {
      oldypos = -1;
      oldxpos = -1;
    }

  p = wint.wimage;

  for (y = 0; y < preview_height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (wint.preview), p, 0, y, preview_width);
      p += preview_width * 3;
    }

  gtk_widget_draw (wint.preview, NULL);
  gdk_flush ();

  return TRUE;
}

/**********************************************************************
 FUNCTION: preview_leave_notify_event
 *********************************************************************/

gint
preview_leave_notify_event (GtkWidget      *widget,
			    GdkEventButton *event)
{
  gint     px;
  gint     py;
  gint     x;
  gint     y;
  gushort  r;
  gushort  g;
  gushort  b;
  guchar  *p_ul;
  guchar  *i;
  guchar  *p;

  ypos = oldypos;
  xpos = oldxpos;

  if (oldypos != -1)
    {
      py = ypos;
      px = 0;
      p_ul = wint.wimage + 3 * (preview_width * py + 0);

      for (x = 0; x < preview_width; x++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3;
	  px += 1;
	}

      py = 0;
      p_ul = wint.wimage + 3 * (preview_width * 0 + (int) xpos);
      px = xpos;

      for (y = 0; y < preview_height; y++)
	{
	  i = wint.wimage + 3 * (preview_width * py + px);
	  r = (*i++) ^ 254;
	  g = (*i++) ^ 254;
	  b = (*i) ^ 254;
	  p_ul[0] = r;
	  p_ul[1] = g;
	  p_ul[2] = b;
	  p_ul += 3 * (preview_width);
	  py += 1;
	}
    }
  oldxpos = -1;
  oldypos = -1;

  p = wint.wimage;

  for (y = 0; y < preview_height; y++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (wint.preview), p, 0, y, preview_width);
      p += preview_width * 3;
    }

  gtk_widget_draw (wint.preview, NULL);
  gdk_flush ();

  MyCursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);
  gdk_window_set_cursor (maindlg->window, MyCursor);

  return TRUE;
}

/**********************************************************************
 FUNCTION: preview_enter_notify_event
 *********************************************************************/

gint
preview_enter_notify_event (GtkWidget      *widget,
			    GdkEventButton *event)
{
  MyCursor = gdk_cursor_new (GDK_TCROSS);
  gdk_window_set_cursor (maindlg->window, MyCursor);

  return TRUE;
}

/**********************************************************************
 FUNCTION: preview_button_release_event
 *********************************************************************/

gint
preview_button_release_event (GtkWidget      *widget,
			      GdkEventButton *event)
{
  gdouble l_xmin;
  gdouble l_xmax;
  gdouble l_ymin;
  gdouble l_ymax;

  if (event->button == 1)
    {
      x_release = event->x;
      y_release = event->y;

      if ((x_press >= 0.0) && (y_press >= 0.0) &&
	  (x_release >= 0.0) && (y_release >= 0.0) &&
	  (x_press < preview_width) && (y_press < preview_height) &&
	  (x_release < preview_width) && (y_release < preview_height))
	{
	  l_xmin = (wvals.xmin +
		    (wvals.xmax - wvals.xmin) * (x_press / preview_width));
	  l_xmax = (wvals.xmin +
		    (wvals.xmax - wvals.xmin) * (x_release / preview_width));
	  l_ymin = (wvals.ymin +
		    (wvals.ymax - wvals.ymin) * (y_press / preview_height));
	  l_ymax = (wvals.ymin +
		    (wvals.ymax - wvals.ymin) * (y_release / preview_height));

	  zooms[zoomindex] = wvals;
	  zoomindex++;
	  if (zoomindex > 99)
	    zoomindex = 99;
	  zoommax = zoomindex;
	  wvals.xmin = l_xmin;
	  wvals.xmax = l_xmax;
	  wvals.ymin = l_ymin;
	  wvals.ymax = l_ymax;
	  dialog_change_scale ();
	  dialog_update_preview ();
	  oldypos = oldxpos = -1;
	}
    }

  return TRUE;
}
