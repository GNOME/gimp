/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatternmenu.c
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

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


#define PATTERN_SELECT_DATA_KEY "gimp-pattern-select-data"
#define CELL_SIZE               20


typedef struct _PatternSelect PatternSelect;

struct _PatternSelect
{
  gchar                  *title;
  GimpRunPatternCallback  callback;
  gpointer                data;

  GtkWidget              *preview;
  GtkWidget              *button;

  GtkWidget              *popup;

  gchar                  *pattern_name;      /* Local copy */
  gint                    width;
  gint                    height;
  gint                    bytes;
  guchar                 *mask_data;         /* local copy */

  const gchar            *temp_pattern_callback;
};


static void     gimp_pattern_select_widget_callback (const gchar   *name,
                                                     gint           width,
                                                     gint           height,
                                                     gint           bytes,
                                                     const guchar  *mask_data,
                                                     gboolean       closing,
                                                     gpointer       data);
static void     gimp_pattern_select_widget_clicked  (GtkWidget     *widget,
                                                     PatternSelect *pattern_sel);
static void     gimp_pattern_select_widget_destroy  (GtkWidget     *widget,
                                                     PatternSelect *pattern_sel);
static gboolean gimp_pattern_select_preview_events  (GtkWidget     *widget,
                                                     GdkEvent      *event,
                                                     PatternSelect *pattern_sel);
static void     gimp_pattern_select_preview_update  (GtkWidget     *preview,
                                                     gint           width,
                                                     gint           height,
                                                     gint           bytes,
                                                     const guchar  *mask_data);
static void     gimp_pattern_select_preview_resize  (PatternSelect *pattern_sel);
static void     gimp_pattern_select_popup_open      (PatternSelect *pattern_sel,
                                                     gint           x,
                                                     gint           y);
static void     gimp_pattern_select_popup_close     (PatternSelect *pattern_sel);

static void     gimp_pattern_select_drag_data_received (GtkWidget        *preview,
                                                        GdkDragContext   *context,
                                                        gint              x,
                                                        gint              y,
                                                        GtkSelectionData *selection,
                                                        guint             info,
                                                        guint             time,
                                                        GtkWidget        *widget);


static const GtkTargetEntry target = { "application/x-gimp-pattern-name", 0 };


/**
 * gimp_pattern_select_widget_new:
 * @title:        Title of the dialog to use or %NULL to use the default title.
 * @pattern_name: Initial pattern name or %NULL to use current selection.
 * @callback:     A function to call when the selected pattern changes.
 * @data:         A pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a pattern. This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_pattern_select_widget_new (const gchar            *title,
                                const gchar            *pattern_name,
                                GimpRunPatternCallback  callback,
                                gpointer                data)
{
  PatternSelect *pattern_sel;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  gint           mask_data_size;

  g_return_val_if_fail (callback != NULL, NULL);

  if (! title)
    title = _("Pattern Selection");

  pattern_sel = g_new0 (PatternSelect, 1);

  pattern_sel->title    = g_strdup (title);
  pattern_sel->callback = callback;
  pattern_sel->data     = data;

  hbox = gtk_hbox_new (FALSE, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  pattern_sel->preview = gimp_preview_area_new ();
  gtk_widget_add_events (pattern_sel->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (pattern_sel->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), pattern_sel->preview);
  gtk_widget_show (pattern_sel->preview);

  g_signal_connect_swapped (pattern_sel->preview, "size-allocate",
                            G_CALLBACK (gimp_pattern_select_preview_resize),
                            pattern_sel);
  g_signal_connect (pattern_sel->preview, "event",
                    G_CALLBACK (gimp_pattern_select_preview_events),
                    pattern_sel);

  gtk_drag_dest_set (GTK_WIDGET (pattern_sel->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect (pattern_sel->preview, "drag-data-received",
                    G_CALLBACK (gimp_pattern_select_drag_data_received),
                    hbox);

  pattern_sel->button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), pattern_sel->button, TRUE, TRUE, 0);
  gtk_widget_show (pattern_sel->button);

  g_signal_connect (pattern_sel->button, "clicked",
                    G_CALLBACK (gimp_pattern_select_widget_clicked),
                    pattern_sel);
  g_signal_connect (pattern_sel->button, "destroy",
                    G_CALLBACK (gimp_pattern_select_widget_destroy),
                    pattern_sel);

  /* Do initial pattern setup */
  if (! pattern_name || ! strlen (pattern_name))
    pattern_sel->pattern_name = gimp_context_get_pattern ();
  else
    pattern_sel->pattern_name = g_strdup (pattern_name);

  gimp_pattern_get_pixels (pattern_sel->pattern_name,
                           &pattern_sel->width,
                           &pattern_sel->height,
                           &pattern_sel->bytes,
                           &mask_data_size,
                           &pattern_sel->mask_data);

  g_object_set_data (G_OBJECT (hbox), PATTERN_SELECT_DATA_KEY, pattern_sel);

  return hbox;
}

