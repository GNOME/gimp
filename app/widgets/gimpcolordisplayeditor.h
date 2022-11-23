/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacolordisplayeditor.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_DISPLAY_EDITOR_H__
#define __LIGMA_COLOR_DISPLAY_EDITOR_H__


#define LIGMA_TYPE_COLOR_DISPLAY_EDITOR            (ligma_color_display_editor_get_type ())
#define LIGMA_COLOR_DISPLAY_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_DISPLAY_EDITOR, LigmaColorDisplayEditor))
#define LIGMA_COLOR_DISPLAY_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_DISPLAY_EDITOR, LigmaColorDisplayEditorClass))
#define LIGMA_IS_COLOR_DISPLAY_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_DISPLAY_EDITOR))
#define LIGMA_IS_COLOR_DISPLAY_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_DISPLAY_EDITOR))
#define LIGMA_COLOR_DISPLAY_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_DISPLAY_EDITOR, LigmaColorDisplayEditorClass))


typedef struct _LigmaColorDisplayEditorClass  LigmaColorDisplayEditorClass;

struct _LigmaColorDisplayEditor
{
  GtkBox                 parent_instance;

  Ligma                  *ligma;
  LigmaColorDisplayStack *stack;
  LigmaColorConfig       *config;
  LigmaColorManaged      *managed;

  GtkListStore          *src;
  GtkListStore          *dest;

  GtkTreeSelection      *src_sel;
  GtkTreeSelection      *dest_sel;

  LigmaColorDisplay      *selected;

  GtkWidget             *add_button;

  GtkWidget             *remove_button;
  GtkWidget             *up_button;
  GtkWidget             *down_button;

  GtkWidget             *config_frame;
  GtkWidget             *config_box;
  GtkWidget             *config_widget;

  GtkWidget             *reset_button;
};

struct _LigmaColorDisplayEditorClass
{
  GtkBoxClass parent_class;
};


GType       ligma_color_display_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_color_display_editor_new      (Ligma                  *ligma,
                                                LigmaColorDisplayStack *stack,
                                                LigmaColorConfig       *config,
                                                LigmaColorManaged      *managed);


#endif  /*  __LIGMA_COLOR_DISPLAY_EDITOR_H__  */
