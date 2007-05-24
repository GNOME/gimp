/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsessioninfoaux.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_SESSION_INFO_AUX_H__
#define __GIMP_SESSION_INFO_AUX_H__


struct _GimpSessionInfoAux
{
  gchar *name;
  gchar *value;
};


GimpSessionInfoAux *
             gimp_session_info_aux_new            (const gchar         *name,
                                                   const gchar         *value);
void         gimp_session_info_aux_free           (GimpSessionInfoAux  *aux);

GList      * gimp_session_info_aux_new_from_props (GObject             *object,
                                                   ...) G_GNUC_NULL_TERMINATED;
void         gimp_session_info_aux_set_props      (GObject             *object,
                                                   GList               *aux,
                                                   ...) G_GNUC_NULL_TERMINATED;

void         gimp_session_info_aux_serialize      (GimpConfigWriter    *writer,
                                                   GtkWidget           *widget);
GTokenType   gimp_session_info_aux_deserialize    (GScanner            *scanner,
                                                   GList              **aux_list);

void         gimp_session_info_aux_set_list       (GtkWidget           *dialog,
                                                   GList               *aux_info);
GList      * gimp_session_info_aux_get_list       (GtkWidget           *dialog);


#endif  /* __GIMP_SESSION_INFO_AUX_H__ */