/**
 * gimp_pattern_select_widget_close:
 * @widget: A pattern select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_pattern_select_widget_close (GtkWidget *widget)
{
  PatternSelect *pattern_sel;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  pattern_sel = g_object_get_data (G_OBJECT (widget), PATTERN_SELECT_DATA_KEY);

  g_return_if_fail (pattern_sel != NULL);

  if (pattern_sel->temp_pattern_callback)
    {
      gimp_pattern_select_destroy (pattern_sel->temp_pattern_callback);
      pattern_sel->temp_pattern_callback = NULL;
    }
}

/**
 * gimp_pattern_select_widget_set:
 * @widget:       A pattern select widget.
 * @pattern_name: Pattern name to set. NULL means no change.
 *
 * Sets the current pattern for the pattern select widget.  Calls the
 * callback function if one was supplied in the call to
 * gimp_pattern_select_widget_new().
 */
void
gimp_pattern_select_widget_set (GtkWidget   *widget,
                                const gchar *pattern_name)
{
  PatternSelect *pattern_sel;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  pattern_sel = g_object_get_data (G_OBJECT (widget), PATTERN_SELECT_DATA_KEY);

  g_return_if_fail (pattern_sel != NULL);

  if (pattern_sel->temp_pattern_callback)
    {
      gimp_patterns_set_popup (pattern_sel->temp_pattern_callback,
                               pattern_name);
    }
  else
    {
      gint    width;
      gint    height;
      gint    bytes;
      gint    mask_data_size;
      guint8 *mask_data;
      gchar  *name;

      if (! pattern_name || ! strlen (pattern_name))
        name = gimp_context_get_pattern ();
      else
        name = g_strdup (pattern_name);

      if (gimp_pattern_get_pixels (name,
                                   &width,
                                   &height,
                                   &bytes,
                                   &mask_data_size,
                                   &mask_data))
        {
          gimp_pattern_select_widget_callback (name, width, height, bytes,
                                               mask_data, FALSE, pattern_sel);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_pattern_select_widget_callback (const gchar  *name,
                                     gint          width,
                                     gint          height,
                                     gint          bytes,
                                     const guchar *mask_data,
                                     gboolean      closing,
                                     gpointer      data)
{
  PatternSelect *pattern_sel = (PatternSelect *) data;

  g_free (pattern_sel->pattern_name);
  g_free (pattern_sel->mask_data);

  pattern_sel->pattern_name = g_strdup (name);
  pattern_sel->width        = width;
  pattern_sel->height       = height;
  pattern_sel->bytes        = bytes;
  pattern_sel->mask_data    = g_memdup (mask_data, width * height * bytes);

  gimp_pattern_select_preview_update (pattern_sel->preview,
                                      width, height, bytes, mask_data);

  if (pattern_sel->callback)
    pattern_sel->callback (name, width, height, bytes,
                           mask_data, closing, pattern_sel->data);

  if (closing)
    pattern_sel->temp_pattern_callback = NULL;
}

static void
gimp_pattern_select_widget_clicked (GtkWidget     *widget,
                                    PatternSelect *pattern_sel)
{
  if (pattern_sel->temp_pattern_callback)
    {
      /*  calling gimp_patterns_set_popup() raises the dialog  */
      gimp_patterns_set_popup (pattern_sel->temp_pattern_callback,
                               pattern_sel->pattern_name);
    }
  else
    {
      pattern_sel->temp_pattern_callback =
	gimp_pattern_select_new (pattern_sel->title,
                                 pattern_sel->pattern_name,
                                 gimp_pattern_select_widget_callback,
                                 pattern_sel);
    }
}

static void
gimp_pattern_select_widget_destroy (GtkWidget     *widget,
                                    PatternSelect *pattern_sel)
{
  if (pattern_sel->temp_pattern_callback)
    {
      gimp_pattern_select_destroy (pattern_sel->temp_pattern_callback);
      pattern_sel->temp_pattern_callback = NULL;
    }

  g_free (pattern_sel->title);
  g_free (pattern_sel->pattern_name);
  g_free (pattern_sel->mask_data);
  g_free (pattern_sel);
}

static void
gimp_pattern_select_preview_resize (PatternSelect *pattern_sel)
{
  if (pattern_sel->width > 0 && pattern_sel->height > 0)
    gimp_pattern_select_preview_update (pattern_sel->preview,
                                        pattern_sel->width,
                                        pattern_sel->height,
                                        pattern_sel->bytes,
                                        pattern_sel->mask_data);

}

static gboolean
gimp_pattern_select_preview_events (GtkWidget     *widget,
                                    GdkEvent      *event,
                                    PatternSelect *pattern_sel)
{
  GdkEventButton *bevent;

  if (pattern_sel->mask_data)
    {
      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 1)
	    {
	      gtk_grab_add (widget);
	      gimp_pattern_select_popup_open (pattern_sel,
                                              bevent->x, bevent->y);
	    }
	  break;

	case GDK_BUTTON_RELEASE:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 1)
	    {
	      gtk_grab_remove (widget);
	      gimp_pattern_select_popup_close (pattern_sel);
	    }
	  break;

	default:
	  break;
	}
    }

  return FALSE;
}

