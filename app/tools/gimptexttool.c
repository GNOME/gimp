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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmaasyncset.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligma-palettes.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmalayer-floating-selection.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaundostack.h"

#include "text/ligmatext.h"
#include "text/ligmatext-vectors.h"
#include "text/ligmatextlayer.h"
#include "text/ligmatextlayout.h"
#include "text/ligmatextundo.h"

#include "vectors/ligmastroke.h"
#include "vectors/ligmavectors.h"
#include "vectors/ligmavectors-warp.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmatextbuffer.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmaviewabledialog.h"

#include "display/ligmacanvasgroup.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolrectangle.h"

#include "ligmatextoptions.h"
#include "ligmatexttool.h"
#include "ligmatexttool-editor.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


#define TEXT_UNDO_TIMEOUT 3


/*  local function prototypes  */

static void      ligma_text_tool_constructed     (GObject           *object);
static void      ligma_text_tool_finalize        (GObject           *object);

static void      ligma_text_tool_control         (LigmaTool          *tool,
                                                 LigmaToolAction     action,
                                                 LigmaDisplay       *display);
static void      ligma_text_tool_button_press    (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 LigmaButtonPressType  press_type,
                                                 LigmaDisplay       *display);
static void      ligma_text_tool_button_release  (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 LigmaButtonReleaseType release_type,
                                                 LigmaDisplay       *display);
static void      ligma_text_tool_motion          (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 LigmaDisplay       *display);
static gboolean  ligma_text_tool_key_press       (LigmaTool          *tool,
                                                 GdkEventKey       *kevent,
                                                 LigmaDisplay       *display);
static gboolean  ligma_text_tool_key_release     (LigmaTool          *tool,
                                                 GdkEventKey       *kevent,
                                                 LigmaDisplay       *display);
static void      ligma_text_tool_oper_update     (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 GdkModifierType    state,
                                                 gboolean           proximity,
                                                 LigmaDisplay       *display);
static void      ligma_text_tool_cursor_update   (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 GdkModifierType    state,
                                                 LigmaDisplay       *display);
static LigmaUIManager * ligma_text_tool_get_popup (LigmaTool          *tool,
                                                 const LigmaCoords  *coords,
                                                 GdkModifierType    state,
                                                 LigmaDisplay       *display,
                                                 const gchar      **ui_path);

static void      ligma_text_tool_draw            (LigmaDrawTool      *draw_tool);
static void      ligma_text_tool_draw_selection  (LigmaDrawTool      *draw_tool);

static gboolean  ligma_text_tool_start           (LigmaTextTool      *text_tool,
                                                 LigmaDisplay       *display,
                                                 LigmaLayer         *layer,
                                                 GError           **error);
static void      ligma_text_tool_halt            (LigmaTextTool      *text_tool);

static void      ligma_text_tool_frame_item      (LigmaTextTool      *text_tool);

static void      ligma_text_tool_rectangle_response
                                                (LigmaToolRectangle *rectangle,
                                                 gint               response_id,
                                                 LigmaTextTool      *text_tool);
static void      ligma_text_tool_rectangle_change_complete
                                                (LigmaToolRectangle *rectangle,
                                                 LigmaTextTool      *text_tool);

static void      ligma_text_tool_connect         (LigmaTextTool      *text_tool,
                                                 LigmaTextLayer     *layer,
                                                 LigmaText          *text);

static void      ligma_text_tool_layer_notify    (LigmaTextLayer     *layer,
                                                 const GParamSpec  *pspec,
                                                 LigmaTextTool      *text_tool);
static void      ligma_text_tool_proxy_notify    (LigmaText          *text,
                                                 const GParamSpec  *pspec,
                                                 LigmaTextTool      *text_tool);

static void      ligma_text_tool_text_notify     (LigmaText          *text,
                                                 const GParamSpec  *pspec,
                                                 LigmaTextTool      *text_tool);
static void      ligma_text_tool_text_changed    (LigmaText          *text,
                                                 LigmaTextTool      *text_tool);

static void
    ligma_text_tool_fonts_async_set_empty_notify (LigmaAsyncSet      *async_set,
                                                 GParamSpec        *pspec,
                                                 LigmaTextTool      *text_tool);

static void      ligma_text_tool_apply_list      (LigmaTextTool      *text_tool,
                                                 GList             *pspecs);

static void      ligma_text_tool_create_layer    (LigmaTextTool      *text_tool,
                                                 LigmaText          *text);

static void      ligma_text_tool_layer_changed   (LigmaImage         *image,
                                                 LigmaTextTool      *text_tool);
static void      ligma_text_tool_set_image       (LigmaTextTool      *text_tool,
                                                 LigmaImage         *image);
static gboolean  ligma_text_tool_set_drawable    (LigmaTextTool      *text_tool,
                                                 LigmaDrawable      *drawable,
                                                 gboolean           confirm);

static void      ligma_text_tool_block_drawing   (LigmaTextTool      *text_tool);
static void      ligma_text_tool_unblock_drawing (LigmaTextTool      *text_tool);

static void    ligma_text_tool_buffer_begin_edit (LigmaTextBuffer    *buffer,
                                                 LigmaTextTool      *text_tool);
static void    ligma_text_tool_buffer_end_edit   (LigmaTextBuffer    *buffer,
                                                 LigmaTextTool      *text_tool);

static void    ligma_text_tool_buffer_color_applied
                                                (LigmaTextBuffer    *buffer,
                                                 const LigmaRGB     *color,
                                                 LigmaTextTool      *text_tool);


G_DEFINE_TYPE (LigmaTextTool, ligma_text_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_text_tool_parent_class


void
ligma_text_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_TEXT_TOOL,
                LIGMA_TYPE_TEXT_OPTIONS,
                ligma_text_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_FONT       |
                LIGMA_CONTEXT_PROP_MASK_PALETTE /* for the color popup's palette tab */,
                "ligma-text-tool",
                _("Text"),
                _("Text Tool: Create or edit text layers"),
                N_("Te_xt"), "T",
                NULL, LIGMA_HELP_TOOL_TEXT,
                LIGMA_ICON_TOOL_TEXT,
                data);
}

static void
ligma_text_tool_class_init (LigmaTextToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->constructed    = ligma_text_tool_constructed;
  object_class->finalize       = ligma_text_tool_finalize;

  tool_class->control          = ligma_text_tool_control;
  tool_class->button_press     = ligma_text_tool_button_press;
  tool_class->motion           = ligma_text_tool_motion;
  tool_class->button_release   = ligma_text_tool_button_release;
  tool_class->key_press        = ligma_text_tool_key_press;
  tool_class->key_release      = ligma_text_tool_key_release;
  tool_class->oper_update      = ligma_text_tool_oper_update;
  tool_class->cursor_update    = ligma_text_tool_cursor_update;
  tool_class->get_popup        = ligma_text_tool_get_popup;

  draw_tool_class->draw        = ligma_text_tool_draw;
}

