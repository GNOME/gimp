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
#include "appenv.h"
#include "cursorutil.h"
#include "dialog_handler.h"
#include "gdisplay.h" /* for gdisplay_*_override_cursor() */

#include "cursors/mouse.xbm"
#include "cursors/mouse_mask.xbm"
#include "cursors/mouse_add.xbm"
#include "cursors/mouse_add_mask.xbm"
#include "cursors/mouse_subtract.xbm"
#include "cursors/mouse_subtract_mask.xbm"
#include "cursors/mouse_intersect.xbm"
#include "cursors/mouse_intersect_mask.xbm"
#include "cursors/mouse_point.xbm"
#include "cursors/mouse_point_mask.xbm"
#include "cursors/mouse_rectangle.xbm"
#include "cursors/mouse_rectangle_mask.xbm"
#include "cursors/mouse_move.xbm"
#include "cursors/mouse_move_mask.xbm"
#include "cursors/selection.xbm"
#include "cursors/selection_mask.xbm"
#include "cursors/selection_add.xbm"
#include "cursors/selection_add_mask.xbm"
#include "cursors/selection_subtract.xbm"
#include "cursors/selection_subtract_mask.xbm"
#include "cursors/selection_intersect.xbm"
#include "cursors/selection_intersect_mask.xbm"
#include "cursors/bad.xbm"
#include "cursors/bad_mask.xbm"
#include "cursors/dropper.xbm"
#include "cursors/dropper_mask.xbm"
#include "cursors/zoom_in.xbm"
#include "cursors/zoom_in_mask.xbm"
#include "cursors/zoom_out.xbm"
#include "cursors/zoom_out_mask.xbm"

typedef struct
{
  guchar    *bits;
  guchar    *mask_bits;
  gint       width, height;
  gint       x_hot, y_hot;
  GdkCursor *cursor;
} BM_Cursor;
    
static BM_Cursor gimp_cursors[] =
/* these have to match up with the enum in cursorutil.h */
{
  {
    mouse_bits, mouse_mask_bits,
    mouse_width, mouse_height,
    mouse_x_hot, mouse_y_hot, NULL
  },
  {
    mouse_add_bits, mouse_add_mask_bits,
    mouse_add_width, mouse_add_height,
    mouse_add_x_hot, mouse_add_y_hot, NULL
  },
  {
    mouse_subtract_bits, mouse_subtract_mask_bits,
    mouse_subtract_width, mouse_subtract_height,
    mouse_subtract_x_hot, mouse_subtract_y_hot, NULL
  },
  {
    mouse_intersect_bits, mouse_intersect_mask_bits,
    mouse_intersect_width, mouse_intersect_height,
    mouse_intersect_x_hot, mouse_intersect_y_hot, NULL
  },
  {
    mouse_point_bits, mouse_point_mask_bits,
    mouse_point_width, mouse_point_height,
    mouse_point_x_hot, mouse_point_y_hot, NULL
  },
  {
    mouse_rectangle_bits, mouse_rectangle_mask_bits,
    mouse_rectangle_width, mouse_rectangle_height,
    mouse_rectangle_x_hot, mouse_rectangle_y_hot, NULL
  },
  {
    mouse_move_bits, mouse_move_mask_bits,
    mouse_move_width, mouse_move_height,
    mouse_move_x_hot, mouse_move_y_hot, NULL
  },
  {
    selection_bits, selection_mask_bits,
    selection_width, selection_height,
    selection_x_hot, selection_y_hot, NULL
  },
  {
    selection_add_bits, selection_add_mask_bits,
    selection_add_width, selection_add_height,
    selection_add_x_hot, selection_add_y_hot, NULL
  },
  {
    selection_subtract_bits, selection_subtract_mask_bits,
    selection_subtract_width, selection_subtract_height,
    selection_subtract_x_hot, selection_subtract_y_hot, NULL
  },
  {
    selection_intersect_bits, selection_intersect_mask_bits,
    selection_intersect_width, selection_intersect_height,
    selection_intersect_x_hot, selection_intersect_y_hot, NULL
  },
  {
    bad_bits, bad_mask_bits,
    bad_width, bad_height,
    bad_x_hot, bad_y_hot, NULL
  },
  {
    dropper_bits, dropper_mask_bits,
    dropper_width, dropper_height,
    dropper_x_hot, dropper_y_hot, NULL
  },
  {
    zoom_in_bits, zoom_in_mask_bits,
    zoom_in_width, zoom_in_height,
    zoom_in_x_hot, zoom_in_y_hot, NULL
  },
  {
    zoom_out_bits, zoom_out_mask_bits,
    zoom_out_width, zoom_out_height,
    zoom_out_x_hot, zoom_out_y_hot, NULL
  }
};


