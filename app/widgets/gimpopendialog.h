/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpopendialog.h
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


#define GIMP_TYPE_OPEN_DIALOG            (gimp_open_dialog_get_type ())
#define GIMP_OPEN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPEN_DIALOG, GimpOpenDialog))
#define GIMP_OPEN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_OPEN_DIALOG, GimpOpenDialogClass))
#define GIMP_IS_OPEN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPEN_DIALOG))
#define GIMP_IS_OPEN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_OPEN_DIALOG))
#define GIMP_OPEN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_OPEN_DIALOG, GimpOpenDialogClass))


typedef struct _GimpOpenDialogClass GimpOpenDialogClass;

struct _GimpOpenDialog
{
  GimpFileDialog       parent_instance;

  gboolean             open_as_layers;
  gboolean             open_as_link;
};

struct _GimpOpenDialogClass
{
  GimpFileDialogClass  parent_class;
};


GType       gimp_open_dialog_get_type   (void) G_GNUC_CONST;

GtkWidget * gimp_open_dialog_new        (Gimp           *gimp);

void        gimp_open_dialog_set_image  (GimpOpenDialog *dialog,
                                         GimpImage      *image,
                                         gboolean        open_as_layers,
                                         gboolean        open_as_link);
