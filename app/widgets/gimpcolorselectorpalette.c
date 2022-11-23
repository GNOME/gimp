/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselectorpalette.c
 * Copyright (C) 2006 Michael Natterer <mitch@ligma.org>
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

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"
#include "core/ligmapalette.h"

#include "ligmacolorselectorpalette.h"
#include "ligmapaletteview.h"
#include "ligmaviewrendererpalette.h"

#include "ligma-intl.h"


static void   ligma_color_selector_palette_set_color  (LigmaColorSelector *selector,
                                                      const LigmaRGB     *rgb,
                                                      const LigmaHSV     *hsv);
static void   ligma_color_selector_palette_set_config (LigmaColorSelector *selector,
                                                      LigmaColorConfig   *config);


G_DEFINE_TYPE (LigmaColorSelectorPalette, ligma_color_selector_palette,
               LIGMA_TYPE_COLOR_SELECTOR)

#define parent_class ligma_color_selector_palette_parent_class


static void
ligma_color_selector_palette_class_init (LigmaColorSelectorPaletteClass *klass)
{
  LigmaColorSelectorClass *selector_class = LIGMA_COLOR_SELECTOR_CLASS (klass);

  selector_class->name       = _("Palette");
  selector_class->help_id    = "ligma-colorselector-palette";
  selector_class->icon_name  = LIGMA_ICON_PALETTE;
  selector_class->set_color  = ligma_color_selector_palette_set_color;
  selector_class->set_config = ligma_color_selector_palette_set_config;

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass),
                                 "LigmaColorSelectorPalette");
}

static void
ligma_color_selector_palette_init (LigmaColorSelectorPalette *select)
{
}

static void
ligma_color_selector_palette_set_color (LigmaColorSelector *selector,
                                       const LigmaRGB     *rgb,
                                       const LigmaHSV     *hsv)
{
  LigmaColorSelectorPalette *select = LIGMA_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      LigmaPalette *palette = ligma_context_get_palette (select->context);

      if (palette && ligma_palette_get_n_colors (palette) > 0)
        {
          LigmaPaletteEntry *entry;

          entry = ligma_palette_find_entry (palette, rgb,
                                           LIGMA_PALETTE_VIEW (select->view)->selected);

          if (entry)
            ligma_palette_view_select_entry (LIGMA_PALETTE_VIEW (select->view),
                                            entry);
        }
    }
}

static void
ligma_color_selector_palette_palette_changed (LigmaContext              *context,
                                             LigmaPalette              *palette,
                                             LigmaColorSelectorPalette *select)
{
  ligma_view_set_viewable (LIGMA_VIEW (select->view), LIGMA_VIEWABLE (palette));
}

static void
ligma_color_selector_palette_entry_clicked (LigmaPaletteView   *view,
                                           LigmaPaletteEntry  *entry,
                                           GdkModifierType    state,
                                           LigmaColorSelector *selector)
{
  selector->rgb = entry->color;
  ligma_rgb_to_hsv (&selector->rgb, &selector->hsv);

  ligma_color_selector_emit_color_changed (selector);
}

static void
ligma_color_selector_palette_set_config (LigmaColorSelector *selector,
                                        LigmaColorConfig   *config)
{
  LigmaColorSelectorPalette *select = LIGMA_COLOR_SELECTOR_PALETTE (selector);

  if (select->context)
    {
      g_signal_handlers_disconnect_by_func (select->context,
                                            ligma_color_selector_palette_palette_changed,
                                            select);
      ligma_view_renderer_set_context (LIGMA_VIEW (select->view)->renderer,
                                      NULL);

      g_clear_object (&select->context);
    }

  if (config)
    select->context = g_object_get_data (G_OBJECT (config), "ligma-context");

  if (select->context)
    {
      g_object_ref (select->context);

      if (! select->view)
        {
          select->view = ligma_view_new_full_by_types (select->context,
                                                      LIGMA_TYPE_PALETTE_VIEW,
                                                      LIGMA_TYPE_PALETTE,
                                                      100, 100, 0,
                                                      FALSE, TRUE, FALSE);
          ligma_view_set_expand (LIGMA_VIEW (select->view), TRUE);
          ligma_view_renderer_palette_set_cell_size
            (LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (select->view)->renderer),
             -1);
          ligma_view_renderer_palette_set_draw_grid
            (LIGMA_VIEW_RENDERER_PALETTE (LIGMA_VIEW (select->view)->renderer),
             TRUE);
          gtk_box_pack_start (GTK_BOX (select), select->view, TRUE, TRUE, 0);
          gtk_widget_show (select->view);

          g_signal_connect (select->view, "entry-clicked",
                            G_CALLBACK (ligma_color_selector_palette_entry_clicked),
                            select);
        }
      else
        {
          ligma_view_renderer_set_context (LIGMA_VIEW (select->view)->renderer,
                                          select->context);
        }

      g_signal_connect_object (select->context, "palette-changed",
                               G_CALLBACK (ligma_color_selector_palette_palette_changed),
                               select, 0);

      ligma_color_selector_palette_palette_changed (select->context,
                                                   ligma_context_get_palette (select->context),
                                                   select);
    }
}
