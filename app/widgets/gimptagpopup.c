/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptag.h"
#include "core/gimptagged.h"
#include "core/gimptaggedcontainer.h"
#include "core/gimpviewable.h"

#include "gimpcombotagentry.h"
#include "gimptagentry.h"
#include "gimptagpopup.h"

#include "gimp-intl.h"


#define MENU_SCROLL_STEP1            8
#define MENU_SCROLL_STEP2           15
#define MENU_SCROLL_FAST_ZONE        8
#define MENU_SCROLL_TIMEOUT1        50
#define MENU_SCROLL_TIMEOUT2        20

#define GIMP_TAG_POPUP_MARGIN        5
#define GIMP_TAG_POPUP_PADDING       2
#define GIMP_TAG_POPUP_LINE_SPACING  2

enum
{
  PROP_0,
  PROP_OWNER
};

struct _PopupTagData
{
  GimpTag      *tag;
  GdkRectangle  bounds;
  GtkStateType  state;
};


static void     gimp_tag_popup_constructed             (GObject        *object);
static void     gimp_tag_popup_dispose                 (GObject        *object);
static void     gimp_tag_popup_set_property            (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void     gimp_tag_popup_get_property            (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);

static gboolean gimp_tag_popup_border_draw             (GtkWidget      *widget,
                                                        cairo_t        *cr,
                                                        GimpTagPopup   *popup);
static gboolean gimp_tag_popup_list_draw               (GtkWidget      *widget,
                                                        cairo_t        *cr,
                                                        GimpTagPopup   *popup);
static gboolean gimp_tag_popup_border_event            (GtkWidget      *widget,
                                                        GdkEvent       *event);
static gboolean gimp_tag_popup_list_event              (GtkWidget      *widget,
                                                        GdkEvent       *event,
                                                        GimpTagPopup   *popup);
static gboolean gimp_tag_popup_is_in_tag               (PopupTagData   *tag_data,
                                                        gint            x,
                                                        gint            y);
static void     gimp_tag_popup_queue_draw_tag          (GimpTagPopup   *widget,
                                                        PopupTagData   *tag_data);
static void     gimp_tag_popup_toggle_tag              (GimpTagPopup   *popup,
                                                        PopupTagData   *tag_data);
static void     gimp_tag_popup_check_can_toggle        (GimpTagged     *tagged,
                                                        GimpTagPopup   *popup);
static gint     gimp_tag_popup_layout_tags             (GimpTagPopup   *popup,
                                                        gint            width);
static gboolean gimp_tag_popup_scroll_timeout          (gpointer        data);
static void     gimp_tag_popup_remove_scroll_timeout   (GimpTagPopup   *popup);
static gboolean gimp_tag_popup_scroll_timeout_initial  (gpointer        data);
static void     gimp_tag_popup_start_scrolling         (GimpTagPopup   *popup);
static void     gimp_tag_popup_stop_scrolling          (GimpTagPopup   *popup);
static void     gimp_tag_popup_scroll_by               (GimpTagPopup   *popup,
                                                        gint            step);
static void     gimp_tag_popup_handle_scrolling        (GimpTagPopup   *popup,
                                                        gint            x,
                                                        gint            y,
                                                        gboolean        enter,
                                                        gboolean        motion);

static gboolean gimp_tag_popup_button_scroll           (GimpTagPopup   *popup,
                                                        GdkEventButton *event);

static void     get_arrows_visible_area                (GimpTagPopup   *combo_entry,
                                                        GdkRectangle   *border,
                                                        GdkRectangle   *upper,
                                                        GdkRectangle   *lower,
                                                        gint           *arrow_space);
static void     get_arrows_sensitive_area              (GimpTagPopup   *popup,
                                                        GdkRectangle   *upper,
                                                        GdkRectangle   *lower);


G_DEFINE_TYPE (GimpTagPopup, gimp_tag_popup, GTK_TYPE_WINDOW);

#define parent_class gimp_tag_popup_parent_class


static void
gimp_tag_popup_class_init (GimpTagPopupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_tag_popup_constructed;
  object_class->dispose      = gimp_tag_popup_dispose;
  object_class->set_property = gimp_tag_popup_set_property;
  object_class->get_property = gimp_tag_popup_get_property;

  g_object_class_install_property (object_class, PROP_OWNER,
                                   g_param_spec_object ("owner", NULL, NULL,
                                                        GIMP_TYPE_COMBO_TAG_ENTRY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_tag_popup_init (GimpTagPopup *popup)
{
  GtkWidget *widget = GTK_WIDGET (popup);

  popup->upper_arrow_state = GTK_STATE_NORMAL;
  popup->lower_arrow_state = GTK_STATE_NORMAL;

  gtk_widget_add_events (widget,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_RELEASE_MASK    |
                         GDK_SCROLL_MASK);

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_show (popup->frame);

  popup->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (popup->frame), popup->alignment);
  gtk_widget_show (popup->alignment);

  popup->tag_area = gtk_drawing_area_new ();
  gtk_widget_add_events (popup->tag_area,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);
  gtk_container_add (GTK_CONTAINER (popup->alignment), popup->tag_area);
  gtk_widget_show (popup->tag_area);

  g_signal_connect (popup->alignment, "draw",
                    G_CALLBACK (gimp_tag_popup_border_draw),
                    popup);
  g_signal_connect (popup, "event",
                    G_CALLBACK (gimp_tag_popup_border_event),
                    NULL);
  g_signal_connect (popup->tag_area, "draw",
                    G_CALLBACK (gimp_tag_popup_list_draw),
                    popup);
  g_signal_connect (popup->tag_area, "event",
                    G_CALLBACK (gimp_tag_popup_list_event),
                    popup);
}

static void
gimp_tag_popup_constructed (GObject *object)
{
  GimpTagPopup        *popup = GIMP_TAG_POPUP (object);
  GimpTaggedContainer *container;
  GtkWidget           *entry;
  GtkAllocation        entry_allocation;
  GtkStyle            *frame_style;
  gint                 x;
  gint                 y;
  gint                 width;
  gint                 height;
  gint                 popup_height;
  GHashTable          *tag_hash;
  GList               *tag_list;
  GList               *tag_iterator;
  gint                 i;
  gint                 max_height;
  gint                 screen_height;
  gchar              **current_tags;
  gint                 current_count;
  GdkRectangle         popup_rects[2]; /* variants of popup placement */
  GdkRectangle         popup_rect; /* best popup rect in screen coordinates */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  entry = GTK_WIDGET (popup->combo_entry);

  gtk_window_set_screen (GTK_WINDOW (popup), gtk_widget_get_screen (entry));

  popup->context = gtk_widget_create_pango_context (GTK_WIDGET (popup));
  popup->layout  = pango_layout_new (popup->context);

  gtk_widget_get_allocation (entry, &entry_allocation);

  gtk_widget_style_get (GTK_WIDGET (popup),
                        "scroll-arrow-vlength", &popup->scroll_arrow_height,
                        NULL);

  pango_layout_set_attributes (popup->layout,
                               popup->combo_entry->normal_item_attr);

  current_tags  = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (popup->combo_entry));
  current_count = g_strv_length (current_tags);

  container = GIMP_TAG_ENTRY (popup->combo_entry)->container;

  tag_hash = container->tag_ref_counts;
  tag_list = g_hash_table_get_keys (tag_hash);
  tag_list = g_list_sort (tag_list, gimp_tag_compare_func);

  popup->tag_count = g_list_length (tag_list);
  popup->tag_data  = g_new0 (PopupTagData, popup->tag_count);

  for (i = 0, tag_iterator = tag_list;
       i < popup->tag_count;
       i++, tag_iterator = g_list_next (tag_iterator))
    {
      PopupTagData *tag_data = &popup->tag_data[i];
      gint          j;

      tag_data->tag   = tag_iterator->data;
      tag_data->state = GTK_STATE_NORMAL;

      g_object_ref (tag_data->tag);

      for (j = 0; j < current_count; j++)
        {
          if (! gimp_tag_compare_with_string (tag_data->tag, current_tags[j]))
            {
              tag_data->state = GTK_STATE_SELECTED;
              break;
            }
        }
    }

  g_list_free (tag_list);
  g_strfreev (current_tags);

  if (GIMP_TAG_ENTRY (popup->combo_entry)->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      for (i = 0; i < popup->tag_count; i++)
        {
          if (popup->tag_data[i].state != GTK_STATE_SELECTED)
            {
              popup->tag_data[i].state = GTK_STATE_INSENSITIVE;
            }
        }

      gimp_container_foreach (GIMP_CONTAINER (container),
                              (GFunc) gimp_tag_popup_check_can_toggle,
                              popup);
    }

  frame_style = gtk_widget_get_style (popup->frame);

  width  = (entry_allocation.width -
            2 * frame_style->xthickness);
  height = (gimp_tag_popup_layout_tags (popup, width) +
            2 * frame_style->ythickness);

  gdk_window_get_origin (gtk_widget_get_window (entry), &x, &y);

  if (! gtk_widget_get_has_window (entry))
    {
      x += entry_allocation.x;
      y += entry_allocation.y;
    }

  max_height = entry_allocation.height * 10;

  screen_height = gdk_screen_get_height (gtk_widget_get_screen (entry));

  popup_height = MIN (height, max_height);

  popup_rects[0].x      = x;
  popup_rects[0].y      = 0;
  popup_rects[0].width  = entry_allocation.width;
  popup_rects[0].height = y + entry_allocation.height;

  popup_rects[1].x      = x;
  popup_rects[1].y      = y;
  popup_rects[1].width  = popup_rects[0].width;
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
          popup_rect.y += popup->scroll_arrow_height + frame_style->ythickness;
        }
      else
        {
          popup_rect = popup_rects[1];
          popup_rect.y -= popup->scroll_arrow_height + frame_style->ythickness;
        }

      popup_height = popup_rect.height;
    }

  if (popup_height < height)
    {
      popup->arrows_visible    = TRUE;
      popup->upper_arrow_state = GTK_STATE_INSENSITIVE;

      gtk_alignment_set_padding (GTK_ALIGNMENT (popup->alignment),
                                 popup->scroll_arrow_height + 2,
                                 popup->scroll_arrow_height + 2, 0, 0);

      popup_height -= 2 * popup->scroll_arrow_height + 4;

      popup->scroll_height = height - popup_height;
      popup->scroll_y      = 0;
      popup->scroll_step   = 0;
    }

  gtk_widget_set_size_request (popup->tag_area, width, popup_height);

  gtk_window_move (GTK_WINDOW (popup), popup_rect.x, popup_rect.y);
  gtk_window_resize (GTK_WINDOW (popup), popup_rect.width, popup_rect.height);
}

