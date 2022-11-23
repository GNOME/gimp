/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The LIGMA Help Browser
 * Copyright (C) 1999-2003 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
 *
 * dialog.h
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

#ifndef __DIALOG_H__
#define __DIALOG_H__

#include <gtk/gtk.h>

#define LIGMA_TYPE_HELP_BROWSER_DIALOG (ligma_help_browser_dialog_get_type ())
G_DECLARE_FINAL_TYPE (LigmaHelpBrowserDialog, ligma_help_browser_dialog,
                      LIGMA, HELP_BROWSER_DIALOG,
                      GtkApplicationWindow)

LigmaHelpBrowserDialog * ligma_help_browser_dialog_new         (const char   *plug_in_binary,
                                                              GApplication *app);

void                    ligma_help_browser_dialog_load        (LigmaHelpBrowserDialog *self,
                                                              const char            *uri);

void                    ligma_help_browser_dialog_make_index  (LigmaHelpBrowserDialog *self,
                                                              LigmaHelpDomain        *domain,
                                                              LigmaHelpLocale        *locale);

#endif /* ! __DIALOG_H__ */
