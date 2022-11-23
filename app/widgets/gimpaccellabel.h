/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaaccellabel.h
 * Copyright (C) 2020 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_ACCEL_LABEL_H__
#define __LIGMA_ACCEL_LABEL_H__


#define LIGMA_TYPE_ACCEL_LABEL            (ligma_accel_label_get_type ())
#define LIGMA_ACCEL_LABEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACCEL_LABEL, LigmaAccelLabel))
#define LIGMA_ACCEL_LABEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ACCEL_LABEL, LigmaAccelLabelClass))
#define LIGMA_IS_ACCEL_LABEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_ACCEL_LABEL))
#define LIGMA_IS_ACCEL_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ACCEL_LABEL))
#define LIGMA_ACCEL_LABEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ACCEL_LABEL, LigmaAccelLabelClass))


typedef struct _LigmaAccelLabelPrivate LigmaAccelLabelPrivate;
typedef struct _LigmaAccelLabelClass   LigmaAccelLabelClass;

struct _LigmaAccelLabel
{
  GtkLabel               parent_instance;

  LigmaAccelLabelPrivate *priv;
};

struct _LigmaAccelLabelClass
{
  GtkLabelClass  parent_class;
};


GType        ligma_accel_label_get_type   (void) G_GNUC_CONST;

GtkWidget  * ligma_accel_label_new        (LigmaAction     *action);

void         ligma_accel_label_set_action (LigmaAccelLabel *accel_label,
                                          LigmaAction     *action);
LigmaAction * ligma_accel_label_get_action (LigmaAccelLabel *accel_label);


#endif /* __LIGMA_ACCEL_LABEL_H__ */
