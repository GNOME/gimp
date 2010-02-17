/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextTool
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"

#include "text/gimptext.h"
#include "text/gimptext-vectors.h"
#include "text/gimptextlayer.h"
#include "text/gimptextlayout.h"
#include "text/gimptextundo.h"

#include "vectors/gimpvectors-warp.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimptexteditor.h"
#include "widgets/gimptextproxy.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "gimpeditselectiontool.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TEXT_UNDO_TIMEOUT 3


/*  local function prototypes  */

static void gimp_text_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject * gimp_text_tool_constructor     (GType              type,
                                                 guint              n_params,
                                                 GObjectConstructParam *params);
static void      gimp_text_tool_dispose         (GObject           *object);
static void      gimp_text_tool_finalize        (GObject           *object);

static void      gimp_text_tool_control         (GimpTool          *tool,
                                                 GimpToolAction     action,
                                                 GimpDisplay       *display);
static void      gimp_text_tool_button_press    (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 GimpButtonPressType  press_type,
                                                 GimpDisplay       *display);
static void      gimp_text_tool_button_release  (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 GimpButtonReleaseType release_type,
                                                 GimpDisplay       *display);
static void      gimp_text_tool_motion          (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 GimpDisplay       *display);
static gboolean  gimp_text_tool_key_press       (GimpTool          *tool,
                                                 GdkEventKey       *kevent,
                                                 GimpDisplay       *display);
static void      gimp_text_tool_oper_update     (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 GdkModifierType    state,
                                                 gboolean           proximity,
                                                 GimpDisplay       *display);
static void      gimp_text_tool_cursor_update   (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 GdkModifierType    state,
                                                 GimpDisplay       *display);
static GimpUIManager * gimp_text_tool_get_popup (GimpTool          *tool,
                                                 const GimpCoords  *coords,
                                                 GdkModifierType    state,
                                                 GimpDisplay       *display,
                                                 const gchar      **ui_path);

static void      gimp_text_tool_draw            (GimpDrawTool      *draw_tool);
static void      gimp_text_tool_draw_preedit    (GimpDrawTool      *draw_tool,
                                                 gint               logical_off_x,
                                                 gint               logical_off_y);
static void      gimp_text_tool_draw_selection  (GimpDrawTool      *draw_tool,
                                                 gint               logical_off_x,
                                                 gint               logical_off_y);

static gboolean  gimp_text_tool_rectangle_change_complete
                                                (GimpRectangleTool *rect_tool);

static void      gimp_text_tool_halt            (GimpTextTool      *text_tool);

static void      gimp_text_tool_ensure_proxy    (GimpTextTool      *text_tool);
static void      gimp_text_tool_move_cursor     (GimpTextTool      *text_tool,
                                                 GtkMovementStep    step,
                                                 gint               count,
                                                 gboolean           extend_selection);
static void     gimp_text_tool_insert_at_cursor (GimpTextTool      *text_tool,
                                                 const gchar       *str);
static void   gimp_text_tool_delete_from_cursor (GimpTextTool      *text_tool,
                                                 GtkDeleteType      type,
                                                 gint               count);
static void      gimp_text_tool_backspace       (GimpTextTool      *text_tool);
static void     gimp_text_tool_toggle_overwrite (GimpTextTool      *text_tool);
static void      gimp_text_tool_select_all      (GimpTextTool      *text_tool,
                                                 gboolean           select);

static void      gimp_text_tool_connect         (GimpTextTool      *text_tool,
                                                 GimpTextLayer     *layer,
                                                 GimpText          *text);
static void      gimp_text_tool_layer_notify    (GimpTextLayer     *layer,
                                                 GParamSpec        *pspec,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_proxy_notify    (GimpText          *text,
                                                 GParamSpec        *pspec,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_text_notify     (GimpText          *text,
                                                 GParamSpec        *pspec,
                                                 GimpTextTool      *text_tool);
static gboolean  gimp_text_tool_idle_apply      (GimpTextTool      *text_tool);
static void      gimp_text_tool_apply           (GimpTextTool      *text_tool);

static void      gimp_text_tool_create_layer    (GimpTextTool      *text_tool,
                                                 GimpText          *text);

static void      gimp_text_tool_editor_dialog   (GimpTextTool      *text_tool);
static void      gimp_text_tool_editor          (GimpTextTool      *text_tool);
static gchar   * gimp_text_tool_editor_get_text (GimpTextTool      *text_tool);

static void      gimp_text_tool_layer_changed   (GimpImage         *image,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_set_image       (GimpTextTool      *text_tool,
                                                 GimpImage         *image);
static gboolean  gimp_text_tool_set_drawable    (GimpTextTool      *text_tool,
                                                 GimpDrawable      *drawable,
                                                 gboolean           confirm);

static void      gimp_text_tool_update_layout   (GimpTextTool      *text_tool);
static void      gimp_text_tool_update_proxy    (GimpTextTool      *text_tool);

static void      gimp_text_tool_enter_text      (GimpTextTool      *text_tool,
                                                 const gchar       *str);
static void gimp_text_tool_text_buffer_changed  (GtkTextBuffer     *text_buffer,
                                                 GimpTextTool      *text_tool);
static void gimp_text_tool_text_buffer_mark_set (GtkTextBuffer     *text_buffer,
                                                 GtkTextIter       *location,
                                                 GtkTextMark       *mark,
                                                 GimpTextTool      *text_tool);
static void gimp_text_tool_use_editor_notify    (GimpTextOptions   *options,
                                                 GParamSpec        *pspec,
                                                 GimpTextTool      *text_tool);

static gint gimp_text_tool_xy_to_offset         (GimpTextTool      *text_tool,
                                                 gdouble            x,
                                                 gdouble            y);

/*  IM context utilities and callbacks  */
static void gimp_text_tool_reset_im_context     (GimpTextTool      *text_tool);
static void gimp_text_tool_commit_cb            (GtkIMContext      *context,
                                                 const gchar       *str,
                                                 GimpTextTool      *text_tool);
static void gimp_text_tool_preedit_changed_cb   (GtkIMContext      *context,
                                                 GimpTextTool      *text_tool);


G_DEFINE_TYPE_WITH_CODE (GimpTextTool, gimp_text_tool,
                         GIMP_TYPE_DRAW_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_text_tool_rectangle_tool_iface_init))

#define parent_class gimp_text_tool_parent_class


void
gimp_text_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_TEXT_TOOL,
                GIMP_TYPE_TEXT_OPTIONS,
                gimp_text_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_FONT_MASK       |
                GIMP_CONTEXT_PALETTE_MASK /* for the color popup's palette tab */,
                "gimp-text-tool",
                _("Text"),
                _("Text Tool: Create or edit text layers"),
                N_("Te_xt"), "T",
                NULL, GIMP_HELP_TOOL_TEXT,
                GIMP_STOCK_TOOL_TEXT,
                data);
}

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor    = gimp_text_tool_constructor;
  object_class->dispose        = gimp_text_tool_dispose;
  object_class->finalize       = gimp_text_tool_finalize;
  object_class->set_property   = gimp_rectangle_tool_set_property;
  object_class->get_property   = gimp_rectangle_tool_get_property;

  tool_class->control          = gimp_text_tool_control;
  tool_class->button_press     = gimp_text_tool_button_press;
  tool_class->motion           = gimp_text_tool_motion;
  tool_class->button_release   = gimp_text_tool_button_release;
  tool_class->key_press        = gimp_text_tool_key_press;
  tool_class->oper_update      = gimp_text_tool_oper_update;
  tool_class->cursor_update    = gimp_text_tool_cursor_update;
  tool_class->get_popup        = gimp_text_tool_get_popup;

  draw_tool_class->draw        = gimp_text_tool_draw;

  gimp_rectangle_tool_install_properties (object_class);
}

