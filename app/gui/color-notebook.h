/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __COLOR_NOTEBOOK_H__
#define __COLOR_NOTEBOOK_H__


typedef enum
{
  COLOR_NOTEBOOK_OK,
  COLOR_NOTEBOOK_CANCEL,
  COLOR_NOTEBOOK_UPDATE
} ColorNotebookState;


typedef void (* ColorNotebookCallback) (ColorNotebook      *cnb,
					const GimpRGB      *color,
					ColorNotebookState  state,
					gpointer            callback_data);


ColorNotebook * color_notebook_new          (const gchar           *title,
                                             GtkWidget             *parent,
                                             GimpDialogFactory     *dialog_factory,
                                             const gchar           *dialog_identifier,
                                             const GimpRGB         *color,
                                             ColorNotebookCallback  callback,
                                             gpointer               callback_data,
                                             gboolean               wants_update,
                                             gboolean               show_aplha);

ColorNotebook * color_notebook_viewable_new (GimpViewable          *viewable,
                                             const gchar           *title,
                                             const gchar           *stock_id,
                                             const gchar           *desc,
                                             GtkWidget             *parent,
                                             GimpDialogFactory     *dialog_factory,
                                             const gchar           *dialog_identifier,
                                             const GimpRGB         *color,
                                             ColorNotebookCallback  callback,
                                             gpointer               callback_data,
                                             gboolean               wants_update,
                                             gboolean               show_alpha);

void            color_notebook_free         (ColorNotebook         *cnb);

void            color_notebook_set_viewable (ColorNotebook         *cnb,
                                             GimpViewable          *viewable);
void            color_notebook_set_title    (ColorNotebook         *cnb,
                                             const gchar           *title);

void            color_notebook_show         (ColorNotebook         *cnb);
void            color_notebook_hide         (ColorNotebook         *cnb);

void            color_notebook_set_color    (ColorNotebook         *cnb,
                                             const GimpRGB         *color);
void            color_notebook_get_color    (ColorNotebook         *cnb,
                                             GimpRGB               *color);


#endif /* __COLOR_NOTEBOOK_H__ */
