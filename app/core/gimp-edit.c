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
#include <stdlib.h>
#include "appenv.h"
#include "actionarea.h"
#include "canvas.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "general.h"
#include "global_edit.h"
#include "interface.h"
#include "layer.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "tools.h"
#include "undo.h"

#include "tile_manager_pvt.h"
#include "drawable_pvt.h"


/*  The named paste dialog  */
typedef struct _PasteNamedDlg PasteNamedDlg;
struct _PasteNamedDlg
{
  GtkWidget   *shell;
  GtkWidget   *list;
  int          paste_into;
  GDisplay    *gdisp;
};

/*  The named buffer structure...  */
typedef struct _named_buffer NamedBuffer;

struct _named_buffer
{
  TileManager * buf;
  Canvas *buf_canvas;
  char *        name;
};


/*  The named buffer list  */
GSList * named_buffers = NULL;

/*  The global edit buffer  */
TileManager * global_buf = NULL;
Canvas *global_buf_canvas = NULL;

/*  Crop the buffer to the size of pixels with non-zero transparency */

Canvas *
crop_buffer_16 (Canvas *canvas,
	     int          border)
{
  PixelArea PR;
  Canvas *new_canvas;
  int num_channels, alpha;
  guchar * data;
  int empty;
  int x1, y1, x2, y2;
  int x, y;
  int ex, ey;
  int found;
  void * pr;
  COLOR16_NEW (black_color, canvas_tag(canvas) );
  COLOR16_INIT (black_color);
  color16_black (&black_color);

  num_channels = tag_num_channels (canvas_tag (canvas));
  alpha = num_channels - 1;

  /*  go through and calculate the bounds  */
  x1 = canvas_width (canvas);
  y1 = canvas_height (canvas);
  x2 = 0;
  y2 = 0;

  pixelarea_init (&PR, canvas, NULL, 
	0, 0, x1, y1, FALSE);
  for (pr = pixelarea_register (1, &PR); 
	pr != NULL; 
	pr = pixelarea_process (pr))
    {
      data = pixelarea_data (&PR) + alpha;
      ex = pixelarea_x (&PR) + pixelarea_width (&PR);
      ey = pixelarea_y (&PR) + pixelarea_height (&PR);

      for (y = pixelarea_y (&PR); y < ey; y++)
	{
	  found = FALSE;
	  for (x = pixelarea_x (&PR); x < ex; x++, data+=num_channels)
	    if (*data)
	      {
		if (x < x1)
		  x1 = x;
		if (x > x2)
		  x2 = x;
		found = TRUE;
	      }
	  if (found)
	    {
	      if (y < y1)
		y1 = y;
	      if (y > y2)
		y2 = y;
	    }
	}
    }

  x2 = BOUNDS (x2 + 1, 0, canvas_width (canvas));
  y2 = BOUNDS (y2 + 1, 0, canvas_height (canvas));

  empty = (x1 == canvas_width (canvas) && y1 == canvas_height (canvas));

  /*  If there are no visible pixels, return NULL */
  if (empty)
    new_canvas = NULL;
  /*  If no cropping, return original buffer  */
  else if (x1 == 0 && y1 == 0 && x2 == canvas_width (canvas) &&
	   y2 == canvas_height (canvas) && border == 0)
    new_canvas = canvas;
  /*  Otherwise, crop the original area  */
  else
    {
      PixelArea srcPR, destPR;
      int new_width, new_height;

      new_width = (x2 - x1) + border * 2;
      new_height = (y2 - y1) + border * 2;
      new_canvas = canvas_new (canvas_tag (canvas), new_width, new_height, STORAGE_FLAT);

      /*  If there is a border, make sure to clear the new tiles first  */
      if (border)
	{
	  pixelarea_init (&destPR, new_canvas, NULL, 
		0, 0, new_width, border, TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_canvas, NULL, 
		0, border, border, (y2 - y1), TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_canvas, NULL,
		new_width - border, border, border, (y2 - y1), TRUE);
	  color_area (&destPR, &black_color);
	  pixelarea_init (&destPR, new_canvas, NULL,
		0, new_height - border, new_width, border, TRUE);
	  color_area (&destPR, &black_color);
	}

      pixelarea_init (&srcPR, canvas, NULL, 
		x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixelarea_init (&destPR, new_canvas, NULL, 
		border, border, (x2 - x1), (y2 - y1), TRUE);

      copy_area (&srcPR, &destPR);
/*
	we dont have a way of setting this in canvas....
      canvas->x = x1;
      canvas->y = y1;
*/
    }

  return new_canvas;
}

Canvas *
edit_cut_16 (GImage *gimage,
	  GimpDrawable *drawable)
{
  Canvas *cut;
  Canvas *cropped_cut;
  int empty;

  if (!gimage || drawable == NULL)
    return NULL;

  /*  Start a group undo  */
  undo_push_group_start (gimage, EDIT_CUT_UNDO);

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  Next, cut the mask portion from the gimage  */
  cut = gimage_mask_extract (gimage, drawable, TRUE, FALSE);

  /*  Only crop if the gimage mask wasn't empty  */
  if (cut && empty == FALSE)
    {
      cropped_cut = crop_buffer_16 (cut, 0);

      if (cropped_cut != cut)
	canvas_delete (cut);
    }
  else if (cut)
    cropped_cut = cut;
  else
    cropped_cut = NULL;

  /*  end the group undo  */
  undo_push_group_end (gimage);

  if (cropped_cut)
    {
      /*  Free the old global edit buffer  */
      if (global_buf_canvas)
	canvas_delete(global_buf_canvas);
      /*  Set the global edit buffer  */
      global_buf_canvas = cropped_cut;

      return cropped_cut;
    }
  else
    return NULL;
}

Canvas *
edit_copy_16 (GImage *gimage,
	   GimpDrawable *drawable)
{
  Canvas * copy;
  Canvas * cropped_copy;
  int empty;

  if (!gimage || drawable == NULL)
    return NULL;

  /*  See if the gimage mask is empty  */
  empty = gimage_mask_is_empty (gimage);

  /*  First, copy the masked portion of the gimage  */
  copy = gimage_mask_extract (gimage, drawable, FALSE, FALSE);

  /*  Only crop if the gimage mask wasn't empty  */
  if (copy && empty == FALSE)
    {
      cropped_copy = crop_buffer_16 (copy, 0);

      if (cropped_copy != copy)
        canvas_delete (copy);
    }
  else if (copy)
    cropped_copy = copy;
  else
    cropped_copy = NULL;

  if (cropped_copy)
    {
      /*  Free the old global edit buffer  */
      if (global_buf_canvas)
	canvas_delete (global_buf_canvas);
      /*  Set the global edit buffer  */
      global_buf_canvas = cropped_copy;

      return cropped_copy;
    }
  else
    return NULL;
}

int
edit_paste_16 (GImage      *gimage,
	    GimpDrawable *drawable,
	    Canvas       *paste,
	    int          paste_into)
{
  Layer * float_layer;
  int x1, y1, x2, y2;
  int cx, cy;

  /*  Make a new floating layer  */
  float_layer = layer_from_canvas (gimage, drawable, paste, "Pasted Layer", OPAQUE_OPACITY, NORMAL);

  if (float_layer)
    {
      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      /*  Set the offsets to the center of the image  */
      drawable_offsets ( (drawable), &cx, &cy);
      drawable_mask_bounds ( (drawable), &x1, &y1, &x2, &y2);
      cx += (x1 + x2) >> 1;
      cy += (y1 + y2) >> 1;

      GIMP_DRAWABLE(float_layer)->offset_x = cx - (GIMP_DRAWABLE(float_layer)->width >> 1);
      GIMP_DRAWABLE(float_layer)->offset_y = cy - (GIMP_DRAWABLE(float_layer)->height >> 1);

      /*  If there is a selection mask clear it--
       *  this might not always be desired, but in general,
       *  it seems like the correct behavior.
       */
      if (! gimage_mask_is_empty (gimage) && !paste_into)
	channel_clear (gimage_get_mask (gimage));

      /*  add a new floating selection  */
      floating_sel_attach (float_layer, drawable);

      /*  end the group undo  */
      undo_push_group_end (gimage);

      return GIMP_DRAWABLE(float_layer)->ID;
    }
  else
    return 0;
}

int
edit_clear (GImage *gimage,
	    GimpDrawable *drawable)
{
  Canvas *buf_canvas;
  PixelArea bufPR;
  int x1, y1, x2, y2;
  Tag d_tag = drawable_tag (drawable);
  COLOR16_NEW (background_color, d_tag);
  COLOR16_INIT (background_color);
  color16_background (&background_color);

  if (!gimage || drawable == NULL)
    return FALSE;

/* This should happen automatically with color_area
  if (drawable_has_alpha (drawable))
    col [drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;
*/
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return FALSE;

  buf_canvas = canvas_new (d_tag, (x2 - x1), (y2 - y1), STORAGE_FLAT);
  pixelarea_init (&bufPR, buf_canvas, NULL, 
	0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_area (&bufPR, &background_color);

  pixelarea_init (&bufPR, buf_canvas, NULL, 
	0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_painthit(gimage, drawable, NULL, 
			&bufPR, 1, 1.0, 
			ERASE_MODE, x1, y1);

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary canvas  */
  canvas_delete (buf_canvas);

  return TRUE;
}

int
edit_fill (GImage *gimage,
	   GimpDrawable *drawable)
{
  Canvas *buf_canvas;
  PixelArea bufPR;
  int x1, y1, x2, y2;
  Tag d_tag = drawable_tag (drawable);
  COLOR16_NEW (background_color, d_tag);
  COLOR16_INIT (background_color);
  color16_black (&background_color);

  if (!gimage || drawable == NULL)
    return FALSE;

/* this should happen automatically if drawable has alpha
  if (tag_alpha (d_tag) == ALPHA_YES))
    col [drawable_bytes (drawable) - 1] = OPAQUE_OPACITY;
*/
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if (!(x2 - x1) || !(y2 - y1))
    return FALSE;

  buf_canvas = canvas_new (d_tag, (x2 - x1), (y2 - y1), STORAGE_FLAT);
  pixelarea_init (&bufPR, buf_canvas, NULL, 
	0, 0, (x2 - x1), (y2 - y1), TRUE);
  color_area (&bufPR, &background_color);

  pixelarea_init (&bufPR, buf_canvas, NULL, 
	0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_painthit (gimage, drawable, NULL, &bufPR, 1, 
			1.0, NORMAL_MODE, x1, y1); 

  /*  update the image  */
  drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary canvas  */
  canvas_delete (buf_canvas);

  return TRUE;
}

int
global_edit_cut (void *gdisp_ptr)
{
  GDisplay *gdisp;

  /*  stop any active tool  */
  gdisp = (GDisplay *) gdisp_ptr;
  active_tool_control (HALT, gdisp_ptr);

  if (!edit_cut_16 (gdisp->gimage, gimage_active_drawable (gdisp->gimage)))
    return FALSE;
  else
    {
      /*  flush the display  */
      gdisplays_flush ();
      return TRUE;
    }
}

int
global_edit_copy (void *gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;

  if (!edit_copy_16 (gdisp->gimage, gimage_active_drawable (gdisp->gimage)))
    return FALSE;
  else
    return TRUE;
}

int
global_edit_paste (void *gdisp_ptr,
		   int   paste_into)
{
  GDisplay *gdisp = (GDisplay *) gdisp_ptr;
  GimpDrawable *d = gimage_active_drawable (gdisp->gimage);

  /*  stop any active tool  */
  active_tool_control (HALT, gdisp_ptr);

  if (!edit_paste_16 (gdisp->gimage, 
		d, 
		global_buf_canvas, paste_into))
    return FALSE;
  else
    {
      /*  flush the display  */
      gdisplays_flush ();
      return TRUE;
    }
}

void
global_edit_free ()
{
  if (global_buf_canvas)
     canvas_delete (global_buf_canvas);
  global_buf_canvas = NULL;
}

/*********************************************/
/*        Named buffer operations            */

static void
set_list_of_named_buffers (GtkWidget *list_widget)
{
  GSList *list;
  NamedBuffer *nb;
  GtkWidget *list_item;

  gtk_list_clear_items (GTK_LIST (list_widget), 0, -1);
  list = named_buffers;

  while (list)
    {
      nb = (NamedBuffer *) list->data;
      list = g_slist_next (list);

      list_item = gtk_list_item_new_with_label (nb->name);
      gtk_container_add (GTK_CONTAINER (list_widget), list_item);
      gtk_widget_show (list_item);
      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) nb);
    }
}

static void
named_buffer_paste_foreach (GtkWidget *w,
			    gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  NamedBuffer *nb;

  if (w->state == GTK_STATE_SELECTED)
    {
      pn_dlg = (PasteNamedDlg *) client_data;
      nb = (NamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));
      edit_paste_16 (pn_dlg->gdisp->gimage,
		  gimage_active_drawable (pn_dlg->gdisp->gimage),
		  nb->buf_canvas, pn_dlg->paste_into);
    }
}

static void
named_buffer_paste_callback (GtkWidget *w,
			     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 named_buffer_paste_foreach, client_data);

  /*  Destroy the box  */
  gtk_widget_destroy (pn_dlg->shell);

  g_free (pn_dlg);
      
  /*  flush the display  */
  gdisplays_flush ();
}

