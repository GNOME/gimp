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
#include "tools.h"

#include "cursors/mouse.xbm"
#include "cursors/mouse_mask.xbm"
#include "cursors/crosshair.xbm"
#include "cursors/crosshair_mask.xbm"
#include "cursors/crosshair_small.xbm"
#include "cursors/crosshair_small_mask.xbm"

/*  stuff to be reordered  */
#include "cursors/zoom.xbm"
#include "cursors/zoom_mask.xbm"

/*  old stuff  */
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
#include "cursors/selection_move.xbm"
#include "cursors/selection_move_mask.xbm"
#include "cursors/bad.xbm"
#include "cursors/bad_mask.xbm"
#include "cursors/dropper.xbm"
#include "cursors/dropper_mask.xbm"
#include "cursors/zoom_in.xbm"
#include "cursors/zoom_in_mask.xbm"
#include "cursors/zoom_out.xbm"
#include "cursors/zoom_out_mask.xbm"

/* modifiers */
#include "cursors/plus.xbm"
#include "cursors/plus_mask.xbm"
#include "cursors/minus.xbm"
#include "cursors/minus_mask.xbm"
#include "cursors/intersect.xbm"
#include "cursors/intersect_mask.xbm"
#include "cursors/move.xbm"
#include "cursors/move_mask.xbm"


/* FIXME: gimp_busy HACK */
gboolean gimp_busy = FALSE;

static BitmapCursor gimp_cursors[] =
/* these have to match up with the enum in cursorutil.h */
{
  {
    mouse_bits, mouse_mask_bits,
    mouse_width, mouse_height,
    mouse_x_hot, mouse_y_hot, NULL, NULL, NULL
  },
  {
    crosshair_bits, crosshair_mask_bits,
    crosshair_width, crosshair_height,
    crosshair_x_hot, crosshair_y_hot, NULL, NULL, NULL
  },
  {
    crosshair_small_bits, crosshair_small_mask_bits,
    crosshair_small_width, crosshair_small_height,
    crosshair_small_x_hot, crosshair_small_y_hot, NULL, NULL, NULL
  },

  /*  to be reordered  */
  {
    zoom_bits, zoom_mask_bits,
    zoom_width, zoom_height,
    zoom_x_hot, zoom_y_hot, NULL, NULL, NULL
  },

  /*  to be removed  */
  {
    mouse_add_bits, mouse_add_mask_bits,
    mouse_add_width, mouse_add_height,
    mouse_add_x_hot, mouse_add_y_hot, NULL, NULL, NULL
  },
  {
    mouse_subtract_bits, mouse_subtract_mask_bits,
    mouse_subtract_width, mouse_subtract_height,
    mouse_subtract_x_hot, mouse_subtract_y_hot, NULL, NULL, NULL
  },
  {
    mouse_intersect_bits, mouse_intersect_mask_bits,
    mouse_intersect_width, mouse_intersect_height,
    mouse_intersect_x_hot, mouse_intersect_y_hot, NULL, NULL, NULL
  },
  {
    mouse_point_bits, mouse_point_mask_bits,
    mouse_point_width, mouse_point_height,
    mouse_point_x_hot, mouse_point_y_hot, NULL, NULL, NULL
  },
  {
    mouse_rectangle_bits, mouse_rectangle_mask_bits,
    mouse_rectangle_width, mouse_rectangle_height,
    mouse_rectangle_x_hot, mouse_rectangle_y_hot, NULL, NULL, NULL
  },
  {
    mouse_move_bits, mouse_move_mask_bits,
    mouse_move_width, mouse_move_height,
    mouse_move_x_hot, mouse_move_y_hot, NULL, NULL, NULL
  },
  {
    selection_bits, selection_mask_bits,
    selection_width, selection_height,
    selection_x_hot, selection_y_hot, NULL, NULL, NULL
  },
  {
    selection_add_bits, selection_add_mask_bits,
    selection_add_width, selection_add_height,
    selection_add_x_hot, selection_add_y_hot, NULL, NULL, NULL
  },
  {
    selection_subtract_bits, selection_subtract_mask_bits,
    selection_subtract_width, selection_subtract_height,
    selection_subtract_x_hot, selection_subtract_y_hot, NULL, NULL, NULL
  },
  {
    selection_intersect_bits, selection_intersect_mask_bits,
    selection_intersect_width, selection_intersect_height,
    selection_intersect_x_hot, selection_intersect_y_hot, NULL, NULL, NULL
  },
  {
    selection_move_bits, selection_move_mask_bits,
    selection_move_width, selection_move_height,
    selection_move_x_hot, selection_move_y_hot, NULL, NULL, NULL
  },
  {
    bad_bits, bad_mask_bits,
    bad_width, bad_height,
    bad_x_hot, bad_y_hot, NULL, NULL, NULL
  },
  {
    dropper_bits, dropper_mask_bits,
    dropper_width, dropper_height,
    dropper_x_hot, dropper_y_hot, NULL, NULL, NULL
  },
  {
    zoom_in_bits, zoom_in_mask_bits,
    zoom_in_width, zoom_in_height,
    zoom_in_x_hot, zoom_in_y_hot, NULL, NULL, NULL
  },
  {
    zoom_out_bits, zoom_out_mask_bits,
    zoom_out_width, zoom_out_height,
    zoom_out_x_hot, zoom_out_y_hot, NULL, NULL, NULL
  }
};

