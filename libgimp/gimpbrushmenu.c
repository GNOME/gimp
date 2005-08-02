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

#include "gimp.h"
#include "gimpui.h"

#include "libgimp-intl.h"


#define BRUSH_SELECT_DATA_KEY "gimp-brush-select-data"
#define CELL_SIZE             20


typedef struct _BrushSelect BrushSelect;

struct _BrushSelect
{
  gchar                *title;
  GimpRunBrushCallback  callback;
  gpointer              data;

  GtkWidget            *preview;
  GtkWidget            *button;

  GtkWidget            *popup;

  gchar                *brush_name;       /* Local copy */
  gdouble               opacity;
  gint                  spacing;
  GimpLayerModeEffects  paint_mode;
  gint                  width;
  gint                  height;
  guchar               *mask_data;        /* local copy */

  const gchar          *temp_brush_callback;
};


/*  local function prototypes  */

static void     gimp_brush_select_widget_callback (const gchar  *name,
                                                   gdouble       opacity,
                                                   gint          spacing,
                                                   GimpLayerModeEffects  paint_mode,
                                                   gint          width,
                                                   gint          height,
                                                   const guchar *mask_data,
                                                   gboolean      closing,
                                                   gpointer      data);
static void     gimp_brush_select_widget_clicked  (GtkWidget    *widget,
                                                   BrushSelect  *brush_sel);
static void     gimp_brush_select_widget_destroy  (GtkWidget    *widget,
                                                   BrushSelect  *brush_sel);
static gboolean gimp_brush_select_preview_events  (GtkWidget    *widget,
                                                   GdkEvent     *event,
                                                   BrushSelect  *brush_sel);
static void     gimp_brush_select_preview_update  (GtkWidget    *preview,
                                                   gint          brush_width,
                                                   gint          brush_height,
                                                   const guchar *mask_data);
static void     gimp_brush_select_preview_resize  (BrushSelect  *brush_sel);
static void     gimp_brush_select_popup_open      (BrushSelect  *brush_sel,
                                                   gint          x,
                                                   gint          y);
static void     gimp_brush_select_popup_close     (BrushSelect  *brush_sel);

static void  gimp_brush_select_drag_data_received (GtkWidget        *preview,
                                                   GdkDragContext   *context,
                                                   gint              x,
                                                   gint              y,
                                                   GtkSelectionData *selection,
                                                   guint             info,
                                                   guint             time,
                                                   GtkWidget        *widget);


static const GtkTargetEntry target = { "application/x-gimp-brush-name", 0 };


/**
 * gimp_brush_select_widget_new:
 * @title:      Title of the dialog to use or %NULL to use the default title.
 * @brush_name: Initial brush name or %NULL to use current selection.
 * @opacity:    Initial opacity. -1 means to use current opacity.
 * @spacing:    Initial spacing. -1 means to use current spacing.
 * @paint_mode: Initial paint mode.  -1 means to use current paint mode.
 * @callback:   A function to call when the selected brush changes.
 * @data:       A pointer to arbitary data to be used in the call to @callback.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a #GimpBrush. This widget is suitable for placement in a table in
 * a plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 */