static void
gimp_text_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute                   = NULL;
  iface->cancel                    = NULL;
  iface->rectangle_change_complete = gimp_text_tool_rectangle_change_complete;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  text_tool->proxy   = NULL;
  text_tool->pending = NULL;
  text_tool->idle_id = 0;

  text_tool->text    = NULL;
  text_tool->layer   = NULL;
  text_tool->image   = NULL;
  text_tool->layout  = NULL;

  text_tool->text_buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (text_tool->text_buffer, "", -1);

  g_signal_connect (text_tool->text_buffer, "changed",
                    G_CALLBACK (gimp_text_tool_text_buffer_changed),
                    text_tool);
  g_signal_connect (text_tool->text_buffer, "mark-set",
                    G_CALLBACK (gimp_text_tool_text_buffer_mark_set),
                    text_tool);

  text_tool->im_context = gtk_im_multicontext_new ();

  text_tool->preedit_string = NULL;

  g_signal_connect (text_tool->im_context, "commit",
                    G_CALLBACK (gimp_text_tool_commit_cb),
                    text_tool);
  g_signal_connect (text_tool->im_context, "preedit-changed",
                    G_CALLBACK (gimp_text_tool_preedit_changed_cb),
                    text_tool);

  gimp_tool_control_set_scroll_lock          (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click   (tool->control, TRUE);
  gimp_tool_control_set_wants_triple_click   (tool->control, TRUE);
  gimp_tool_control_set_wants_all_key_events (tool->control, TRUE);
  gimp_tool_control_set_precision            (tool->control,
                                              GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor          (tool->control,
                                              GIMP_TOOL_CURSOR_TEXT);
  gimp_tool_control_set_action_object_1      (tool->control,
                                              "context/context-font-select-set");

  text_tool->handle_rectangle_change_complete = TRUE;

  text_tool->overwrite_mode = FALSE;

  text_tool->x_pos = -1;
}

static GObject *
gimp_text_tool_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject         *object;
  GimpTextTool    *text_tool;
  GimpTextOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  text_tool = GIMP_TEXT_TOOL (object);
  options   = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);

  text_tool->proxy = g_object_new (GIMP_TYPE_TEXT, NULL);

  gimp_text_options_connect_text (options, text_tool->proxy);

  g_signal_connect_object (text_tool->proxy, "notify",
                           G_CALLBACK (gimp_text_tool_proxy_notify),
                           text_tool, 0);

  g_signal_connect_object (options, "notify::use-editor",
                           G_CALLBACK (gimp_text_tool_use_editor_notify),
                           text_tool, 0);

  g_object_set (options,
                "highlight", FALSE,
                NULL);

  return object;
}

static void
gimp_text_tool_dispose (GObject *object)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (object);

  gimp_text_tool_halt (text_tool);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_text_tool_finalize (GObject *object)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (object);

  if (text_tool->proxy)
    {
      g_object_unref (text_tool->proxy);
      text_tool->proxy = NULL;
    }

  if (text_tool->layout)
    {
      g_object_unref (text_tool->layout);
      text_tool->layout = NULL;
    }

  if (text_tool->text_buffer)
    {
      g_object_unref (text_tool->text_buffer);
      text_tool->text_buffer = NULL;
    }

  if (text_tool->im_context)
    {
      g_object_unref (text_tool->im_context);
      text_tool->im_context = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  gimp_rectangle_tool_control (tool, action, display);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_text_tool_halt (text_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_text_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (tool);
  GimpText          *text      = text_tool->text;
  GtkTextBuffer     *buffer    = text_tool->text_buffer;
  GimpDrawable      *drawable;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      g_signal_handlers_block_by_func (buffer,
                                       gimp_text_tool_text_buffer_mark_set,
                                       text_tool);

      text_tool->selecting = FALSE;

      if (gimp_rectangle_tool_point_in_rectangle (rect_tool,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          text_tool->selecting = TRUE;

          gimp_rectangle_tool_set_function (rect_tool, GIMP_RECTANGLE_TOOL_DEAD);
          gimp_tool_control_activate (tool->control);
        }
      else
        {
          gimp_rectangle_tool_button_press (tool, coords, time, state, display);
        }

      /* bail out now if the rectangle is narrow and the button press is
       * outside the layer
       */
      if (text_tool->layer &&
          gimp_rectangle_tool_get_function (rect_tool) !=
          GIMP_RECTANGLE_TOOL_CREATING)
        {
          GimpItem *item = GIMP_ITEM (text_tool->layer);
          gdouble   x    = coords->x - gimp_item_get_offset_x (item);
          gdouble   y    = coords->y - gimp_item_get_offset_y (item);

          if (x < 0 || x >= gimp_item_get_width  (item) ||
              y < 0 || y >= gimp_item_get_height (item))
            {
              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

              return;
            }
        }
    }

  drawable = gimp_image_get_active_drawable (gimp_display_get_image (display));

  if (GIMP_IS_LAYER (drawable))
    {
      GimpItem *item = GIMP_ITEM (drawable);
      gdouble   x    = coords->x - gimp_item_get_offset_x (item);
      gdouble   y    = coords->y - gimp_item_get_offset_y (item);

      if (x >= 0 && x < gimp_item_get_width (item) &&
          y >= 0 && y < gimp_item_get_height (item))
        {
          /*  did the user click on a text layer?  */
          if (gimp_text_tool_set_drawable (text_tool, drawable, TRUE))
            {
              /*  if we clicked on a text layer while the tool was idle
               *  (didn't show a rectangle), frame the layer and switch
               *  to selecting instead of drawing a new rectangle
               */
              if (gimp_rectangle_tool_get_function (rect_tool) ==
                  GIMP_RECTANGLE_TOOL_CREATING)
                {
                  text_tool->selecting = TRUE;

                  gimp_rectangle_tool_set_function (rect_tool,
                                                    GIMP_RECTANGLE_TOOL_DEAD);
                  gimp_rectangle_tool_frame_item (rect_tool,
                                                  GIMP_ITEM (drawable));
                }

              if (press_type == GIMP_BUTTON_PRESS_NORMAL)
                {
                  /* enable keyboard-handling for the text */
                  if (text_tool->text && text_tool->text != text)
                    {
                      gimp_text_tool_editor (text_tool);
                    }
                }

              if (text_tool->layout && ! text_tool->moving)
                {
                  GtkTextIter cursor;
                  GtkTextIter selection;
                  gint        offset;

                  offset = gimp_text_tool_xy_to_offset (text_tool, x, y);

                  gtk_text_buffer_get_iter_at_offset (buffer, &cursor, offset);

                  selection = cursor;

                  text_tool->select_start_offset = offset;
                  text_tool->select_words        = FALSE;
                  text_tool->select_lines        = FALSE;

                  switch (press_type)
                    {
                    case GIMP_BUTTON_PRESS_NORMAL:
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

              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

              return;
            }
        }
    }

  /*  create a new text layer  */
  text_tool->text_box_fixed = FALSE;

  gimp_text_tool_connect (text_tool, NULL, NULL);
  gimp_text_tool_editor (text_tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

#define MIN_LAYER_WIDTH 20

/*
 * Here is what we want to do:
 * 1) If the user clicked on an existing text layer, and no rectangle
 *    yet exists there, we want to create one with the right shape.
 * 2) If the user has modified the rectangle for an existing text layer,
 *    we want to change its shape accordingly.  We do this by falling
 *    through to code that causes the "rectangle-change-complete" signal
 *    to be emitted.
 * 3) If the rectangle that has been swept out is too small, we want to
 *    use dynamic text.
 * 4) Otherwise, we want to use the new rectangle that the user has
 *    created as our text box.  This again is done by causing
 *    "rectangle-change-complete" to be emitted.
 */
static void
gimp_text_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (tool);
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpText          *text      = text_tool->text;
  gint               x1, y1;
  gint               x2, y2;

  g_object_get (text_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (text_tool->selecting)
    {
      if (gtk_text_buffer_get_has_selection (text_tool->text_buffer))
        {
          GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
          GtkClipboard     *clipboard;

          clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                                GDK_SELECTION_PRIMARY);

          gtk_text_buffer_copy_clipboard (text_tool->text_buffer, clipboard);
        }

      text_tool->selecting = FALSE;
    }

  if (text && text_tool->text == text)
    {
      if (gimp_rectangle_tool_rectangle_is_new (rect_tool))
        {
          /* user has clicked on an existing text layer */

          gimp_tool_control_halt (tool->control);

          text_tool->handle_rectangle_change_complete = FALSE;

          gimp_rectangle_tool_frame_item (rect_tool,
                                          GIMP_ITEM (text_tool->layer));

          text_tool->handle_rectangle_change_complete = TRUE;

          g_signal_handlers_unblock_by_func (text_tool->text_buffer,
                                             gimp_text_tool_text_buffer_mark_set,
                                             text_tool);

          return;
        }
    }
  else if (y2 - y1 < MIN_LAYER_WIDTH)
    {
      /* user has clicked in dead space */

      if (GIMP_IS_TEXT (text_tool->proxy))
        g_object_set (text_tool->proxy,
                      "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                      NULL);

      text_tool->handle_rectangle_change_complete = FALSE;
    }
  else
    {
      /* user has defined box for a new text layer */
    }

  gimp_rectangle_tool_button_release (tool, coords, time, state,
                                      release_type, display);

  text_tool->handle_rectangle_change_complete = TRUE;

  g_signal_handlers_unblock_by_func (text_tool->text_buffer,
                                     gimp_text_tool_text_buffer_mark_set,
                                     text_tool);
}

void
gimp_text_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  if (text_tool->selecting)
    {
      if (text_tool->layout)
        {
          GimpItem      *item   = GIMP_ITEM (text_tool->layer);
          gdouble        x      = coords->x - gimp_item_get_offset_x (item);
          gdouble        y      = coords->y - gimp_item_get_offset_y (item);
          GtkTextBuffer *buffer = text_tool->text_buffer;
          GtkTextIter    cursor;
          GtkTextIter    selection;
          gint           cursor_offset;
          gint           selection_offset;
          gint           offset;

          offset = gimp_text_tool_xy_to_offset (text_tool, x, y);

          gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                            gtk_text_buffer_get_insert (buffer));
          gtk_text_buffer_get_iter_at_mark (buffer, &selection,
                                            gtk_text_buffer_get_selection_bound (buffer));

          cursor_offset    = gtk_text_iter_get_offset (&cursor);
          selection_offset = gtk_text_iter_get_offset (&selection);

          if (text_tool->select_words ||
              text_tool->select_lines)
            {
              GtkTextIter start;
              GtkTextIter end;

              gtk_text_buffer_get_iter_at_offset (buffer, &cursor,
                                                  offset);
              gtk_text_buffer_get_iter_at_offset (buffer, &selection,
                                                  text_tool->select_start_offset);

              if (offset <= text_tool->select_start_offset)
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

              if (offset <= text_tool->select_start_offset)
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
          else
            {
              if (cursor_offset != offset)
                {
                  gtk_text_buffer_get_iter_at_offset (buffer, &cursor, offset);
                }
            }

          if (cursor_offset    != gtk_text_iter_get_offset (&cursor) ||
              selection_offset != gtk_text_iter_get_offset (&selection))
            {
              gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

              gtk_text_buffer_select_range (buffer, &cursor, &selection);

              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
            }
        }
    }
  else
    {
      gimp_rectangle_tool_motion (tool, coords, time, state, display);
    }
}

static gboolean
gimp_text_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpTextTool  *text_tool = GIMP_TEXT_TOOL (tool);
  GtkTextBuffer *buffer    = text_tool->text_buffer;
  GtkTextIter    cursor;
  GtkTextIter    selection;
  gint           x_pos  = -1;
  gboolean       retval = TRUE;

  if (display != tool->display)
    return FALSE;

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
      g_printerr ("binding handled event!\n");
      return TRUE;
    }

  gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                    gtk_text_buffer_get_insert (buffer));
  gtk_text_buffer_get_iter_at_mark (buffer, &selection,
                                    gtk_text_buffer_get_selection_bound (buffer));

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  switch (kevent->keyval)
    {
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      gimp_text_tool_enter_text (text_tool, "\n");
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_update_layout (text_tool);
      break;

    case GDK_Tab:
    case GDK_KP_Tab:
    case GDK_ISO_Left_Tab:
      gimp_text_tool_enter_text (text_tool, "\t");
      gimp_text_tool_reset_im_context (text_tool);
      gimp_text_tool_update_layout (text_tool);
      break;

    case GDK_Escape:
      gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (tool));
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;

    default:
      retval = FALSE;
    }

  text_tool->x_pos = x_pos;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  return retval;
}

