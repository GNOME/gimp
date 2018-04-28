/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpregioniterator.h
 * Contains all kinds of miscellaneous routines factored out from different
 * plug-ins. They stay here until their API has crystalized a bit and we can
 * put them into the file where they belong (Maurits Rijk
 * <lpeek.mrijk@consunet.nl> if you want to blame someone for this mess)
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_REGION_ITERATOR_H__
#define __GIMP_REGION_ITERATOR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

typedef struct _GimpRgnIterator  GimpRgnIterator;

typedef void   (* GimpRgnFunc1)       (const guchar *src,
                                       gint          bpp,
                                       gpointer      data);
typedef void   (* GimpRgnFunc2)       (const guchar *src,
                                       guchar       *dest,
                                       gint          bpp,
                                       gpointer      data);
typedef void   (* GimpRgnFuncDest)    (gint          x,
                                       gint          y,
                                       guchar       *dest,
                                       gint          bpp,
                                       gpointer      data);

GIMP_DEPRECATED_FOR(GeglBufferIterator)
GimpRgnIterator * gimp_rgn_iterator_new      (GimpDrawable      *drawable,
                                              GimpRunMode        unused);
GIMP_DEPRECATED_FOR(GeglBufferIterator)
void              gimp_rgn_iterator_free     (GimpRgnIterator   *iter);
GIMP_DEPRECATED_FOR(GeglBufferIterator)
void              gimp_rgn_iterator_dest     (GimpRgnIterator   *iter,
                                              GimpRgnFuncDest    func,
                                              gpointer           data);

GIMP_DEPRECATED_FOR(GeglBufferIterator)
void              gimp_rgn_iterate1          (GimpDrawable      *drawable,
                                              GimpRunMode        unused,
                                              GimpRgnFunc1       func,
                                              gpointer           data);

GIMP_DEPRECATED_FOR(GeglBufferIterator)
void              gimp_rgn_iterate2          (GimpDrawable      *drawable,
                                              GimpRunMode        unused,
                                              GimpRgnFunc2       func,
                                              gpointer           data);

G_END_DECLS

#endif /* __GIMP_REGION_ITERATOR_H__ */
