/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 */

#include "libgimp/gimp.h"

#include "imap_cmd_edit_object.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_popup.h"
#include "imap_preview.h"

#define PREVIEW_MASK   GDK_EXPOSURE_MASK | \
                       GDK_MOTION_NOTIFY | \
		       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK | \
		       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON_MOTION_MASK | \
		       GDK_KEY_PRESS_MASK | \
		       GDK_KEY_RELEASE_MASK | \
		       GDK_ENTER_NOTIFY_MASK | \
		       GDK_LEAVE_NOTIFY_MASK

#define PREVIEW_SIZE 400

/*======================================================================
		Preview Rendering Util routine
=======================================================================*/

#define CHECKWIDTH 4
#define LIGHTCHECK 192
#define DARKCHECK  128
#ifndef OPAQUE
#define OPAQUE	   255
#endif

static Preview_t*
preview_user_data(GtkWidget *preview)
{
   return (Preview_t*) gtk_object_get_user_data(GTK_OBJECT(preview));
}

gint
preview_get_width(GtkWidget *preview)
{
   return preview_user_data(preview)->width;
}

gint
preview_get_height(GtkWidget *preview)
{
   return preview_user_data(preview)->height;
}

static void
render_gray_image(GtkWidget *preview, GPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_row, *src, *dest;
   gint		 row, col;
   gint		 bpp, dwidth, dheight, pwidth, pheight;
   gint		 *src_col;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   if(GTK_PREVIEW(preview)->buffer) {
      pwidth  = GTK_PREVIEW(preview)->buffer_width;
      pheight = GTK_PREVIEW(preview)->buffer_height;
   } else {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
   }
   bpp = srcrgn->bpp;

   src_row = g_new(guchar, dwidth * bpp);
   dest_row = g_new(guchar, pwidth * 3);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);

      dest = dest_row;
      src = src_row;

      for (col = 0; col < pwidth; col++) {
	 src = &src_row[src_col[col]];
	 *dest++ = *src;
	 *dest++ = *src;
	 *dest++ = *src;
      }
      gtk_preview_draw_row(GTK_PREVIEW(preview), dest_row, 0, row, pwidth);
   }
   g_free(src_col);
   g_free(src_row);
   g_free(dest_row);

}

static void
render_indexed_image(GtkWidget *preview, GPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_row, *src, *dest;
   gint		 row, col;
   gint		 dwidth, dheight, pwidth, pheight;
   gint		 *src_col;
   gint		 bpp, alpha, has_alpha;
   guchar 	 *cmap, *colour;
   gint 	 ncols;
   gboolean	 gray = get_map_info()->show_gray;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   if(GTK_PREVIEW(preview)->buffer) {
      pwidth  = GTK_PREVIEW(preview)->buffer_width;
      pheight = GTK_PREVIEW(preview)->buffer_height;
   } else {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
   }
   bpp = srcrgn->bpp;
   alpha = bpp;
   has_alpha = gimp_drawable_has_alpha(srcrgn->drawable->id);
   if (has_alpha)
      alpha--;

   cmap = gimp_image_get_cmap(gimp_drawable_image_id(srcrgn->drawable->id),
			      &ncols);

   src_row = g_new(guchar, dwidth * bpp);
   dest_row = g_new(guchar, pwidth * 3);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);
      dest = dest_row;
      for (col = 0; col < pwidth; col++) {
	 src = &src_row[src_col[col]];
	 colour = cmap + 3 * (int)(*src);

	 if (gray) {
	    guchar avg = (299 * *colour++ + 587 * *colour++ +
			  114 * *colour++) / 1000;
	    *dest++ = avg;
	    *dest++ = avg;
	    *dest++ = avg;
	 } else {
	    *dest++ = *colour++;
	    *dest++ = *colour++;
	    *dest++ = *colour++;
	 }
      }
      gtk_preview_draw_row(GTK_PREVIEW(preview), dest_row, 0, row, pwidth);
   }
   g_free(src_col);
   g_free(src_row);
   g_free(dest_row);
}

