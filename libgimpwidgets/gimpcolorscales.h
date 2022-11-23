/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscales.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_SCALES_H__
#define __LIGMA_COLOR_SCALES_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_SCALES            (ligma_color_scales_get_type ())
#define LIGMA_COLOR_SCALES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SCALES, LigmaColorScales))
#define LIGMA_IS_COLOR_SCALES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SCALES))


GType      ligma_color_scales_get_type        (void) G_GNUC_CONST;

void       ligma_color_scales_set_show_rgb_u8 (LigmaColorScales *scales,
                                              gboolean         show_rgb_u8);
gboolean   ligma_color_scales_get_show_rgb_u8 (LigmaColorScales *scales);


G_END_DECLS

#endif /* __LIGMA_COLOR_SCALES_H__ */
