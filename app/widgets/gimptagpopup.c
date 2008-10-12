/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagentry.c
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

#include <stdlib.h>
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

#include "gimp-intl.h"

#define MENU_SCROLL_STEP1               8
#define MENU_SCROLL_STEP2               15
#define MENU_SCROLL_FAST_ZONE           8
#define MENU_SCROLL_TIMEOUT1            50
#define MENU_SCROLL_TIMEOUT2            20

#define GIMP_TAG_POPUP_MARGIN           5

static void     gimp_tag_popup_dispose                 (GObject           *object);

static gboolean gimp_tag_popup_border_expose           (GtkWidget          *widget,
                                                        GdkEventExpose     *event,
                                                        GimpTagPopup       *tag_popup);
static gboolean gimp_tag_popup_list_expose             (GtkWidget          *widget,
                                                        GdkEventExpose     *event,
                                                        GimpTagPopup       *tag_popup);
static gboolean gimp_tag_popup_border_event            (GtkWidget          *widget,
                                                        GdkEvent           *event,
                                                        gpointer            user_data);
static gboolean gimp_tag_popup_list_event              (GtkWidget          *widget,
                                                        GdkEvent           *event,
                                                        GimpTagPopup       *tag_popup);
static void     gimp_tag_popup_toggle_tag              (GimpTagPopup       *tag_popup,
                                                        PopupTagData       *tag_data);
static void     gimp_tag_popup_check_can_toggle        (GimpTagged         *tagged,
                                                        GimpTagPopup       *tag_popup);
static gint     gimp_tag_popup_layout_tags             (GimpTagPopup       *tag_popup,
                                                        gint                width);
static void     gimp_tag_popup_do_timeout_scroll       (GimpTagPopup       *tag_popup,
                                                        gboolean            touchscreen_mode);
static gboolean gimp_tag_popup_scroll_timeout          (gpointer            data);
static void     gimp_tag_popup_remove_scroll_timeout   (GimpTagPopup       *tag_popup);
static gboolean gimp_tag_popup_scroll_timeout_initial  (gpointer            data);
static void     gimp_tag_popup_start_scrolling         (GimpTagPopup       *tag_popup);
static void     gimp_tag_popup_stop_scrolling          (GimpTagPopup       *tag_popup);
static void     gimp_tag_popup_scroll_by               (GimpTagPopup       *tag_popup,
                                                        gint                step);
static void     gimp_tag_popup_handle_scrolling        (GimpTagPopup       *tag_popup,
                                                        gint                x,
                                                        gint                y,
                                                        gboolean            enter,
                                                        gboolean            motion);

static gboolean gimp_tag_popup_button_scroll           (GimpTagPopup       *tag_popup,
                                                        GdkEventButton     *event);

static void     get_arrows_visible_area                (GimpTagPopup       *combo_entry,
                                                        GdkRectangle       *border,
                                                        GdkRectangle       *upper,
                                                        GdkRectangle       *lower,
                                                        gint               *arrow_space);
static void     get_arrows_sensitive_area              (GimpTagPopup       *tag_popup,
                                                        GdkRectangle       *upper,
                                                        GdkRectangle       *lower);


G_DEFINE_TYPE (GimpTagPopup, gimp_tag_popup, GTK_TYPE_WINDOW);

#define parent_class gimp_tag_popup_parent_class


static void
gimp_tag_popup_class_init (GimpTagPopupClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose         = gimp_tag_popup_dispose;
}

static void
gimp_tag_popup_init (GimpTagPopup        *tag_popup)
{
}

