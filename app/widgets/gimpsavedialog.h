/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsavedialog.h
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

#ifndef __GIMP_SAVE_DIALOG_H__
#define __GIMP_SAVE_DIALOG_H__

#include "gimpfiledialog.h"

G_BEGIN_DECLS

#define GIMP_TYPE_SAVE_DIALOG            (gimp_save_dialog_get_type ())
#define GIMP_SAVE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SAVE_DIALOG, GimpSaveDialog))
#define GIMP_SAVE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SAVE_DIALOG, GimpSaveDialogClass))
#define GIMP_IS_SAVE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SAVE_DIALOG))
#define GIMP_IS_SAVE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SAVE_DIALOG))
#define GIMP_SAVE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SAVE_DIALOG, GimpSaveDialogClass))


typedef struct _GimpSaveDialogClass GimpSaveDialogClass;

struct _GimpSaveDialog
{
  GimpFileDialog       parent_instance;

  gboolean             save_a_copy;
  gboolean             close_after_saving;
  GimpObject          *display_to_close;

  GtkWidget           *compression_frame;
  GtkWidget           *compat_info;
  gboolean             compression;
};

struct _GimpSaveDialogClass
{
  GimpFileDialogClass  parent_class;
};


GType       gimp_save_dialog_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_save_dialog_new       (Gimp           *gimp);

void        gimp_save_dialog_set_image (GimpSaveDialog *dialog,
                                        GimpImage      *image,
                                        gboolean        save_a_copy,
                                        gboolean        close_after_saving,
                                        GimpObject     *display);

G_END_DECLS

#endif /* __GIMP_SAVE_DIALOG_H__ */
