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
#include "gimpcombotagentry.h"


#define GIMP_TAG_POPUP_MARGIN           5

typedef struct
{
  GimpTag               tag;
  GdkRectangle          bounds;
  gboolean              selected;
} PopupTagData;

typedef struct
{
  GimpComboTagEntry    *combo_entry;
  GtkWidget            *drawing_area;
  PangoContext         *context;
  PangoLayout          *layout;
  PopupTagData         *tag_data;
  gint                  tag_count;
} PopupData;


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
static gboolean gimp_combo_tag_entry_popup_expose      (GtkWidget         *widget,
                                                        GdkEventExpose    *event,
                                                        PopupData         *popup_dta);
static gboolean gimp_combo_tag_entry_popup_event       (GtkWidget          *widget,
                                                        GdkEvent           *event,
                                                        PopupData          *popup_data);
static gboolean gimp_combo_tag_entry_drawing_area_event(GtkWidget          *widget,
                                                        GdkEvent           *event,
                                                        PopupData          *popup_data);
static void     gimp_combo_tag_entry_toggle_tag        (GimpComboTagEntry  *combo_entry,
                                                        PopupTagData       *tag_data);
static gint     gimp_combo_tag_entry_layout_tags       (PopupData          *popup_data,
                                                        gint                width);

static void     popup_data_destroy                     (PopupData          *popup_data);

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

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

GtkWidget *
gimp_combo_tag_entry_new (GimpTagEntry         *tag_entry)
{
  GimpComboTagEntry            *combo_entry;

  combo_entry = g_object_new (GIMP_TYPE_COMBO_TAG_ENTRY, NULL);
  combo_entry->tag_entry = GTK_WIDGET (tag_entry);
  combo_entry->normal_item_attr = pango_attr_list_new ();
  pango_attr_list_insert (combo_entry->normal_item_attr,
                          pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));

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

  if (combo_entry->selected_item_attr)
    {
      pango_attr_list_unref (combo_entry->selected_item_attr);
    }
  combo_entry->selected_item_attr = pango_attr_list_copy (combo_entry->normal_item_attr);
  color = widget->style->text[GTK_STATE_SELECTED];
  attribute = pango_attr_foreground_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->selected_item_attr, attribute);
  color = widget->style->bg[GTK_STATE_SELECTED];
  attribute = pango_attr_background_new (color.red, color.green, color.blue);
  pango_attr_list_insert (combo_entry->selected_item_attr, attribute);
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
  GtkWidget            *popup;
  GtkWidget            *scrolled_window;
  GtkWidget            *drawing_area;
  GtkWidget            *viewport;
  GtkWidget            *frame;
  gint                  x;
  gint                  y;
  gint                  width;
  gint                  height;
  gint                  popup_height;
  PopupData            *popup_data;
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

  popup = gtk_window_new (GTK_WINDOW_POPUP);
  combo_entry->popup = popup;
  gtk_widget_add_events (GTK_WIDGET (popup),
                         GDK_BUTTON_PRESS_MASK);
  gtk_window_set_screen (GTK_WINDOW (popup),
                         gtk_widget_get_screen (GTK_WIDGET (combo_entry)));

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (drawing_area),
                         GDK_BUTTON_PRESS_MASK);

  popup_data = g_malloc (sizeof (PopupData));
  popup_data->combo_entry = combo_entry;
  popup_data->drawing_area = drawing_area;
  popup_data->context = gtk_widget_create_pango_context (GTK_WIDGET (popup));
  popup_data->layout = pango_layout_new (popup_data->context);

  pango_layout_set_attributes (popup_data->layout, combo_entry->normal_item_attr);

  current_tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (combo_entry->tag_entry));
  current_count = g_strv_length (current_tags);

  tag_hash = GIMP_TAG_ENTRY (combo_entry->tag_entry)->tagged_container->tag_ref_counts;
  tag_list = g_hash_table_get_keys (tag_hash);
  tag_list = g_list_sort (tag_list, gimp_tag_compare_func);
  popup_data->tag_count = g_list_length (tag_list);
  popup_data->tag_data = g_malloc (sizeof (PopupTagData) * popup_data->tag_count);
  tag_iterator = tag_list;
  for (i = 0; i < popup_data->tag_count; i++)
    {
      popup_data->tag_data[i].tag = GPOINTER_TO_UINT (tag_iterator->data);
      popup_data->tag_data[i].selected = FALSE;
      list_tag = gimp_tag_to_string (popup_data->tag_data[i].tag);
      for (j = 0; j < current_count; j++)
        {
          if (! strcmp (current_tags[j], list_tag))
            {
              popup_data->tag_data[i].selected = TRUE;
              break;
            }
        }
      tag_iterator = g_list_next (tag_iterator);
    }
  g_list_free (tag_list);
  g_strfreev (current_tags);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (viewport), drawing_area);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window),
                     viewport);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);

  width = GTK_WIDGET (combo_entry)->allocation.width - frame->style->xthickness * 2;
  height = gimp_combo_tag_entry_layout_tags (popup_data, width);
  gdk_window_get_origin (GTK_WIDGET (combo_entry)->window, &x, &y);
  max_height = GTK_WIDGET (combo_entry)->allocation.height * 7;
  screen_height = gdk_screen_get_height (gtk_widget_get_screen (GTK_WIDGET (combo_entry)));
  height += frame->style->ythickness * 2;
  popup_height = MIN (height, max_height);
  if (y > screen_height / 2)
    {
      y -= popup_height + frame->style->ythickness * 2;
    }
  else
    {
      y += GTK_WIDGET (combo_entry)->allocation.height;
    }

  drawing_area->requisition.width = width;
  drawing_area->requisition.height = height;

  gtk_widget_set_size_request (scrolled_window, width, popup_height);
  gtk_window_move (GTK_WINDOW (popup), x, y);

  gtk_container_add (GTK_CONTAINER (popup), frame);
  gtk_widget_show_all (GTK_WIDGET (popup));

  if (popup_height < height)
    {
      height = gimp_combo_tag_entry_layout_tags (popup_data, viewport->allocation.width);
      gtk_widget_set_size_request (drawing_area, width, height);
    }

  g_object_set_data_full (G_OBJECT (popup), "popup-data",
                          popup_data, (GDestroyNotify) popup_data_destroy);

  g_signal_connect (drawing_area, "expose-event",
                    G_CALLBACK (gimp_combo_tag_entry_popup_expose),
                    popup_data);
  g_signal_connect (drawing_area, "event",
                    G_CALLBACK (gimp_combo_tag_entry_drawing_area_event),
                    popup_data);
  g_signal_connect (popup, "event",
                    G_CALLBACK (gimp_combo_tag_entry_popup_event),
                    popup_data);

  gtk_grab_add (popup);
  gtk_widget_grab_focus (combo_entry->tag_entry);
  grab_status = gdk_pointer_grab (popup->window, TRUE,
                                  GDK_BUTTON_PRESS_MASK, NULL, NULL,
                                  GDK_CURRENT_TIME);
  if (grab_status != GDK_GRAB_SUCCESS)
    {
      /* pointer grab must be attained otherwise user would have
       * problems closing the popup window. */
      gtk_grab_remove (popup);
      gtk_widget_destroy (popup);
    }
}

