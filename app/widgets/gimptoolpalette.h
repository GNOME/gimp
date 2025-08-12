/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpalette.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_TOOL_PALETTE            (gimp_tool_palette_get_type ())
#define GIMP_TOOL_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_PALETTE, GimpToolPalette))
#define GIMP_TOOL_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_PALETTE, GimpToolPaletteClass))
#define GIMP_IS_TOOL_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_PALETTE))
#define GIMP_IS_TOOL_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_PALETTE))
#define GIMP_TOOL_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_PALETTE, GimpToolPaletteClass))


typedef struct _GimpToolPaletteClass GimpToolPaletteClass;

struct _GimpToolPalette
{
  GtkToolPalette  parent_instance;
};

struct _GimpToolPaletteClass
{
  GtkToolPaletteClass  parent_class;
};


GType       gimp_tool_palette_get_type        (void) G_GNUC_CONST;

GtkWidget * gimp_tool_palette_new             (void);
void        gimp_tool_palette_set_toolbox     (GimpToolPalette   *palette,
                                               GimpToolbox       *toolbox);
gboolean    gimp_tool_palette_get_button_size (GimpToolPalette   *palette,
                                               gint              *width,
                                               gint              *height);