static void
ligma_text_tool_init (LigmaTextTool *text_tool)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  text_tool->buffer = ligma_text_buffer_new ();

  g_signal_connect (text_tool->buffer, "begin-user-action",
                    G_CALLBACK (ligma_text_tool_buffer_begin_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "end-user-action",
                    G_CALLBACK (ligma_text_tool_buffer_end_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "color-applied",
                    G_CALLBACK (ligma_text_tool_buffer_color_applied),
                    text_tool);

  text_tool->handle_rectangle_change_complete = TRUE;

  ligma_text_tool_editor_init (text_tool);

  ligma_tool_control_set_scroll_lock          (tool->control, TRUE);
  ligma_tool_control_set_handle_empty_image   (tool->control, TRUE);
  ligma_tool_control_set_wants_click          (tool->control, TRUE);
  ligma_tool_control_set_wants_double_click   (tool->control, TRUE);
  ligma_tool_control_set_wants_triple_click   (tool->control, TRUE);
  ligma_tool_control_set_wants_all_key_events (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers     (tool->control,
                                              LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision            (tool->control,
                                              LIGMA_CURSOR_PRECISION_PIXEL_BORDER);
  ligma_tool_control_set_tool_cursor          (tool->control,
                                              LIGMA_TOOL_CURSOR_TEXT);
  ligma_tool_control_set_action_object_1      (tool->control,
                                              "context/context-font-select-set");
}

static void
ligma_text_tool_constructed (GObject *object)
{
  LigmaTextTool    *text_tool = LIGMA_TEXT_TOOL (object);
  LigmaTextOptions *options   = LIGMA_TEXT_TOOL_GET_OPTIONS (text_tool);
  LigmaTool        *tool      = LIGMA_TOOL (text_tool);
  LigmaAsyncSet    *async_set;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  text_tool->proxy = g_object_new (LIGMA_TYPE_TEXT, NULL);

  ligma_text_options_connect_text (options, text_tool->proxy);

  g_signal_connect_object (text_tool->proxy, "notify",
                           G_CALLBACK (ligma_text_tool_proxy_notify),
                           text_tool, 0);

  async_set =
    ligma_data_factory_get_async_set (tool->tool_info->ligma->font_factory);

  g_signal_connect_object (async_set,
                           "notify::empty",
                           G_CALLBACK (ligma_text_tool_fonts_async_set_empty_notify),
                           text_tool, 0);
}

static void
ligma_text_tool_finalize (GObject *object)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (object);

  g_clear_object (&text_tool->proxy);
  g_clear_object (&text_tool->buffer);
  g_clear_object (&text_tool->ui_manager);

  ligma_text_tool_editor_finalize (text_tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_text_tool_remove_empty_text_layer (LigmaTextTool *text_tool)
{
  LigmaTextLayer *text_layer = text_tool->layer;

  if (text_layer && text_layer->auto_rename)
    {
      LigmaText *text = ligma_text_layer_get_text (text_layer);

      if (text && text->box_mode == LIGMA_TEXT_BOX_DYNAMIC &&
          (! text->text || text->text[0] == '\0') &&
          (! text->markup || text->markup[0] == '\0'))
        {
          LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (text_layer));

          if (text_tool->image == image)
            g_signal_handlers_block_by_func (image,
                                             ligma_text_tool_layer_changed,
                                             text_tool);

          ligma_image_remove_layer (image, LIGMA_LAYER (text_layer), TRUE, NULL);
          ligma_image_flush (image);

          if (text_tool->image == image)
            g_signal_handlers_unblock_by_func (image,
                                               ligma_text_tool_layer_changed,
                                               text_tool);
        }
    }
}

static void
ligma_text_tool_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_text_tool_halt (text_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_text_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (tool);
  LigmaImage         *image     = ligma_display_get_image (display);
  LigmaText          *text      = text_tool->text;
  LigmaToolRectangle *rectangle;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if (tool->display && tool->display != display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

  if (! text_tool->widget)
    {
      GError *error = NULL;

      if (! ligma_text_tool_start (text_tool, display, NULL, &error))
        {
          ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

          ligma_tool_message_literal (tool, display, error->message);

          g_clear_error (&error);

          return;
        }

      ligma_tool_widget_hover (text_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      ligma_tool_rectangle_set_function (LIGMA_TOOL_RECTANGLE (text_tool->widget),
                                        LIGMA_TOOL_RECTANGLE_CREATING);
    }

  rectangle = LIGMA_TOOL_RECTANGLE (text_tool->widget);

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    {
      ligma_tool_control_activate (tool->control);

      /* clicking anywhere while a preedit is going on aborts the
       * preedit, this is ugly but at least leaves everything in
       * a consistent state
       */
      if (text_tool->preedit_active)
        ligma_text_tool_abort_im_context (text_tool);
      else
        ligma_text_tool_reset_im_context (text_tool);

      text_tool->selecting = FALSE;

      if (ligma_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          ligma_tool_rectangle_set_function (rectangle,
                                            LIGMA_TOOL_RECTANGLE_DEAD);
        }
      else if (ligma_tool_widget_button_press (text_tool->widget, coords,
                                              time, state, press_type))
        {
          text_tool->grab_widget = text_tool->widget;
        }

      /*  bail out now if the user user clicked on a handle of an
       *  existing rectangle, but not inside an existing framed layer
       */
      if (ligma_tool_rectangle_get_function (rectangle) !=
          LIGMA_TOOL_RECTANGLE_CREATING)
        {
          if (text_tool->layer)
            {
              LigmaItem *item = LIGMA_ITEM (text_tool->layer);
              gdouble   x    = coords->x - ligma_item_get_offset_x (item);
              gdouble   y    = coords->y - ligma_item_get_offset_y (item);

              if (x < 0 || x >= ligma_item_get_width  (item) ||
                  y < 0 || y >= ligma_item_get_height (item))
                {
                  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
                  return;
                }
            }
          else
            {
              ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
              return;
            }
        }

      /* if the the click is not related to the currently edited text
       * layer in any way, try to pick a text layer
       */
      if (! text_tool->moving &&
          ligma_tool_rectangle_get_function (rectangle) ==
          LIGMA_TOOL_RECTANGLE_CREATING)
        {
          LigmaTextLayer *text_layer;

          text_layer = ligma_image_pick_text_layer (image, coords->x, coords->y);

          if (text_layer && text_layer != text_tool->layer)
            {
              GList *selection = g_list_prepend (NULL, text_layer);

              if (text_tool->image == image)
                g_signal_handlers_block_by_func (image,
                                                 ligma_text_tool_layer_changed,
                                                 text_tool);

              ligma_image_set_selected_layers (image, selection);
              g_list_free (selection);

              if (text_tool->image == image)
                g_signal_handlers_unblock_by_func (image,
                                                   ligma_text_tool_layer_changed,
                                                   text_tool);
            }
        }
    }

  if (ligma_image_coords_in_active_pickable (image, coords, FALSE, FALSE, FALSE))
    {
      GList        *drawables = ligma_image_get_selected_drawables (image);
      LigmaDrawable *drawable  = NULL;
      gdouble       x         = coords->x;
      gdouble       y         = coords->y;

      if (g_list_length (drawables) == 1)
        {
          LigmaItem *item = LIGMA_ITEM (drawables->data);

          x = coords->x - ligma_item_get_offset_x (item);
          y = coords->y - ligma_item_get_offset_y (item);

          drawable = drawables->data;
        }
      g_list_free (drawables);

      /*  did the user click on a text layer?  */
      if (drawable &&
          ligma_text_tool_set_drawable (text_tool, drawable, TRUE))
        {
          if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
            {
              /*  if we clicked on a text layer while the tool was idle
               *  (didn't show a rectangle), frame the layer and switch to
               *  selecting instead of drawing a new rectangle
               */
              if (ligma_tool_rectangle_get_function (rectangle) ==
                  LIGMA_TOOL_RECTANGLE_CREATING)
                {
                  ligma_tool_rectangle_set_function (rectangle,
                                                    LIGMA_TOOL_RECTANGLE_DEAD);

                  ligma_text_tool_frame_item (text_tool);
                }

              if (text_tool->text && text_tool->text != text)
                {
                  ligma_text_tool_editor_start (text_tool);
                }
            }

          if (text_tool->text && ! text_tool->moving)
            {
              text_tool->selecting = TRUE;

              ligma_text_tool_editor_button_press (text_tool, x, y, press_type);
            }
          else
            {
              text_tool->selecting = FALSE;
            }

          ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

          return;
        }
    }

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    {
      /*  create a new text layer  */
      text_tool->text_box_fixed = FALSE;

      /* make sure the text tool has an image, even if the user didn't click
       * inside the active drawable, in particular, so that the text style
       * editor picks the correct resolution.
       */
      ligma_text_tool_set_image (text_tool, image);

      ligma_text_tool_connect (text_tool, NULL, NULL);
      ligma_text_tool_editor_start (text_tool);
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_text_tool_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (tool);
  LigmaToolRectangle *rectangle = LIGMA_TOOL_RECTANGLE (text_tool->widget);

  ligma_tool_control_halt (tool->control);

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
                                       ligma_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       ligma_text_tool_buffer_end_edit,
                                       text_tool);

      ligma_text_tool_editor_button_release (text_tool);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_begin_edit,
                                         text_tool);

      text_tool->selecting = FALSE;

      text_tool->handle_rectangle_change_complete = FALSE;

      /*  there is no cancelling of selections yet  */
      if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        release_type = LIGMA_BUTTON_RELEASE_NORMAL;
    }
  else if (text_tool->moving)
    {
      /*  the user has moved the text layer with Alt-drag, fall
       *  through and let rectangle-change-complete do its job of
       *  setting text layer's new position.
       */
    }
  else if (ligma_tool_rectangle_get_function (rectangle) ==
           LIGMA_TOOL_RECTANGLE_DEAD)
    {
      /*  the user clicked in dead space (like between the corner and
       *  edge handles, so completely ignore that.
       */

      text_tool->handle_rectangle_change_complete = FALSE;
    }
  else if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
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

      if (release_type == LIGMA_BUTTON_RELEASE_CLICK ||
          (x2 - x1) < 3                             ||
          (y2 - y1) < 3)
        {
          /*  unless the rectangle is unreasonably small to hold any
           *  real text (the user has eitherjust clicked or just made
           *  a rectangle of a few pixels), so set the text box to
           *  dynamic and ignore rectangle-change-complete.
           */

          g_object_set (text_tool->proxy,
                        "box-mode", LIGMA_TEXT_BOX_DYNAMIC,
                        NULL);

          text_tool->handle_rectangle_change_complete = FALSE;
        }
    }

  if (text_tool->grab_widget)
    {
      ligma_tool_widget_button_release (text_tool->grab_widget,
                                       coords, time, state, release_type);
      text_tool->grab_widget = NULL;
    }

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
ligma_text_tool_motion (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       LigmaDisplay      *display)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (tool);

  if (! text_tool->selecting)
    {
      if (text_tool->grab_widget)
        {
          ligma_tool_widget_motion (text_tool->grab_widget,
                                   coords, time, state);
        }
    }
  else
    {
      LigmaItem *item = LIGMA_ITEM (text_tool->layer);
      gdouble   x    = coords->x - ligma_item_get_offset_x (item);
      gdouble   y    = coords->y - ligma_item_get_offset_y (item);

      ligma_text_tool_editor_motion (text_tool, x, y);
    }
}

static gboolean
ligma_text_tool_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (tool);

  if (display == tool->display)
    return ligma_text_tool_editor_key_press (text_tool, kevent);

  return FALSE;
}

static gboolean
ligma_text_tool_key_release (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (tool);

  if (display == tool->display)
    return ligma_text_tool_editor_key_release (text_tool, kevent);

  return FALSE;
}

static void
ligma_text_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (tool);
  LigmaToolRectangle *rectangle = LIGMA_TOOL_RECTANGLE (text_tool->widget);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  text_tool->moving = (text_tool->widget &&
                       ligma_tool_rectangle_get_function (rectangle) ==
                       LIGMA_TOOL_RECTANGLE_MOVING &&
                       (state & GDK_MOD1_MASK));
}

