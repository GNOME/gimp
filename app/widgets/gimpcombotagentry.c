/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcombotagentry.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptag.h"
#include "core/gimptagged.h"
#include "core/gimptaggedcontainer.h"
#include "core/gimpviewable.h"

#include "gimptagentry.h"
#include "gimptagpopup.h"
#include "gimpcombotagentry.h"


static void     gimp_combo_tag_entry_constructed       (GObject              *object);
static void     gimp_combo_tag_entry_dispose           (GObject              *object);

static gboolean gimp_combo_tag_entry_draw              (GtkWidget            *widget,
                                                        cairo_t              *cr);
static void     gimp_combo_tag_entry_style_updated     (GtkWidget            *widget);

static void     gimp_combo_tag_entry_icon_press        (GtkWidget            *widget,
                                                        GtkEntryIconPosition  icon_pos,
                                                        GdkEvent             *event,
                                                        gpointer              user_data);

static void     gimp_combo_tag_entry_popup_destroy     (GtkWidget            *widget,
                                                        GimpComboTagEntry    *entry);

static void     gimp_combo_tag_entry_tag_count_changed (GimpTaggedContainer  *container,
                                                        gint                  tag_count,
                                                        GimpComboTagEntry    *entry);


G_DEFINE_TYPE (GimpComboTagEntry, gimp_combo_tag_entry, GIMP_TYPE_TAG_ENTRY);

#define parent_class gimp_combo_tag_entry_parent_class


static void
gimp_combo_tag_entry_class_init (GimpComboTagEntryClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_combo_tag_entry_constructed;
  object_class->dispose       = gimp_combo_tag_entry_dispose;

  widget_class->draw          = gimp_combo_tag_entry_draw;
  widget_class->style_updated = gimp_combo_tag_entry_style_updated;
}

static void
gimp_combo_tag_entry_init (GimpComboTagEntry *entry)
{
  entry->popup                 = NULL;
  entry->normal_item_attr      = NULL;
  entry->selected_item_attr    = NULL;
  entry->insensitive_item_attr = NULL;

  gtk_widget_add_events (GTK_WIDGET (entry),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_POINTER_MOTION_MASK);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     GIMP_ICON_GO_DOWN);

  g_signal_connect (entry, "icon-press",
                    G_CALLBACK (gimp_combo_tag_entry_icon_press),
                    NULL);
}