static void
gimp_text_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (tool);

  gimp_rectangle_tool_oper_update (tool, coords, state, proximity, display);

  text_tool->moving = (gimp_rectangle_tool_get_function (rect_tool) ==
                       GIMP_RECTANGLE_TOOL_MOVING &&
                       (state & GDK_MOD1_MASK));
}

static void
gimp_text_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  if (tool->display == display)
    {
      GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

      if (gimp_rectangle_tool_point_in_rectangle (GIMP_RECTANGLE_TOOL (tool),
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          gimp_tool_control_set_cursor          (tool->control, GDK_XTERM);
          gimp_tool_control_set_cursor_modifier (tool->control,
                                                 GIMP_CURSOR_MODIFIER_NONE);
        }
      else
        {
          gimp_rectangle_tool_cursor_update (tool, coords, state, display);
        }
    }
  else
    {
      gimp_tool_control_set_cursor          (tool->control, GDK_XTERM);
      gimp_tool_control_set_cursor_modifier (tool->control,
                                             GIMP_CURSOR_MODIFIER_NONE);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static GimpUIManager *
gimp_text_tool_get_popup (GimpTool         *tool,
                          const GimpCoords *coords,
                          GdkModifierType   state,
                          GimpDisplay      *display,
                          const gchar     **ui_path)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);
  gint          x1, y1;
  gint          x2, y2;

  g_object_get (text_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (coords->x >= x1 && coords->x < x2 &&
      coords->y >= y1 && coords->y < y2)
    {
      if (! text_tool->ui_manager)
        {
          GimpDialogFactory *dialog_factory;
          GtkWidget         *im_menu;

          dialog_factory = gimp_dialog_factory_from_name ("toplevel");

          text_tool->ui_manager =
            gimp_menu_factory_manager_new (gimp_dialog_factory_get_menu_factory (dialog_factory),
                                           "<TextTool>",
                                           text_tool, FALSE);

          im_menu = gtk_ui_manager_get_widget (GTK_UI_MANAGER (text_tool->ui_manager),
                                               "/text-tool-popup/text-tool-input-methods-menu");

          if (GTK_IS_MENU_ITEM (im_menu))
            im_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (im_menu));

          gtk_im_multicontext_append_menuitems (GTK_IM_MULTICONTEXT (text_tool->im_context),
                                                GTK_MENU_SHELL (im_menu));
        }

      gimp_ui_manager_update (text_tool->ui_manager, text_tool);

      *ui_path = "/text-tool-popup";

      return text_tool->ui_manager;
    }

  return NULL;
}

