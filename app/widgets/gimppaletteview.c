/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapaletteview.c
 * Copyright (C) 2005 Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmapalette.h"
#include "core/ligmamarshal.h"

#include "ligmadnd.h"
#include "ligmapaletteview.h"
#include "ligmaviewrendererpalette.h"


enum
{
  ENTRY_CLICKED,
  ENTRY_SELECTED,
  ENTRY_ACTIVATED,
  COLOR_DROPPED,
  LAST_SIGNAL
};


static gboolean ligma_palette_view_draw           (GtkWidget        *widget,
                                                  cairo_t          *cr);
static gboolean ligma_palette_view_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *bevent);
static gboolean ligma_palette_view_key_press      (GtkWidget        *widget,
                                                  GdkEventKey      *kevent);
static gboolean ligma_palette_view_focus          (GtkWidget        *widget,
                                                  GtkDirectionType  direction);
static void     ligma_palette_view_set_viewable   (LigmaView         *view,
                                                  LigmaViewable     *old_viewable,
                                                  LigmaViewable     *new_viewable);
static LigmaPaletteEntry *
                ligma_palette_view_find_entry     (LigmaPaletteView *view,
                                                  gint             x,
                                                  gint             y);
static void     ligma_palette_view_expose_entry   (LigmaPaletteView  *view,
                                                  LigmaPaletteEntry *entry);
static void     ligma_palette_view_invalidate     (LigmaPalette      *palette,
                                                  LigmaPaletteView  *view);
static void     ligma_palette_view_drag_color     (GtkWidget        *widget,
                                                  LigmaRGB          *color,
                                                  gpointer          data);
static void     ligma_palette_view_drop_color     (GtkWidget        *widget,
                                                  gint              x,
                                                  gint              y,
                                                  const LigmaRGB    *color,
                                                  gpointer          data);


G_DEFINE_TYPE (LigmaPaletteView, ligma_palette_view, LIGMA_TYPE_VIEW)

#define parent_class ligma_palette_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_palette_view_class_init (LigmaPaletteViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaViewClass  *view_class   = LIGMA_VIEW_CLASS (klass);

  view_signals[ENTRY_CLICKED] =
    g_signal_new ("entry-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPaletteViewClass, entry_clicked),
                  NULL, NULL,
                  ligma_marshal_VOID__POINTER_ENUM,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  GDK_TYPE_MODIFIER_TYPE);

  view_signals[ENTRY_SELECTED] =
    g_signal_new ("entry-selected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPaletteViewClass, entry_selected),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPaletteViewClass, entry_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  view_signals[COLOR_DROPPED] =
    g_signal_new ("color-dropped",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaPaletteViewClass, color_dropped),
                  NULL, NULL,
                  ligma_marshal_VOID__POINTER_BOXED,
                  G_TYPE_NONE, 2,
                  G_TYPE_POINTER,
                  LIGMA_TYPE_RGB);

  widget_class->draw               = ligma_palette_view_draw;
  widget_class->button_press_event = ligma_palette_view_button_press;
  widget_class->key_press_event    = ligma_palette_view_key_press;
  widget_class->focus              = ligma_palette_view_focus;

  view_class->set_viewable         = ligma_palette_view_set_viewable;
}

static void
ligma_palette_view_init (LigmaPaletteView *view)
{
  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);

  view->selected  = NULL;
  view->dnd_entry = NULL;
}

static gboolean
ligma_palette_view_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
  LigmaPaletteView *pal_view = LIGMA_PALETTE_VIEW (widget);
  LigmaView        *view     = LIGMA_VIEW (widget);
  LigmaPalette     *palette;

  palette = LIGMA_PALETTE (LIGMA_VIEW (view)->renderer->viewable);

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  if (view->renderer->viewable && pal_view->selected)
    {
      LigmaViewRendererPalette *renderer;
      GtkAllocation            allocation;
      gint                     pos, row, col;

      renderer = LIGMA_VIEW_RENDERER_PALETTE (view->renderer);

      gtk_widget_get_allocation (widget, &allocation);

      pos = ligma_palette_get_entry_position (palette, pal_view->selected);

      row = pos / renderer->columns;
      col = pos % renderer->columns;

      cairo_rectangle (cr,
                       col * renderer->cell_width  + 0.5,
                       row * renderer->cell_height + 0.5,
                       renderer->cell_width,
                       renderer->cell_height);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
      cairo_stroke_preserve (cr);

      if (ligma_cairo_set_focus_line_pattern (cr, widget))
        {
          cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
          cairo_stroke (cr);
        }
    }

  return FALSE;
}