static void
gimp_tag_popup_dispose (GObject           *object)
{
  GimpTagPopup         *tag_popup = GIMP_TAG_POPUP (object);

  gimp_tag_popup_remove_scroll_timeout (tag_popup);

  tag_popup->combo_entry = NULL;

  if (tag_popup->layout)
    {
      g_object_unref (tag_popup->layout);
      tag_popup->layout = NULL;
    }

  if (tag_popup->context)
    {
      g_object_unref (tag_popup->context);
      tag_popup->context = NULL;
    }

  if (tag_popup->close_rectangles)
    {
      g_list_foreach (tag_popup->close_rectangles, (GFunc) g_free, NULL);
      g_list_free (tag_popup->close_rectangles);
      tag_popup->close_rectangles = NULL;
    }

  g_free (tag_popup->tag_data);
  tag_popup->tag_data = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/**
 * gimp_tag_popup_new:
 * @combo_entry:        #GimpComboTagEntry which is owner of the popup
 *                      window.
 *
 * Tag popup widget is only useful for for #GimpComboTagEntry and
 * should not be used elsewhere.
 *
 * Return value: a newly created #GimpTagPopup widget.
 **/
GtkWidget *
gimp_tag_popup_new (GimpComboTagEntry             *combo_entry)
{
  GimpTagPopup         *popup;
  GtkWidget            *alignment;
  GtkWidget            *drawing_area;
  GtkWidget            *frame;
  gint                  x;
  gint                  y;
  gint                  width;
  gint                  height;
  gint                  popup_height;
  GHashTable           *tag_hash;
  GList                *tag_list;
  GList                *tag_iterator;
  gint                  i;
  gint                  j;
  GdkGrabStatus         grab_status;
  gint                  max_height;
  gint                  screen_height;
  gchar               **current_tags;
  gint                  current_count;
  const gchar          *list_tag;
  GdkRectangle          popup_rects[2]; /* variants of popup placement */
  GdkRectangle          popup_rect; /* best popup rect in screen coordinates */

  g_return_val_if_fail (GIMP_IS_COMBO_TAG_ENTRY (combo_entry), NULL);

  popup = g_object_new (GIMP_TYPE_TAG_POPUP,
                        "type", GTK_WINDOW_POPUP,
                        NULL);
  gtk_widget_add_events (GTK_WIDGET (popup),
                         GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_KEY_RELEASE_MASK
                         | GDK_SCROLL_MASK);
  gtk_window_set_screen (GTK_WINDOW (popup),
                         gtk_widget_get_screen (GTK_WIDGET (combo_entry)));

  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (popup), frame);

  alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (drawing_area),
                         GDK_BUTTON_PRESS_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         | GDK_POINTER_MOTION_MASK);
  gtk_container_add (GTK_CONTAINER (alignment), drawing_area);

  popup->combo_entry          = combo_entry;
  popup->alignment            = alignment;
  popup->drawing_area         = drawing_area;
  popup->context              = gtk_widget_create_pango_context (GTK_WIDGET (popup));
  popup->layout               = pango_layout_new (popup->context);
  popup->prelight             = NULL;
  popup->upper_arrow_state    = GTK_STATE_NORMAL;
  popup->lower_arrow_state    = GTK_STATE_NORMAL;
  gtk_widget_style_get (GTK_WIDGET (popup),
                        "scroll-arrow-vlength", &popup->scroll_arrow_height,
                        NULL);

  pango_layout_set_attributes (popup->layout, combo_entry->normal_item_attr);


  current_tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (combo_entry));
  current_count = g_strv_length (current_tags);

  tag_hash = combo_entry->filtered_container->tag_ref_counts;
  tag_list = g_hash_table_get_keys (tag_hash);
  tag_list = g_list_sort (tag_list, gimp_tag_compare_func);
  popup->tag_count = g_list_length (tag_list);
  popup->tag_data = g_malloc (sizeof (PopupTagData) * popup->tag_count);
  tag_iterator = tag_list;
  for (i = 0; i < popup->tag_count; i++)
    {
      popup->tag_data[i].tag = GIMP_TAG (tag_iterator->data);
      popup->tag_data[i].state = GTK_STATE_NORMAL;
      list_tag = gimp_tag_get_name (popup->tag_data[i].tag);
      for (j = 0; j < current_count; j++)
        {
          if (! strcmp (current_tags[j], list_tag))
            {
              popup->tag_data[i].state = GTK_STATE_SELECTED;
              break;
            }
        }
      tag_iterator = g_list_next (tag_iterator);
    }
  g_list_free (tag_list);
  g_strfreev (current_tags);

  if (GIMP_TAG_ENTRY (combo_entry)->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      for (i = 0; i < popup->tag_count; i++)
        {
          if (popup->tag_data[i].state != GTK_STATE_SELECTED)
            {
              popup->tag_data[i].state = GTK_STATE_INSENSITIVE;
            }
        }
      gimp_container_foreach (GIMP_CONTAINER (combo_entry->filtered_container),
                              (GFunc) gimp_tag_popup_check_can_toggle, popup);
    }

  width = GTK_WIDGET (combo_entry)->allocation.width - frame->style->xthickness * 2;
  height = gimp_tag_popup_layout_tags (popup, width);
  gdk_window_get_origin (GTK_WIDGET (combo_entry)->window, &x, &y);
  max_height = GTK_WIDGET (combo_entry)->allocation.height * 7;
  screen_height = gdk_screen_get_height (gtk_widget_get_screen (GTK_WIDGET (combo_entry)));
  height += frame->style->ythickness * 2;
  popup_height = height;
  popup_rects[0].x = x;
  popup_rects[0].y = 0;
  popup_rects[0].width = GTK_WIDGET (combo_entry)->allocation.width;
  popup_rects[0].height = y + GTK_WIDGET (combo_entry)->allocation.height;
  popup_rects[1].x = popup_rects[0].x;
  popup_rects[1].y = y;
  popup_rects[1].width = popup_rects[0].width;
  popup_rects[1].height = screen_height - popup_rects[0].height;
  if (popup_rects[0].height >= popup_height)
    {
      popup_rect = popup_rects[0];
      popup_rect.y += popup_rects[0].height - popup_height;
      popup_rect.height = popup_height;
    }
  else if (popup_rects[1].height >= popup_height)
    {
      popup_rect = popup_rects[1];
      popup_rect.height = popup_height;
    }
  else
    {
      if (popup_rects[0].height >= popup_rects[1].height)
        {
          popup_rect = popup_rects[0];
          popup_rect.y += popup->scroll_arrow_height + frame->style->ythickness;
        }
      else
        {
          popup_rect = popup_rects[1];
          popup_rect.y -= popup->scroll_arrow_height + frame->style->ythickness;
        }

      popup->arrows_visible = TRUE;
      popup->upper_arrow_state = GTK_STATE_INSENSITIVE;
      gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
                                 popup->scroll_arrow_height + 2,
                                 popup->scroll_arrow_height + 2, 0, 0);
      popup_height              = popup_rect.height - popup->scroll_arrow_height * 2 + 4;
      popup->scroll_height = height - popup_rect.height;
      popup->scroll_y      = 0;
      popup->scroll_step   = 0;
    }

  drawing_area->requisition.width = width;
  drawing_area->requisition.height = popup_height;

  gtk_window_move (GTK_WINDOW (popup), popup_rect.x, popup_rect.y);
  gtk_window_resize (GTK_WINDOW (popup), popup_rect.width, popup_rect.height);

  gtk_widget_show_all (GTK_WIDGET (popup));

  g_signal_connect (alignment, "expose-event",
                    G_CALLBACK (gimp_tag_popup_border_expose),
                    popup);
  g_signal_connect (popup, "event",
                    G_CALLBACK (gimp_tag_popup_border_event),
                    NULL);
  g_signal_connect (drawing_area, "expose-event",
                    G_CALLBACK (gimp_tag_popup_list_expose),
                    popup);
  g_signal_connect (drawing_area, "event",
                    G_CALLBACK (gimp_tag_popup_list_event),
                    popup);

  gtk_grab_add (GTK_WIDGET (popup));
  gtk_widget_grab_focus (GTK_WIDGET (popup));
  grab_status = gdk_pointer_grab (GTK_WIDGET (popup)->window, TRUE,
                                  GDK_BUTTON_PRESS_MASK
                                  | GDK_BUTTON_RELEASE_MASK
                                  | GDK_POINTER_MOTION_MASK, NULL, NULL,
                                  GDK_CURRENT_TIME);
  if (grab_status != GDK_GRAB_SUCCESS)
    {
      /* pointer grab must be attained otherwise user would have
       * problems closing the popup window. */
      gtk_grab_remove (GTK_WIDGET (popup));
      gtk_widget_destroy (GTK_WIDGET (popup));
      return NULL;
    }
  else
    {
      return GTK_WIDGET (popup);
    }
}

