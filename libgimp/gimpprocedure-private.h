/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprocedure-private.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PROCEDURE_PRIVATE_H__
#define __LIGMA_PROCEDURE_PRIVATE_H__

G_BEGIN_DECLS


LigmaDisplay * _ligma_procedure_get_display     (LigmaProcedure *procedure,
                                               gint32         display_id);
LigmaImage   * _ligma_procedure_get_image       (LigmaProcedure *procedure,
                                               gint32         image_id);
LigmaItem    * _ligma_procedure_get_item        (LigmaProcedure *procedure,
                                               gint32         item_id);

void          _ligma_procedure_destroy_proxies (LigmaProcedure *procedure);


G_END_DECLS

#endif  /*  __LIGMA_PROCEDURE_H__  */
