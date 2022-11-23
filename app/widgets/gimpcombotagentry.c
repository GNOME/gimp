/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacombotagentry.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmatag.h"
#include "core/ligmatagged.h"
#include "core/ligmataggedcontainer.h"
#include "core/ligmaviewable.h"

#include "ligmatagentry.h"
#include "ligmatagpopup.h"
#include "ligmacombotagentry.h"


static void     ligma_combo_tag_entry_constructed       (GObject              *object);

static gboolean ligma_combo_tag_entry_draw              (GtkWidget            *widget,
                                                        cairo_t              *cr);

static void     ligma_combo_tag_entry_icon_press        (GtkWidget            *widget,
                                                        GtkEntryIconPosition  icon_pos,
                                                        GdkEvent             *event,
                                                        gpointer              user_data);

static void     ligma_combo_tag_entry_popup_destroy     (GtkWidget            *widget,
                                                        LigmaComboTagEntry    *entry);

static void     ligma_combo_tag_entry_tag_count_changed (LigmaTaggedContainer  *container,
                                                        gint                  tag_count,
                                                        LigmaComboTagEntry    *entry);


G_DEFINE_TYPE (LigmaComboTagEntry, ligma_combo_tag_entry, LIGMA_TYPE_TAG_ENTRY);

#define parent_class ligma_combo_tag_entry_parent_class


static void
ligma_combo_tag_entry_class_init (LigmaComboTagEntryClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ligma_combo_tag_entry_constructed;

  widget_class->draw        = ligma_combo_tag_entry_draw;
}

static void
ligma_combo_tag_entry_init (LigmaComboTagEntry *entry)
{
  entry->popup = NULL;

  gtk_widget_add_events (GTK_WIDGET (entry),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_POINTER_MOTION_MASK);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     LIGMA_ICON_GO_DOWN);

  g_signal_connect (entry, "icon-press",
                    G_CALLBACK (ligma_combo_tag_entry_icon_press),
                    NULL);
}

static void
ligma_combo_tag_entry_constructed (GObject *object)
{
  LigmaComboTagEntry *entry = LIGMA_COMBO_TAG_ENTRY (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect_object (LIGMA_TAG_ENTRY (entry)->container,
                           "tag-count-changed",
                           G_CALLBACK (ligma_combo_tag_entry_tag_count_changed),
                           entry, 0);
}

static gboolean
ligma_combo_tag_entry_draw (GtkWidget *widget,
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

/**
 * ligma_combo_tag_entry_new:
 * @container: a tagged container to be used.
 * @mode:      tag entry mode to work in.
 *
 * Creates a new #LigmaComboTagEntry widget which extends #LigmaTagEntry by
 * adding ability to pick tags using popup window (similar to combo box).
 *
 * Returns: a new #LigmaComboTagEntry widget.
 **/
GtkWidget *
ligma_combo_tag_entry_new (LigmaTaggedContainer *container,
                          LigmaTagEntryMode     mode)
{
  g_return_val_if_fail (LIGMA_IS_TAGGED_CONTAINER (container), NULL);

  return g_object_new (LIGMA_TYPE_COMBO_TAG_ENTRY,
                       "container", container,
                       "mode",      mode,
                       NULL);
}

static void
ligma_combo_tag_entry_icon_press (GtkWidget            *widget,
                                 GtkEntryIconPosition  icon_pos,
                                 GdkEvent             *event,
                                 gpointer              user_data)
{
  LigmaComboTagEntry *entry = LIGMA_COMBO_TAG_ENTRY (widget);

  if (! entry->popup)
    {
      LigmaTaggedContainer *container = LIGMA_TAG_ENTRY (entry)->container;
      gint                 tag_count;

      tag_count = ligma_tagged_container_get_tag_count (container);

      if (tag_count > 0 && ! LIGMA_TAG_ENTRY (entry)->has_invalid_tags)
        {
          entry->popup = ligma_tag_popup_new (entry);
          g_signal_connect (entry->popup, "destroy",
                            G_CALLBACK (ligma_combo_tag_entry_popup_destroy),
                            entry);
          ligma_tag_popup_show (LIGMA_TAG_POPUP (entry->popup), event);
        }
    }
  else
    {
      gtk_widget_destroy (entry->popup);
    }
}

static void
ligma_combo_tag_entry_popup_destroy (GtkWidget         *widget,
                                    LigmaComboTagEntry *entry)
{
  entry->popup = NULL;
  gtk_widget_grab_focus (GTK_WIDGET (entry));
}

static void
ligma_combo_tag_entry_tag_count_changed (LigmaTaggedContainer *container,
                                        gint                 tag_count,
                                        LigmaComboTagEntry   *entry)
{
  gboolean sensitive;

  sensitive = tag_count > 0 && ! LIGMA_TAG_ENTRY (entry)->has_invalid_tags;

  gtk_entry_set_icon_sensitive (GTK_ENTRY (entry),
                                GTK_ENTRY_ICON_SECONDARY,
                                sensitive);
}
