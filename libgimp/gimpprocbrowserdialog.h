/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocbrowserdialog.h
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

/*
 * dbbrowser_utils.h
 * 0.08  26th sept 97  by Thomas NOEL <thomas@minet.net>
 */

#ifndef __GIMP_PROC_BROWSER_DIALOG_H__
#define __GIMP_PROC_BROWSER_DIALOG_H__

#include <libgimpwidgets/gimpdialog.h>


#define GIMP_TYPE_PROC_BROWSER_DIALOG            (gimp_proc_browser_dialog_get_type ())
#define GIMP_PROC_BROWSER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROC_BROWSER_DIALOG, GimpProcBrowserDialog))
#define GIMP_PROC_BROWSER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROC_BROWSER_DIALOG, GimpProcBrowserDialogClass))
#define GIMP_IS_PROC_BROWSER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROC_BROWSER_DIALOG))
#define GIMP_IS_PROC_BROWSER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROC_BROWSER_DIALOG))
#define GIMP_PROC_BROWSER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROC_BROWSER_DIALOG, GimpProcBrowserDialogClass))


typedef struct _GimpProcBrowserDialog      GimpProcBrowserDialog;
typedef struct _GimpProcBrowserDialogClass GimpProcBrowserDialogClass;

struct _GimpProcBrowserDialog
{
  GimpDialog    parent_instance;

  gboolean      scheme_names;

  GtkWidget    *browser;

  GtkListStore *store;
  GtkWidget    *tree_view;
};

struct _GimpProcBrowserDialogClass
{
  GimpDialogClass  parent_class;

  void (* selection_changed) (GimpProcBrowserDialog *dialog);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


GType       gimp_proc_browser_dialog_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_proc_browser_dialog_new          (gboolean               scheme_names,
                                                   gboolean               apply_button);

gchar     * gimp_proc_browser_dialog_get_selected (GimpProcBrowserDialog *dialog);


#endif  /* __GIMP_PROC_BROWSER_DIALOG_H__ */
