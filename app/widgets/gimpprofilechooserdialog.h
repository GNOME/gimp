/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpProfileChooserDialog
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_PROFILE_CHOOSER_DIALOG_H__
#define __GIMP_PROFILE_CHOOSER_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_PROFILE_CHOOSER_DIALOG            (gimp_profile_chooser_dialog_get_type ())
#define GIMP_PROFILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROFILE_CHOOSER_DIALOG, GimpProfileChooserDialog))
#define GIMP_PROFILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROFILE_CHOOSER_DIALOG, GimpProfileChooserDialogClass))
#define GIMP_IS_PROFILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROFILE_CHOOSER_DIALOG))
#define GIMP_IS_PROFILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROFILE_CHOOSER_DIALOG))
#define GIMP_PROFILE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROFILE_CHOOSER_DIALOG, GimpProfileChooserDialogClass))


typedef struct _GimpProfileChooserDialogClass  GimpProfileChooserDialogClass;

struct _GimpProfileChooserDialog
{
  GtkFileChooserDialog  parent_instance;

  Gimp                 *gimp;
  GtkTextBuffer        *buffer;
  guint                 idle_id;
};

struct _GimpProfileChooserDialogClass
{
  GtkFileChooserDialogClass  parent_class;
};


GType       gimp_profile_chooser_dialog_get_type      (void) G_GNUC_CONST;

GtkWidget * gimp_profile_chooser_dialog_new           (Gimp        *gimp,
                                                       const gchar *title);


#endif /* __GIMP_PROFILE_CHOOSER_DIALOG_H__ */
