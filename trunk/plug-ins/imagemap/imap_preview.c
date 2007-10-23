/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimp/gimp.h"

#include <libgimp/gimpui.h>

#include "imap_commands.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_preview.h"

#define PREVIEW_MASK  (GDK_EXPOSURE_MASK       | \
		       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK   | \
		       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON_MOTION_MASK  | \
		       GDK_KEY_PRESS_MASK      | \
		       GDK_KEY_RELEASE_MASK    | \
		       GDK_ENTER_NOTIFY_MASK   | \
		       GDK_LEAVE_NOTIFY_MASK)

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
  return (Preview_t*) g_object_get_data (G_OBJECT (preview), "preview");
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
render_gray_image(Preview_t *preview_base, GimpPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_buffer, *src, *dest;
   gint		 row, col;
   gint		 bpp, dwidth, dheight, pwidth, pheight;
   gint		 *src_col;
   GtkWidget     *preview = preview_base->preview;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   pwidth = preview_base->widget_width;
   pheight = preview_base->widget_height;
   bpp = srcrgn->bpp;

   src_row = g_new(guchar, dwidth * bpp);
   dest_buffer = g_new(guchar, pwidth * pheight);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   dest = dest_buffer;

   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);

      src = src_row;

      for (col = 0; col < pwidth; col++) {
	 src = &src_row[src_col[col]];
	 *dest++ = *src;
      }
   }
   gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                           0, 0, pwidth, pheight,
                           GIMP_GRAY_IMAGE,
                           dest_buffer,
                           pwidth);
   g_free(src_col);
   g_free(src_row);
   g_free(dest_buffer);

}

static void
render_indexed_image(Preview_t *preview_base, GimpPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_buffer, *src, *dest;
   gint		 row, col;
   gint		 dwidth, dheight, pwidth, pheight;
   gint		 *src_col;
   gint		 bpp, alpha, has_alpha;
   guchar 	 *cmap, *colour;
   gint 	 ncols;
   gboolean	 gray = get_map_info()->show_gray;
   GtkWidget    *preview = preview_base->preview;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   pwidth = preview_base->widget_width;
   pheight = preview_base->widget_height;
   bpp = srcrgn->bpp;
   alpha = bpp;
   has_alpha = gimp_drawable_has_alpha(srcrgn->drawable->drawable_id);
   if (has_alpha)
      alpha--;

   cmap = gimp_image_get_colormap (gimp_drawable_get_image (srcrgn->drawable->drawable_id),
                                   &ncols);

   src_row = g_new(guchar, dwidth * bpp);
   dest_buffer = g_new(guchar, pwidth * pheight * 3);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   dest = dest_buffer;
   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);

      for (col = 0; col < pwidth; col++) {
	 src = &src_row[src_col[col]];
	 colour = cmap + 3 * (int)(*src);

	 if (gray) {
	    guchar avg = (299 * colour[0] + 587 * colour[1] +
			  114 * colour[2]) / 1000;
	    *dest++ = avg;
	    *dest++ = avg;
	    *dest++ = avg;
	 } else {
	    *dest++ = colour[0];
	    *dest++ = colour[1];
	    *dest++ = colour[2];
	 }
      }
   }
   gimp_preview_area_draw(GIMP_PREVIEW_AREA(preview),
                          0, 0, pwidth, pheight,
                          GIMP_RGB_IMAGE,
                          dest_buffer,
                          pwidth * 3);
   g_free(src_col);
   g_free(src_row);
   g_free(dest_buffer);
}

static void
render_rgb_image(Preview_t *preview_base, GimpPixelRgn *srcrgn)
{
   guchar	 *src_row, *dest_buffer, *src, *dest;
   gint		 row, col;
   gint		 dwidth, dheight, pwidth, pheight;
   gint		 *src_col;
   gint		 bpp, alpha, has_alpha, b;
   guchar	 check;
   gboolean	 gray = get_map_info()->show_gray;
   GtkWidget    *preview = preview_base->preview;

   dwidth  = srcrgn->w;
   dheight = srcrgn->h;
   pwidth = preview_base->widget_width;
   pheight = preview_base->widget_height;
   bpp = srcrgn->bpp;
   alpha = bpp;
   has_alpha = gimp_drawable_has_alpha(srcrgn->drawable->drawable_id);
   if (has_alpha)
      alpha--;

   src_row = g_new(guchar, dwidth * bpp);
   dest_buffer = g_new(guchar, pwidth * pheight * bpp);
   src_col = g_new(gint, pwidth);

   for (col = 0; col < pwidth; col++)
      src_col[col] = (col * dwidth / pwidth) * bpp;

   dest = dest_buffer;
   for (row = 0; row < pheight; row++) {
      gimp_pixel_rgn_get_row(srcrgn, src_row, 0, row * dheight / pheight,
			     dwidth);
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
   }
   gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                           0, 0, pwidth, pheight,
                           GIMP_RGB_IMAGE,
                           dest_buffer,
                           pwidth * 3);
   g_free(src_col);
   g_free(src_row);
   g_free(dest_buffer);
}

