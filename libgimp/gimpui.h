/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_UI_H__
#define __GIMP_UI_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include <libgimpwidgets/gimpwidgets.h>

#define __GIMP_UI_H_INSIDE__

#include <libgimp/gimpuitypes.h>

#include <libgimp/gimpaspectpreview.h>
#include <libgimp/gimpbrushchooser.h>
#include <libgimp/gimpdrawablechooser.h>
#include <libgimp/gimpdrawablepreview.h>
#include <libgimp/gimpfontchooser.h>
#include <libgimp/gimpgradientchooser.h>
#include <libgimp/gimpimagecombobox.h>
#include <libgimp/gimpitemcombobox.h>
#include <libgimp/gimppalettechooser.h>
#include <libgimp/gimppatternchooser.h>
#include <libgimp/gimpprocbrowserdialog.h>
#include <libgimp/gimpproceduredialog.h>
#include <libgimp/gimpexportproceduredialog.h>
#include <libgimp/gimpvectorloadproceduredialog.h>
#include <libgimp/gimpprocview.h>
#include <libgimp/gimpprogressbar.h>
#include <libgimp/gimppropwidgets.h>
#include <libgimp/gimpresourcechooser.h>
#include <libgimp/gimpzoompreview.h>

#undef __GIMP_UI_H_INSIDE__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


void        gimp_ui_init                          (const gchar *prog_name);

void        gimp_window_set_transient             (GtkWindow   *window);
void        gimp_window_set_transient_for         (GtkWindow   *window,
                                                   GBytes      *handle);
void        gimp_window_set_transient_for_display (GtkWindow   *window,
                                                   GimpDisplay *display);


G_END_DECLS

#endif /* __GIMP_UI_H__ */
