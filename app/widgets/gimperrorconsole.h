/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimperrorconsole.h
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

#pragma once

#include "gimpeditor.h"


#define GIMP_TYPE_ERROR_CONSOLE            (gimp_error_console_get_type ())
#define GIMP_ERROR_CONSOLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ERROR_CONSOLE, GimpErrorConsole))
#define GIMP_ERROR_CONSOLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ERROR_CONSOLE, GimpErrorConsoleClass))
#define GIMP_IS_ERROR_CONSOLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ERROR_CONSOLE))
#define GIMP_IS_ERROR_CONSOLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ERROR_CONSOLE))
#define GIMP_ERROR_CONSOLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ERROR_CONSOLE, GimpErrorConsoleClass))


typedef struct _GimpErrorConsoleClass GimpErrorConsoleClass;

struct _GimpErrorConsole
{
  GimpEditor     parent_instance;

  Gimp          *gimp;

  GtkTextBuffer *text_buffer;
  GtkWidget     *text_view;

  GtkWidget     *clear_button;
  GtkWidget     *save_button;

  GtkWidget     *file_dialog;
  gboolean       save_selection;

  gboolean       highlight[GIMP_MESSAGE_ERROR + 1];
};

struct _GimpErrorConsoleClass
{
  GimpEditorClass  parent_class;
};


GType       gimp_error_console_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_error_console_new      (Gimp                *gimp,
                                         GimpMenuFactory     *menu_factory);

void        gimp_error_console_add      (GimpErrorConsole    *console,
                                         GimpMessageSeverity  severity,
                                         const gchar         *domain,
                                         const gchar         *message);
