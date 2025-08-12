/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportdialog.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#include "gimpfiledialog.h"


#define GIMP_TYPE_EXPORT_DIALOG            (gimp_export_dialog_get_type ())
#define GIMP_EXPORT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXPORT_DIALOG, GimpExportDialog))
#define GIMP_EXPORT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXPORT_DIALOG, GimpExportDialogClass))
#define GIMP_IS_EXPORT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXPORT_DIALOG))
#define GIMP_IS_EXPORT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXPORT_DIALOG))
#define GIMP_EXPORT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXPORT_DIALOG, GimpExportDialogClass))


typedef struct _GimpExportDialogClass GimpExportDialogClass;

struct _GimpExportDialog
{
  GimpFileDialog       parent_instance;

  GimpObject          *display;
};

struct _GimpExportDialogClass
{
  GimpFileDialogClass  parent_class;
};


GType       gimp_export_dialog_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_export_dialog_new       (Gimp             *gimp);

void        gimp_export_dialog_set_image (GimpExportDialog *dialog,
                                          GimpImage        *image,
                                          GimpObject       *display);