static gboolean
ligma_palette_view_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  LigmaPaletteView  *view = LIGMA_PALETTE_VIEW (widget);
  LigmaPaletteEntry *entry;

  if (gtk_widget_get_can_focus (widget) && ! gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  entry = ligma_palette_view_find_entry (view, bevent->x, bevent->y);

  view->dnd_entry = entry;

  if (! entry || bevent->button == 2)
    return TRUE;

  if (bevent->type == GDK_BUTTON_PRESS)
    g_signal_emit (view, view_signals[ENTRY_CLICKED], 0,
                   entry, bevent->state);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      if (entry != view->selected)
        ligma_palette_view_select_entry (view, entry);

      /* Usually the menu is provided by a LigmaEditor.
	   * Make sure it's also run by returning FALSE here */
      return FALSE;
    }
  else if (bevent->button == 1)
    {
      if (bevent->type == GDK_BUTTON_PRESS)
        {
          ligma_palette_view_select_entry (view, entry);
        }
      else if (bevent->type == GDK_2BUTTON_PRESS && entry == view->selected)
        {
          g_signal_emit (view, view_signals[ENTRY_ACTIVATED], 0, entry);
        }
    }

  return TRUE;
}

static gboolean
ligma_palette_view_key_press (GtkWidget   *widget,
                             GdkEventKey *kevent)
{
  LigmaPaletteView *view = LIGMA_PALETTE_VIEW (widget);

  if (view->selected &&
      (kevent->keyval == GDK_KEY_space    ||
       kevent->keyval == GDK_KEY_KP_Space ||
       kevent->keyval == GDK_KEY_Return   ||
       kevent->keyval == GDK_KEY_KP_Enter ||
       kevent->keyval == GDK_KEY_ISO_Enter))
    {
      g_signal_emit (view, view_signals[ENTRY_CLICKED], 0,
                     view->selected, kevent->state);
    }

  return FALSE;
}

static gboolean
ligma_palette_view_focus (GtkWidget        *widget,
                         GtkDirectionType  direction)
{
  LigmaPaletteView *view = LIGMA_PALETTE_VIEW (widget);
  LigmaPalette     *palette;

  palette = LIGMA_PALETTE (LIGMA_VIEW (view)->renderer->viewable);

  if (gtk_widget_get_can_focus (widget) &&
      ! gtk_widget_has_focus (widget))
    {
      gtk_widget_grab_focus (widget);

      if (! view->selected &&
          palette && ligma_palette_get_n_colors (palette) > 0)
        {
          LigmaPaletteEntry *entry = ligma_palette_get_entry (palette, 0);

          ligma_palette_view_select_entry (view, entry);
        }

      return TRUE;
    }

  if (view->selected)
    {
      LigmaViewRendererPalette *renderer;
      gint                     skip = 0;

      renderer = LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (view)->renderer);

      switch (direction)
        {
        case GTK_DIR_UP:
          skip = -renderer->columns;
          break;
        case GTK_DIR_DOWN:
          skip = renderer->columns;
          break;
        case GTK_DIR_LEFT:
          skip = -1;
          break;
        case GTK_DIR_RIGHT:
          skip = 1;
          break;

        case GTK_DIR_TAB_FORWARD:
        case GTK_DIR_TAB_BACKWARD:
          return FALSE;
        }

      if (skip != 0)
        {
          LigmaPaletteEntry *entry;
          LigmaPalette      *palette;
          gint              position;

          palette = LIGMA_PALETTE (LIGMA_VIEW (view)->renderer->viewable);
          position = ligma_palette_get_entry_position (palette, view->selected);
          position += skip;

          entry = ligma_palette_get_entry (palette, position);

          if (entry)
            ligma_palette_view_select_entry (view, entry);
        }

      return TRUE;
    }

  return FALSE;
}

static void
ligma_palette_view_set_viewable (LigmaView     *view,
                                LigmaViewable *old_viewable,
                                LigmaViewable *new_viewable)
{
  LigmaPaletteView *pal_view = LIGMA_PALETTE_VIEW (view);

  pal_view->dnd_entry = NULL;
  ligma_palette_view_select_entry (pal_view, NULL);

  if (old_viewable)
    {
      g_signal_handlers_disconnect_by_func (old_viewable,
                                            ligma_palette_view_invalidate,
                                            view);

      if (! new_viewable)
        {
          ligma_dnd_color_source_remove (GTK_WIDGET (view));
          ligma_dnd_color_dest_remove (GTK_WIDGET (view));
        }
    }

  LIGMA_VIEW_CLASS (parent_class)->set_viewable (view,
                                                old_viewable, new_viewable);

  if (new_viewable)
    {
      g_signal_connect (new_viewable, "invalidate-preview",
                        G_CALLBACK (ligma_palette_view_invalidate),
                        view);

      /*  unset the palette drag handler set by LigmaView  */
      ligma_dnd_viewable_source_remove (GTK_WIDGET (view), LIGMA_TYPE_PALETTE);

      if (! old_viewable)
        {
          ligma_dnd_color_source_add (GTK_WIDGET (view),
                                     ligma_palette_view_drag_color,
                                     view);
          ligma_dnd_color_dest_add (GTK_WIDGET (view),
                                   ligma_palette_view_drop_color,
                                   view);
        }
    }
}


