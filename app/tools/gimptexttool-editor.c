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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "text/gimptext.h"
#include "text/gimptextlayout.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimptextproxy.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimprectangletool.h"
#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptexttool-editor.h"

#include "gimp-log.h"
#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_text_tool_ensure_proxy       (GimpTextTool    *text_tool);
static void   gimp_text_tool_move_cursor        (GimpTextTool    *text_tool,
                                                 GtkMovementStep  step,
                                                 gint             count,
                                                 gboolean         extend_selection);
static void   gimp_text_tool_insert_at_cursor   (GimpTextTool    *text_tool,
                                                 const gchar     *str);
static void   gimp_text_tool_delete_from_cursor (GimpTextTool    *text_tool,
                                                 GtkDeleteType    type,
                                                 gint             count);
static void   gimp_text_tool_backspace          (GimpTextTool    *text_tool);
static void   gimp_text_tool_toggle_overwrite   (GimpTextTool    *text_tool);
static void   gimp_text_tool_select_all         (GimpTextTool    *text_tool,
                                                 gboolean         select);
static void   gimp_text_tool_options_notify     (GimpTextOptions *options,
                                                 GParamSpec      *pspec,
                                                 GimpTextTool    *text_tool);
static void   gimp_text_tool_editor_dialog      (GimpTextTool    *text_tool);
static void   gimp_text_tool_enter_text         (GimpTextTool    *text_tool,
                                                 const gchar     *str);
static void   gimp_text_tool_commit_cb          (GtkIMContext    *context,
                                                 const gchar     *str,
                                                 GimpTextTool    *text_tool);
static void   gimp_text_tool_preedit_start      (GtkIMContext    *context,
                                                 GimpTextTool    *text_tool);
static void   gimp_text_tool_preedit_end        (GtkIMContext    *context,
                                                 GimpTextTool    *text_tool);
static void   gimp_text_tool_preedit_changed    (GtkIMContext    *context,
                                                 GimpTextTool    *text_tool);


/*  public functions  */

void
gimp_text_tool_editor_init (GimpTextTool *text_tool)
{
  text_tool->im_context     = gtk_im_multicontext_new ();
  text_tool->needs_im_reset = FALSE;

  text_tool->preedit_string = NULL;
  text_tool->preedit_cursor = 0;
  text_tool->overwrite_mode = FALSE;
  text_tool->x_pos          = -1;

  g_signal_connect (text_tool->im_context, "commit",
                    G_CALLBACK (gimp_text_tool_commit_cb),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-start",
                    G_CALLBACK (gimp_text_tool_preedit_start),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-end",
                    G_CALLBACK (gimp_text_tool_preedit_end),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-changed",
                    G_CALLBACK (gimp_text_tool_preedit_changed),
                    text_tool);

}

void
gimp_text_tool_editor_finalize (GimpTextTool *text_tool)
{
  if (text_tool->im_context)
    {
      g_object_unref (text_tool->im_context);
      text_tool->im_context = NULL;
    }
}

void
gimp_text_tool_editor_start (GimpTextTool *text_tool)
{
  GimpTool         *tool    = GIMP_TOOL (text_tool);
  GimpTextOptions  *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpDisplayShell *shell   = gimp_display_get_shell (tool->display);

  gtk_im_context_set_client_window (text_tool->im_context,
                                    gtk_widget_get_window (shell->canvas));

  text_tool->needs_im_reset = TRUE;
  gimp_text_tool_reset_im_context (text_tool);

  gtk_im_context_focus_in (text_tool->im_context);

  if (text_tool->text)
    gtk_text_buffer_set_text (text_tool->text_buffer,
                              text_tool->text->text, -1);
  else
    gtk_text_buffer_set_text (text_tool->text_buffer, "", -1);

  if (options->use_editor)
    gimp_text_tool_editor_dialog (text_tool);

  g_signal_connect (options, "notify::use-editor",
                    G_CALLBACK (gimp_text_tool_options_notify),
                    text_tool);
}

void
gimp_text_tool_editor_halt (GimpTextTool *text_tool)
{
  GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);

  g_signal_handlers_disconnect_by_func (options,
                                        gimp_text_tool_options_notify,
                                        text_tool);

  if (text_tool->editor_dialog)
    gtk_widget_destroy (text_tool->editor_dialog);

  if (text_tool->proxy_text_view)
    {
      gtk_widget_destroy (text_tool->offscreen_window);
      text_tool->offscreen_window = NULL;
      text_tool->proxy_text_view = NULL;
    }

  text_tool->needs_im_reset = TRUE;
  gimp_text_tool_reset_im_context (text_tool);

  gtk_im_context_focus_out (text_tool->im_context);

  gtk_im_context_set_client_window (text_tool->im_context, NULL);
}