static gint
gimp_tag_popup_layout_tags (GimpTagPopup       *tag_popup,
                            gint                width)
{
  gint                  x;
  gint                  y;
  gint                  height = 0;
  gint                  i;
  gint                  line_height;
  gint                  space_width;
  PangoFontMetrics     *font_metrics;

  x = GIMP_TAG_POPUP_MARGIN;
  y = GIMP_TAG_POPUP_MARGIN;
  font_metrics = pango_context_get_metrics (tag_popup->context,
                                            pango_context_get_font_description (tag_popup->context),
                                            NULL);
  line_height = pango_font_metrics_get_ascent (font_metrics) +
      pango_font_metrics_get_descent (font_metrics);
  space_width = pango_font_metrics_get_approximate_char_width (font_metrics);
  line_height /= PANGO_SCALE;
  space_width /= PANGO_SCALE;
  pango_font_metrics_unref (font_metrics);
  for (i = 0; i < tag_popup->tag_count; i++)
    {
      pango_layout_set_text (tag_popup->layout,
                             gimp_tag_get_name (tag_popup->tag_data[i].tag), -1);
      pango_layout_get_size (tag_popup->layout,
                             &tag_popup->tag_data[i].bounds.width,
                             &tag_popup->tag_data[i].bounds.height);
      tag_popup->tag_data[i].bounds.width      /= PANGO_SCALE;
      tag_popup->tag_data[i].bounds.height     /= PANGO_SCALE;
      if (tag_popup->tag_data[i].bounds.width + x + 3 +GIMP_TAG_POPUP_MARGIN > width)
        {
          if (tag_popup->tag_data[i].bounds.width + line_height + GIMP_TAG_POPUP_MARGIN < width)
            {
              GdkRectangle     *close_rect = g_malloc (sizeof (GdkRectangle));
              close_rect->x = x - space_width - 5;
              close_rect->y = y;
              close_rect->width = width - close_rect->x;
              close_rect->height = line_height + 2;
              tag_popup->close_rectangles = g_list_append (tag_popup->close_rectangles,
                                                           close_rect);
            }
          x = GIMP_TAG_POPUP_MARGIN;
          y += line_height + 2;
        }

      tag_popup->tag_data[i].bounds.x = x;
      tag_popup->tag_data[i].bounds.y = y;

      x += tag_popup->tag_data[i].bounds.width + space_width + 5;
    }

  if (tag_popup->tag_count > 0
      && (width - x) > line_height + GIMP_TAG_POPUP_MARGIN)
    {
      GdkRectangle     *close_rect = g_malloc (sizeof (GdkRectangle));
      close_rect->x = x - space_width - 5;
      close_rect->y = y;
      close_rect->width = width - close_rect->x;
      close_rect->height = line_height + 2;
      tag_popup->close_rectangles = g_list_append (tag_popup->close_rectangles,
                                                   close_rect);
    }

  if (gtk_widget_get_direction (GTK_WIDGET (tag_popup)) == GTK_TEXT_DIR_RTL)
    {
      GList    *iterator;

      for (i = 0; i < tag_popup->tag_count; i++)
        {
          PopupTagData *tag_data = &tag_popup->tag_data[i];
          tag_data->bounds.x = width - tag_data->bounds.x - tag_data->bounds.width;
        }

      for (iterator = tag_popup->close_rectangles; iterator;
           iterator = g_list_next (iterator))
        {
          GdkRectangle *rect = (GdkRectangle *) iterator->data;
          rect->x = width - rect->x - rect->width;
        }
    }
  height = y + line_height + GIMP_TAG_POPUP_MARGIN;

  return height;
}

