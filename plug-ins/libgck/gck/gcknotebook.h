/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/***************************************************************************/

#ifndef __GCKNOTEBOOK_H__
#define __GCKNOTEBOOK_H__

#include <gck/gck.h>

#ifdef __cplusplus
extern "C" {
#endif

GckNoteBook     *gck_notebook_new          (GtkWidget *container,
                                            gint width,gint height,
                                            GtkPositionType tab_position);

void             gck_notebook_destroy      (GckNoteBook *notebook);

GckNoteBookPage *gck_notebook_page_new     (gchar *label,
                                            GckNoteBook *notebook);

void             gck_notebook_insert_page  (GckNoteBook *notebook,
                                            GckNoteBookPage *page,
                                            gint position);

void             gck_notebook_append_page  (GckNoteBook *notebook,
                                            GckNoteBookPage *page);

void             gck_notebook_prepend_page (GckNoteBook *notebook,
                                            GckNoteBookPage *page);

void             gck_notebook_remove_page  (GckNoteBook *notebook,
                                            gint page_num);

GckNoteBookPage *gck_notebook_get_page     (GckNoteBook *notebook);

void             gck_notebook_set_page     (GckNoteBook *notebook,
                                            gint page_num);

#ifdef __cplusplus
}
#endif

#endif
