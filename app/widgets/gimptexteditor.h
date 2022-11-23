/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextEditor
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_EDITOR_H__
#define __LIGMA_TEXT_EDITOR_H__


#define LIGMA_TYPE_TEXT_EDITOR    (ligma_text_editor_get_type ())
#define LIGMA_TEXT_EDITOR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_EDITOR, LigmaTextEditor))
#define LIGMA_IS_TEXT_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_EDITOR))


typedef struct _LigmaTextEditorClass  LigmaTextEditorClass;

struct _LigmaTextEditor
{
  LigmaDialog         parent_instance;

  /*<  private  >*/
  LigmaTextDirection  base_dir;
  gchar             *font_name;

  GtkWidget         *view;
  GtkWidget         *font_toggle;
  GtkWidget         *file_dialog;
  LigmaUIManager     *ui_manager;
};

struct _LigmaTextEditorClass
{
  LigmaDialogClass   parent_class;

  void (* text_changed) (LigmaTextEditor *editor);
  void (* dir_changed)  (LigmaTextEditor *editor);
};


GType               ligma_text_editor_get_type      (void) G_GNUC_CONST;
GtkWidget         * ligma_text_editor_new           (const gchar       *title,
                                                    GtkWindow         *parent,
                                                    Ligma              *ligma,
                                                    LigmaMenuFactory   *menu_factory,
                                                    LigmaText          *text,
                                                    LigmaTextBuffer    *text_buffer,
                                                    gdouble            xres,
                                                    gdouble            yres);

void                ligma_text_editor_set_text      (LigmaTextEditor    *editor,
                                                    const gchar       *text,
                                                    gint               len);
gchar             * ligma_text_editor_get_text      (LigmaTextEditor    *editor);

void                ligma_text_editor_set_direction (LigmaTextEditor    *editor,
                                                    LigmaTextDirection  base_dir);
LigmaTextDirection   ligma_text_editor_get_direction (LigmaTextEditor    *editor);

void                ligma_text_editor_set_font_name (LigmaTextEditor    *editor,
                                                    const gchar       *font_name);
const gchar       * ligma_text_editor_get_font_name (LigmaTextEditor    *editor);


#endif  /* __LIGMA_TEXT_EDITOR_H__ */
