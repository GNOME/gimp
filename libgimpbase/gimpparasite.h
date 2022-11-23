/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaparasite.h
 * Copyright (C) 1998 Jay Cox <jaycox@ligma.org>
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_PARASITE_H__
#define __LIGMA_PARASITE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * LIGMA_TYPE_PARASITE
 */

#define LIGMA_TYPE_PARASITE               (ligma_parasite_get_type ())
#define LIGMA_VALUE_HOLDS_PARASITE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_PARASITE))

GType   ligma_parasite_get_type           (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_PARASITE
 */

#define LIGMA_TYPE_PARAM_PARASITE           (ligma_param_parasite_get_type ())
#define LIGMA_IS_PARAM_SPEC_PARASITE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_PARASITE))

typedef struct _LigmaParamSpecParasite LigmaParamSpecParasite;

GType        ligma_param_parasite_get_type  (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_parasite      (const gchar  *name,
                                            const gchar  *nick,
                                            const gchar  *blurb,
                                            GParamFlags   flags);


#define LIGMA_PARASITE_PERSISTENT 1
#define LIGMA_PARASITE_UNDOABLE   2

#define LIGMA_PARASITE_ATTACH_PARENT     (0x80 << 8)
#define LIGMA_PARASITE_PARENT_PERSISTENT (LIGMA_PARASITE_PERSISTENT << 8)
#define LIGMA_PARASITE_PARENT_UNDOABLE   (LIGMA_PARASITE_UNDOABLE << 8)

#define LIGMA_PARASITE_ATTACH_GRANDPARENT     (0x80 << 16)
#define LIGMA_PARASITE_GRANDPARENT_PERSISTENT (LIGMA_PARASITE_PERSISTENT << 16)
#define LIGMA_PARASITE_GRANDPARENT_UNDOABLE   (LIGMA_PARASITE_UNDOABLE << 16)


/**
 * LigmaParasite:
 * @name:  the parasite name, USE A UNIQUE PREFIX
 * @flags: the parasite flags, like save in XCF etc.
 * @size:  the parasite size in bytes
 * @data:  the parasite data, the owner os the parasite is responsible
 *   for tracking byte order and internal structure
 **/
struct _LigmaParasite
{
  gchar    *name;
  guint32   flags;
  guint32   size;
  gpointer  data;
};


LigmaParasite * ligma_parasite_new           (const gchar        *name,
                                            guint32             flags,
                                            guint32             size,
                                            gconstpointer       data);
void           ligma_parasite_free          (LigmaParasite       *parasite);

LigmaParasite * ligma_parasite_copy          (const LigmaParasite *parasite);

gboolean       ligma_parasite_compare       (const LigmaParasite *a,
                                            const LigmaParasite *b);

gboolean       ligma_parasite_is_type       (const LigmaParasite *parasite,
                                            const gchar        *name);
gboolean       ligma_parasite_is_persistent (const LigmaParasite *parasite);
gboolean       ligma_parasite_is_undoable   (const LigmaParasite *parasite);
gboolean       ligma_parasite_has_flag      (const LigmaParasite *parasite,
                                            gulong              flag);
gulong         ligma_parasite_get_flags     (const LigmaParasite *parasite);
const gchar  * ligma_parasite_get_name      (const LigmaParasite *parasite);

gconstpointer  ligma_parasite_get_data      (const LigmaParasite *parasite,
                                            guint32            *num_bytes);

G_END_DECLS

#endif /* __LIGMA_PARASITE_H__ */