static void
named_buffer_delete_foreach (GtkWidget *w,
			     gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;
  NamedBuffer * nb;

  if (w->state == GTK_STATE_SELECTED)
    {
      pn_dlg = (PasteNamedDlg *) client_data;
      nb = (NamedBuffer *) gtk_object_get_user_data (GTK_OBJECT (w));
      named_buffers = g_slist_remove (named_buffers, (void *) nb);
      g_free (nb->name);
      canvas_delete (nb->buf_canvas);
      g_free (nb);
    }
}

static void
named_buffer_delete_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;
  gtk_container_foreach ((GtkContainer*) pn_dlg->list,
			 named_buffer_delete_foreach, client_data);
  set_list_of_named_buffers (pn_dlg->list);
}

static void
named_buffer_cancel_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  /*  Destroy the box  */
  gtk_widget_destroy (pn_dlg->shell);

  g_free (pn_dlg);
}

static gint
named_buffer_dialog_delete_callback (GtkWidget *w,
				     GdkEvent  *e,
				     gpointer   client_data)
{
  named_buffer_cancel_callback (w, client_data);

  return TRUE;
}

static void
named_buffer_paste_into_update (GtkWidget *w,
				gpointer   client_data)
{
  PasteNamedDlg *pn_dlg;

  pn_dlg = (PasteNamedDlg *) client_data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    pn_dlg->paste_into = FALSE;
  else
    pn_dlg->paste_into = TRUE;
}

