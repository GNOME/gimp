/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedure-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PROCEDURE_PRIVATE_H__
#define __GIMP_PROCEDURE_PRIVATE_H__

G_BEGIN_DECLS


GimpDisplay  * _gimp_procedure_get_display     (GimpProcedure *procedure,
                                                gint32         display_id);
GimpImage    * _gimp_procedure_get_image       (GimpProcedure *procedure,
                                               gint32          image_id);
GimpItem     * _gimp_procedure_get_item        (GimpProcedure *procedure,
                                                gint32         item_id);
GimpResource * _gimp_procedure_get_resource    (GimpProcedure *procedure,
                                                gint32         resource_id);

void           _gimp_procedure_destroy_proxies (GimpProcedure *procedure);


G_END_DECLS

#endif  /*  __GIMP_PROCEDURE_H__  */
