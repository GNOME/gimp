/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacursorview.h
 * Copyright (C) 2005-2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CURSOR_VIEW_H__
#define __LIGMA_CURSOR_VIEW_H__


#include "widgets/ligmaeditor.h"


#define LIGMA_TYPE_CURSOR_VIEW            (ligma_cursor_view_get_type ())
#define LIGMA_CURSOR_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CURSOR_VIEW, LigmaCursorView))
#define LIGMA_CURSOR_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CURSOR_VIEW, LigmaCursorViewClass))
#define LIGMA_IS_CURSOR_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CURSOR_VIEW))
#define LIGMA_IS_CURSOR_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CURSOR_VIEW))
#define LIGMA_CURSOR_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CURSOR_VIEW, LigmaCursorViewClass))


typedef struct _LigmaCursorViewClass   LigmaCursorViewClass;
typedef struct _LigmaCursorViewPrivate LigmaCursorViewPrivate;

struct _LigmaCursorView
{
  LigmaEditor             parent_instance;

  LigmaCursorViewPrivate *priv;
};

struct _LigmaCursorViewClass
{
  LigmaEditorClass  parent_class;
};


GType       ligma_cursor_view_get_type          (void) G_GNUC_CONST;

GtkWidget * ligma_cursor_view_new               (Ligma            *ligma,
                                                LigmaMenuFactory *menu_factory);

void        ligma_cursor_view_set_sample_merged (LigmaCursorView  *view,
                                                gboolean         sample_merged);
gboolean    ligma_cursor_view_get_sample_merged (LigmaCursorView  *view);

void        ligma_cursor_view_update_cursor     (LigmaCursorView  *view,
                                                LigmaImage       *image,
                                                LigmaUnit         shell_unit,
                                                gdouble          x,
                                                gdouble          y);
void        ligma_cursor_view_clear_cursor      (LigmaCursorView  *view);


#endif /* __LIGMA_CURSOR_VIEW_H__ */
