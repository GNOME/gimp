/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TEXT_TOOL_H__
#define __GIMP_TEXT_TOOL_H__


#include "gimpdrawtool.h"


#define GIMP_TYPE_TEXT_TOOL            (gimp_text_tool_get_type ())
#define GIMP_TEXT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_TOOL, GimpTextTool))
#define GIMP_IS_TEXT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_TOOL))
#define GIMP_TEXT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT_TOOL, GimpTextToolClass))
#define GIMP_IS_TEXT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT_TOOL))

#define GIMP_TEXT_TOOL_GET_OPTIONS(t)  (GIMP_TEXT_OPTIONS (gimp_tool_get_options (GIMP_TOOL (t))))


typedef struct _GimpTextTool       GimpTextTool;
typedef struct _GimpTextToolClass  GimpTextToolClass;

struct _GimpTextTool
{
  GimpDrawTool    parent_instance;

  GimpText       *proxy;
  GList          *pending;
  guint           idle_id;

  gboolean        moving;

  GimpTextBuffer *buffer;

  GimpText       *text;
  GimpTextLayer  *layer;
  GimpImage      *image;

  GtkWidget      *confirm_dialog;

  gboolean        handle_rectangle_change_complete;
  gboolean        text_box_fixed;

  GimpTextLayout *layout;
  gint            drawing_blocked;

  GimpToolWidget *widget;
  GimpToolWidget *grab_widget;

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

struct _GimpTextToolClass
{
  GimpDrawToolClass  parent_class;
};


void       gimp_text_tool_register               (GimpToolRegisterCallback  callback,
                                                  gpointer                  data);

GType      gimp_text_tool_get_type               (void) G_GNUC_CONST;

gboolean   gimp_text_tool_set_layer              (GimpTextTool  *text_tool,
                                                  GimpLayer     *layer);

gboolean   gimp_text_tool_get_has_text_selection (GimpTextTool  *text_tool);

void       gimp_text_tool_delete_selection       (GimpTextTool  *text_tool);
void       gimp_text_tool_cut_clipboard          (GimpTextTool  *text_tool);
void       gimp_text_tool_copy_clipboard         (GimpTextTool  *text_tool);
void       gimp_text_tool_paste_clipboard        (GimpTextTool  *text_tool);

void       gimp_text_tool_create_vectors         (GimpTextTool  *text_tool);
gboolean   gimp_text_tool_create_vectors_warped  (GimpTextTool  *text_tool,
                                                  GError       **error);

GimpTextDirection
           gimp_text_tool_get_direction          (GimpTextTool  *text_tool);

/*  only for the text editor  */
void       gimp_text_tool_clear_layout           (GimpTextTool  *text_tool);
gboolean   gimp_text_tool_ensure_layout          (GimpTextTool  *text_tool);
void       gimp_text_tool_apply                  (GimpTextTool  *text_tool,
                                                  gboolean       push_undo);


#endif /* __GIMP_TEXT_TOOL_H__ */
