/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UI_H__
#define __GIMP_UI_H__

#include <gtk/gtk.h>

#include <libgimpwidgets/gimpwidgets.h>

#include <libgimp/gimpuitypes.h>

#include <libgimp/gimpexport.h>
#include <libgimp/gimpmenu.h>
#include <libgimp/gimpaspectpreview.h>
#include <libgimp/gimpdrawablepreview.h>
#include <libgimp/gimpbrushmenu.h>
#include <libgimp/gimpfontmenu.h>
#include <libgimp/gimpgradientmenu.h>
#include <libgimp/gimppalettemenu.h>
#include <libgimp/gimppatternmenu.h>
#include <libgimp/gimppixbuf.h>
#include <libgimp/gimpprocbrowserdialog.h>
#include <libgimp/gimpprocview.h>
#include <libgimp/gimpprogressbar.h>
#include <libgimp/gimpitemcombobox.h>
#include <libgimp/gimpimagecombobox.h>
#include <libgimp/gimpselectbutton.h>
#include <libgimp/gimpbrushselectbutton.h>
#include <libgimp/gimpfontselectbutton.h>
#include <libgimp/gimpgradientselectbutton.h>
#include <libgimp/gimppaletteselectbutton.h>
#include <libgimp/gimppatternselectbutton.h>
#include <libgimp/gimpzoompreview.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


void        gimp_ui_init                          (const gchar *prog_name,
                                                   gboolean     preview);

GdkWindow * gimp_ui_get_display_window            (guint32      gdisp_ID);
GdkWindow * gimp_ui_get_progress_window           (void);

void        gimp_window_set_transient_for_display (GtkWindow   *window,
                                                   guint32      gdisp_ID);
void        gimp_window_set_transient             (GtkWindow   *window);

G_END_DECLS

#endif /* __GIMP_UI_H__ */