static void
gimp_text_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTextTool   *text_tool = GIMP_TEXT_TOOL (draw_tool);
  GtkTextBuffer  *buffer    = text_tool->text_buffer;
  GdkRectangle    clip_rect;
  gint            x1, x2;
  gint            y1, y2;
  gint            logical_off_x = 0;
  gint            logical_off_y = 0;
  PangoLayout    *layout;
  PangoRectangle  ink_extents;
  PangoRectangle  logical_extents;

  g_object_set (text_tool,
                "narrow-mode", TRUE,
                NULL);

  gimp_rectangle_tool_draw (draw_tool);

  if (! text_tool->text  ||
      ! text_tool->layer ||
      ! text_tool->layer->text)
    return;

  /* There will be no layout if the function is called from the wrong place */
  if (! text_tool->layout)
    gimp_text_tool_update_layout (text_tool);

  /* Turn on clipping for text-cursor and selections */
  g_object_get (text_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  clip_rect.x      = x1;
  clip_rect.width  = x2 - x1;
  clip_rect.y      = y1;
  clip_rect.height = y2 - y1;

  gimp_draw_tool_set_clip_rect (draw_tool, &clip_rect, FALSE);

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);

  pango_layout_get_pixel_extents (layout, &ink_extents, &logical_extents);
  gimp_text_layout_transform_rect (text_tool->layout, &logical_extents);

  if (ink_extents.x < 0)
    logical_off_x = -ink_extents.x;

  if (ink_extents.y < 0)
    logical_off_y = -ink_extents.y;

  if (! gtk_text_buffer_get_has_selection (buffer))
    {
      /* If the text buffer has no selection, draw the text cursor */

      GtkTextIter     start;
      GtkTextIter     cursor;
      gint            cursor_index;
      PangoRectangle  crect;
      gchar          *string;
      gboolean        overwrite_cursor;

      gtk_text_buffer_get_start_iter (buffer, &start);
      gtk_text_buffer_get_iter_at_mark (buffer, &cursor,
                                        gtk_text_buffer_get_insert (buffer));

      string = gtk_text_buffer_get_text (buffer, &start, &cursor, FALSE);

      /* Using strlen to get the byte index, not the character offset */
      cursor_index = strlen (string);

      /* TODO: make cursor position itself even inside preedits! */
      if (text_tool->preedit_len > 0)
        cursor_index += text_tool->preedit_len;

      g_free (string);

      pango_layout_index_to_pos (layout, cursor_index, &crect);
      gimp_text_layout_transform_rect (text_tool->layout, &crect);

      crect.x      = PANGO_PIXELS (crect.x) + logical_off_x;
      crect.y      = PANGO_PIXELS (crect.y) + logical_off_y;
      crect.width  = PANGO_PIXELS (crect.width);
      crect.height = PANGO_PIXELS (crect.height);

      overwrite_cursor = text_tool->overwrite_mode && crect.width > 0;

      gimp_draw_tool_draw_text_cursor (draw_tool,
                                       crect.x, crect.y,
                                       overwrite_cursor ?
                                       crect.x + crect.width : crect.x,
                                       crect.y + crect.height,
                                       overwrite_cursor,
                                       TRUE);

      if (text_tool->preedit_string && text_tool->preedit_len > 0)
        gimp_text_tool_draw_preedit (draw_tool, logical_off_x, logical_off_y);
    }
  else
    {
      /* If the text buffer has a selection, highlight the selected
       * letters
       */
      gimp_text_tool_draw_selection (draw_tool, logical_off_x, logical_off_y);
    }

  /* Turn off clipping when done */
  gimp_draw_tool_set_clip_rect (draw_tool, NULL, FALSE);
}

static void
gimp_text_tool_draw_preedit (GimpDrawTool *draw_tool,
                             gint          logical_off_x,
                             gint          logical_off_y)
{
  GimpTextTool    *text_tool = GIMP_TEXT_TOOL (draw_tool);
  PangoLayout     *layout;
  PangoLayoutIter *line_iter;
  GtkTextIter      cursor, start;
  gint             i;
  gint             min, max;
  gchar           *string;

  gtk_text_buffer_get_selection_bounds (text_tool->text_buffer, &cursor, NULL);
  gtk_text_buffer_get_start_iter (text_tool->text_buffer, &start);

  string = gtk_text_buffer_get_text (text_tool->text_buffer,
                                     &start, &cursor, FALSE);
  min = strlen (string);
  g_free (string);

  max = min + text_tool->preedit_len;

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);
  line_iter = pango_layout_get_iter (layout);
  i = 0;

  do
    {
      gint    firstline, lastline;
      gint    first_x,   last_x;
      gdouble first_tmp, last_tmp;

      pango_layout_index_to_line_x (layout, min, 0, &firstline, &first_x);
      pango_layout_index_to_line_x (layout, max, 0, &lastline,  &last_x);

      first_tmp = first_x;
      last_tmp  = last_x;

      gimp_text_layout_transform_distance (text_tool->layout, &first_tmp, NULL);
      gimp_text_layout_transform_distance (text_tool->layout, &last_tmp,  NULL);

      first_x = PANGO_PIXELS (first_tmp) + logical_off_x;
      last_x  = PANGO_PIXELS (last_tmp)  + logical_off_x;

      if (i >= firstline && i <= lastline)
        {
          PangoRectangle crect;

          pango_layout_iter_get_line_extents (line_iter, NULL, &crect);
          pango_extents_to_pixels (&crect, NULL);

          gimp_text_layout_transform_rect (text_tool->layout, &crect);

          crect.x += logical_off_x;
          crect.y += logical_off_y;

          gimp_draw_tool_draw_line (draw_tool,
                                    crect.x, crect.y + crect.height,
                                    crect.x + crect.width,
                                    crect.y + crect.height,
                                    TRUE);

          if (i == firstline)
            {
              PangoRectangle crect2 = crect;

              crect2.width = first_x - crect.x;
              crect2.x     = crect.x;

              gimp_draw_tool_draw_line (draw_tool,
                                        crect2.x, crect2.y + crect2.height,
                                        crect2.width,
                                        crect2.y + crect2.height,
                                        TRUE);
            }

          if (i == lastline)
            {
              PangoRectangle crect2 = crect;

              crect2.width = crect.x + crect.width - last_x;
              crect2.x     = last_x;

              gimp_draw_tool_draw_line (draw_tool,
                                        crect2.x, crect2.y + crect2.height,
                                        crect2.x + crect2.width,
                                        crect2.y + crect2.height,
                                        TRUE);
            }
        }

      i++;
    }
  while (pango_layout_iter_next_line (line_iter));

  pango_layout_iter_free (line_iter);
}

static void
gimp_text_tool_draw_selection (GimpDrawTool *draw_tool,
                               gint          logical_off_x,
                               gint          logical_off_y)
{
  GimpTextTool    *text_tool = GIMP_TEXT_TOOL (draw_tool);
  GtkTextBuffer   *buffer    = text_tool->text_buffer;
  PangoLayout     *layout;
  PangoLayoutIter *line_iter;
  GtkTextIter      start;
  GtkTextIter      sel_start, sel_end;
  gint             i;
  gint             min, max;
  gchar           *string;

  gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);
  gtk_text_buffer_get_start_iter (buffer, &start);

  string = gtk_text_buffer_get_text (buffer, &start, &sel_start, FALSE);
  min = strlen (string);
  g_free (string);

  string = gtk_text_buffer_get_text (buffer, &start, &sel_end, FALSE);
  max = strlen (string);
  g_free (string);

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);
  line_iter = pango_layout_get_iter (layout);
  i = 0;

  /* Invert the selected letters by inverting all lines containing
   * selected letters, then invert the unselected letters on these
   * lines a second time to make them look normal
   */
  do
    {
      gint    firstline, lastline;
      gint    first_x,   last_x;
      gdouble first_tmp, last_tmp;

      pango_layout_index_to_line_x (layout, min, 0, &firstline, &first_x);
      pango_layout_index_to_line_x (layout, max, 0, &lastline,  &last_x);

      first_tmp = first_x;
      last_tmp  = last_x;

      gimp_text_layout_transform_distance (text_tool->layout, &first_tmp, NULL);
      gimp_text_layout_transform_distance (text_tool->layout, &last_tmp,  NULL);

      first_x = PANGO_PIXELS (first_tmp) + logical_off_x;
      last_x  = PANGO_PIXELS (last_tmp)  + logical_off_x;

      if (i >= firstline && i <= lastline)
        {
          PangoRectangle crect;

          pango_layout_iter_get_line_extents (line_iter, NULL, &crect);
          pango_extents_to_pixels (&crect, NULL);

          gimp_text_layout_transform_rect (text_tool->layout, &crect);

          crect.x += logical_off_x;
          crect.y += logical_off_y;

          gimp_draw_tool_draw_rectangle (draw_tool, TRUE,
                                         crect.x, crect.y,
                                         crect.width, crect.height,
                                         TRUE);

          if (i == firstline)
            {
              /* Twice invert all letters before the selection, making
               * them look normal
               */
              PangoRectangle crect2 = crect;

              crect2.width = first_x - crect.x;
              crect2.x     = crect.x;

              gimp_draw_tool_draw_rectangle (draw_tool, TRUE,
                                             crect2.x, crect2.y,
                                             crect2.width, crect2.height,
                                             TRUE);
            }

          if (i == lastline)
            {
              /* Twice invert all letters after the selection, making
               * them look normal
               */
              PangoRectangle crect2 = crect;

              crect2.width = crect.x + crect.width - last_x;
              crect2.x     = last_x;

              gimp_draw_tool_draw_rectangle (draw_tool, TRUE,
                                             crect2.x, crect2.y,
                                             crect2.width, crect2.height,
                                             TRUE);
            }
        }

      i++;
    }
  while (pango_layout_iter_next_line (line_iter));

  pango_layout_iter_free (line_iter);
}

