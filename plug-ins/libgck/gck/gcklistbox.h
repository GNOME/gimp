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

#ifndef __GCKLISTBOX_H__
#define __GCKLISTBOX_H__

#include "gcktypes.h"

#ifdef __cplusplus
extern "C" {
#endif

GckListBox *gck_listbox_new                        (GtkWidget *container,
                                                    gint expand, gint fill, gint padding,
                                                    gint width, gint height,
                                                    GtkSelectionMode selection_mode,
                                                    GckListBoxItem *list_items,
                                                    GckEventFunction event_handler);

void        gck_listbox_destroy                    (GckListBox *listbox);

GckListBox *gck_listbox_get                        (GList *itemlist);

void        gck_listbox_prepend_item               (GckListBox *listbox,
                                                    GckListBoxItem *item);

void        gck_listbox_prepend_items              (GckListBox *listbox,
                                                    GList *itemlist);

void        gck_listbox_append_item                (GckListBox *listbox,
                                                    GckListBoxItem *item);

void        gck_listbox_append_items               (GckListBox *listbox,
                                                    GList *itemlist);

void        gck_listbox_insert_item                (GckListBox *listbox,
                                                    GckListBoxItem *item,
                                                    gint position);

void        gck_listbox_insert_items               (GckListBox *listbox,
                                                    GList *itemlist,
                                                    gint position);

GList      *gck_listbox_get_current_selection      (GckListBox *listbox);

void        gck_listbox_set_current_selection      (GckListBox *listbox);

GList      *gck_listbox_item_find_by_position      (GckListBox *listbox,
                                                    gint position);

GList      *gck_listbox_item_find_by_label         (GckListBox *listbox,
                                                    char *label,
                                                    gint *position);

GList      *gck_listbox_item_find_by_user_data     (GckListBox *listbox,
                                                    gpointer user_data,
                                                    gint *position);

void        gck_listbox_delete_item_by_position    (GckListBox *listbox,
                                                    gint position);

void        gck_listbox_delete_item_by_label       (GckListBox *listbox,
                                                    char *label);

void        gck_listbox_delete_items_by_label      (GckListBox *listbox,
                                                    GList *itemlist);

void        gck_listbox_delete_item_by_user_data   (GckListBox *listbox,
                                                    gpointer user_data);

void        gck_listbox_delete_items_by_user_data  (GckListBox *listbox,
                                                    GList *itemlist);

void        gck_listbox_clear_items                (GckListBox *listbox,
                                                    gint start, gint end);

GList      *gck_listbox_select_item_by_position    (GckListBox *listbox,
                                                    gint position);

GList      *gck_listbox_select_item_by_label       (GckListBox *listbox,
                                                    char *label);

GList      *gck_listbox_select_item_by_user_data   (GckListBox *listbox,
                                                    gpointer user_data);

GList      *gck_listbox_unselect_item_by_position  (GckListBox *listbox,
                                                    gint position);

GList      *gck_listbox_unselect_item_by_label     (GckListBox *listbox,
                                                    char *label);

GList      *gck_listbox_unselect_item_by_user_data (GckListBox *listbox,
                                                    gpointer user_data);

void        gck_listbox_select_all                 (GckListBox *listbox);

void        gck_listbox_unselect_all               (GckListBox *listbox);

void        gck_listbox_disable_signals            (GckListBox *listbox);

void        gck_listbox_enable_signals             (GckListBox *listbox);

#ifdef __cplusplus
}
#endif


#endif
