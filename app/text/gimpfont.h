/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafont.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
 *                    Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_FONT_H__
#define __LIGMA_FONT_H__


#include "core/ligmadata.h"


#define LIGMA_TYPE_FONT            (ligma_font_get_type ())
#define LIGMA_FONT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FONT, LigmaFont))
#define LIGMA_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FONT, LigmaFontClass))
#define LIGMA_IS_FONT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FONT))
#define LIGMA_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FONT))
#define LIGMA_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FONT, LigmaFontClass))


typedef struct _LigmaFontClass LigmaFontClass;


GType      ligma_font_get_type     (void) G_GNUC_CONST;

LigmaData * ligma_font_get_standard (void);


#endif /* __LIGMA_FONT_H__ */