gboolean
gimp_text_tool_editor_key_press (GimpTextTool *text_tool,
                                 GdkEventKey  *kevent,
                                 GimpDisplay  *display)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  GtkTextIter    cursor;
  GtkTextIter    selection;
  gint           x_pos  = -1;
  gboolean       retval = TRUE;

  if (gtk_im_context_filter_keypress (text_tool->im_context, kevent))
    {
      text_tool->needs_im_reset = TRUE;
      text_tool->x_pos          = -1;

      return TRUE;
    }

  gimp_text_tool_ensure_proxy (text_tool);

  if (gtk_bindings_activate_event (GTK_OBJECT (text_tool->proxy_text_view),
                                   kevent))
    {
      GIMP_LOG (TEXT_EDITING, "binding handled event");

      return TRUE;
    }

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_get_iter_at_mark (buffer, &selection,
                                    gtk_text_buffer_get_selection_bound (buffer));

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  switch (kevent->keyval)
    {
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_enter_text (text_tool, "\n");
      break;

    case GDK_Tab:
    case GDK_KP_Tab:
    case GDK_ISO_Left_Tab:
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_enter_text (text_tool, "\t");
      break;

    case GDK_Escape:
      gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (text_tool));
      gimp_tool_control (GIMP_TOOL (text_tool), GIMP_TOOL_ACTION_HALT,
                         GIMP_TOOL (text_tool)->display);
      break;

    default:
      retval = FALSE;
    }

  text_tool->x_pos = x_pos;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));

  return retval;
}

gboolean
gimp_text_tool_editor_key_release (GimpTextTool *text_tool,
                                   GdkEventKey  *kevent,
                                   GimpDisplay  *display)
{
  if (gtk_im_context_filter_keypress (text_tool->im_context, kevent))
    {
      text_tool->needs_im_reset = TRUE;

      return TRUE;
    }

  gimp_text_tool_ensure_proxy (text_tool);

  if (gtk_bindings_activate_event (GTK_OBJECT (text_tool->proxy_text_view),
                                   kevent))
    {
      GIMP_LOG (TEXT_EDITING, "binding handled event");

      return TRUE;
    }

  return FALSE;
}

void
gimp_text_tool_reset_im_context (GimpTextTool *text_tool)
{
  if (text_tool->needs_im_reset)
    {
      text_tool->needs_im_reset = FALSE;
      gtk_im_context_reset (text_tool->im_context);
    }
}

gchar *
gimp_text_tool_editor_get_text (GimpTextTool *text_tool)
{
  GtkTextIter start, end;

  gtk_text_buffer_get_bounds (text_tool->text_buffer, &start, &end);

  return gtk_text_buffer_get_text (text_tool->text_buffer,
                                   &start, &end, TRUE);
}

void
gimp_text_tool_editor_get_cursor_rect (GimpTextTool   *text_tool,
                                       PangoRectangle *cursor_rect,
                                       gint           *logical_off_x,
                                       gint           *logical_off_y)
{
  GtkTextBuffer  *buffer = text_tool->text_buffer;
  PangoLayout    *layout;
  PangoRectangle  ink_extents;
  PangoRectangle  logical_extents;
  GtkTextIter     start;
  GtkTextIter     cursor;
  gint            cursor_index;
  gchar          *string;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));
  g_return_if_fail (cursor_rect != NULL);

  if (! text_tool->layout)
    gimp_text_tool_update_layout (text_tool);

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);

  pango_layout_get_pixel_extents (layout, &ink_extents, &logical_extents);
  gimp_text_layout_transform_rect (text_tool->layout, &logical_extents);

  if (ink_extents.x < 0)
    *logical_off_x = -ink_extents.x;
  else
    *logical_off_x = 0;

  if (ink_extents.y < 0)
    *logical_off_y = -ink_extents.y;
  else
    *logical_off_y = 0;

  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));

  string = gtk_text_buffer_get_text (buffer, &start, &cursor, TRUE);
  cursor_index = strlen (string);
  g_free (string);

  pango_layout_index_to_pos (layout, cursor_index, cursor_rect);
  gimp_text_layout_transform_rect (text_tool->layout, cursor_rect);

  cursor_rect->x      = PANGO_PIXELS (cursor_rect->x) + *logical_off_x;
  cursor_rect->y      = PANGO_PIXELS (cursor_rect->y) + *logical_off_y;
  cursor_rect->width  = PANGO_PIXELS (cursor_rect->width);
  cursor_rect->height = PANGO_PIXELS (cursor_rect->height);
}