static gboolean
gimp_text_tool_rectangle_change_complete (GimpRectangleTool *rect_tool)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (rect_tool);

  if (text_tool->handle_rectangle_change_complete)
    {
      GimpText *text = text_tool->text;
      GimpItem *item = GIMP_ITEM (text_tool->layer);
      gint      x1, y1;
      gint      x2, y2;

      g_object_get (rect_tool,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      text_tool->text_box_fixed = TRUE;

      if (! text || ! text->text || (text->text[0] == 0))
        {
          /* we can't set properties for the text layer, because it
           * isn't created until some text has been inserted, so we
           * need to make a special note that will remind us what to
           * do when we actually create the layer
           */
          return TRUE;
        }

      g_object_set (text_tool->proxy,
                    "box-mode",   GIMP_TEXT_BOX_FIXED,
                    "box-width",  (gdouble) (x2 - x1),
                    "box-height", (gdouble) (y2 - y1),
                    NULL);

      gimp_image_undo_group_start (text_tool->image, GIMP_UNDO_GROUP_TEXT,
                                   _("Reshape Text Layer"));

      gimp_item_translate (item,
                           x1 - gimp_item_get_offset_x (item),
                           y1 - gimp_item_get_offset_y (item),
                           TRUE);
      gimp_text_tool_apply (text_tool);

      gimp_image_undo_group_end (text_tool->image);
    }

  return TRUE;
}

static void
gimp_text_tool_halt (GimpTextTool *text_tool)
{
  gimp_text_tool_set_drawable (text_tool, NULL, FALSE);

  if (text_tool->editor)
    gtk_widget_destroy (text_tool->editor);

  if (text_tool->proxy_text_view)
    {
      gtk_widget_destroy (text_tool->offscreen_window);
      text_tool->offscreen_window = NULL;
      text_tool->proxy_text_view = NULL;
    }

  gtk_im_context_focus_out (text_tool->im_context);

  gtk_im_context_set_client_window (text_tool->im_context, NULL);
}

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

  g_printerr ("%s: %s count = %d, select = %s\n",
              G_STRFUNC,
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

        string = gtk_text_buffer_get_text (buffer, &start, &cursor, FALSE);
        cursor_index = strlen (string);
        g_free (string);

        layout = gimp_text_layout_get_pango_layout (text_tool->layout);

        pango_layout_index_to_line_x (layout, cursor_index, FALSE,
                                      &line, &x_pos);

        layout_iter = pango_layout_get_iter (layout);
        for (i = 0; i < line; i++)
          pango_layout_iter_next_line (layout_iter);

        pango_layout_iter_get_line_extents (layout_iter, NULL, &logical);

        pango_layout_iter_free (layout_iter);

        /*  try to go to the remembered x_pos if it exists *and* we are at
         *  the beginning or at the end of the current line
         */
        if (text_tool->x_pos != -1 && (x_pos <= logical.x ||
                                       x_pos >= logical.width))
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

        pango_layout_line_x_to_index (layout_line, x_pos,
                                      &cursor_index, &trailing);

        string = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

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

  gtk_text_buffer_select_range (buffer, &cursor, sel_start);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_insert_at_cursor (GimpTextTool *text_tool,
                                 const gchar  *str)
{
  gtk_text_buffer_insert_interactive_at_cursor (text_tool->text_buffer,
                                                str, -1, TRUE);
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

  g_printerr ("%s: %s count = %d\n",
              G_STRFUNC,
              g_enum_get_value (g_type_class_ref (GTK_TYPE_DELETE_TYPE),
                                type)->value_name,
              count);

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
  GtkTextBuffer *buffer = text_tool->text_buffer;

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


/*  private functions  */

static void
gimp_text_tool_connect (GimpTextTool  *text_tool,
                        GimpTextLayer *layer,
                        GimpText      *text)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  g_return_if_fail (text == NULL || (layer != NULL && layer->text == text));

  if (text_tool->text != text)
    {
      GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (tool);

      if (text_tool->text)
        {
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                gimp_text_tool_text_notify,
                                                text_tool);

          if (text_tool->pending)
            gimp_text_tool_apply (text_tool);

          g_object_unref (text_tool->text);
          text_tool->text = NULL;

          g_object_set (text_tool->proxy, "text", NULL, NULL);
        }

      gimp_context_define_property (GIMP_CONTEXT (options),
                                    GIMP_CONTEXT_PROP_FOREGROUND,
                                    text != NULL);

      if (text)
        {
          gimp_config_sync (G_OBJECT (text), G_OBJECT (text_tool->proxy), 0);

          text_tool->text = g_object_ref (text);

          g_signal_connect (text, "notify",
                            G_CALLBACK (gimp_text_tool_text_notify),
                            text_tool);
        }
    }

  if (text_tool->layer != layer)
    {
      if (text_tool->layer)
        g_signal_handlers_disconnect_by_func (text_tool->layer,
                                              gimp_text_tool_layer_notify,
                                              text_tool);

      text_tool->layer = layer;

      if (layer)
        g_signal_connect_object (text_tool->layer, "notify::modified",
                                 G_CALLBACK (gimp_text_tool_layer_notify),
                                 text_tool, 0);
    }
}

static void
gimp_text_tool_use_editor_notify (GimpTextOptions *options,
                                  GParamSpec      *pspec,
                                  GimpTextTool    *text_tool)
{
  if (options->use_editor)
    {
      if (text_tool->text)
        gimp_text_tool_editor_dialog (text_tool);
    }
  else
    {
      if (text_tool->editor)
        gtk_widget_destroy (text_tool->editor);
    }
}

static void
gimp_text_tool_layer_notify (GimpTextLayer *layer,
                             GParamSpec    *pspec,
                             GimpTextTool  *text_tool)
{
  if (layer->modified)
    gimp_text_tool_connect (text_tool, NULL, NULL);
}

static void
gimp_text_tool_proxy_notify (GimpText     *text,
                             GParamSpec   *pspec,
                             GimpTextTool *text_tool)
{
  if (! text_tool->text)
    return;

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      text_tool->pending = g_list_append (text_tool->pending, pspec);

      if (text_tool->idle_id)
        g_source_remove (text_tool->idle_id);

      text_tool->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) gimp_text_tool_idle_apply, text_tool,
                         NULL);
    }
}