static gboolean
gimp_tag_popup_border_expose (GtkWidget           *widget,
                              GdkEventExpose      *event,
                              GimpTagPopup        *tag_popup)
{
  GdkGC                *gc;
  GdkRectangle          border;
  GdkRectangle          upper;
  GdkRectangle          lower;
  gint                  arrow_space;

  if (event->window == widget->window)
    {
      gc = gdk_gc_new (GDK_DRAWABLE (widget->window));

      get_arrows_visible_area (tag_popup, &border, &upper, &lower, &arrow_space);

      if (event->window == widget->window)
        {
          gint arrow_size = 0.7 * arrow_space;

          gtk_paint_box (widget->style,
                         widget->window,
                         GTK_STATE_NORMAL,
                         GTK_SHADOW_OUT,
                         &event->area, widget, "menu",
                         0, 0, -1, -1);

          if (tag_popup->arrows_visible)
            {
              gtk_paint_box (widget->style,
                             widget->window,
                             tag_popup->upper_arrow_state,
                             GTK_SHADOW_OUT,
                             &event->area, widget, "menu",
                             upper.x,
                             upper.y,
                             upper.width,
                             upper.height);

              gtk_paint_arrow (widget->style,
                               widget->window,
                               tag_popup->upper_arrow_state,
                               GTK_SHADOW_OUT,
                               &event->area, widget, "menu_scroll_arrow_up",
                               GTK_ARROW_UP,
                               TRUE,
                               upper.x + (upper.width - arrow_size) / 2,
                               upper.y + widget->style->ythickness + (arrow_space - arrow_size) / 2,
                               arrow_size, arrow_size);
            }

          if (tag_popup->arrows_visible)
            {
              gtk_paint_box (widget->style,
                             widget->window,
                             tag_popup->lower_arrow_state,
                             GTK_SHADOW_OUT,
                             &event->area, widget, "menu",
                             lower.x,
                             lower.y,
                             lower.width,
                             lower.height);

              gtk_paint_arrow (widget->style,
                               widget->window,
                               tag_popup->lower_arrow_state,
                               GTK_SHADOW_OUT,
                               &event->area, widget, "menu_scroll_arrow_down",
                               GTK_ARROW_DOWN,
                               TRUE,
                               lower.x + (lower.width - arrow_size) / 2,
                               lower.y + widget->style->ythickness + (arrow_space - arrow_size) / 2,
                               arrow_size, arrow_size);
            }
        }

      g_object_unref (gc);
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_border_event (GtkWidget          *widget,
                             GdkEvent           *event,
                             gpointer            user_data)
{
  GimpTagPopup         *tag_popup = GIMP_TAG_POPUP (widget);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;

      button_event = (GdkEventButton *) event;

      if (button_event->window == widget->window
          && gimp_tag_popup_button_scroll (tag_popup, button_event))
        {
          return TRUE;
        }

      gdk_window_get_pointer (widget->window, &x, &y, NULL);

      if (button_event->window != tag_popup->drawing_area->window
          && (x < widget->allocation.y
          || y < widget->allocation.x
          || x > widget->allocation.x + widget->allocation.width
          || y > widget->allocation.y + widget->allocation.height))
        {
          /* user has clicked outside the popup area,
           * which means it should be hidden. */
          gtk_grab_remove (widget);
          gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                      GDK_CURRENT_TIME);
          gtk_widget_destroy (widget);
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      gint              x;
      gint              y;

      gdk_window_get_pointer (widget->window, &x, &y, NULL);
      x += widget->allocation.x;
      y += widget->allocation.y;
      tag_popup->ignore_button_release = FALSE;
      gimp_tag_popup_handle_scrolling (tag_popup, x, y,
                                       tag_popup->timeout_id == 0, TRUE);
    }
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      tag_popup->single_select_disabled = TRUE;

      if (((GdkEventButton *)event)->window == widget->window
          && ! tag_popup->ignore_button_release
          && gimp_tag_popup_button_scroll (tag_popup, (GdkEventButton *) event))
        {
          return TRUE;
        }
    }
  else if (event->type == GDK_GRAB_BROKEN)
    {
      gtk_grab_remove (widget);
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  GDK_CURRENT_TIME);
      gtk_widget_destroy (widget);
    }
  else if (event->type == GDK_KEY_PRESS)
    {
      gtk_widget_destroy (GTK_WIDGET (tag_popup));
    }
  else if (event->type == GDK_SCROLL)
    {
      GdkEventScroll   *scroll_event = (GdkEventScroll *) event;

      switch (scroll_event->direction)
        {
          case GDK_SCROLL_RIGHT:
          case GDK_SCROLL_DOWN:
              gimp_tag_popup_scroll_by (tag_popup, MENU_SCROLL_STEP2);
              return TRUE;

          case GDK_SCROLL_LEFT:
          case GDK_SCROLL_UP:
              gimp_tag_popup_scroll_by (tag_popup, - MENU_SCROLL_STEP2);
              return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_list_expose (GtkWidget           *widget,
                            GdkEventExpose      *event,
                            GimpTagPopup        *tag_popup)
{
  GdkGC                *gc;
  PangoRenderer        *renderer;
  gint                  i;
  PangoAttribute       *attribute;
  PangoAttrList        *attributes;

  renderer = gdk_pango_renderer_get_default (gtk_widget_get_screen (widget));
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), widget->style->black_gc);
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer),
                                   widget->window);

  gc = gdk_gc_new (GDK_DRAWABLE (widget->window));
  gdk_gc_set_rgb_fg_color (gc, &tag_popup->combo_entry->selected_item_color);
  gdk_gc_set_line_attributes (gc, 5, GDK_LINE_SOLID, GDK_CAP_ROUND,
                              GDK_JOIN_ROUND);

  for (i = 0; i < tag_popup->tag_count; i++)
    {
      pango_layout_set_text (tag_popup->layout,
                             gimp_tag_get_name (tag_popup->tag_data[i].tag), -1);
      if (tag_popup->tag_data[i].state == GTK_STATE_SELECTED)
        {
          attributes = pango_attr_list_copy (tag_popup->combo_entry->selected_item_attr);
        }
      else if (tag_popup->tag_data[i].state == GTK_STATE_INSENSITIVE)
        {
          attributes = pango_attr_list_copy (tag_popup->combo_entry->insensitive_item_attr);
        }
      else
        {
          attributes = pango_attr_list_copy (tag_popup->combo_entry->normal_item_attr);
        }

      if (&tag_popup->tag_data[i] == tag_popup->prelight
          && tag_popup->tag_data[i].state != GTK_STATE_INSENSITIVE)
        {
          attribute = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          pango_attr_list_insert (attributes, attribute);
        }

      pango_layout_set_attributes (tag_popup->layout, attributes);
      pango_attr_list_unref (attributes);

      if (tag_popup->tag_data[i].state == GTK_STATE_SELECTED)
        {
          gdk_draw_rectangle (widget->window, gc, FALSE,
                              tag_popup->tag_data[i].bounds.x - 1,
                              tag_popup->tag_data[i].bounds.y - tag_popup->scroll_y + 1,
                              tag_popup->tag_data[i].bounds.width + 2,
                              tag_popup->tag_data[i].bounds.height - 2);
        }
      pango_renderer_draw_layout (renderer, tag_popup->layout,
                                  (tag_popup->tag_data[i].bounds.x) * PANGO_SCALE,
                                  (tag_popup->tag_data[i].bounds.y - tag_popup->scroll_y) * PANGO_SCALE);

      if (&tag_popup->tag_data[i] == tag_popup->prelight
          && tag_popup->tag_data[i].state != GTK_STATE_INSENSITIVE
          && ! tag_popup->single_select_disabled)
        {
          gtk_paint_focus (widget->style, widget->window,
                           tag_popup->tag_data[i].state,
                           &event->area, widget, NULL,
                           tag_popup->tag_data[i].bounds.x,
                           tag_popup->tag_data[i].bounds.y - tag_popup->scroll_y,
                           tag_popup->tag_data[i].bounds.width,
                           tag_popup->tag_data[i].bounds.height);
        }
    }

  g_object_unref (gc);

  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), NULL);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), NULL);

  return FALSE;
}

