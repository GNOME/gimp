/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#ifndef _IMAP_OBJECT_POPUP_H
#define _IMAP_OBJECT_POPUP_H

#include "imap_object.h"

typedef struct {
   GtkWidget *menu;
   GtkWidget *up;
   GtkWidget *down;
   Object_t  *obj;
} ObjectPopup_t;

void object_handle_popup (ObjectPopup_t  *popup,
                          Object_t       *obj,
                          GdkEventButton *event,
                          GimpImap       *imap);
void object_do_popup     (Object_t       *obj,
                          GdkEventButton *event,
                          gpointer        data);

#endif /* _IMAP_OBJECT_POPUP_H */
