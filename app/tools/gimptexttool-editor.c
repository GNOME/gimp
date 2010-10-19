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

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"

#include "text/gimptext.h"
#include "text/gimptextlayout.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimpoverlaybox.h"
#include "widgets/gimpoverlayframe.h"
#include "widgets/gimptextbuffer.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimptextproxy.h"
#include "widgets/gimptextstyleeditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimprectangletool.h"
#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptexttool-editor.h"

#include "gimp-log.h"
#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_text_tool_ensure_proxy       (GimpTextTool    *text_tool);
static void     gimp_text_tool_move_cursor        (GimpTextTool    *text_tool,
                                                   GtkMovementStep  step,
                                                   gint             count,
                                                   gboolean         extend_selection);
static void     gimp_text_tool_insert_at_cursor   (GimpTextTool    *text_tool,
                                                   const gchar     *str);
static void     gimp_text_tool_delete_from_cursor (GimpTextTool    *text_tool,
                                                   GtkDeleteType    type,
                                                   gint             count);
static void     gimp_text_tool_backspace          (GimpTextTool    *text_tool);
static void     gimp_text_tool_toggle_overwrite   (GimpTextTool    *text_tool);
static void     gimp_text_tool_select_all         (GimpTextTool    *text_tool,
                                                   gboolean         select);
static void     gimp_text_tool_change_size        (GimpTextTool    *text_tool,
                                                   gdouble          amount);
static void     gimp_text_tool_change_baseline    (GimpTextTool    *text_tool,
                                                   gdouble          amount);
static void     gimp_text_tool_change_kerning     (GimpTextTool    *text_tool,
                                                   gdouble          amount);

static void     gimp_text_tool_options_notify     (GimpTextOptions *options,
                                                   GParamSpec      *pspec,
                                                   GimpTextTool    *text_tool);
static void     gimp_text_tool_editor_dialog      (GimpTextTool    *text_tool);
static void     gimp_text_tool_editor_destroy     (GtkWidget       *dialog,
                                                   GimpTextTool    *text_tool);
static void     gimp_text_tool_enter_text         (GimpTextTool    *text_tool,
                                                   const gchar     *str);
static void     gimp_text_tool_xy_to_iter         (GimpTextTool    *text_tool,
                                                   gdouble          x,
                                                   gdouble          y,
                                                   GtkTextIter     *iter);

static void     gimp_text_tool_im_preedit_start   (GtkIMContext    *context,
                                                   GimpTextTool    *text_tool);
static void     gimp_text_tool_im_preedit_end     (GtkIMContext    *context,
                                                   GimpTextTool    *text_tool);
static void     gimp_text_tool_im_preedit_changed (GtkIMContext    *context,
                                                   GimpTextTool    *text_tool);
static void     gimp_text_tool_im_commit          (GtkIMContext    *context,
                                                   const gchar     *str,
                                                   GimpTextTool    *text_tool);
static gboolean gimp_text_tool_im_retrieve_surrounding
                                                  (GtkIMContext    *context,
                                                   GimpTextTool    *text_tool);
static gboolean gimp_text_tool_im_delete_surrounding
                                                  (GtkIMContext    *context,
                                                   gint             offset,
                                                   gint             n_chars,
                                                   GimpTextTool    *text_tool);

static void     gimp_text_tool_im_delete_preedit  (GimpTextTool    *text_tool);

