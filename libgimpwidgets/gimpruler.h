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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_RULER_H__
#define __LIGMA_RULER_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_RULER            (ligma_ruler_get_type ())
#define LIGMA_RULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_RULER, LigmaRuler))
#define LIGMA_RULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_RULER, LigmaRulerClass))
#define LIGMA_IS_RULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_RULER))
#define LIGMA_IS_RULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_RULER))
#define LIGMA_RULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_RULER, LigmaRulerClass))


typedef struct _LigmaRulerPrivate LigmaRulerPrivate;
typedef struct _LigmaRulerClass   LigmaRulerClass;

struct _LigmaRuler
{
  GtkWidget         parent_instance;

  LigmaRulerPrivate *priv;
};

struct _LigmaRulerClass
{
  GtkWidgetClass  parent_class;

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


GType       ligma_ruler_get_type            (void) G_GNUC_CONST;

GtkWidget * ligma_ruler_new                 (GtkOrientation  orientation);

void        ligma_ruler_add_track_widget    (LigmaRuler      *ruler,
                                            GtkWidget      *widget);
void        ligma_ruler_remove_track_widget (LigmaRuler      *ruler,
                                            GtkWidget      *widget);

void        ligma_ruler_set_unit            (LigmaRuler      *ruler,
                                            LigmaUnit        unit);
LigmaUnit    ligma_ruler_get_unit            (LigmaRuler      *ruler);
void        ligma_ruler_set_position        (LigmaRuler      *ruler,
                                            gdouble         position);
gdouble     ligma_ruler_get_position        (LigmaRuler      *ruler);
void        ligma_ruler_set_range           (LigmaRuler      *ruler,
                                            gdouble         lower,
                                            gdouble         upper,
                                            gdouble         max_size);
void        ligma_ruler_get_range           (LigmaRuler      *ruler,
                                            gdouble        *lower,
                                            gdouble        *upper,
                                            gdouble        *max_size);

G_END_DECLS

#endif /* __LIGMA_RULER_H__ */