/*  private functions  */

static void
gimp_text_tool_ensure_proxy (GimpTextTool *text_tool)
{
  GimpTool         *tool  = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

  if (text_tool->offscreen_window &&
      gtk_widget_get_screen (text_tool->offscreen_window) !=
      gtk_widget_get_screen (GTK_WIDGET (shell)))
    {
      gtk_window_set_screen (GTK_WINDOW (text_tool->offscreen_window),
                             gtk_widget_get_screen (GTK_WIDGET (shell)));
      gtk_window_move (GTK_WINDOW (text_tool->offscreen_window), -200, -200);
    }
  else if (! text_tool->offscreen_window)
    {
      text_tool->offscreen_window = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_window_set_screen (GTK_WINDOW (text_tool->offscreen_window),
                             gtk_widget_get_screen (GTK_WIDGET (shell)));
      gtk_window_move (GTK_WINDOW (text_tool->offscreen_window), -200, -200);
      gtk_widget_show (text_tool->offscreen_window);

      text_tool->proxy_text_view = gimp_text_proxy_new ();
      gtk_container_add (GTK_CONTAINER (text_tool->offscreen_window),
                         text_tool->proxy_text_view);
      gtk_widget_show (text_tool->proxy_text_view);

      g_signal_connect_swapped (text_tool->proxy_text_view, "move-cursor",
                                G_CALLBACK (gimp_text_tool_move_cursor),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "insert-at-cursor",
                                G_CALLBACK (gimp_text_tool_insert_at_cursor),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "delete-from-cursor",
                                G_CALLBACK (gimp_text_tool_delete_from_cursor),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "backspace",
                                G_CALLBACK (gimp_text_tool_backspace),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "cut-clipboard",
                                G_CALLBACK (gimp_text_tool_cut_clipboard),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "copy-clipboard",
                                G_CALLBACK (gimp_text_tool_copy_clipboard),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "paste-clipboard",
                                G_CALLBACK (gimp_text_tool_paste_clipboard),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "toggle-overwrite",
                                G_CALLBACK (gimp_text_tool_toggle_overwrite),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "select-all",
                                G_CALLBACK (gimp_text_tool_select_all),
                                text_tool);
    }
}

