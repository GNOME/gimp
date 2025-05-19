/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselectorpalette.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimppalette.h"

#include "gimpcolorselectorpalette.h"
#include "gimphelp-ids.h"
#include "gimppaletteview.h"
#include "gimpviewrendererpalette.h"

#include "gimp-intl.h"


static void   gimp_color_selector_palette_set_color  (GimpColorSelector *selector,
                                                      GeglColor         *color);
static void   gimp_color_selector_palette_set_config (GimpColorSelector *selector,
                                                      GimpColorConfig   *config);


G_DEFINE_TYPE (GimpColorSelectorPalette, gimp_color_selector_palette,
               GIMP_TYPE_COLOR_SELECTOR)

#define parent_class gimp_color_selector_palette_parent_class


static void
gimp_color_selector_palette_class_init (GimpColorSelectorPaletteClass *klass)
{
  GimpColorSelectorClass *selector_class = GIMP_COLOR_SELECTOR_CLASS (klass);

  selector_class->name       = _("Palette");
  selector_class->help_id    = GIMP_HELP_COLORSELECTOR_PALETTE;
  selector_class->icon_name  = GIMP_ICON_PALETTE;
  selector_class->set_color  = gimp_color_selector_palette_set_color;
  selector_class->set_config = gimp_color_selector_palette_set_config;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass),
                                 "GimpColorSelectorPalette");
}

static void
gimp_color_selector_palette_init (GimpColorSelectorPalette *select)
{
}

static void
gimp_color_selector_palette_set_color (GimpColorSelector *selector,
                                       GeglColor         *color)
{
  GimpColorSelectorPalette *select = GIMP_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      GimpPalette *palette = gimp_context_get_palette (select->context);

      if (palette && gimp_palette_get_n_colors (palette) > 0)
        {
          GimpPaletteEntry *entry;

          entry = gimp_palette_find_entry (palette, color,
                                           GIMP_PALETTE_VIEW (select->view)->selected);

          if (entry)
            gimp_palette_view_select_entry (GIMP_PALETTE_VIEW (select->view),
                                            entry);
        }
    }
}

static void
gimp_color_selector_palette_palette_changed (GimpContext              *context,
                                             GimpPalette              *palette,
                                             GimpColorSelectorPalette *select)
{
  gimp_view_set_viewable (GIMP_VIEW (select->view), GIMP_VIEWABLE (palette));

  if (palette != NULL)
    {
      gchar *palette_name;

      g_object_get (palette, "name", &palette_name, NULL);
      gtk_label_set_text (GTK_LABEL (select->name_label), palette_name);
      g_free (palette_name);
    }
}

static void
gimp_color_selector_palette_entry_clicked (GimpPaletteView   *view,
                                           GimpPaletteEntry  *entry,
                                           GdkModifierType    state,
                                           GimpColorSelector *selector)
{
  gimp_color_selector_set_color (selector, entry->color);
}

static void
gimp_color_selector_palette_set_config (GimpColorSelector *selector,
                                        GimpColorConfig   *config)
{
  GimpColorSelectorPalette *select = GIMP_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      g_signal_handlers_disconnect_by_func (select->context,
                                            gimp_color_selector_palette_palette_changed,
                                            select);
      gimp_view_renderer_set_context (GIMP_VIEW (select->view)->renderer,
                                      NULL);

      g_clear_object (&select->context);
    }

  if (config)
    select->context = g_object_get_data (G_OBJECT (config), "gimp-context");

  if (select->context)
    {
      g_object_ref (select->context);

      if (! select->view)
        {
          select->view = gimp_view_new_full_by_types (select->context,
                                                      GIMP_TYPE_PALETTE_VIEW,
                                                      GIMP_TYPE_PALETTE,
                                                      100, 100, 0,
                                                      FALSE, TRUE, FALSE);
          gimp_view_set_expand (GIMP_VIEW (select->view), TRUE);
          gimp_view_renderer_palette_set_cell_size
            (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (select->view)->renderer),
             -1);
          gimp_view_renderer_palette_set_draw_grid
            (GIMP_VIEW_RENDERER_PALETTE (GIMP_VIEW (select->view)->renderer),
             TRUE);
          gtk_box_pack_start (GTK_BOX (select), select->view, TRUE, TRUE, 0);
          gtk_widget_set_visible (select->view, TRUE);

          g_signal_connect (select->view, "entry-clicked",
                            G_CALLBACK (gimp_color_selector_palette_entry_clicked),
                            select);

          select->name_label = gtk_label_new (NULL);
          gtk_label_set_ellipsize (GTK_LABEL (select->name_label),
                                              PANGO_ELLIPSIZE_END);
          gtk_widget_set_halign (select->name_label, GTK_ALIGN_START);

          gtk_box_pack_start (GTK_BOX (select), select->name_label, FALSE,
                              FALSE, 6);
          gtk_widget_set_visible (select->name_label, TRUE);
        }
      else
        {
          gimp_view_renderer_set_context (GIMP_VIEW (select->view)->renderer,
                                          select->context);
        }

      g_signal_connect_object (select->context, "palette-changed",
                               G_CALLBACK (gimp_color_selector_palette_palette_changed),
                               select, 0);

      gimp_color_selector_palette_palette_changed (select->context,
                                                   gimp_context_get_palette (select->context),
                                                   select);
    }
}
