/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbusybox.h
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

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_BUSY_BOX_H__
#define __GIMP_BUSY_BOX_H__

G_BEGIN_DECLS

#define GIMP_TYPE_BUSY_BOX            (gimp_busy_box_get_type ())
#define GIMP_BUSY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BUSY_BOX, GimpBusyBox))
#define GIMP_BUSY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BUSY_BOX, GimpBusyBoxClass))
#define GIMP_IS_BUSY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BUSY_BOX))
#define GIMP_IS_BUSY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BUSY_BOX))
#define GIMP_BUSY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BUSY_BOX, GimpBusyBoxClass))


typedef struct _GimpBusyBoxPrivate  GimpBusyBoxPrivate;
typedef struct _GimpBusyBoxClass    GimpBusyBoxClass;

struct _GimpBusyBox
{
  GtkAlignment        parent_instance;

  GimpBusyBoxPrivate *priv;
};

struct _GimpBusyBoxClass
{
  GtkAlignmentClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType         gimp_busy_box_get_type    (void) G_GNUC_CONST;

GtkWidget   * gimp_busy_box_new         (const gchar *message);

void          gimp_busy_box_set_message (GimpBusyBox *box,
                                         const gchar *message);
const gchar * gimp_busy_box_get_message (GimpBusyBox *box);


G_END_DECLS

#endif /* __GIMP_BUSY_BOX_H__ */
