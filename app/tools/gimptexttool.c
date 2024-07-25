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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpasyncset.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimp-palettes.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimplist.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "menus/menus.h"

#include "text/gimptext.h"
#include "text/gimptext-path.h"
#include "text/gimptextlayer.h"
#include "text/gimptextlayout.h"
#include "text/gimptextundo.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimppath.h"
#include "vectors/gimppath-warp.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimptextbuffer.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpviewabledialog.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolrectangle.h"

#include "gimptextoptions.h"
#include "gimptexttool.h"
#include "gimptexttool-editor.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


#define TEXT_UNDO_TIMEOUT 3


/*  local function prototypes  */

static void      gimp_text_tool_constructed     (GObject           *object);
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
static gboolean  gimp_text_tool_key_release     (GimpTool          *tool,
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
static void      gimp_text_tool_draw_selection  (GimpDrawTool      *draw_tool);

static gboolean  gimp_text_tool_start           (GimpTextTool      *text_tool,
                                                 GimpDisplay       *display,
                                                 GimpLayer         *layer,
                                                 GError           **error);
static void      gimp_text_tool_halt            (GimpTextTool      *text_tool);

static void      gimp_text_tool_frame_item      (GimpTextTool      *text_tool);

static void      gimp_text_tool_rectangle_response
                                                (GimpToolRectangle *rectangle,
                                                 gint               response_id,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_rectangle_change_complete
                                                (GimpToolRectangle *rectangle,
                                                 GimpTextTool      *text_tool);

static void      gimp_text_tool_connect         (GimpTextTool      *text_tool,
                                                 GimpTextLayer     *layer,
                                                 GimpText          *text);

static void      gimp_text_tool_layer_notify    (GimpTextLayer     *layer,
                                                 const GParamSpec  *pspec,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_proxy_notify    (GimpText          *text,
                                                 const GParamSpec  *pspec,
                                                 GimpTextTool      *text_tool);

static void      gimp_text_tool_text_notify     (GimpText          *text,
                                                 const GParamSpec  *pspec,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_text_changed    (GimpText          *text,
                                                 GimpTextTool      *text_tool);

static void
    gimp_text_tool_fonts_async_set_empty_notify (GimpAsyncSet      *async_set,
                                                 GParamSpec        *pspec,
                                                 GimpTextTool      *text_tool);

static void      gimp_text_tool_apply_list      (GimpTextTool      *text_tool,
                                                 GList             *pspecs);

static void      gimp_text_tool_create_layer    (GimpTextTool      *text_tool,
                                                 GimpText          *text);

static void      gimp_text_tool_layer_changed   (GimpImage         *image,
                                                 GimpTextTool      *text_tool);
static void      gimp_text_tool_set_image       (GimpTextTool      *text_tool,
                                                 GimpImage         *image);
static gboolean  gimp_text_tool_set_drawable    (GimpTextTool      *text_tool,
                                                 GimpDrawable      *drawable,
                                                 gboolean           confirm);

static void      gimp_text_tool_block_drawing   (GimpTextTool      *text_tool);
static void      gimp_text_tool_unblock_drawing (GimpTextTool      *text_tool);

static void    gimp_text_tool_buffer_begin_edit (GimpTextBuffer    *buffer,
                                                 GimpTextTool      *text_tool);
static void    gimp_text_tool_buffer_end_edit   (GimpTextBuffer    *buffer,
                                                 GimpTextTool      *text_tool);

static void    gimp_text_tool_buffer_color_applied
                                                (GimpTextBuffer    *buffer,
                                                 GeglColor         *color,
                                                 GimpTextTool      *text_tool);


G_DEFINE_TYPE (GimpTextTool, gimp_text_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_text_tool_parent_class


void
gimp_text_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_TEXT_TOOL,
                GIMP_TYPE_TEXT_OPTIONS,
                gimp_text_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_FONT       |
                GIMP_CONTEXT_PROP_MASK_PALETTE /* for the color popup's palette tab */,
                "gimp-text-tool",
                _("Text"),
                _("Text Tool: Create or edit text layers"),
                N_("Te_xt"), "T",
                NULL, GIMP_HELP_TOOL_TEXT,
                GIMP_ICON_TOOL_TEXT,
                data);
}

static void
gimp_text_tool_class_init (GimpTextToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructed    = gimp_text_tool_constructed;
  object_class->finalize       = gimp_text_tool_finalize;

  tool_class->control          = gimp_text_tool_control;
  tool_class->button_press     = gimp_text_tool_button_press;
  tool_class->motion           = gimp_text_tool_motion;
  tool_class->button_release   = gimp_text_tool_button_release;
  tool_class->key_press        = gimp_text_tool_key_press;
  tool_class->key_release      = gimp_text_tool_key_release;
  tool_class->oper_update      = gimp_text_tool_oper_update;
  tool_class->cursor_update    = gimp_text_tool_cursor_update;
  tool_class->get_popup        = gimp_text_tool_get_popup;

  draw_tool_class->draw        = gimp_text_tool_draw;
}

static void
gimp_text_tool_init (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  text_tool->buffer = gimp_text_buffer_new ();

  g_signal_connect (text_tool->buffer, "begin-user-action",
                    G_CALLBACK (gimp_text_tool_buffer_begin_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "end-user-action",
                    G_CALLBACK (gimp_text_tool_buffer_end_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "color-applied",
                    G_CALLBACK (gimp_text_tool_buffer_color_applied),
                    text_tool);

  text_tool->handle_rectangle_change_complete = TRUE;

  gimp_text_tool_editor_init (text_tool);

  gimp_tool_control_set_scroll_lock          (tool->control, TRUE);
  gimp_tool_control_set_handle_empty_image   (tool->control, TRUE);
  gimp_tool_control_set_wants_click          (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click   (tool->control, TRUE);
  gimp_tool_control_set_wants_triple_click   (tool->control, TRUE);
  gimp_tool_control_set_wants_all_key_events (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers     (tool->control,
                                              GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision            (tool->control,
                                              GIMP_CURSOR_PRECISION_PIXEL_BORDER);
  gimp_tool_control_set_tool_cursor          (tool->control,
                                              GIMP_TOOL_CURSOR_TEXT);
  gimp_tool_control_set_action_object_1      (tool->control,
                                              "context-font-select-set");
}

static void
gimp_text_tool_constructed (GObject *object)
{
  GimpTextTool    *text_tool = GIMP_TEXT_TOOL (object);
  GimpTextOptions *options   = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  GimpTool        *tool      = GIMP_TOOL (text_tool);
  GimpAsyncSet    *async_set;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  text_tool->proxy = g_object_new (GIMP_TYPE_TEXT, "gimp", tool->tool_info->gimp, NULL);

  gimp_text_options_connect_text (options, text_tool->proxy);

  g_signal_connect_object (text_tool->proxy, "notify",
                           G_CALLBACK (gimp_text_tool_proxy_notify),
                           text_tool, 0);

  async_set =
    gimp_data_factory_get_async_set (tool->tool_info->gimp->font_factory);

  g_signal_connect_object (async_set,
                           "notify::empty",
                           G_CALLBACK (gimp_text_tool_fonts_async_set_empty_notify),
                           text_tool, 0);
}

static void
gimp_text_tool_finalize (GObject *object)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (object);

  g_clear_object (&text_tool->proxy);
  g_clear_object (&text_tool->buffer);

  gimp_text_tool_editor_finalize (text_tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_tool_remove_empty_text_layer (GimpTextTool *text_tool)
{
  GimpTextLayer *text_layer = text_tool->layer;

  if (text_layer && text_layer->auto_rename)
    {
      GimpText *text = gimp_text_layer_get_text (text_layer);

      if (text && text->box_mode == GIMP_TEXT_BOX_DYNAMIC &&
          (! text->text || text->text[0] == '\0') &&
          (! text->markup || text->markup[0] == '\0'))
        {
          GimpImage *image = gimp_item_get_image (GIMP_ITEM (text_layer));

          if (text_tool->image == image)
            g_signal_handlers_block_by_func (image,
                                             gimp_text_tool_layer_changed,
                                             text_tool);

          gimp_image_remove_layer (image, GIMP_LAYER (text_layer), TRUE, NULL);
          gimp_image_flush (image);

          if (text_tool->image == image)
            g_signal_handlers_unblock_by_func (image,
                                               gimp_text_tool_layer_changed,
                                               text_tool);
        }
    }
}

static void
gimp_text_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_text_tool_halt (text_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
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
  GimpImage         *image     = gimp_display_get_image (display);
  GimpText          *text      = text_tool->text;
  GimpToolRectangle *rectangle;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (tool->display && tool->display != display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  if (! text_tool->widget)
    {
      GError *error = NULL;

      if (! gimp_text_tool_start (text_tool, display, NULL, &error))
        {
          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

          gimp_tool_message_literal (tool, display, error->message);

          g_clear_error (&error);

          return;
        }

      gimp_tool_widget_hover (text_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      gimp_tool_rectangle_set_function (GIMP_TOOL_RECTANGLE (text_tool->widget),
                                        GIMP_TOOL_RECTANGLE_CREATING);
    }

  rectangle = GIMP_TOOL_RECTANGLE (text_tool->widget);

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      gimp_tool_control_activate (tool->control);

      /* clicking anywhere while a preedit is going on aborts the
       * preedit, this is ugly but at least leaves everything in
       * a consistent state
       */
      if (text_tool->preedit_active)
        gimp_text_tool_abort_im_context (text_tool);
      else
        gimp_text_tool_reset_im_context (text_tool);

      text_tool->selecting = FALSE;

      if (gimp_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          gimp_tool_rectangle_set_function (rectangle,
                                            GIMP_TOOL_RECTANGLE_DEAD);
        }
      else if (gimp_tool_widget_button_press (text_tool->widget, coords,
                                              time, state, press_type))
        {
          text_tool->grab_widget = text_tool->widget;
        }

      /*  bail out now if the user user clicked on a handle of an
       *  existing rectangle, but not inside an existing framed layer
       */
      if (gimp_tool_rectangle_get_function (rectangle) !=
          GIMP_TOOL_RECTANGLE_CREATING)
        {
          if (text_tool->layer)
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
          else
            {
              gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
              return;
            }
        }

      /* if the the click is not related to the currently edited text
       * layer in any way, try to pick a text layer
       */
      if (! text_tool->moving &&
          gimp_tool_rectangle_get_function (rectangle) ==
          GIMP_TOOL_RECTANGLE_CREATING)
        {
          GimpTextLayer *text_layer;

          text_layer = gimp_image_pick_text_layer (image, coords->x, coords->y);

          if (text_layer && text_layer != text_tool->layer)
            {
              GList *selection = g_list_prepend (NULL, text_layer);

              if (text_tool->image == image)
                g_signal_handlers_block_by_func (image,
                                                 gimp_text_tool_layer_changed,
                                                 text_tool);

              gimp_image_set_selected_layers (image, selection);
              g_list_free (selection);

              if (text_tool->image == image)
                g_signal_handlers_unblock_by_func (image,
                                                   gimp_text_tool_layer_changed,
                                                   text_tool);
            }
        }
    }

  if (gimp_image_coords_in_active_pickable (image, coords, FALSE, FALSE, FALSE))
    {
      GList        *drawables = gimp_image_get_selected_drawables (image);
      GimpDrawable *drawable  = NULL;
      gdouble       x         = coords->x;
      gdouble       y         = coords->y;

      if (g_list_length (drawables) == 1)
        {
          GimpItem *item = GIMP_ITEM (drawables->data);

          x = coords->x - gimp_item_get_offset_x (item);
          y = coords->y - gimp_item_get_offset_y (item);

          drawable = drawables->data;
        }
      g_list_free (drawables);

      /*  did the user click on a text layer?  */
      if (drawable &&
          gimp_text_tool_set_drawable (text_tool, drawable, TRUE))
        {
          if (press_type == GIMP_BUTTON_PRESS_NORMAL)
            {
              /*  if we clicked on a text layer while the tool was idle
               *  (didn't show a rectangle), frame the layer and switch to
               *  selecting instead of drawing a new rectangle
               */
              if (gimp_tool_rectangle_get_function (rectangle) ==
                  GIMP_TOOL_RECTANGLE_CREATING)
                {
                  gimp_tool_rectangle_set_function (rectangle,
                                                    GIMP_TOOL_RECTANGLE_DEAD);

                  gimp_text_tool_frame_item (text_tool);
                }

              if (text_tool->text && text_tool->text != text)
                {
                  gimp_text_tool_editor_start (text_tool);
                }
            }

          if (text_tool->text && ! text_tool->moving)
            {
              text_tool->selecting = TRUE;

              gimp_text_tool_editor_button_press (text_tool, x, y, press_type);
            }
          else
            {
              text_tool->selecting = FALSE;
            }

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

          return;
        }
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      /*  create a new text layer  */
      text_tool->text_box_fixed = FALSE;

      /* make sure the text tool has an image, even if the user didn't click
       * inside the active drawable, in particular, so that the text style
       * editor picks the correct resolution.
       */
      gimp_text_tool_set_image (text_tool, image);

      gimp_text_tool_connect (text_tool, NULL, NULL);
      gimp_text_tool_editor_start (text_tool);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_text_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpToolRectangle *rectangle = GIMP_TOOL_RECTANGLE (text_tool->widget);

  gimp_tool_control_halt (tool->control);

  if (text_tool->selecting)
    {
      /*  we are in a selection process (user has initially clicked on
       *  an existing text layer), so finish the selection process and
       *  ignore rectangle-change-complete.
       */

      /*  need to block "end-user-action" on the text buffer, because
       *  GtkTextBuffer considers copying text to the clipboard an
       *  undo-relevant user action, which is clearly a bug, but what
       *  can we do...
       */
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_end_edit,
                                       text_tool);

      gimp_text_tool_editor_button_release (text_tool);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_begin_edit,
                                         text_tool);

      text_tool->selecting = FALSE;

      text_tool->handle_rectangle_change_complete = FALSE;

      /*  there is no cancelling of selections yet  */
      if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
        release_type = GIMP_BUTTON_RELEASE_NORMAL;
    }
  else if (text_tool->moving)
    {
      /*  the user has moved the text layer with Alt-drag, fall
       *  through and let rectangle-change-complete do its job of
       *  setting text layer's new position.
       */
    }
  else if (gimp_tool_rectangle_get_function (rectangle) ==
           GIMP_TOOL_RECTANGLE_DEAD)
    {
      /*  the user clicked in dead space (like between the corner and
       *  edge handles, so completely ignore that.
       */

      text_tool->handle_rectangle_change_complete = FALSE;
    }
  else if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /*  user has canceled the rectangle resizing, fall through
       *  and let the rectangle handle restoring the previous size
       */
    }
  else
    {
      gdouble x1, y1;
      gdouble x2, y2;

      /*  otherwise the user has clicked outside of any text layer in
       *  order to create a new text, fall through and let
       *  rectangle-change-complete do its job of setting the new text
       *  layer's size.
       */

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      if (release_type == GIMP_BUTTON_RELEASE_CLICK ||
          (x2 - x1) < 3                             ||
          (y2 - y1) < 3)
        {
          /*  unless the rectangle is unreasonably small to hold any
           *  real text (the user has eitherjust clicked or just made
           *  a rectangle of a few pixels), so set the text box to
           *  dynamic and ignore rectangle-change-complete.
           */

          g_object_set (text_tool->proxy,
                        "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                        NULL);

          text_tool->handle_rectangle_change_complete = FALSE;
        }
    }

  if (text_tool->grab_widget)
    {
      gimp_tool_widget_button_release (text_tool->grab_widget,
                                       coords, time, state, release_type);
      text_tool->grab_widget = NULL;
    }

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
gimp_text_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  if (! text_tool->selecting)
    {
      if (text_tool->grab_widget)
        {
          gimp_tool_widget_motion (text_tool->grab_widget,
                                   coords, time, state);
        }
    }
  else
    {
      GimpItem *item = GIMP_ITEM (text_tool->layer);
      gdouble   x    = coords->x - gimp_item_get_offset_x (item);
      gdouble   y    = coords->y - gimp_item_get_offset_y (item);

      gimp_text_tool_editor_motion (text_tool, x, y);
    }
}

static gboolean
gimp_text_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  if (display == tool->display)
    return gimp_text_tool_editor_key_press (text_tool, kevent);

  return FALSE;
}

static gboolean
gimp_text_tool_key_release (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (tool);

  if (display == tool->display)
    return gimp_text_tool_editor_key_release (text_tool, kevent);

  return FALSE;
}

static void
gimp_text_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpToolRectangle *rectangle = GIMP_TOOL_RECTANGLE (text_tool->widget);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  text_tool->moving = (text_tool->widget &&
                       gimp_tool_rectangle_get_function (rectangle) ==
                       GIMP_TOOL_RECTANGLE_MOVING &&
                       (state & GDK_MOD1_MASK));
}

static void
gimp_text_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpToolRectangle *rectangle = GIMP_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle && tool->display == display)
    {
      if (gimp_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          gimp_tool_set_cursor (tool, display,
                                (GimpCursorType) GDK_XTERM,
                                gimp_tool_control_get_tool_cursor (tool->control),
                                GIMP_CURSOR_MODIFIER_NONE);
        }
      else
        {
          GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
    }
  else
    {
      GimpAsyncSet *async_set;

      async_set =
        gimp_data_factory_get_async_set (tool->tool_info->gimp->font_factory);

      if (gimp_async_set_is_empty (async_set))
        {
          GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
      else
        {
          gimp_tool_set_cursor (tool, display,
                                gimp_tool_control_get_cursor (tool->control),
                                gimp_tool_control_get_tool_cursor (tool->control),
                                GIMP_CURSOR_MODIFIER_BAD);
        }
    }
}

static GimpUIManager *
gimp_text_tool_get_popup (GimpTool         *tool,
                          const GimpCoords *coords,
                          GdkModifierType   state,
                          GimpDisplay      *display,
                          const gchar     **ui_path)
{
  GimpTextTool      *text_tool = GIMP_TEXT_TOOL (tool);
  GimpToolRectangle *rectangle = GIMP_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle &&
      gimp_tool_rectangle_point_in_rectangle (rectangle,
                                              coords->x,
                                              coords->y))
    {
      GimpMenuFactory *menu_factory;
      GimpUIManager   *ui_manager;

      menu_factory = menus_get_global_menu_factory (tool->tool_info->gimp);
      ui_manager   = gimp_menu_factory_get_manager (menu_factory, "<TextTool>", text_tool);

      gimp_ui_manager_update (ui_manager, text_tool);

      *ui_path = "/text-tool-popup";

      return ui_manager;
    }

  return NULL;
}

static void
gimp_text_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTextTool *text_tool = GIMP_TEXT_TOOL (draw_tool);

  GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (! text_tool->text  ||
      ! text_tool->layer ||
      ! text_tool->layer->text)
    {
      gimp_text_tool_editor_update_im_cursor (text_tool);

      return;
    }

  gimp_text_tool_ensure_layout (text_tool);

  if (gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (text_tool->buffer)))
    {
      /* If the text buffer has a selection, highlight the selected letters */

      gimp_text_tool_draw_selection (draw_tool);
    }
  else
    {
      /* If the text buffer has no selection, draw the text cursor */

      GimpCanvasItem   *item;
      PangoRectangle    cursor_rect;
      gint              off_x, off_y;
      gboolean          overwrite;
      GimpTextDirection direction;

      gimp_text_tool_editor_get_cursor_rect (text_tool,
                                             text_tool->overwrite_mode,
                                             &cursor_rect);

      gimp_item_get_offset (GIMP_ITEM (text_tool->layer), &off_x, &off_y);
      cursor_rect.x += off_x;
      cursor_rect.y += off_y;

      overwrite = text_tool->overwrite_mode && cursor_rect.width != 0;

      direction = gimp_text_tool_get_direction (text_tool);

      item = gimp_draw_tool_add_text_cursor (draw_tool, &cursor_rect,
                                             overwrite, direction);
      gimp_canvas_item_set_highlight (item, TRUE);
    }

  gimp_text_tool_editor_update_im_cursor (text_tool);
}

static void
gimp_text_tool_draw_selection (GimpDrawTool *draw_tool)
{
  GimpTextTool     *text_tool = GIMP_TEXT_TOOL (draw_tool);
  GtkTextBuffer    *buffer    = GTK_TEXT_BUFFER (text_tool->buffer);
  GimpCanvasGroup  *group;
  PangoLayout      *layout;
  gint              offset_x;
  gint              offset_y;
  gint              width;
  gint              height;
  gint              off_x, off_y;
  PangoLayoutIter  *iter;
  GtkTextIter       sel_start, sel_end;
  gint              min, max;
  gint              i;
  GimpTextDirection direction;

  group = gimp_draw_tool_add_stroke_group (draw_tool);
  gimp_canvas_item_set_highlight (GIMP_CANVAS_ITEM (group), TRUE);

  gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

  min = gimp_text_buffer_get_iter_index (text_tool->buffer, &sel_start, TRUE);
  max = gimp_text_buffer_get_iter_index (text_tool->buffer, &sel_end, TRUE);

  layout = gimp_text_layout_get_pango_layout (text_tool->layout);

  gimp_text_layout_get_offsets (text_tool->layout, &offset_x, &offset_y);

  gimp_text_layout_get_size (text_tool->layout, &width, &height);

  gimp_item_get_offset (GIMP_ITEM (text_tool->layer), &off_x, &off_y);
  offset_x += off_x;
  offset_y += off_y;

  direction = gimp_text_tool_get_direction (text_tool);

  iter = pango_layout_get_iter (layout);

  gimp_draw_tool_push_group (draw_tool, group);

  do
    {
      if (! pango_layout_iter_get_run (iter))
        continue;

      i = pango_layout_iter_get_index (iter);

      if (i >= min && i < max)
        {
          PangoRectangle rect;
          gint           ytop, ybottom;

          pango_layout_iter_get_char_extents (iter, &rect);
          pango_layout_iter_get_line_yrange (iter, &ytop, &ybottom);

          rect.y      = ytop;
          rect.height = ybottom - ytop;

          pango_extents_to_pixels (&rect, NULL);

          gimp_text_layout_transform_rect (text_tool->layout, &rect);

          switch (direction)
            {
            case GIMP_TEXT_DIRECTION_LTR:
            case GIMP_TEXT_DIRECTION_RTL:
              rect.x += offset_x;
              rect.y += offset_y;
              gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.x, rect.y,
                                            rect.width, rect.height);
              break;
            case GIMP_TEXT_DIRECTION_TTB_RTL:
            case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              rect.y = offset_x - rect.y + width;
              rect.x = offset_y + rect.x;
              gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            -rect.height, rect.width);
              break;
            case GIMP_TEXT_DIRECTION_TTB_LTR:
            case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              rect.y = offset_x + rect.y;
              rect.x = offset_y - rect.x + height;
              gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            rect.height, -rect.width);
              break;
            }
        }
    }
  while (pango_layout_iter_next_char (iter));

  gimp_draw_tool_pop_group (draw_tool);

  pango_layout_iter_free (iter);
}

static gboolean
gimp_text_tool_start (GimpTextTool  *text_tool,
                      GimpDisplay   *display,
                      GimpLayer     *layer,
                      GError       **error)
{
  GimpTool         *tool  = GIMP_TOOL (text_tool);
  GimpDisplayShell *shell = gimp_display_get_shell (display);
  GimpToolWidget   *widget;
  GimpAsyncSet     *async_set;

  async_set =
    gimp_data_factory_get_async_set (tool->tool_info->gimp->font_factory);

  if (! gimp_async_set_is_empty (async_set))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Fonts are still loading"));

      return FALSE;
    }

  tool->display = display;

  text_tool->widget = widget = gimp_tool_rectangle_new (shell);

  g_object_set (widget,
                "force-narrow-mode", TRUE,
                "status-title",      _("Text box: "),
                NULL);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

  g_signal_connect (widget, "response",
                    G_CALLBACK (gimp_text_tool_rectangle_response),
                    text_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (gimp_text_tool_rectangle_change_complete),
                    text_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  if (layer)
    {
      gimp_text_tool_frame_item (text_tool);
      gimp_text_tool_editor_start (text_tool);
      gimp_text_tool_editor_position (text_tool);
    }

  return TRUE;
}

static void
gimp_text_tool_halt (GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  gimp_text_tool_editor_halt (text_tool);
  gimp_text_tool_clear_layout (text_tool);
  gimp_text_tool_set_drawable (text_tool, NULL, FALSE);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&text_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
gimp_text_tool_frame_item (GimpTextTool *text_tool)
{
  g_return_if_fail (GIMP_IS_LAYER (text_tool->layer));

  text_tool->handle_rectangle_change_complete = FALSE;

  gimp_tool_rectangle_frame_item (GIMP_TOOL_RECTANGLE (text_tool->widget),
                                  GIMP_ITEM (text_tool->layer));

  /* Update crop of any filters applied to text */
  if (text_tool->layer)
  {
    GList         *list;
    GimpDrawable  *drawable = GIMP_DRAWABLE (text_tool->layer);
    GimpContainer *filters  = gimp_drawable_get_filters (drawable);

    for (list = GIMP_LIST (filters)->queue->tail;
         list; list = g_list_previous (list))
      {
        GimpDrawableFilter *filter = list->data;

        /* TODO: Handle partial layer effect */
        gimp_drawable_filter_set_region (filter, GIMP_FILTER_REGION_SELECTION);
        gimp_drawable_filter_set_region (filter, GIMP_FILTER_REGION_DRAWABLE);
      }
    if (list)
      g_list_free (list);
  }

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
gimp_text_tool_rectangle_response (GimpToolRectangle *rectangle,
                                   gint               response_id,
                                   GimpTextTool      *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  /* this happens when a newly created rectangle gets canceled,
   * we have to shut down the tool
   */
  if (response_id == GIMP_TOOL_WIDGET_RESPONSE_CANCEL)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
}

static void
gimp_text_tool_rectangle_change_complete (GimpToolRectangle *rectangle,
                                          GimpTextTool      *text_tool)
{
  gimp_text_tool_editor_position (text_tool);

  if (text_tool->handle_rectangle_change_complete)
    {
      GimpItem *item = GIMP_ITEM (text_tool->layer);
      gdouble   x1, y1;
      gdouble   x2, y2;

      if (! item)
        {
          /* we can't set properties for the text layer, because it
           * isn't created until some text has been inserted, so we
           * need to make a special note that will remind us what to
           * do when we actually create the layer
           */
          text_tool->text_box_fixed = TRUE;

          return;
        }

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      if ((x2 - x1) != gimp_item_get_width  (item) ||
          (y2 - y1) != gimp_item_get_height (item))
        {
          GimpUnit *box_unit = text_tool->proxy->box_unit;
          gdouble   xres, yres;
          gboolean  push_undo = TRUE;
          GimpUndo *undo;

          gimp_image_get_resolution (text_tool->image, &xres, &yres);

          g_object_set (text_tool->proxy,
                        "box-mode",   GIMP_TEXT_BOX_FIXED,
                        "box-width",  gimp_pixels_to_units (x2 - x1,
                                                            box_unit, xres),
                        "box-height", gimp_pixels_to_units (y2 - y1,
                                                            box_unit, yres),
                        NULL);

          undo = gimp_image_undo_can_compress (text_tool->image,
                                               GIMP_TYPE_UNDO_STACK,
                                               GIMP_UNDO_GROUP_TEXT);

          if (undo &&
              gimp_undo_get_age (undo) <= TEXT_UNDO_TIMEOUT &&
              g_object_get_data (G_OBJECT (undo), "reshape-text-layer") == (gpointer) item)
            push_undo = FALSE;

          if (push_undo)
            {
              gimp_image_undo_group_start (text_tool->image, GIMP_UNDO_GROUP_TEXT,
                                           _("Reshape Text Layer"));

              undo = gimp_image_undo_can_compress (text_tool->image, GIMP_TYPE_UNDO_STACK,
                                                   GIMP_UNDO_GROUP_TEXT);

              if (undo)
                g_object_set_data (G_OBJECT (undo), "reshape-text-layer",
                                   (gpointer) item);
            }

          gimp_text_tool_block_drawing (text_tool);

          gimp_item_translate (item,
                               x1 - gimp_item_get_offset_x (item),
                               y1 - gimp_item_get_offset_y (item),
                               push_undo);
          gimp_text_tool_apply (text_tool, push_undo);

          gimp_text_tool_unblock_drawing (text_tool);

          if (push_undo)
            gimp_image_undo_group_end (text_tool->image);
        }
      else if (x1 != gimp_item_get_offset_x (item) ||
               y1 != gimp_item_get_offset_y (item))
        {
          gimp_text_tool_block_drawing (text_tool);

          gimp_text_tool_apply (text_tool, TRUE);

          gimp_item_translate (item,
                               x1 - gimp_item_get_offset_x (item),
                               y1 - gimp_item_get_offset_y (item),
                               TRUE);

          gimp_text_tool_unblock_drawing (text_tool);

          gimp_image_flush (text_tool->image);
        }
    }
}

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

      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_end_edit,
                                       text_tool);

      if (text_tool->text)
        {
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                gimp_text_tool_text_notify,
                                                text_tool);
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                gimp_text_tool_text_changed,
                                                text_tool);

          if (text_tool->pending)
            gimp_text_tool_apply (text_tool, TRUE);

          g_clear_object (&text_tool->text);

          g_object_set (text_tool->proxy,
                        "text",   NULL,
                        "markup", NULL,
                        NULL);
          gimp_text_buffer_set_text (text_tool->buffer, NULL);

          gimp_text_tool_clear_layout (text_tool);
        }

      gimp_context_define_property (GIMP_CONTEXT (options),
                                    GIMP_CONTEXT_PROP_FOREGROUND,
                                    text != NULL);

      if (text)
        {
          if (text->unit != text_tool->proxy->unit)
            gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_entry),
                                      text->unit);

          gimp_config_sync (G_OBJECT (text), G_OBJECT (text_tool->proxy), 0);

          if (text->markup)
            gimp_text_buffer_set_markup (text_tool->buffer, text->markup);
          else
            gimp_text_buffer_set_text (text_tool->buffer, text->text);

          gimp_text_tool_clear_layout (text_tool);

          text_tool->text = g_object_ref (text);

          g_signal_connect (text, "notify",
                            G_CALLBACK (gimp_text_tool_text_notify),
                            text_tool);
          g_signal_connect (text, "changed",
                            G_CALLBACK (gimp_text_tool_text_changed),
                            text_tool);
        }

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  if (text_tool->layer != layer)
    {
      if (text_tool->layer)
        {
          g_signal_handlers_disconnect_by_func (text_tool->layer,
                                                gimp_text_tool_layer_notify,
                                                text_tool);

          /*  don't try to remove the layer if it is not attached,
           *  which can happen if we got here because the layer was
           *  somehow deleted from the image (like by the user in the
           *  layers dialog).
           */
          if (gimp_item_is_attached (GIMP_ITEM (text_tool->layer)))
            gimp_text_tool_remove_empty_text_layer (text_tool);
        }

      text_tool->layer = layer;

      if (layer)
        {
          g_signal_connect_object (text_tool->layer, "notify",
                                   G_CALLBACK (gimp_text_tool_layer_notify),
                                   text_tool, 0);
        }
    }
}

