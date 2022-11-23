/* LIBLIGMA - The LIGMA Library
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

#ifndef __LIGMA_UI_H__
#define __LIGMA_UI_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libligma/ligma.h>

#include <libligmawidgets/ligmawidgets.h>

#define __LIGMA_UI_H_INSIDE__

#include <libligma/ligmauitypes.h>

#include <libligma/ligmaaspectpreview.h>
#include <libligma/ligmabrushselectbutton.h>
#include <libligma/ligmadrawablepreview.h>
#include <libligma/ligmaexport.h>
#include <libligma/ligmafontselectbutton.h>
#include <libligma/ligmagradientselectbutton.h>
#include <libligma/ligmaimagecombobox.h>
#include <libligma/ligmaitemcombobox.h>
#include <libligma/ligmapaletteselectbutton.h>
#include <libligma/ligmapatternselectbutton.h>
#include <libligma/ligmaprocbrowserdialog.h>
#include <libligma/ligmaproceduredialog.h>
#include <libligma/ligmaprocview.h>
#include <libligma/ligmaprogressbar.h>
#include <libligma/ligmasaveproceduredialog.h>
#include <libligma/ligmaselectbutton.h>
#include <libligma/ligmazoompreview.h>

#undef __LIGMA_UI_H_INSIDE__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


void        ligma_ui_init                          (const gchar *prog_name);

GdkWindow * ligma_ui_get_progress_window           (void);

void        ligma_window_set_transient             (GtkWindow   *window);

GdkWindow * ligma_ui_get_display_window            (LigmaDisplay *display);
void        ligma_window_set_transient_for_display (GtkWindow   *window,
                                                   LigmaDisplay *display);


G_END_DECLS

#endif /* __LIGMA_UI_H__ */
