/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_TEXT_LAYER_H__
#define __LIGMA_TEXT_LAYER_H__


#include "core/ligmalayer.h"


#define LIGMA_TYPE_TEXT_LAYER            (ligma_text_layer_get_type ())
#define LIGMA_TEXT_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_LAYER, LigmaTextLayer))
#define LIGMA_TEXT_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_LAYER, LigmaTextLayerClass))
#define LIGMA_IS_TEXT_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_LAYER))
#define LIGMA_IS_TEXT_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_LAYER))
#define LIGMA_TEXT_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEXT_LAYER, LigmaTextLayerClass))


typedef struct _LigmaTextLayerClass   LigmaTextLayerClass;
typedef struct _LigmaTextLayerPrivate LigmaTextLayerPrivate;

struct _LigmaTextLayer
{
  LigmaLayer     layer;

  LigmaText     *text;
  const gchar  *text_parasite;  /*  parasite name that this text was set from,
                                 *  and that should be removed when the text
                                 *  is changed.
                                 */
  gboolean      auto_rename;
  gboolean      modified;

  const Babl   *convert_format;

  LigmaTextLayerPrivate *private;
};

struct _LigmaTextLayerClass
{
  LigmaLayerClass  parent_class;
};


GType       ligma_text_layer_get_type    (void) G_GNUC_CONST;

LigmaLayer * ligma_text_layer_new         (LigmaImage     *image,
                                         LigmaText      *text);
LigmaText  * ligma_text_layer_get_text    (LigmaTextLayer *layer);
void        ligma_text_layer_set_text    (LigmaTextLayer *layer,
                                         LigmaText      *text);
void        ligma_text_layer_discard     (LigmaTextLayer *layer);
void        ligma_text_layer_set         (LigmaTextLayer *layer,
                                         const gchar   *undo_desc,
                                         const gchar   *first_property_name,
                                         ...) G_GNUC_NULL_TERMINATED;

gboolean    ligma_item_is_text_layer     (LigmaItem      *item);


#endif /* __LIGMA_TEXT_LAYER_H__ */
