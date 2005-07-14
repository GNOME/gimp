/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppaletteview.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimppalette.h"
#include "core/gimpmarshal.h"

#include "gimppaletteview.h"
#include "gimpviewrendererpalette.h"


enum
{
  ENTRY_SELECTED,
  ENTRY_ACTIVATED,
  ENTRY_CONTEXT,
  LAST_SIGNAL
};


static void   gimp_palette_view_class_init (GimpPaletteViewClass   *klass);
static void   gimp_palette_view_init       (GimpPaletteView        *view);

static void     gimp_palette_view_realize        (GtkWidget        *widget);
static void     gimp_palette_view_unrealize      (GtkWidget        *widget);
static gboolean gimp_palette_view_expose         (GtkWidget        *widget,
                                                  GdkEventExpose   *eevent);
static gboolean gimp_palette_view_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static void     gimp_palette_view_set_viewable   (GimpView         *view,
                                                  GimpViewable     *old_viewable,
                                                  GimpViewable     *new_viewable);
static void     gimp_palette_view_expose_entry   (GimpPaletteView  *view,
                                                  GimpPaletteEntry *entry);
static void     gimp_palette_view_draw_selected  (GimpPaletteView  *view,
                                                  GdkRectangle     *area);
static void     gimp_palette_view_invalidate     (GimpPalette      *palette,
                                                  GimpPaletteView  *view);


static guint view_signals[LAST_SIGNAL] = { 0 };

static GimpViewClass *parent_class = NULL;


GType
gimp_palette_view_get_type (void)
{
  static GType view_type = 0;

  if (! view_type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpPaletteViewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_palette_view_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPaletteView),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_palette_view_init,
      };

      view_type = g_type_register_static (GIMP_TYPE_VIEW,
                                          "GimpPaletteView",
                                          &view_info, 0);
    }

  return view_type;
}

static void
gimp_palette_view_class_init (GimpPaletteViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GimpViewClass  *view_class   = GIMP_VIEW_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  view_signals[ENTRY_SELECTED] =
    g_signal_new ("entry-selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPaletteViewClass, entry_selected),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPaletteViewClass, entry_activated),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[ENTRY_CONTEXT] =
    g_signal_new ("entry-context",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPaletteViewClass, entry_context),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  widget_class->realize            = gimp_palette_view_realize;
  widget_class->unrealize          = gimp_palette_view_unrealize;
  widget_class->expose_event       = gimp_palette_view_expose;
  widget_class->button_press_event = gimp_palette_view_button_press;

  view_class->set_viewable         = gimp_palette_view_set_viewable;
}

static void
gimp_palette_view_init (GimpPaletteView *view)
{
  GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);

  view->selected  = NULL;
  view->dnd_entry = NULL;
  view->gc        = NULL;
}

static void
gimp_palette_view_realize (GtkWidget *widget)
{
  GimpPaletteView *view = GIMP_PALETTE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  view->gc = gdk_gc_new (widget->window);

  gdk_gc_set_function (view->gc, GDK_INVERT);
  gdk_gc_set_line_attributes (view->gc, 1,
                              GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
}

static void
gimp_palette_view_unrealize (GtkWidget *widget)
{
  GimpPaletteView *view = GIMP_PALETTE_VIEW (widget);

  if (view->gc)
    {
      g_object_unref (view->gc);
      view->gc = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static gboolean
gimp_palette_view_expose (GtkWidget      *widget,
                          GdkEventExpose *eevent)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GimpPaletteView *view = GIMP_PALETTE_VIEW (widget);

      GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);

      if (view->selected)
        gimp_palette_view_draw_selected (view, &eevent->area);
    }

  return FALSE;
}