static void
gimp_tag_popup_dispose (GObject *object)
{
  GimpTagPopup *popup = GIMP_TAG_POPUP (object);

  gimp_tag_popup_remove_scroll_timeout (popup);

  if (popup->combo_entry)
    {
      g_object_unref (popup->combo_entry);
      popup->combo_entry = NULL;
    }

  if (popup->layout)
    {
      g_object_unref (popup->layout);
      popup->layout = NULL;
    }

  if (popup->context)
    {
      g_object_unref (popup->context);
      popup->context = NULL;
    }

  if (popup->tag_data)
    {
      gint i;

      for (i = 0; i < popup->tag_count; i++)
        {
          g_object_unref (popup->tag_data[i].tag);
        }

      g_free (popup->tag_data);
      popup->tag_data = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tag_popup_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpTagPopup *popup = GIMP_TAG_POPUP (object);

  switch (property_id)
    {
    case PROP_OWNER:
      popup->combo_entry = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tag_popup_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpTagPopup *popup = GIMP_TAG_POPUP (object);

  switch (property_id)
    {
    case PROP_OWNER:
      g_value_set_object (value, popup->combo_entry);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_tag_popup_new:
 * @combo_entry: #GimpComboTagEntry which is owner of the popup window.
 *
 * Tag popup widget is only useful for for #GimpComboTagEntry and
 * should not be used elsewhere.
 *
 * Return value: a newly created #GimpTagPopup widget.
 **/
GtkWidget *
gimp_tag_popup_new (GimpComboTagEntry *combo_entry)
{
  g_return_val_if_fail (GIMP_IS_COMBO_TAG_ENTRY (combo_entry), NULL);

  return g_object_new (GIMP_TYPE_TAG_POPUP,
                       "type",  GTK_WINDOW_POPUP,
                       "owner", combo_entry,
                       NULL);
}

/**
 * gimp_tag_popup_show:
 * @tag_popup:  an instance of #GimpTagPopup
 *
 * Show tag popup widget. If mouse grab cannot be obtained for widget,
 * it is destroyed.
 **/
void
gimp_tag_popup_show (GimpTagPopup *popup)
{
  GtkWidget *widget;

  g_return_if_fail (GIMP_IS_TAG_POPUP (popup));

  widget = GTK_WIDGET (popup);

  gtk_widget_show (widget);

  gtk_grab_add (widget);
  gtk_widget_grab_focus (widget);

  if (gdk_pointer_grab (gtk_widget_get_window (widget), TRUE,
                        GDK_BUTTON_PRESS_MASK   |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK,
                        NULL, NULL,
                        GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    {
      /* pointer grab must be attained otherwise user would have
       * problems closing the popup window.
       */
      gtk_grab_remove (widget);
      gtk_widget_destroy (widget);
    }
}

static gint
gimp_tag_popup_layout_tags (GimpTagPopup *popup,
                            gint          width)
{
  PangoFontMetrics *font_metrics;
  gint              x;
  gint              y;
  gint              height = 0;
  gint              i;
  gint              line_height;
  gint              space_width;

  x = GIMP_TAG_POPUP_MARGIN;
  y = GIMP_TAG_POPUP_MARGIN;

  font_metrics = pango_context_get_metrics (popup->context,
                                            pango_context_get_font_description (popup->context),
                                            NULL);

  line_height = PANGO_PIXELS ((pango_font_metrics_get_ascent (font_metrics) +
                               pango_font_metrics_get_descent (font_metrics)));
  space_width = PANGO_PIXELS (pango_font_metrics_get_approximate_char_width (font_metrics));

  pango_font_metrics_unref (font_metrics);

  for (i = 0; i < popup->tag_count; i++)
    {
      PopupTagData *tag_data = &popup->tag_data[i];
      gint          w, h;

      pango_layout_set_text (popup->layout,
                             gimp_tag_get_name (tag_data->tag), -1);
      pango_layout_get_pixel_size (popup->layout, &w, &h);

      tag_data->bounds.width  = w + 2 * GIMP_TAG_POPUP_PADDING;
      tag_data->bounds.height = h + 2 * GIMP_TAG_POPUP_PADDING;

      if (x + space_width + tag_data->bounds.width +
          GIMP_TAG_POPUP_MARGIN - 1 > width)
        {
          x = GIMP_TAG_POPUP_MARGIN;
          y += line_height + 2 * GIMP_TAG_POPUP_PADDING + GIMP_TAG_POPUP_LINE_SPACING;
        }

      tag_data->bounds.x = x;
      tag_data->bounds.y = y;

      x += tag_data->bounds.width + space_width;
    }

  if (gtk_widget_get_direction (GTK_WIDGET (popup)) == GTK_TEXT_DIR_RTL)
    {
      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          tag_data->bounds.x = (width -
                                tag_data->bounds.x -
                                tag_data->bounds.width);
        }
    }

  height = y + line_height + GIMP_TAG_POPUP_MARGIN;

  return height;
}

static gboolean
gimp_tag_popup_border_draw (GtkWidget    *widget,
                            cairo_t      *cr,
                            GimpTagPopup *popup)
{
  GtkStyle     *style = gtk_widget_get_style (widget);
  GdkRectangle  border;
  GdkRectangle  upper;
  GdkRectangle  lower;
  gint          arrow_space;
  gint          arrow_size;

  if (! gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget)))
    return FALSE;

  get_arrows_visible_area (popup, &border, &upper, &lower, &arrow_space);

  arrow_size = 0.7 * arrow_space;

  gtk_paint_box (style, cr,
                 GTK_STATE_NORMAL,
                 GTK_SHADOW_OUT,
                 widget, "menu",
                 0, 0,
                 gtk_widget_get_allocated_width (widget),
                 gtk_widget_get_allocated_height (widget));

  if (popup->arrows_visible)
    {
      /*  upper arrow  */

      gtk_paint_box (style, cr,
                     popup->upper_arrow_state,
                     GTK_SHADOW_OUT,
                     widget, "menu",
                     upper.x,
                     upper.y,
                     upper.width,
                     upper.height);

      gtk_paint_arrow (style, cr,
                       popup->upper_arrow_state,
                       GTK_SHADOW_OUT,
                       widget, "menu_scroll_arrow_up",
                       GTK_ARROW_UP,
                       TRUE,
                       upper.x + (upper.width - arrow_size) / 2,
                       upper.y + style->ythickness + (arrow_space - arrow_size) / 2,
                       arrow_size, arrow_size);

      /*  lower arrow  */

      gtk_paint_box (style, cr,
                     popup->lower_arrow_state,
                     GTK_SHADOW_OUT,
                     widget, "menu",
                     lower.x,
                     lower.y,
                     lower.width,
                     lower.height);

      gtk_paint_arrow (style, cr,
                       popup->lower_arrow_state,
                       GTK_SHADOW_OUT,
                       widget, "menu_scroll_arrow_down",
                       GTK_ARROW_DOWN,
                       TRUE,
                       lower.x + (lower.width - arrow_size) / 2,
                       lower.y + style->ythickness + (arrow_space - arrow_size) / 2,
                       arrow_size, arrow_size);
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_border_event (GtkWidget *widget,
                             GdkEvent  *event)
{
  GimpTagPopup *popup = GIMP_TAG_POPUP (widget);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      GtkAllocation   allocation;
      gint            x;
      gint            y;

      if (button_event->window == gtk_widget_get_window (widget) &&
          gimp_tag_popup_button_scroll (popup, button_event))
        {
          return TRUE;
        }

      gtk_widget_get_allocation (widget, &allocation);

      gdk_window_get_pointer (gtk_widget_get_window (widget), &x, &y, NULL);

      if (button_event->window != gtk_widget_get_window (popup->tag_area) &&
          (x < allocation.y                    ||
           y < allocation.x                    ||
           x > allocation.x + allocation.width ||
           y > allocation.y + allocation.height))
        {
          /* user has clicked outside the popup area,
           * which means it should be hidden.
           */
          gtk_grab_remove (widget);
          gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                      GDK_CURRENT_TIME);
          gtk_widget_destroy (widget);
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      GdkEventMotion *motion_event = (GdkEventMotion *) event;
      gint            x, y;

      gdk_window_get_pointer (gtk_widget_get_window (widget), &x, &y, NULL);

      gimp_tag_popup_handle_scrolling (popup, x, y,
                                       motion_event->window ==
                                       gtk_widget_get_window (widget),
                                       TRUE);
    }
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;

      popup->single_select_disabled = TRUE;

      if (button_event->window == gtk_widget_get_window (widget) &&
          gimp_tag_popup_button_scroll (popup, button_event))
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
      gtk_grab_remove (widget);
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  GDK_CURRENT_TIME);
      gtk_widget_destroy (widget);
    }
  else if (event->type == GDK_SCROLL)
    {
      GdkEventScroll *scroll_event = (GdkEventScroll *) event;

      switch (scroll_event->direction)
        {
        case GDK_SCROLL_RIGHT:
        case GDK_SCROLL_DOWN:
          gimp_tag_popup_scroll_by (popup, MENU_SCROLL_STEP2);
          return TRUE;

        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_UP:
          gimp_tag_popup_scroll_by (popup, - MENU_SCROLL_STEP2);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_list_draw (GtkWidget    *widget,
                          cairo_t      *cr,
                          GimpTagPopup *popup)
{
  GtkStyle       *style = gtk_widget_get_style (widget);
  PangoAttribute *attribute;
  PangoAttrList  *attributes;
  gint            i;

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  for (i = 0; i < popup->tag_count; i++)
    {
      PopupTagData *tag_data = &popup->tag_data[i];

      pango_layout_set_text (popup->layout,
                             gimp_tag_get_name (tag_data->tag), -1);

      switch (tag_data->state)
        {
        case GTK_STATE_SELECTED:
          attributes = pango_attr_list_copy (popup->combo_entry->selected_item_attr);
          break;

        case GTK_STATE_INSENSITIVE:
          attributes = pango_attr_list_copy (popup->combo_entry->insensitive_item_attr);
          break;

        default:
          attributes = pango_attr_list_copy (popup->combo_entry->normal_item_attr);
          break;
        }

      if (tag_data == popup->prelight &&
          tag_data->state != GTK_STATE_INSENSITIVE)
        {
          attribute = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          pango_attr_list_insert (attributes, attribute);
        }

      pango_layout_set_attributes (popup->layout, attributes);
      pango_attr_list_unref (attributes);

      if (tag_data->state == GTK_STATE_SELECTED)
        {
          gdk_cairo_set_source_color (cr,
                                      &popup->combo_entry->selected_item_color);

          cairo_rectangle (cr,
                           tag_data->bounds.x - 1,
                           tag_data->bounds.y - popup->scroll_y,
                           tag_data->bounds.width + 2,
                           tag_data->bounds.height);
          cairo_fill (cr);

          cairo_translate (cr, 0.5, 0.5);

          cairo_move_to (cr,
                         tag_data->bounds.x,
                         tag_data->bounds.y - popup->scroll_y - 1);
          cairo_line_to (cr,
                         tag_data->bounds.x + tag_data->bounds.width - 1,
                         tag_data->bounds.y - popup->scroll_y - 1);

          cairo_move_to (cr,
                         tag_data->bounds.x,
                         tag_data->bounds.y - popup->scroll_y + tag_data->bounds.height);
          cairo_line_to (cr,
                         tag_data->bounds.x + tag_data->bounds.width - 1,
                         tag_data->bounds.y - popup->scroll_y + tag_data->bounds.height);

          cairo_stroke (cr);

          cairo_translate (cr, -0.5, -0.5);
        }

      cairo_move_to (cr,
                     (tag_data->bounds.x +
                      GIMP_TAG_POPUP_PADDING),
                     (tag_data->bounds.y -
                      popup->scroll_y +
                      GIMP_TAG_POPUP_PADDING));

      pango_cairo_show_layout (cr, popup->layout);

      if (tag_data == popup->prelight              &&
          tag_data->state != GTK_STATE_INSENSITIVE &&
          ! popup->single_select_disabled)
        {
          gtk_paint_focus (style, cr,
                           tag_data->state,
                           widget, NULL,
                           tag_data->bounds.x,
                           tag_data->bounds.y - popup->scroll_y,
                           tag_data->bounds.width,
                           tag_data->bounds.height);
        }
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_list_event (GtkWidget    *widget,
                           GdkEvent     *event,
                           GimpTagPopup *popup)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      gint            x;
      gint            y;
      gint            i;

      popup->single_select_disabled = TRUE;

      x = button_event->x;
      y = button_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (gimp_tag_popup_is_in_tag (tag_data, x, y))
            {
              gimp_tag_popup_toggle_tag (popup, tag_data);
              gtk_widget_queue_draw (widget);
              break;
            }
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      GdkEventMotion *motion_event = (GdkEventMotion *) event;
      PopupTagData   *prelight     = NULL;
      gint            x;
      gint            y;
      gint            i;

      x = motion_event->x;
      y = motion_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (gimp_tag_popup_is_in_tag (tag_data, x, y))
            {
              prelight = tag_data;
              break;
            }
        }

      if (prelight != popup->prelight)
        {
          if (popup->prelight)
            gimp_tag_popup_queue_draw_tag (popup, popup->prelight);

          popup->prelight = prelight;

          if (popup->prelight)
            gimp_tag_popup_queue_draw_tag (popup, popup->prelight);
        }
    }
  else if (event->type == GDK_BUTTON_RELEASE &&
           ! popup->single_select_disabled)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      gint            x;
      gint            y;
      gint            i;

      popup->single_select_disabled = TRUE;

      x = button_event->x;
      y = button_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (gimp_tag_popup_is_in_tag (tag_data, x, y))
            {
              gimp_tag_popup_toggle_tag (popup, tag_data);
              gtk_widget_destroy (GTK_WIDGET (popup));
              break;
            }
        }
    }

  return FALSE;
}

static gboolean
gimp_tag_popup_is_in_tag (PopupTagData *tag_data,
                          gint          x,
                          gint          y)
{
  if (x >= tag_data->bounds.x                          &&
      y >= tag_data->bounds.y                          &&
      x <  tag_data->bounds.x + tag_data->bounds.width &&
      y <  tag_data->bounds.y + tag_data->bounds.height)
    {
      return TRUE;
    }

  return FALSE;
}

static void
gimp_tag_popup_queue_draw_tag (GimpTagPopup *popup,
                               PopupTagData *tag_data)
{
  gtk_widget_queue_draw_area (popup->tag_area,
                              tag_data->bounds.x,
                              tag_data->bounds.y - popup->scroll_y,
                              tag_data->bounds.width,
                              tag_data->bounds.height);
}

static void
gimp_tag_popup_toggle_tag (GimpTagPopup *popup,
                           PopupTagData *tag_data)
{
  gchar    **current_tags;
  GString   *tag_str;
  gint       length;
  gint       i;
  gboolean   tag_toggled_off = FALSE;

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

  current_tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (popup->combo_entry));
  tag_str = g_string_new ("");
  length = g_strv_length (current_tags);
  for (i = 0; i < length; i++)
    {
      if (! gimp_tag_compare_with_string (tag_data->tag, current_tags[i]))
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
      /* this tag was not selected yet, so it needs to be toggled on */

      if (tag_str->len)
        {
          g_string_append (tag_str, gimp_tag_entry_get_separator ());
          g_string_append_c (tag_str, ' ');
        }

      g_string_append (tag_str, gimp_tag_get_name (tag_data->tag));
    }

  gimp_tag_entry_set_tag_string (GIMP_TAG_ENTRY (popup->combo_entry),
                                 tag_str->str);

  g_string_free (tag_str, TRUE);
  g_strfreev (current_tags);

  if (GIMP_TAG_ENTRY (popup->combo_entry)->mode == GIMP_TAG_ENTRY_MODE_QUERY)
    {
      GimpTaggedContainer *container;

      container = GIMP_TAG_ENTRY (popup->combo_entry)->container;

      for (i = 0; i < popup->tag_count; i++)
        {
          if (popup->tag_data[i].state != GTK_STATE_SELECTED)
            {
              popup->tag_data[i].state = GTK_STATE_INSENSITIVE;
            }
        }

      gimp_container_foreach (GIMP_CONTAINER (container),
                              (GFunc) gimp_tag_popup_check_can_toggle,
                              popup);
    }
}

static int
gimp_tag_popup_data_compare (const void *a,
                             const void *b)
{
  return gimp_tag_compare_func (((PopupTagData *) a)->tag,
                                ((PopupTagData *) b)->tag);
}

static void
gimp_tag_popup_check_can_toggle (GimpTagged   *tagged,
                                 GimpTagPopup *popup)
{
  GList *iterator;

  for (iterator = gimp_tagged_get_tags (tagged);
       iterator;
       iterator = g_list_next (iterator))
    {
      PopupTagData  search_key;
      PopupTagData *search_result;

      search_key.tag = iterator->data;

      search_result =
        (PopupTagData *) bsearch (&search_key,
                                  popup->tag_data, popup->tag_count,
                                  sizeof (PopupTagData),
                                  gimp_tag_popup_data_compare);

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
  GimpTagPopup *popup = data;
  gboolean      touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_scroll_by (popup, popup->scroll_step);

  return TRUE;
}

static void
gimp_tag_popup_remove_scroll_timeout (GimpTagPopup *popup)
{
  if (popup->scroll_timeout_id)
    {
      g_source_remove (popup->scroll_timeout_id);
      popup->scroll_timeout_id = 0;
    }
}

static gboolean
gimp_tag_popup_scroll_timeout_initial (gpointer data)
{
  GimpTagPopup *popup = data;
  guint         timeout;
  gboolean      touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-timeout-repeat",   &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_scroll_by (popup, popup->scroll_step);

  gimp_tag_popup_remove_scroll_timeout (popup);

  popup->scroll_timeout_id =
    gdk_threads_add_timeout (timeout,
                             gimp_tag_popup_scroll_timeout,
                             popup);

  return FALSE;
}

static void
gimp_tag_popup_start_scrolling (GimpTagPopup *popup)
{
  guint    timeout;
  gboolean touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-timeout-repeat",   &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  gimp_tag_popup_scroll_by (popup, popup->scroll_step);

  popup->scroll_timeout_id =
    gdk_threads_add_timeout (timeout,
                             gimp_tag_popup_scroll_timeout_initial,
                             popup);
}

static void
gimp_tag_popup_stop_scrolling (GimpTagPopup *popup)
{
  gboolean touchscreen_mode;

  gimp_tag_popup_remove_scroll_timeout (popup);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  if (! touchscreen_mode)
    {
      popup->upper_arrow_prelight = FALSE;
      popup->lower_arrow_prelight = FALSE;
    }
}

static void
gimp_tag_popup_scroll_by (GimpTagPopup *popup,
                          gint          step)
{
  GtkStateType arrow_state;
  gint         new_scroll_y = popup->scroll_y + step;

  arrow_state = popup->upper_arrow_state;

  if (new_scroll_y < 0)
    {
      new_scroll_y = 0;

      if (arrow_state != GTK_STATE_INSENSITIVE)
        gimp_tag_popup_stop_scrolling (popup);

      arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      arrow_state = (popup->upper_arrow_prelight ?
                     GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
    }

  if (arrow_state != popup->upper_arrow_state)
    {
      popup->upper_arrow_state = arrow_state;
      gtk_widget_queue_draw (GTK_WIDGET (popup));
    }

  arrow_state = popup->lower_arrow_state;

  if (new_scroll_y >= popup->scroll_height)
    {
      new_scroll_y = popup->scroll_height - 1;

      if (arrow_state != GTK_STATE_INSENSITIVE)
        gimp_tag_popup_stop_scrolling (popup);

      arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      arrow_state = (popup->lower_arrow_prelight ?
                     GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
    }

  if (arrow_state != popup->lower_arrow_state)
    {
      popup->lower_arrow_state = arrow_state;
      gtk_widget_queue_draw (GTK_WIDGET (popup));
    }

  if (new_scroll_y != popup->scroll_y)
    {
      popup->scroll_y = new_scroll_y;

      gdk_window_scroll (gtk_widget_get_window (popup->tag_area), 0, -step);
    }
}

static void
gimp_tag_popup_handle_scrolling (GimpTagPopup *popup,
                                 gint          x,
                                 gint          y,
                                 gboolean      enter,
                                 gboolean      motion)
{
  GdkRectangle rect;
  gboolean     in_arrow;
  gboolean     scroll_fast = FALSE;
  gboolean     touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  /*  upper arrow handling  */

  get_arrows_sensitive_area (popup, &rect, NULL);

  in_arrow = FALSE;
  if (popup->arrows_visible    &&
      x >= rect.x              &&
      x <  rect.x + rect.width &&
      y >= rect.y              &&
      y <  rect.y + rect.height)
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    popup->upper_arrow_prelight = in_arrow;

  if (popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && popup->upper_arrow_prelight)
                {
                  if (popup->scroll_timeout_id == 0)
                    {
                      gimp_tag_popup_remove_scroll_timeout (popup);
                      popup->scroll_step = -MENU_SCROLL_STEP2; /* always fast */

                      if (! motion)
                        {
                          /* Only do stuff on click. */
                          gimp_tag_popup_start_scrolling (popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (! enter)
                {
                  gimp_tag_popup_stop_scrolling (popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y < rect.y + MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (! popup->upper_arrow_prelight ||
                   popup->scroll_fast != scroll_fast))
                {
                  popup->upper_arrow_prelight = TRUE;
                  popup->scroll_fast          = scroll_fast;

                  gimp_tag_popup_remove_scroll_timeout (popup);
                  popup->scroll_step = (scroll_fast ?
                                        -MENU_SCROLL_STEP2 : -MENU_SCROLL_STEP1);

                  popup->scroll_timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             gimp_tag_popup_scroll_timeout,
                                             popup);
                }
              else if (! enter && ! in_arrow && popup->upper_arrow_prelight)
                {
                  gimp_tag_popup_stop_scrolling (popup);
                }
            }
        }

      /*  gimp_tag_popup_start_scrolling() might have hit the top of the
       *  tag_popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (popup->upper_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != popup->upper_arrow_state)
            {
              popup->upper_arrow_state = arrow_state;

              gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (popup)),
                                          &rect, FALSE);
            }
        }
    }

  /*  lower arrow handling  */

  get_arrows_sensitive_area (popup, NULL, &rect);

  in_arrow = FALSE;
  if (popup->arrows_visible    &&
      x >= rect.x              &&
      x <  rect.x + rect.width &&
      y >= rect.y              &&
      y <  rect.y + rect.height)
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    popup->lower_arrow_prelight = in_arrow;

  if (popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && popup->lower_arrow_prelight)
                {
                  if (popup->scroll_timeout_id == 0)
                    {
                      gimp_tag_popup_remove_scroll_timeout (popup);
                      popup->scroll_step = MENU_SCROLL_STEP2; /* always fast */

                      if (! motion)
                        {
                          /* Only do stuff on click. */
                          gimp_tag_popup_start_scrolling (popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (! enter)
                {
                  gimp_tag_popup_stop_scrolling (popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y > rect.y + rect.height - MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (! popup->lower_arrow_prelight ||
                   popup->scroll_fast != scroll_fast))
                {
                  popup->lower_arrow_prelight = TRUE;
                  popup->scroll_fast          = scroll_fast;

                  gimp_tag_popup_remove_scroll_timeout (popup);
                  popup->scroll_step = (scroll_fast ?
                                        MENU_SCROLL_STEP2 : MENU_SCROLL_STEP1);

                  popup->scroll_timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             gimp_tag_popup_scroll_timeout,
                                             popup);
                }
              else if (! enter && ! in_arrow && popup->lower_arrow_prelight)
                {
                  gimp_tag_popup_stop_scrolling (popup);
                }
            }
        }

      /*  gimp_tag_popup_start_scrolling() might have hit the bottom of the
       *  popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (popup->lower_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != popup->lower_arrow_state)
            {
              popup->lower_arrow_state = arrow_state;

              gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (popup)),
                                          &rect, FALSE);
            }
        }
    }
}

static gboolean
gimp_tag_popup_button_scroll (GimpTagPopup   *popup,
                              GdkEventButton *event)
{
  if (popup->upper_arrow_prelight || popup->lower_arrow_prelight)
    {
      gboolean touchscreen_mode;

      g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                    "gtk-touchscreen-mode", &touchscreen_mode,
                    NULL);

      if (touchscreen_mode)
        gimp_tag_popup_handle_scrolling (popup,
                                         event->x_root,
                                         event->y_root,
                                         event->type == GDK_BUTTON_PRESS,
                                         FALSE);

      return TRUE;
    }

  return FALSE;
}

static void
get_arrows_visible_area (GimpTagPopup *popup,
                         GdkRectangle *border,
                         GdkRectangle *upper,
                         GdkRectangle *lower,
                         gint         *arrow_space)
{
  GtkWidget *widget = GTK_WIDGET (popup->alignment);
  guint      padding_top;
  guint      padding_bottom;
  guint      padding_left;
  guint      padding_right;

  gtk_alignment_get_padding (GTK_ALIGNMENT (popup->alignment),
                             &padding_top, &padding_bottom,
                             &padding_left, &padding_right);

  gtk_widget_get_allocation (widget, border);

  upper->x      = border->x + padding_left;
  upper->y      = border->y;
  upper->width  = border->width - padding_left - padding_right;
  upper->height = padding_top;

  lower->x      = border->x + padding_left;
  lower->y      = border->y + border->height - padding_bottom;
  lower->width  = border->width - padding_left - padding_right;
  lower->height = padding_bottom;

  *arrow_space = popup->scroll_arrow_height;
}

static void
get_arrows_sensitive_area (GimpTagPopup *popup,
                           GdkRectangle *upper,
                           GdkRectangle *lower)
{
  GdkRectangle tmp_border;
  GdkRectangle tmp_upper;
  GdkRectangle tmp_lower;
  gint         tmp_arrow_space;

  get_arrows_visible_area (popup,
                           &tmp_border, &tmp_upper, &tmp_lower, &tmp_arrow_space);

  if (upper)
    *upper = tmp_upper;

  if (lower)
    *lower = tmp_lower;
}