static void
gimp_text_tool_layer_notify (GimpTextLayer    *layer,
                             const GParamSpec *pspec,
                             GimpTextTool     *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  if (! strcmp (pspec->name, "modified"))
    {
      if (layer->modified)
        gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "text"))
    {
      if (! layer->text)
        gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "offset-x") ||
           ! strcmp (pspec->name, "offset-y"))
    {
      if (gimp_item_is_attached (GIMP_ITEM (layer)))
        {
          gimp_text_tool_block_drawing (text_tool);

          gimp_text_tool_frame_item (text_tool);

          gimp_text_tool_unblock_drawing (text_tool);
        }
    }
}

static gboolean
gimp_text_tool_apply_idle (GimpTextTool *text_tool)
{
  text_tool->idle_id = 0;

  gimp_text_tool_apply (text_tool, TRUE);

  gimp_text_tool_unblock_drawing (text_tool);

  return G_SOURCE_REMOVE;
}

static void
gimp_text_tool_proxy_notify (GimpText         *text,
                             const GParamSpec *pspec,
                             GimpTextTool     *text_tool)
{
  if (! text_tool->text)
    return;

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE &&
      pspec->owner_type == GIMP_TYPE_TEXT)
    {
      if (text_tool->preedit_active)
        {
          /* if there is a preedit going on, don't queue pending
           * changes to be idle-applied with undo; instead, flush the
           * pending queue (happens only when preedit starts), and
           * apply the changes to text_tool->text directly. Preedit
           * will *always* end by removing the preedit string, and if
           * the preedit was committed, it will insert the resulting
           * text, which will not trigger this if() any more.
           */

          GList *list = NULL;

         /* if there are pending changes, apply them before applying
           * preedit stuff directly (bypassing undo)
           */
          if (text_tool->pending)
            {
              gimp_text_tool_block_drawing (text_tool);
              gimp_text_tool_apply (text_tool, TRUE);
              gimp_text_tool_unblock_drawing (text_tool);
            }

          gimp_text_tool_block_drawing (text_tool);

          list = g_list_append (list, (gpointer) pspec);
          gimp_text_tool_apply_list (text_tool, list);
          g_list_free (list);

          gimp_text_tool_frame_item (text_tool);

          gimp_image_flush (gimp_item_get_image (GIMP_ITEM (text_tool->layer)));

          gimp_text_tool_unblock_drawing (text_tool);
        }
      else
        {
          /* else queue the property change for normal processing,
           * including undo
           */

          text_tool->pending = g_list_append (text_tool->pending,
                                              (gpointer) pspec);

          if (! text_tool->idle_id)
            {
              gimp_text_tool_block_drawing (text_tool);

              text_tool->idle_id =
                g_idle_add_full (G_PRIORITY_LOW,
                                 (GSourceFunc) gimp_text_tool_apply_idle,
                                 text_tool,
                                 NULL);
            }
        }
    }
}