static void
ligma_text_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (tool);
  LigmaToolRectangle *rectangle = LIGMA_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle && tool->display == display)
    {
      if (ligma_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          ligma_tool_set_cursor (tool, display,
                                (LigmaCursorType) GDK_XTERM,
                                ligma_tool_control_get_tool_cursor (tool->control),
                                LIGMA_CURSOR_MODIFIER_NONE);
        }
      else
        {
          LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
    }
  else
    {
      LigmaAsyncSet *async_set;

      async_set =
        ligma_data_factory_get_async_set (tool->tool_info->ligma->font_factory);

      if (ligma_async_set_is_empty (async_set))
        {
          LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
      else
        {
          ligma_tool_set_cursor (tool, display,
                                ligma_tool_control_get_cursor (tool->control),
                                ligma_tool_control_get_tool_cursor (tool->control),
                                LIGMA_CURSOR_MODIFIER_BAD);
        }
    }
}

static LigmaUIManager *
ligma_text_tool_get_popup (LigmaTool         *tool,
                          const LigmaCoords *coords,
                          GdkModifierType   state,
                          LigmaDisplay      *display,
                          const gchar     **ui_path)
{
  LigmaTextTool      *text_tool = LIGMA_TEXT_TOOL (tool);
  LigmaToolRectangle *rectangle = LIGMA_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle &&
      ligma_tool_rectangle_point_in_rectangle (rectangle,
                                              coords->x,
                                              coords->y))
    {
      if (! text_tool->ui_manager)
        {
          LigmaDisplayShell  *shell = ligma_display_get_shell (tool->display);
          LigmaImageWindow   *image_window;
          LigmaDialogFactory *dialog_factory;

          image_window   = ligma_display_shell_get_window (shell);
          dialog_factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (image_window));

          text_tool->ui_manager =
            ligma_menu_factory_manager_new (ligma_dialog_factory_get_menu_factory (dialog_factory),
                                           "<TextTool>",
                                           text_tool);
        }

      ligma_ui_manager_update (text_tool->ui_manager, text_tool);

      *ui_path = "/text-tool-popup";

      return text_tool->ui_manager;
    }

  return NULL;
}

