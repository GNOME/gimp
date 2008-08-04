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
#include "core/gimptag.h"
#include "core/gimptagged.h"

#include "gimptagentry.h"
#include "gimptagpopup.h"
#include "gimpcombotagentry.h"

static void     gimp_combo_tag_entry_dispose           (GObject           *object);
static gboolean gimp_combo_tag_entry_expose_event      (GtkWidget         *widget,
                                                        GdkEventExpose    *event,
                                                        GimpComboTagEntry *combo_entry);
static void     gimp_combo_tag_entry_style_set         (GtkWidget         *widget,
                                                        GtkStyle          *previous_style,
                                                        GimpComboTagEntry *combo_entry);
static gboolean gimp_combo_tag_entry_focus_in_out      (GtkWidget         *widget,
                                                        GdkEventFocus     *event,
                                                        GimpComboTagEntry *combo_entry);
static gboolean gimp_combo_tag_entry_event             (GtkWidget         *widget,
                                                        GdkEvent          *event,
                                                        gpointer           user_data);
static void     gimp_combo_tag_entry_popup_list        (GimpComboTagEntry *combo_entry);


G_DEFINE_TYPE (GimpComboTagEntry, gimp_combo_tag_entry, GTK_TYPE_EVENT_BOX);

#define parent_class gimp_combo_tag_entry_parent_class


static void
gimp_combo_tag_entry_class_init (GimpComboTagEntryClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose         = gimp_combo_tag_entry_dispose;
}

static void
gimp_combo_tag_entry_init (GimpComboTagEntry *combo_entry)
{
  combo_entry->tag_entry            = NULL;
  combo_entry->alignment            = NULL;
  combo_entry->popup                = NULL;
  combo_entry->focus_width          = 0;
  combo_entry->interior_focus       = FALSE;
  combo_entry->normal_item_attr     = NULL;
  combo_entry->selected_item_attr   = NULL;
  combo_entry->insensitive_item_attr = NULL;

  g_signal_connect (combo_entry, "event",
                    G_CALLBACK (gimp_combo_tag_entry_event),
                    NULL);
}

static void
gimp_combo_tag_entry_dispose (GObject           *object)
{
  GimpComboTagEntry            *combo_entry = GIMP_COMBO_TAG_ENTRY (object);

  if (combo_entry->normal_item_attr)
    {
      pango_attr_list_unref (combo_entry->normal_item_attr);
      combo_entry->normal_item_attr = NULL;
    }
  if (combo_entry->selected_item_attr)
    {
      pango_attr_list_unref (combo_entry->selected_item_attr);
      combo_entry->selected_item_attr = NULL;
    }
  if (combo_entry->insensitive_item_attr)
    {
      pango_attr_list_unref (combo_entry->insensitive_item_attr);
      combo_entry->insensitive_item_attr = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
gimp_combo_tag_entry_new (GimpTagEntry         *tag_entry)
{
  GimpComboTagEntry            *combo_entry;

  combo_entry = g_object_new (GIMP_TYPE_COMBO_TAG_ENTRY, NULL);
  combo_entry->tag_entry = GTK_WIDGET (tag_entry);

  gtk_widget_add_events (GTK_WIDGET (combo_entry),
                         GDK_BUTTON_PRESS_MASK);

  combo_entry->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_widget_show (combo_entry->alignment);
  gtk_container_add (GTK_CONTAINER (combo_entry), combo_entry->alignment);

  g_signal_connect (combo_entry->alignment, "expose-event",
                    G_CALLBACK (gimp_combo_tag_entry_expose_event),
                    combo_entry);

  gtk_entry_set_has_frame (GTK_ENTRY (tag_entry), FALSE);
  gtk_widget_show (GTK_WIDGET (tag_entry));
  gtk_container_add (GTK_CONTAINER (combo_entry->alignment),
                     GTK_WIDGET (tag_entry));

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
                                   GdkEventExpose    *event,
                                   GimpComboTagEntry *combo_entry)
{
  GdkGC                *gc;
  GtkWidget            *tag_entry;
  GtkStyle             *style;
  GtkAllocation        *allocation;
  GdkRectangle          client_area;
  GdkRectangle          shadow_area;

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

  gtk_paint_arrow (style, widget->window, GTK_WIDGET_STATE (tag_entry),
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
  GdkColor                      color;
  PangoAttribute               *attribute;

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

  gtk_alignment_set_padding (GTK_ALIGNMENT (combo_entry->alignment),
                             ymargin, ymargin, xmargin, xmargin + 16);

  style = widget->style;
  if (combo_entry->normal_item_attr)
    {
      pango_attr_list_unref (combo_entry->normal_item_attr);
    }
  combo_entry->normal_item_attr = pango_attr_list_new ();
  if (style->font_desc)
    {
      attribute = pango_attr_font_desc_new (style->font_desc);
      pango_attr_list_insert (combo_entry->normal_item_attr, attribute);
    }
  color = style->text[GTK_STATE_NORMAL];
  attribute = pango_attr_foreground_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->normal_item_attr, attribute);

  if (combo_entry->selected_item_attr)
    {
      pango_attr_list_unref (combo_entry->selected_item_attr);
    }
  combo_entry->selected_item_attr = pango_attr_list_copy (combo_entry->normal_item_attr);
  color = style->text[GTK_STATE_SELECTED];
  attribute = pango_attr_foreground_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->selected_item_attr, attribute);
  color = style->base[GTK_STATE_SELECTED];
  attribute = pango_attr_background_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->selected_item_attr, attribute);

  if (combo_entry->insensitive_item_attr)
    {
      pango_attr_list_unref (combo_entry->insensitive_item_attr);
    }
  combo_entry->insensitive_item_attr = pango_attr_list_copy (combo_entry->normal_item_attr);
  color = style->text[GTK_STATE_INSENSITIVE];
  attribute = pango_attr_foreground_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->insensitive_item_attr, attribute);
  color = style->base[GTK_STATE_INSENSITIVE];
  attribute = pango_attr_background_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->insensitive_item_attr, attribute);


  combo_entry->selected_item_color = style->base[GTK_STATE_SELECTED];
}

static gboolean
gimp_combo_tag_entry_focus_in_out (GtkWidget               *widget,
                                   GdkEventFocus           *event,
                                   GimpComboTagEntry       *combo_entry)
{
  gtk_widget_queue_draw (GTK_WIDGET (combo_entry));

  return FALSE;
}

static gboolean
gimp_combo_tag_entry_event (GtkWidget          *widget,
                            GdkEvent           *event,
                            gpointer            user_data)
{
  GimpComboTagEntry            *combo_entry;

  combo_entry = GIMP_COMBO_TAG_ENTRY (widget);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;
      guint             padding_top;
      guint             padding_bottom;
      guint             padding_left;
      guint             padding_right;

      button_event = (GdkEventButton *) event;
      x = button_event->x;
      y = button_event->y;

      gtk_alignment_get_padding (GTK_ALIGNMENT (combo_entry->alignment),
                                 &padding_top, &padding_bottom,
                                 &padding_left, &padding_right);
      if (x > widget->allocation.width - padding_right
          && y > padding_top
          && x < widget->allocation.width - padding_left
          && y < widget->allocation.height - padding_bottom)
        {
          if (! combo_entry->popup)
            {
              gimp_combo_tag_entry_popup_list (combo_entry);
            }
          else
            {
              gtk_widget_destroy (combo_entry->popup);
            }
        }
    }

  return FALSE;
}

static void
gimp_combo_tag_entry_popup_list (GimpComboTagEntry             *combo_entry)
{
  combo_entry->popup = gimp_tag_popup_new (combo_entry);
}