static void     gimp_text_tool_editor_copy_selection_to_clipboard
                                                  (GimpTextTool    *text_tool);


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

  g_signal_connect (text_tool->im_context, "preedit-start",
                    G_CALLBACK (gimp_text_tool_im_preedit_start),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-end",
                    G_CALLBACK (gimp_text_tool_im_preedit_end),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-changed",
                    G_CALLBACK (gimp_text_tool_im_preedit_changed),
                    text_tool);
  g_signal_connect (text_tool->im_context, "commit",
                    G_CALLBACK (gimp_text_tool_im_commit),
                    text_tool);
  g_signal_connect (text_tool->im_context, "retrieve-surrounding",
                    G_CALLBACK (gimp_text_tool_im_retrieve_surrounding),
                    text_tool);
  g_signal_connect (text_tool->im_context, "delete-surrounding",
                    G_CALLBACK (gimp_text_tool_im_delete_surrounding),
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

  if (options->use_editor)
    gimp_text_tool_editor_dialog (text_tool);

  g_signal_connect (options, "notify::use-editor",
                    G_CALLBACK (gimp_text_tool_options_notify),
                    text_tool);

  if (! text_tool->style_overlay)
    {
      Gimp    *gimp = GIMP_CONTEXT (options)->gimp;
      gdouble  xres = 1.0;
      gdouble  yres = 1.0;

      text_tool->style_overlay = gimp_overlay_frame_new ();
      gtk_container_set_border_width (GTK_CONTAINER (text_tool->style_overlay),
                                      4);
      gimp_display_shell_add_overlay (shell,
                                      text_tool->style_overlay,
                                      0, 0,
                                      GIMP_HANDLE_ANCHOR_CENTER, 0, 0);
      gimp_overlay_box_set_child_opacity (GIMP_OVERLAY_BOX (shell->canvas),
                                          text_tool->style_overlay, 0.7);

      if (text_tool->image)
        gimp_image_get_resolution (text_tool->image, &xres, &yres);

      text_tool->style_editor = gimp_text_style_editor_new (gimp,
                                                            text_tool->proxy,
                                                            text_tool->buffer,
                                                            gimp->fonts,
                                                            xres, yres);
      gtk_container_add (GTK_CONTAINER (text_tool->style_overlay),
                         text_tool->style_editor);
      gtk_widget_show (text_tool->style_editor);
    }

  gimp_text_tool_editor_position (text_tool);
  gtk_widget_show (text_tool->style_overlay);
}

void
gimp_text_tool_editor_position (GimpTextTool *text_tool)
{
  if (text_tool->style_overlay)
    {
      GimpTool         *tool    = GIMP_TOOL (text_tool);
      GimpDisplayShell *shell   = gimp_display_get_shell (tool->display);
      GtkRequisition    requisition;
      gint              x, y;

      gtk_widget_get_preferred_size (text_tool->style_overlay,
                                     &requisition, NULL);

      g_object_get (text_tool,
                    "x1", &x,
                    "y1", &y,
                    NULL);

      gimp_display_shell_move_overlay (shell,
                                       text_tool->style_overlay,
                                       x, y,
                                       GIMP_HANDLE_ANCHOR_SOUTH_WEST, 4, 12);

      if (text_tool->image)
        {
          gdouble xres, yres;

          gimp_image_get_resolution (text_tool->image, &xres, &yres);

          g_object_set (text_tool->style_editor,
                        "resolution-x", xres,
                        "resolution-y", yres,
                        NULL);
        }
    }
}

void
gimp_text_tool_editor_halt (GimpTextTool *text_tool)
{
  GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);

  if (text_tool->style_overlay)
    {
      gtk_widget_destroy (text_tool->style_overlay);
      text_tool->style_overlay = NULL;
      text_tool->style_editor  = NULL;
    }

  g_signal_handlers_disconnect_by_func (options,
                                        gimp_text_tool_options_notify,
                                        text_tool);

  if (text_tool->editor_dialog)
    {
      g_signal_handlers_disconnect_by_func (text_tool->editor_dialog,
                                            gimp_text_tool_editor_destroy,
                                            text_tool);
      gtk_widget_destroy (text_tool->editor_dialog);
    }

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

void
gimp_text_tool_editor_button_press (GimpTextTool        *text_tool,
                                    gdouble              x,
                                    gdouble              y,
                                    GimpButtonPressType  press_type)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    cursor;
  GtkTextIter    selection;

  gimp_text_tool_xy_to_iter (text_tool, x, y, &cursor);

  selection = cursor;

  text_tool->select_start_iter = cursor;
  text_tool->select_words      = FALSE;
  text_tool->select_lines      = FALSE;

  switch (press_type)
    {
      GtkTextIter start, end;

    case GIMP_BUTTON_PRESS_NORMAL:
      if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end) ||
          gtk_text_iter_compare (&start, &cursor))
        gtk_text_buffer_place_cursor (buffer, &cursor);
      break;

    case GIMP_BUTTON_PRESS_DOUBLE:
      text_tool->select_words = TRUE;

      if (! gtk_text_iter_starts_word (&cursor))
        gtk_text_iter_backward_visible_word_starts (&cursor, 1);

      if (! gtk_text_iter_ends_word (&selection) &&
          ! gtk_text_iter_forward_visible_word_ends (&selection, 1))
        gtk_text_iter_forward_to_line_end (&selection);

      gtk_text_buffer_select_range (buffer, &cursor, &selection);
      break;

    case GIMP_BUTTON_PRESS_TRIPLE:
      text_tool->select_lines = TRUE;

      gtk_text_iter_set_line_offset (&cursor, 0);
      gtk_text_iter_forward_to_line_end (&selection);

      gtk_text_buffer_select_range (buffer, &cursor, &selection);
      break;
    }
}

