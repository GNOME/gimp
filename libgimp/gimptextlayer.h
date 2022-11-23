/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmatextlayer.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_TEXT_LAYER_H__
#define __LIGMA_TEXT_LAYER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_TEXT_LAYER (ligma_text_layer_get_type ())
G_DECLARE_FINAL_TYPE (LigmaTextLayer, ligma_text_layer, LIGMA, TEXT_LAYER, LigmaLayer)


LigmaTextLayer * ligma_text_layer_get_by_id (gint32       layer_id);

LigmaTextLayer * ligma_text_layer_new       (LigmaImage   *image,
                                           const gchar *text,
                                           const gchar *fontname,
                                           gdouble      size,
                                           LigmaUnit     unit);


G_END_DECLS

#endif /* __LIGMA_TEXT_LAYER_H__ */
