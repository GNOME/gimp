/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpvectors.h
 * Copyright (C) Jehan
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

#ifndef __GIMP_VECTORS_H__
#define __GIMP_VECTORS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_VECTORS            (gimp_vectors_get_type ())
#define GIMP_VECTORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTORS, GimpVectors))
#define GIMP_VECTORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VECTORS, GimpVectorsClass))
#define GIMP_IS_VECTORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTORS))
#define GIMP_IS_VECTORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VECTORS))
#define GIMP_VECTORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VECTORS, GimpVectorsClass))


typedef struct _GimpVectorsClass   GimpVectorsClass;
typedef struct _GimpVectorsPrivate GimpVectorsPrivate;

struct _GimpVectors
{
  GimpItem            parent_instance;

  GimpVectorsPrivate *priv;
};

struct _GimpVectorsClass
{
  GimpItemClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
};


GType         gimp_vectors_get_type  (void) G_GNUC_CONST;

GimpVectors * gimp_vectors_get_by_id (gint32 vectors_id);


G_END_DECLS

#endif /* __GIMP_VECTORS_H__ */
