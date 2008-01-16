/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfiledialog.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DATA_CHOOSER_DIALOG_H__
#define __GIMP_DATA_CHOOSER_DIALOG_H__

G_BEGIN_DECLS

#define GIMP_TYPE_DATA_CHOOSER_DIALOG            (gimp_data_chooser_dialog_get_type ())
#define GIMP_DATA_CHOOSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DATA_CHOOSER_DIALOG, GimpDataChooserDialog))
#define GIMP_DATA_CHOOSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_CHOOSER_DIALOG, GimpDataChooserDialogClass))
#define GIMP_IS_DATA_CHOOSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DATA_CHOOSER_DIALOG))
#define GIMP_IS_DATA_CHOOSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_CHOOSER_DIALOG))
#define GIMP_DATA_CHOOSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DATA_CHOOSER_DIALOG, GimpDataChooserDialogClass))


typedef struct _GimpDataChooserDialogClass  GimpDataChooserDialogClass;

struct _GimpDataChooserDialog
{
  GimpDialog            parent_instance;

  GimpDataFactory      *working_factory;
  GimpDataFactoryView  *factory_view;
  gchar                *filename;
};

struct _GimpDataChooserDialogClass
{
  GimpDialogClass  parent_class;
};


GType   gimp_data_chooser_dialog_get_type (void) G_GNUC_CONST;

gchar * gimp_data_chooser_dialog_new      (GimpDataFactory *factory,
                                           GimpViewType     view_type);

G_END_DECLS

#endif /* __GIMP_DATA_CHOOSER_DIALOG_H__ */