enum
{
  GIMP_PLUS_CURSOR = GIMP_LAST_CURSOR_ENTRY + 1,
  GIMP_MINUS_CURSOR,
  GIMP_INTERSECT_CURSOR,
  GIMP_MOVE_CURSOR
};

static BitmapCursor modifier_cursors[] =
/* these have to match up with the enum above */
{
  {
    plus_bits, plus_mask_bits,
    plus_width, plus_height,
    plus_x_hot, plus_y_hot, NULL, NULL, NULL
  },
  {
    minus_bits, minus_mask_bits,
    minus_width, minus_height,
    minus_x_hot, minus_y_hot, NULL, NULL, NULL
  },
  {
    intersect_bits, intersect_mask_bits,
    intersect_width, intersect_height,
    intersect_x_hot, intersect_y_hot, NULL, NULL, NULL
  },
  {
    move_bits, move_mask_bits,
    move_width, move_height,
    move_x_hot, move_y_hot, NULL, NULL, NULL
  }
};


extern GSList   *display_list; /* It's in gdisplay.c, FYI */
static gboolean  pending_removebusy = FALSE;


static void
create_cursor_bitmaps (BitmapCursor *bmcursor)
{
  if (bmcursor->bitmap == NULL)
    bmcursor->bitmap = gdk_bitmap_create_from_data (NULL, bmcursor->bits,
						    bmcursor->width,
						    bmcursor->height);
  g_return_if_fail (bmcursor->bitmap != NULL);

  if (bmcursor->mask == NULL)
    bmcursor->mask = gdk_bitmap_create_from_data (NULL, bmcursor->mask_bits,
						  bmcursor->width,
						  bmcursor->height);
  g_return_if_fail (bmcursor->mask != NULL);
}

static void
create_cursor (BitmapCursor *bmcursor)
{
  if (bmcursor->bitmap == NULL ||
      bmcursor->mask == NULL)
    create_cursor_bitmaps (bmcursor);

  if (bmcursor->cursor == NULL)
    {
      GdkColor fg, bg;

      /* should have a way to configure the mouse colors */
      gdk_color_parse ("#FFFFFF", &bg);
      gdk_color_parse ("#000000", &fg);

      bmcursor->cursor = gdk_cursor_new_from_pixmap (bmcursor->bitmap,
						     bmcursor->mask,
						     &fg, &bg,
						     bmcursor->x_hot,
						     bmcursor->y_hot);
    }

  g_return_if_fail (bmcursor->cursor != NULL);
}