static void
gimp_text_tool_text_notify (GimpText     *text,
                            GParamSpec   *pspec,
                            GimpTextTool *text_tool)
{
  g_return_if_fail (text == text_tool->text);

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      GValue value = { 0, };

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (text), pspec->name, &value);

      g_signal_handlers_block_by_func (text_tool->proxy,
                                       gimp_text_tool_proxy_notify,
                                       text_tool);

      g_object_set_property (G_OBJECT (text_tool->proxy), pspec->name, &value);

      g_signal_handlers_unblock_by_func (text_tool->proxy,
                                         gimp_text_tool_proxy_notify,
                                         text_tool);

      g_value_unset (&value);
    }

  /* we need to redraw the rectangle if it is visible and the shape of
   * the layer has changed, because of an undo for example.
   */
  if (strcmp (pspec->name, "box-width") == 0  ||
      strcmp (pspec->name, "box-height") == 0 ||
      text->box_mode == GIMP_TEXT_BOX_DYNAMIC)
    {
      GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);

      text_tool->handle_rectangle_change_complete = FALSE;

      gimp_rectangle_tool_frame_item (rect_tool,
                                      GIMP_ITEM (text_tool->layer));

      text_tool->handle_rectangle_change_complete = TRUE;
    }

  /* if the text has changed, (probably because of an undo), we put
   * the new text into the text buffer
   */
  if (strcmp (pspec->name, "text") == 0)
    {
      g_signal_handlers_block_by_func (text_tool->proxy,
                                       gimp_text_tool_text_buffer_changed,
                                       text_tool);

      gtk_text_buffer_set_text (text_tool->text_buffer, text->text, -1);

      g_signal_handlers_unblock_by_func (text_tool->proxy,
                                         gimp_text_tool_text_buffer_changed,
                                         text_tool);

      /* force change of cursor and selection display */
      gimp_text_tool_update_layout (text_tool);

      return;
    }
}

static gboolean
gimp_text_tool_idle_apply (GimpTextTool *text_tool)
{
  text_tool->idle_id = 0;

  gimp_text_tool_apply (text_tool);

  return FALSE;
}

static void
gimp_text_tool_apply (GimpTextTool *text_tool)
{
  const GParamSpec *pspec = NULL;
  GimpImage        *image;
  GimpTextLayer    *layer;
  GObject          *src;
  GObject          *dest;
  GList            *list;
  gboolean          push_undo  = TRUE;
  gboolean          undo_group = FALSE;

  if (text_tool->idle_id)
    {
      g_source_remove (text_tool->idle_id);
      text_tool->idle_id = 0;
    }

  g_return_if_fail (text_tool->text != NULL);
  g_return_if_fail (text_tool->layer != NULL);

  layer = text_tool->layer;
  image = gimp_item_get_image (GIMP_ITEM (layer));

  g_return_if_fail (layer->text == text_tool->text);

  /*  Walk over the list of changes and figure out if we are changing
   *  a single property or need to push a full text undo.
   */
  for (list = text_tool->pending;
       list && list->next && list->next->data == list->data;
       list = list->next)
    /* do nothing */;

  if (g_list_length (list) == 1)
    pspec = list->data;

  /*  If we are changing a single property, we don't need to push
   *  an undo if all of the following is true:
   *   - the redo stack is empty
   *   - the last item on the undo stack is a text undo
   *   - the last undo changed the same text property on the same layer
   *   - the last undo happened less than TEXT_UNDO_TIMEOUT seconds ago
   */
  if (pspec)
    {
      GimpUndo *undo = gimp_image_undo_can_compress (image, GIMP_TYPE_TEXT_UNDO,
                                                     GIMP_UNDO_TEXT_LAYER);

      if (undo && GIMP_ITEM_UNDO (undo)->item == GIMP_ITEM (layer))
        {
          GimpTextUndo *text_undo = GIMP_TEXT_UNDO (undo);

          if (text_undo->pspec == pspec)
            {
              if (gimp_undo_get_age (undo) < TEXT_UNDO_TIMEOUT)
                {
                  GimpTool    *tool = GIMP_TOOL (text_tool);
                  GimpContext *context;

                  context = GIMP_CONTEXT (gimp_tool_get_options (tool));

                  push_undo = FALSE;
                  gimp_undo_reset_age (undo);
                  gimp_undo_refresh_preview (undo, context);
                }
            }
        }
    }

  if (push_undo)
    {
      if (layer->modified)
        {
          undo_group = TRUE;
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT, NULL);

          gimp_image_undo_push_text_layer_modified (image, NULL, layer);
          gimp_image_undo_push_drawable_mod (image,
                                             NULL, GIMP_DRAWABLE (layer));
        }

      gimp_image_undo_push_text_layer (image, NULL, layer, pspec);
    }

  src  = G_OBJECT (text_tool->proxy);
  dest = G_OBJECT (text_tool->text);

  g_signal_handlers_block_by_func (dest,
                                   gimp_text_tool_text_notify,
                                   text_tool);

  g_object_freeze_notify (dest);

  for (; list; list = list->next)
    {
      GValue value = { 0, };

      /*  look ahead and compress changes  */
      if (list->next && list->next->data == list->data)
        continue;

      pspec = list->data;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (src,  pspec->name, &value);
      g_object_set_property (dest, pspec->name, &value);

      g_value_unset (&value);
    }

  g_list_free (text_tool->pending);
  text_tool->pending = NULL;

  g_object_thaw_notify (dest);

  g_signal_handlers_unblock_by_func (dest,
                                     gimp_text_tool_text_notify,
                                     text_tool);

  if (push_undo)
    {
      g_object_set (layer, "modified", FALSE, NULL);

      if (undo_group)
        gimp_image_undo_group_end (image);
    }

  /* if we're doing dynamic text, we want to update the shape of the
   * rectangle
   */
  if (layer->text->box_mode == GIMP_TEXT_BOX_DYNAMIC)
    {
      text_tool->handle_rectangle_change_complete = FALSE;

      gimp_rectangle_tool_frame_item (GIMP_RECTANGLE_TOOL (text_tool),
                                      GIMP_ITEM (layer));

      text_tool->handle_rectangle_change_complete = TRUE;
    }

  gimp_image_flush (image);
  gimp_text_tool_update_layout (text_tool);
}

static void
gimp_text_tool_create_layer (GimpTextTool *text_tool,
                             GimpText     *text)
{
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);
  GimpTool          *tool      = GIMP_TOOL (text_tool);
  GimpImage         *image     = gimp_display_get_image (tool->display);
  GimpLayer         *layer;
  gint               x1, y1;
  gint               x2, y2;

  if (text)
    {
      text = gimp_config_duplicate (GIMP_CONFIG (text));
    }
  else
    {
      gchar *str = gimp_text_tool_editor_get_text (text_tool);

      g_object_set (text_tool->proxy,
                    "text",     str,
                    "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                    NULL);

      g_free (str);

      text = gimp_config_duplicate (GIMP_CONFIG (text_tool->proxy));
    }

  layer = gimp_text_layer_new (image, text);

  g_object_unref (text);

  if (! layer)
    return;

  gimp_text_tool_connect (text_tool, GIMP_TEXT_LAYER (layer), text);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  if (gimp_image_get_floating_selection (image))
    {
      g_signal_handlers_block_by_func (image,
                                       gimp_text_tool_layer_changed,
                                       text_tool);

      floating_sel_anchor (gimp_image_get_floating_selection (image));

      g_signal_handlers_unblock_by_func (image,
                                         gimp_text_tool_layer_changed,
                                         text_tool);
    }

  g_object_get (rect_tool,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  gimp_item_set_offset (GIMP_ITEM (layer), x1, y1);

  gimp_image_add_layer (image, layer,
                        GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  if (text_tool->text_box_fixed)
    {
      GimpItem *item = GIMP_ITEM (layer);

      g_object_set (text_tool->proxy,
                    "box-mode",   GIMP_TEXT_BOX_FIXED,
                    "box-width",  (gdouble) (x2 - x1),
                    "box-height", (gdouble) (y2 - y1),
                    NULL);
      gimp_item_translate (item,
                           x1 - gimp_item_get_offset_x (item),
                           y1 - gimp_item_get_offset_y (item),
                           TRUE);
    }
  else
    {
      text_tool->handle_rectangle_change_complete = FALSE;

      gimp_rectangle_tool_frame_item (GIMP_RECTANGLE_TOOL (text_tool),
                                      GIMP_ITEM (layer));

      text_tool->handle_rectangle_change_complete = TRUE;
    }

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);

  gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), FALSE);
}

