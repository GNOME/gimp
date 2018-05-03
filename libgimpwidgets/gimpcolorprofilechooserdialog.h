/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorProfileChooserDialog
 * Copyright (C) 2006-2014 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__
#define __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__

G_BEGIN_DECLS


#define GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG            (gimp_color_profile_chooser_dialog_get_type ())
#define GIMP_COLOR_PROFILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, GimpColorProfileChooserDialog))
#define GIMP_COLOR_PROFILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, GimpColorProfileChooserDialogClass))
#define GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG))
#define GIMP_IS_COLOR_PROFILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG))
#define GIMP_COLOR_PROFILE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, GimpColorProfileChooserDialogClass))


typedef struct _GimpColorProfileChooserDialogClass   GimpColorProfileChooserDialogClass;
typedef struct _GimpColorProfileChooserDialogPrivate GimpColorProfileChooserDialogPrivate;

struct _GimpColorProfileChooserDialog
{
  GtkFileChooserDialog                  parent_instance;

  GimpColorProfileChooserDialogPrivate *priv;
};

struct _GimpColorProfileChooserDialogClass
{
  GtkFileChooserDialogClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType       gimp_color_profile_chooser_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_color_profile_chooser_dialog_new      (const gchar          *title,
                                                        GtkWindow            *parent,
                                                        GtkFileChooserAction  action);


G_END_DECLS

#endif /* __GIMP_COLOR_PROFILE_CHOOSER_DIALOG_H__ */