static gboolean
gimp_tag_popup_list_event (GtkWidget          *widget,
                           GdkEvent           *event,
                           GimpTagPopup       *tag_popup)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;
      gint              i;
      GdkRectangle     *bounds;
      GimpTag          *tag;

      tag_popup->single_select_disabled = TRUE;

      button_event = (GdkEventButton *) event;
      x = button_event->x;
      y = button_event->y;

      y += tag_popup->scroll_y;

      for (i = 0; i < tag_popup->tag_count; i++)
        {
          bounds = &tag_popup->tag_data[i].bounds;
          if (x >= bounds->x
              && y >= bounds->y
              && x < bounds->x + bounds->width
              && y < bounds->y + bounds->height)
            {
              tag = tag_popup->tag_data[i].tag;
              gimp_tag_popup_toggle_tag (tag_popup,
                                         &tag_popup->tag_data[i]);
              gtk_widget_queue_draw (widget);
              break;
            }
        }

      if (i == tag_popup->tag_count)
        {
          GList            *iterator;

          for (iterator = tag_popup->close_rectangles; iterator;
               iterator = g_list_next (iterator))
            {
              bounds = (GdkRectangle *) iterator->data;
              if (x >= bounds->x
                  && y >= bounds->y
                  && x < bounds->x + bounds->width
                  && y < bounds->y + bounds->height)
                {
                  gtk_widget_destroy (GTK_WIDGET (tag_popup));
                  break;
                }
            }
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      GdkEventMotion   *motion_event;
      gint              x;
      gint              y;
      gint              i;
      GdkRectangle     *bounds;
      PopupTagData     *previous_prelight = tag_popup->prelight;

      motion_event = (GdkEventMotion*) event;
      x = motion_event->x;
      y = motion_event->y;
      y += tag_popup->scroll_y;

      tag_popup->prelight = NULL;
      for (i = 0; i < tag_popup->tag_count; i++)
        {
          bounds = &tag_popup->tag_data[i].bounds;
          if (x >= bounds->x
              && y >= bounds->y
              && x < bounds->x + bounds->width
              && y < bounds->y + bounds->height)
            {
              tag_popup->prelight = &tag_popup->tag_data[i];
              break;
            }
        }

      if (previous_prelight != tag_popup->prelight)
        {
          gtk_widget_queue_draw (widget);
        }
    }
  else if (event->type == GDK_BUTTON_RELEASE
           && !tag_popup->single_select_disabled)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;
      gint              i;
      GdkRectangle     *bounds;
      GimpTag          *tag;

      tag_popup->single_select_disabled = TRUE;

      button_event = (GdkEventButton *) event;
      x = button_event->x;
      y = button_event->y;

      y += tag_popup->scroll_y;

      for (i = 0; i < tag_popup->tag_count; i++)
        {
          bounds = &tag_popup->tag_data[i].bounds;
          if (x >= bounds->x
              && y >= bounds->y
              && x < bounds->x + bounds->width
              && y < bounds->y + bounds->height)
            {
              tag = tag_popup->tag_data[i].tag;
              gimp_tag_popup_toggle_tag (tag_popup,
                                         &tag_popup->tag_data[i]);
              gtk_widget_destroy (GTK_WIDGET (tag_popup));
              break;
            }
        }
    }

  return FALSE;
}

