/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaopendialog.h
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

#ifndef __LIGMA_OPEN_DIALOG_H__
#define __LIGMA_OPEN_DIALOG_H__

#include "ligmafiledialog.h"

G_BEGIN_DECLS

#define LIGMA_TYPE_OPEN_DIALOG            (ligma_open_dialog_get_type ())
#define LIGMA_OPEN_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPEN_DIALOG, LigmaOpenDialog))
#define LIGMA_OPEN_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_OPEN_DIALOG, LigmaOpenDialogClass))
#define LIGMA_IS_OPEN_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPEN_DIALOG))
#define LIGMA_IS_OPEN_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_OPEN_DIALOG))
#define LIGMA_OPEN_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_OPEN_DIALOG, LigmaOpenDialogClass))


typedef struct _LigmaOpenDialogClass LigmaOpenDialogClass;

struct _LigmaOpenDialog
{
  LigmaFileDialog       parent_instance;

  gboolean             open_as_layers;
};

struct _LigmaOpenDialogClass
{
  LigmaFileDialogClass  parent_class;
};


GType       ligma_open_dialog_get_type   (void) G_GNUC_CONST;

GtkWidget * ligma_open_dialog_new        (Ligma           *ligma);

void        ligma_open_dialog_set_image  (LigmaOpenDialog *dialog,
                                         LigmaImage      *image,
                                         gboolean        open_as_layers);

G_END_DECLS

#endif /* __LIGMA_OPEN_DIALOG_H__ */
