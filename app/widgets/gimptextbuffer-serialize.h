/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextBuffer-serialize
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
 *
 * inspired by
 * gtktextbufferserialize.h
 * Copyright (C) 2004  Nokia Corporation.
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

#ifndef __LIGMA_TEXT_BUFFER_SERIALIZE_H__
#define __LIGMA_TEXT_BUFFER_SERIALIZE_H__


#define WORD_JOINER        "\342\201\240"
#define WORD_JOINER_LENGTH 3


guint8   * ligma_text_buffer_serialize        (GtkTextBuffer     *register_buffer,
                                              GtkTextBuffer     *content_buffer,
                                              const GtkTextIter *start,
                                              const GtkTextIter *end,
                                              gsize             *length,
                                              gpointer           user_data);
gboolean   ligma_text_buffer_deserialize      (GtkTextBuffer     *register_buffer,
                                              GtkTextBuffer     *content_buffer,
                                              GtkTextIter       *iter,
                                              const guint8      *data,
                                              gsize              length,
                                              gboolean           create_tags,
                                              gpointer           user_data,
                                              GError           **error);

void       ligma_text_buffer_pre_serialize    (LigmaTextBuffer    *buffer,
                                              GtkTextBuffer     *content);
void       ligma_text_buffer_post_deserialize (LigmaTextBuffer    *buffer,
                                              GtkTextBuffer     *content);



#endif /* __LIGMA_TEXT_BUFFER_SERIALIZE_H__ */