static void
gimp_tag_popup_toggle_tag (GimpTagPopup        *tag_popup,
                           PopupTagData        *tag_data)
{
  gchar               **current_tags;
  GString              *tag_str;
  const gchar          *tag;
  gint                  length;
  gint                  i;
  gboolean              tag_toggled_off = FALSE;

  if (tag_data->state == GTK_STATE_NORMAL)
    {
      tag_data->state = GTK_STATE_SELECTED;
    }
  else if (tag_data->state == GTK_STATE_SELECTED)
    {
      tag_data->state = GTK_STATE_NORMAL;
    }
  else
    {
      return;
    }

  tag = gimp_tag_get_name (tag_data->tag);
  current_tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (tag_popup->combo_entry));
  tag_str = g_string_new ("");
  length = g_strv_length (current_tags);
  for (i = 0; i < length; i++)
    {
      if (! strcmp (current_tags[i], tag))
        {
          tag_toggled_off = TRUE;
        }
      else
        {
          if (tag_str->len)
            {
              g_string_append (tag_str, gimp_tag_entry_get_separator ());
              g_string_append_c (tag_str, ' ');
            }
          g_string_append (tag_str, current_tags[i]);
        }
    }

  if (! tag_toggled_off)
    {
      /* this tag was not selected yet,
       * so it needs to be toggled on. */
      if (tag_str->len)
        {
          g_string_append (tag_str, gimp_tag_entry_get_separator ());
          g_string_append_c (tag_str, ' ');
        }
      g_string_append (tag_str, tag);
    }

  gimp_tag_entry_set_tag_string (GIMP_TAG_ENTRY (tag_popup->combo_entry),
                                 tag_str->str);

  g_string_free (tag_str, TRUE);
  g_strfreev (current_tags);

  if (GIMP_TAG_ENTRY (tag_popup->combo_entry)->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      for (i = 0; i < tag_popup->tag_count; i++)
        {
          if (tag_popup->tag_data[i].state != GTK_STATE_SELECTED)
            {
              tag_popup->tag_data[i].state = GTK_STATE_INSENSITIVE;
            }
        }
      gimp_container_foreach (GIMP_CONTAINER (tag_popup->combo_entry->filtered_container),
                              (GFunc) gimp_tag_popup_check_can_toggle, tag_popup);
    }
}

static int
gimp_tag_popup_data_compare (const void *a, const void *b)
{
  return gimp_tag_compare_func (GIMP_TAG (((PopupTagData *) a)->tag),
                                GIMP_TAG (((PopupTagData *) b)->tag));
}

static void
gimp_tag_popup_check_can_toggle (GimpTagged    *tagged,
                                 GimpTagPopup  *tag_popup)
{
  GList        *tag_iterator;
  PopupTagData  search_key;
  PopupTagData *search_result;

  for (tag_iterator = gimp_tagged_get_tags (tagged); tag_iterator;
       tag_iterator = g_list_next (tag_iterator))
    {
      search_key.tag = GIMP_TAG (tag_iterator->data);
      search_result =
          (PopupTagData *) bsearch (&search_key, tag_popup->tag_data, tag_popup->tag_count,
                                    sizeof (PopupTagData), gimp_tag_popup_data_compare);
      if (search_result)
        {
          if (search_result->state == GTK_STATE_INSENSITIVE)
            {
              search_result->state = GTK_STATE_NORMAL;
            }
        }
    }
}

