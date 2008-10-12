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

static void     gimp_combo_tag_entry_dispose           (GObject                *object);
static gboolean gimp_combo_tag_entry_expose_event      (GtkWidget              *widget,
                                                        GdkEventExpose         *event,
                                                        gpointer                user_data);
static gboolean gimp_combo_tag_entry_event             (GtkWidget              *widget,
                                                        GdkEvent               *event,
                                                        gpointer                user_data);
static void     gimp_combo_tag_entry_style_set         (GtkWidget              *widget,
                                                        GtkStyle               *previous_style);

static void     gimp_combo_tag_entry_popup_list        (GimpComboTagEntry      *combo_entry);
static void     gimp_combo_tag_entry_popup_destroy     (GtkObject              *object,
                                                        GimpComboTagEntry      *combo_entry);

static void     gimp_combo_tag_entry_tag_count_changed (GimpFilteredContainer  *container,
                                                        gint                    tag_count,
                                                        GimpComboTagEntry      *combo_entry);

static void     gimp_combo_tag_entry_get_arrow_rect    (GimpComboTagEntry      *combo_entry,
                                                        GdkRectangle           *arrow_rect);


G_DEFINE_TYPE (GimpComboTagEntry, gimp_combo_tag_entry, GIMP_TYPE_TAG_ENTRY);

#define parent_class gimp_combo_tag_entry_parent_class


static void
gimp_combo_tag_entry_class_init (GimpComboTagEntryClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose         = gimp_combo_tag_entry_dispose;

  widget_class->style_set       = gimp_combo_tag_entry_style_set;
}

static void
gimp_combo_tag_entry_init (GimpComboTagEntry *combo_entry)
{
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

  if (combo_entry->filtered_container)
    {
      g_signal_handlers_disconnect_by_func (combo_entry->filtered_container,
                                            G_CALLBACK (gimp_combo_tag_entry_tag_count_changed),
                                            combo_entry);

      g_object_unref (combo_entry->filtered_container);
      combo_entry->filtered_container = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
 * gimp_combo_tag_entry_new:
 * @filtered_container: a filtered container to be used.
 * @mode:               tag entry mode to work in.
 *
 * Creates a new #GimpComboTagEntry widget which extends #GimpTagEntry by
 * adding ability to pick tags using popup window (similar to combo box).
 *
 * Return value: a new #GimpComboTagEntry widget.
 **/
GtkWidget *
gimp_combo_tag_entry_new (GimpFilteredContainer        *filtered_container,
                          GimpTagEntryMode              mode)
{
  GimpComboTagEntry            *combo_entry;
  GtkBorder                     border;

  g_return_val_if_fail (GIMP_IS_FILTERED_CONTAINER (filtered_container), NULL);

  combo_entry = g_object_new (GIMP_TYPE_COMBO_TAG_ENTRY,
                              "filtered-container", filtered_container,
                              "tag-entry-mode", mode,
                              NULL);
  combo_entry->filtered_container = filtered_container;
  g_object_ref (combo_entry->filtered_container);

  gtk_widget_add_events (GTK_WIDGET (combo_entry),
                         GDK_BUTTON_PRESS_MASK);

  if (gtk_widget_get_direction (GTK_WIDGET (combo_entry)) == GTK_TEXT_DIR_RTL)
    {
      border.left   = 18;
      border.right  = 2;
    }
  else
    {
      border.left   = 2;
      border.right  = 18;
    }
  border.top    = 2;
  border.bottom = 2;
  gtk_entry_set_inner_border (GTK_ENTRY (combo_entry), &border);

  g_signal_connect_after (combo_entry, "expose-event",
                          G_CALLBACK (gimp_combo_tag_entry_expose_event),
                          NULL);
  g_signal_connect (combo_entry, "style-set",
                    G_CALLBACK (gimp_combo_tag_entry_style_set),
                    NULL);
  g_signal_connect (combo_entry->filtered_container,
                    "tag-count-changed",
                    G_CALLBACK (gimp_combo_tag_entry_tag_count_changed),
                    combo_entry);

  return GTK_WIDGET (combo_entry);
}

static gboolean
gimp_combo_tag_entry_expose_event (GtkWidget         *widget,
                                   GdkEventExpose    *event,
                                   gpointer           user_data)
{
  GimpComboTagEntry    *combo_entry = GIMP_COMBO_TAG_ENTRY (widget);
  GdkRectangle          arrow_rect;
  gint                  tag_count;
  gint                  window_width;
  gint                  window_height;
  GtkStateType          arrow_state;

  if (widget->window == event->window)
    {
      return FALSE;
    }

  gimp_combo_tag_entry_get_arrow_rect (combo_entry, &arrow_rect);
  tag_count = gimp_filtered_container_get_tag_count (combo_entry->filtered_container);

  gdk_drawable_get_size (GDK_DRAWABLE (event->window), &window_width, &window_height);
  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      gdk_draw_rectangle (event->window, widget->style->base_gc[widget->state],
                          TRUE, 0, 0, 14, window_height);
    }
  else
    {
      gdk_draw_rectangle (event->window, widget->style->base_gc[widget->state],
                          TRUE, window_width - 14, 0, 14, window_height);
    }

  if (tag_count > 0
      && ! GIMP_TAG_ENTRY (combo_entry)->has_invalid_tags)
    {
      arrow_state = GTK_STATE_NORMAL;
    }
  else
    {
      arrow_state = GTK_STATE_INSENSITIVE;
    }

  gtk_paint_arrow (widget->style,
                   event->window, arrow_state,
                   GTK_SHADOW_NONE, NULL, NULL, NULL,
                   GTK_ARROW_DOWN, TRUE,
                   arrow_rect.x + arrow_rect.width / 2 - 4,
                   arrow_rect.y + arrow_rect.height / 2 - 4, 8, 8);

  return FALSE;
}

static gboolean
gimp_combo_tag_entry_event (GtkWidget          *widget,
                            GdkEvent           *event,
                            gpointer            user_data)
{
  GimpComboTagEntry    *combo_entry = GIMP_COMBO_TAG_ENTRY (widget);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;
      GdkRectangle      arrow_rect;

      button_event = (GdkEventButton *) event;
      x = button_event->x;
      y = button_event->y;

      gimp_combo_tag_entry_get_arrow_rect (combo_entry, &arrow_rect);
      if (x > arrow_rect.x
          && y > arrow_rect.y
          && x < arrow_rect.x + arrow_rect.width
          && y < arrow_rect.y + arrow_rect.height)
        {
          if (! combo_entry->popup)
            {
              gimp_combo_tag_entry_popup_list (combo_entry);
            }
          else
            {
              gtk_widget_destroy (combo_entry->popup);
            }

          return TRUE;
        }
    }

  return FALSE;
}