static gboolean
gimp_palette_view_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpPaletteView         *view  = GIMP_PALETTE_VIEW (widget);
  GimpViewRendererPalette *renderer;
  GimpPaletteEntry        *entry = NULL;
  gint                     row, col;

  renderer = GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (view)->renderer);

  col = bevent->x / renderer->cell_width;
  row = bevent->y / renderer->cell_height;

  if (col >= 0 && col < renderer->columns &&
      row >= 0 && row < renderer->rows)
    {
      entry =
        g_list_nth_data (GIMP_PALETTE (GIMP_VIEW (view)->renderer->viewable)->colors,
                         row * renderer->columns + col);
    }

  view->dnd_entry = entry;

  if (! entry || bevent->button == 2)
    return FALSE;

  switch (bevent->button)
    {
    case 1:
      if (bevent->type == GDK_BUTTON_PRESS)
        {
          gimp_palette_view_select_entry (view, entry);
        }
      else if (bevent->type == GDK_2BUTTON_PRESS && entry == view->selected)
        {
          g_signal_emit (view, view_signals[ENTRY_ACTIVATED], 0, entry);
        }
      break;

    case 3:
      if (bevent->type == GDK_BUTTON_PRESS)
        {
          if (entry != view->selected)
            gimp_palette_view_select_entry (view, entry);

          g_signal_emit (view, view_signals[ENTRY_CONTEXT], 0, entry);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_palette_view_set_viewable (GimpView     *view,
                                GimpViewable *old_viewable,
                                GimpViewable *new_viewable)
{
  GimpPaletteView *pal_view = GIMP_PALETTE_VIEW (view);

  pal_view->dnd_entry = NULL;
  pal_view->selected  = NULL;

  if (old_viewable)
    g_signal_handlers_disconnect_by_func (old_viewable,
                                          gimp_palette_view_invalidate,
                                          view);

  GIMP_VIEW_CLASS (parent_class)->set_viewable (view,
                                                old_viewable, new_viewable);

  if (new_viewable)
    g_signal_connect (new_viewable, "invalidate-preview",
                      G_CALLBACK (gimp_palette_view_invalidate),
                      view);
}


/*  public functions  */

void
gimp_palette_view_select_entry (GimpPaletteView  *view,
                                GimpPaletteEntry *entry)
{
  g_return_if_fail (GIMP_IS_PALETTE_VIEW (view));

  if (entry == view->selected)
    return;

  if (view->selected)
    gimp_palette_view_expose_entry (view, view->selected);

  view->selected = entry;

  if (view->selected)
    gimp_palette_view_expose_entry (view, view->selected);

  g_signal_emit (view, view_signals[ENTRY_SELECTED], 0, view->selected);
}


/*  private funcions  */

static void
gimp_palette_view_expose_entry (GimpPaletteView  *view,
                                GimpPaletteEntry *entry)
{
  GimpViewRendererPalette *renderer;
  gint                     row, col;
      GtkWidget               *widget = GTK_WIDGET (view);

  renderer = GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (view)->renderer);

  row = entry->position / renderer->columns;
  col = entry->position % renderer->columns;

  gtk_widget_queue_draw_area (GTK_WIDGET (view),
                              widget->allocation.x + col * renderer->cell_width,
                              widget->allocation.y + row * renderer->cell_height,
                              renderer->cell_width  + 1,
                              renderer->cell_height + 1);
}

static void
gimp_palette_view_draw_selected (GimpPaletteView *pal_view,
                                 GdkRectangle    *area)
{
  GimpView *view = GIMP_VIEW (pal_view);

  if (view->renderer->viewable && pal_view->selected)
    {
      GtkWidget               *widget = GTK_WIDGET (view);
      GimpViewRendererPalette *renderer;
      gint                     row, col;

      renderer = GIMP_VIEW_RENDERER_PALETTE (view->renderer);

      row = pal_view->selected->position / renderer->columns;
      col = pal_view->selected->position % renderer->columns;

      if (area)
        gdk_gc_set_clip_rectangle (pal_view->gc, area);

      gdk_draw_rectangle (widget->window, pal_view->gc,
                          FALSE,
                          widget->allocation.x + col * renderer->cell_width,
                          widget->allocation.y + row * renderer->cell_height,
                          renderer->cell_width,
                          renderer->cell_height);

      if (area)
        gdk_gc_set_clip_rectangle (pal_view->gc, NULL);
    }
}

static void
gimp_palette_view_invalidate (GimpPalette     *palette,
                              GimpPaletteView *view)
{
  view->dnd_entry = NULL;

  if (view->selected && ! g_list_find (palette->colors, view->selected))
    view->selected = NULL;
}