static gint
gimp_combo_tag_entry_layout_tags (PopupData            *popup_data,
                                  gint                  width)
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
  font_metrics = pango_context_get_metrics (popup_data->context,
                                            pango_context_get_font_description (popup_data->context),
                                            NULL);
  line_height = pango_font_metrics_get_ascent (font_metrics) +
      pango_font_metrics_get_descent (font_metrics);
  space_width = pango_font_metrics_get_approximate_char_width (font_metrics);
  line_height /= PANGO_SCALE;
  space_width /= PANGO_SCALE;
  pango_font_metrics_unref (font_metrics);
  for (i = 0; i < popup_data->tag_count; i++)
    {
      pango_layout_set_text (popup_data->layout,
                             g_quark_to_string (popup_data->tag_data[i].tag), -1);
      pango_layout_get_size (popup_data->layout,
                             &popup_data->tag_data[i].bounds.width,
                             &popup_data->tag_data[i].bounds.height);
      popup_data->tag_data[i].bounds.width      /= PANGO_SCALE;
      popup_data->tag_data[i].bounds.height     /= PANGO_SCALE;
      if (popup_data->tag_data[i].bounds.width + x + GIMP_TAG_POPUP_MARGIN > width)
        {
          x = GIMP_TAG_POPUP_MARGIN;
          y += line_height;
        }

      popup_data->tag_data[i].bounds.x = x;
      popup_data->tag_data[i].bounds.y = y;

      x += popup_data->tag_data[i].bounds.width + space_width;
    }
  height = y + line_height + GIMP_TAG_POPUP_MARGIN;

  return height;
}