void
gimp_text_tool_editor_button_release (GimpTextTool *text_tool)
{
  gimp_text_tool_editor_copy_selection_to_clipboard (text_tool);
}

void
gimp_text_tool_editor_motion (GimpTextTool *text_tool,
                              gdouble       x,
                              gdouble       y)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    old_cursor;
  GtkTextIter    old_selection;
  GtkTextIter    cursor;
  GtkTextIter    selection;

  gtk_text_buffer_get_iter_at_mark (buffer, &old_cursor,
                                    gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_get_iter_at_mark (buffer, &old_selection,
                                    gtk_text_buffer_get_selection_bound (buffer));

  gimp_text_tool_xy_to_iter (text_tool, x, y, &cursor);
  selection = text_tool->select_start_iter;

  if (text_tool->select_words ||
      text_tool->select_lines)
    {
      GtkTextIter start;
      GtkTextIter end;

      if (gtk_text_iter_compare (&cursor, &selection) < 0)
        {
          start = cursor;
          end   = selection;
        }
      else
        {
          start = selection;
          end   = cursor;
        }

      if (text_tool->select_words)
        {
          if (! gtk_text_iter_starts_word (&start))
            gtk_text_iter_backward_visible_word_starts (&start, 1);

          if (! gtk_text_iter_ends_word (&end) &&
              ! gtk_text_iter_forward_visible_word_ends (&end, 1))
            gtk_text_iter_forward_to_line_end (&end);
        }
      else if (text_tool->select_lines)
        {
          gtk_text_iter_set_line_offset (&start, 0);
          gtk_text_iter_forward_to_line_end (&end);
        }

      if (gtk_text_iter_compare (&cursor, &selection) < 0)
        {
          cursor    = start;
          selection = end;
        }
      else
        {
          selection = start;
          cursor    = end;
        }
    }

  if (! gtk_text_iter_equal (&cursor,    &old_cursor) ||
      ! gtk_text_iter_equal (&selection, &old_selection))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

      gtk_text_buffer_select_range (buffer, &cursor, &selection);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
    }
}

gboolean
gimp_text_tool_editor_key_press (GimpTextTool *text_tool,
                                 GdkEventKey  *kevent)
{
  GimpTool         *tool   = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell  = gimp_display_get_shell (tool->display);
  GtkTextBuffer    *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter       cursor;
  GtkTextIter       selection;
  gboolean          retval = TRUE;

  if (! gtk_widget_has_focus (shell->canvas))
    {
      /*  The focus is in the floating style editor, and the event
       *  was not handled there, focus the canvas.
       */
      switch (kevent->keyval)
        {
        case GDK_KEY_Tab:
        case GDK_KEY_KP_Tab:
        case GDK_KEY_ISO_Left_Tab:
        case GDK_KEY_Escape:
          gtk_widget_grab_focus (shell->canvas);
          return TRUE;

        default:
          break;
        }
    }

  if (gtk_im_context_filter_keypress (text_tool->im_context, kevent))
    {
      text_tool->needs_im_reset = TRUE;
      text_tool->x_pos          = -1;

      return TRUE;
    }

  gimp_text_tool_ensure_proxy (text_tool);

  if (gtk_bindings_activate_event (G_OBJECT (text_tool->proxy_text_view),
                                   kevent))
    {
      GIMP_LOG (TEXT_EDITING, "binding handled event");

      return TRUE;
    }

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_get_iter_at_mark (buffer, &selection,
                                    gtk_text_buffer_get_selection_bound (buffer));

  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_enter_text (text_tool, "\n");
      break;

    case GDK_KEY_Tab:
    case GDK_KEY_KP_Tab:
    case GDK_KEY_ISO_Left_Tab:
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_enter_text (text_tool, "\t");
      break;

    case GDK_KEY_Escape:
      gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (text_tool));
      gimp_tool_control (GIMP_TOOL (text_tool), GIMP_TOOL_ACTION_HALT,
                         GIMP_TOOL (text_tool)->display);
      break;

    default:
      retval = FALSE;
    }

  text_tool->x_pos = -1;

  return retval;
}