static void
paste_named_buffer (GDisplay *gdisp)
{
  static ActionAreaItem action_items[3] =
  {
    { "Paste", named_buffer_paste_callback, NULL, NULL },
    { "Delete", named_buffer_delete_callback, NULL, NULL },
    { "Cancel", named_buffer_cancel_callback, NULL, NULL }
  };
  PasteNamedDlg *pn_dlg;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *paste_into;
  GtkWidget *listbox;

  pn_dlg = (PasteNamedDlg *) g_malloc (sizeof (PasteNamedDlg));
  pn_dlg->gdisp = gdisp;

  pn_dlg->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (pn_dlg->shell), "paste_named_buffer", "Gimp");
  gtk_window_set_title (GTK_WINDOW (pn_dlg->shell), "Paste Named Buffer");
  gtk_window_position (GTK_WINDOW (pn_dlg->shell), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (pn_dlg->shell), "delete_event",
		      GTK_SIGNAL_FUNC (named_buffer_dialog_delete_callback),
		      pn_dlg);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pn_dlg->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new ("Select a buffer to paste:");
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 150);
  gtk_widget_show (listbox);

  pn_dlg->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (pn_dlg->list), GTK_SELECTION_BROWSE);
  gtk_container_add (GTK_CONTAINER (listbox), pn_dlg->list);
  set_list_of_named_buffers (pn_dlg->list);
  gtk_widget_show (pn_dlg->list);

  paste_into = gtk_check_button_new_with_label ("Replace Current Selection");
  gtk_box_pack_start (GTK_BOX (vbox), paste_into, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (paste_into), "toggled",
		      (GtkSignalFunc) named_buffer_paste_into_update,
		      pn_dlg);
  gtk_widget_show (paste_into);

  action_items[0].user_data = pn_dlg;
  action_items[1].user_data = pn_dlg;
  action_items[2].user_data = pn_dlg;
  build_action_area (GTK_DIALOG (pn_dlg->shell), action_items, 3, 0);

  gtk_widget_show (pn_dlg->shell);
}