extern GSList   *display_list; /* It's in gdisplay.c, FYI */
static gboolean  pending_removebusy = FALSE;


static void
create_cursor (BM_Cursor *bmcursor)
{
  GdkPixmap *pixmap;
  GdkPixmap *pixmapmsk;
  GdkColor fg, bg;

  /* should have a way to configure the mouse colors */
  gdk_color_parse ("#FFFFFF", &bg);
  gdk_color_parse ("#000000", &fg);

  pixmap = gdk_bitmap_create_from_data (NULL, bmcursor->bits,
					bmcursor->width, bmcursor->height);
  g_return_if_fail (pixmap != NULL);

  pixmapmsk = gdk_bitmap_create_from_data (NULL, bmcursor->mask_bits,
					   bmcursor->width,
					   bmcursor->height);
  g_return_if_fail (pixmapmsk != NULL);

  bmcursor->cursor = gdk_cursor_new_from_pixmap (pixmap, pixmapmsk, &fg, &bg,
						 bmcursor->x_hot,
						 bmcursor->y_hot);
  g_return_if_fail (bmcursor->cursor != NULL);
}

static void
gimp_change_win_cursor (GdkWindow      *win,
			GimpCursorType  curtype)
{
  GdkCursor *cursor;

  g_return_if_fail (curtype < GIMP_LAST_CURSOR_ENTRY);
  curtype -= GIMP_MOUSE_CURSOR;
  if (!gimp_cursors[(int)curtype].cursor)
    create_cursor (&gimp_cursors[(int)curtype]);
  cursor = gimp_cursors[(int)curtype].cursor;
  gdk_window_set_cursor (win, cursor);
}

void
change_win_cursor (GdkWindow     *win,
		   GdkCursorType  cursortype)
{
  GdkCursor *cursor;
  
  if (cursortype > GDK_LAST_CURSOR)
    {
      gimp_change_win_cursor (win, (GimpCursorType)cursortype);
      return;
    }
  cursor = gdk_cursor_new (cursortype);
  gdk_window_set_cursor (win, cursor);
  gdk_cursor_destroy (cursor);
}

void
unset_win_cursor (GdkWindow *win)
{
  gdk_window_set_cursor (win, NULL);
}
     
void
gimp_add_busy_cursors_until_idle (void)
{
  if (!pending_removebusy)
    {
      gimp_add_busy_cursors ();
      gtk_idle_add_priority (GTK_PRIORITY_HIGH,
			     gimp_remove_busy_cursors, NULL);
      pending_removebusy = TRUE;
    }
}
     
void
gimp_add_busy_cursors (void)
{
  GDisplay *gdisp;
  GSList   *list;

  /* Canvases */
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_install_override_cursor (gdisp, GDK_WATCH);
    }

  /* Dialogs */
  dialog_idle_all ();

  gdk_flush ();
}

gint
gimp_remove_busy_cursors (gpointer data)
{
  GDisplay *gdisp;
  GSList   *list;

  /* Canvases */
  for (list = display_list; list; list = g_slist_next (list))
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_remove_override_cursor (gdisp);
    }

  /* Dialogs */
  dialog_unidle_all ();

  pending_removebusy = FALSE;

  return 0;
}


/***************************************************************/
/* gtkutil_compress_motion:

   This function walks the whole GDK event queue seeking motion events
   corresponding to the widget 'widget'.  If it finds any it will
   remove them from the queue, write the most recent motion offset
   to 'lastmotion_x' and 'lastmotion_y', then return TRUE.  Otherwise
   it will return FALSE and 'lastmotion_x' / 'lastmotion_y' will be
   untouched.
 */
/* The gtkutil_compress_motion function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
gboolean
gtkutil_compress_motion (GtkWidget *widget,
			 gdouble   *lastmotion_x,
			 gdouble   *lastmotion_y)
{
  GdkEvent *event;
  GList *requeued_events = NULL;
  gboolean success = FALSE;

  /* Move the entire GDK event queue to a private list, filtering
     out any motion events for the desired widget. */
  while (gdk_events_pending ())
    {
      event = gdk_event_get ();

      if ((gtk_get_event_widget (event) == widget) &&
	  (event->any.type == GDK_MOTION_NOTIFY))
	{
	  *lastmotion_x = event->motion.x;
	  *lastmotion_y = event->motion.y;
	  
	  gdk_event_free (event);
	  success = TRUE;
	}
      else
	{
	  requeued_events = g_list_append (requeued_events, event);
	}
    }
  
  /* Replay the remains of our private event list back into the
     event queue in order. */
  while (requeued_events)
    {
      gdk_event_put ((GdkEvent*) requeued_events->data);

      gdk_event_free ((GdkEvent*) requeued_events->data);      
      requeued_events =
	g_list_remove_link (requeued_events, requeued_events);
    }
  
  return success;
}
