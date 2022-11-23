/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_BUFFER_H__
#define __LIGMA_BUFFER_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_BUFFER            (ligma_buffer_get_type ())
#define LIGMA_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BUFFER, LigmaBuffer))
#define LIGMA_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BUFFER, LigmaBufferClass))
#define LIGMA_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BUFFER))
#define LIGMA_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BUFFER))
#define LIGMA_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BUFFER, LigmaBufferClass))


typedef struct _LigmaBufferClass LigmaBufferClass;

struct _LigmaBuffer
{
  LigmaViewable      parent_instance;

  GeglBuffer       *buffer;
  gint              offset_x;
  gint              offset_y;

  gdouble           resolution_x;
  gdouble           resolution_y;
  LigmaUnit          unit;

  LigmaColorProfile *color_profile;
  LigmaColorProfile *format_profile;
};

struct _LigmaBufferClass
{
  LigmaViewableClass  parent_class;
};


GType              ligma_buffer_get_type          (void) G_GNUC_CONST;

LigmaBuffer       * ligma_buffer_new               (GeglBuffer       *buffer,
                                                  const gchar      *name,
                                                  gint              offset_x,
                                                  gint              offset_y,
                                                  gboolean          copy_pixels);
LigmaBuffer       * ligma_buffer_new_from_pixbuf   (GdkPixbuf        *pixbuf,
                                                  const gchar      *name,
                                                  gint              offset_x,
                                                  gint              offset_y);

gint               ligma_buffer_get_width         (LigmaBuffer       *buffer);
gint               ligma_buffer_get_height        (LigmaBuffer       *buffer);
const Babl       * ligma_buffer_get_format        (LigmaBuffer       *buffer);

GeglBuffer       * ligma_buffer_get_buffer        (LigmaBuffer       *buffer);

void               ligma_buffer_set_resolution    (LigmaBuffer       *buffer,
                                                  gdouble           resolution_x,
                                                  gdouble           resolution_y);
gboolean           ligma_buffer_get_resolution    (LigmaBuffer       *buffer,
                                                  gdouble          *resolution_x,
                                                  gdouble          *resolution_y);

void               ligma_buffer_set_unit          (LigmaBuffer       *buffer,
                                                  LigmaUnit          unit);
LigmaUnit           ligma_buffer_get_unit          (LigmaBuffer       *buffer);

void               ligma_buffer_set_color_profile (LigmaBuffer       *buffer,
                                                  LigmaColorProfile *profile);
LigmaColorProfile * ligma_buffer_get_color_profile (LigmaBuffer       *buffer);


#endif /* __LIGMA_BUFFER_H__ */
