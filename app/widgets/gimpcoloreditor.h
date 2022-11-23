/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacoloreditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_EDITOR_H__
#define __LIGMA_COLOR_EDITOR_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_COLOR_EDITOR            (ligma_color_editor_get_type ())
#define LIGMA_COLOR_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_EDITOR, LigmaColorEditor))
#define LIGMA_COLOR_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_EDITOR, LigmaColorEditorClass))
#define LIGMA_IS_COLOR_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_EDITOR))
#define LIGMA_IS_COLOR_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_EDITOR))
#define LIGMA_COLOR_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_EDITOR, LigmaColorEditorClass))


typedef struct _LigmaColorEditorClass LigmaColorEditorClass;

struct _LigmaColorEditor
{
  LigmaEditor   parent_instance;

  LigmaContext *context;
  gboolean     edit_bg;

  GtkWidget   *hbox;
  GtkWidget   *notebook;
  GtkWidget   *fg_bg;
  GtkWidget   *hex_entry;
};

struct _LigmaColorEditorClass
{
  LigmaEditorClass  parent_class;
};


GType       ligma_color_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_color_editor_new      (LigmaContext *context);


#endif /* __LIGMA_COLOR_EDITOR_H__ */