static void
ligma_text_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaTextTool *text_tool = LIGMA_TEXT_TOOL (draw_tool);

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (! text_tool->text  ||
      ! text_tool->layer ||
      ! text_tool->layer->text)
    {
      ligma_text_tool_editor_update_im_cursor (text_tool);

      return;
    }

  ligma_text_tool_ensure_layout (text_tool);

  if (gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (text_tool->buffer)))
    {
      /* If the text buffer has a selection, highlight the selected letters */

      ligma_text_tool_draw_selection (draw_tool);
    }
  else
    {
      /* If the text buffer has no selection, draw the text cursor */

      LigmaCanvasItem   *item;
      PangoRectangle    cursor_rect;
      gint              off_x, off_y;
      gboolean          overwrite;
      LigmaTextDirection direction;

      ligma_text_tool_editor_get_cursor_rect (text_tool,
                                             text_tool->overwrite_mode,
                                             &cursor_rect);

      ligma_item_get_offset (LIGMA_ITEM (text_tool->layer), &off_x, &off_y);
      cursor_rect.x += off_x;
      cursor_rect.y += off_y;

      overwrite = text_tool->overwrite_mode && cursor_rect.width != 0;

      direction = ligma_text_tool_get_direction (text_tool);

      item = ligma_draw_tool_add_text_cursor (draw_tool, &cursor_rect,
                                             overwrite, direction);
      ligma_canvas_item_set_highlight (item, TRUE);
    }

  ligma_text_tool_editor_update_im_cursor (text_tool);
}

static void
ligma_text_tool_draw_selection (LigmaDrawTool *draw_tool)
{
  LigmaTextTool     *text_tool = LIGMA_TEXT_TOOL (draw_tool);
  GtkTextBuffer    *buffer    = GTK_TEXT_BUFFER (text_tool->buffer);
  LigmaCanvasGroup  *group;
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
  LigmaTextDirection direction;

  group = ligma_draw_tool_add_stroke_group (draw_tool);
  ligma_canvas_item_set_highlight (LIGMA_CANVAS_ITEM (group), TRUE);

  gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

  min = ligma_text_buffer_get_iter_index (text_tool->buffer, &sel_start, TRUE);
  max = ligma_text_buffer_get_iter_index (text_tool->buffer, &sel_end, TRUE);

  layout = ligma_text_layout_get_pango_layout (text_tool->layout);

  ligma_text_layout_get_offsets (text_tool->layout, &offset_x, &offset_y);

  ligma_text_layout_get_size (text_tool->layout, &width, &height);

  ligma_item_get_offset (LIGMA_ITEM (text_tool->layer), &off_x, &off_y);
  offset_x += off_x;
  offset_y += off_y;

  direction = ligma_text_tool_get_direction (text_tool);

  iter = pango_layout_get_iter (layout);

  ligma_draw_tool_push_group (draw_tool, group);

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

          ligma_text_layout_transform_rect (text_tool->layout, &rect);

          switch (direction)
            {
            case LIGMA_TEXT_DIRECTION_LTR:
            case LIGMA_TEXT_DIRECTION_RTL:
              rect.x += offset_x;
              rect.y += offset_y;
              ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.x, rect.y,
                                            rect.width, rect.height);
              break;
            case LIGMA_TEXT_DIRECTION_TTB_RTL:
            case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              rect.y = offset_x - rect.y + width;
              rect.x = offset_y + rect.x;
              ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            -rect.height, rect.width);
              break;
            case LIGMA_TEXT_DIRECTION_TTB_LTR:
            case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              rect.y = offset_x + rect.y;
              rect.x = offset_y - rect.x + height;
              ligma_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            rect.height, -rect.width);
              break;
            }
        }
    }
  while (pango_layout_iter_next_char (iter));

  ligma_draw_tool_pop_group (draw_tool);

  pango_layout_iter_free (iter);
}

static gboolean
ligma_text_tool_start (LigmaTextTool  *text_tool,
                      LigmaDisplay   *display,
                      LigmaLayer     *layer,
                      GError       **error)
{
  LigmaTool         *tool  = LIGMA_TOOL (text_tool);
  LigmaDisplayShell *shell = ligma_display_get_shell (display);
  LigmaToolWidget   *widget;
  LigmaAsyncSet     *async_set;

  async_set =
    ligma_data_factory_get_async_set (tool->tool_info->ligma->font_factory);

  if (! ligma_async_set_is_empty (async_set))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Fonts are still loading"));

      return FALSE;
    }

  tool->display = display;

  text_tool->widget = widget = ligma_tool_rectangle_new (shell);

  g_object_set (widget,
                "force-narrow-mode", TRUE,
                "status-title",      _("Text box: "),
                NULL);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), widget);

  g_signal_connect (widget, "response",
                    G_CALLBACK (ligma_text_tool_rectangle_response),
                    text_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (ligma_text_tool_rectangle_change_complete),
                    text_tool);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);

  if (layer)
    {
      ligma_text_tool_frame_item (text_tool);
      ligma_text_tool_editor_start (text_tool);
      ligma_text_tool_editor_position (text_tool);
    }

  return TRUE;
}

static void
ligma_text_tool_halt (LigmaTextTool *text_tool)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  ligma_text_tool_editor_halt (text_tool);
  ligma_text_tool_clear_layout (text_tool);
  ligma_text_tool_set_drawable (text_tool, NULL, FALSE);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&text_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
ligma_text_tool_frame_item (LigmaTextTool *text_tool)
{
  g_return_if_fail (LIGMA_IS_LAYER (text_tool->layer));

  text_tool->handle_rectangle_change_complete = FALSE;

  ligma_tool_rectangle_frame_item (LIGMA_TOOL_RECTANGLE (text_tool->widget),
                                  LIGMA_ITEM (text_tool->layer));

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
ligma_text_tool_rectangle_response (LigmaToolRectangle *rectangle,
                                   gint               response_id,
                                   LigmaTextTool      *text_tool)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  /* this happens when a newly created rectangle gets canceled,
   * we have to shut down the tool
   */
  if (response_id == LIGMA_TOOL_WIDGET_RESPONSE_CANCEL)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
}