static void
gimp_combo_tag_entry_popup_list (GimpComboTagEntry             *combo_entry)
{
  gint          tag_count;

  tag_count = gimp_filtered_container_get_tag_count (combo_entry->filtered_container);
  if (tag_count > 0
      && ! GIMP_TAG_ENTRY (combo_entry)->has_invalid_tags)
    {
      combo_entry->popup = gimp_tag_popup_new (combo_entry);
      g_signal_connect (combo_entry->popup, "destroy",
                        G_CALLBACK (gimp_combo_tag_entry_popup_destroy),
                        combo_entry);
    }
}


static void
gimp_combo_tag_entry_popup_destroy     (GtkObject         *object,
                                        GimpComboTagEntry *combo_entry)
{
  combo_entry->popup = NULL;
  gtk_widget_grab_focus (GTK_WIDGET (combo_entry));
}

static void
gimp_combo_tag_entry_tag_count_changed (GimpFilteredContainer  *container,
                                        gint                    tag_count,
                                        GimpComboTagEntry      *combo_entry)
{
  gtk_widget_queue_draw (GTK_WIDGET (combo_entry));
}

static void
gimp_combo_tag_entry_style_set (GtkWidget              *widget,
                                GtkStyle               *previous_style)
{
  GimpComboTagEntry            *combo_entry = GIMP_COMBO_TAG_ENTRY (widget);
  GtkStyle                     *style;
  GdkColor                      color;
  PangoAttribute               *attribute;

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

  if (GTK_WIDGET_CLASS (parent_class))
    {
      GTK_WIDGET_CLASS (parent_class)->style_set (widget, previous_style);
    }
}

static void
gimp_combo_tag_entry_get_arrow_rect    (GimpComboTagEntry      *combo_entry,
                                        GdkRectangle           *arrow_rect)
{
  GtkWidget    *widget = GTK_WIDGET (combo_entry);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      arrow_rect->x = widget->style->xthickness;
    }
  else
    {
      arrow_rect->x = widget->allocation.width - 16 - widget->style->xthickness * 2;
    }
  arrow_rect->y = 0;
  arrow_rect->width = 12;
  arrow_rect->height = widget->allocation.height - widget->style->ythickness * 2;
}