static gboolean
gimp_tag_popup_scroll_timeout (gpointer data)
{
  GimpTagPopup    *tag_popup;
  gboolean  touchscreen_mode;

  tag_popup = (GimpTagPopup*) data;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_do_timeout_scroll (tag_popup, touchscreen_mode);

  return TRUE;
}

static void
gimp_tag_popup_remove_scroll_timeout (GimpTagPopup *tag_popup)
{
  if (tag_popup->timeout_id)
    {
      g_source_remove (tag_popup->timeout_id);
      tag_popup->timeout_id = 0;
    }
}

static gboolean
gimp_tag_popup_scroll_timeout_initial (gpointer data)
{
  GimpTagPopup *tag_popup;
  guint     timeout;
  gboolean  touchscreen_mode;

  tag_popup = (GimpTagPopup*) (data);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                "gtk-timeout-repeat", &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_do_timeout_scroll (tag_popup, touchscreen_mode);

  gimp_tag_popup_remove_scroll_timeout (tag_popup);

  tag_popup->timeout_id = gdk_threads_add_timeout (timeout,
                                              gimp_tag_popup_scroll_timeout,
                                              tag_popup);

  return FALSE;
}

static void
gimp_tag_popup_start_scrolling (GimpTagPopup    *tag_popup)
{
  guint    timeout;
  gboolean touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                "gtk-timeout-repeat", &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_do_timeout_scroll (tag_popup, touchscreen_mode);

  tag_popup->timeout_id = gdk_threads_add_timeout (timeout,
                                                    gimp_tag_popup_scroll_timeout_initial,
                                                    tag_popup);
}

static void
gimp_tag_popup_stop_scrolling (GimpTagPopup   *tag_popup)
{
  gboolean touchscreen_mode;

  gimp_tag_popup_remove_scroll_timeout (tag_popup);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  if (!touchscreen_mode)
    {
      tag_popup->upper_arrow_prelight = FALSE;
      tag_popup->lower_arrow_prelight = FALSE;
    }
}