static void
gimp_text_tool_text_notify (GimpText         *text,
                            const GParamSpec *pspec,
                            GimpTextTool     *text_tool)
{
  g_return_if_fail (text == text_tool->text);

  /* an undo cancels all preedit operations */
  if (text_tool->preedit_active)
    gimp_text_tool_abort_im_context (text_tool);

  gimp_text_tool_block_drawing (text_tool);

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      GValue value = G_VALUE_INIT;

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

  /* if the text has changed, (probably because of an undo), we put
   * the new text into the text buffer
   */
  if (strcmp (pspec->name, "text")   == 0 ||
      strcmp (pspec->name, "markup") == 0)
    {
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       gimp_text_tool_buffer_end_edit,
                                       text_tool);

      if (text->markup)
        gimp_text_buffer_set_markup (text_tool->buffer, text->markup);
      else
        gimp_text_buffer_set_text (text_tool->buffer, text->text);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         gimp_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  gimp_text_tool_unblock_drawing (text_tool);
}

static void
gimp_text_tool_text_changed (GimpText     *text,
                             GimpTextTool *text_tool)
{
  gimp_text_tool_block_drawing (text_tool);

  /* we need to redraw the rectangle in any case because whatever
   * changes to the text can change its size
   */
  gimp_text_tool_frame_item (text_tool);

  gimp_text_tool_unblock_drawing (text_tool);
}

