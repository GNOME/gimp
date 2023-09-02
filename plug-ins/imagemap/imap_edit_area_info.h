/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_EDIT_AREA_INFO_H
#define _IMAP_EDIT_AREA_INFO_H

typedef struct AreaInfoDialog_t AreaInfoDialog_t;

#include "imap_default_dialog.h"
#include "imap_object.h"

struct AreaInfoDialog_t {
   DefaultDialog_t *dialog;
   Object_t        *obj;
   Object_t        *clone;
   gboolean         add;
   gboolean         geometry_lock;
   gboolean         preview;

   GtkWidget       *notebook;
   GtkWidget       *web_site;
   GtkWidget       *ftp_site;
   GtkWidget       *gopher;
   GtkWidget       *other;
   GtkWidget       *file;
   GtkWidget       *wais;
   GtkWidget       *telnet;
   GtkWidget       *email;
   GtkWidget       *url;
   GtkWidget       *relative_link;
   GtkWidget       *target;
   GtkWidget       *comment;
   GtkWidget       *mouse_over;
   GtkWidget       *mouse_out;
   GtkWidget       *focus;
   GtkWidget       *blur;
   GtkWidget       *click;
   GtkWidget       *accesskey;
   GtkWidget       *tabindex;
   GtkWidget       *browse;
   gpointer         infotab;
   gpointer         geometry_cb_id;
};

AreaInfoDialog_t *create_edit_area_info_dialog(Object_t *obj);
void edit_area_info_dialog_show(AreaInfoDialog_t *dialog, Object_t *obj,
                                gboolean add);
void edit_area_info_dialog_emit_geometry_signal(AreaInfoDialog_t *dialog);

#endif /* _IMAP_EDIT_AREA_INFO_H */