static void
gimp_change_win_cursor (GdkWindow      *win,
			GimpCursorType  curtype,
			ToolType        tool_type,
			CursorModifier  modifier,
			gboolean        toggle_cursor)
{
  GdkCursor      *cursor;
  GimpCursorType  modtype = GIMP_PLUS_CURSOR;

  GdkBitmap *bitmap;
  GdkBitmap *mask;

  GdkColor   color;
  GdkColor   fg, bg;

  static GdkGC *gc = NULL;

  gint width;
  gint height;

  BitmapCursor *bmcursor   = NULL;
  BitmapCursor *bmmodifier = NULL;
  BitmapCursor *bmtool     = NULL;

  g_return_if_fail (curtype < GIMP_LAST_CURSOR_ENTRY);

  /*  allow the small tool cursor only with the standard mouse and
   *  the small crosshair
   */
  if (curtype != GIMP_MOUSE_CURSOR &&
      curtype != GIMP_CROSSHAIR_SMALL_CURSOR)
    tool_type = TOOL_TYPE_NONE;

  curtype -= GIMP_MOUSE_CURSOR;
  bmcursor = &gimp_cursors[(int)curtype];

  if (modifier  == CURSOR_MODIFIER_NONE &&
      tool_type == TOOL_TYPE_NONE)
    {
      if  (bmcursor->cursor == NULL)
	create_cursor (bmcursor);

      gdk_window_set_cursor (win, bmcursor->cursor);

      return;
    }

  switch (modifier)
    {
    case CURSOR_MODIFIER_PLUS:
      modtype = GIMP_PLUS_CURSOR;
      break;
    case CURSOR_MODIFIER_MINUS:
      modtype = GIMP_MINUS_CURSOR;
      break;
    case CURSOR_MODIFIER_INTERSECT:
      modtype = GIMP_INTERSECT_CURSOR;
      break;
    case CURSOR_MODIFIER_MOVE:
      modtype = GIMP_MOVE_CURSOR;
      break;
    default:
      break;
    }

  if (modifier != CURSOR_MODIFIER_NONE)
    {
      modtype -= GIMP_PLUS_CURSOR;
      bmmodifier = &modifier_cursors[(int)modtype];
    }

  if (tool_type != TOOL_TYPE_NONE)
    {
      if (toggle_cursor)
	{
	  if (tool_info[(gint) tool_type].toggle_cursor.bits != NULL &&
	      tool_info[(gint) tool_type].toggle_cursor.mask_bits != NULL)
	    bmtool = &tool_info[(gint) tool_type].toggle_cursor;
	}
      else
	{
	  if (tool_info[(gint) tool_type].tool_cursor.bits != NULL &&
	      tool_info[(gint) tool_type].tool_cursor.mask_bits != NULL)
	    bmtool = &tool_info[(gint) tool_type].tool_cursor;
	}
    }

  if (bmcursor->bitmap == NULL ||
      bmcursor->mask == NULL)
    create_cursor_bitmaps (bmcursor);

  if (bmmodifier->bitmap == NULL ||
      bmmodifier->mask == NULL)
    create_cursor_bitmaps (bmmodifier);

  if (bmtool &&
      (bmtool->bitmap == NULL ||
       bmtool->mask == NULL))
    create_cursor_bitmaps (bmtool);

  if (gc == NULL)
    gc = gdk_gc_new (bmcursor->bitmap);

  gdk_window_get_size (bmcursor->bitmap, &width, &height);

  bitmap = gdk_pixmap_new (NULL, width, height, 1);
  mask   = gdk_pixmap_new (NULL, width, height, 1);

  color.pixel = 1;
  gdk_gc_set_foreground (gc, &color);

  gdk_draw_pixmap (bitmap, gc, bmcursor->bitmap,
                   0, 0, 0, 0, width, height);

  if (bmmodifier)
    {
      gdk_gc_set_clip_mask (gc, bmmodifier->bitmap);
      gdk_draw_pixmap (bitmap, gc, bmmodifier->bitmap,
		       0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  if (bmtool)
    {
      gdk_gc_set_clip_mask (gc, bmtool->bitmap);
      gdk_draw_pixmap (bitmap, gc, bmtool->bitmap,
		       0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  gdk_draw_pixmap (mask, gc, bmcursor->mask,
                   0, 0, 0, 0, width, height);

  if (bmmodifier)
    {
      gdk_gc_set_clip_mask (gc, bmmodifier->mask);
      gdk_draw_pixmap (mask, gc, bmmodifier->mask,
		       0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  if (bmtool)
    {
      gdk_gc_set_clip_mask (gc, bmtool->mask);
      gdk_draw_pixmap (mask, gc, bmtool->mask,
		       0, 0, 0, 0, width, height);
      gdk_gc_set_clip_mask (gc, NULL);
    }

  /* should have a way to configure the mouse colors */
  gdk_color_parse ("#FFFFFF", &bg);
  gdk_color_parse ("#000000", &fg);

  cursor = gdk_cursor_new_from_pixmap (bitmap, mask,
				       &fg, &bg,
				       bmcursor->x_hot,
				       bmcursor->y_hot);
  gdk_window_set_cursor (win, cursor);
  gdk_cursor_destroy (cursor);
}

void
change_win_cursor (GdkWindow      *win,
		   GdkCursorType   cursortype,
		   ToolType        tool_type,
		   CursorModifier  modifier,
		   gboolean        toggle_cursor)
{
  GdkCursor *cursor;
  
  if (cursortype > GDK_LAST_CURSOR)
    {
      gimp_change_win_cursor (win, (GimpCursorType) cursortype,
			      tool_type, modifier, toggle_cursor);
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

  /* FIXME: gimp_busy HACK */
  gimp_busy = TRUE;

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

  /* FIXME: gimp_busy HACK */
  gimp_busy = FALSE;

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

      if (!event)
	{
	  /* Do nothing */
	}
      else if ((gtk_get_event_widget (event) == widget) &&
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