static void
gimp_text_tool_move_cursor (GimpTextTool    *text_tool,
                            GtkMovementStep  step,
                            gint             count,
                            gboolean         extend_selection)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  GtkTextIter    cursor;
  GtkTextIter    selection;
  GtkTextIter   *sel_start;
  gboolean       cancel_selection = FALSE;
  gint           x_pos  = -1;

  GIMP_LOG (TEXT_EDITING, "%s count = %d, select = %s",
            g_enum_get_value (g_type_class_ref (GTK_TYPE_MOVEMENT_STEP),
                              step)->value_name,
            count,
            extend_selection ? "TRUE" : "FALSE");

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_get_iter_at_mark (buffer, &selection,
                                    gtk_text_buffer_get_selection_bound (buffer));

  if (extend_selection)
    {
      sel_start = &selection;
    }
  else
    {
      /*  when there is a selection, moving the cursor without
       *  extending it should move the cursor to the end of the
       *  selection that is in moving direction
       */
      if (count > 0)
        gtk_text_iter_order (&selection, &cursor);
      else
        gtk_text_iter_order (&cursor, &selection);

      sel_start = &cursor;

      /* if we actually have a selection, just move *to* the beginning/end
       * of the selection and not *from* there on LOGICAL_POSITIONS
       * and VISUAL_POSITIONS movement
       */
      if (! gtk_text_iter_equal (&cursor, &selection))
        cancel_selection = TRUE;
    }

  switch (step)
    {
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
      if (! cancel_selection)
        gtk_text_iter_forward_visible_cursor_positions (&cursor, count);
      break;

    case GTK_MOVEMENT_VISUAL_POSITIONS:
      if (! cancel_selection)
        {
          if (count < 0)
            gtk_text_iter_backward_cursor_position (&cursor);
          else if (count > 0)
            gtk_text_iter_forward_cursor_position (&cursor);
        }
      break;

    case GTK_MOVEMENT_WORDS:
      if (count < 0)
        {
          gtk_text_iter_backward_visible_word_starts (&cursor, -count);
        }
      else if (count > 0)
        {
	  if (! gtk_text_iter_forward_visible_word_ends (&cursor, count))
	    gtk_text_iter_forward_to_line_end (&cursor);
        }
      break;

    case GTK_MOVEMENT_DISPLAY_LINES:
      {
        GtkTextIter      start;
        GtkTextIter      end;
        gchar           *string;
        gint             cursor_index;
        PangoLayout     *layout;
        PangoLayoutLine *layout_line;
        PangoLayoutIter *layout_iter;
        PangoRectangle   logical;
        gint             line;
        gint             trailing;
        gint             i;

        gtk_text_buffer_get_bounds (buffer, &start, &end);

        string = gtk_text_buffer_get_text (buffer, &start, &cursor, TRUE);
        cursor_index = strlen (string);
        g_free (string);

        layout = gimp_text_layout_get_pango_layout (text_tool->layout);

        pango_layout_index_to_line_x (layout, cursor_index, FALSE,
                                      &line, &x_pos);

        layout_iter = pango_layout_get_iter (layout);
        for (i = 0; i < line; i++)
          pango_layout_iter_next_line (layout_iter);

        pango_layout_iter_get_line_extents (layout_iter, NULL, &logical);

        x_pos += logical.x;

        pango_layout_iter_free (layout_iter);

        /*  try to go to the remembered x_pos if it exists *and* we are at
         *  the beginning or at the end of the current line
         */
        if (text_tool->x_pos != -1 && (x_pos <= logical.x ||
                                       x_pos >= logical.x + logical.width))
          x_pos = text_tool->x_pos;

        line += count;

        if (line < 0)
          {
            cursor = start;
            break;
          }
        else if (line >= pango_layout_get_line_count (layout))
          {
            cursor = end;
            break;
          }

        layout_iter = pango_layout_get_iter (layout);
        for (i = 0; i < line; i++)
          pango_layout_iter_next_line (layout_iter);

        layout_line = pango_layout_iter_get_line_readonly (layout_iter);
        pango_layout_iter_get_line_extents (layout_iter, NULL, &logical);

        pango_layout_iter_free (layout_iter);

        pango_layout_line_x_to_index (layout_line, x_pos - logical.x,
                                      &cursor_index, &trailing);

        string = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

        string[cursor_index] = '\0';

        gtk_text_buffer_get_iter_at_offset (buffer, &cursor,
                                            g_utf8_strlen (string, -1));

        g_free (string);

        while (trailing--)
          gtk_text_iter_forward_char (&cursor);
      }
      break;

    case GTK_MOVEMENT_PAGES: /* well... */
    case GTK_MOVEMENT_BUFFER_ENDS:
      if (count < 0)
        {
          gtk_text_buffer_get_start_iter (buffer, &cursor);
        }
      else if (count > 0)
        {
          gtk_text_buffer_get_end_iter (buffer, &cursor);
        }
      break;

    case GTK_MOVEMENT_PARAGRAPH_ENDS:
      if (count < 0)
        {
          gtk_text_iter_set_line_offset (&cursor, 0);
        }
      else if (count > 0)
        {
          if (! gtk_text_iter_ends_line (&cursor))
            gtk_text_iter_forward_to_line_end (&cursor);
        }
      break;

    case GTK_MOVEMENT_DISPLAY_LINE_ENDS:
      if (count < 0)
        {
          gtk_text_iter_set_line_offset (&cursor, 0);
        }
      else if (count > 0)
        {
          if (! gtk_text_iter_ends_line (&cursor))
            gtk_text_iter_forward_to_line_end (&cursor);
        }
      break;

    default:
      return;
    }

  text_tool->x_pos = x_pos;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  gimp_text_tool_reset_im_context (text_tool);

  gtk_text_buffer_select_range (buffer, &cursor, sel_start);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_insert_at_cursor (GimpTextTool *text_tool,
                                 const gchar  *str)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  gtk_text_buffer_insert_interactive_at_cursor (text_tool->text_buffer,
                                                str, -1, TRUE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static gboolean
is_whitespace (gunichar ch,
               gpointer user_data)
{
  return (ch == ' ' || ch == '\t');
}

static gboolean
is_not_whitespace (gunichar ch,
                   gpointer user_data)
{
  return ! is_whitespace (ch, user_data);
}

static gboolean
find_whitepace_region (const GtkTextIter *center,
                       GtkTextIter       *start,
                       GtkTextIter       *end)
{
  *start = *center;
  *end   = *center;

  if (gtk_text_iter_backward_find_char (start, is_not_whitespace, NULL, NULL))
    gtk_text_iter_forward_char (start); /* we want the first whitespace... */

  if (is_whitespace (gtk_text_iter_get_char (end), NULL))
    gtk_text_iter_forward_find_char (end, is_not_whitespace, NULL, NULL);

  return ! gtk_text_iter_equal (start, end);
}

static void
gimp_text_tool_delete_from_cursor (GimpTextTool  *text_tool,
                                   GtkDeleteType  type,
                                   gint           count)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  GtkTextIter    cursor;
  GtkTextIter    end;

  GIMP_LOG (TEXT_EDITING, "%s count = %d",
            g_enum_get_value (g_type_class_ref (GTK_TYPE_DELETE_TYPE),
                              type)->value_name,
            count);

  gimp_text_tool_reset_im_context (text_tool);

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  end = cursor;

  switch (type)
    {
    case GTK_DELETE_CHARS:
      if (gtk_text_buffer_get_has_selection (buffer))
        {
          gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
          return;
        }
      else
        {
          gtk_text_iter_forward_cursor_positions (&end, count);
        }
      break;

    case GTK_DELETE_WORD_ENDS:
      if (count < 0)
        {
          if (! gtk_text_iter_starts_word (&cursor))
            gtk_text_iter_backward_visible_word_starts (&cursor, 1);
        }
      else if (count > 0)
        {
          if (! gtk_text_iter_ends_word (&end) &&
              ! gtk_text_iter_forward_visible_word_ends (&end, 1))
            gtk_text_iter_forward_to_line_end (&end);
        }
      break;

    case GTK_DELETE_WORDS:
      if (! gtk_text_iter_starts_word (&cursor))
        gtk_text_iter_backward_visible_word_starts (&cursor, 1);

      if (! gtk_text_iter_ends_word (&end) &&
          ! gtk_text_iter_forward_visible_word_ends (&end, 1))
        gtk_text_iter_forward_to_line_end (&end);
      break;

    case GTK_DELETE_DISPLAY_LINES:
      break;

    case GTK_DELETE_DISPLAY_LINE_ENDS:
      break;

    case GTK_DELETE_PARAGRAPH_ENDS:
      if (count < 0)
        {
          gtk_text_iter_set_line_offset (&cursor, 0);
        }
      else if (count > 0)
        {
          if (! gtk_text_iter_ends_line (&end))
            gtk_text_iter_forward_to_line_end (&end);
          else
            gtk_text_iter_forward_cursor_positions (&end, 1);
        }
      break;

    case GTK_DELETE_PARAGRAPHS:
      break;

    case GTK_DELETE_WHITESPACE:
      find_whitepace_region (&cursor, &cursor, &end);
      break;
    }

  if (! gtk_text_iter_equal (&cursor, &end))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

      gtk_text_buffer_delete_interactive (buffer, &cursor, &end, TRUE);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
    }
}

