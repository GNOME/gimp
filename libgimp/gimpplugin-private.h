/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaplugin-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PLUG_IN_PRIVATE_H__
#define __LIGMA_PLUG_IN_PRIVATE_H__

G_BEGIN_DECLS


void            _ligma_plug_in_query             (LigmaPlugIn      *plug_in);
void            _ligma_plug_in_init              (LigmaPlugIn      *plug_in);
void            _ligma_plug_in_run               (LigmaPlugIn      *plug_in);
void            _ligma_plug_in_quit              (LigmaPlugIn      *plug_in);

GIOChannel    * _ligma_plug_in_get_read_channel  (LigmaPlugIn      *plug_in);
GIOChannel    * _ligma_plug_in_get_write_channel (LigmaPlugIn      *plug_in);

void            _ligma_plug_in_read_expect_msg   (LigmaPlugIn      *plug_in,
                                                 LigmaWireMessage *msg,
                                                 gint             type);

gboolean        _ligma_plug_in_set_i18n          (LigmaPlugIn      *plug_in,
                                                 const gchar     *procedure_name,
                                                 gchar          **gettext_domain,
                                                 gchar          **catalog_dir);

LigmaProcedure * _ligma_plug_in_create_procedure  (LigmaPlugIn      *plug_in,
                                                 const gchar     *procedure_name);

LigmaProcedure * _ligma_plug_in_get_procedure     (LigmaPlugIn      *plug_in);

LigmaDisplay   * _ligma_plug_in_get_display       (LigmaPlugIn      *plug_in,
                                                 gint32           display_id);
LigmaImage     * _ligma_plug_in_get_image         (LigmaPlugIn      *plug_in,
                                                 gint32           image_id);
LigmaItem      * _ligma_plug_in_get_item          (LigmaPlugIn      *plug_in,
                                                 gint32           item_id);


G_END_DECLS

#endif /* __LIGMA_PLUG_IN_PRIVATE_H__ */
