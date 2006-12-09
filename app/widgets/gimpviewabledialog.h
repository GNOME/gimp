/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewabledialog.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEWABLE_DIALOG_H__
#define __GIMP_VIEWABLE_DIALOG_H__

#include "libgimpwidgets/gimpdialog.h"

G_BEGIN_DECLS


#define GIMP_TYPE_VIEWABLE_DIALOG            (gimp_viewable_dialog_get_type ())
#define GIMP_VIEWABLE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEWABLE_DIALOG, GimpViewableDialog))
#define GIMP_VIEWABLE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEWABLE_DIALOG, GimpViewableDialogClass))
#define GIMP_IS_VIEWABLE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VIEWABLE_DIALOG))
#define GIMP_IS_VIEWABLE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEWABLE_DIALOG))
#define GIMP_VIEWABLE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEWABLE_DIALOG, GimpViewableDialogClass))


typedef struct _GimpViewableDialogClass  GimpViewableDialogClass;

struct _GimpViewableDialog
{
  GimpDialog   parent_instance;

  GimpContext *context;

  GtkWidget   *icon;
  GtkWidget   *view;
  GtkWidget   *desc_label;
  GtkWidget   *viewable_label;
};

struct _GimpViewableDialogClass
{
  GimpDialogClass  parent_class;
};


GType       gimp_viewable_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_viewable_dialog_new      (GimpViewable       *viewable,
                                           GimpContext        *context,
                                           const gchar        *title,
                                           const gchar        *role,
                                           const gchar        *stock_id,
                                           const gchar        *desc,
                                           GtkWidget          *parent,
                                           GimpHelpFunc        help_func,
                                           const gchar        *help_id,
                                           ...) G_GNUC_NULL_TERMINATED;

void    gimp_viewable_dialog_set_viewable (GimpViewableDialog *dialog,
                                           GimpViewable       *viewable,
                                           GimpContext        *context);


G_END_DECLS

#endif /* __GIMP_VIEWABLE_DIALOG_H__ */
