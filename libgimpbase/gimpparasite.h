/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasite.h
 * Copyright (C) 1998 Jay Cox <jaycox@gimp.org>
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_PARASITE_H__
#define __GIMP_PARASITE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * GIMP_TYPE_PARASITE
 */

#define GIMP_TYPE_PARASITE               (gimp_parasite_get_type ())
#define GIMP_VALUE_HOLDS_PARASITE(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_PARASITE))

GType   gimp_parasite_get_type           (void) G_GNUC_CONST;


/*
 * GIMP_TYPE_PARAM_PARASITE
 */

#define GIMP_TYPE_PARAM_PARASITE           (gimp_param_parasite_get_type ())
#define GIMP_IS_PARAM_SPEC_PARASITE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PARASITE))

GType        gimp_param_parasite_get_type  (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_parasite      (const gchar  *name,
                                            const gchar  *nick,
                                            const gchar  *blurb,
                                            GParamFlags   flags);


#define GIMP_PARASITE_PERSISTENT 1
#define GIMP_PARASITE_UNDOABLE   2

#define GIMP_PARASITE_ATTACH_PARENT     (0x80 << 8)
#define GIMP_PARASITE_PARENT_PERSISTENT (GIMP_PARASITE_PERSISTENT << 8)
#define GIMP_PARASITE_PARENT_UNDOABLE   (GIMP_PARASITE_UNDOABLE << 8)

#define GIMP_PARASITE_ATTACH_GRANDPARENT     (0x80 << 16)
#define GIMP_PARASITE_GRANDPARENT_PERSISTENT (GIMP_PARASITE_PERSISTENT << 16)
#define GIMP_PARASITE_GRANDPARENT_UNDOABLE   (GIMP_PARASITE_UNDOABLE << 16)


struct _GimpParasite
{
  gchar    *name;   /* The name of the parasite. USE A UNIQUE PREFIX! */
  guint32   flags;  /* save Parasite in XCF file, etc.                */
  guint32   size;   /* amount of data                                 */
  gpointer  data;   /* a pointer to the data.  plugin is              *
                     * responsible for tracking byte order            */
};


GimpParasite * gimp_parasite_new           (const gchar        *name,
                                            guint32             flags,
                                            guint32             size,
                                            gconstpointer       data);
void           gimp_parasite_free          (GimpParasite       *parasite);

GimpParasite * gimp_parasite_copy          (const GimpParasite *parasite);

gboolean       gimp_parasite_compare       (const GimpParasite *a,
                                            const GimpParasite *b);

gboolean       gimp_parasite_is_type       (const GimpParasite *parasite,
                                            const gchar        *name);
gboolean       gimp_parasite_is_persistent (const GimpParasite *parasite);
gboolean       gimp_parasite_is_undoable   (const GimpParasite *parasite);
gboolean       gimp_parasite_has_flag      (const GimpParasite *parasite,
                                            gulong              flag);
gulong         gimp_parasite_flags         (const GimpParasite *parasite);
const gchar  * gimp_parasite_name          (const GimpParasite *parasite);
gconstpointer  gimp_parasite_data          (const GimpParasite *parasite);
glong          gimp_parasite_data_size     (const GimpParasite *parasite);


G_END_DECLS

#endif /* __GIMP_PARASITE_H__ */