static void
gimp_combo_tag_entry_constructed (GObject *object)
{
  GimpComboTagEntry *entry = GIMP_COMBO_TAG_ENTRY (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (GIMP_TAG_ENTRY (entry)->container,
                           "tag-count-changed",
                           G_CALLBACK (gimp_combo_tag_entry_tag_count_changed),
                           entry, 0);
}

static void
gimp_combo_tag_entry_dispose (GObject *object)
{
  GimpComboTagEntry *combo_entry = GIMP_COMBO_TAG_ENTRY (object);

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

static gboolean
gimp_combo_tag_entry_draw (GtkWidget *widget,
                           cairo_t   *cr)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GdkRectangle     icon_area;
  gint             x, y;

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  gtk_entry_get_icon_area (GTK_ENTRY (widget), GTK_ENTRY_ICON_SECONDARY,
                           &icon_area);

  x = icon_area.x + (icon_area.width  - 8) / 2;
  y = icon_area.y + (icon_area.height - 8) / 2;

  gtk_render_arrow (style, cr, G_PI, x, y, 8);

  return FALSE;
}

static void
gimp_combo_tag_entry_style_updated (GtkWidget *widget)
{
  GimpComboTagEntry          *entry = GIMP_COMBO_TAG_ENTRY (widget);
  GtkStyleContext            *style = gtk_widget_get_style_context (widget);
  GdkRGBA                     color;
  const PangoFontDescription *font_desc;
  PangoAttribute             *attribute;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (entry->normal_item_attr)
    pango_attr_list_unref (entry->normal_item_attr);
  entry->normal_item_attr = pango_attr_list_new ();

  font_desc = gtk_style_context_get_font (style, 0);
  attribute = pango_attr_font_desc_new (font_desc);
  pango_attr_list_insert (entry->normal_item_attr, attribute);

  gtk_style_context_get_color (style, 0, &color);
  attribute = pango_attr_foreground_new (color.red   * 65535.99,
                                         color.green * 65535.99,
                                         color.blue  * 65535.99);
  pango_attr_list_insert (entry->normal_item_attr, attribute);

  if (entry->selected_item_attr)
    pango_attr_list_unref (entry->selected_item_attr);
  entry->selected_item_attr = pango_attr_list_copy (entry->normal_item_attr);

  gtk_style_context_get_color (style, GTK_STATE_FLAG_SELECTED, &color);
  attribute = pango_attr_foreground_new (color.red   * 65535.99,
                                         color.green * 65535.99,
                                         color.blue  * 65535.99);
  pango_attr_list_insert (entry->selected_item_attr, attribute);
  gtk_style_context_get_background_color (style, GTK_STATE_FLAG_SELECTED, &color);
  attribute = pango_attr_background_new (color.red   * 65535.99,
                                         color.green * 65535.99,
                                         color.blue  * 65535.99);
  pango_attr_list_insert (entry->selected_item_attr, attribute);

  if (entry->insensitive_item_attr)
    pango_attr_list_unref (entry->insensitive_item_attr);
  entry->insensitive_item_attr = pango_attr_list_copy (entry->normal_item_attr);

  gtk_style_context_get_color (style, GTK_STATE_FLAG_INSENSITIVE, &color);
  attribute = pango_attr_foreground_new (color.red   * 65535.99,
                                         color.green * 65535.99,
                                         color.blue  * 65535.99);
  pango_attr_list_insert (entry->insensitive_item_attr, attribute);
  gtk_style_context_get_background_color (style, GTK_STATE_FLAG_INSENSITIVE, &color);
  attribute = pango_attr_background_new (color.red   * 65535.99,
                                         color.green * 65535.99,
                                         color.blue  * 65535.99);
  pango_attr_list_insert (entry->insensitive_item_attr, attribute);

  gtk_style_context_get_background_color (style, GTK_STATE_FLAG_SELECTED,
                                          &entry->selected_item_color);
}

/**
 * gimp_combo_tag_entry_new:
 * @container: a tagged container to be used.
 * @mode:      tag entry mode to work in.
 *
 * Creates a new #GimpComboTagEntry widget which extends #GimpTagEntry by
 * adding ability to pick tags using popup window (similar to combo box).
 *
 * Return value: a new #GimpComboTagEntry widget.
 **/
GtkWidget *
gimp_combo_tag_entry_new (GimpTaggedContainer *container,
                          GimpTagEntryMode     mode)
{
  g_return_val_if_fail (GIMP_IS_TAGGED_CONTAINER (container), NULL);

  return g_object_new (GIMP_TYPE_COMBO_TAG_ENTRY,
                       "container", container,
                       "mode",      mode,
                       NULL);
}

static void
gimp_combo_tag_entry_icon_press (GtkWidget            *widget,
                                 GtkEntryIconPosition  icon_pos,
                                 GdkEvent             *event,
                                 gpointer              user_data)
{
  GimpComboTagEntry *entry = GIMP_COMBO_TAG_ENTRY (widget);

  if (! entry->popup)
    {
      GimpTaggedContainer *container = GIMP_TAG_ENTRY (entry)->container;
      gint                 tag_count;

      tag_count = gimp_tagged_container_get_tag_count (container);

      if (tag_count > 0 && ! GIMP_TAG_ENTRY (entry)->has_invalid_tags)
        {
          entry->popup = gimp_tag_popup_new (entry);
          g_signal_connect (entry->popup, "destroy",
                            G_CALLBACK (gimp_combo_tag_entry_popup_destroy),
                            entry);
          gimp_tag_popup_show (GIMP_TAG_POPUP (entry->popup), event);
        }
    }
  else
    {
      gtk_widget_destroy (entry->popup);
    }
}

static void
gimp_combo_tag_entry_popup_destroy (GtkWidget         *widget,
                                    GimpComboTagEntry *entry)
{
  entry->popup = NULL;
  gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void
gimp_combo_tag_entry_tag_count_changed (GimpTaggedContainer *container,
                                        gint                 tag_count,
                                        GimpComboTagEntry   *entry)
{
  gboolean sensitive;

  sensitive = tag_count > 0 && ! GIMP_TAG_ENTRY (entry)->has_invalid_tags;

  gtk_entry_set_icon_sensitive (GTK_ENTRY (entry),
                                GTK_ENTRY_ICON_SECONDARY,
                                sensitive);
}