static void
render_preview(Preview_t *preview_base, GimpPixelRgn *srcrgn)
{
   switch (gimp_drawable_type(srcrgn->drawable->drawable_id)) {
   case GIMP_RGB_IMAGE:
   case GIMP_RGBA_IMAGE:
      render_rgb_image(preview_base, srcrgn);
      break;
   case GIMP_GRAY_IMAGE:
   case GIMP_GRAYA_IMAGE:
      render_gray_image(preview_base, srcrgn);
      break;
   case GIMP_INDEXED_IMAGE:
   case GIMP_INDEXEDA_IMAGE:
      render_indexed_image(preview_base, srcrgn);
      break;
   }
}

static gboolean
arrow_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   if (event->button == 1)
      do_main_popup_menu(event);
   return TRUE;
}

static gboolean
preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
   draw_grid(widget);
   draw_shapes(widget);
   return FALSE;
}

void
add_preview_motion_event(Preview_t *preview, GCallback func)
{
   g_return_if_fail (func != NULL);

   g_signal_connect(preview->preview, "motion-notify-event",
		    func, NULL);
}

void
add_enter_notify_event(Preview_t *preview, GCallback func)
{
   g_return_if_fail (func != NULL);

   g_signal_connect(preview->preview, "enter-notify-event",
		    func, NULL);
}

void
add_leave_notify_event(Preview_t *preview, GCallback func)
{
   g_return_if_fail (func != NULL);

   g_signal_connect(preview->preview, "leave-notify-event",
		    func, NULL);
}

void
add_preview_button_press_event(Preview_t *preview, GCallback func)
{
   g_return_if_fail (func != NULL);

   g_signal_connect(preview->preview, "button-press-event",
		    func, NULL);
}

void
preview_redraw(Preview_t *preview)
{
  gtk_widget_queue_draw(preview->preview);
}

void
preview_zoom(Preview_t *preview, gint zoom_factor)
{
   preview->widget_width = preview->width * zoom_factor;
   preview->widget_height = preview->height * zoom_factor;
   gtk_widget_set_size_request (preview->preview, preview->widget_width,
                                preview->widget_height);
   gtk_widget_queue_resize(preview->window);
   render_preview(preview, &preview->src_rgn);
   preview_redraw(preview);
}

GdkCursorType
preview_set_cursor(Preview_t *preview, GdkCursorType cursor_type)
{
   GdkCursorType  prev_cursor = preview->cursor;
   GdkDisplay    *display     = gtk_widget_get_display (preview->window);
   GdkCursor     *cursor      = gdk_cursor_new_for_display (display,
                                                            cursor_type);

   gdk_window_set_cursor(preview->window->window, cursor);
   gdk_cursor_unref(cursor);

   preview->cursor = cursor_type;

   return prev_cursor;
}

static const GtkTargetEntry target_table[] =
{
   {"STRING",     0, 1 },
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
	 object_set_url(obj, (const gchar *) data->data);
	 object_emit_update_signal(obj);
	 success = TRUE;
      }
   }
   gtk_drag_finish(context, success, FALSE, time);
}

static void
preview_size_allocate (GtkWidget *widget,
                       GtkAllocation *allocation,
                       gpointer preview_void)
{
  Preview_t * preview = preview_void;

  render_preview(preview, &preview->src_rgn);
}

static void
scroll_adj_changed (GtkAdjustment *adj,
                    GtkRuler      *ruler)
{
  gtk_ruler_set_range (ruler,
                       adj->value, adj->value + adj->page_size,
                       G_MAXDOUBLE, adj->page_size);
}