static void
new_named_buffer_callback (GtkWidget *w,
			   gpointer   client_data,
			   gpointer   call_data)
{
  PixelArea srcPR, destPR;
  Canvas *canvas;
  NamedBuffer *nb;

  canvas = (Canvas *) client_data;
  nb = (NamedBuffer *) g_malloc (sizeof (NamedBuffer));

  nb->buf_canvas = canvas_new (canvas_tag (canvas), 
			canvas_width (canvas), canvas_height (canvas), 
			STORAGE_FLAT);
  pixelarea_init (&srcPR, canvas, NULL, 
		0, 0, canvas_width (canvas), canvas_height (canvas), FALSE);
  pixelarea_init (&destPR, nb->buf_canvas, NULL, 
		0, 0, canvas_width (canvas), canvas_height (canvas), TRUE);
  copy_area (&srcPR, &destPR);

  nb->name = g_strdup ((char *) call_data);
  named_buffers = g_slist_append (named_buffers, (void *) nb);
}

static void
new_named_buffer (Canvas *new_canvas)
{
  /*  Create the dialog box to ask for a name  */
  query_string_box ("Named Buffer", "Enter a name for this buffer", NULL,
		    new_named_buffer_callback, new_canvas);
}

int
named_edit_cut (void *gdisp_ptr)
{
  Canvas *new_canvas;
  GDisplay *gdisp;

  /*  stop any active tool  */
  gdisp = (GDisplay *) gdisp_ptr;
  active_tool_control (HALT, gdisp_ptr);

  new_canvas = edit_cut_16 (gdisp->gimage, 
	gimage_active_drawable (gdisp->gimage));

  if (! new_canvas)
    return FALSE;
  else
    {
      new_named_buffer (new_canvas);
      gdisplays_flush ();
      return TRUE;
    }
}

int
named_edit_copy (void *gdisp_ptr)
{
  Canvas *new_canvas;
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  new_canvas = edit_copy_16 (gdisp->gimage, 
	gimage_active_drawable (gdisp->gimage));

  if (! new_canvas)
    return FALSE;
  else
    {
      new_named_buffer (new_canvas);
      return TRUE;
    }
}

int
named_edit_paste (void *gdisp_ptr)
{
  paste_named_buffer ((GDisplay *) gdisp_ptr);

  gdisplays_flush();

  return TRUE;
}

void
named_buffers_free ()
{
  GSList *list;
  NamedBuffer * nb;

  list = named_buffers;

  while (list)
    {
      nb = (NamedBuffer *) list->data;
      canvas_delete (nb->buf_canvas);
      g_free (nb->name);
      g_free (nb);
      list = g_slist_next (list);
    }

  g_slist_free (named_buffers);
  named_buffers = NULL;
}