/*  public functions  */

void
ligma_palette_view_select_entry (LigmaPaletteView  *view,
                                LigmaPaletteEntry *entry)
{
  g_return_if_fail (LIGMA_IS_PALETTE_VIEW (view));

  if (entry == view->selected)
    return;

  if (view->selected)
    ligma_palette_view_expose_entry (view, view->selected);

  view->selected = entry;

  if (view->selected)
    ligma_palette_view_expose_entry (view, view->selected);

  g_signal_emit (view, view_signals[ENTRY_SELECTED], 0, view->selected);
}

LigmaPaletteEntry *
ligma_palette_view_get_selected_entry (LigmaPaletteView *view)
{
  g_return_val_if_fail (LIGMA_IS_PALETTE_VIEW (view), NULL);

  return view->selected;
}

void
ligma_palette_view_get_entry_rect (LigmaPaletteView  *view,
                                  LigmaPaletteEntry *entry,
                                  GdkRectangle     *rect)
{
  LigmaViewRendererPalette *renderer;
  LigmaPalette             *palette;
  GtkAllocation            allocation;
  gint                     pos, row, col;

  g_return_if_fail (LIGMA_IS_PALETTE_VIEW (view));
  g_return_if_fail (entry);
  g_return_if_fail (rect);

  gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);

  renderer = LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (view)->renderer);
  palette = LIGMA_PALETTE (LIGMA_VIEW_RENDERER (renderer)->viewable);
  pos = ligma_palette_get_entry_position (palette, entry);
  row = pos / renderer->columns;
  col = pos % renderer->columns;

  rect->x = allocation.x + col * renderer->cell_width;
  rect->y = allocation.y + row * renderer->cell_height;
  rect->width = renderer->cell_width;
  rect->height = renderer->cell_height;
}


/*  private functions  */

static LigmaPaletteEntry *
ligma_palette_view_find_entry (LigmaPaletteView *view,
                              gint             x,
                              gint             y)
{
  LigmaPalette             *palette;
  LigmaViewRendererPalette *renderer;
  LigmaPaletteEntry        *entry = NULL;
  gint                     col, row;

  renderer = LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (view)->renderer);
  palette  = LIGMA_PALETTE (LIGMA_VIEW_RENDERER (renderer)->viewable);

  if (! palette || ! ligma_palette_get_n_colors (palette))
    return NULL;

  col = x / renderer->cell_width;
  row = y / renderer->cell_height;

  if (col >= 0 && col < renderer->columns &&
      row >= 0 && row < renderer->rows)
    {
      entry = ligma_palette_get_entry (palette,
                                      row * renderer->columns + col);
    }

  return entry;
}

static void
ligma_palette_view_expose_entry (LigmaPaletteView  *view,
                                LigmaPaletteEntry *entry)
{
  LigmaViewRendererPalette *renderer;
  gint                     pos, row, col;
  GtkWidget               *widget = GTK_WIDGET (view);
  GtkAllocation            allocation;
  LigmaPalette             *palette;

  renderer = LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (view)->renderer);
  palette = LIGMA_PALETTE (LIGMA_VIEW_RENDERER (renderer)->viewable);

  gtk_widget_get_allocation (widget, &allocation);

  pos = ligma_palette_get_entry_position (palette, entry);
  row = pos / renderer->columns;
  col = pos % renderer->columns;

  gtk_widget_queue_draw_area (GTK_WIDGET (view),
                              allocation.x + col * renderer->cell_width,
                              allocation.y + row * renderer->cell_height,
                              renderer->cell_width  + 1,
                              renderer->cell_height + 1);
}

static void
ligma_palette_view_invalidate (LigmaPalette     *palette,
                              LigmaPaletteView *view)
{
  view->dnd_entry = NULL;

  if (view->selected &&
      ! g_list_find (ligma_palette_get_colors (palette), view->selected))
    {
      ligma_palette_view_select_entry (view, NULL);
    }
}

static void
ligma_palette_view_drag_color (GtkWidget *widget,
                              LigmaRGB   *color,
                              gpointer   data)
{
  LigmaPaletteView *view = LIGMA_PALETTE_VIEW (data);

  if (view->dnd_entry)
    *color = view->dnd_entry->color;
  else
    ligma_rgba_set (color, 0.0, 0.0, 0.0, 1.0);
}

static void
ligma_palette_view_drop_color (GtkWidget     *widget,
                              gint           x,
                              gint           y,
                              const LigmaRGB *color,
                              gpointer       data)
{
  LigmaPaletteView  *view = LIGMA_PALETTE_VIEW (data);
  LigmaPaletteEntry *entry;

  entry = ligma_palette_view_find_entry (view, x, y);

  g_signal_emit (view, view_signals[COLOR_DROPPED], 0,
                 entry, color);
}
