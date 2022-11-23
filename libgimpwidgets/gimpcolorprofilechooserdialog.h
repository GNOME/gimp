/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaColorProfileChooserDialog
 * Copyright (C) 2006-2014 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COLOR_PROFILE_CHOOSER_DIALOG_H__
#define __LIGMA_COLOR_PROFILE_CHOOSER_DIALOG_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG            (ligma_color_profile_chooser_dialog_get_type ())
#define LIGMA_COLOR_PROFILE_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, LigmaColorProfileChooserDialog))
#define LIGMA_COLOR_PROFILE_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, LigmaColorProfileChooserDialogClass))
#define LIGMA_IS_COLOR_PROFILE_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG))
#define LIGMA_IS_COLOR_PROFILE_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG))
#define LIGMA_COLOR_PROFILE_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG, LigmaColorProfileChooserDialogClass))


typedef struct _LigmaColorProfileChooserDialogClass   LigmaColorProfileChooserDialogClass;
typedef struct _LigmaColorProfileChooserDialogPrivate LigmaColorProfileChooserDialogPrivate;

struct _LigmaColorProfileChooserDialog
{
  GtkFileChooserDialog                  parent_instance;

  LigmaColorProfileChooserDialogPrivate *priv;
};

struct _LigmaColorProfileChooserDialogClass
{
  GtkFileChooserDialogClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType       ligma_color_profile_chooser_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_color_profile_chooser_dialog_new      (const gchar          *title,
                                                        GtkWindow            *parent,
                                                        GtkFileChooserAction  action);


G_END_DECLS

#endif /* __LIGMA_COLOR_PROFILE_CHOOSER_DIALOG_H__ */
