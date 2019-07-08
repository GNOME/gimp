/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimperrordialog.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_ERROR_DIALOG_H__
#define __GIMP_ERROR_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_ERROR_DIALOG            (gimp_error_dialog_get_type ())
#define GIMP_ERROR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ERROR_DIALOG, GimpErrorDialog))
#define GIMP_ERROR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ERROR_DIALOG, GimpErrorDialogClass))
#define GIMP_IS_ERROR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ERROR_DIALOG))
#define GIMP_IS_ERROR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ERROR_DIALOG))
#define GIMP_ERROR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ERROR_DIALOG, GimpErrorDialogClass))


typedef struct _GimpErrorDialogClass  GimpErrorDialogClass;

struct _GimpErrorDialog
{
  GimpDialog       parent_instance;

  GtkWidget       *vbox;

  GList           *messages;
  gboolean         overflow;
};

struct _GimpErrorDialogClass
{
  GimpDialogClass  parent_class;
};


GType       gimp_error_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_error_dialog_new      (const gchar     *title);
void        gimp_error_dialog_add      (GimpErrorDialog *dialog,
                                        const gchar     *icon_name,
                                        const gchar     *domain,
                                        const gchar     *message);



G_END_DECLS

#endif /* __GIMP_ERROR_DIALOG_H__ */