static void
gimp_text_tool_fonts_async_set_empty_notify (GimpAsyncSet *async_set,
                                             GParamSpec   *pspec,
                                             GimpTextTool *text_tool)
{
  GimpTool *tool = GIMP_TOOL (text_tool);

  if (! gimp_async_set_is_empty (async_set) && tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
}

static void
gimp_text_tool_apply_list (GimpTextTool *text_tool,
                           GList        *pspecs)
{
  GObject *src  = G_OBJECT (text_tool->proxy);
  GObject *dest = G_OBJECT (text_tool->text);
  GList   *list;

  g_signal_handlers_block_by_func (dest,
                                   gimp_text_tool_text_notify,
                                   text_tool);
  g_signal_handlers_block_by_func (dest,
                                   gimp_text_tool_text_changed,
                                   text_tool);

  g_object_freeze_notify (dest);

  for (list = pspecs; list; list = g_list_next (list))
    {
      const GParamSpec *pspec;
      GValue            value = G_VALUE_INIT;

      /*  look ahead and compress changes  */
      if (list->next && list->next->data == list->data)
        continue;

      pspec = list->data;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (src,  pspec->name, &value);
      g_object_set_property (dest, pspec->name, &value);

      g_value_unset (&value);
    }

  g_object_thaw_notify (dest);

  g_signal_handlers_unblock_by_func (dest,
                                     gimp_text_tool_text_notify,
                                     text_tool);
  g_signal_handlers_unblock_by_func (dest,
                                     gimp_text_tool_text_changed,
                                     text_tool);
}

static void
gimp_text_tool_create_layer (GimpTextTool *text_tool,
                             GimpText     *text)
{
  GimpTool  *tool  = GIMP_TOOL (text_tool);
  GimpImage *image = gimp_display_get_image (tool->display);
  GimpLayer *layer;
  gdouble    x1, y1;
  gdouble    x2, y2;

  gimp_text_tool_block_drawing (text_tool);

  if (text)
    {
      text = gimp_config_duplicate (GIMP_CONFIG (text));
    }
  else
    {
      gchar *string;

      if (gimp_text_buffer_has_markup (text_tool->buffer))
        {
          string = gimp_text_buffer_get_markup (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "markup",   string,
                        "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                        NULL);
        }
      else
        {
          string = gimp_text_buffer_get_text (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "text",     string,
                        "box-mode", GIMP_TEXT_BOX_DYNAMIC,
                        NULL);
        }

      g_free (string);

      text = gimp_config_duplicate (GIMP_CONFIG (text_tool->proxy));
    }

  layer = gimp_text_layer_new (image, text);

  g_object_unref (text);

  if (! layer)
    {
      gimp_text_tool_unblock_drawing (text_tool);
      return;
    }

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

  g_object_get (text_tool->widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (text_tool->text_box_fixed == FALSE)
    {
      if (text_tool->text &&
          (text_tool->text->base_dir == GIMP_TEXT_DIRECTION_TTB_RTL ||
           text_tool->text->base_dir == GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          x1 -= gimp_item_get_width (GIMP_ITEM (layer));
        }
    }
  gimp_item_set_offset (GIMP_ITEM (layer), x1, y1);

  gimp_image_add_layer (image, layer,
                        GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  if (text_tool->text_box_fixed)
    {
      GimpUnit *box_unit = text_tool->proxy->box_unit;
      gdouble   xres, yres;

      gimp_image_get_resolution (image, &xres, &yres);

      g_object_set (text_tool->proxy,
                    "box-mode",   GIMP_TEXT_BOX_FIXED,
                    "box-width",  gimp_pixels_to_units (x2 - x1,
                                                        box_unit, xres),
                    "box-height", gimp_pixels_to_units (y2 - y1,
                                                        box_unit, yres),
                    NULL);

      gimp_text_tool_apply (text_tool, TRUE); /* unblocks drawing */
    }
  else
    {
      gimp_text_tool_frame_item (text_tool);
    }

  gimp_image_undo_group_end (image);

  gimp_image_flush (image);

  gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer), FALSE);

  gimp_text_tool_unblock_drawing (text_tool);
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
          g_object_notify (G_OBJECT (text_tool->proxy), "markup");

          gimp_text_tool_editor_start (text_tool);
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

  dialog = gimp_viewable_dialog_new (g_list_prepend (NULL, text_tool->layer),
                                     GIMP_CONTEXT (gimp_tool_get_options (tool)),
                                     _("Confirm Text Editing"),
                                     "gimp-text-tool-confirm",
                                     GIMP_ICON_LAYER_TEXT_LAYER,
                                     _("Confirm Text Editing"),
                                     GTK_WIDGET (shell),
                                     gimp_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     _("_Cancel"),           GTK_RESPONSE_CANCEL,
                                     _("_Edit"),             GTK_RESPONSE_ACCEPT,

                                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_NEW,
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_text_tool_confirm_response),
                    text_tool);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
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
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
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
  GList *layers = gimp_image_get_selected_layers (image);

  if (g_list_length (layers) != 1 || layers->data != text_tool->layer)
    {
      GimpTool    *tool    = GIMP_TOOL (text_tool);
      GimpDisplay *display = tool->display;

      if (display)
        {
          GimpLayer *layer = NULL;

          gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

          if (g_list_length (layers) == 1)
            layer = layers->data;

          /* The tool can only be started when a single layer is
           * selected and this is a text layer.
           */
          if (layer &&
              gimp_text_tool_set_drawable (text_tool, GIMP_DRAWABLE (layer),
                                           FALSE) &&
              GIMP_LAYER (text_tool->layer) == layer)
            {
              GError *error = NULL;

              if (! gimp_text_tool_start (text_tool, display, layer, &error))
                {
                  gimp_text_tool_set_drawable (text_tool, NULL, FALSE);

                  gimp_tool_message_literal (tool, display, error->message);

                  g_clear_error (&error);

                  return;
                }
            }
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
    }

  g_set_weak_pointer (&text_tool->image, image);

  if (image)
    {
      GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
      gdouble          xres;
      gdouble          yres;

      g_signal_connect_object (text_tool->image, "selected-layers-changed",
                               G_CALLBACK (gimp_text_tool_layer_changed),
                               text_tool, 0);

      gimp_image_get_resolution (image, &xres, &yres);
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
gimp_text_tool_block_drawing (GimpTextTool *text_tool)
{
  if (text_tool->drawing_blocked == 0)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (text_tool));

      gimp_text_tool_clear_layout (text_tool);
    }

  text_tool->drawing_blocked++;
}

static void
gimp_text_tool_unblock_drawing (GimpTextTool *text_tool)
{
  g_return_if_fail (text_tool->drawing_blocked > 0);

  text_tool->drawing_blocked--;

  if (text_tool->drawing_blocked == 0)
    gimp_draw_tool_resume (GIMP_DRAW_TOOL (text_tool));
}

static void
gimp_text_tool_buffer_begin_edit (GimpTextBuffer *buffer,
                                  GimpTextTool   *text_tool)
{
  gimp_text_tool_block_drawing (text_tool);
}

static void
gimp_text_tool_buffer_end_edit (GimpTextBuffer *buffer,
                                GimpTextTool   *text_tool)
{
  if (text_tool->text)
    {
      gchar *string;

      if (gimp_text_buffer_has_markup (buffer))
        {
          string = gimp_text_buffer_get_markup (buffer);

          g_object_set (text_tool->proxy,
                        "markup", string,
                        NULL);
        }
      else
        {
          string = gimp_text_buffer_get_text (buffer);

          g_object_set (text_tool->proxy,
                        "text", string,
                        NULL);
        }

      g_free (string);
    }
  else
    {
      gimp_text_tool_create_layer (text_tool, NULL);
    }

  gimp_text_tool_unblock_drawing (text_tool);
}

static void
gimp_text_tool_buffer_color_applied (GimpTextBuffer *buffer,
                                     GeglColor      *color,
                                     GimpTextTool   *text_tool)
{
  gimp_palettes_add_color_history (GIMP_TOOL (text_tool)->tool_info->gimp, color);
}


/*  public functions  */

void
gimp_text_tool_clear_layout (GimpTextTool *text_tool)
{
  g_clear_object (&text_tool->layout);
}

gboolean
gimp_text_tool_ensure_layout (GimpTextTool *text_tool)
{
  if (! text_tool->layout && text_tool->text)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (text_tool->layer));
      gdouble    xres;
      gdouble    yres;
      GError    *error = NULL;

      gimp_image_get_resolution (image, &xres, &yres);

      text_tool->layout = gimp_text_layout_new (text_tool->layer->text,
                                                image, xres, yres, &error);
      if (error)
        {
          gimp_message_literal (image->gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  return text_tool->layout != NULL;
}

void
gimp_text_tool_apply (GimpTextTool *text_tool,
                      gboolean      push_undo)
{
  const GParamSpec *pspec = NULL;
  GimpImage        *image;
  GimpTextLayer    *layer;
  GList            *list;
  gboolean          undo_group = FALSE;

  if (text_tool->idle_id)
    {
      g_source_remove (text_tool->idle_id);
      text_tool->idle_id = 0;

      gimp_text_tool_unblock_drawing (text_tool);
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

          /*  see comment in gimp_text_layer_set()  */
          gimp_image_undo_push_drawable_mod (image, NULL,
                                             GIMP_DRAWABLE (layer), TRUE);
        }

      if (pspec)
        gimp_image_undo_push_text_layer (image, NULL, layer, pspec);
    }

  gimp_text_tool_apply_list (text_tool, list);

  g_list_free (text_tool->pending);
  text_tool->pending = NULL;

  if (push_undo)
    {
      g_object_set (layer, "modified", FALSE, NULL);

      if (undo_group)
        gimp_image_undo_group_end (image);
    }

  gimp_text_tool_frame_item (text_tool);

  gimp_image_flush (image);
}

gboolean
gimp_text_tool_set_layer (GimpTextTool *text_tool,
                          GimpLayer    *layer)
{
  g_return_val_if_fail (GIMP_IS_TEXT_TOOL (text_tool), FALSE);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), FALSE);

  if (layer == GIMP_LAYER (text_tool->layer))
    return TRUE;

  /*  FIXME this function works, and I have no clue why: first we set
   *  the drawable, then we HALT the tool and start() it without
   *  re-setting the drawable. Why this works perfectly anyway when
   *  double clicking a text layer in the layers dialog... no idea.
   */
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

      if (tool->display)
        gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

      if (display)
        {
          GError *error = NULL;

          if (! gimp_text_tool_start (text_tool, display, layer, &error))
            {
              gimp_text_tool_set_drawable (text_tool, NULL, FALSE);

              gimp_tool_message_literal (tool, display, error->message);

              g_clear_error (&error);

              return FALSE;
            }

          g_list_free (tool->drawables);
          tool->drawables = g_list_prepend (NULL, GIMP_DRAWABLE (layer));
        }
    }

  return TRUE;
}