static void
ligma_text_tool_rectangle_change_complete (LigmaToolRectangle *rectangle,
                                          LigmaTextTool      *text_tool)
{
  ligma_text_tool_editor_position (text_tool);

  if (text_tool->handle_rectangle_change_complete)
    {
      LigmaItem *item = LIGMA_ITEM (text_tool->layer);
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

      if ((x2 - x1) != ligma_item_get_width  (item) ||
          (y2 - y1) != ligma_item_get_height (item))
        {
          LigmaUnit  box_unit = text_tool->proxy->box_unit;
          gdouble   xres, yres;
          gboolean  push_undo = TRUE;
          LigmaUndo *undo;

          ligma_image_get_resolution (text_tool->image, &xres, &yres);

          g_object_set (text_tool->proxy,
                        "box-mode",   LIGMA_TEXT_BOX_FIXED,
                        "box-width",  ligma_pixels_to_units (x2 - x1,
                                                            box_unit, xres),
                        "box-height", ligma_pixels_to_units (y2 - y1,
                                                            box_unit, yres),
                        NULL);

          undo = ligma_image_undo_can_compress (text_tool->image,
                                               LIGMA_TYPE_UNDO_STACK,
                                               LIGMA_UNDO_GROUP_TEXT);

          if (undo &&
              ligma_undo_get_age (undo) <= TEXT_UNDO_TIMEOUT &&
              g_object_get_data (G_OBJECT (undo), "reshape-text-layer") == (gpointer) item)
            push_undo = FALSE;

          if (push_undo)
            {
              ligma_image_undo_group_start (text_tool->image, LIGMA_UNDO_GROUP_TEXT,
                                           _("Reshape Text Layer"));

              undo = ligma_image_undo_can_compress (text_tool->image, LIGMA_TYPE_UNDO_STACK,
                                                   LIGMA_UNDO_GROUP_TEXT);

              if (undo)
                g_object_set_data (G_OBJECT (undo), "reshape-text-layer",
                                   (gpointer) item);
            }

          ligma_text_tool_block_drawing (text_tool);

          ligma_item_translate (item,
                               x1 - ligma_item_get_offset_x (item),
                               y1 - ligma_item_get_offset_y (item),
                               push_undo);
          ligma_text_tool_apply (text_tool, push_undo);

          ligma_text_tool_unblock_drawing (text_tool);

          if (push_undo)
            ligma_image_undo_group_end (text_tool->image);
        }
      else if (x1 != ligma_item_get_offset_x (item) ||
               y1 != ligma_item_get_offset_y (item))
        {
          ligma_text_tool_block_drawing (text_tool);

          ligma_text_tool_apply (text_tool, TRUE);

          ligma_item_translate (item,
                               x1 - ligma_item_get_offset_x (item),
                               y1 - ligma_item_get_offset_y (item),
                               TRUE);

          ligma_text_tool_unblock_drawing (text_tool);

          ligma_image_flush (text_tool->image);
        }
    }
}

static void
ligma_text_tool_connect (LigmaTextTool  *text_tool,
                        LigmaTextLayer *layer,
                        LigmaText      *text)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  g_return_if_fail (text == NULL || (layer != NULL && layer->text == text));

  if (text_tool->text != text)
    {
      LigmaTextOptions *options = LIGMA_TEXT_TOOL_GET_OPTIONS (tool);

      g_signal_handlers_block_by_func (text_tool->buffer,
                                       ligma_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       ligma_text_tool_buffer_end_edit,
                                       text_tool);

      if (text_tool->text)
        {
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                ligma_text_tool_text_notify,
                                                text_tool);
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                ligma_text_tool_text_changed,
                                                text_tool);

          if (text_tool->pending)
            ligma_text_tool_apply (text_tool, TRUE);

          g_clear_object (&text_tool->text);

          g_object_set (text_tool->proxy,
                        "text",   NULL,
                        "markup", NULL,
                        NULL);
          ligma_text_buffer_set_text (text_tool->buffer, NULL);

          ligma_text_tool_clear_layout (text_tool);
        }

      ligma_context_define_property (LIGMA_CONTEXT (options),
                                    LIGMA_CONTEXT_PROP_FOREGROUND,
                                    text != NULL);

      if (text)
        {
          if (text->unit != text_tool->proxy->unit)
            ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (options->size_entry),
                                      text->unit);

          ligma_config_sync (G_OBJECT (text), G_OBJECT (text_tool->proxy), 0);

          if (text->markup)
            ligma_text_buffer_set_markup (text_tool->buffer, text->markup);
          else
            ligma_text_buffer_set_text (text_tool->buffer, text->text);

          ligma_text_tool_clear_layout (text_tool);

          text_tool->text = g_object_ref (text);

          g_signal_connect (text, "notify",
                            G_CALLBACK (ligma_text_tool_text_notify),
                            text_tool);
          g_signal_connect (text, "changed",
                            G_CALLBACK (ligma_text_tool_text_changed),
                            text_tool);
        }

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  if (text_tool->layer != layer)
    {
      if (text_tool->layer)
        {
          g_signal_handlers_disconnect_by_func (text_tool->layer,
                                                ligma_text_tool_layer_notify,
                                                text_tool);

          /*  don't try to remove the layer if it is not attached,
           *  which can happen if we got here because the layer was
           *  somehow deleted from the image (like by the user in the
           *  layers dialog).
           */
          if (ligma_item_is_attached (LIGMA_ITEM (text_tool->layer)))
            ligma_text_tool_remove_empty_text_layer (text_tool);
        }

      text_tool->layer = layer;

      if (layer)
        {
          g_signal_connect_object (text_tool->layer, "notify",
                                   G_CALLBACK (ligma_text_tool_layer_notify),
                                   text_tool, 0);
        }
    }
}

static void
ligma_text_tool_layer_notify (LigmaTextLayer    *layer,
                             const GParamSpec *pspec,
                             LigmaTextTool     *text_tool)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  if (! strcmp (pspec->name, "modified"))
    {
      if (layer->modified)
        ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "text"))
    {
      if (! layer->text)
        ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "offset-x") ||
           ! strcmp (pspec->name, "offset-y"))
    {
      if (ligma_item_is_attached (LIGMA_ITEM (layer)))
        {
          ligma_text_tool_block_drawing (text_tool);

          ligma_text_tool_frame_item (text_tool);

          ligma_text_tool_unblock_drawing (text_tool);
        }
    }
}

static gboolean
ligma_text_tool_apply_idle (LigmaTextTool *text_tool)
{
  text_tool->idle_id = 0;

  ligma_text_tool_apply (text_tool, TRUE);

  ligma_text_tool_unblock_drawing (text_tool);

  return G_SOURCE_REMOVE;
}

static void
ligma_text_tool_proxy_notify (LigmaText         *text,
                             const GParamSpec *pspec,
                             LigmaTextTool     *text_tool)
{
  if (! text_tool->text)
    return;

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE &&
      pspec->owner_type == LIGMA_TYPE_TEXT)
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
              ligma_text_tool_block_drawing (text_tool);
              ligma_text_tool_apply (text_tool, TRUE);
              ligma_text_tool_unblock_drawing (text_tool);
            }

          ligma_text_tool_block_drawing (text_tool);

          list = g_list_append (list, (gpointer) pspec);
          ligma_text_tool_apply_list (text_tool, list);
          g_list_free (list);

          ligma_text_tool_frame_item (text_tool);

          ligma_image_flush (ligma_item_get_image (LIGMA_ITEM (text_tool->layer)));

          ligma_text_tool_unblock_drawing (text_tool);
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
              ligma_text_tool_block_drawing (text_tool);

              text_tool->idle_id =
                g_idle_add_full (G_PRIORITY_LOW,
                                 (GSourceFunc) ligma_text_tool_apply_idle,
                                 text_tool,
                                 NULL);
            }
        }
    }
}

static void
ligma_text_tool_text_notify (LigmaText         *text,
                            const GParamSpec *pspec,
                            LigmaTextTool     *text_tool)
{
  g_return_if_fail (text == text_tool->text);

  /* an undo cancels all preedit operations */
  if (text_tool->preedit_active)
    ligma_text_tool_abort_im_context (text_tool);

  ligma_text_tool_block_drawing (text_tool);

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (text), pspec->name, &value);

      g_signal_handlers_block_by_func (text_tool->proxy,
                                       ligma_text_tool_proxy_notify,
                                       text_tool);

      g_object_set_property (G_OBJECT (text_tool->proxy), pspec->name, &value);

      g_signal_handlers_unblock_by_func (text_tool->proxy,
                                         ligma_text_tool_proxy_notify,
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
                                       ligma_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       ligma_text_tool_buffer_end_edit,
                                       text_tool);

      if (text->markup)
        ligma_text_buffer_set_markup (text_tool->buffer, text->markup);
      else
        ligma_text_buffer_set_text (text_tool->buffer, text->text);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         ligma_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  ligma_text_tool_unblock_drawing (text_tool);
}

