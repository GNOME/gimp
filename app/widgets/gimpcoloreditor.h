/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoloreditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_COLOR_EDITOR            (gimp_color_editor_get_type ())
#define GIMP_COLOR_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_EDITOR, GimpColorEditor))
#define GIMP_COLOR_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_EDITOR, GimpColorEditorClass))
#define GIMP_IS_COLOR_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_EDITOR))
#define GIMP_IS_COLOR_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_EDITOR))
#define GIMP_COLOR_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_EDITOR, GimpColorEditorClass))


typedef struct _GimpColorEditorClass GimpColorEditorClass;

struct _GimpColorEditor
{
  GimpEditor        parent_instance;

  GimpContext      *context;
  GimpDisplay      *active_display;
  GimpDisplayShell *active_shell;
  GimpImage        *active_image;
  gboolean          edit_bg;

  GtkWidget        *hbox;
  GtkWidget        *notebook;
  GtkWidget        *fg_bg;
  GtkWidget        *hex_entry;
};

struct _GimpColorEditorClass
{
  GimpEditorClass  parent_class;
};


GType       gimp_color_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_color_editor_new      (GimpContext *context);
