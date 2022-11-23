/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselect.h
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

#ifndef __LIGMA_COLOR_SELECT_H__
#define __LIGMA_COLOR_SELECT_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_SELECT            (ligma_color_select_get_type ())
#define LIGMA_COLOR_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_SELECT, LigmaColorSelect))
#define LIGMA_IS_COLOR_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_SELECT))


GType   ligma_color_select_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_COLOR_SELECT_H__ */