static void
ligma_text_tool_text_changed (LigmaText     *text,
                             LigmaTextTool *text_tool)
{
  ligma_text_tool_block_drawing (text_tool);

  /* we need to redraw the rectangle in any case because whatever
   * changes to the text can change its size
   */
  ligma_text_tool_frame_item (text_tool);

  ligma_text_tool_unblock_drawing (text_tool);
}

static void
ligma_text_tool_fonts_async_set_empty_notify (LigmaAsyncSet *async_set,
                                             GParamSpec   *pspec,
                                             LigmaTextTool *text_tool)
{
  LigmaTool *tool = LIGMA_TOOL (text_tool);

  if (! ligma_async_set_is_empty (async_set) && tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
}

static void
ligma_text_tool_apply_list (LigmaTextTool *text_tool,
                           GList        *pspecs)
{
  GObject *src  = G_OBJECT (text_tool->proxy);
  GObject *dest = G_OBJECT (text_tool->text);
  GList   *list;

  g_signal_handlers_block_by_func (dest,
                                   ligma_text_tool_text_notify,
                                   text_tool);
  g_signal_handlers_block_by_func (dest,
                                   ligma_text_tool_text_changed,
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
                                     ligma_text_tool_text_notify,
                                     text_tool);
  g_signal_handlers_unblock_by_func (dest,
                                     ligma_text_tool_text_changed,
                                     text_tool);
}

static void
ligma_text_tool_create_layer (LigmaTextTool *text_tool,
                             LigmaText     *text)
{
  LigmaTool  *tool  = LIGMA_TOOL (text_tool);
  LigmaImage *image = ligma_display_get_image (tool->display);
  LigmaLayer *layer;
  gdouble    x1, y1;
  gdouble    x2, y2;

  ligma_text_tool_block_drawing (text_tool);

  if (text)
    {
      text = ligma_config_duplicate (LIGMA_CONFIG (text));
    }
  else
    {
      gchar *string;

      if (ligma_text_buffer_has_markup (text_tool->buffer))
        {
          string = ligma_text_buffer_get_markup (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "markup",   string,
                        "box-mode", LIGMA_TEXT_BOX_DYNAMIC,
                        NULL);
        }
      else
        {
          string = ligma_text_buffer_get_text (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "text",     string,
                        "box-mode", LIGMA_TEXT_BOX_DYNAMIC,
                        NULL);
        }

      g_free (string);

      text = ligma_config_duplicate (LIGMA_CONFIG (text_tool->proxy));
    }

  layer = ligma_text_layer_new (image, text);

  g_object_unref (text);

  if (! layer)
    {
      ligma_text_tool_unblock_drawing (text_tool);
      return;
    }

  ligma_text_tool_connect (text_tool, LIGMA_TEXT_LAYER (layer), text);

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  if (ligma_image_get_floating_selection (image))
    {
      g_signal_handlers_block_by_func (image,
                                       ligma_text_tool_layer_changed,
                                       text_tool);

      floating_sel_anchor (ligma_image_get_floating_selection (image));

      g_signal_handlers_unblock_by_func (image,
                                         ligma_text_tool_layer_changed,
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
          (text_tool->text->base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL ||
           text_tool->text->base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          x1 -= ligma_item_get_width (LIGMA_ITEM (layer));
        }
    }
  ligma_item_set_offset (LIGMA_ITEM (layer), x1, y1);

  ligma_image_add_layer (image, layer,
                        LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  if (text_tool->text_box_fixed)
    {
      LigmaUnit box_unit = text_tool->proxy->box_unit;
      gdouble  xres, yres;

      ligma_image_get_resolution (image, &xres, &yres);

      g_object_set (text_tool->proxy,
                    "box-mode",   LIGMA_TEXT_BOX_FIXED,
                    "box-width",  ligma_pixels_to_units (x2 - x1,
                                                        box_unit, xres),
                    "box-height", ligma_pixels_to_units (y2 - y1,
                                                        box_unit, yres),
                    NULL);

      ligma_text_tool_apply (text_tool, TRUE); /* unblocks drawing */
    }
  else
    {
      ligma_text_tool_frame_item (text_tool);
    }

  ligma_image_undo_group_end (image);

  ligma_image_flush (image);

  ligma_text_tool_set_drawable (text_tool, LIGMA_DRAWABLE (layer), FALSE);

  ligma_text_tool_unblock_drawing (text_tool);
}

#define  RESPONSE_NEW 1

static void
ligma_text_tool_confirm_response (GtkWidget    *widget,
                                 gint          response_id,
                                 LigmaTextTool *text_tool)
{
  LigmaTextLayer *layer = text_tool->layer;

  gtk_widget_destroy (widget);

  if (layer && layer->text)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          ligma_text_tool_create_layer (text_tool, layer->text);
          break;

        case GTK_RESPONSE_ACCEPT:
          ligma_text_tool_connect (text_tool, layer, layer->text);

          /*  cause the text layer to be rerendered  */
          g_object_notify (G_OBJECT (text_tool->proxy), "markup");

          ligma_text_tool_editor_start (text_tool);
          break;

        default:
          break;
        }
    }
}

static void
ligma_text_tool_confirm_dialog (LigmaTextTool *text_tool)
{
  LigmaTool         *tool  = LIGMA_TOOL (text_tool);
  LigmaDisplayShell *shell = ligma_display_get_shell (tool->display);
  GtkWidget        *dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;

  g_return_if_fail (text_tool->layer != NULL);

  if (text_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->confirm_dialog));
      return;
    }

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, text_tool->layer),
                                     LIGMA_CONTEXT (ligma_tool_get_options (tool)),
                                     _("Confirm Text Editing"),
                                     "ligma-text-tool-confirm",
                                     LIGMA_ICON_LAYER_TEXT_LAYER,
                                     _("Confirm Text Editing"),
                                     GTK_WIDGET (shell),
                                     ligma_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     _("_Cancel"),           GTK_RESPONSE_CANCEL,
                                     _("_Edit"),             GTK_RESPONSE_ACCEPT,

                                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_NEW,
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ligma_text_tool_confirm_response),
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
ligma_text_tool_layer_changed (LigmaImage    *image,
                              LigmaTextTool *text_tool)
{
  GList *layers = ligma_image_get_selected_layers (image);

  if (g_list_length (layers) != 1 || layers->data != text_tool->layer)
    {
      LigmaTool    *tool    = LIGMA_TOOL (text_tool);
      LigmaDisplay *display = tool->display;

      if (display)
        {
          LigmaLayer *layer = NULL;

          ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

          if (g_list_length (layers) == 1)
            layer = layers->data;

          /* The tool can only be started when a single layer is
           * selected and this is a text layer.
           */
          if (layer &&
              ligma_text_tool_set_drawable (text_tool, LIGMA_DRAWABLE (layer),
                                           FALSE) &&
              LIGMA_LAYER (text_tool->layer) == layer)
            {
              GError *error = NULL;

              if (! ligma_text_tool_start (text_tool, display, layer, &error))
                {
                  ligma_text_tool_set_drawable (text_tool, NULL, FALSE);

                  ligma_tool_message_literal (tool, display, error->message);

                  g_clear_error (&error);

                  return;
                }
            }
        }
    }
}

