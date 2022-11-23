/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabuffersourcebox.h
 * Copyright (C) 2015  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_BUFFER_SOURCE_BOX_H__
#define __LIGMA_BUFFER_SOURCE_BOX_H__


#define LIGMA_TYPE_BUFFER_SOURCE_BOX            (ligma_buffer_source_box_get_type ())
#define LIGMA_BUFFER_SOURCE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUFFER_SOURCE_BOX, LigmaBufferSourceBox))
#define LIGMA_BUFFER_SOURCE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUFFER_SOURCE_BOX, LigmaBufferSourceBoxClass))
#define LIGMA_IS_BUFFER_SOURCE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUFFER_SOURCE_BOX))
#define LIGMA_IS_BUFFER_SOURCE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUFFER_SOURCE_BOX))
#define LIGMA_BUFFER_SOURCE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUFFER_SOURCE_BOX, LigmaBufferSourceBoxClass))


typedef struct _LigmaBufferSourceBoxPrivate LigmaBufferSourceBoxPrivate;
typedef struct _LigmaBufferSourceBoxClass   LigmaBufferSourceBoxClass;

struct _LigmaBufferSourceBox
{
  GtkBox                      parent_instance;

  LigmaBufferSourceBoxPrivate *priv;
};

struct _LigmaBufferSourceBoxClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_buffer_source_box_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_buffer_source_box_new        (LigmaContext         *context,
                                               GeglNode            *source_node,
                                               const gchar         *name);

GtkWidget * ligma_buffer_source_box_get_toggle (LigmaBufferSourceBox *box);


#endif  /*  __LIGMA_BUFFER_SOURCE_BOX_H__  */
