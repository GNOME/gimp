/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextEditor
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TEXT_EDITOR_H__
#define __GIMP_TEXT_EDITOR_H__


#define GIMP_TYPE_TEXT_EDITOR    (gimp_text_editor_get_type ())
#define GIMP_TEXT_EDITOR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_EDITOR, GimpTextEditor))
#define GIMP_IS_TEXT_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_EDITOR))


typedef struct _GimpTextEditorClass  GimpTextEditorClass;

struct _GimpTextEditor
{
  GimpDialog         parent_instance;

  /*<  private  >*/
  GimpTextDirection  base_dir;
  gchar             *font_name;

  GtkWidget         *view;
  GtkWidget         *font_toggle;
  GtkWidget         *file_dialog;
  GimpUIManager     *ui_manager;
};

struct _GimpTextEditorClass
{
  GimpDialogClass   parent_class;

  void (* text_changed) (GimpTextEditor *editor);
  void (* dir_changed)  (GimpTextEditor *editor);
};


GType               gimp_text_editor_get_type      (void) G_GNUC_CONST;
GtkWidget         * gimp_text_editor_new           (const gchar       *title,
                                                    GtkWindow         *parent,
                                                    Gimp              *gimp,
                                                    GimpMenuFactory   *menu_factory,
                                                    GimpText          *text,
                                                    GimpTextBuffer    *text_buffer,
                                                    gdouble            xres,
                                                    gdouble            yres);

void                gimp_text_editor_set_text      (GimpTextEditor    *editor,
                                                    const gchar       *text,
                                                    gint               len);
gchar             * gimp_text_editor_get_text      (GimpTextEditor    *editor);

void                gimp_text_editor_set_direction (GimpTextEditor    *editor,
                                                    GimpTextDirection  base_dir);
GimpTextDirection   gimp_text_editor_get_direction (GimpTextEditor    *editor);

void                gimp_text_editor_set_font_name (GimpTextEditor    *editor,
                                                    const gchar       *font_name);
const gchar       * gimp_text_editor_get_font_name (GimpTextEditor    *editor);


#endif  /* __GIMP_TEXT_EDITOR_H__ */
