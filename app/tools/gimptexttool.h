/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@ligma.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_TOOL_H__
#define __LIGMA_TEXT_TOOL_H__


#include "ligmadrawtool.h"


#define LIGMA_TYPE_TEXT_TOOL            (ligma_text_tool_get_type ())
#define LIGMA_TEXT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEXT_TOOL, LigmaTextTool))
#define LIGMA_IS_TEXT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEXT_TOOL))
#define LIGMA_TEXT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEXT_TOOL, LigmaTextToolClass))
#define LIGMA_IS_TEXT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEXT_TOOL))

#define LIGMA_TEXT_TOOL_GET_OPTIONS(t)  (LIGMA_TEXT_OPTIONS (ligma_tool_get_options (LIGMA_TOOL (t))))


typedef struct _LigmaTextTool       LigmaTextTool;
typedef struct _LigmaTextToolClass  LigmaTextToolClass;

struct _LigmaTextTool
{
  LigmaDrawTool    parent_instance;

  LigmaText       *proxy;
  GList          *pending;
  guint           idle_id;

  gboolean        moving;

  LigmaTextBuffer *buffer;

  LigmaText       *text;
  LigmaTextLayer  *layer;
  LigmaImage      *image;

  GtkWidget      *confirm_dialog;
  LigmaUIManager  *ui_manager;

  gboolean        handle_rectangle_change_complete;
  gboolean        text_box_fixed;

  LigmaTextLayout *layout;
  gint            drawing_blocked;

  LigmaToolWidget *widget;
  LigmaToolWidget *grab_widget;

  /* text editor state: */

  GtkWidget      *style_overlay;
  GtkWidget      *style_editor;

  gboolean        selecting;
  GtkTextIter     select_start_iter;
  gboolean        select_words;
  gboolean        select_lines;

  GtkIMContext   *im_context;
  gboolean        needs_im_reset;

  gboolean        preedit_active;
  gchar          *preedit_string;
  gint            preedit_cursor;
  GtkTextMark    *preedit_start;
  GtkTextMark    *preedit_end;

  gboolean        overwrite_mode;
  gint            x_pos;

  GtkWidget      *offscreen_window;
  GtkWidget      *proxy_text_view;

  GtkWidget      *editor_dialog;
};

struct _LigmaTextToolClass
{
  LigmaDrawToolClass  parent_class;
};


void       ligma_text_tool_register               (LigmaToolRegisterCallback  callback,
                                                  gpointer                  data);

GType      ligma_text_tool_get_type               (void) G_GNUC_CONST;

gboolean   ligma_text_tool_set_layer              (LigmaTextTool  *text_tool,
                                                  LigmaLayer     *layer);

gboolean   ligma_text_tool_get_has_text_selection (LigmaTextTool  *text_tool);

void       ligma_text_tool_delete_selection       (LigmaTextTool  *text_tool);
void       ligma_text_tool_cut_clipboard          (LigmaTextTool  *text_tool);
void       ligma_text_tool_copy_clipboard         (LigmaTextTool  *text_tool);
void       ligma_text_tool_paste_clipboard        (LigmaTextTool  *text_tool);

void       ligma_text_tool_create_vectors         (LigmaTextTool  *text_tool);
gboolean   ligma_text_tool_create_vectors_warped  (LigmaTextTool  *text_tool,
                                                  GError       **error);

LigmaTextDirection
           ligma_text_tool_get_direction          (LigmaTextTool  *text_tool);

/*  only for the text editor  */
void       ligma_text_tool_clear_layout           (LigmaTextTool  *text_tool);
gboolean   ligma_text_tool_ensure_layout          (LigmaTextTool  *text_tool);
void       ligma_text_tool_apply                  (LigmaTextTool  *text_tool,
                                                  gboolean       push_undo);


#endif /* __LIGMA_TEXT_TOOL_H__ */
