/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpparasite.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PARASITE_H__
#define __GIMP_PARASITE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


#define GIMP_PARASITE_PERSISTENT 1
#define GIMP_PARASITE_UNDOABLE   2

#define GIMP_PARASITE_ATTACH_PARENT     (0x80 << 8)
#define GIMP_PARASITE_PARENT_PERSISTENT (GIMP_PARASITE_PERSISTENT << 8)
#define GIMP_PARASITE_PARENT_UNDOABLE   (GIMP_PARASITE_UNDOABLE << 8)

#define GIMP_PARASITE_ATTACH_GRANDPARENT     (0x80 << 16)
#define GIMP_PARASITE_GRANDPARENT_PERSISTENT (GIMP_PARASITE_PERSISTENT << 16)
#define GIMP_PARASITE_GRANDPARENT_UNDOABLE   (GIMP_PARASITE_UNDOABLE << 16)


typedef struct _GimpParasite GimpParasite;

struct _GimpParasite
{
  gchar    *name;   /* The name of the parasite. USE A UNIQUE PREFIX! */
  guint32   flags;  /* save Parasite in XCF file, etc.                */
  guint32   size;   /* amount of data                                 */
  gpointer  data;   /* a pointer to the data.  plugin is              *
		     * responsible for tracking byte order            */
};


GimpParasite * gimp_parasite_new                 (const gchar        *name, 
						  guint32             flags,
						  guint32             size, 
						  const gpointer      data);
void           gimp_parasite_free                (GimpParasite       *parasite);

GimpParasite * gimp_parasite_copy                (const GimpParasite *parasite);

gboolean       gimp_parasite_compare             (const GimpParasite *a, 
						  const GimpParasite *b);

gboolean       gimp_parasite_is_type             (const GimpParasite *parasite,
						  const gchar        *name);
gboolean       gimp_parasite_is_persistent       (const GimpParasite *parasite);
gboolean       gimp_parasite_is_undoable         (const GimpParasite *parasite);
gboolean       gimp_parasite_has_flag            (const GimpParasite *parasite,
						  gulong              flag);
gulong         gimp_parasite_flags               (const GimpParasite *parasite);
const gchar  * gimp_parasite_name                (const GimpParasite *parasite);
gpointer       gimp_parasite_data                (const GimpParasite *parasite);
glong          gimp_parasite_data_size           (const GimpParasite *parasite);

void           gimp_attach_new_parasite          (const gchar        *name, 
						  gint                flags,
						  gint                size, 
						  const gpointer      data);
void           gimp_drawable_attach_new_parasite (gint32              drawable_ID,
						  const gchar        *name, 
						  gint                flags,
						  gint                size, 
						  const gpointer      data);
void           gimp_image_attach_new_parasite    (gint32              image_ID,
						  const gchar        *name, 
						  gint                flags,
						  gint                size, 
						  const gpointer      data);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_PARASITE_H__ */