gboolean
gimp_text_tool_editor_key_release (GimpTextTool *text_tool,
                                   GdkEventKey  *kevent)
{
  if (gtk_im_context_filter_keypress (text_tool->im_context, kevent))
    {
      text_tool->needs_im_reset = TRUE;

      return TRUE;
    }

  gimp_text_tool_ensure_proxy (text_tool);

  if (gtk_bindings_activate_event (G_OBJECT (text_tool->proxy_text_view),
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

void
gimp_text_tool_abort_im_context (GimpTextTool *text_tool)
{
  GimpTool         *tool  = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

  text_tool->needs_im_reset = TRUE;
  gimp_text_tool_reset_im_context (text_tool);

  /* Making sure preedit text is removed. */
  gimp_text_tool_im_delete_preedit (text_tool);

  /* the following lines seem to be the only way of really getting
   * rid of any ongoing preedit state, please somebody tell me
   * a clean way... mitch
   */

  gtk_im_context_focus_out (text_tool->im_context);
  gtk_im_context_set_client_window (text_tool->im_context, NULL);

  g_object_unref (text_tool->im_context);
  text_tool->im_context = gtk_im_multicontext_new ();
  gtk_im_context_set_client_window (text_tool->im_context,
                                    gtk_widget_get_window (shell->canvas));
  gtk_im_context_focus_in (text_tool->im_context);
  g_signal_connect (text_tool->im_context, "preedit-start",
                    G_CALLBACK (gimp_text_tool_im_preedit_start),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-end",
                    G_CALLBACK (gimp_text_tool_im_preedit_end),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-changed",
                    G_CALLBACK (gimp_text_tool_im_preedit_changed),
                    text_tool);
  g_signal_connect (text_tool->im_context, "commit",
                    G_CALLBACK (gimp_text_tool_im_commit),
                    text_tool);
  g_signal_connect (text_tool->im_context, "retrieve-surrounding",
                    G_CALLBACK (gimp_text_tool_im_retrieve_surrounding),
                    text_tool);
  g_signal_connect (text_tool->im_context, "delete-surrounding",
                    G_CALLBACK (gimp_text_tool_im_delete_surrounding),
                    text_tool);
}

void
gimp_text_tool_editor_get_cursor_rect (GimpTextTool   *text_tool,
                                       gboolean        overwrite,
                                       PangoRectangle *cursor_rect)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  PangoLayout   *layout;
  gint           offset_x;
  gint           offset_y;
  GtkTextIter    cursor;
  gint           cursor_index;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));
  g_return_if_fail (cursor_rect != NULL);

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  cursor_index = gimp_text_buffer_get_iter_index (text_tool->buffer, &cursor,
                                                  TRUE);

  gimp_text_tool_ensure_layout (text_tool);

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);

  gimp_text_layout_get_offsets (text_tool->layout, &offset_x, &offset_y);

  if (overwrite)
    pango_layout_index_to_pos (layout, cursor_index, cursor_rect);
  else
    pango_layout_get_cursor_pos (layout, cursor_index, cursor_rect, NULL);

  gimp_text_layout_transform_rect (text_tool->layout, cursor_rect);

  cursor_rect->x      = PANGO_PIXELS (cursor_rect->x) + offset_x;
  cursor_rect->y      = PANGO_PIXELS (cursor_rect->y) + offset_y;
  cursor_rect->width  = PANGO_PIXELS (cursor_rect->width);
  cursor_rect->height = PANGO_PIXELS (cursor_rect->height);
}

