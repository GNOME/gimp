/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscaleentry.h
 * Copyright (C) 2020 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_COLOR_SCALE_ENTRY_H__
#define __LIGMA_COLOR_SCALE_ENTRY_H__

#include <libligmawidgets/ligmascaleentry.h>

G_BEGIN_DECLS

#define LIGMA_TYPE_COLOR_SCALE_ENTRY (ligma_color_scale_entry_get_type ())
G_DECLARE_FINAL_TYPE (LigmaColorScaleEntry, ligma_color_scale_entry, LIGMA, COLOR_SCALE_ENTRY, LigmaScaleEntry)

GtkWidget     * ligma_color_scale_entry_new        (const gchar *text,
                                                   gdouble      value,
                                                   gdouble      lower,
                                                   gdouble      upper,
                                                   guint        digits);


G_END_DECLS

#endif /* __LIGMA_COLOR_SCALE_ENTRY_H__ */
