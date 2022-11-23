/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextBuffer
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_BUFFER_H__
#define __LIGMA_TEXT_BUFFER_H__


#define LIGMA_TYPE_TEXT_BUFFER            (ligma_text_buffer_get_type ())
#define LIGMA_TEXT_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_BUFFER, LigmaTextBuffer))
#define LIGMA_IS_TEXT_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_BUFFER))
#define LIGMA_TEXT_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_BUFFER, LigmaTextBufferClass))
#define LIGMA_IS_TEXT_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_BUFFER))


typedef struct _LigmaTextBufferClass  LigmaTextBufferClass;

struct _LigmaTextBuffer
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

struct _LigmaTextBufferClass
{
  GtkTextBufferClass  parent_class;

  void (* color_applied) (LigmaTextBuffer *buffer,
                          const LigmaRGB  *color);
};


GType            ligma_text_buffer_get_type          (void) G_GNUC_CONST;

LigmaTextBuffer * ligma_text_buffer_new               (void);

void             ligma_text_buffer_set_text          (LigmaTextBuffer    *buffer,
                                                     const gchar       *text);
gchar          * ligma_text_buffer_get_text          (LigmaTextBuffer    *buffer);

void             ligma_text_buffer_set_markup        (LigmaTextBuffer    *buffer,
                                                     const gchar       *markup);
gchar          * ligma_text_buffer_get_markup        (LigmaTextBuffer    *buffer);

gboolean         ligma_text_buffer_has_markup        (LigmaTextBuffer    *buffer);

GtkTextTag     * ligma_text_buffer_get_iter_size     (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *iter,
                                                     gint              *size);
GtkTextTag     * ligma_text_buffer_get_size_tag      (LigmaTextBuffer    *buffer,
                                                     gint               size);
void             ligma_text_buffer_set_size          (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               size);
void             ligma_text_buffer_change_size       (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               amount);

GtkTextTag     * ligma_text_buffer_get_iter_baseline (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *iter,
                                                     gint              *baseline);
void             ligma_text_buffer_set_baseline      (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               count);
void             ligma_text_buffer_change_baseline   (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               count);

GtkTextTag     * ligma_text_buffer_get_iter_kerning  (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *iter,
                                                     gint              *kerning);
void             ligma_text_buffer_set_kerning       (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               count);
void             ligma_text_buffer_change_kerning    (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     gint               count);

GtkTextTag     * ligma_text_buffer_get_iter_font     (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *iter,
                                                     gchar            **font);
GtkTextTag     * ligma_text_buffer_get_font_tag      (LigmaTextBuffer    *buffer,
                                                     const gchar       *font);
void             ligma_text_buffer_set_font          (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     const gchar       *font);

GtkTextTag     * ligma_text_buffer_get_iter_color    (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *iter,
                                                     LigmaRGB           *color);
GtkTextTag     * ligma_text_buffer_get_color_tag     (LigmaTextBuffer    *buffer,
                                                     const LigmaRGB     *color);
void             ligma_text_buffer_set_color         (LigmaTextBuffer    *buffer,
                                                     const GtkTextIter *start,
                                                     const GtkTextIter *end,
                                                     const LigmaRGB     *color);

GtkTextTag * ligma_text_buffer_get_preedit_color_tag    (LigmaTextBuffer    *buffer,
                                                        const LigmaRGB     *color);
void         ligma_text_buffer_set_preedit_color        (LigmaTextBuffer    *buffer,
                                                        const GtkTextIter *start,
                                                        const GtkTextIter *end,
                                                        const LigmaRGB     *color);
GtkTextTag * ligma_text_buffer_get_preedit_bg_color_tag (LigmaTextBuffer    *buffer,
                                                        const LigmaRGB     *color);
void         ligma_text_buffer_set_preedit_bg_color     (LigmaTextBuffer    *buffer,
                                                        const GtkTextIter *start,
                                                        const GtkTextIter *end,
                                                        const LigmaRGB     *color);

const gchar    * ligma_text_buffer_tag_to_name       (LigmaTextBuffer    *buffer,
                                                     GtkTextTag        *tag,
                                                     const gchar      **attribute,
                                                     gchar            **value);
GtkTextTag     * ligma_text_buffer_name_to_tag       (LigmaTextBuffer    *buffer,
                                                     const gchar       *name,
                                                     const gchar       *attribute,
                                                     const gchar       *value);

void             ligma_text_buffer_set_insert_tags   (LigmaTextBuffer    *buffer,
                                                     GList             *insert_tags,
                                                     GList             *remove_tags);
void             ligma_text_buffer_clear_insert_tags (LigmaTextBuffer    *buffer);
void             ligma_text_buffer_insert            (LigmaTextBuffer    *buffer,
                                                     const gchar       *text);

gint             ligma_text_buffer_get_iter_index    (LigmaTextBuffer    *buffer,
                                                     GtkTextIter       *iter,
                                                     gboolean           layout_index);
void             ligma_text_buffer_get_iter_at_index (LigmaTextBuffer    *buffer,
                                                     GtkTextIter       *iter,
                                                     gint               index,
                                                     gboolean           layout_index);

gboolean         ligma_text_buffer_load              (LigmaTextBuffer    *buffer,
                                                     GFile             *file,
                                                     GError           **error);
gboolean         ligma_text_buffer_save              (LigmaTextBuffer    *buffer,
                                                     GFile             *file,
                                                     gboolean           selection_only,
                                                     GError           **error);


#endif /* __LIGMA_TEXT_BUFFER_H__ */
