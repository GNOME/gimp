/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmadisplay.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_DISPLAY_H__
#define __LIGMA_DISPLAY_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */

#define LIGMA_TYPE_DISPLAY            (ligma_display_get_type ())
#define LIGMA_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DISPLAY, LigmaDisplay))
#define LIGMA_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DISPLAY, LigmaDisplayClass))
#define LIGMA_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DISPLAY))
#define LIGMA_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DISPLAY))
#define LIGMA_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DISPLAY, LigmaDisplayClass))


typedef struct _LigmaDisplayClass   LigmaDisplayClass;
typedef struct _LigmaDisplayPrivate LigmaDisplayPrivate;

struct _LigmaDisplay
{
  GObject             parent_instance;

  LigmaDisplayPrivate *priv;
};

struct _LigmaDisplayClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType         ligma_display_get_type     (void) G_GNUC_CONST;

gint32        ligma_display_get_id       (LigmaDisplay    *display);
LigmaDisplay * ligma_display_get_by_id    (gint32          display_id);

gboolean      ligma_display_is_valid     (LigmaDisplay    *display);


G_END_DECLS

#endif /* __LIGMA_DISPLAY_H__ */
