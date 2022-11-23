/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafontselect.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_FONT_SELECT_H__
#define __LIGMA_FONT_SELECT_H__

#include "ligmapdbdialog.h"

G_BEGIN_DECLS


#define LIGMA_TYPE_FONT_SELECT            (ligma_font_select_get_type ())
#define LIGMA_FONT_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FONT_SELECT, LigmaFontSelect))
#define LIGMA_FONT_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FONT_SELECT, LigmaFontSelectClass))
#define LIGMA_IS_FONT_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FONT_SELECT))
#define LIGMA_IS_FONT_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FONT_SELECT))
#define LIGMA_FONT_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FONT_SELECT, LigmaFontSelectClass))


typedef struct _LigmaFontSelectClass  LigmaFontSelectClass;

struct _LigmaFontSelect
{
  LigmaPdbDialog  parent_instance;
};

struct _LigmaFontSelectClass
{
  LigmaPdbDialogClass  parent_class;
};


GType  ligma_font_select_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_FONT_SELECT_H__ */
