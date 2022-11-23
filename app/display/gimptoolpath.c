/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpath.c
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "vectors/ligmaanchor.h"
#include "vectors/ligmabezierstroke.h"
#include "vectors/ligmavectors.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "tools/ligmatools-utils.h"

#include "ligmacanvashandle.h"
#include "ligmacanvasitem-utils.h"
#include "ligmacanvasline.h"
#include "ligmacanvaspath.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmatoolpath.h"

#include "ligma-intl.h"


#define TOGGLE_MASK  ligma_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  ligma_get_toggle_behavior_mask ()


/*  possible vector functions  */
typedef enum
{
  VECTORS_SELECT_VECTOR,
  VECTORS_CREATE_VECTOR,
  VECTORS_CREATE_STROKE,
  VECTORS_ADD_ANCHOR,
  VECTORS_MOVE_ANCHOR,
  VECTORS_MOVE_ANCHORSET,
  VECTORS_MOVE_HANDLE,
  VECTORS_MOVE_CURVE,
  VECTORS_MOVE_STROKE,
  VECTORS_MOVE_VECTORS,
  VECTORS_INSERT_ANCHOR,
  VECTORS_DELETE_ANCHOR,
  VECTORS_CONNECT_STROKES,
  VECTORS_DELETE_SEGMENT,
  VECTORS_CONVERT_EDGE,
  VECTORS_FINISHED
} LigmaVectorFunction;

enum
{
  PROP_0,
  PROP_VECTORS,
  PROP_EDIT_MODE,
  PROP_POLYGONAL
};

enum
{
  BEGIN_CHANGE,
  END_CHANGE,
  ACTIVATE,
  LAST_SIGNAL
};

struct _LigmaToolPathPrivate
{
  LigmaVectors          *vectors;        /* the current Vector data           */
  LigmaVectorMode        edit_mode;
  gboolean              polygonal;

  LigmaVectorFunction    function;       /* function we're performing         */
  LigmaAnchorFeatureType restriction;    /* movement restriction              */
  gboolean              modifier_lock;  /* can we toggle the Shift key?      */
  GdkModifierType       saved_state;    /* modifier state at button_press    */
  gdouble               last_x;         /* last x coordinate                 */
  gdouble               last_y;         /* last y coordinate                 */
  gboolean              undo_motion;    /* we need a motion to have an undo  */
  gboolean              have_undo;      /* did we push an undo at            */
                                        /* ..._button_press?                 */

  LigmaAnchor           *cur_anchor;     /* the current Anchor                */
  LigmaAnchor           *cur_anchor2;    /* secondary Anchor (end on_curve)   */
  LigmaStroke           *cur_stroke;     /* the current Stroke                */
  gdouble               cur_position;   /* the current Position on a segment */

  gint                  sel_count;      /* number of selected anchors        */
  LigmaAnchor           *sel_anchor;     /* currently selected anchor, NULL   */
                                        /* if multiple anchors are selected  */
  LigmaStroke           *sel_stroke;     /* selected stroke                   */

  LigmaVectorMode        saved_mode;     /* used by modifier_key()            */

  LigmaCanvasItem       *path;
  GList                *items;

  LigmaUIManager        *ui_manager;
};


/*  local function prototypes  */

static void     ligma_tool_path_constructed     (GObject               *object);
static void     ligma_tool_path_dispose         (GObject               *object);
static void     ligma_tool_path_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     ligma_tool_path_get_property    (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);