static void
ligma_text_tool_set_image (LigmaTextTool *text_tool,
                          LigmaImage    *image)
{
  if (text_tool->image == image)
    return;

  if (text_tool->image)
    {
      g_signal_handlers_disconnect_by_func (text_tool->image,
                                            ligma_text_tool_layer_changed,
                                            text_tool);

      g_object_remove_weak_pointer (G_OBJECT (text_tool->image),
                                    (gpointer) &text_tool->image);
    }

  text_tool->image = image;

  if (image)
    {
      LigmaTextOptions *options = LIGMA_TEXT_TOOL_GET_OPTIONS (text_tool);
      gdouble          xres;
      gdouble          yres;

      g_object_add_weak_pointer (G_OBJECT (text_tool->image),
                                 (gpointer) &text_tool->image);

      g_signal_connect_object (text_tool->image, "selected-layers-changed",
                               G_CALLBACK (ligma_text_tool_layer_changed),
                               text_tool, 0);

      ligma_image_get_resolution (image, &xres, &yres);
      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (options->size_entry), 0,
                                      yres, FALSE);
    }
}

static gboolean
ligma_text_tool_set_drawable (LigmaTextTool *text_tool,
                             LigmaDrawable *drawable,
                             gboolean      confirm)
{
  LigmaImage *image = NULL;

  if (text_tool->confirm_dialog)
    gtk_widget_destroy (text_tool->confirm_dialog);

  if (drawable)
    image = ligma_item_get_image (LIGMA_ITEM (drawable));

  ligma_text_tool_set_image (text_tool, image);

  if (LIGMA_IS_TEXT_LAYER (drawable) && LIGMA_TEXT_LAYER (drawable)->text)
    {
      LigmaTextLayer *layer = LIGMA_TEXT_LAYER (drawable);

      if (layer == text_tool->layer && layer->text == text_tool->text)
        return TRUE;

      if (layer->modified)
        {
          if (confirm)
            {
              ligma_text_tool_connect (text_tool, layer, NULL);
              ligma_text_tool_confirm_dialog (text_tool);
              return TRUE;
            }
        }
      else
        {
          ligma_text_tool_connect (text_tool, layer, layer->text);
          return TRUE;
        }
    }

  ligma_text_tool_connect (text_tool, NULL, NULL);

  return FALSE;
}

static void
ligma_text_tool_block_drawing (LigmaTextTool *text_tool)
{
  if (text_tool->drawing_blocked == 0)
    {
      ligma_draw_tool_pause (LIGMA_DRAW_TOOL (text_tool));

      ligma_text_tool_clear_layout (text_tool);
    }

  text_tool->drawing_blocked++;
}

static void
ligma_text_tool_unblock_drawing (LigmaTextTool *text_tool)
{
  g_return_if_fail (text_tool->drawing_blocked > 0);

  text_tool->drawing_blocked--;

  if (text_tool->drawing_blocked == 0)
    ligma_draw_tool_resume (LIGMA_DRAW_TOOL (text_tool));
}

static void
ligma_text_tool_buffer_begin_edit (LigmaTextBuffer *buffer,
                                  LigmaTextTool   *text_tool)
{
  ligma_text_tool_block_drawing (text_tool);
}

static void
ligma_text_tool_buffer_end_edit (LigmaTextBuffer *buffer,
                                LigmaTextTool   *text_tool)
{
  if (text_tool->text)
    {
      gchar *string;

      if (ligma_text_buffer_has_markup (buffer))
        {
          string = ligma_text_buffer_get_markup (buffer);

          g_object_set (text_tool->proxy,
                        "markup", string,
                        NULL);
        }
      else
        {
          string = ligma_text_buffer_get_text (buffer);

          g_object_set (text_tool->proxy,
                        "text", string,
                        NULL);
        }

      g_free (string);
    }
  else
    {
      ligma_text_tool_create_layer (text_tool, NULL);
    }

  ligma_text_tool_unblock_drawing (text_tool);
}

static void
ligma_text_tool_buffer_color_applied (LigmaTextBuffer *buffer,
                                     const LigmaRGB  *color,
                                     LigmaTextTool   *text_tool)
{
  ligma_palettes_add_color_history (LIGMA_TOOL (text_tool)->tool_info->ligma,
                                   color);
}


/*  public functions  */

void
ligma_text_tool_clear_layout (LigmaTextTool *text_tool)
{
  g_clear_object (&text_tool->layout);
}

gboolean
ligma_text_tool_ensure_layout (LigmaTextTool *text_tool)
{
  if (! text_tool->layout && text_tool->text)
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (text_tool->layer));
      gdouble    xres;
      gdouble    yres;
      GError    *error = NULL;

      ligma_image_get_resolution (image, &xres, &yres);

      text_tool->layout = ligma_text_layout_new (text_tool->layer->text,
                                                xres, yres, &error);
      if (error)
        {
          ligma_message_literal (image->ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  return text_tool->layout != NULL;
}

void
ligma_text_tool_apply (LigmaTextTool *text_tool,
                      gboolean      push_undo)
{
  const GParamSpec *pspec = NULL;
  LigmaImage        *image;
  LigmaTextLayer    *layer;
  GList            *list;
  gboolean          undo_group = FALSE;

  if (text_tool->idle_id)
    {
      g_source_remove (text_tool->idle_id);
      text_tool->idle_id = 0;

      ligma_text_tool_unblock_drawing (text_tool);
    }

  g_return_if_fail (text_tool->text != NULL);
  g_return_if_fail (text_tool->layer != NULL);

  layer = text_tool->layer;
  image = ligma_item_get_image (LIGMA_ITEM (layer));

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
      LigmaUndo *undo = ligma_image_undo_can_compress (image, LIGMA_TYPE_TEXT_UNDO,
                                                     LIGMA_UNDO_TEXT_LAYER);

      if (undo && LIGMA_ITEM_UNDO (undo)->item == LIGMA_ITEM (layer))
        {
          LigmaTextUndo *text_undo = LIGMA_TEXT_UNDO (undo);

          if (text_undo->pspec == pspec)
            {
              if (ligma_undo_get_age (undo) < TEXT_UNDO_TIMEOUT)
                {
                  LigmaTool    *tool = LIGMA_TOOL (text_tool);
                  LigmaContext *context;

                  context = LIGMA_CONTEXT (ligma_tool_get_options (tool));

                  push_undo = FALSE;
                  ligma_undo_reset_age (undo);
                  ligma_undo_refresh_preview (undo, context);
                }
            }
        }
    }

  if (push_undo)
    {
      if (layer->modified)
        {
          undo_group = TRUE;
          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_TEXT, NULL);

          ligma_image_undo_push_text_layer_modified (image, NULL, layer);

          /*  see comment in ligma_text_layer_set()  */
          ligma_image_undo_push_drawable_mod (image, NULL,
                                             LIGMA_DRAWABLE (layer), TRUE);
        }

      if (pspec)
        ligma_image_undo_push_text_layer (image, NULL, layer, pspec);
    }

  ligma_text_tool_apply_list (text_tool, list);

  g_list_free (text_tool->pending);
  text_tool->pending = NULL;

  if (push_undo)
    {
      g_object_set (layer, "modified", FALSE, NULL);

      if (undo_group)
        ligma_image_undo_group_end (image);
    }

  ligma_text_tool_frame_item (text_tool);

  ligma_image_flush (image);
}