static void
render_rgb_image(GtkWidget *preview, GPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_row, *src, *dest;
   gint		 row, col;
   gint		 dwidth, dheight, pwidth, pheight;
   gint		 *src_col;
   gint		 bpp, alpha, has_alpha, b;
   guchar	 check;
   gboolean	 gray = get_map_info()->show_gray;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   if(GTK_PREVIEW(preview)->buffer) {
      pwidth  = GTK_PREVIEW(preview)->buffer_width;
      pheight = GTK_PREVIEW(preview)->buffer_height;
   } else {
      pwidth  = preview->requisition.width;
      pheight = preview->requisition.height;
   }
   bpp = srcrgn->bpp;
   alpha = bpp;
   has_alpha = gimp_drawable_has_alpha(srcrgn->drawable->id);
   if (has_alpha)
      alpha--;

   src_row = g_new(guchar, dwidth * bpp);
   dest_row = g_new(guchar, pwidth * bpp);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);
      dest = dest_row;
      for (col = 0; col < pwidth; col++) {
	 src = &src_row[src_col[col]];
	 if(!has_alpha || src[alpha] == OPAQUE) {
	    /* no alpha channel or opaque -- simple way */
	    for (b = 0; b < alpha; b++)
	       dest[b] = src[b];
	 } else {
	    /* more or less transparent */
	    if( ( col % (CHECKWIDTH*2) < CHECKWIDTH ) ^
		( row % (CHECKWIDTH*2) < CHECKWIDTH ) )
	       check = LIGHTCHECK;
	    else
	       check = DARKCHECK;

	    if (src[alpha] == 0) {
	       /* full transparent -- check */
	       for (b = 0; b < alpha; b++)
		  dest[b] = check;
	    } else {
	       /* middlemost transparent -- mix check and src */
	       for (b = 0; b < alpha; b++)
		  dest[b] = (src[b] * src[alpha] +
			     check * (OPAQUE - src[alpha])) / OPAQUE;
	    }
	 }
	 if (gray) {
	    guchar avg;
	    avg = (299 * dest[0] + 587 * dest[1] + 114 * dest[2]) / 1000;
	    for (b = 0; b < alpha; b++)
	       dest[b] = avg;
	 }
	 dest += alpha;
      }
      gtk_preview_draw_row(GTK_PREVIEW(preview), dest_row, 0, row, pwidth);
   }
   g_free(src_col);
   g_free(src_row);
   g_free(dest_row);
}

static void
render_preview(GtkWidget *preview, GPixelRgn *srcrgn)
{
   switch (gimp_drawable_type(srcrgn->drawable->id)) {
   case RGB_IMAGE:
   case RGBA_IMAGE:
      render_rgb_image(preview, srcrgn);
      break;
   case GRAY_IMAGE:
   case GRAYA_IMAGE:
      render_gray_image(preview, srcrgn);
      break;
   case INDEXED_IMAGE:
   case INDEXEDA_IMAGE:
      render_indexed_image(preview, srcrgn);
      break;
   }
}

static gint
arrow_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	if (event->button == 1)
		do_main_popup_menu(event);
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event");
	return FALSE;
}

static gint
preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
   Preview_t *data = preview_user_data(widget);

   gtk_signal_handler_block(GTK_OBJECT(widget), data->exp_id);
   gtk_widget_draw(widget, (event) ? &event->area : NULL);
   gtk_signal_handler_unblock(GTK_OBJECT(widget), data->exp_id );

   draw_grid(widget);
   draw_shapes(widget);

   return FALSE;
}

void
add_preview_motion_event(Preview_t *preview, GtkSignalFunc func)
{
   gtk_signal_connect(GTK_OBJECT(preview->preview),
		      "motion_notify_event", func, NULL);
}

void
add_enter_notify_event(Preview_t *preview, GtkSignalFunc func)
{
   gtk_signal_connect(GTK_OBJECT(preview->preview),
		      "enter_notify_event", func, NULL);
}

void
add_leave_notify_event(Preview_t *preview, GtkSignalFunc func)
{
   gtk_signal_connect(GTK_OBJECT(preview->preview),
		      "leave_notify_event", func, NULL);
}

void
add_preview_button_press_event(Preview_t *preview, GtkSignalFunc func)
{
   gtk_signal_connect(GTK_OBJECT(preview->preview),
		      "button_press_event", func, NULL);
}

void
preview_redraw(Preview_t *preview)
{
   preview_expose(preview->preview, NULL);
}

void
preview_zoom(Preview_t *preview, gint zoom_factor)
{
   gint pwidth = preview->width * zoom_factor;
   gint pheight = preview->height * zoom_factor;

   gtk_preview_size(GTK_PREVIEW(preview->preview), pwidth, pheight);
   gtk_widget_queue_resize(preview->window);
   render_preview(preview->preview, &preview->src_rgn);
   preview_redraw(preview);
}

GdkCursorType
preview_set_cursor(Preview_t *preview, GdkCursorType cursor_type)
{
   GdkCursorType prev_cursor = preview->cursor;
   GdkCursor *cursor = gdk_cursor_new(cursor_type);
   gdk_window_set_cursor(preview->window->window, cursor);
   gdk_cursor_destroy(cursor);
   gdk_flush();
   preview->cursor = cursor_type;
   return prev_cursor;
}