void
gimp_text_tool_editor_update_im_cursor (GimpTextTool *text_tool)
{
  GimpDisplayShell *shell;
  PangoRectangle    rect = { 0, };
  gint              off_x, off_y;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  shell = gimp_display_get_shell (GIMP_TOOL (text_tool)->display);

  if (text_tool->text)
    gimp_text_tool_editor_get_cursor_rect (text_tool,
                                           text_tool->overwrite_mode,
                                           &rect);

  g_object_get (text_tool, "x1", &off_x, "y1", &off_y, NULL);
  rect.x += off_x;
  rect.y += off_y;

  gimp_display_shell_transform_xy (shell, rect.x, rect.y, &rect.x, &rect.y);

  gtk_im_context_set_cursor_location (text_tool->im_context,
                                      (GdkRectangle *) &rect);
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
      g_signal_connect_swapped (text_tool->proxy_text_view, "change-size",
                                G_CALLBACK (gimp_text_tool_change_size),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "change-baseline",
                                G_CALLBACK (gimp_text_tool_change_baseline),
                                text_tool);
      g_signal_connect_swapped (text_tool->proxy_text_view, "change-kerning",
                                G_CALLBACK (gimp_text_tool_change_kerning),
                                text_tool);
    }
}

static void
gimp_text_tool_move_cursor (GimpTextTool    *text_tool,
                            GtkMovementStep  step,
                            gint             count,
                            gboolean         extend_selection)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    cursor;
  GtkTextIter    selection;
  GtkTextIter   *sel_start;
  gboolean       cancel_selection = FALSE;
  gint           x_pos  = -1;

  if (text_tool->pending)
    {
      /* If there are any pending text commits, there would be
       * inconsistencies between the text_tool->buffer and layout.
       * This could result in crashes. See bug 751333.
       * Therefore we apply them first.
       */
      gimp_text_tool_apply (text_tool, TRUE);
    }
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
          PangoLayout *layout;
          const gchar *text;

          if (! gimp_text_tool_ensure_layout (text_tool))
            break;

          layout = gimp_text_layout_get_pango_layout (text_tool->layout);
          text = pango_layout_get_text (layout);

          while (count != 0)
            {
              const gunichar word_joiner = 8288; /*g_utf8_get_char(WORD_JOINER);*/
              gint index;
              gint trailing = 0;
              gint new_index;

              index = gimp_text_buffer_get_iter_index (text_tool->buffer,
                                                       &cursor, TRUE);

              if (count > 0)
                {
                  if (g_utf8_get_char (text + index) == word_joiner)
                    pango_layout_move_cursor_visually (layout, TRUE,
                                                       index, 0, 1,
                                                       &new_index, &trailing);
                  else
                    new_index = index;

                  pango_layout_move_cursor_visually (layout, TRUE,
                                                     new_index, trailing, 1,
                                                     &new_index, &trailing);
                  count--;
                }
              else
                {
                  pango_layout_move_cursor_visually (layout, TRUE,
                                                     index, 0, -1,
                                                     &new_index, &trailing);

                  if (new_index != -1 && new_index != G_MAXINT &&
                      g_utf8_get_char (text + new_index) == word_joiner)
                    {
                      pango_layout_move_cursor_visually (layout, TRUE,
                                                         new_index, trailing, -1,
                                                         &new_index, &trailing);
                    }

                  count++;
                }

              if (new_index != G_MAXINT && new_index != -1)
                index = new_index;
              else
                break;

              gimp_text_buffer_get_iter_at_index (text_tool->buffer,
                                                  &cursor, index, TRUE);
              gtk_text_iter_forward_chars (&cursor, trailing);
            }
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
        gint             cursor_index;
        PangoLayout     *layout;
        PangoLayoutLine *layout_line;
        PangoLayoutIter *layout_iter;
        PangoRectangle   logical;
        gint             line;
        gint             trailing;
        gint             i;

        gtk_text_buffer_get_bounds (buffer, &start, &end);

        cursor_index = gimp_text_buffer_get_iter_index (text_tool->buffer,
                                                        &cursor, TRUE);

        if (! gimp_text_tool_ensure_layout (text_tool))
          break;

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

        gimp_text_buffer_get_iter_at_index (text_tool->buffer, &cursor,
                                            cursor_index, TRUE);

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
  gimp_text_tool_editor_copy_selection_to_clipboard (text_tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_insert_at_cursor (GimpTextTool *text_tool,
                                 const gchar  *str)
{
  gimp_text_buffer_insert (text_tool->buffer, str);
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
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
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
      gtk_text_buffer_delete_interactive (buffer, &cursor, &end, TRUE);
    }
}