gboolean
ligma_text_tool_set_layer (LigmaTextTool *text_tool,
                          LigmaLayer    *layer)
{
  g_return_val_if_fail (LIGMA_IS_TEXT_TOOL (text_tool), FALSE);
  g_return_val_if_fail (layer == NULL || LIGMA_IS_LAYER (layer), FALSE);

  if (layer == LIGMA_LAYER (text_tool->layer))
    return TRUE;

  /*  FIXME this function works, and I have no clue why: first we set
   *  the drawable, then we HALT the tool and start() it without
   *  re-setting the drawable. Why this works perfectly anyway when
   *  double clicking a text layer in the layers dialog... no idea.
   */
  if (ligma_text_tool_set_drawable (text_tool, LIGMA_DRAWABLE (layer), TRUE))
    {
      LigmaTool    *tool = LIGMA_TOOL (text_tool);
      LigmaItem    *item = LIGMA_ITEM (layer);
      LigmaContext *context;
      LigmaDisplay *display;

      context = ligma_get_user_context (tool->tool_info->ligma);
      display = ligma_context_get_display (context);

      if (! display ||
          ligma_display_get_image (display) != ligma_item_get_image (item))
        {
          GList *list;

          display = NULL;

          for (list = ligma_get_display_iter (tool->tool_info->ligma);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (ligma_display_get_image (display) == ligma_item_get_image (item))
                {
                  ligma_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      if (tool->display)
        ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

      if (display)
        {
          GError *error = NULL;

          if (! ligma_text_tool_start (text_tool, display, layer, &error))
            {
              ligma_text_tool_set_drawable (text_tool, NULL, FALSE);

              ligma_tool_message_literal (tool, display, error->message);

              g_clear_error (&error);

              return FALSE;
            }

          g_list_free (tool->drawables);
          tool->drawables = g_list_prepend (NULL, LIGMA_DRAWABLE (layer));
        }
    }

  return TRUE;
}

gboolean
ligma_text_tool_get_has_text_selection (LigmaTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  return gtk_text_buffer_get_has_selection (buffer);
}

void
ligma_text_tool_delete_selection (LigmaTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
    }
}

void
ligma_text_tool_cut_clipboard (LigmaTextTool *text_tool)
{
  LigmaDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (LIGMA_IS_TEXT_TOOL (text_tool));

  shell = ligma_display_get_shell (LIGMA_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                 clipboard, TRUE);
}

void
ligma_text_tool_copy_clipboard (LigmaTextTool *text_tool)
{
  LigmaDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (LIGMA_IS_TEXT_TOOL (text_tool));

  shell = ligma_display_get_shell (LIGMA_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  /*  need to block "end-user-action" on the text buffer, because
   *  GtkTextBuffer considers copying text to the clipboard an
   *  undo-relevant user action, which is clearly a bug, but what
   *  can we do...
   */
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   ligma_text_tool_buffer_begin_edit,
                                   text_tool);
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   ligma_text_tool_buffer_end_edit,
                                   text_tool);

  gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                  clipboard);

  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     ligma_text_tool_buffer_end_edit,
                                     text_tool);
  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     ligma_text_tool_buffer_begin_edit,
                                     text_tool);
}

void
ligma_text_tool_paste_clipboard (LigmaTextTool *text_tool)
{
  LigmaDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (LIGMA_IS_TEXT_TOOL (text_tool));

  shell = ligma_display_get_shell (LIGMA_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                   clipboard, NULL, TRUE);
}

void
ligma_text_tool_create_vectors (LigmaTextTool *text_tool)
{
  LigmaVectors *vectors;

  g_return_if_fail (LIGMA_IS_TEXT_TOOL (text_tool));

  if (! text_tool->text || ! text_tool->image)
    return;

  vectors = ligma_text_vectors_new (text_tool->image, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      ligma_item_get_offset (LIGMA_ITEM (text_tool->layer), &x, &y);
      ligma_item_translate (LIGMA_ITEM (vectors), x, y, FALSE);
    }

  ligma_image_add_vectors (text_tool->image, vectors,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  ligma_image_flush (text_tool->image);
}

gboolean
ligma_text_tool_create_vectors_warped (LigmaTextTool  *text_tool,
                                      GError       **error)
{
  GList             *vectors0;
  LigmaVectors       *vectors;
  gdouble            box_width;
  gdouble            box_height;
  LigmaTextDirection  dir;
  gdouble            offset = 0.0;

  g_return_val_if_fail (LIGMA_IS_TEXT_TOOL (text_tool), FALSE);

  if (! text_tool->text || ! text_tool->image || ! text_tool->layer)
    {
      if (! text_tool->text)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                             _("Text is required."));
      if (! text_tool->image)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                             _("No image."));
      if (! text_tool->layer)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                             _("No layer."));
      return FALSE;
    }

  box_width  = ligma_item_get_width  (LIGMA_ITEM (text_tool->layer));
  box_height = ligma_item_get_height (LIGMA_ITEM (text_tool->layer));

  vectors0 = ligma_image_get_selected_vectors (text_tool->image);
  if (g_list_length (vectors0) != 1)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Exactly one path must be selected."));
      return FALSE;
    }

  vectors = ligma_text_vectors_new (text_tool->image, text_tool->text);

  offset = 0;
  dir = ligma_text_tool_get_direction (text_tool);
  switch (dir)
    {
    case LIGMA_TEXT_DIRECTION_LTR:
    case LIGMA_TEXT_DIRECTION_RTL:
      offset = 0.5 * box_height;
      break;
    case LIGMA_TEXT_DIRECTION_TTB_RTL:
    case LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
    case LIGMA_TEXT_DIRECTION_TTB_LTR:
    case LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      {
        LigmaStroke *stroke = NULL;

        while ((stroke = ligma_vectors_stroke_get_next (vectors, stroke)))
          {
            ligma_stroke_rotate (stroke, 0, 0, 270);
            ligma_stroke_translate (stroke, 0, box_width);
          }
      }
      offset = 0.5 * box_width;
      break;
    }

  ligma_vectors_warp_vectors (vectors0->data, vectors, offset);

  ligma_item_set_visible (LIGMA_ITEM (vectors), TRUE, FALSE);

  ligma_image_add_vectors (text_tool->image, vectors,
                          LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);

  ligma_image_flush (text_tool->image);

  return TRUE;
}

LigmaTextDirection
ligma_text_tool_get_direction  (LigmaTextTool *text_tool)
{
  LigmaTextOptions *options = LIGMA_TEXT_TOOL_GET_OPTIONS (text_tool);
  return options->base_dir;
}