static void
gimp_text_tool_editor_dialog (GimpTextTool *text_tool)
{
  GimpTool          *tool    = GIMP_TOOL (text_tool);
  GimpTextOptions   *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpDialogFactory *dialog_factory;
  GtkWindow         *parent  = NULL;

  if (text_tool->editor)
    {
      gtk_window_present (GTK_WINDOW (text_tool->editor));
      return;
    }

  dialog_factory = gimp_dialog_factory_from_name ("toplevel");

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      parent = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (shell)));
    }

  text_tool->editor = gimp_text_options_editor_new (parent, options,
                                                    gimp_dialog_factory_get_menu_factory (dialog_factory),
                                                    _("GIMP Text Editor"),
                                                    text_tool->text_buffer);

  g_object_add_weak_pointer (G_OBJECT (text_tool->editor),
                             (gpointer) &text_tool->editor);

  gimp_dialog_factory_add_foreign (dialog_factory,
                                   "gimp-text-tool-dialog",
                                   text_tool->editor);

  gtk_widget_show (text_tool->editor);
}

static void
gimp_text_tool_editor (GimpTextTool *text_tool)
{
  GimpTool         *tool    = GIMP_TOOL (text_tool);
  GimpTextOptions  *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpDisplayShell *shell   = gimp_display_get_shell (tool->display);

  gtk_im_context_set_client_window (text_tool->im_context,
                                    gtk_widget_get_window (shell->canvas));

  gtk_im_context_focus_in (text_tool->im_context);

  if (text_tool->text)
    gtk_text_buffer_set_text (text_tool->text_buffer,
                              text_tool->text->text, -1);
  else
    gtk_text_buffer_set_text (text_tool->text_buffer, "", -1);

  gimp_text_tool_update_layout (text_tool);

  if (options->use_editor)
    gimp_text_tool_editor_dialog (text_tool);
}

static gchar *
gimp_text_tool_editor_get_text (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  GtkTextIter    start, end;
  GtkTextIter    selstart, selend;
  gchar         *string;
  gchar         *fb;
  gchar         *lb;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_get_selection_bounds (buffer, &selstart, &selend);

  fb = gtk_text_buffer_get_text (buffer, &start, &selstart, TRUE);
  lb = gtk_text_buffer_get_text (buffer, &selstart, &end, TRUE);

  if (text_tool->preedit_string)
    {
      if (fb == NULL)
        string = g_strconcat (text_tool->preedit_string, lb, NULL);
      else
        string = g_strconcat (fb, text_tool->preedit_string, lb, NULL);
    }
  else
    {
      string = g_strconcat (fb, lb, NULL);
    }

  g_free (fb);
  g_free (lb);

  return string;
}


#define  RESPONSE_NEW 1

static void
gimp_text_tool_confirm_response (GtkWidget    *widget,
                                 gint          response_id,
                                 GimpTextTool *text_tool)
{
  GimpTextLayer *layer = text_tool->layer;

  gtk_widget_destroy (widget);

  if (layer && layer->text)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          gimp_text_tool_create_layer (text_tool, layer->text);
          break;

        case GTK_RESPONSE_ACCEPT:
          gimp_text_tool_connect (text_tool, layer, layer->text);

          /*  cause the text layer to be rerendered  */
          if (text_tool->proxy)
            g_object_notify (G_OBJECT (text_tool->proxy), "text");

          gimp_text_tool_editor (text_tool);
          break;

        default:
          break;
        }
    }
}

static void
gimp_text_tool_confirm_dialog (GimpTextTool *text_tool)
{
  GimpTool         *tool  = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (tool->display);
  GtkWidget        *dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;

  g_return_if_fail (text_tool->layer != NULL);

  if (text_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->confirm_dialog));
      return;
    }

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (text_tool->layer),
                                     GIMP_CONTEXT (gimp_tool_get_options (tool)),
                                     _("Confirm Text Editing"),
                                     "gimp-text-tool-confirm",
                                     GIMP_STOCK_TEXT_LAYER,
                                     _("Confirm Text Editing"),
                                     GTK_WIDGET (shell),
                                     gimp_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     GTK_STOCK_CANCEL,       GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_EDIT,         GTK_RESPONSE_ACCEPT,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_NEW,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_text_tool_confirm_response),
                    text_tool);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("The layer you selected is a text layer but "
                           "it has been modified using other tools. "
                           "Editing the layer with the text tool will "
                           "discard these modifications."
                           "\n\n"
                           "You can edit the layer or create a new "
                           "text layer from its text attributes."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  text_tool->confirm_dialog = dialog;
  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_nullify_pointer),
                            &text_tool->confirm_dialog);
}

static void
gimp_text_tool_layer_changed (GimpImage    *image,
                              GimpTextTool *text_tool)
{
  GimpLayer         *layer     = gimp_image_get_active_layer (image);
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (text_tool);

  gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), FALSE);

  if (text_tool->layer)
    {
      if (! gimp_rectangle_tool_rectangle_is_new (rect_tool))
        {
          text_tool->handle_rectangle_change_complete = FALSE;

          gimp_rectangle_tool_frame_item (rect_tool,
                                          GIMP_ITEM (text_tool->layer));

          text_tool->handle_rectangle_change_complete = TRUE;
        }
    }
}

static void
gimp_text_tool_set_image (GimpTextTool *text_tool,
                          GimpImage    *image)
{
  if (text_tool->image == image)
    return;

  if (text_tool->image)
    {
      g_signal_handlers_disconnect_by_func (text_tool->image,
                                            gimp_text_tool_layer_changed,
                                            text_tool);

      g_object_remove_weak_pointer (G_OBJECT (text_tool->image),
                                    (gpointer) &text_tool->image);
      text_tool->image = NULL;
    }

  if (image)
    {
      GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
      gdouble          xres;
      gdouble          yres;

      gimp_image_get_resolution (image, &xres, &yres);

      text_tool->image = image;
      g_object_add_weak_pointer (G_OBJECT (text_tool->image),
                                 (gpointer) &text_tool->image);

      g_signal_connect_object (text_tool->image, "active-layer-changed",
                               G_CALLBACK (gimp_text_tool_layer_changed),
                               text_tool, 0);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_entry), 0,
                                      yres, FALSE);
    }
}

static gboolean
gimp_text_tool_set_drawable (GimpTextTool *text_tool,
                             GimpDrawable *drawable,
                             gboolean      confirm)
{
  GimpImage *image = NULL;

  if (text_tool->confirm_dialog)
    gtk_widget_destroy (text_tool->confirm_dialog);

  if (drawable)
    image = gimp_item_get_image (GIMP_ITEM (drawable));

  gimp_text_tool_set_image (text_tool, image);

  if (GIMP_IS_TEXT_LAYER (drawable) && GIMP_TEXT_LAYER (drawable)->text)
    {
      GimpTextLayer *layer = GIMP_TEXT_LAYER (drawable);

      if (layer == text_tool->layer && layer->text == text_tool->text)
        return TRUE;

      if (layer->modified)
        {
          if (confirm)
            {
              gimp_text_tool_connect (text_tool, layer, NULL);
              gimp_text_tool_confirm_dialog (text_tool);
              return TRUE;
            }
        }
      else
        {
          gimp_text_tool_connect (text_tool, layer, layer->text);
          return TRUE;
        }
    }

  gimp_text_tool_connect (text_tool, NULL, NULL);

  return FALSE;
}