static gboolean
gimp_combo_tag_entry_popup_expose (GtkWidget           *widget,
                                   GdkEventExpose      *event,
                                   PopupData           *popup_data)
{
  PangoRenderer        *renderer;
  gint                  i;

  renderer = gdk_pango_renderer_get_default (gtk_widget_get_screen (widget));
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), widget->style->black_gc);
  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer),
                                   widget->window);

  for (i = 0; i < popup_data->tag_count; i++)
    {
      pango_layout_set_text (popup_data->layout,
                             g_quark_to_string (popup_data->tag_data[i].tag), -1);
      if (popup_data->tag_data[i].selected)
        {
          pango_layout_set_attributes (popup_data->layout,
                                       popup_data->combo_entry->selected_item_attr);
        }
      else
        {
          pango_layout_set_attributes (popup_data->layout,
                                       popup_data->combo_entry->normal_item_attr);
        }

      pango_renderer_draw_layout (renderer, popup_data->layout,
                                  popup_data->tag_data[i].bounds.x * PANGO_SCALE,
                                  popup_data->tag_data[i].bounds.y * PANGO_SCALE);
    }

  gdk_pango_renderer_set_drawable (GDK_PANGO_RENDERER (renderer), NULL);
  gdk_pango_renderer_set_gc (GDK_PANGO_RENDERER (renderer), NULL);

  return FALSE;
}

static gboolean
gimp_combo_tag_entry_popup_event (GtkWidget          *widget,
                                  GdkEvent           *event,
                                  PopupData          *popup_data)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;

      button_event = (GdkEventButton *) event;

      gdk_window_get_pointer (widget->window, &x, &y, NULL);

      if (button_event->window != popup_data->drawing_area->window
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
  else if (event->type == GDK_GRAB_BROKEN)
    {
      gtk_grab_remove (widget);
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  GDK_CURRENT_TIME);
      gtk_widget_destroy (widget);
    }

  return FALSE;
}

static gboolean
gimp_combo_tag_entry_drawing_area_event (GtkWidget          *widget,
                                         GdkEvent           *event,
                                         PopupData          *popup_data)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton   *button_event;
      gint              x;
      gint              y;
      gint              i;
      GdkRectangle     *bounds;
      GimpTag           tag;

      button_event = (GdkEventButton *) event;
      x = button_event->x;
      y = button_event->y;

      for (i = 0; i < popup_data->tag_count; i++)
        {
          bounds = &popup_data->tag_data[i].bounds;
          if (x >= bounds->x
              && y >= bounds->y
              && x < bounds->x + bounds->width
              && y < bounds->y + bounds->height)
            {
              tag = popup_data->tag_data[i].tag;
              gimp_combo_tag_entry_toggle_tag (popup_data->combo_entry,
                                               &popup_data->tag_data[i]);
              gtk_widget_queue_draw (widget);
              break;
            }
        }
    }

  return FALSE;
}

static void
gimp_combo_tag_entry_toggle_tag (GimpComboTagEntry     *combo_entry,
                                 PopupTagData          *tag_data)
{
  gchar               **current_tags;
  GString              *tag_str;
  const gchar          *tag;
  gint                  length;
  gint                  i;
  gboolean              tag_toggled_off = FALSE;

  tag_data->selected = ! tag_data->selected;

  tag = gimp_tag_to_string (tag_data->tag);
  current_tags = gimp_tag_entry_parse_tags (GIMP_TAG_ENTRY (combo_entry->tag_entry));
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
              g_string_append (tag_str, ", ");
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
          g_string_append (tag_str, ", ");
        }
      g_string_append (tag_str, tag);
    }

  gimp_tag_entry_set_tag_string (GIMP_TAG_ENTRY (combo_entry->tag_entry),
                                 tag_str->str);

  g_string_free (tag_str, TRUE);
  g_strfreev (current_tags);
}

static void
popup_data_destroy (PopupData          *popup_data)
{
  popup_data->combo_entry->popup = NULL;

  if (popup_data->layout)
    {
      g_object_unref (popup_data->layout);
    }
  if (popup_data->context)
    {
      g_object_unref (popup_data->context);
    }

  g_free (popup_data->tag_data);
  g_free (popup_data);
}