static void
gimp_text_tool_backspace (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;

  gimp_text_tool_reset_im_context (text_tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
    }
  else
    {
      GtkTextIter cursor;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      gtk_text_buffer_backspace (buffer, &cursor, TRUE, TRUE);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_toggle_overwrite (GimpTextTool *text_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  text_tool->overwrite_mode = ! text_tool->overwrite_mode;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_select_all (GimpTextTool *text_tool,
                           gboolean      select)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  if (select)
    {
      GtkTextIter start, end;

      gtk_text_buffer_get_bounds (buffer, &start, &end);
      gtk_text_buffer_select_range (buffer, &start, &end);
    }
  else
    {
      GtkTextIter cursor;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
					gtk_text_buffer_get_insert (buffer));
      gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &cursor);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_options_notify (GimpTextOptions *options,
                               GParamSpec      *pspec,
                               GimpTextTool    *text_tool)
{
  const gchar *param_name = g_param_spec_get_name (pspec);

  if (! strcmp (param_name, "use-editor"))
    {
      if (options->use_editor)
        {
          if (text_tool->text)
            gimp_text_tool_editor_dialog (text_tool);
        }
      else
        {
          if (text_tool->editor_dialog)
            gtk_widget_destroy (text_tool->editor_dialog);
        }
    }
}

static void
gimp_text_tool_editor_dialog (GimpTextTool *text_tool)
{
  GimpTool          *tool    = GIMP_TOOL (text_tool);
  GimpTextOptions   *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpDialogFactory *dialog_factory;
  GtkWindow         *parent  = NULL;

  if (text_tool->editor_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor_dialog));
      return;
    }

  dialog_factory = gimp_dialog_factory_from_name ("toplevel");

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (shell)));
    }

  text_tool->editor_dialog =
    gimp_text_options_editor_new (parent, options,
                                  gimp_dialog_factory_get_menu_factory (dialog_factory),
                                  _("GIMP Text Editor"),
                                  text_tool->text_buffer);

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor_dialog),
                             (gpointer) &text_tool->editor_dialog);

  gimp_dialog_factory_add_foreign (dialog_factory,
                                   "gimp-text-tool-dialog",
                                   text_tool->editor_dialog);

  gtk_widget_show (text_tool->editor_dialog);
}

