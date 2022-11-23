/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahelpui.h
 * Copyright (C) 2000-2003 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_HELP_UI_H__
#define __LIGMA_HELP_UI_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  the standard ligma help function
 */
void   ligma_standard_help_func             (const gchar    *help_id,
                                            gpointer        help_data);

/*  connect the help callback of a window  */
void   ligma_help_connect                   (GtkWidget      *widget,
                                            LigmaHelpFunc    help_func,
                                            const gchar    *help_id,
                                            gpointer        help_data,
                                            GDestroyNotify  help_data_destroy);

/*  set help data for non-window widgets  */
void   ligma_help_set_help_data             (GtkWidget      *widget,
                                            const gchar    *tooltip,
                                            const gchar    *help_id);

/*  set help data with markup for non-window widgets  */
void   ligma_help_set_help_data_with_markup (GtkWidget      *widget,
                                            const gchar    *tooltip,
                                            const gchar    *help_id);

/*  activate the context help inspector  */
void   ligma_context_help                   (GtkWidget      *widget);


/**
 * LIGMA_HELP_ID:
 *
 * The #GQuark used to attach LIGMA help IDs to widgets.
 *
 * Since: 2.2
 **/
#define LIGMA_HELP_ID (ligma_help_id_quark ())

GQuark ligma_help_id_quark           (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_HELP_UI_H__ */
