/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolordisplayeditor.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_COLOR_DISPLAY_EDITOR_H__
#define __GIMP_COLOR_DISPLAY_EDITOR_H__


#define GIMP_TYPE_COLOR_DISPLAY_EDITOR            (gimp_color_display_editor_get_type ())
#define GIMP_COLOR_DISPLAY_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_DISPLAY_EDITOR, GimpColorDisplayEditor))
#define GIMP_COLOR_DISPLAY_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_DISPLAY_EDITOR, GimpColorDisplayEditorClass))
#define GIMP_IS_COLOR_DISPLAY_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_DISPLAY_EDITOR))
#define GIMP_IS_COLOR_DISPLAY_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_DISPLAY_EDITOR))
#define GIMP_COLOR_DISPLAY_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_DISPLAY_EDITOR, GimpColorDisplayEditorClass))


typedef struct _GimpColorDisplayEditorClass  GimpColorDisplayEditorClass;

struct _GimpColorDisplayEditor
{
  GtkBox                 parent_instance;

  Gimp                  *gimp;
  GimpColorDisplayStack *stack;
  GimpColorConfig       *config;
  GimpColorManaged      *managed;

  GtkListStore          *src;
  GtkListStore          *dest;

  GtkTreeSelection      *src_sel;
  GtkTreeSelection      *dest_sel;

  GimpColorDisplay      *selected;

  GtkWidget             *add_button;

  GtkWidget             *remove_button;
  GtkWidget             *up_button;
  GtkWidget             *down_button;

  GtkWidget             *config_frame;
  GtkWidget             *config_box;
  GtkWidget             *config_widget;

  GtkWidget             *reset_button;
};

struct _GimpColorDisplayEditorClass
{
  GtkBoxClass parent_class;
};


GType       gimp_color_display_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_color_display_editor_new      (Gimp                  *gimp,
                                                GimpColorDisplayStack *stack,
                                                GimpColorConfig       *config,
                                                GimpColorManaged      *managed);


#endif  /*  __GIMP_COLOR_DISPLAY_EDITOR_H__  */
