/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvastextcursor.h
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CANVAS_TEXT_CURSOR_H__
#define __LIGMA_CANVAS_TEXT_CURSOR_H__


#include "ligmacanvasitem.h"


#define LIGMA_TYPE_CANVAS_TEXT_CURSOR            (ligma_canvas_text_cursor_get_type ())
#define LIGMA_CANVAS_TEXT_CURSOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_TEXT_CURSOR, LigmaCanvasTextCursor))
#define LIGMA_CANVAS_TEXT_CURSOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_TEXT_CURSOR, LigmaCanvasTextCursorClass))
#define LIGMA_IS_CANVAS_TEXT_CURSOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_TEXT_CURSOR))
#define LIGMA_IS_CANVAS_TEXT_CURSOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_TEXT_CURSOR))
#define LIGMA_CANVAS_TEXT_CURSOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_TEXT_CURSOR, LigmaCanvasTextCursorClass))


typedef struct _LigmaCanvasTextCursor      LigmaCanvasTextCursor;
typedef struct _LigmaCanvasTextCursorClass LigmaCanvasTextCursorClass;

struct _LigmaCanvasTextCursor
{
  LigmaCanvasItem  parent_instance;
};

struct _LigmaCanvasTextCursorClass
{
  LigmaCanvasItemClass  parent_class;
};


GType            ligma_canvas_text_cursor_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_text_cursor_new      (LigmaDisplayShell *shell,
                                                   PangoRectangle   *cursor,
                                                   gboolean          overwrite,
                                                   LigmaTextDirection direction);


#endif /* __LIGMA_CANVAS_RECTANGLE_H__ */
