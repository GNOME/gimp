/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabusybox.h
 * Copyright (C) 2018 Ell
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_BUSY_BOX_H__
#define __LIGMA_BUSY_BOX_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_BUSY_BOX            (ligma_busy_box_get_type ())
#define LIGMA_BUSY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUSY_BOX, LigmaBusyBox))
#define LIGMA_BUSY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUSY_BOX, LigmaBusyBoxClass))
#define LIGMA_IS_BUSY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUSY_BOX))
#define LIGMA_IS_BUSY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUSY_BOX))
#define LIGMA_BUSY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUSY_BOX, LigmaBusyBoxClass))


typedef struct _LigmaBusyBoxPrivate  LigmaBusyBoxPrivate;
typedef struct _LigmaBusyBoxClass    LigmaBusyBoxClass;

struct _LigmaBusyBox
{
  GtkBox              parent_instance;

  LigmaBusyBoxPrivate *priv;
};

struct _LigmaBusyBoxClass
{
  GtkBoxClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType         ligma_busy_box_get_type    (void) G_GNUC_CONST;

GtkWidget   * ligma_busy_box_new         (const gchar *message);

void          ligma_busy_box_set_message (LigmaBusyBox *box,
                                         const gchar *message);
const gchar * ligma_busy_box_get_message (LigmaBusyBox *box);


G_END_DECLS

#endif /* __LIGMA_BUSY_BOX_H__ */