static void
gimp_text_tool_backspace (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  gimp_text_tool_reset_im_context (text_tool);

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
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

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
gimp_text_tool_change_size (GimpTextTool *text_tool,
                            gdouble       amount)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start;
  GtkTextIter    end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      return;
    }

  gtk_text_iter_order (&start, &end);
  gimp_text_buffer_change_size (text_tool->buffer, &start, &end,
                                amount * PANGO_SCALE);
}

static void
gimp_text_tool_change_baseline (GimpTextTool *text_tool,
                                gdouble       amount)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start;
  GtkTextIter    end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      gtk_text_buffer_get_end_iter (buffer, &end);
    }

  gtk_text_iter_order (&start, &end);
  gimp_text_buffer_change_baseline (text_tool->buffer, &start, &end,
                                    amount * PANGO_SCALE);
}

static void
gimp_text_tool_change_kerning (GimpTextTool *text_tool,
                               gdouble       amount)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start;
  GtkTextIter    end;

  if (! gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
    {
      gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                        gtk_text_buffer_get_insert (buffer));
      end = start;
      gtk_text_iter_forward_char (&end);
    }

  gtk_text_iter_order (&start, &end);
  gimp_text_buffer_change_kerning (text_tool->buffer, &start, &end,
                                   amount * PANGO_SCALE);
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
  GimpDisplayShell  *shell   = gimp_display_get_shell (tool->display);
  GimpImageWindow   *image_window;
  GimpDialogFactory *dialog_factory;
  GtkWindow         *parent  = NULL;
  gdouble            xres    = 1.0;
  gdouble            yres    = 1.0;

  if (text_tool->editor_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor_dialog));
      return;
    }

  image_window   = gimp_display_shell_get_window (shell);
  dialog_factory = gimp_dock_container_get_dialog_factory (GIMP_DOCK_CONTAINER (image_window));

  if (text_tool->image)
    gimp_image_get_resolution (text_tool->image, &xres, &yres);

  text_tool->editor_dialog =
    gimp_text_options_editor_new (parent, tool->tool_info->gimp, options,
                                  gimp_dialog_factory_get_menu_factory (dialog_factory),
                                  _("GIMP Text Editor"),
                                  text_tool->proxy, text_tool->buffer,
                                  xres, yres);

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor_dialog),
                             (gpointer) &text_tool->editor_dialog);

  gimp_dialog_factory_add_foreign (dialog_factory,
                                   "gimp-text-tool-dialog",
                                   text_tool->editor_dialog,
                                   gtk_widget_get_screen (GTK_WIDGET (image_window)),
                                   gimp_widget_get_monitor (GTK_WIDGET (image_window)));

  g_signal_connect (text_tool->editor_dialog, "destroy",
                    G_CALLBACK (gimp_text_tool_editor_destroy),
                    text_tool);

  gtk_widget_show (text_tool->editor_dialog);
}

static void
gimp_text_tool_editor_destroy (GtkWidget    *dialog,
                               GimpTextTool *text_tool)
{
  GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);

  g_object_set (options,
                "use-editor", FALSE,
                NULL);
}

static void
gimp_text_tool_enter_text (GimpTextTool *text_tool,
                           const gchar  *str)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  gboolean       had_selection;

  had_selection = gtk_text_buffer_get_has_selection (buffer);

  gtk_text_buffer_begin_user_action (buffer);

  gimp_text_tool_delete_selection (text_tool);

  if (! had_selection && text_tool->overwrite_mode && strcmp (str, "\n"))
    {
      GtkTextIter cursor;

      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      if (! gtk_text_iter_ends_line (&cursor))
        gimp_text_tool_delete_from_cursor (text_tool, GTK_DELETE_CHARS, 1);
    }

  gimp_text_buffer_insert (text_tool->buffer, str);

  gtk_text_buffer_end_user_action (buffer);
}

static void
gimp_text_tool_xy_to_iter (GimpTextTool *text_tool,
                           gdouble       x,
                           gdouble       y,
                           GtkTextIter  *iter)
{
  PangoLayout *layout;
  gint         offset_x;
  gint         offset_y;
  gint         index;
  gint         trailing;

  gimp_text_tool_ensure_layout (text_tool);

  gimp_text_layout_untransform_point (text_tool->layout, &x, &y);

  gimp_text_layout_get_offsets (text_tool->layout, &offset_x, &offset_y);
  x -= offset_x;
  y -= offset_y;

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);

  pango_layout_xy_to_index (layout,
                            x * PANGO_SCALE,
                            y * PANGO_SCALE,
                            &index, &trailing);

  gimp_text_buffer_get_iter_at_index (text_tool->buffer, iter, index, TRUE);

  if (trailing)
    gtk_text_iter_forward_char (iter);
}

