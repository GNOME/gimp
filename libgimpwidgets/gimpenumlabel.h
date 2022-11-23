/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumlabel.h
 * Copyright (C) 2005  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_ENUM__LABEL_H__
#define __LIGMA_ENUM__LABEL_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_ENUM_LABEL            (ligma_enum_label_get_type ())
#define LIGMA_ENUM_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ENUM_LABEL, LigmaEnumLabel))
#define LIGMA_ENUM_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ENUM_LABEL, LigmaEnumLabelClass))
#define LIGMA_IS_ENUM_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ENUM_LABEL))
#define LIGMA_IS_ENUM_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ENUM_LABEL))
#define LIGMA_ENUM_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ENUM_LABEL, LigmaEnumLabelClass))


typedef struct _LigmaEnumLabelPrivate LigmaEnumLabelPrivate;
typedef struct _LigmaEnumLabelClass   LigmaEnumLabelClass;

struct _LigmaEnumLabel
{
  GtkLabel              parent_instance;

  LigmaEnumLabelPrivate *priv;
};

struct _LigmaEnumLabelClass
{
  GtkLabelClass  parent_class;

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


GType       ligma_enum_label_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_enum_label_new              (GType          enum_type,
                                              gint           value);
void        ligma_enum_label_set_value        (LigmaEnumLabel *label,
                                              gint           value);


G_END_DECLS

#endif  /* __LIGMA_ENUM_LABEL_H__ */