GtkWidget *
gimp_brush_select_widget_new (const gchar          *title,
                              const gchar          *brush_name,
                              gdouble               opacity,
                              gint                  spacing,
                              GimpLayerModeEffects  paint_mode,
                              GimpRunBrushCallback  callback,
                              gpointer              data)
{
  BrushSelect *brush_sel;
  GtkWidget   *frame;
  GtkWidget   *hbox;
  gint         mask_bpp;
  gint         mask_data_size;
  gint         color_bpp;
  gint         color_data_size;
  guint8      *color_data;

  g_return_val_if_fail (callback != NULL, NULL);

  if (! title)
    title = _("Brush Selection");

  brush_sel = g_new0 (BrushSelect, 1);

  brush_sel->title    = g_strdup (title);
  brush_sel->callback = callback;
  brush_sel->data     = data;

  hbox = gtk_hbox_new (FALSE, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  brush_sel->preview = gimp_preview_area_new ();
  gtk_widget_add_events (brush_sel->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (brush_sel->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), brush_sel->preview);
  gtk_widget_show (brush_sel->preview);

  g_signal_connect_swapped (brush_sel->preview, "size-allocate",
                            G_CALLBACK (gimp_brush_select_preview_resize),
                            brush_sel);
  g_signal_connect (brush_sel->preview, "event",
                    G_CALLBACK (gimp_brush_select_preview_events),
                    brush_sel);

  gtk_drag_dest_set (GTK_WIDGET (brush_sel->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect (brush_sel->preview, "drag-data-received",
                    G_CALLBACK (gimp_brush_select_drag_data_received),
                    hbox);

  brush_sel->button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_end (GTK_BOX (hbox), brush_sel->button, TRUE, TRUE, 0);
  gtk_widget_show (brush_sel->button);

  g_signal_connect (brush_sel->button, "clicked",
                    G_CALLBACK (gimp_brush_select_widget_clicked),
                    brush_sel);
  g_signal_connect (brush_sel->button, "destroy",
                    G_CALLBACK (gimp_brush_select_widget_destroy),
                    brush_sel);

  /* Do initial brush setup */
  if (! brush_name || ! strlen (brush_name))
    brush_sel->brush_name = gimp_context_get_brush ();
  else
    brush_sel->brush_name = g_strdup (brush_name);

  if (gimp_brush_get_pixels (brush_sel->brush_name,
                             &brush_sel->width,
                             &brush_sel->height,
                             &mask_bpp,
                             &mask_data_size,
                             &brush_sel->mask_data,
                             &color_bpp,
                             &color_data_size,
                             &color_data))
    {
      if (color_data)
        g_free (color_data);

      if (opacity == -1)
        brush_sel->opacity = gimp_context_get_opacity ();
      else
        brush_sel->opacity = opacity;

      if (paint_mode == -1)
        brush_sel->paint_mode = gimp_context_get_paint_mode ();
      else
        brush_sel->paint_mode = paint_mode;

      if (spacing == -1)
        gimp_brush_get_spacing (brush_sel->brush_name, &brush_sel->spacing);
      else
        brush_sel->spacing = spacing;
    }

  g_object_set_data (G_OBJECT (hbox), BRUSH_SELECT_DATA_KEY, brush_sel);

  return hbox;
}

/**
 * gimp_brush_select_widget_close:
 * @widget: A brush select widget.
 *
 * Closes the popup window associated with @widget.
 */
void
gimp_brush_select_widget_close (GtkWidget *widget)
{
  BrushSelect *brush_sel;

  brush_sel = g_object_get_data (G_OBJECT (widget), BRUSH_SELECT_DATA_KEY);

  g_return_if_fail (brush_sel != NULL);

  if (brush_sel->temp_brush_callback)
    {
      gimp_brush_select_destroy (brush_sel->temp_brush_callback);
      brush_sel->temp_brush_callback = NULL;
    }
}

/**
 * gimp_brush_select_widget_set;
 * @widget:     A brush select widget.
 * @brush_name: Brush name to set; %NULL means no change.
 * @opacity:    Opacity to set. -1.0 means no change.
 * @spacing:    Spacing to set. -1 means no change.
 * @paint_mode: Paint mode to set.  -1 means no change.
 *
 * Sets the current brush and other values for the brush select
 * widget.  Calls the callback function if one was supplied in the
 * call to gimp_brush_select_widget_new().
 */
void
gimp_brush_select_widget_set (GtkWidget            *widget,
                              const gchar          *brush_name,
                              gdouble               opacity,
                              gint                  spacing,
                              GimpLayerModeEffects  paint_mode)
{
  BrushSelect *brush_sel;

  brush_sel = g_object_get_data (G_OBJECT (widget), BRUSH_SELECT_DATA_KEY);

  g_return_if_fail (brush_sel != NULL);

  if (brush_sel->temp_brush_callback)
    {
      gimp_brushes_set_popup (brush_sel->temp_brush_callback,
                              brush_name, opacity, spacing, paint_mode);
    }
  else
    {
      gchar  *name;
      gint    width;
      gint    height;
      gint    mask_bpp;
      gint    mask_data_size;
      guint8 *mask_data;
      gint    color_bpp;
      gint    color_data_size;
      guint8 *color_data;

      if (! brush_name || ! strlen (brush_name))
        name = gimp_context_get_brush ();
      else
        name = g_strdup (brush_name);

      if (gimp_brush_get_pixels (name,
                                 &width,
                                 &height,
                                 &mask_bpp,
                                 &mask_data_size,
                                 &mask_data,
                                 &color_bpp,
                                 &color_data_size,
                                 &color_data))
        {
          if (color_data)
            g_free (color_data);

          if (opacity < 0.0)
            opacity = gimp_context_get_opacity ();

          if (paint_mode == -1)
            paint_mode = gimp_context_get_paint_mode ();

          if (spacing == -1)
            gimp_brush_get_spacing (name, &spacing);

          gimp_brush_select_widget_callback (name, opacity, spacing,
                                             paint_mode, width, height,
                                             mask_data, FALSE, brush_sel);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
gimp_brush_select_widget_callback (const gchar          *name,
                                   gdouble               opacity,
                                   gint                  spacing,
                                   GimpLayerModeEffects  paint_mode,
                                   gint                  width,
                                   gint                  height,
                                   const guchar         *mask_data,
                                   gboolean              closing,
                                   gpointer              data)
{
  BrushSelect *brush_sel = (BrushSelect *) data;

  g_free (brush_sel->brush_name);
  g_free (brush_sel->mask_data);

  brush_sel->brush_name = g_strdup (name);
  brush_sel->width      = width;
  brush_sel->height     = height;
  brush_sel->mask_data  = g_memdup (mask_data, width * height);
  brush_sel->opacity    = opacity;
  brush_sel->spacing    = spacing;
  brush_sel->paint_mode = paint_mode;

  gimp_brush_select_preview_update (brush_sel->preview,
                                    width, height, mask_data);

  if (brush_sel->callback)
    brush_sel->callback (name, opacity, spacing, paint_mode,
                         width, height, mask_data, closing, brush_sel->data);

  if (closing)
    brush_sel->temp_brush_callback = NULL;
}

static void
gimp_brush_select_widget_clicked (GtkWidget   *widget,
                                  BrushSelect *brush_sel)
{
  if (brush_sel->temp_brush_callback)
    {
      /*  calling gimp_brushes_set_popup() raises the dialog  */
      gimp_brushes_set_popup (brush_sel->temp_brush_callback,
			      brush_sel->brush_name,
			      brush_sel->opacity,
			      brush_sel->spacing,
			      brush_sel->paint_mode);
    }
  else
    {
      brush_sel->temp_brush_callback =
        gimp_brush_select_new (brush_sel->title,
                               brush_sel->brush_name,
                               brush_sel->opacity,
                               brush_sel->spacing,
                               brush_sel->paint_mode,
                               gimp_brush_select_widget_callback,
                               brush_sel);
    }
}

static void
gimp_brush_select_widget_destroy (GtkWidget   *widget,
                                  BrushSelect *brush_sel)
{
  if (brush_sel->temp_brush_callback)
    {
      gimp_brush_select_destroy (brush_sel->temp_brush_callback);
      brush_sel->temp_brush_callback = NULL;
    }

  g_free (brush_sel->title);
  g_free (brush_sel->brush_name);
  g_free (brush_sel->mask_data);
  g_free (brush_sel);
}


static void
gimp_brush_select_preview_resize (BrushSelect *brush_sel)
{
  if (brush_sel->width > 0 && brush_sel->height > 0)
    gimp_brush_select_preview_update (brush_sel->preview,
                                      brush_sel->width,
                                      brush_sel->height,
                                      brush_sel->mask_data);

}


static gboolean
gimp_brush_select_preview_events (GtkWidget   *widget,
                                  GdkEvent    *event,
                                  BrushSelect *brush_sel)
{
  GdkEventButton *bevent;

  if (brush_sel->mask_data)
    {
      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 1)
	    {
	      gtk_grab_add (widget);
	      gimp_brush_select_popup_open (brush_sel, bevent->x, bevent->y);
	    }
	  break;

	case GDK_BUTTON_RELEASE:
	  bevent = (GdkEventButton *) event;

	  if (bevent->button == 1)
	    {
	      gtk_grab_remove (widget);
	      gimp_brush_select_popup_close (brush_sel);
	    }
	  break;

	default:
	  break;
	}
    }

  return FALSE;
}

static void
gimp_brush_select_preview_draw (GimpPreviewArea *area,
                                gint             x,
                                gint             y,
                                gint             width,
                                gint             height,
                                const guchar    *mask_data,
                                gint             rowstride)
{
  const guchar *src;
  guchar       *dest;
  guchar       *buf;
  gint          i, j;

  buf = g_new (guchar, width * height);

  src  = mask_data;
  dest = buf;

  for (j = 0; j < height; j++)
    {
      const guchar *s = src;

      for (i = 0; i < width; i++, s++, dest++)
        *dest = 255 - *s;

      src += rowstride;
    }

  gimp_preview_area_draw (area,
                          x, y, width, height,
                          GIMP_GRAY_IMAGE,
                          buf,
                          width);

  g_free (buf);
}

static void
gimp_brush_select_preview_update (GtkWidget    *preview,
                                  gint          brush_width,
                                  gint          brush_height,
                                  const guchar *mask_data)
{
  GimpPreviewArea *area = GIMP_PREVIEW_AREA (preview);
  gint             x, y;
  gint             width, height;

  width  = MIN (brush_width,  preview->allocation.width);
  height = MIN (brush_height, preview->allocation.height);

  x = ((preview->allocation.width  - width)  / 2);
  y = ((preview->allocation.height - height) / 2);

  if (x || y)
    gimp_preview_area_fill (area,
                            0, 0,
                            preview->allocation.width,
                            preview->allocation.height,
                            0xFF, 0xFF, 0xFF);

  gimp_brush_select_preview_draw (area,
                                  x, y, width, height,
                                  mask_data, brush_width);
}

static void
gimp_brush_select_popup_open (BrushSelect  *brush_sel,
                              gint          x,
                              gint          y)
{
  GtkWidget    *frame;
  GtkWidget    *preview;
  GdkScreen    *screen;
  gint          x_org;
  gint          y_org;
  gint          scr_w;
  gint          scr_h;

  if (brush_sel->popup)
    gimp_brush_select_popup_close (brush_sel);

  if (brush_sel->width <= CELL_SIZE && brush_sel->height <= CELL_SIZE)
    return;

  brush_sel->popup = gtk_window_new (GTK_WINDOW_POPUP);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (brush_sel->popup), frame);
  gtk_widget_show (frame);

  preview = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview, brush_sel->width, brush_sel->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup */
  gdk_window_get_origin (brush_sel->preview->window, &x_org, &y_org);

  screen = gtk_widget_get_screen (brush_sel->popup);

  scr_w = gdk_screen_get_width (screen);
  scr_h = gdk_screen_get_height (screen);

  x = x_org + x - (brush_sel->width  / 2);
  y = y_org + y - (brush_sel->height / 2);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + brush_sel->width  > scr_w) ? scr_w - brush_sel->width  : x;
  y = (y + brush_sel->height > scr_h) ? scr_h - brush_sel->height : y;

  gtk_window_move (GTK_WINDOW (brush_sel->popup), x, y);

  gtk_widget_show (brush_sel->popup);

  /*  Draw the brush  */
  gimp_brush_select_preview_draw (GIMP_PREVIEW_AREA (preview),
                                  0, 0, brush_sel->width, brush_sel->height,
                                  brush_sel->mask_data, brush_sel->width);
}

static void
gimp_brush_select_popup_close (BrushSelect *brush_sel)
{
  if (brush_sel->popup)
    {
      gtk_widget_destroy (brush_sel->popup);
      brush_sel->popup = NULL;
    }
}

static void
gimp_brush_select_drag_data_received (GtkWidget        *preview,
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
      g_warning ("Received invalid brush data!");
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

          gimp_brush_select_widget_set (widget, name, -1.0, -1, -1);
        }
    }

  g_free (str);
}
