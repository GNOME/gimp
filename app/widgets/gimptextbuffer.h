/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextBuffer
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_TEXT_BUFFER            (gimp_text_buffer_get_type ())
#define GIMP_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_BUFFER, GimpTextBuffer))
#define GIMP_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_BUFFER))
#define GIMP_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_BUFFER, GimpTextBufferClass))
#define GIMP_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_BUFFER))


typedef struct _GimpTextBufferClass  GimpTextBufferClass;

struct _GimpTextBuffer
{
  GtkTextBuffer  parent_instance;

  GtkTextTag    *bold_tag;
  GtkTextTag    *italic_tag;
  GtkTextTag    *underline_tag;
  GtkTextTag    *strikethrough_tag;

  GList         *size_tags;
  GList         *baseline_tags;
  GList         *kerning_tags;
  GList         *font_tags;
  GList         *color_tags;

  GtkTextTag    *preedit_underline_tag;
  GList         *preedit_color_tags;
  GList         *preedit_bg_color_tags;

  gboolean       insert_tags_set;
  GList         *insert_tags;
  GList         *remove_tags;

  GdkAtom        markup_atom;
};

struct _GimpTextBufferClass
{
  GtkTextBufferClass  parent_class;

  void (* color_applied) (GimpTextBuffer *buffer,
                          GeglColor      *color);
};


GType            gimp_text_buffer_get_type             (void) G_GNUC_CONST;

GimpTextBuffer * gimp_text_buffer_new                  (void);

void             gimp_text_buffer_set_text             (GimpTextBuffer     *buffer,
                                                        const gchar        *text);
gchar          * gimp_text_buffer_get_text             (GimpTextBuffer     *buffer);

void             gimp_text_buffer_set_markup           (GimpTextBuffer     *buffer,
                                                        const gchar        *markup);
gchar          * gimp_text_buffer_get_markup           (GimpTextBuffer     *buffer);

gboolean         gimp_text_buffer_has_markup           (GimpTextBuffer     *buffer);

GtkTextTag     * gimp_text_buffer_get_iter_size        (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *size);
GtkTextTag     * gimp_text_buffer_get_size_tag         (GimpTextBuffer     *buffer,
                                                        gint                size);
void             gimp_text_buffer_set_size             (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                size);
void             gimp_text_buffer_change_size          (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                amount);

GtkTextTag     * gimp_text_buffer_get_iter_baseline    (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *baseline);
void             gimp_text_buffer_set_baseline         (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);
void             gimp_text_buffer_change_baseline      (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);

GtkTextTag     * gimp_text_buffer_get_iter_kerning     (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gint               *kerning);
void             gimp_text_buffer_set_kerning          (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);
void             gimp_text_buffer_change_kerning       (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        gint                count);

GtkTextTag     * gimp_text_buffer_get_iter_font        (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        gchar             **font);
GtkTextTag     * gimp_text_buffer_get_font_tag         (GimpTextBuffer     *buffer,
                                                        const gchar        *font);
void             gimp_text_buffer_set_font             (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        const gchar        *font);

GtkTextTag     * gimp_text_buffer_get_iter_color       (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter,
                                                        GeglColor         **color);
GtkTextTag     * gimp_text_buffer_get_color_tag        (GimpTextBuffer     *buffer,
                                                        GeglColor          *color);
void             gimp_text_buffer_set_color            (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);

GtkTextTag * gimp_text_buffer_get_preedit_color_tag    (GimpTextBuffer     *buffer,
                                                        GeglColor          *color);
void         gimp_text_buffer_set_preedit_color        (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);
GtkTextTag * gimp_text_buffer_get_preedit_bg_color_tag (GimpTextBuffer     *buffer,
                                                        GeglColor          *color);
void         gimp_text_buffer_set_preedit_bg_color     (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *start,
                                                        const GtkTextIter  *end,
                                                        GeglColor          *color);

const gchar    * gimp_text_buffer_tag_to_name          (GimpTextBuffer     *buffer,
                                                        GtkTextTag         *tag,
                                                        const gchar       **attribute,
                                                        gchar             **value);
GtkTextTag     * gimp_text_buffer_name_to_tag          (GimpTextBuffer     *buffer,
                                                        const gchar        *name,
                                                        const gchar        *attribute,
                                                        const gchar        *value);

void             gimp_text_buffer_set_insert_tags      (GimpTextBuffer     *buffer,
                                                        GList              *insert_tags,
                                                        GList              *remove_tags);
void             gimp_text_buffer_clear_insert_tags    (GimpTextBuffer     *buffer);
void             gimp_text_buffer_insert               (GimpTextBuffer     *buffer,
                                                        const gchar        *text);

gint             gimp_text_buffer_get_iter_index       (GimpTextBuffer     *buffer,
                                                        GtkTextIter        *iter,
                                                        gboolean            layout_index);
void             gimp_text_buffer_get_iter_at_index    (GimpTextBuffer     *buffer,
                                                        GtkTextIter        *iter,
                                                        gint                index,
                                                        gboolean            layout_index);

gboolean         gimp_text_buffer_load                 (GimpTextBuffer     *buffer,
                                                        GFile              *file,
                                                        GError            **error);
gboolean         gimp_text_buffer_save                 (GimpTextBuffer     *buffer,
                                                        GFile              *file,
                                                        gboolean            selection_only,
                                                        GError            **error);

GList          * gimp_text_buffer_get_tags_on_iter     (GimpTextBuffer     *buffer,
                                                        const GtkTextIter  *iter);
GList          * gimp_text_buffer_get_all_tags         (GimpTextBuffer     *buffer);