static void     ligma_tool_path_changed         (LigmaToolWidget        *widget);
static gint     ligma_tool_path_button_press    (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonPressType    press_type);
static void     ligma_tool_path_button_release  (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                LigmaButtonReleaseType  release_type);
static void     ligma_tool_path_motion          (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state);
static LigmaHit  ligma_tool_path_hit             (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     ligma_tool_path_hover           (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static gboolean ligma_tool_path_key_press       (LigmaToolWidget        *widget,
                                                GdkEventKey           *kevent);
static gboolean ligma_tool_path_get_cursor      (LigmaToolWidget        *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                LigmaCursorType        *cursor,
                                                LigmaToolCursorType    *tool_cursor,
                                                LigmaCursorModifier    *modifier);
static LigmaUIManager * ligma_tool_path_get_popup (LigmaToolWidget       *widget,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state,
                                                const gchar          **ui_path);

static LigmaVectorFunction
                   ligma_tool_path_get_function (LigmaToolPath          *path,
                                                const LigmaCoords      *coords,
                                                GdkModifierType        state);

static void     ligma_tool_path_update_status   (LigmaToolPath          *path,
                                                GdkModifierType        state,
                                                gboolean               proximity);

static void     ligma_tool_path_begin_change    (LigmaToolPath          *path,
                                                const gchar           *desc);
static void     ligma_tool_path_end_change      (LigmaToolPath          *path,
                                                gboolean               success);

static void     ligma_tool_path_vectors_visible (LigmaVectors           *vectors,
                                                LigmaToolPath          *path);
static void     ligma_tool_path_vectors_freeze  (LigmaVectors           *vectors,
                                                LigmaToolPath          *path);
static void     ligma_tool_path_vectors_thaw    (LigmaVectors           *vectors,
                                                LigmaToolPath          *path);
static void     ligma_tool_path_verify_state    (LigmaToolPath          *path);

static void     ligma_tool_path_move_selected_anchors
                                               (LigmaToolPath          *path,
                                                gdouble                x,
                                                gdouble                y);
static void     ligma_tool_path_delete_selected_anchors
                                               (LigmaToolPath          *path);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaToolPath, ligma_tool_path, LIGMA_TYPE_TOOL_WIDGET)

#define parent_class ligma_tool_path_parent_class

static guint path_signals[LAST_SIGNAL] = { 0 };


static void
ligma_tool_path_class_init (LigmaToolPathClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaToolWidgetClass *widget_class = LIGMA_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = ligma_tool_path_constructed;
  object_class->dispose         = ligma_tool_path_dispose;
  object_class->set_property    = ligma_tool_path_set_property;
  object_class->get_property    = ligma_tool_path_get_property;

  widget_class->changed         = ligma_tool_path_changed;
  widget_class->focus_changed   = ligma_tool_path_changed;
  widget_class->button_press    = ligma_tool_path_button_press;
  widget_class->button_release  = ligma_tool_path_button_release;
  widget_class->motion          = ligma_tool_path_motion;
  widget_class->hit             = ligma_tool_path_hit;
  widget_class->hover           = ligma_tool_path_hover;
  widget_class->key_press       = ligma_tool_path_key_press;
  widget_class->get_cursor      = ligma_tool_path_get_cursor;
  widget_class->get_popup       = ligma_tool_path_get_popup;

  path_signals[BEGIN_CHANGE] =
    g_signal_new ("begin-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolPathClass, begin_change),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  path_signals[END_CHANGE] =
    g_signal_new ("end-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolPathClass, end_change),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  path_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaToolPathClass, activate),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  g_object_class_install_property (object_class, PROP_VECTORS,
                                   g_param_spec_object ("vectors", NULL, NULL,
                                                        LIGMA_TYPE_VECTORS,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EDIT_MODE,
                                   g_param_spec_enum ("edit-mode",
                                                      _("Edit Mode"),
                                                      NULL,
                                                      LIGMA_TYPE_VECTOR_MODE,
                                                      LIGMA_VECTOR_MODE_DESIGN,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_POLYGONAL,
                                   g_param_spec_boolean ("polygonal",
                                                         _("Polygonal"),
                                                         _("Restrict editing to polygons"),
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_tool_path_init (LigmaToolPath *path)
{
  path->private = ligma_tool_path_get_instance_private (path);
}

static void
ligma_tool_path_constructed (GObject *object)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (object);
  LigmaToolWidget      *widget  = LIGMA_TOOL_WIDGET (object);
  LigmaToolPathPrivate *private = path->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->path = ligma_tool_widget_add_path (widget, NULL);

  ligma_tool_path_changed (widget);
}

static void
ligma_tool_path_dispose (GObject *object)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (object);
  LigmaToolPathPrivate *private = path->private;

  ligma_tool_path_set_vectors (path, NULL);

  g_clear_object (&private->ui_manager);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_tool_path_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (object);
  LigmaToolPathPrivate *private = path->private;

  switch (property_id)
    {
    case PROP_VECTORS:
      ligma_tool_path_set_vectors (path, g_value_get_object (value));
      break;
    case PROP_EDIT_MODE:
      private->edit_mode = g_value_get_enum (value);
      break;
    case PROP_POLYGONAL:
      private->polygonal = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tool_path_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (object);
  LigmaToolPathPrivate *private = path->private;

  switch (property_id)
    {
    case PROP_VECTORS:
      g_value_set_object (value, private->vectors);
      break;
    case PROP_EDIT_MODE:
      g_value_set_enum (value, private->edit_mode);
      break;
    case PROP_POLYGONAL:
      g_value_set_boolean (value, private->polygonal);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
item_remove_func (LigmaCanvasItem *item,
                  LigmaToolWidget *widget)
{
  ligma_tool_widget_remove_item (widget, item);
}

static void
ligma_tool_path_changed (LigmaToolWidget *widget)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;
  LigmaVectors         *vectors = private->vectors;

  if (private->items)
    {
      g_list_foreach (private->items, (GFunc) item_remove_func, widget);
      g_list_free (private->items);
      private->items = NULL;
    }

  if (vectors && ligma_vectors_get_bezier (vectors))
    {
      LigmaStroke *cur_stroke;

      ligma_canvas_path_set (private->path,
                            ligma_vectors_get_bezier (vectors));
      ligma_canvas_item_set_visible (private->path,
                                    ! ligma_item_get_visible (LIGMA_ITEM (vectors)));

      for (cur_stroke = ligma_vectors_stroke_get_next (vectors, NULL);
           cur_stroke;
           cur_stroke = ligma_vectors_stroke_get_next (vectors, cur_stroke))
        {
          LigmaCanvasItem *item;
          GArray         *coords;
          GList          *draw_anchors;
          GList          *list;
          gboolean        first = TRUE;

          /* anchor handles */
          draw_anchors = ligma_stroke_get_draw_anchors (cur_stroke);

          for (list = draw_anchors; list; list = g_list_next (list))
            {
              LigmaAnchor *cur_anchor = LIGMA_ANCHOR (list->data);

              if (cur_anchor->type == LIGMA_ANCHOR_ANCHOR)
                {
                  item =
                    ligma_tool_widget_add_handle (widget,
                                                 cur_anchor->selected ?
                                                 LIGMA_HANDLE_CIRCLE :
                                                 LIGMA_HANDLE_FILLED_CIRCLE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 LIGMA_HANDLE_ANCHOR_CENTER);

                  if (first)
                    {
                      gdouble angle = 0.0;
                      LigmaAnchor *next;

                      for (next = ligma_stroke_anchor_get_next (cur_stroke,
                                                               cur_anchor);
                           next;
                           next = ligma_stroke_anchor_get_next (cur_stroke, next))
                        {
                          if (((next->position.x - cur_anchor->position.x) *
                               (next->position.x - cur_anchor->position.x) +
                               (next->position.y - cur_anchor->position.y) *
                               (next->position.y - cur_anchor->position.y)) >= 0.1)
                            break;
                        }

                      if (next)
                        {
                          angle = atan2 (next->position.y - cur_anchor->position.y,
                                         next->position.x - cur_anchor->position.x);
                          g_object_set (item,
                                        "type", (cur_anchor->selected ?
                                                 LIGMA_HANDLE_DROP :
                                                 LIGMA_HANDLE_FILLED_DROP),
                                        "start-angle", angle,
                                        NULL);
                        }
                    }

                  private->items = g_list_prepend (private->items, item);

                  first = FALSE;
                }
            }

          g_list_free (draw_anchors);

          if (private->sel_count <= 2)
            {
              /* the lines to the control handles */
              coords = ligma_stroke_get_draw_lines (cur_stroke);

              if (coords)
                {
                  if (coords->len % 2 == 0)
                    {
                      gint i;

                      for (i = 0; i < coords->len; i += 2)
                        {
                          item = ligma_tool_widget_add_line
                            (widget,
                             g_array_index (coords, LigmaCoords, i).x,
                             g_array_index (coords, LigmaCoords, i).y,
                             g_array_index (coords, LigmaCoords, i + 1).x,
                             g_array_index (coords, LigmaCoords, i + 1).y);

                          if (ligma_tool_widget_get_focus (widget))
                            ligma_canvas_item_set_highlight (item, TRUE);

                          private->items = g_list_prepend (private->items, item);
                        }
                    }

                  g_array_free (coords, TRUE);
                }

              /* control handles */
              draw_anchors = ligma_stroke_get_draw_controls (cur_stroke);

              for (list = draw_anchors; list; list = g_list_next (list))
                {
                  LigmaAnchor *cur_anchor = LIGMA_ANCHOR (list->data);

                  item =
                    ligma_tool_widget_add_handle (widget,
                                                 LIGMA_HANDLE_SQUARE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 LIGMA_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 LIGMA_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 LIGMA_HANDLE_ANCHOR_CENTER);

                  private->items = g_list_prepend (private->items, item);
                }

              g_list_free (draw_anchors);
            }
        }
    }
  else
    {
      ligma_canvas_path_set (private->path, NULL);
    }
}

static gboolean
ligma_tool_path_check_writable (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private     = path->private;
  LigmaToolWidget      *widget      = LIGMA_TOOL_WIDGET (path);
  LigmaDisplayShell    *shell       = ligma_tool_widget_get_shell (widget);
  LigmaItem            *locked_item = NULL;

  if (ligma_item_is_content_locked (LIGMA_ITEM (private->vectors), &locked_item) ||
      ligma_item_is_position_locked (LIGMA_ITEM (private->vectors), &locked_item))
    {
      ligma_tool_widget_message_literal (LIGMA_TOOL_WIDGET (path),
                                        _("The selected path is locked."));

      if (locked_item == NULL)
        locked_item = LIGMA_ITEM (private->vectors);

      /* FIXME: this should really be done by the tool */
      ligma_tools_blink_lock_box (shell->display->ligma, locked_item);

      private->function = VECTORS_FINISHED;

      return FALSE;
    }

  return TRUE;
}

gboolean
ligma_tool_path_button_press (LigmaToolWidget      *widget,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;

  /* do nothing if we are in a FINISHED state */
  if (private->function == VECTORS_FINISHED)
    return 0;

  g_return_val_if_fail (private->vectors  != NULL                  ||
                        private->function == VECTORS_SELECT_VECTOR ||
                        private->function == VECTORS_CREATE_VECTOR, 0);

  private->undo_motion = FALSE;

  /* save the current modifier state */

  private->saved_state = state;


  /* select a vectors object */

  if (private->function == VECTORS_SELECT_VECTOR)
    {
      LigmaVectors *vectors;

      if (ligma_canvas_item_on_vectors (private->path,
                                       coords,
                                       LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                       LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                       NULL, NULL, NULL, NULL, NULL, &vectors))
        {
          ligma_tool_path_set_vectors (path, vectors);
        }

      private->function = VECTORS_FINISHED;
    }


  /* create a new vector from scratch */

  if (private->function == VECTORS_CREATE_VECTOR)
    {
      LigmaDisplayShell *shell = ligma_tool_widget_get_shell (widget);
      LigmaImage        *image = ligma_display_get_image (shell->display);
      LigmaVectors      *vectors;

      vectors = ligma_vectors_new (image, _("Unnamed"));
      g_object_ref_sink (vectors);

      /* Undo step gets added implicitly */
      private->have_undo = TRUE;

      private->undo_motion = TRUE;

      ligma_tool_path_set_vectors (path, vectors);
      g_object_unref (vectors);

      private->function = VECTORS_CREATE_STROKE;
    }


  ligma_vectors_freeze (private->vectors);

  /* create a new stroke */

  if (private->function == VECTORS_CREATE_STROKE &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Add Stroke"));
      private->undo_motion = TRUE;

      private->cur_stroke = ligma_bezier_stroke_new ();
      ligma_vectors_stroke_add (private->vectors, private->cur_stroke);
      g_object_unref (private->cur_stroke);

      private->sel_stroke = private->cur_stroke;
      private->cur_anchor = NULL;
      private->sel_anchor = NULL;
      private->function   = VECTORS_ADD_ANCHOR;
    }


  /* add an anchor to an existing stroke */

  if (private->function == VECTORS_ADD_ANCHOR &&
      ligma_tool_path_check_writable (path))
    {
      LigmaCoords position = LIGMA_COORDS_DEFAULT_VALUES;

      position.x = coords->x;
      position.y = coords->y;

      ligma_tool_path_begin_change (path, _("Add Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = ligma_bezier_stroke_extend (private->sel_stroke,
                                                       &position,
                                                       private->sel_anchor,
                                                       EXTEND_EDITABLE);

      private->restriction = LIGMA_ANCHOR_FEATURE_SYMMETRIC;

      if (! private->polygonal)
        private->function = VECTORS_MOVE_HANDLE;
      else
        private->function = VECTORS_MOVE_ANCHOR;

      private->cur_stroke = private->sel_stroke;
    }


  /* insertion of an anchor in a curve segment */

  if (private->function == VECTORS_INSERT_ANCHOR &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Insert Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = ligma_stroke_anchor_insert (private->cur_stroke,
                                                       private->cur_anchor,
                                                       private->cur_position);
      if (private->cur_anchor)
        {
          if (private->polygonal)
            {
              ligma_stroke_anchor_convert (private->cur_stroke,
                                          private->cur_anchor,
                                          LIGMA_ANCHOR_FEATURE_EDGE);
            }

          private->function = VECTORS_MOVE_ANCHOR;
        }
      else
        {
          private->function = VECTORS_FINISHED;
        }
    }


  /* move a handle */

  if (private->function == VECTORS_MOVE_HANDLE &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Drag Handle"));

      if (private->cur_anchor->type == LIGMA_ANCHOR_ANCHOR)
        {
          if (! private->cur_anchor->selected)
            {
              ligma_vectors_anchor_select (private->vectors,
                                          private->cur_stroke,
                                          private->cur_anchor,
                                          TRUE, TRUE);
              private->undo_motion = TRUE;
            }

          ligma_canvas_item_on_vectors_handle (private->path,
                                              private->vectors, coords,
                                              LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                              LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                              LIGMA_ANCHOR_CONTROL, TRUE,
                                              &private->cur_anchor,
                                              &private->cur_stroke);
          if (! private->cur_anchor)
            private->function = VECTORS_FINISHED;
        }
    }


  /* move an anchor */

  if (private->function == VECTORS_MOVE_ANCHOR &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Drag Anchor"));

      if (! private->cur_anchor->selected)
        {
          ligma_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor,
                                      TRUE, TRUE);
          private->undo_motion = TRUE;
        }
    }


  /* move multiple anchors */

  if (private->function == VECTORS_MOVE_ANCHORSET &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Drag Anchors"));

      if (state & TOGGLE_MASK)
        {
          ligma_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor,
                                      !private->cur_anchor->selected,
                                      FALSE);
          private->undo_motion = TRUE;

          if (private->cur_anchor->selected == FALSE)
            private->function = VECTORS_FINISHED;
        }
    }


  /* move a curve segment directly */

  if (private->function == VECTORS_MOVE_CURVE &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Drag Curve"));

      /* the magic numbers are taken from the "feel good" parameter
       * from ligma_bezier_stroke_point_move_relative in ligmabezierstroke.c. */
      if (private->cur_position < 5.0 / 6.0)
        {
          ligma_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor, TRUE, TRUE);
          private->undo_motion = TRUE;
        }

      if (private->cur_position > 1.0 / 6.0)
        {
          ligma_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor2, TRUE,
                                      (private->cur_position >= 5.0 / 6.0));
          private->undo_motion = TRUE;
        }

    }


  /* connect two strokes */

  if (private->function == VECTORS_CONNECT_STROKES &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Connect Strokes"));
      private->undo_motion = TRUE;

      ligma_stroke_connect_stroke (private->sel_stroke,
                                  private->sel_anchor,
                                  private->cur_stroke,
                                  private->cur_anchor);

      if (private->cur_stroke != private->sel_stroke &&
          ligma_stroke_is_empty (private->cur_stroke))
        {
          ligma_vectors_stroke_remove (private->vectors,
                                      private->cur_stroke);
        }

      private->sel_anchor = private->cur_anchor;
      private->cur_stroke = private->sel_stroke;

      ligma_vectors_anchor_select (private->vectors,
                                  private->sel_stroke,
                                  private->sel_anchor, TRUE, TRUE);

      private->function = VECTORS_FINISHED;
    }


  /* move a stroke or all strokes of a vectors object */

  if ((private->function == VECTORS_MOVE_STROKE ||
       private->function == VECTORS_MOVE_VECTORS) &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Drag Path"));

      /* Work is being done in ligma_tool_path_motion()... */
    }


  /* convert an anchor to something that looks like an edge */

  if (private->function == VECTORS_CONVERT_EDGE &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Convert Edge"));
      private->undo_motion = TRUE;

      ligma_stroke_anchor_convert (private->cur_stroke,
                                  private->cur_anchor,
                                  LIGMA_ANCHOR_FEATURE_EDGE);

      if (private->cur_anchor->type == LIGMA_ANCHOR_ANCHOR)
        {
          ligma_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor, TRUE, TRUE);

          private->function = VECTORS_MOVE_ANCHOR;
        }
      else
        {
          private->cur_stroke = NULL;
          private->cur_anchor = NULL;

          /* avoid doing anything stupid */
          private->function = VECTORS_FINISHED;
        }
    }


  /* removal of a node in a stroke */

  if (private->function == VECTORS_DELETE_ANCHOR &&
      ligma_tool_path_check_writable (path))
    {
      ligma_tool_path_begin_change (path, _("Delete Anchor"));
      private->undo_motion = TRUE;

      ligma_stroke_anchor_delete (private->cur_stroke,
                                 private->cur_anchor);

      if (ligma_stroke_is_empty (private->cur_stroke))
        ligma_vectors_stroke_remove (private->vectors,
                                    private->cur_stroke);

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function = VECTORS_FINISHED;
    }


  /* deleting a segment (opening up a stroke) */

  if (private->function == VECTORS_DELETE_SEGMENT &&
      ligma_tool_path_check_writable (path))
    {
      LigmaStroke *new_stroke;

      ligma_tool_path_begin_change (path, _("Delete Segment"));
      private->undo_motion = TRUE;

      new_stroke = ligma_stroke_open (private->cur_stroke,
                                     private->cur_anchor);
      if (new_stroke)
        {
          ligma_vectors_stroke_add (private->vectors, new_stroke);
          g_object_unref (new_stroke);
        }

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function   = VECTORS_FINISHED;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  ligma_vectors_thaw (private->vectors);

  return 1;
}

void
ligma_tool_path_button_release (LigmaToolWidget        *widget,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;

  private->function = VECTORS_FINISHED;

  if (private->have_undo)
    {
      if (! private->undo_motion ||
          release_type == LIGMA_BUTTON_RELEASE_CANCEL)
        {
          ligma_tool_path_end_change (path, FALSE);
        }
      else
        {
          ligma_tool_path_end_change (path, TRUE);
        }
    }
}

void
ligma_tool_path_motion (LigmaToolWidget   *widget,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state)
{
  LigmaToolPath        *path     = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private  = path->private;
  LigmaCoords           position = LIGMA_COORDS_DEFAULT_VALUES;
  LigmaAnchor          *anchor;

  if (private->function == VECTORS_FINISHED)
    return;

  position.x = coords->x;
  position.y = coords->y;

  ligma_vectors_freeze (private->vectors);

  if ((private->saved_state & TOGGLE_MASK) != (state & TOGGLE_MASK))
    private->modifier_lock = FALSE;

  if (! private->modifier_lock)
    {
      if (state & TOGGLE_MASK)
        {
          private->restriction = LIGMA_ANCHOR_FEATURE_SYMMETRIC;
        }
      else
        {
          private->restriction = LIGMA_ANCHOR_FEATURE_NONE;
        }
    }

  switch (private->function)
    {
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_HANDLE:
      anchor = private->cur_anchor;

      if (anchor)
        {
          ligma_stroke_anchor_move_absolute (private->cur_stroke,
                                            private->cur_anchor,
                                            &position,
                                            private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        {
          ligma_tool_path_move_selected_anchors (path,
                                                coords->x - private->last_x,
                                                coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else
        {
          ligma_stroke_point_move_absolute (private->cur_stroke,
                                           private->cur_anchor,
                                           private->cur_position,
                                           &position,
                                           private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_ANCHORSET:
      ligma_tool_path_move_selected_anchors (path,
                                            coords->x - private->last_x,
                                            coords->y - private->last_y);
      private->undo_motion = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      if (private->cur_stroke)
        {
          ligma_stroke_translate (private->cur_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else if (private->sel_stroke)
        {
          ligma_stroke_translate (private->sel_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_VECTORS:
      ligma_item_translate (LIGMA_ITEM (private->vectors),
                           coords->x - private->last_x,
                           coords->y - private->last_y, FALSE);
      private->undo_motion = TRUE;
      break;

    default:
      break;
    }

  ligma_vectors_thaw (private->vectors);

  private->last_x = coords->x;
  private->last_y = coords->y;
}

LigmaHit
ligma_tool_path_hit (LigmaToolWidget   *widget,
                    const LigmaCoords *coords,
                    GdkModifierType   state,
                    gboolean          proximity)
{
  LigmaToolPath *path = LIGMA_TOOL_PATH (widget);

  switch (ligma_tool_path_get_function (path, coords, state))
    {
    case VECTORS_SELECT_VECTOR:
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_ANCHORSET:
    case VECTORS_MOVE_HANDLE:
    case VECTORS_MOVE_CURVE:
    case VECTORS_MOVE_STROKE:
    case VECTORS_DELETE_ANCHOR:
    case VECTORS_DELETE_SEGMENT:
    case VECTORS_INSERT_ANCHOR:
    case VECTORS_CONNECT_STROKES:
    case VECTORS_CONVERT_EDGE:
      return LIGMA_HIT_DIRECT;

    case VECTORS_CREATE_VECTOR:
    case VECTORS_CREATE_STROKE:
    case VECTORS_ADD_ANCHOR:
    case VECTORS_MOVE_VECTORS:
      return LIGMA_HIT_INDIRECT;

    case VECTORS_FINISHED:
      return LIGMA_HIT_NONE;
    }

  return LIGMA_HIT_NONE;
}

void
ligma_tool_path_hover (LigmaToolWidget   *widget,
                      const LigmaCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;

  private->function = ligma_tool_path_get_function (path, coords, state);

  ligma_tool_path_update_status (path, state, proximity);
}

static gboolean
ligma_tool_path_key_press (LigmaToolWidget *widget,
                          GdkEventKey    *kevent)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;
  LigmaDisplayShell    *shell;
  gdouble              xdist, ydist;
  gdouble              pixels = 1.0;

  if (! private->vectors)
    return FALSE;

  shell = ligma_tool_widget_get_shell (widget);

  if (kevent->state & ligma_get_extend_selection_mask ())
    pixels = 10.0;

  if (kevent->state & ligma_get_toggle_behavior_mask ())
    pixels = 50.0;

  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      g_signal_emit (path, path_signals[ACTIVATE], 0,
                     kevent->state);
      break;

    case GDK_KEY_BackSpace:
    case GDK_KEY_Delete:
      ligma_tool_path_delete_selected_anchors (path);
      break;

    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      xdist = FUNSCALEX (shell, pixels);
      ydist = FUNSCALEY (shell, pixels);

      ligma_tool_path_begin_change (path, _("Move Anchors"));
      ligma_vectors_freeze (private->vectors);

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          ligma_tool_path_move_selected_anchors (path, -xdist, 0);
          break;

        case GDK_KEY_Right:
          ligma_tool_path_move_selected_anchors (path, xdist, 0);
          break;

        case GDK_KEY_Up:
          ligma_tool_path_move_selected_anchors (path, 0, -ydist);
          break;

        case GDK_KEY_Down:
          ligma_tool_path_move_selected_anchors (path, 0, ydist);
          break;

        default:
          break;
        }

      ligma_vectors_thaw (private->vectors);
      ligma_tool_path_end_change (path, TRUE);
      break;

    case GDK_KEY_Escape:
      if (private->edit_mode != LIGMA_VECTOR_MODE_DESIGN)
        g_object_set (private,
                      "vectors-edit-mode", LIGMA_VECTOR_MODE_DESIGN,
                      NULL);
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
ligma_tool_path_get_cursor (LigmaToolWidget     *widget,
                           const LigmaCoords   *coords,
                           GdkModifierType     state,
                           LigmaCursorType     *cursor,
                           LigmaToolCursorType *tool_cursor,
                           LigmaCursorModifier *modifier)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;

  *tool_cursor = LIGMA_TOOL_CURSOR_PATHS;
  *modifier    = LIGMA_CURSOR_MODIFIER_NONE;

  switch (private->function)
    {
    case VECTORS_SELECT_VECTOR:
      *tool_cursor = LIGMA_TOOL_CURSOR_HAND;
      break;

    case VECTORS_CREATE_VECTOR:
    case VECTORS_CREATE_STROKE:
      *modifier = LIGMA_CURSOR_MODIFIER_CONTROL;
      break;

    case VECTORS_ADD_ANCHOR:
    case VECTORS_INSERT_ANCHOR:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = LIGMA_CURSOR_MODIFIER_PLUS;
      break;

    case VECTORS_DELETE_ANCHOR:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = LIGMA_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_DELETE_SEGMENT:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = LIGMA_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_HANDLE:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONVERT_EDGE:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = LIGMA_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_ANCHOR:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_CURVE:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_STROKE:
    case VECTORS_MOVE_VECTORS:
      *modifier = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_ANCHORSET:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONNECT_STROKES:
      *tool_cursor = LIGMA_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = LIGMA_CURSOR_MODIFIER_JOIN;
      break;

    default:
      *modifier = LIGMA_CURSOR_MODIFIER_BAD;
      break;
    }

  return TRUE;
}

static LigmaUIManager *
ligma_tool_path_get_popup (LigmaToolWidget    *widget,
                          const LigmaCoords  *coords,
                          GdkModifierType    state,
                          const gchar      **ui_path)
{
  LigmaToolPath        *path    = LIGMA_TOOL_PATH (widget);
  LigmaToolPathPrivate *private = path->private;

  if (!private->ui_manager)
    {
      LigmaDisplayShell  *shell = ligma_tool_widget_get_shell (widget);
      LigmaImageWindow   *image_window;
      LigmaDialogFactory *dialog_factory;

      image_window   = ligma_display_shell_get_window (shell);
      dialog_factory = ligma_dock_container_get_dialog_factory (LIGMA_DOCK_CONTAINER (image_window));

      private->ui_manager =
        ligma_menu_factory_manager_new (ligma_dialog_factory_get_menu_factory (dialog_factory),
                                       "<VectorToolPath>",
                                       widget);
    }

  /* we're using a side effects of ligma_tool_path_get_function
   * that update the private->cur_* variables. */
  ligma_tool_path_get_function (path, coords, state);

  if (private->cur_stroke)
    {
      ligma_ui_manager_update (private->ui_manager, widget);

      *ui_path = "/vector-toolpath-popup";
      return private->ui_manager;
    }

  return NULL;
}


static LigmaVectorFunction
ligma_tool_path_get_function (LigmaToolPath     *path,
                             const LigmaCoords *coords,
                             GdkModifierType   state)
{
  LigmaToolPathPrivate *private    = path->private;
  LigmaAnchor          *anchor     = NULL;
  LigmaAnchor          *anchor2    = NULL;
  LigmaStroke          *stroke     = NULL;
  gdouble              position   = -1;
  gboolean             on_handle  = FALSE;
  gboolean             on_curve   = FALSE;
  gboolean             on_vectors = FALSE;
  LigmaVectorFunction   function   = VECTORS_FINISHED;

  private->modifier_lock = FALSE;

  /* are we hovering the current vectors on the current display? */
  if (private->vectors)
    {
      on_handle = ligma_canvas_item_on_vectors_handle (private->path,
                                                      private->vectors,
                                                      coords,
                                                      LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      LIGMA_ANCHOR_ANCHOR,
                                                      private->sel_count > 2,
                                                      &anchor, &stroke);

      if (! on_handle)
        on_curve = ligma_canvas_item_on_vectors_curve (private->path,
                                                      private->vectors,
                                                      coords,
                                                      LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      NULL,
                                                      &position, &anchor,
                                                      &anchor2, &stroke);
    }

  if (! on_handle && ! on_curve)
    {
      on_vectors = ligma_canvas_item_on_vectors (private->path,
                                                coords,
                                                LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                LIGMA_CANVAS_HANDLE_SIZE_CIRCLE,
                                                NULL, NULL, NULL, NULL, NULL,
                                                NULL);
    }

  private->cur_position = position;
  private->cur_anchor   = anchor;
  private->cur_anchor2  = anchor2;
  private->cur_stroke   = stroke;

  switch (private->edit_mode)
    {
    case LIGMA_VECTOR_MODE_DESIGN:
      if (! private->vectors)
        {
          if (on_vectors)
            {
              function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              function               = VECTORS_CREATE_VECTOR;
              private->restriction   = LIGMA_ANCHOR_FEATURE_SYMMETRIC;
              private->modifier_lock = TRUE;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == LIGMA_ANCHOR_ANCHOR)
            {
              if (state & TOGGLE_MASK)
                {
                  function = VECTORS_MOVE_ANCHORSET;
                }
              else
                {
                  if (private->sel_count >= 2 && anchor->selected)
                    function = VECTORS_MOVE_ANCHORSET;
                  else
                    function = VECTORS_MOVE_ANCHOR;
                }
            }
          else
            {
              function = VECTORS_MOVE_HANDLE;

              if (state & TOGGLE_MASK)
                private->restriction = LIGMA_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = LIGMA_ANCHOR_FEATURE_NONE;
            }
        }
      else if (on_curve)
        {
          if (ligma_stroke_point_is_movable (stroke, anchor, position))
            {
              function = VECTORS_MOVE_CURVE;

              if (state & TOGGLE_MASK)
                private->restriction = LIGMA_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = LIGMA_ANCHOR_FEATURE_NONE;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else
        {
          if (private->sel_stroke &&
              private->sel_anchor &&
              ligma_stroke_is_extendable (private->sel_stroke,
                                         private->sel_anchor) &&
              ! (state & TOGGLE_MASK))
            function = VECTORS_ADD_ANCHOR;
          else
            function = VECTORS_CREATE_STROKE;

          private->restriction   = LIGMA_ANCHOR_FEATURE_SYMMETRIC;
          private->modifier_lock = TRUE;
        }

      break;

    case LIGMA_VECTOR_MODE_EDIT:
      if (! private->vectors)
        {
          if (on_vectors)
            {
              function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == LIGMA_ANCHOR_ANCHOR)
            {
              if (! (state & TOGGLE_MASK) &&
                  private->sel_anchor &&
                  private->sel_anchor != anchor &&
                  ligma_stroke_is_extendable (private->sel_stroke,
                                             private->sel_anchor) &&
                  ligma_stroke_is_extendable (stroke, anchor))
                {
                  function = VECTORS_CONNECT_STROKES;
                }
              else
                {
                  if (state & TOGGLE_MASK)
                    {
                      function = VECTORS_DELETE_ANCHOR;
                    }
                  else
                    {
                      if (private->polygonal)
                        function = VECTORS_MOVE_ANCHOR;
                      else
                        function = VECTORS_MOVE_HANDLE;
                    }
                }
            }
          else
            {
              if (state & TOGGLE_MASK)
                function = VECTORS_CONVERT_EDGE;
              else
                function = VECTORS_MOVE_HANDLE;
            }
        }
      else if (on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              function = VECTORS_DELETE_SEGMENT;
            }
          else if (ligma_stroke_anchor_is_insertable (stroke, anchor, position))
            {
              function = VECTORS_INSERT_ANCHOR;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else
        {
          function = VECTORS_FINISHED;
        }

      break;

    case LIGMA_VECTOR_MODE_MOVE:
      if (! private->vectors)
        {
          if (on_vectors)
            {
              function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              function = VECTORS_FINISHED;
            }
        }
      else if (on_handle || on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              function = VECTORS_MOVE_VECTORS;
            }
          else
            {
              function = VECTORS_MOVE_STROKE;
            }
        }
      else
        {
          if (on_vectors)
            {
              function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              function = VECTORS_MOVE_VECTORS;
            }
        }
      break;
    }

  return function;
}

static void
ligma_tool_path_update_status (LigmaToolPath    *path,
                              GdkModifierType  state,
                              gboolean         proximity)
{
  LigmaToolPathPrivate *private     = path->private;
  GdkModifierType      extend_mask = ligma_get_extend_selection_mask ();
  GdkModifierType      toggle_mask = ligma_get_toggle_behavior_mask ();
  const gchar         *status      = NULL;
  gboolean             free_status = FALSE;

  if (! proximity)
    {
      ligma_tool_widget_set_status (LIGMA_TOOL_WIDGET (path), NULL);
      return;
    }

  switch (private->function)
    {
    case VECTORS_SELECT_VECTOR:
      status = _("Click to pick path to edit");
      break;

    case VECTORS_CREATE_VECTOR:
      status = _("Click to create a new path");
      break;

    case VECTORS_CREATE_STROKE:
      status = _("Click to create a new component of the path");
      break;

    case VECTORS_ADD_ANCHOR:
      status = ligma_suggest_modifiers (_("Click or Click-Drag to create "
                                         "a new anchor"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_ANCHOR:
      if (private->edit_mode != LIGMA_VECTOR_MODE_EDIT)
        {
          status = ligma_suggest_modifiers (_("Click-Drag to move the "
                                             "anchor around"),
                                           toggle_mask & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
        }
      else
        status = _("Click-Drag to move the anchor around");
      break;

    case VECTORS_MOVE_ANCHORSET:
      status = _("Click-Drag to move the anchors around");
      break;

    case VECTORS_MOVE_HANDLE:
      if (private->restriction != LIGMA_ANCHOR_FEATURE_SYMMETRIC)
        {
          status = ligma_suggest_modifiers (_("Click-Drag to move the "
                                             "handle around"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      else
        {
          status = ligma_suggest_modifiers (_("Click-Drag to move the "
                                             "handles around symmetrically"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      free_status = TRUE;
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        status = ligma_suggest_modifiers (_("Click-Drag to move the "
                                           "anchors around"),
                                         extend_mask & ~state,
                                         NULL, NULL, NULL);
      else
        status = ligma_suggest_modifiers (_("Click-Drag to change the "
                                           "shape of the curve"),
                                         extend_mask & ~state,
                                         _("%s: symmetrical"), NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      status = ligma_suggest_modifiers (_("Click-Drag to move the "
                                         "component around"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_VECTORS:
      status = _("Click-Drag to move the path around");
      break;

    case VECTORS_INSERT_ANCHOR:
      status = ligma_suggest_modifiers (_("Click-Drag to insert an anchor "
                                         "on the path"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_DELETE_ANCHOR:
      status = _("Click to delete this anchor");
      break;

    case VECTORS_CONNECT_STROKES:
      status = _("Click to connect this anchor "
                 "with the selected endpoint");
      break;

    case VECTORS_DELETE_SEGMENT:
      status = _("Click to open up the path");
      break;

    case VECTORS_CONVERT_EDGE:
      status = _("Click to make this node angular");
      break;

    case VECTORS_FINISHED:
      status = _("Clicking here does nothing, try clicking on path elements.");
      break;
    }

  ligma_tool_widget_set_status (LIGMA_TOOL_WIDGET (path), status);

  if (free_status)
    g_free ((gchar *) status);
}

static void
ligma_tool_path_begin_change (LigmaToolPath *path,
                             const gchar  *desc)
{
  LigmaToolPathPrivate *private = path->private;

  g_return_if_fail (private->vectors != NULL);

  /* don't push two undos */
  if (private->have_undo)
    return;

  g_signal_emit (path, path_signals[BEGIN_CHANGE], 0,
                 desc);

  private->have_undo = TRUE;
}

static void
ligma_tool_path_end_change (LigmaToolPath *path,
                           gboolean      success)
{
  LigmaToolPathPrivate *private = path->private;

  private->have_undo   = FALSE;
  private->undo_motion = FALSE;

  g_signal_emit (path, path_signals[END_CHANGE], 0,
                 success);
}

static void
ligma_tool_path_vectors_visible (LigmaVectors  *vectors,
                                LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;

  ligma_canvas_item_set_visible (private->path,
                                ! ligma_item_get_visible (LIGMA_ITEM (vectors)));
}

static void
ligma_tool_path_vectors_freeze (LigmaVectors  *vectors,
                               LigmaToolPath *path)
{
}

static void
ligma_tool_path_vectors_thaw (LigmaVectors  *vectors,
                             LigmaToolPath *path)
{
  /*  Ok, the vector might have changed externally (e.g. Undo) we need
   *  to validate our internal state.
   */
  ligma_tool_path_verify_state (path);
  ligma_tool_path_changed (LIGMA_TOOL_WIDGET (path));
}

static void
ligma_tool_path_verify_state (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private          = path->private;
  LigmaStroke          *cur_stroke       = NULL;
  gboolean             cur_anchor_valid = FALSE;
  gboolean             cur_stroke_valid = FALSE;

  private->sel_count  = 0;
  private->sel_anchor = NULL;
  private->sel_stroke = NULL;

  if (! private->vectors)
    {
      private->cur_position = -1;
      private->cur_anchor   = NULL;
      private->cur_stroke   = NULL;
      return;
    }

  while ((cur_stroke = ligma_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      GList *anchors;
      GList *list;

      /* anchor handles */
      anchors = ligma_stroke_get_draw_anchors (cur_stroke);

      if (cur_stroke == private->cur_stroke)
        cur_stroke_valid = TRUE;

      for (list = anchors; list; list = g_list_next (list))
        {
          LigmaAnchor *cur_anchor = list->data;

          if (cur_anchor == private->cur_anchor)
            cur_anchor_valid = TRUE;

          if (cur_anchor->type == LIGMA_ANCHOR_ANCHOR &&
              cur_anchor->selected)
            {
              private->sel_count++;
              if (private->sel_count == 1)
                {
                  private->sel_anchor = cur_anchor;
                  private->sel_stroke = cur_stroke;
                }
              else
                {
                  private->sel_anchor = NULL;
                  private->sel_stroke = NULL;
                }
            }
        }

      g_list_free (anchors);

      anchors = ligma_stroke_get_draw_controls (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          LigmaAnchor *cur_anchor = list->data;

          if (cur_anchor == private->cur_anchor)
            cur_anchor_valid = TRUE;
        }

      g_list_free (anchors);
    }

  if (! cur_stroke_valid)
    private->cur_stroke = NULL;

  if (! cur_anchor_valid)
    private->cur_anchor = NULL;
}

static void
ligma_tool_path_move_selected_anchors (LigmaToolPath *path,
                                      gdouble       x,
                                      gdouble       y)
{
  LigmaToolPathPrivate *private = path->private;
  LigmaAnchor          *cur_anchor;
  LigmaStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  LigmaCoords           offset = { 0.0, };

  offset.x = x;
  offset.y = y;

  while ((cur_stroke = ligma_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      /* anchors */
      anchors = ligma_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = LIGMA_ANCHOR (list->data);

          if (cur_anchor->selected)
            ligma_stroke_anchor_move_relative (cur_stroke,
                                              cur_anchor,
                                              &offset,
                                              LIGMA_ANCHOR_FEATURE_NONE);
        }

      g_list_free (anchors);
    }
}

static void
ligma_tool_path_delete_selected_anchors (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  LigmaAnchor          *cur_anchor;
  LigmaStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  gboolean             have_undo = FALSE;

  ligma_vectors_freeze (private->vectors);

  while ((cur_stroke = ligma_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      /* anchors */
      anchors = ligma_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = LIGMA_ANCHOR (list->data);

          if (cur_anchor->selected)
            {
              if (! have_undo)
                {
                  ligma_tool_path_begin_change (path, _("Delete Anchors"));
                  have_undo = TRUE;
                }

              ligma_stroke_anchor_delete (cur_stroke, cur_anchor);

              if (ligma_stroke_is_empty (cur_stroke))
                {
                  ligma_vectors_stroke_remove (private->vectors, cur_stroke);
                  cur_stroke = NULL;
                }
            }
        }

      g_list_free (anchors);
    }

  if (have_undo)
    ligma_tool_path_end_change (path, TRUE);

  ligma_vectors_thaw (private->vectors);
}


/*  public functions  */

LigmaToolWidget *
ligma_tool_path_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_PATH,
                       "shell", shell,
                       NULL);
}

void
ligma_tool_path_set_vectors (LigmaToolPath *path,
                            LigmaVectors  *vectors)
{
  LigmaToolPathPrivate *private;

  g_return_if_fail (LIGMA_IS_TOOL_PATH (path));
  g_return_if_fail (vectors == NULL || LIGMA_IS_VECTORS (vectors));

  private = path->private;

  if (vectors == private->vectors)
    return;

  if (private->vectors)
    {
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            ligma_tool_path_vectors_visible,
                                            path);
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            ligma_tool_path_vectors_freeze,
                                            path);
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            ligma_tool_path_vectors_thaw,
                                            path);

      g_object_unref (private->vectors);
    }

  private->vectors  = vectors;
  private->function = VECTORS_FINISHED;
  ligma_tool_path_verify_state (path);

  if (private->vectors)
    {
      g_object_ref (private->vectors);

      g_signal_connect_object (private->vectors, "visibility-changed",
                               G_CALLBACK (ligma_tool_path_vectors_visible),
                               path, 0);
      g_signal_connect_object (private->vectors, "freeze",
                               G_CALLBACK (ligma_tool_path_vectors_freeze),
                               path, 0);
      g_signal_connect_object (private->vectors, "thaw",
                               G_CALLBACK (ligma_tool_path_vectors_thaw),
                               path, 0);
    }

  g_object_notify (G_OBJECT (path), "vectors");
}

void
ligma_tool_path_get_popup_state (LigmaToolPath *path,
                                gboolean     *on_handle,
                                gboolean     *on_curve)
{
  LigmaToolPathPrivate *private = path->private;

  if (on_handle)
    *on_handle = private->cur_anchor2 == NULL;

  if (on_curve)
    *on_curve = private->cur_stroke != NULL;

}

void
ligma_tool_path_delete_anchor (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  ligma_vectors_freeze (private->vectors);
  ligma_tool_path_begin_change (path, _("Delete Anchors"));
  if (private->cur_anchor->type == LIGMA_ANCHOR_ANCHOR)
    {
      ligma_stroke_anchor_delete (private->cur_stroke, private->cur_anchor);
      if (ligma_stroke_is_empty (private->cur_stroke))
        ligma_vectors_stroke_remove (private->vectors,
                                    private->cur_stroke);
    }
  else
    {
      ligma_stroke_anchor_convert (private->cur_stroke,
                                  private->cur_anchor,
                                  LIGMA_ANCHOR_FEATURE_EDGE);
    }

  ligma_tool_path_end_change (path, TRUE);
  ligma_vectors_thaw (private->vectors);
}

void
ligma_tool_path_shift_start (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  ligma_vectors_freeze (private->vectors);
  ligma_tool_path_begin_change (path, _("Shift start"));
  ligma_stroke_shift_start (private->cur_stroke, private->cur_anchor);
  ligma_tool_path_end_change (path, TRUE);
  ligma_vectors_thaw (private->vectors);
}

void
ligma_tool_path_insert_anchor (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);
  g_return_if_fail (private->cur_position >= 0.0);

  ligma_vectors_freeze (private->vectors);
  ligma_tool_path_begin_change (path, _("Insert Anchor"));
  private->cur_anchor = ligma_stroke_anchor_insert (private->cur_stroke,
                                                   private->cur_anchor,
                                                   private->cur_position);
  ligma_tool_path_end_change (path, TRUE);
  ligma_vectors_thaw (private->vectors);
}

void
ligma_tool_path_delete_segment (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  LigmaStroke *new_stroke;
  g_return_if_fail (private->cur_stroke != NULL);
  g_return_if_fail (private->cur_anchor != NULL);

  ligma_vectors_freeze (private->vectors);
  ligma_tool_path_begin_change (path, _("Delete Segment"));

  new_stroke = ligma_stroke_open (private->cur_stroke,
                                 private->cur_anchor);
  if (new_stroke)
    {
      ligma_vectors_stroke_add (private->vectors, new_stroke);
      g_object_unref (new_stroke);
    }
  ligma_tool_path_end_change (path, TRUE);
  ligma_vectors_thaw (private->vectors);
}

void
ligma_tool_path_reverse_stroke (LigmaToolPath *path)
{
  LigmaToolPathPrivate *private = path->private;
  g_return_if_fail (private->cur_stroke != NULL);

  ligma_vectors_freeze (private->vectors);
  ligma_tool_path_begin_change (path, _("Insert Anchor"));
  ligma_stroke_reverse (private->cur_stroke);
  ligma_tool_path_end_change (path, TRUE);
  ligma_vectors_thaw (private->vectors);
}