static void
gimp_text_tool_im_preedit_start (GtkIMContext *context,
                                 GimpTextTool *text_tool)
{
  GIMP_LOG (TEXT_EDITING, "preedit start");

  text_tool->preedit_active = TRUE;
}

static void
gimp_text_tool_im_preedit_end (GtkIMContext *context,
                               GimpTextTool *text_tool)
{
  gimp_text_tool_delete_selection (text_tool);

  text_tool->preedit_active = FALSE;

  GIMP_LOG (TEXT_EDITING, "preedit end");
}

static void
gimp_text_tool_im_preedit_changed (GtkIMContext *context,
                                   GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  PangoAttrList *attrs;

  GIMP_LOG (TEXT_EDITING, "preedit changed");

  gtk_text_buffer_begin_user_action (buffer);

  gimp_text_tool_im_delete_preedit (text_tool);

  gimp_text_tool_delete_selection (text_tool);

  gtk_im_context_get_preedit_string (context,
                                     &text_tool->preedit_string, &attrs,
                                     &text_tool->preedit_cursor);

  if (text_tool->preedit_string && *text_tool->preedit_string)
    {
      PangoAttrIterator *attr_iter;
      GtkTextIter        iter;
      gint               i;

      /* Save the preedit start position. */
      gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                        gtk_text_buffer_get_insert (buffer));
      text_tool->preedit_start = gtk_text_buffer_create_mark (buffer,
                                                              "preedit-start",
                                                              &iter, TRUE);

      /* Loop through chunks of preedit text with different attributes. */
      attr_iter = pango_attr_list_get_iterator (attrs);
      do
        {
          gint attr_start;
          gint attr_end;

          pango_attr_iterator_range (attr_iter, &attr_start, &attr_end);
          if (attr_start < strlen (text_tool->preedit_string))
            {
              GSList      *attrs_pos;
              GtkTextMark *start_mark;
              GtkTextIter  start;
              GtkTextIter  end;

              gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                                gtk_text_buffer_get_insert (buffer));
              start_mark = gtk_text_buffer_create_mark (buffer,
                                                        NULL,
                                                        &start, TRUE);

              gtk_text_buffer_begin_user_action (buffer);

              /* Insert the preedit chunk at current cursor position. */
              gtk_text_buffer_insert_at_cursor (GTK_TEXT_BUFFER (text_tool->buffer),
                                                text_tool->preedit_string + attr_start,
                                                attr_end - attr_start);
              gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                                start_mark);
              gtk_text_buffer_delete_mark (buffer, start_mark);
              gtk_text_buffer_get_iter_at_mark (buffer, &end,
                                                gtk_text_buffer_get_insert (buffer));

              /* Apply text attributes to preedit text. */
              attrs_pos = pango_attr_iterator_get_attrs (attr_iter);
              while (attrs_pos)
                {
                  PangoAttribute *attr = attrs_pos->data;

                  if (attr)
                    {
                      switch (attr->klass->type)
                        {
                        case PANGO_ATTR_UNDERLINE:
                          gtk_text_buffer_apply_tag (buffer,
                                                     text_tool->buffer->preedit_underline_tag,
                                                     &start, &end);
                          break;
                        case PANGO_ATTR_BACKGROUND:
                        case PANGO_ATTR_FOREGROUND:
                            {
                              PangoAttrColor *color_attr = (PangoAttrColor *) attr;
                              GimpRGB         color;

                              color.r = (gdouble) color_attr->color.red / 65535.0;
                              color.g = (gdouble) color_attr->color.green / 65535.0;
                              color.b = (gdouble) color_attr->color.blue / 65535.0;

                              if (attr->klass->type == PANGO_ATTR_BACKGROUND)
                                {
                                  gimp_text_buffer_set_preedit_bg_color (text_tool->buffer,
                                                                         &start, &end,
                                                                         &color);
                                }
                              else
                                {
                                  gimp_text_buffer_set_preedit_color (text_tool->buffer,
                                                                      &start, &end,
                                                                      &color);
                                }
                            }
                          break;
                        default:
                          /* Unsupported tags. */
                          break;
                        }
                    }

                  attrs_pos = attrs_pos->next;
                }

              gtk_text_buffer_end_user_action (buffer);
            }
        }
      while (pango_attr_iterator_next (attr_iter));

      /* Save the preedit end position. */
      gtk_text_buffer_get_iter_at_mark (buffer, &iter,
                                        gtk_text_buffer_get_insert (buffer));
      text_tool->preedit_end = gtk_text_buffer_create_mark (buffer,
                                                            "preedit-end",
                                                            &iter, FALSE);

      /* Move the cursor to the expected location. */
      gtk_text_buffer_get_iter_at_mark (buffer, &iter, text_tool->preedit_start);
      for (i = 0; i < text_tool->preedit_cursor; i++)
        gtk_text_iter_forward_char (&iter);
      gtk_text_buffer_place_cursor (buffer, &iter);

      pango_attr_iterator_destroy (attr_iter);
    }

  pango_attr_list_unref (attrs);

  gtk_text_buffer_end_user_action (buffer);
}