static void
gimp_text_tool_enter_text (GimpTextTool *text_tool,
                           const gchar  *str)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  gboolean       had_selection;

  had_selection = gtk_text_buffer_get_has_selection (buffer);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

  gimp_text_tool_delete_selection (text_tool);

  if (! had_selection && text_tool->overwrite_mode && strcmp (str, "\n"))
    {
      GtkTextIter cursor;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      if (! gtk_text_iter_ends_line (&cursor))
        gimp_text_tool_delete_from_cursor (text_tool, GTK_DELETE_CHARS, 1);
    }

  gtk_text_buffer_insert_at_cursor (buffer, str, -1);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_commit_cb (GtkIMContext *context,
                          const gchar  *str,
                          GimpTextTool *text_tool)
{
  gimp_text_tool_enter_text (text_tool, str);
}

static void
gimp_text_tool_preedit_start (GtkIMContext *context,
                              GimpTextTool *text_tool)
{
  GimpTool         *tool  = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GtkStyle         *style = gtk_widget_get_style (shell->canvas);
  GtkWidget        *frame;
  GtkWidget        *ebox;
  PangoRectangle    cursor_rect = { 0, };
  gint              unused1, unused2;
  gint              off_x, off_y;

  if (text_tool->text)
    gimp_text_tool_editor_get_cursor_rect (text_tool, &cursor_rect,
                                           &unused1, &unused2);

  g_object_get (text_tool, "x1", &off_x, "y1", &off_y, NULL);

  text_tool->preedit_overlay = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (text_tool->preedit_overlay),
                             GTK_SHADOW_OUT);
  gimp_display_shell_add_overlay (GIMP_DISPLAY_SHELL (shell),
                                  text_tool->preedit_overlay,
                                  cursor_rect.x + off_x,
                                  cursor_rect.y + off_y);
  gtk_widget_show (text_tool->preedit_overlay);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (text_tool->preedit_overlay), frame);
  gtk_widget_show (frame);

  ebox = gtk_event_box_new ();
  gtk_widget_modify_bg (ebox, GTK_STATE_NORMAL,
                        &style->base[GTK_STATE_NORMAL]);
  gtk_container_add (GTK_CONTAINER (frame), ebox);
  gtk_widget_show (ebox);

  text_tool->preedit_label = gtk_label_new (NULL);
  gtk_widget_modify_bg (text_tool->preedit_label, GTK_STATE_NORMAL,
                        &style->text[GTK_STATE_NORMAL]);
  gtk_misc_set_padding (GTK_MISC (text_tool->preedit_label), 2, 2);
  gtk_container_add (GTK_CONTAINER (ebox), text_tool->preedit_label);
  gtk_widget_show (text_tool->preedit_label);
}

static void
gimp_text_tool_preedit_end (GtkIMContext *context,
                            GimpTextTool *text_tool)
{
  if (text_tool->preedit_overlay)
    {
      gtk_widget_destroy (text_tool->preedit_overlay);
      text_tool->preedit_overlay = NULL;
      text_tool->preedit_label   = NULL;
    }
}

static void
gimp_text_tool_preedit_changed (GtkIMContext *context,
                                GimpTextTool *text_tool)
{
  if (text_tool->preedit_string)
    g_free (text_tool->preedit_string);

  gtk_im_context_get_preedit_string (context,
                                     &text_tool->preedit_string, NULL,
                                     &text_tool->preedit_cursor);

  if (text_tool->preedit_label)
    gtk_label_set_text (GTK_LABEL (text_tool->preedit_label),
                        text_tool->preedit_string);
}