static GtkTargetEntry target_table[] = {
   {"STRING", 0, 1 },
   {"text/plain", 0, 2 }
};

static void
handle_drop(GtkWidget *widget, GdkDragContext *context, gint x, gint y,
	    GtkSelectionData *data, guint info, guint time)
{
   gboolean success = FALSE;
   if (data->length >= 0 && data->format == 8) {
      ObjectList_t *list = get_shapes();
      Object_t *obj;

      x = get_real_coord(x);
      y = get_real_coord(y);
      obj = object_list_find(list, x, y);
      if (obj && !obj->locked) {
	 command_list_add(edit_object_command_new(obj));
	 object_set_url(obj, data->data);
	 object_emit_update_signal(obj);
	 success = TRUE;
      }
   }
   gtk_drag_finish(context, success, FALSE, time);
}

Preview_t*
make_preview(GDrawable *drawable)
{
   Preview_t *data = g_new(Preview_t, 1);
   GtkWidget *preview;
   GtkWidget *window;
   GtkWidget *button, *arrow;
   GtkWidget *ruler;
   GtkWidget *frame;
   GtkWidget *table;
   gint width, height;

   data->drawable = drawable;
   data->preview = preview = gtk_preview_new(GTK_PREVIEW_COLOR);

   gtk_object_set_user_data(GTK_OBJECT(preview), data);
   gtk_widget_set_events(GTK_WIDGET(preview), PREVIEW_MASK);
   data->exp_id = gtk_signal_connect_after(GTK_OBJECT(preview), "expose_event",
					   (GtkSignalFunc) preview_expose,
					   data);

   /* Handle drop of links in preview widget */
   gtk_drag_dest_set(preview, GTK_DEST_DEFAULT_ALL, target_table,
		     2, GDK_ACTION_COPY);
   gtk_signal_connect(GTK_OBJECT(preview), "drag_data_received",
		      GTK_SIGNAL_FUNC(handle_drop), NULL);

   data->width  = gimp_drawable_width(drawable->id);
   data->height = gimp_drawable_height(drawable->id);
   gtk_preview_size(GTK_PREVIEW(preview), data->width, data->height);

   data->window = window = gtk_scrolled_window_new(NULL, NULL);
   width = (data->width > 600) ? 600 : data->width;
   height = (data->height > 400) ? 400 : data->height;
   gtk_widget_set_usize(window, width, height);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_widget_show(window);

   data->frame = frame = gtk_frame_new(NULL);
   gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(window), frame);
   gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

   /* The main table */
   table = gtk_table_new(3, 3, FALSE);
   gtk_table_attach(GTK_TABLE(table), preview, 1, 2, 1, 2, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_container_add(GTK_CONTAINER(frame), table);

   /* Create button with arrow */
   button = gtk_button_new();
   GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
   gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_widget_set_events(button,
						 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
   gtk_signal_connect(GTK_OBJECT(button), "button_press_event",
					  (GtkSignalFunc) arrow_cb, NULL);
   gtk_widget_show(button);

   arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
   gtk_container_add(GTK_CONTAINER(button), arrow);
   gtk_widget_show(arrow);

   /* Create horizontal ruler */
   data->hruler = ruler = gtk_hruler_new();
   gtk_ruler_set_range(GTK_RULER(ruler), 0, data->width, 0, PREVIEW_SIZE);
   gtk_signal_connect_object(GTK_OBJECT(preview), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS(GTK_OBJECT(ruler)->klass)->motion_notify_event,
			     GTK_OBJECT(ruler));
   gtk_table_attach(GTK_TABLE(table), ruler, 1, 2, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_widget_show(ruler);

   /* Create vertical ruler */
   data->vruler = ruler = gtk_vruler_new();
   gtk_ruler_set_range(GTK_RULER(ruler), 0, data->height, 0, PREVIEW_SIZE);
   gtk_signal_connect_object(GTK_OBJECT(preview), "motion_notify_event",
			     (GtkSignalFunc) GTK_WIDGET_CLASS (GTK_OBJECT (ruler)->klass)->motion_notify_event,
			     GTK_OBJECT(ruler));
   gtk_table_attach(GTK_TABLE(table), ruler, 0, 1, 1, 2, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_widget_show(ruler);

   gimp_pixel_rgn_init(&data->src_rgn, drawable, 0, 0, data->width,
		       data->height, FALSE, FALSE);
   render_preview(preview, &data->src_rgn);

   gtk_widget_show(preview);

   gtk_widget_show(frame);
   gtk_widget_show(table);

   return data;
}
