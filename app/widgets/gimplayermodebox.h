/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimplayermodebox.h
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_LAYER_MODE_BOX            (gimp_layer_mode_box_get_type ())
#define GIMP_LAYER_MODE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_MODE_BOX, GimpLayerModeBox))
#define GIMP_LAYER_MODE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_MODE_BOX, GimpLayerModeBoxClass))
#define GIMP_IS_LAYER_MODE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_MODE_BOX))
#define GIMP_IS_LAYER_MODE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_MODE_BOX))
#define GIMP_LAYER_MODE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LAYER_MODE_BOX, GimpLayerModeBoxClass))


typedef struct _GimpLayerModeBoxPrivate GimpLayerModeBoxPrivate;
typedef struct _GimpLayerModeBoxClass   GimpLayerModeBoxClass;

struct _GimpLayerModeBox
{
  GtkBox                   parent_instance;

  GimpLayerModeBoxPrivate *priv;
};

struct _GimpLayerModeBoxClass
{
  GtkBoxClass  parent_class;
};


GType                  gimp_layer_mode_box_get_type      (void) G_GNUC_CONST;

GtkWidget            * gimp_layer_mode_box_new           (GimpLayerModeContext  context);

void                   gimp_layer_mode_box_set_context   (GimpLayerModeBox     *box,
                                                          GimpLayerModeContext  context);
GimpLayerModeContext   gimp_layer_mode_box_get_context   (GimpLayerModeBox     *box);

void                   gimp_layer_mode_box_set_mode      (GimpLayerModeBox     *box,
                                                          GimpLayerMode         mode);
GimpLayerMode          gimp_layer_mode_box_get_mode      (GimpLayerModeBox     *box);

void                   gimp_layer_mode_box_set_label     (GimpLayerModeBox     *box,
                                                          const gchar          *label);
void                   gimp_layer_mode_box_set_ellipsize (GimpLayerModeBox     *box,
                                                          PangoEllipsizeMode    mode);
