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
#include "gimppaletteview.h"
#include "gimpviewrendererpalette.h"

#include "gimp-intl.h"


static void   gimp_color_selector_palette_set_color  (GimpColorSelector *selector,
                                                      const GimpRGB     *rgb,
                                                      const GimpHSV     *hsv);
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
  selector_class->help_id    = "gimp-colorselector-palette";
  selector_class->icon_name  = GIMP_ICON_PALETTE;
  selector_class->set_color  = gimp_color_selector_palette_set_color;
  selector_class->set_config = gimp_color_selector_palette_set_config;
}

static void
gimp_color_selector_palette_init (GimpColorSelectorPalette *select)
{
}

static void
gimp_color_selector_palette_set_color (GimpColorSelector *selector,
                                       const GimpRGB     *rgb,
                                       const GimpHSV     *hsv)
{
  GimpColorSelectorPalette *select = GIMP_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      GimpPalette *palette = gimp_context_get_palette (select->context);

      if (palette && gimp_palette_get_n_colors (palette) > 0)
        {
          GimpPaletteEntry *entry;

          entry = gimp_palette_find_entry (palette, rgb,
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
}

static void
gimp_color_selector_palette_entry_clicked (GimpPaletteView   *view,
                                           GimpPaletteEntry  *entry,
                                           GdkModifierType    state,
                                           GimpColorSelector *selector)
{
  selector->rgb = entry->color;
  gimp_rgb_to_hsv (&selector->rgb, &selector->hsv);

  gimp_color_selector_color_changed (selector);
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
          GtkWidget *frame;

          frame = gtk_frame_new (NULL);
          gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
          gtk_box_pack_start (GTK_BOX (select), frame, TRUE, TRUE, 0);
          gtk_widget_show (frame);

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
          gtk_container_add (GTK_CONTAINER (frame), select->view);
          gtk_widget_show (select->view);

          g_signal_connect (select->view, "entry-clicked",
                            G_CALLBACK (gimp_color_selector_palette_entry_clicked),
                            select);
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
