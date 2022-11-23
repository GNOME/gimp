/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaerrorconsole.h
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

#ifndef __LIGMA_ERROR_CONSOLE_H__
#define __LIGMA_ERROR_CONSOLE_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_ERROR_CONSOLE            (ligma_error_console_get_type ())
#define LIGMA_ERROR_CONSOLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ERROR_CONSOLE, LigmaErrorConsole))
#define LIGMA_ERROR_CONSOLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ERROR_CONSOLE, LigmaErrorConsoleClass))
#define LIGMA_IS_ERROR_CONSOLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ERROR_CONSOLE))
#define LIGMA_IS_ERROR_CONSOLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ERROR_CONSOLE))
#define LIGMA_ERROR_CONSOLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ERROR_CONSOLE, LigmaErrorConsoleClass))


typedef struct _LigmaErrorConsoleClass LigmaErrorConsoleClass;

struct _LigmaErrorConsole
{
  LigmaEditor     parent_instance;

  Ligma          *ligma;

  GtkTextBuffer *text_buffer;
  GtkWidget     *text_view;

  GtkWidget     *clear_button;
  GtkWidget     *save_button;

  GtkWidget     *file_dialog;
  gboolean       save_selection;

  gboolean       highlight[LIGMA_MESSAGE_ERROR + 1];
};

struct _LigmaErrorConsoleClass
{
  LigmaEditorClass  parent_class;
};


GType       ligma_error_console_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_error_console_new      (Ligma                *ligma,
                                         LigmaMenuFactory     *menu_factory);

void        ligma_error_console_add      (LigmaErrorConsole    *console,
                                         LigmaMessageSeverity  severity,
                                         const gchar         *domain,
                                         const gchar         *message);


#endif  /*  __LIGMA_ERROR_CONSOLE_H__  */
