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
#include "../cursors/mouse1"
#include "../cursors/mouse1msk"
#include "../cursors/mouse1_p"
#include "../cursors/mouse1_pmsk"
#include "../cursors/mouse1_m"
#include "../cursors/mouse1_mmsk"
#include "../cursors/mouse1_u"
#include "../cursors/mouse1_umsk"
#include "../cursors/dropper"
#include "../cursors/droppermsk"
#include "../cursors/mouse1_ap"
#include "../cursors/mouse1_apmsk"
#include "../cursors/mouse1_cp"
#include "../cursors/mouse1_cpmsk"
#include "../cursors/mouse1_mm"
#include "../cursors/mouse1_mmmsk"
#include "../cursors/mouse1_selm"
#include "../cursors/mouse1_selmmsk"
#include "../cursors/mouse1_selp"
#include "../cursors/mouse1_selpmsk"
#include "../cursors/mouse1_selu"
#include "../cursors/mouse1_selumsk"
#include "../cursors/mouse1_sel"
#include "../cursors/mouse1_selmsk"
#include "../cursors/bad"
#include "../cursors/badmsk"

typedef struct
{
  unsigned char *bits;
  unsigned char *mask_bits;
  int width, height;
  int x_hot, y_hot;
  GdkCursor *cursor;
} BM_Cursor;
    
static BM_Cursor gimp_cursors[] =
/* these have to match up with the enum in cursorutil.h */
{
  { mouse1_bits, mouse1msk_bits, mouse1_width, mouse1_height,
    mouse1_x_hot, mouse1_y_hot, NULL},
  { mouse1_p_bits, mouse1_pmsk_bits, mouse1_p_width, mouse1_p_height,
    mouse1_p_x_hot, mouse1_p_y_hot, NULL},
  { mouse1_m_bits, mouse1_mmsk_bits, mouse1_m_width, mouse1_m_height,
    mouse1_m_x_hot, mouse1_m_y_hot, NULL},
  { mouse1_u_bits, mouse1_umsk_bits, mouse1_u_width, mouse1_u_height,
    mouse1_u_x_hot, mouse1_u_y_hot, NULL},
  { dropper_bits, droppermsk_bits, dropper_width, dropper_height,
    dropper_x_hot, dropper_y_hot, NULL},
  { mouse1_ap_bits, mouse1_apmsk_bits, mouse1_ap_width, mouse1_ap_height,
    mouse1_ap_x_hot, mouse1_ap_y_hot, NULL},
  { mouse1_cp_bits, mouse1_cpmsk_bits, mouse1_cp_width, mouse1_cp_height,
    mouse1_cp_x_hot, mouse1_cp_y_hot, NULL},
  { mouse1_mm_bits, mouse1_mmmsk_bits, mouse1_mm_width, mouse1_mm_height,
    mouse1_mm_x_hot, mouse1_mm_y_hot, NULL},
  { mouse1_selp_bits, mouse1_selpmsk_bits, mouse1_selp_width, mouse1_selp_height,
    mouse1_selp_x_hot, mouse1_selp_y_hot, NULL},
  { mouse1_selm_bits, mouse1_selmmsk_bits, mouse1_selm_width, mouse1_selm_height,
    mouse1_selm_x_hot, mouse1_selm_y_hot, NULL},
  { mouse1_selu_bits, mouse1_selumsk_bits, mouse1_selu_width, mouse1_selu_height,
    mouse1_selu_x_hot, mouse1_selu_y_hot, NULL},
  { mouse1_sel_bits, mouse1_selmsk_bits, mouse1_sel_width, mouse1_sel_height,
    mouse1_sel_x_hot, mouse1_sel_y_hot, NULL},
  { bad_bits, badmsk_bits, bad_width, bad_height,
    bad_x_hot, bad_y_hot, NULL},
};



extern GSList* display_list; /* It's in gdisplay.c, FYI */
static gboolean pending_removebusy = FALSE;


static void
create_cursor (BM_Cursor *bmcursor)
{
  GdkPixmap *pixmap;
  GdkPixmap *pixmapmsk;
  GdkColor fg, bg;

  /* should have a way to configure the mouse colors */
  gdk_color_parse("#FFFFFF", &bg);
  gdk_color_parse("#000000", &fg);

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
gimp_change_win_cursor (GdkWindow *win, GimpCursorType curtype)
{
  GdkCursor *cursor;

  g_return_if_fail (curtype < GIMP_LAST_CURSOR_ENTRY);
  curtype -= GIMP_MOUSE1_CURSOR;
  if (!gimp_cursors[(int)curtype].cursor)
    create_cursor (&gimp_cursors[(int)curtype]);
  cursor = gimp_cursors[(int)curtype].cursor;
  gdk_window_set_cursor (win, cursor);
}


void
change_win_cursor (win, cursortype)
     GdkWindow *win;
     GdkCursorType cursortype;
{
  GdkCursor *cursor;
  
  if (cursortype > GDK_LAST_CURSOR)
  {
    gimp_change_win_cursor(win, (GimpCursorType)cursortype);
    return;
  }
  cursor = gdk_cursor_new (cursortype);
  gdk_window_set_cursor (win, cursor);
  gdk_cursor_destroy (cursor);
}

void
unset_win_cursor (win)
     GdkWindow *win;
{
  gdk_window_set_cursor (win, NULL);
}
     
void
gimp_add_busy_cursors_until_idle (void)
{
  if (!pending_removebusy)
    {
      gimp_add_busy_cursors(); 
      gtk_idle_add_priority (GTK_PRIORITY_HIGH,
			     gimp_remove_busy_cursors, NULL);
      pending_removebusy = TRUE;
    }
}
     
void
gimp_add_busy_cursors (void)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /* Canvases */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_install_override_cursor (gdisp, GDK_WATCH);

      list = g_slist_next (list);
    }

  /* Dialogs */
  dialog_idle_all();

  gdk_flush();
}

int
gimp_remove_busy_cursors (gpointer data)
{
  GDisplay *gdisp;
  GSList *list = display_list;

  /* Canvases */
  while (list)
    {
      gdisp = (GDisplay *) list->data;
      gdisplay_remove_override_cursor (gdisp);

      list = g_slist_next (list);
    }

  /* Dialogs */
  dialog_unidle_all();

  pending_removebusy = FALSE;

  return 0;
}