Preview_t*
make_preview(GimpDrawable *drawable)
{
   Preview_t *data = g_new(Preview_t, 1);
   GtkAdjustment *hadj;
   GtkAdjustment *vadj;
   GtkWidget *preview;
   GtkWidget *window;
   GtkWidget *button, *arrow;
   GtkWidget *ruler;
   GtkWidget *table;
   GtkWidget *scrollbar;
   gint width, height;

   data->drawable = drawable;
   data->preview = preview = gimp_preview_area_new ();

   g_object_set_data (G_OBJECT (preview), "preview", data);
   gtk_widget_set_events(GTK_WIDGET(preview), PREVIEW_MASK);
   g_signal_connect_after(preview, "expose-event",
			  G_CALLBACK(preview_expose), data);
   g_signal_connect (preview, "size-allocate",
                     G_CALLBACK (preview_size_allocate), (gpointer)data);

   /* Handle drop of links in preview widget */
   gtk_drag_dest_set(preview, GTK_DEST_DEFAULT_ALL, target_table,
		     2, GDK_ACTION_COPY);
   g_signal_connect(preview, "drag-data-received",
		    G_CALLBACK(handle_drop), NULL);

   data->widget_width = data->width =
       gimp_drawable_width(drawable->drawable_id);
   data->widget_height = data->height =
       gimp_drawable_height(drawable->drawable_id);
   gtk_widget_set_size_request (preview, data->widget_width,
                                data->widget_height);

   /* The main table */
   data->window = table = gtk_table_new(3, 3, FALSE);
   gtk_table_set_col_spacings (GTK_TABLE (table), 1);
   gtk_table_set_row_spacings (GTK_TABLE (table), 1);

   /* Create button with arrow */
   button = gtk_button_new();
   GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
   gtk_table_attach(GTK_TABLE(table), button, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_widget_set_events(button,
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
   g_signal_connect(button, "button-press-event",
		    G_CALLBACK(arrow_cb), NULL);
   gtk_widget_show(button);

   arrow = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
   gtk_container_add(GTK_CONTAINER(button), arrow);
   gtk_widget_show(arrow);

   /* Create horizontal ruler */
   data->hruler = ruler = gtk_hruler_new();
   g_signal_connect_swapped(preview, "motion-notify-event",
			    G_CALLBACK(GTK_WIDGET_GET_CLASS(ruler)->motion_notify_event),
			    ruler);

   gtk_table_attach(GTK_TABLE(table), ruler, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(ruler);

   /* Create vertical ruler */
   data->vruler = ruler = gtk_vruler_new();
   g_signal_connect_swapped(preview, "motion-notify-event",
			    G_CALLBACK(GTK_WIDGET_GET_CLASS(ruler)->motion_notify_event),
			    ruler);
   gtk_table_attach(GTK_TABLE(table), ruler, 0, 1, 1, 2,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
   gtk_widget_show(ruler);

   window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window),
				  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
   width = (data->width > 600) ? 600 : data->width;
   height = (data->height > 400) ? 400 : data->height;
   gtk_widget_set_size_request(window, width, height);
   gtk_table_attach(GTK_TABLE(table), window, 1, 2, 1, 2, GTK_FILL, GTK_FILL,
		    0, 0);
   gtk_widget_show(window);

   hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (window));

   g_signal_connect (hadj, "changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->hruler);
   g_signal_connect (hadj, "value-changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->hruler);

   vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (window));

   g_signal_connect (vadj, "changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->vruler);
   g_signal_connect (vadj, "value-changed",
                     G_CALLBACK (scroll_adj_changed),
                     data->vruler);

   gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(window), preview);

   scrollbar = gtk_hscrollbar_new (hadj);
   gtk_table_attach(GTK_TABLE(table), scrollbar, 1, 2, 2, 3,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
   gtk_widget_show (scrollbar);

   scrollbar = gtk_vscrollbar_new (vadj);
   gtk_table_attach(GTK_TABLE(table), scrollbar,  2, 3, 1, 2,
                    GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
   gtk_widget_show (scrollbar);

   gtk_widget_show (preview);

   gimp_pixel_rgn_init(&data->src_rgn, drawable, 0, 0, data->width,
		       data->height, FALSE, FALSE);
   render_preview(data, &data->src_rgn);

   gtk_widget_show(table);

   return data;
}
