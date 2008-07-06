/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcombotagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpfilteredcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"
#include "core/gimptagged.h"

#include "gimptagentry.h"
#include "gimpcombotagentry.h"


static gboolean gimp_combo_tag_entry_expose_event      (GtkWidget         *widget,
                                                        GdkEventExpose    *event);
static void     gimp_combo_tag_entry_style_set         (GtkWidget         *widget,
                                                        GtkStyle          *previous_style,
                                                        GimpComboTagEntry *combo_entry);
static gboolean gimp_combo_tag_entry_focus_in_out      (GtkWidget         *widget,
                                                        GdkEventFocus     *event,
                                                        GimpComboTagEntry *combo_entry);


G_DEFINE_TYPE (GimpComboTagEntry, gimp_combo_tag_entry, GTK_TYPE_ALIGNMENT);

#define parent_class gimp_combo_tag_entry_parent_class


static void
gimp_combo_tag_entry_class_init (GimpComboTagEntryClass *klass)
{
  GtkWidgetClass       *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->expose_event = gimp_combo_tag_entry_expose_event;
}

static void
gimp_combo_tag_entry_init (GimpComboTagEntry *combo_entry)
{
  combo_entry->tag_entry        = NULL;
  combo_entry->focus_width      = 0;
  combo_entry->interior_focus   = FALSE;
}

GtkWidget *
gimp_combo_tag_entry_new (GimpTagEntry         *tag_entry)
{
  GimpComboTagEntry            *combo_entry;

  combo_entry = g_object_new (GIMP_TYPE_COMBO_TAG_ENTRY, NULL);
  combo_entry->tag_entry = GTK_WIDGET (tag_entry);

  gtk_entry_set_has_frame (GTK_ENTRY (tag_entry), FALSE);
  gtk_widget_show (GTK_WIDGET (tag_entry));
  gtk_container_add (GTK_CONTAINER (combo_entry), GTK_WIDGET (tag_entry));

  g_signal_connect (combo_entry->tag_entry, "style-set",
                    G_CALLBACK (gimp_combo_tag_entry_style_set),
                    combo_entry);
  g_signal_connect (combo_entry->tag_entry, "focus-in-event",
                    G_CALLBACK (gimp_combo_tag_entry_focus_in_out),
                    combo_entry);
  g_signal_connect (combo_entry->tag_entry, "focus-out-event",
                    G_CALLBACK (gimp_combo_tag_entry_focus_in_out),
                    combo_entry);

  return GTK_WIDGET (combo_entry);
}

static gboolean
gimp_combo_tag_entry_expose_event (GtkWidget         *widget,
                                   GdkEventExpose    *event)
{
  GdkGC                *gc;
  GimpComboTagEntry    *combo_entry;
  GtkWidget            *tag_entry;
  GtkStyle             *style;
  GtkAllocation        *allocation;
  GdkRectangle          client_area;
  GdkRectangle          shadow_area;

  combo_entry = GIMP_COMBO_TAG_ENTRY (widget);
  tag_entry   = combo_entry->tag_entry;
  style       = gtk_widget_get_style (tag_entry);
  allocation = &widget->allocation;

  gc = gdk_gc_new (widget->window);

  client_area.x         = widget->allocation.x;
  client_area.y         = widget->allocation.y;
  client_area.width     = widget->allocation.width;
  client_area.height    = widget->allocation.height;

  shadow_area = client_area;
  if (GTK_WIDGET_HAS_FOCUS (tag_entry)
      && ! combo_entry->interior_focus)
    {
      shadow_area.x      += combo_entry->interior_focus;
      shadow_area.y      += combo_entry->interior_focus;
      shadow_area.width  -= combo_entry->interior_focus * 2;
      shadow_area.height -= combo_entry->interior_focus * 2;
    }

  gtk_paint_flat_box (style, widget->window,
                      GTK_WIDGET_STATE (tag_entry), GTK_SHADOW_NONE,
                      &event->area, tag_entry, "entry_bg",
                      client_area.x, client_area.y,
                      client_area.width, client_area.height);
  gtk_paint_shadow (style, widget->window,
                    GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN,
                    &event->area, tag_entry, "entry",
                    shadow_area.x, shadow_area.y,
                    shadow_area.width, shadow_area.height);
  if (GTK_WIDGET_HAS_FOCUS (tag_entry)
      && ! combo_entry->interior_focus)
    {
      gtk_paint_focus (widget->style, widget->window,
                       GTK_WIDGET_STATE (tag_entry),
                       &event->area, tag_entry, "entry",
                       client_area.x, client_area.y,
                       client_area.width, client_area.width);
    }

  gtk_paint_arrow (style, widget->window, GTK_STATE_NORMAL,
                  GTK_SHADOW_NONE, NULL, NULL, NULL,
                  GTK_ARROW_DOWN, TRUE,
                  shadow_area.x + shadow_area.width - 14,
                  shadow_area.y + shadow_area.height / 2 - 4, 8, 8);

  g_object_unref (gc);

  return FALSE;
}

static void
gimp_combo_tag_entry_style_set (GtkWidget              *widget,
                                GtkStyle               *previous_style,
                                GimpComboTagEntry      *combo_entry)
{
  GtkStyle                     *style;
  gint                          xmargin;
  gint                          ymargin;

  gtk_widget_style_get (combo_entry->tag_entry,
                        "focus-line-width", &combo_entry->focus_width,
                        "interior-focus", &combo_entry->interior_focus,
                        NULL);

  style = gtk_widget_get_style (combo_entry->tag_entry);
  xmargin = style->xthickness;
  if (! combo_entry->interior_focus)
    {
      xmargin += combo_entry->focus_width;
    }
  ymargin = style->ythickness;
  if (! combo_entry->interior_focus)
    {
      ymargin += combo_entry->focus_width;
    }

  gtk_alignment_set_padding (GTK_ALIGNMENT (combo_entry),
                             ymargin, ymargin, xmargin, xmargin + 16);
}

static gboolean
gimp_combo_tag_entry_focus_in_out (GtkWidget               *widget,
                                   GdkEventFocus           *event,
                                   GimpComboTagEntry       *combo_entry)
{
  gtk_widget_queue_draw (GTK_WIDGET (combo_entry));

  return FALSE;
}