static void
gimp_text_tool_update_proxy (GimpTextTool *text_tool)
{
  if (text_tool->text)
    {
      gchar *string = gimp_text_tool_editor_get_text (text_tool);

      g_object_set (text_tool->proxy,
                    "text", string,
                    NULL);

      g_free (string);
    }
  else
    {
      gimp_text_tool_create_layer (text_tool, NULL);
    }
}

static void
gimp_text_tool_enter_text (GimpTextTool *text_tool,
                           const gchar  *str)
{
  GtkTextBuffer *buffer = text_tool->text_buffer;
  gboolean       had_selection;

  had_selection = gtk_text_buffer_get_has_selection (buffer);

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
}

static void
gimp_text_tool_text_buffer_changed (GtkTextBuffer *text_buffer,
                                    GimpTextTool  *text_tool)
{
  gimp_text_tool_update_proxy (text_tool);
}

static void
gimp_text_tool_text_buffer_mark_set (GtkTextBuffer *text_buffer,
                                     GtkTextIter   *iter,
                                     GtkTextMark   *mark,
                                     GimpTextTool  *text_tool)
{
  gimp_text_tool_update_layout (text_tool);
}

static void
gimp_text_tool_update_layout (GimpTextTool *text_tool)
{
  GimpImage *image;

  if (! text_tool->text)
    {
      gimp_text_tool_update_proxy (text_tool);
      return;
    }

  if (text_tool->layout)
    g_object_unref (text_tool->layout);

  image = gimp_item_get_image (GIMP_ITEM (text_tool->layer));

  text_tool->layout = gimp_text_layout_new (text_tool->layer->text, image);
}

static gint
gimp_text_tool_xy_to_offset (GimpTextTool *text_tool,
                             gdouble       x,
                             gdouble       y)
{
  GtkTextIter     start, end;
  PangoLayout    *layout;
  PangoRectangle  ink_extents;
  gchar          *string;
  gint            offset;
  gint            trailing;

  gimp_text_layout_untransform_point (text_tool->layout, &x, &y);

  /*  adjust to offset of logical rect  */
  layout = gimp_text_layout_get_pango_layout (text_tool->layout);
  pango_layout_get_pixel_extents (layout, &ink_extents, NULL);

  if (ink_extents.x < 0)
    x += ink_extents.x;

  if (ink_extents.y < 0)
    y += ink_extents.y;

  gtk_text_buffer_get_bounds (text_tool->text_buffer, &start, &end);
  string = gtk_text_buffer_get_text (text_tool->text_buffer,
                                     &start, &end, TRUE);

  pango_layout_xy_to_index (layout,
                            x * PANGO_SCALE,
                            y * PANGO_SCALE,
                            &offset, &trailing);

  offset = g_utf8_pointer_to_offset (string, string + offset);
  offset += trailing;

  g_free (string);

  return offset;
}


/*  IM context utilities and callbacks  */

static void
gimp_text_tool_reset_im_context (GimpTextTool *text_tool)
{
  if (text_tool->needs_im_reset)
    {
      text_tool->needs_im_reset = FALSE;
      gtk_im_context_reset (text_tool->im_context);
    }
}

static void
gimp_text_tool_commit_cb (GtkIMContext *context,
                          const gchar  *str,
                          GimpTextTool *text_tool)
{
  gimp_text_tool_enter_text (text_tool, str);
}

static void
gimp_text_tool_preedit_changed_cb (GtkIMContext *context,
                                   GimpTextTool *text_tool)
{
  if (text_tool->preedit_string)
    g_free (text_tool->preedit_string);

  gtk_im_context_get_preedit_string (context,
                                     &text_tool->preedit_string, NULL,
                                     &text_tool->preedit_cursor);

  text_tool->preedit_len = strlen (text_tool->preedit_string);

  gimp_text_tool_update_proxy (text_tool);
}


/*  public functions  */

void
gimp_text_tool_set_layer (GimpTextTool *text_tool,
                          GimpLayer    *layer)
{
  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));
  g_return_if_fail (layer == NULL || GIMP_IS_LAYER (layer));

  if (gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), TRUE))
    {
      GimpTool    *tool = GIMP_TOOL (text_tool);
      GimpItem    *item = GIMP_ITEM (layer);
      GimpContext *context;
      GimpDisplay *display;

      context = gimp_get_user_context (tool->tool_info->gimp);
      display = gimp_context_get_display (context);

      if (! display ||
          gimp_display_get_image (display) != gimp_item_get_image (item))
        {
          GList *list;

          display = NULL;

          for (list = gimp_get_display_iter (tool->tool_info->gimp);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (gimp_display_get_image (display) == gimp_item_get_image (item))
                {
                  gimp_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      tool->display = display;

      if (tool->display)
        {
          GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

          tool->drawable = GIMP_DRAWABLE (layer);

          if (gimp_draw_tool_is_active (draw_tool))
            gimp_draw_tool_stop (draw_tool);

          gimp_draw_tool_start (draw_tool, display);

          gimp_rectangle_tool_frame_item (GIMP_RECTANGLE_TOOL (tool),
                                          GIMP_ITEM (layer));

          gimp_text_tool_editor (text_tool);
        }
    }
}

gboolean
gimp_text_tool_get_has_text_selection (GimpTextTool *text_tool)
{
  return gtk_text_buffer_get_has_selection (text_tool->text_buffer);
}

void
gimp_text_tool_delete_selection (GimpTextTool *text_tool)
{
  if (gtk_text_buffer_get_has_selection (text_tool->text_buffer))
    {
      gtk_text_buffer_delete_selection (text_tool->text_buffer, TRUE, TRUE);
    }
}

void
gimp_text_tool_cut_clipboard (GimpTextTool *text_tool)
{
  GimpDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  shell = gimp_display_get_shell (GIMP_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);
  gtk_text_buffer_cut_clipboard (text_tool->text_buffer, clipboard, TRUE);
}

void
gimp_text_tool_copy_clipboard (GimpTextTool *text_tool)
{
  GimpDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  shell = gimp_display_get_shell (GIMP_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_copy_clipboard (text_tool->text_buffer, clipboard);
}

void
gimp_text_tool_paste_clipboard (GimpTextTool *text_tool)
{
  GimpDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  shell = gimp_display_get_shell (GIMP_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_paste_clipboard (text_tool->text_buffer, clipboard, NULL, TRUE);
}

void
gimp_text_tool_create_vectors (GimpTextTool *text_tool)
{
  GimpVectors *vectors;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  if (! text_tool->text || ! text_tool->image)
    return;

  vectors = gimp_text_vectors_new (text_tool->image, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      gimp_item_get_offset (GIMP_ITEM (text_tool->layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (vectors), x, y, FALSE);
    }

  gimp_image_add_vectors (text_tool->image, vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_flush (text_tool->image);
}

void
gimp_text_tool_create_vectors_warped (GimpTextTool *text_tool)
{
  GimpVectors *vectors0;
  GimpVectors *vectors;
  gdouble      box_height;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  if (! text_tool->text || ! text_tool->image || ! text_tool->layer)
    return;

  box_height = gimp_item_get_height (GIMP_ITEM (text_tool->layer));

  vectors0 = gimp_image_get_active_vectors (text_tool->image);
  if (! vectors0)
    return;

  vectors = gimp_text_vectors_new (text_tool->image, text_tool->text);

  gimp_vectors_warp_vectors (vectors0, vectors, 0.5 * box_height);

  gimp_image_add_vectors (text_tool->image, vectors,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
  gimp_item_set_visible (GIMP_ITEM (vectors), TRUE, FALSE);

  gimp_image_flush (text_tool->image);
}
