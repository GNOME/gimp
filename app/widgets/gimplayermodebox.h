/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * ligmalayermodebox.h
 * Copyright (C) 2017  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_LAYER_MODE_BOX_H__
#define __LIGMA_LAYER_MODE_BOX_H__


#define LIGMA_TYPE_LAYER_MODE_BOX            (ligma_layer_mode_box_get_type ())
#define LIGMA_LAYER_MODE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_MODE_BOX, LigmaLayerModeBox))
#define LIGMA_LAYER_MODE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LAYER_MODE_BOX, LigmaLayerModeBoxClass))
#define LIGMA_IS_LAYER_MODE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_MODE_BOX))
#define LIGMA_IS_LAYER_MODE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LAYER_MODE_BOX))
#define LIGMA_LAYER_MODE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LAYER_MODE_BOX, LigmaLayerModeBoxClass))


typedef struct _LigmaLayerModeBoxPrivate LigmaLayerModeBoxPrivate;
typedef struct _LigmaLayerModeBoxClass   LigmaLayerModeBoxClass;

struct _LigmaLayerModeBox
{
  GtkBox                   parent_instance;

  LigmaLayerModeBoxPrivate *priv;
};

struct _LigmaLayerModeBoxClass
{
  GtkBoxClass  parent_class;
};


GType                  ligma_layer_mode_box_get_type      (void) G_GNUC_CONST;

GtkWidget            * ligma_layer_mode_box_new           (LigmaLayerModeContext  context);

void                   ligma_layer_mode_box_set_context   (LigmaLayerModeBox     *box,
                                                          LigmaLayerModeContext  context);
LigmaLayerModeContext   ligma_layer_mode_box_get_context   (LigmaLayerModeBox     *box);

void                   ligma_layer_mode_box_set_mode      (LigmaLayerModeBox     *box,
                                                          LigmaLayerMode         mode);
LigmaLayerMode          ligma_layer_mode_box_get_mode      (LigmaLayerModeBox     *box);

void                   ligma_layer_mode_box_set_label     (LigmaLayerModeBox     *box,
                                                          const gchar          *label);
void                   ligma_layer_mode_box_set_ellipsize (LigmaLayerModeBox     *box,
                                                          PangoEllipsizeMode    mode);


#endif  /* __LIGMA_LAYER_MODE_BOX_H__ */