static void
gimp_text_tool_im_commit (GtkIMContext *context,
                          const gchar  *str,
                          GimpTextTool *text_tool)
{
  gboolean preedit_active = text_tool->preedit_active;

  gimp_text_tool_im_delete_preedit (text_tool);

  /* Some IMEs would emit a preedit-commit before preedit-end.
   * To keep undo consistency, we fake and end then immediate restart of
   * preediting.
   */
  if (preedit_active)
    gimp_text_tool_im_preedit_end (context, text_tool);

  gimp_text_tool_enter_text (text_tool, str);

  if (preedit_active)
    gimp_text_tool_im_preedit_start (context, text_tool);
}

static gboolean
gimp_text_tool_im_retrieve_surrounding (GtkIMContext *context,
                                        GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start;
  GtkTextIter    end;
  gint           pos;
  gchar         *text;

  gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                    gtk_text_buffer_get_insert (buffer));
  end = start;

  pos = gtk_text_iter_get_line_index (&start);
  gtk_text_iter_set_line_offset (&start, 0);
  gtk_text_iter_forward_to_line_end (&end);

  text = gtk_text_iter_get_slice (&start, &end);
  gtk_im_context_set_surrounding (context, text, -1, pos);
  g_free (text);

  return TRUE;
}

static gboolean
gimp_text_tool_im_delete_surrounding (GtkIMContext *context,
                                      gint          offset,
                                      gint          n_chars,
                                      GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
  GtkTextIter    start;
  GtkTextIter    end;

  gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                    gtk_text_buffer_get_insert (buffer));
  end = start;

  gtk_text_iter_forward_chars (&start, offset);
  gtk_text_iter_forward_chars (&end, offset + n_chars);

  gtk_text_buffer_delete_interactive (buffer, &start, &end, TRUE);

  return TRUE;
}

static void
gimp_text_tool_im_delete_preedit (GimpTextTool *text_tool)
{
  if (text_tool->preedit_string)
    {
      if (*text_tool->preedit_string)
        {
          GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);
          GtkTextIter    start;
          GtkTextIter    end;

          gtk_text_buffer_get_iter_at_mark (buffer, &start,
                                            text_tool->preedit_start);
          gtk_text_buffer_get_iter_at_mark (buffer, &end,
                                            text_tool->preedit_end);

          gtk_text_buffer_delete_interactive (buffer, &start, &end, TRUE);

          gtk_text_buffer_delete_mark (buffer, text_tool->preedit_start);
          gtk_text_buffer_delete_mark (buffer, text_tool->preedit_end);
          text_tool->preedit_start = NULL;
          text_tool->preedit_end = NULL;
        }

      g_free (text_tool->preedit_string);
      text_tool->preedit_string = NULL;
    }
}

static void
gimp_text_tool_editor_copy_selection_to_clipboard (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  if (! text_tool->editor_dialog &&
      gtk_text_buffer_get_has_selection (buffer))
    {
      GimpTool         *tool  = GIMP_TOOL (text_tool);
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
      GtkClipboard     *clipboard;

      clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                            GDK_SELECTION_PRIMARY);

      gtk_text_buffer_copy_clipboard (buffer, clipboard);
    }
}
