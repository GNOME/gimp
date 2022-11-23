/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasavedialog.h
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

#ifndef __LIGMA_SAVE_DIALOG_H__
#define __LIGMA_SAVE_DIALOG_H__

#include "ligmafiledialog.h"

G_BEGIN_DECLS

#define LIGMA_TYPE_SAVE_DIALOG            (ligma_save_dialog_get_type ())
#define LIGMA_SAVE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SAVE_DIALOG, LigmaSaveDialog))
#define LIGMA_SAVE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SAVE_DIALOG, LigmaSaveDialogClass))
#define LIGMA_IS_SAVE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SAVE_DIALOG))
#define LIGMA_IS_SAVE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SAVE_DIALOG))
#define LIGMA_SAVE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SAVE_DIALOG, LigmaSaveDialogClass))


typedef struct _LigmaSaveDialogClass LigmaSaveDialogClass;

struct _LigmaSaveDialog
{
  LigmaFileDialog       parent_instance;

  gboolean             save_a_copy;
  gboolean             close_after_saving;
  LigmaObject          *display_to_close;

  GtkWidget           *compression_frame;
  GtkWidget           *compat_info;
  gboolean             compression;
};

struct _LigmaSaveDialogClass
{
  LigmaFileDialogClass  parent_class;
};


GType       ligma_save_dialog_get_type  (void) G_GNUC_CONST;

GtkWidget * ligma_save_dialog_new       (Ligma           *ligma);

void        ligma_save_dialog_set_image (LigmaSaveDialog *dialog,
                                        LigmaImage      *image,
                                        gboolean        save_a_copy,
                                        gboolean        close_after_saving,
                                        LigmaObject     *display);

G_END_DECLS

#endif /* __LIGMA_SAVE_DIALOG_H__ */
