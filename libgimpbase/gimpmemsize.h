/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_MEMSIZE_H__
#define __LIGMA_MEMSIZE_H__

G_BEGIN_DECLS


/**
 * LIGMA_TYPE_MEMSIZE:
 *
 * #LIGMA_TYPE_MEMSIZE is a #GType derived from #G_TYPE_UINT64.
 **/

#define LIGMA_TYPE_MEMSIZE               (ligma_memsize_get_type ())
#define LIGMA_VALUE_HOLDS_MEMSIZE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_MEMSIZE))

GType      ligma_memsize_get_type         (void) G_GNUC_CONST;

gchar    * ligma_memsize_serialize        (guint64      memsize) G_GNUC_MALLOC;
gboolean   ligma_memsize_deserialize      (const gchar *string,
                                          guint64     *memsize);


/*
 * LIGMA_TYPE_PARAM_MEMSIZE
 */

#define LIGMA_TYPE_PARAM_MEMSIZE           (ligma_param_memsize_get_type ())
#define LIGMA_IS_PARAM_SPEC_MEMSIZE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_MEMSIZE))

GType        ligma_param_memsize_get_type  (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_memsize      (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           guint64       minimum,
                                           guint64       maximum,
                                           guint64       default_value,
                                           GParamFlags   flags);


G_END_DECLS

#endif  /* __LIGMA_MEMSIZE_H__ */
