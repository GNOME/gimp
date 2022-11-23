/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_STYLE_EDITOR_H__
#define __LIGMA_TEXT_STYLE_EDITOR_H__


#define LIGMA_TYPE_TEXT_STYLE_EDITOR            (ligma_text_style_editor_get_type ())
#define LIGMA_TEXT_STYLE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_STYLE_EDITOR, LigmaTextStyleEditor))
#define LIGMA_TEXT_STYLE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_STYLE_EDITOR, LigmaTextStyleEditorClass))
#define LIGMA_IS_TEXT_STYLE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_STYLE_EDITOR))
#define LIGMA_IS_TEXT_STYLE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_STYLE_EDITOR))
#define LIGMA_TEXT_STYLE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEXT_STYLE_EDITOR, LigmaTextStyleEditorClass))


typedef struct _LigmaTextStyleEditorClass LigmaTextStyleEditorClass;

struct _LigmaTextStyleEditor
{
  GtkBox          parent_instance;

  Ligma           *ligma;
  LigmaContext    *context;

  LigmaText       *text; /* read-only for default values */
  LigmaTextBuffer *buffer;

  LigmaContainer  *fonts;
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

struct _LigmaTextStyleEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_text_style_editor_get_type  (void) G_GNUC_CONST;

GtkWidget * ligma_text_style_editor_new       (Ligma                 *ligma,
                                              LigmaText             *text,
                                              LigmaTextBuffer       *buffer,
                                              LigmaContainer        *fonts,
                                              gdouble               resolution_x,
                                              gdouble               resolution_y);

GList     * ligma_text_style_editor_list_tags (LigmaTextStyleEditor  *editor,
                                              GList               **remove_tags);


#endif /*  __LIGMA_TEXT_STYLE_EDITOR_H__  */
