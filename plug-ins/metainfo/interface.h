/* interface.h - user interface for the metadata editor
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <glib-object.h>

#include "libgimp/stdplugins-intl.h"

#include "page-description.h"
#include "page-artwork.h"
#include "page-administration.h"
#include "page-rights.h"

G_BEGIN_DECLS

#define THUMB_SIZE              48

#define METAINFO_RESPONSE_SAVE 201

void             metainfo_message_dialog      (GtkMessageType  type,
                                               GtkWindow      *parent,
                                               const gchar    *title,
                                               const gchar    *message);
gboolean         metainfo_get_save_attributes (void);
                                          
gboolean         metainfo_dialog              (gint32          image_ID,
                                               GimpAttributes *attributes);

G_END_DECLS

#endif /* INTERFACE_H */