gboolean
gimp_text_tool_get_has_text_selection (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  return gtk_text_buffer_get_has_selection (buffer);
}

void
gimp_text_tool_delete_selection (GimpTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
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

  gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                 clipboard, TRUE);
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

  /*  need to block "end-user-action" on the text buffer, because
   *  GtkTextBuffer considers copying text to the clipboard an
   *  undo-relevant user action, which is clearly a bug, but what
   *  can we do...
   */
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   gimp_text_tool_buffer_begin_edit,
                                   text_tool);
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   gimp_text_tool_buffer_end_edit,
                                   text_tool);

  gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                  clipboard);

  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     gimp_text_tool_buffer_end_edit,
                                     text_tool);
  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     gimp_text_tool_buffer_begin_edit,
                                     text_tool);
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

  gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                   clipboard, NULL, TRUE);
}

void
gimp_text_tool_create_vectors (GimpTextTool *text_tool)
{
  GimpPath *path;

  g_return_if_fail (GIMP_IS_TEXT_TOOL (text_tool));

  if (! text_tool->text || ! text_tool->image)
    return;

  path = gimp_text_path_new (text_tool->image, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      gimp_item_get_offset (GIMP_ITEM (text_tool->layer), &x, &y);
      gimp_item_translate (GIMP_ITEM (path), x, y, FALSE);
    }

  gimp_image_add_path (text_tool->image, path,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_flush (text_tool->image);
}

gboolean
gimp_text_tool_create_vectors_warped (GimpTextTool  *text_tool,
                                      GError       **error)
{
  GList             *vectors0;
  GimpPath          *vectors;
  gdouble            box_width;
  gdouble            box_height;
  GimpTextDirection  dir;
  gdouble            offset = 0.0;

  g_return_val_if_fail (GIMP_IS_TEXT_TOOL (text_tool), FALSE);

  if (! text_tool->text || ! text_tool->image || ! text_tool->layer)
    {
      if (! text_tool->text)
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                             _("Text is required."));
      if (! text_tool->image)
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                             _("No image."));
      if (! text_tool->layer)
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                             _("No layer."));
      return FALSE;
    }

  box_width  = gimp_item_get_width  (GIMP_ITEM (text_tool->layer));
  box_height = gimp_item_get_height (GIMP_ITEM (text_tool->layer));

  vectors0 = gimp_image_get_selected_paths (text_tool->image);
  if (g_list_length (vectors0) != 1)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Exactly one path must be selected."));
      return FALSE;
    }

  vectors = gimp_text_path_new (text_tool->image, text_tool->text);

  offset = 0;
  dir = gimp_text_tool_get_direction (text_tool);
  switch (dir)
    {
    case GIMP_TEXT_DIRECTION_LTR:
    case GIMP_TEXT_DIRECTION_RTL:
      offset = 0.5 * box_height;
      break;
    case GIMP_TEXT_DIRECTION_TTB_RTL:
    case GIMP_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
    case GIMP_TEXT_DIRECTION_TTB_LTR:
    case GIMP_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      {
        GimpStroke *stroke = NULL;

        while ((stroke = gimp_path_stroke_get_next (vectors, stroke)))
          {
            gimp_stroke_rotate (stroke, 0, 0, 270);
            gimp_stroke_translate (stroke, 0, box_width);
          }
      }
      offset = 0.5 * box_width;
      break;
    }

  gimp_path_warp_path (vectors0->data, vectors, offset);

  gimp_item_set_visible (GIMP_ITEM (vectors), TRUE, FALSE);

  gimp_image_add_path (text_tool->image, vectors,
                       GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  gimp_image_flush (text_tool->image);

  return TRUE;
}

GimpTextDirection
gimp_text_tool_get_direction  (GimpTextTool *text_tool)
{
  GimpTextOptions *options = GIMP_TEXT_TOOL_GET_OPTIONS (text_tool);
  return options->base_dir;
}
