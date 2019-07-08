/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TEXT_STYLE_EDITOR_H__
#define __GIMP_TEXT_STYLE_EDITOR_H__


#define GIMP_TYPE_TEXT_STYLE_EDITOR            (gimp_text_style_editor_get_type ())
#define GIMP_TEXT_STYLE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_STYLE_EDITOR, GimpTextStyleEditor))
#define GIMP_TEXT_STYLE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_STYLE_EDITOR, GimpTextStyleEditorClass))
#define GIMP_IS_TEXT_STYLE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_STYLE_EDITOR))
#define GIMP_IS_TEXT_STYLE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_STYLE_EDITOR))
#define GIMP_TEXT_STYLE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEXT_STYLE_EDITOR, GimpTextStyleEditorClass))


typedef struct _GimpTextStyleEditorClass GimpTextStyleEditorClass;

struct _GimpTextStyleEditor
{
  GtkBox          parent_instance;

  Gimp           *gimp;
  GimpContext    *context;

  GimpText       *text; /* read-only for default values */
  GimpTextBuffer *buffer;

  GimpContainer  *fonts;
  gdouble         resolution_x;
  gdouble         resolution_y;

  GtkWidget      *upper_hbox;
  GtkWidget      *lower_hbox;

  GtkWidget      *font_entry;
  GtkWidget      *size_entry;

  GtkWidget      *color_button;

  GtkWidget      *clear_button;

  GtkWidget      *baseline_spinbutton;
  GtkAdjustment  *baseline_adjustment;

  GtkWidget      *kerning_spinbutton;
  GtkAdjustment  *kerning_adjustment;

  GList          *toggles;

  guint           update_idle_id;
};

struct _GimpTextStyleEditorClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_text_style_editor_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_text_style_editor_new       (Gimp                 *gimp,
                                              GimpText             *text,
                                              GimpTextBuffer       *buffer,
                                              GimpContainer        *fonts,
                                              gdouble               resolution_x,
                                              gdouble               resolution_y);

GList     * gimp_text_style_editor_list_tags (GimpTextStyleEditor  *editor,
                                              GList               **remove_tags);


#endif /*  __GIMP_TEXT_STYLE_EDITOR_H__  */