static void
gimp_tag_popup_scroll_by (GimpTagPopup         *tag_popup, 
                          gint                  step)
{
  gint          new_scroll_y = tag_popup->scroll_y + step;

  if (new_scroll_y < 0)
    {
      new_scroll_y = 0;
      if (tag_popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
        {
          gimp_tag_popup_stop_scrolling (tag_popup);
          gtk_widget_queue_draw (GTK_WIDGET (tag_popup));
        }
      tag_popup->upper_arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      tag_popup->upper_arrow_state = tag_popup->upper_arrow_prelight ?
          GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;
    }

  if (new_scroll_y >= tag_popup->scroll_height)
    {
      new_scroll_y = tag_popup->scroll_height - 1;
       if (tag_popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
        {
          gimp_tag_popup_stop_scrolling (tag_popup);
          gtk_widget_queue_draw (GTK_WIDGET (tag_popup));
        }
      tag_popup->lower_arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      tag_popup->lower_arrow_state = tag_popup->lower_arrow_prelight ?
          GTK_STATE_PRELIGHT : GTK_STATE_NORMAL;
    }

  if (new_scroll_y != tag_popup->scroll_y)
    {
      tag_popup->scroll_y = new_scroll_y;
      gdk_window_scroll (tag_popup->drawing_area->window, 0, -step);
    }
}

static void
gimp_tag_popup_do_timeout_scroll (GimpTagPopup *tag_popup,
                                  gboolean      touchscreen_mode)
{
  gimp_tag_popup_scroll_by (tag_popup, tag_popup->scroll_step);
}

static void
gimp_tag_popup_handle_scrolling (GimpTagPopup  *tag_popup,
                                 gint           x,
                                 gint           y,
                                 gboolean       enter,
                                 gboolean       motion)
{
  GdkRectangle rect;
  gboolean in_arrow;
  gboolean scroll_fast = FALSE;
  gboolean touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  /*  upper arrow handling  */

  get_arrows_sensitive_area (tag_popup, &rect, NULL);

  in_arrow = FALSE;
  if (tag_popup->arrows_visible &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    tag_popup->upper_arrow_prelight = in_arrow;

  if (tag_popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (tag_popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && tag_popup->upper_arrow_prelight)
                {
                  if (tag_popup->timeout_id == 0)
                    {
                      gimp_tag_popup_remove_scroll_timeout (tag_popup);
                      tag_popup->scroll_step = -MENU_SCROLL_STEP2; /* always fast */

                      if (!motion)
                        {
                          /* Only do stuff on click. */
                          gimp_tag_popup_start_scrolling (tag_popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (!enter)
                {
                  gimp_tag_popup_stop_scrolling (tag_popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y < rect.y + MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (!tag_popup->upper_arrow_prelight ||
                   tag_popup->scroll_fast != scroll_fast))
                {
                  tag_popup->upper_arrow_prelight = TRUE;
                  tag_popup->scroll_fast = scroll_fast;

                  gimp_tag_popup_remove_scroll_timeout (tag_popup);
                  tag_popup->scroll_step = scroll_fast ?
                    -MENU_SCROLL_STEP2 : -MENU_SCROLL_STEP1;

                  tag_popup->timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             gimp_tag_popup_scroll_timeout, tag_popup);
                }
              else if (!enter && !in_arrow && tag_popup->upper_arrow_prelight)
                {
                  gimp_tag_popup_stop_scrolling (tag_popup);
                }
            }
        }

      /*  gimp_tag_popup_start_scrolling() might have hit the top of the
       *  tag_popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (tag_popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (tag_popup->upper_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != tag_popup->upper_arrow_state)
            {
              tag_popup->upper_arrow_state = arrow_state;

              gdk_window_invalidate_rect (GTK_WIDGET (tag_popup)->window,
                                          &rect, FALSE);
            }
        }
    }

  /*  lower arrow handling  */

  get_arrows_sensitive_area (tag_popup, NULL, &rect);

  in_arrow = FALSE;
  if (tag_popup->arrows_visible &&
      (x >= rect.x) && (x < rect.x + rect.width) &&
      (y >= rect.y) && (y < rect.y + rect.height))
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    tag_popup->lower_arrow_prelight = in_arrow;

  if (tag_popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (tag_popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && tag_popup->lower_arrow_prelight)
                {
                  if (tag_popup->timeout_id == 0)
                    {
                      gimp_tag_popup_remove_scroll_timeout (tag_popup);
                      tag_popup->scroll_step = MENU_SCROLL_STEP2; /* always fast */

                      if (!motion)
                        {
                          /* Only do stuff on click. */
                          gimp_tag_popup_start_scrolling (tag_popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (!enter)
                {
                  gimp_tag_popup_stop_scrolling (tag_popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y > rect.y + rect.height - MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (!tag_popup->lower_arrow_prelight ||
                   tag_popup->scroll_fast != scroll_fast))
                {
                  tag_popup->lower_arrow_prelight = TRUE;
                  tag_popup->scroll_fast = scroll_fast;

                  gimp_tag_popup_remove_scroll_timeout (tag_popup);
                  tag_popup->scroll_step = scroll_fast ?
                    MENU_SCROLL_STEP2 : MENU_SCROLL_STEP1;

                  tag_popup->timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             gimp_tag_popup_scroll_timeout, tag_popup);
                }
              else if (!enter && !in_arrow && tag_popup->lower_arrow_prelight)
                {
                  gimp_tag_popup_stop_scrolling (tag_popup);
                }
            }
        }

      /*  gimp_tag_popup_start_scrolling() might have hit the bottom of the
       *  tag_popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (tag_popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (tag_popup->lower_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != tag_popup->lower_arrow_state)
            {
              tag_popup->lower_arrow_state = arrow_state;

              gdk_window_invalidate_rect (GTK_WIDGET (tag_popup)->window,
                                          &rect, FALSE);
            }
        }
    }
}

static gboolean
gimp_tag_popup_button_scroll (GimpTagPopup     *tag_popup,
                              GdkEventButton   *event)
{
  if (tag_popup->upper_arrow_prelight
      || tag_popup->lower_arrow_prelight)
    {
      gboolean touchscreen_mode;

      g_object_get (gtk_widget_get_settings (GTK_WIDGET (tag_popup)),
                    "gtk-touchscreen-mode", &touchscreen_mode,
                    NULL);

      if (touchscreen_mode)
        gimp_tag_popup_handle_scrolling (tag_popup,
                                   event->x_root, event->y_root,
                                   event->type == GDK_BUTTON_PRESS,
                                   FALSE);

      return TRUE;
    }

  return FALSE;
}

static void
get_arrows_visible_area (GimpTagPopup  *tag_popup,
                         GdkRectangle  *border,
                         GdkRectangle  *upper,
                         GdkRectangle  *lower,
                         gint          *arrow_space)
{
  GtkWidget    *widget = GTK_WIDGET (tag_popup->alignment);
  gint          scroll_arrow_height = tag_popup->scroll_arrow_height;
  guint         padding_top;
  guint         padding_bottom;
  guint         padding_left;
  guint         padding_right;

  gtk_alignment_get_padding (GTK_ALIGNMENT (tag_popup->alignment),
                             &padding_top, &padding_bottom,
                             &padding_left, &padding_right);

  *border = widget->allocation;

  upper->x = border->x + padding_left;
  upper->y = border->y;
  upper->width = border->width - padding_left - padding_right;
  upper->height = padding_top;

  lower->x = border->x + padding_left;
  lower->y = border->y + border->height - padding_bottom;
  lower->width = border->width - padding_left - padding_right;
  lower->height = padding_bottom;

  *arrow_space = scroll_arrow_height;
}

static void
get_arrows_sensitive_area (GimpTagPopup        *tag_popup,
                           GdkRectangle        *upper,
                           GdkRectangle        *lower)
{
  GdkRectangle  tmp_border;
  GdkRectangle  tmp_upper;
  GdkRectangle  tmp_lower;
  gint          tmp_arrow_space;

  get_arrows_visible_area (tag_popup, &tmp_border, &tmp_upper, &tmp_lower, &tmp_arrow_space);
  if (upper)
    {
      *upper = tmp_upper;
    }
  if (lower)
    {
      *lower = tmp_lower;
    }
}