static void
gimp_pattern_select_preview_update (GtkWidget    *preview,
                                    gint          width,
                                    gint          height,
                                    gint          bytes,
                                    const guchar *mask_data)
{
  GimpImageType  type;

  switch (bytes)
    {
    case 1:  type = GIMP_GRAY_IMAGE;   break;
    case 2:  type = GIMP_GRAYA_IMAGE;  break;
    case 3:  type = GIMP_RGB_IMAGE;    break;
    case 4:  type = GIMP_RGBA_IMAGE;   break;
    default:
      return;
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview),
                          0, 0, width, height,
                          type,
                          mask_data,
                          width * bytes);

}

static void
gimp_pattern_select_popup_open (PatternSelect *pattern_sel,
                                gint           x,
                                gint           y)
{
  GtkWidget *frame;
  GtkWidget *preview;
  GdkScreen *screen;
  gint       x_org;
  gint       y_org;
  gint       scr_w;
  gint       scr_h;

  if (pattern_sel->popup)
    gimp_pattern_select_popup_close (pattern_sel);

  if (pattern_sel->width <= CELL_SIZE && pattern_sel->height <= CELL_SIZE)
    return;

  pattern_sel->popup = gtk_window_new (GTK_WINDOW_POPUP);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (pattern_sel->popup), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview,
                               pattern_sel->width, pattern_sel->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup */
  gdk_window_get_origin (pattern_sel->preview->window, &x_org, &y_org);

  screen = gtk_widget_get_screen (pattern_sel->popup);
  scr_w = gdk_screen_get_width (screen);
  scr_h = gdk_screen_get_height (screen);

  x = x_org + x - (pattern_sel->width  / 2);
  y = y_org + y - (pattern_sel->height / 2);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + pattern_sel->width  > scr_w) ? scr_w - pattern_sel->width  : x;
  y = (y + pattern_sel->height > scr_h) ? scr_h - pattern_sel->height : y;

  gtk_window_move (GTK_WINDOW (pattern_sel->popup), x, y);

  gtk_widget_show (pattern_sel->popup);

  /*  Draw the pattern  */
  gimp_pattern_select_preview_update (preview,
                                      pattern_sel->width, pattern_sel->height,
                                      pattern_sel->bytes,
                                      pattern_sel->mask_data);
}

static void
gimp_pattern_select_popup_close (PatternSelect *pattern_sel)
{
  if (pattern_sel->popup)
    {
      gtk_widget_destroy (pattern_sel->popup);
      pattern_sel->popup = NULL;
    }
}

static void
gimp_pattern_select_drag_data_received (GtkWidget        *preview,
                                        GdkDragContext   *context,
                                        gint              x,
                                        gint              y,
                                        GtkSelectionData *selection,
                                        guint             info,
                                        guint             time,
                                        GtkWidget        *widget)
{
  gchar *str;

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid pattern data!");
      return;
    }

  str = g_strndup ((const gchar *) selection->data, selection->length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == gimp_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          gimp_pattern_select_widget_set (widget, name);
        }
    }

  g_free (str);
}
