/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpath.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "vectors/gimpanchor.h"
#include "vectors/gimpbezierstroke.h"
#include "vectors/gimpvectors.h"

#include "widgets/gimpwidgets-utils.h"

#include "tools/gimptools-utils.h"

#include "gimpcanvashandle.h"
#include "gimpcanvasitem-utils.h"
#include "gimpcanvasline.h"
#include "gimpcanvaspath.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimptoolpath.h"

#include "gimp-intl.h"


#define TOGGLE_MASK  gimp_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  gimp_get_toggle_behavior_mask ()


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
} GimpVectorFunction;

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

struct _GimpToolPathPrivate
{
  GimpVectors          *vectors;        /* the current Vector data           */
  GimpVectorMode        edit_mode;
  gboolean              polygonal;

  GimpVectorFunction    function;       /* function we're performing         */
  GimpAnchorFeatureType restriction;    /* movement restriction              */
  gboolean              modifier_lock;  /* can we toggle the Shift key?      */
  GdkModifierType       saved_state;    /* modifier state at button_press    */
  gdouble               last_x;         /* last x coordinate                 */
  gdouble               last_y;         /* last y coordinate                 */
  gboolean              undo_motion;    /* we need a motion to have an undo  */
  gboolean              have_undo;      /* did we push an undo at            */
                                        /* ..._button_press?                 */

  GimpAnchor           *cur_anchor;     /* the current Anchor                */
  GimpAnchor           *cur_anchor2;    /* secondary Anchor (end on_curve)   */
  GimpStroke           *cur_stroke;     /* the current Stroke                */
  gdouble               cur_position;   /* the current Position on a segment */

  gint                  sel_count;      /* number of selected anchors        */
  GimpAnchor           *sel_anchor;     /* currently selected anchor, NULL   */
                                        /* if multiple anchors are selected  */
  GimpStroke           *sel_stroke;     /* selected stroke                   */

  GimpVectorMode        saved_mode;     /* used by modifier_key()            */

  GimpCanvasItem       *path;
  GList                *items;
};


/*  local function prototypes  */

static void     gimp_tool_path_constructed     (GObject               *object);
static void     gimp_tool_path_dispose         (GObject               *object);
static void     gimp_tool_path_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);
static void     gimp_tool_path_get_property    (GObject               *object,
                                                guint                  property_id,
                                                GValue                *value,
                                                GParamSpec            *pspec);

static void     gimp_tool_path_changed         (GimpToolWidget        *widget);
static gint     gimp_tool_path_button_press    (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonPressType    press_type);
static void     gimp_tool_path_button_release  (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                GimpButtonReleaseType  release_type);
static void     gimp_tool_path_motion          (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state);
static GimpHit  gimp_tool_path_hit             (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     gimp_tool_path_hover           (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static gboolean gimp_tool_path_key_press       (GimpToolWidget        *widget,
                                                GdkEventKey           *kevent);
static gboolean gimp_tool_path_get_cursor      (GimpToolWidget        *widget,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state,
                                                GimpCursorType        *cursor,
                                                GimpToolCursorType    *tool_cursor,
                                                GimpCursorModifier    *modifier);

static GimpVectorFunction
                   gimp_tool_path_get_function (GimpToolPath          *path,
                                                const GimpCoords      *coords,
                                                GdkModifierType        state);
                                                
static void     gimp_tool_path_update_status   (GimpToolPath          *path,
                                                GdkModifierType        state,
                                                gboolean               proximity);

static void     gimp_tool_path_begin_change    (GimpToolPath          *path,
                                                const gchar           *desc);
static void     gimp_tool_path_end_change      (GimpToolPath          *path,
                                                gboolean               success);

static void     gimp_tool_path_vectors_visible (GimpVectors           *vectors,
                                                GimpToolPath          *path);
static void     gimp_tool_path_vectors_freeze  (GimpVectors           *vectors,
                                                GimpToolPath          *path);
static void     gimp_tool_path_vectors_thaw    (GimpVectors           *vectors,
                                                GimpToolPath          *path);
static void     gimp_tool_path_verify_state    (GimpToolPath          *path);

static void     gimp_tool_path_move_selected_anchors
                                               (GimpToolPath          *path,
                                                gdouble                x,
                                                gdouble                y);
static void     gimp_tool_path_delete_selected_anchors
                                               (GimpToolPath          *path);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolPath, gimp_tool_path, GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_path_parent_class

static guint path_signals[LAST_SIGNAL] = { 0 };


static void
gimp_tool_path_class_init (GimpToolPathClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_path_constructed;
  object_class->dispose         = gimp_tool_path_dispose;
  object_class->set_property    = gimp_tool_path_set_property;
  object_class->get_property    = gimp_tool_path_get_property;

  widget_class->changed         = gimp_tool_path_changed;
  widget_class->focus_changed   = gimp_tool_path_changed;
  widget_class->button_press    = gimp_tool_path_button_press;
  widget_class->button_release  = gimp_tool_path_button_release;
  widget_class->motion          = gimp_tool_path_motion;
  widget_class->hit             = gimp_tool_path_hit;
  widget_class->hover           = gimp_tool_path_hover;
  widget_class->key_press       = gimp_tool_path_key_press;
  widget_class->get_cursor      = gimp_tool_path_get_cursor;

  path_signals[BEGIN_CHANGE] =
    g_signal_new ("begin-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolPathClass, begin_change),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  path_signals[END_CHANGE] =
    g_signal_new ("end-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolPathClass, end_change),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  path_signals[ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolPathClass, activate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__FLAGS,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_MODIFIER_TYPE);

  g_object_class_install_property (object_class, PROP_VECTORS,
                                   g_param_spec_object ("vectors", NULL, NULL,
                                                        GIMP_TYPE_VECTORS,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EDIT_MODE,
                                   g_param_spec_enum ("edit-mode",
                                                      _("Edit Mode"),
                                                      NULL,
                                                      GIMP_TYPE_VECTOR_MODE,
                                                      GIMP_VECTOR_MODE_DESIGN,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_POLYGONAL,
                                   g_param_spec_boolean ("polygonal",
                                                         _("Polygonal"),
                                                         _("Restrict editing to polygons"),
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_tool_path_init (GimpToolPath *path)
{
  path->private = gimp_tool_path_get_instance_private (path);
}

static void
gimp_tool_path_constructed (GObject *object)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (object);
  GimpToolWidget      *widget  = GIMP_TOOL_WIDGET (object);
  GimpToolPathPrivate *private = path->private;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  private->path = gimp_tool_widget_add_path (widget, NULL);

  gimp_tool_path_changed (widget);
}

static void
gimp_tool_path_dispose (GObject *object)
{
  GimpToolPath *path = GIMP_TOOL_PATH (object);

  gimp_tool_path_set_vectors (path, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_path_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (object);
  GimpToolPathPrivate *private = path->private;

  switch (property_id)
    {
    case PROP_VECTORS:
      gimp_tool_path_set_vectors (path, g_value_get_object (value));
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
gimp_tool_path_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (object);
  GimpToolPathPrivate *private = path->private;

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
item_remove_func (GimpCanvasItem *item,
                  GimpToolWidget *widget)
{
  gimp_tool_widget_remove_item (widget, item);
}

static void
gimp_tool_path_changed (GimpToolWidget *widget)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;
  GimpVectors         *vectors = private->vectors;

  if (private->items)
    {
      g_list_foreach (private->items, (GFunc) item_remove_func, widget);
      g_list_free (private->items);
      private->items = NULL;
    }

  if (vectors && gimp_vectors_get_bezier (vectors))
    {
      GimpStroke *cur_stroke;

      gimp_canvas_path_set (private->path,
                            gimp_vectors_get_bezier (vectors));
      gimp_canvas_item_set_visible (private->path,
                                    ! gimp_item_get_visible (GIMP_ITEM (vectors)));

      for (cur_stroke = gimp_vectors_stroke_get_next (vectors, NULL);
           cur_stroke;
           cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke))
        {
          GimpCanvasItem *item;
          GArray         *coords;
          GList          *draw_anchors;
          GList          *list;

          /* anchor handles */
          draw_anchors = gimp_stroke_get_draw_anchors (cur_stroke);

          for (list = draw_anchors; list; list = g_list_next (list))
            {
              GimpAnchor *cur_anchor = GIMP_ANCHOR (list->data);

              if (cur_anchor->type == GIMP_ANCHOR_ANCHOR)
                {
                  item =
                    gimp_tool_widget_add_handle (widget,
                                                 cur_anchor->selected ?
                                                 GIMP_HANDLE_CIRCLE :
                                                 GIMP_HANDLE_FILLED_CIRCLE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                 GIMP_HANDLE_ANCHOR_CENTER);

                  private->items = g_list_prepend (private->items, item);
                }
            }

          g_list_free (draw_anchors);

          if (private->sel_count <= 2)
            {
              /* the lines to the control handles */
              coords = gimp_stroke_get_draw_lines (cur_stroke);

              if (coords)
                {
                  if (coords->len % 2 == 0)
                    {
                      gint i;

                      for (i = 0; i < coords->len; i += 2)
                        {
                          item = gimp_tool_widget_add_line
                            (widget,
                             g_array_index (coords, GimpCoords, i).x,
                             g_array_index (coords, GimpCoords, i).y,
                             g_array_index (coords, GimpCoords, i + 1).x,
                             g_array_index (coords, GimpCoords, i + 1).y);

                          if (gimp_tool_widget_get_focus (widget))
                            gimp_canvas_item_set_highlight (item, TRUE);

                          private->items = g_list_prepend (private->items, item);
                        }
                    }

                  g_array_free (coords, TRUE);
                }

              /* control handles */
              draw_anchors = gimp_stroke_get_draw_controls (cur_stroke);

              for (list = draw_anchors; list; list = g_list_next (list))
                {
                  GimpAnchor *cur_anchor = GIMP_ANCHOR (list->data);

                  item =
                    gimp_tool_widget_add_handle (widget,
                                                 GIMP_HANDLE_SQUARE,
                                                 cur_anchor->position.x,
                                                 cur_anchor->position.y,
                                                 GIMP_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 GIMP_CANVAS_HANDLE_SIZE_CIRCLE - 3,
                                                 GIMP_HANDLE_ANCHOR_CENTER);

                  private->items = g_list_prepend (private->items, item);
                }

              g_list_free (draw_anchors);
            }
        }
    }
  else
    {
      gimp_canvas_path_set (private->path, NULL);
    }
}

static gboolean
gimp_tool_path_check_writable (GimpToolPath *path)
{
  GimpToolPathPrivate *private = path->private;
  GimpToolWidget      *widget  = GIMP_TOOL_WIDGET (path);
  GimpDisplayShell    *shell   = gimp_tool_widget_get_shell (widget);

  if (gimp_item_is_content_locked (GIMP_ITEM (private->vectors)) ||
      gimp_item_is_position_locked (GIMP_ITEM (private->vectors)))
    {
      gimp_tool_widget_message_literal (GIMP_TOOL_WIDGET (path),
                                        _("The active path is locked."));

      /* FIXME: this should really be done by the tool */
      gimp_tools_blink_lock_box (shell->display->gimp,
                                 GIMP_ITEM (private->vectors));

      private->function = VECTORS_FINISHED;

      return FALSE;
    }

  return TRUE;
}

gboolean
gimp_tool_path_button_press (GimpToolWidget      *widget,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;

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
      GimpVectors *vectors;

      if (gimp_canvas_item_on_vectors (private->path,
                                       coords,
                                       GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                       GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                       NULL, NULL, NULL, NULL, NULL, &vectors))
        {
          gimp_tool_path_set_vectors (path, vectors);
        }

      private->function = VECTORS_FINISHED;
    }


  /* create a new vector from scratch */

  if (private->function == VECTORS_CREATE_VECTOR)
    {
      GimpDisplayShell *shell = gimp_tool_widget_get_shell (widget);
      GimpImage        *image = gimp_display_get_image (shell->display);
      GimpVectors      *vectors;

      vectors = gimp_vectors_new (image, _("Unnamed"));
      g_object_ref_sink (vectors);

      /* Undo step gets added implicitly */
      private->have_undo = TRUE;

      private->undo_motion = TRUE;

      gimp_tool_path_set_vectors (path, vectors);
      g_object_unref (vectors);

      private->function = VECTORS_CREATE_STROKE;
    }


  gimp_vectors_freeze (private->vectors);

  /* create a new stroke */

  if (private->function == VECTORS_CREATE_STROKE &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Add Stroke"));
      private->undo_motion = TRUE;

      private->cur_stroke = gimp_bezier_stroke_new ();
      gimp_vectors_stroke_add (private->vectors, private->cur_stroke);
      g_object_unref (private->cur_stroke);

      private->sel_stroke = private->cur_stroke;
      private->cur_anchor = NULL;
      private->sel_anchor = NULL;
      private->function   = VECTORS_ADD_ANCHOR;
    }


  /* add an anchor to an existing stroke */

  if (private->function == VECTORS_ADD_ANCHOR &&
      gimp_tool_path_check_writable (path))
    {
      GimpCoords position = GIMP_COORDS_DEFAULT_VALUES;

      position.x = coords->x;
      position.y = coords->y;

      gimp_tool_path_begin_change (path, _("Add Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = gimp_bezier_stroke_extend (private->sel_stroke,
                                                       &position,
                                                       private->sel_anchor,
                                                       EXTEND_EDITABLE);

      private->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;

      if (! private->polygonal)
        private->function = VECTORS_MOVE_HANDLE;
      else
        private->function = VECTORS_MOVE_ANCHOR;

      private->cur_stroke = private->sel_stroke;
    }


  /* insertion of an anchor in a curve segment */

  if (private->function == VECTORS_INSERT_ANCHOR &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Insert Anchor"));
      private->undo_motion = TRUE;

      private->cur_anchor = gimp_stroke_anchor_insert (private->cur_stroke,
                                                       private->cur_anchor,
                                                       private->cur_position);
      if (private->cur_anchor)
        {
          if (private->polygonal)
            {
              gimp_stroke_anchor_convert (private->cur_stroke,
                                          private->cur_anchor,
                                          GIMP_ANCHOR_FEATURE_EDGE);
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
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Drag Handle"));

      if (private->cur_anchor->type == GIMP_ANCHOR_ANCHOR)
        {
          if (! private->cur_anchor->selected)
            {
              gimp_vectors_anchor_select (private->vectors,
                                          private->cur_stroke,
                                          private->cur_anchor,
                                          TRUE, TRUE);
              private->undo_motion = TRUE;
            }

          gimp_canvas_item_on_vectors_handle (private->path,
                                              private->vectors, coords,
                                              GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                              GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                              GIMP_ANCHOR_CONTROL, TRUE,
                                              &private->cur_anchor,
                                              &private->cur_stroke);
          if (! private->cur_anchor)
            private->function = VECTORS_FINISHED;
        }
    }


  /* move an anchor */

  if (private->function == VECTORS_MOVE_ANCHOR &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Drag Anchor"));

      if (! private->cur_anchor->selected)
        {
          gimp_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor,
                                      TRUE, TRUE);
          private->undo_motion = TRUE;
        }
    }


  /* move multiple anchors */

  if (private->function == VECTORS_MOVE_ANCHORSET &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Drag Anchors"));

      if (state & TOGGLE_MASK)
        {
          gimp_vectors_anchor_select (private->vectors,
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
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Drag Curve"));

      /* the magic numbers are taken from the "feel good" parameter
       * from gimp_bezier_stroke_point_move_relative in gimpbezierstroke.c. */
      if (private->cur_position < 5.0 / 6.0)
        {
          gimp_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor, TRUE, TRUE);
          private->undo_motion = TRUE;
        }

      if (private->cur_position > 1.0 / 6.0)
        {
          gimp_vectors_anchor_select (private->vectors,
                                      private->cur_stroke,
                                      private->cur_anchor2, TRUE,
                                      (private->cur_position >= 5.0 / 6.0));
          private->undo_motion = TRUE;
        }

    }


  /* connect two strokes */

  if (private->function == VECTORS_CONNECT_STROKES &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Connect Strokes"));
      private->undo_motion = TRUE;

      gimp_stroke_connect_stroke (private->sel_stroke,
                                  private->sel_anchor,
                                  private->cur_stroke,
                                  private->cur_anchor);

      if (private->cur_stroke != private->sel_stroke &&
          gimp_stroke_is_empty (private->cur_stroke))
        {
          gimp_vectors_stroke_remove (private->vectors,
                                      private->cur_stroke);
        }

      private->sel_anchor = private->cur_anchor;
      private->cur_stroke = private->sel_stroke;

      gimp_vectors_anchor_select (private->vectors,
                                  private->sel_stroke,
                                  private->sel_anchor, TRUE, TRUE);

      private->function = VECTORS_FINISHED;
    }


  /* move a stroke or all strokes of a vectors object */

  if ((private->function == VECTORS_MOVE_STROKE ||
       private->function == VECTORS_MOVE_VECTORS) &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Drag Path"));

      /* Work is being done in gimp_tool_path_motion()... */
    }


  /* convert an anchor to something that looks like an edge */

  if (private->function == VECTORS_CONVERT_EDGE &&
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Convert Edge"));
      private->undo_motion = TRUE;

      gimp_stroke_anchor_convert (private->cur_stroke,
                                  private->cur_anchor,
                                  GIMP_ANCHOR_FEATURE_EDGE);

      if (private->cur_anchor->type == GIMP_ANCHOR_ANCHOR)
        {
          gimp_vectors_anchor_select (private->vectors,
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
      gimp_tool_path_check_writable (path))
    {
      gimp_tool_path_begin_change (path, _("Delete Anchor"));
      private->undo_motion = TRUE;

      gimp_stroke_anchor_delete (private->cur_stroke,
                                 private->cur_anchor);

      if (gimp_stroke_is_empty (private->cur_stroke))
        gimp_vectors_stroke_remove (private->vectors,
                                    private->cur_stroke);

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function = VECTORS_FINISHED;
    }


  /* deleting a segment (opening up a stroke) */

  if (private->function == VECTORS_DELETE_SEGMENT &&
      gimp_tool_path_check_writable (path))
    {
      GimpStroke *new_stroke;

      gimp_tool_path_begin_change (path, _("Delete Segment"));
      private->undo_motion = TRUE;

      new_stroke = gimp_stroke_open (private->cur_stroke,
                                     private->cur_anchor);
      if (new_stroke)
        {
          gimp_vectors_stroke_add (private->vectors, new_stroke);
          g_object_unref (new_stroke);
        }

      private->cur_stroke = NULL;
      private->cur_anchor = NULL;
      private->function   = VECTORS_FINISHED;
    }

  private->last_x = coords->x;
  private->last_y = coords->y;

  gimp_vectors_thaw (private->vectors);

  return 1;
}

void
gimp_tool_path_button_release (GimpToolWidget        *widget,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;

  private->function = VECTORS_FINISHED;

  if (private->have_undo)
    {
      if (! private->undo_motion ||
          release_type == GIMP_BUTTON_RELEASE_CANCEL)
        {
          gimp_tool_path_end_change (path, FALSE);
        }
      else
        {
          gimp_tool_path_end_change (path, TRUE);
        }
    }
}

void
gimp_tool_path_motion (GimpToolWidget   *widget,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state)
{
  GimpToolPath        *path     = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private  = path->private;
  GimpCoords           position = GIMP_COORDS_DEFAULT_VALUES;
  GimpAnchor          *anchor;

  if (private->function == VECTORS_FINISHED)
    return;

  position.x = coords->x;
  position.y = coords->y;

  gimp_vectors_freeze (private->vectors);

  if ((private->saved_state & TOGGLE_MASK) != (state & TOGGLE_MASK))
    private->modifier_lock = FALSE;

  if (! private->modifier_lock)
    {
      if (state & TOGGLE_MASK)
        {
          private->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
        }
      else
        {
          private->restriction = GIMP_ANCHOR_FEATURE_NONE;
        }
    }

  switch (private->function)
    {
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_HANDLE:
      anchor = private->cur_anchor;

      if (anchor)
        {
          gimp_stroke_anchor_move_absolute (private->cur_stroke,
                                            private->cur_anchor,
                                            &position,
                                            private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        {
          gimp_tool_path_move_selected_anchors (path,
                                                coords->x - private->last_x,
                                                coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else
        {
          gimp_stroke_point_move_absolute (private->cur_stroke,
                                           private->cur_anchor,
                                           private->cur_position,
                                           &position,
                                           private->restriction);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_ANCHORSET:
      gimp_tool_path_move_selected_anchors (path,
                                            coords->x - private->last_x,
                                            coords->y - private->last_y);
      private->undo_motion = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      if (private->cur_stroke)
        {
          gimp_stroke_translate (private->cur_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      else if (private->sel_stroke)
        {
          gimp_stroke_translate (private->sel_stroke,
                                 coords->x - private->last_x,
                                 coords->y - private->last_y);
          private->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_VECTORS:
      gimp_item_translate (GIMP_ITEM (private->vectors),
                           coords->x - private->last_x,
                           coords->y - private->last_y, FALSE);
      private->undo_motion = TRUE;
      break;

    default:
      break;
    }

  gimp_vectors_thaw (private->vectors);

  private->last_x = coords->x;
  private->last_y = coords->y;
}

GimpHit
gimp_tool_path_hit (GimpToolWidget   *widget,
                    const GimpCoords *coords,
                    GdkModifierType   state,
                    gboolean          proximity)
{
  GimpToolPath *path = GIMP_TOOL_PATH (widget);

  switch (gimp_tool_path_get_function (path, coords, state))
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
      return GIMP_HIT_DIRECT;

    case VECTORS_CREATE_VECTOR:
    case VECTORS_CREATE_STROKE:
    case VECTORS_ADD_ANCHOR:
    case VECTORS_MOVE_VECTORS:
      return GIMP_HIT_INDIRECT;
    
    case VECTORS_FINISHED:
      return GIMP_HIT_NONE;
    }

  return GIMP_HIT_NONE;
}

void
gimp_tool_path_hover (GimpToolWidget   *widget,
                      const GimpCoords *coords,
                      GdkModifierType   state,
                      gboolean          proximity)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;

  private->function = gimp_tool_path_get_function (path, coords, state);

  gimp_tool_path_update_status (path, state, proximity);
}

static gboolean
gimp_tool_path_key_press (GimpToolWidget *widget,
                          GdkEventKey    *kevent)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;
  GimpDisplayShell    *shell;
  gdouble              xdist, ydist;
  gdouble              pixels = 1.0;

  if (! private->vectors)
    return FALSE;

  shell = gimp_tool_widget_get_shell (widget);

  if (kevent->state & gimp_get_extend_selection_mask ())
    pixels = 10.0;

  if (kevent->state & gimp_get_toggle_behavior_mask ())
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
      gimp_tool_path_delete_selected_anchors (path);
      break;

    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      xdist = FUNSCALEX (shell, pixels);
      ydist = FUNSCALEY (shell, pixels);

      gimp_tool_path_begin_change (path, _("Move Anchors"));
      gimp_vectors_freeze (private->vectors);

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          gimp_tool_path_move_selected_anchors (path, -xdist, 0);
          break;

        case GDK_KEY_Right:
          gimp_tool_path_move_selected_anchors (path, xdist, 0);
          break;

        case GDK_KEY_Up:
          gimp_tool_path_move_selected_anchors (path, 0, -ydist);
          break;

        case GDK_KEY_Down:
          gimp_tool_path_move_selected_anchors (path, 0, ydist);
          break;

        default:
          break;
        }

      gimp_vectors_thaw (private->vectors);
      gimp_tool_path_end_change (path, TRUE);
      break;

    case GDK_KEY_Escape:
      if (private->edit_mode != GIMP_VECTOR_MODE_DESIGN)
        g_object_set (private,
                      "vectors-edit-mode", GIMP_VECTOR_MODE_DESIGN,
                      NULL);
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_tool_path_get_cursor (GimpToolWidget     *widget,
                           const GimpCoords   *coords,
                           GdkModifierType     state,
                           GimpCursorType     *cursor,
                           GimpToolCursorType *tool_cursor,
                           GimpCursorModifier *modifier)
{
  GimpToolPath        *path    = GIMP_TOOL_PATH (widget);
  GimpToolPathPrivate *private = path->private;

  *tool_cursor = GIMP_TOOL_CURSOR_PATHS;
  *modifier    = GIMP_CURSOR_MODIFIER_NONE;

  switch (private->function)
    {
    case VECTORS_SELECT_VECTOR:
      *tool_cursor = GIMP_TOOL_CURSOR_HAND;
      break;

    case VECTORS_CREATE_VECTOR:
    case VECTORS_CREATE_STROKE:
      *modifier = GIMP_CURSOR_MODIFIER_CONTROL;
      break;

    case VECTORS_ADD_ANCHOR:
    case VECTORS_INSERT_ANCHOR:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = GIMP_CURSOR_MODIFIER_PLUS;
      break;

    case VECTORS_DELETE_ANCHOR:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_DELETE_SEGMENT:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_HANDLE:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONVERT_EDGE:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_CONTROL;
      *modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_ANCHOR:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_CURVE:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_STROKE:
    case VECTORS_MOVE_VECTORS:
      *modifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_ANCHORSET:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      *modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONNECT_STROKES:
      *tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      *modifier    = GIMP_CURSOR_MODIFIER_JOIN;
      break;

    default:
      *modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  return TRUE;
}

static GimpVectorFunction
gimp_tool_path_get_function (GimpToolPath     *path,
                             const GimpCoords *coords,
                             GdkModifierType   state)
{
  GimpToolPathPrivate *private    = path->private;
  GimpAnchor          *anchor     = NULL;
  GimpAnchor          *anchor2    = NULL;
  GimpStroke          *stroke     = NULL;
  gdouble              position   = -1;
  gboolean             on_handle  = FALSE;
  gboolean             on_curve   = FALSE;
  gboolean             on_vectors = FALSE;
  GimpVectorFunction   function   = VECTORS_FINISHED;

  private->modifier_lock = FALSE;

  /* are we hovering the current vectors on the current display? */
  if (private->vectors)
    {
      on_handle = gimp_canvas_item_on_vectors_handle (private->path,
                                                      private->vectors,
                                                      coords,
                                                      GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      GIMP_ANCHOR_ANCHOR,
                                                      private->sel_count > 2,
                                                      &anchor, &stroke);

      if (! on_handle)
        on_curve = gimp_canvas_item_on_vectors_curve (private->path,
                                                      private->vectors,
                                                      coords,
                                                      GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                      NULL,
                                                      &position, &anchor,
                                                      &anchor2, &stroke);
    }

  if (! on_handle && ! on_curve)
    {
      on_vectors = gimp_canvas_item_on_vectors (private->path,
                                                coords,
                                                GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                GIMP_CANVAS_HANDLE_SIZE_CIRCLE,
                                                NULL, NULL, NULL, NULL, NULL,
                                                NULL);
    }

  private->cur_position = position;
  private->cur_anchor   = anchor;
  private->cur_anchor2  = anchor2;
  private->cur_stroke   = stroke;

  switch (private->edit_mode)
    {
    case GIMP_VECTOR_MODE_DESIGN:
      if (! private->vectors)
        {
          if (on_vectors)
            {
              function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              function               = VECTORS_CREATE_VECTOR;
              private->restriction   = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              private->modifier_lock = TRUE;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == GIMP_ANCHOR_ANCHOR)
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
                private->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = GIMP_ANCHOR_FEATURE_NONE;
            }
        }
      else if (on_curve)
        {
          if (gimp_stroke_point_is_movable (stroke, anchor, position))
            {
              function = VECTORS_MOVE_CURVE;

              if (state & TOGGLE_MASK)
                private->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              else
                private->restriction = GIMP_ANCHOR_FEATURE_NONE;
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
              gimp_stroke_is_extendable (private->sel_stroke,
                                         private->sel_anchor) &&
              ! (state & TOGGLE_MASK))
            function = VECTORS_ADD_ANCHOR;
          else
            function = VECTORS_CREATE_STROKE;

          private->restriction   = GIMP_ANCHOR_FEATURE_SYMMETRIC;
          private->modifier_lock = TRUE;
        }

      break;

    case GIMP_VECTOR_MODE_EDIT:
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
          if (anchor->type == GIMP_ANCHOR_ANCHOR)
            {
              if (! (state & TOGGLE_MASK) &&
                  private->sel_anchor &&
                  private->sel_anchor != anchor &&
                  gimp_stroke_is_extendable (private->sel_stroke,
                                             private->sel_anchor) &&
                  gimp_stroke_is_extendable (stroke, anchor))
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
          else if (gimp_stroke_anchor_is_insertable (stroke, anchor, position))
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

    case GIMP_VECTOR_MODE_MOVE:
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
gimp_tool_path_update_status (GimpToolPath    *path,
                              GdkModifierType  state,
                              gboolean         proximity)
{
  GimpToolPathPrivate *private     = path->private;
  GdkModifierType      extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType      toggle_mask = gimp_get_toggle_behavior_mask ();
  const gchar         *status      = NULL;
  gboolean             free_status = FALSE;

  if (! proximity)
    {
      gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (path), NULL);
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
      status = gimp_suggest_modifiers (_("Click or Click-Drag to create "
                                         "a new anchor"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_ANCHOR:
      if (private->edit_mode != GIMP_VECTOR_MODE_EDIT)
        {
          status = gimp_suggest_modifiers (_("Click-Drag to move the "
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
      if (private->restriction != GIMP_ANCHOR_FEATURE_SYMMETRIC)
        {
          status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                             "handle around"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      else
        {
          status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                             "handles around symmetrically"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
        }
      free_status = TRUE;
      break;

    case VECTORS_MOVE_CURVE:
      if (private->polygonal)
        status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                           "anchors around"),
                                         extend_mask & ~state,
                                         NULL, NULL, NULL);
      else
        status = gimp_suggest_modifiers (_("Click-Drag to change the "
                                           "shape of the curve"),
                                         extend_mask & ~state,
                                         _("%s: symmetrical"), NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                         "component around"),
                                       extend_mask & ~state,
                                       NULL, NULL, NULL);
      free_status = TRUE;
      break;

    case VECTORS_MOVE_VECTORS:
      status = _("Click-Drag to move the path around");
      break;

    case VECTORS_INSERT_ANCHOR:
      status = gimp_suggest_modifiers (_("Click-Drag to insert an anchor "
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

  gimp_tool_widget_set_status (GIMP_TOOL_WIDGET (path), status);

  if (free_status)
    g_free ((gchar *) status);
}

static void
gimp_tool_path_begin_change (GimpToolPath *path,
                             const gchar  *desc)
{
  GimpToolPathPrivate *private = path->private;

  g_return_if_fail (private->vectors != NULL);

  /* don't push two undos */
  if (private->have_undo)
    return;

  g_signal_emit (path, path_signals[BEGIN_CHANGE], 0,
                 desc);

  private->have_undo = TRUE;
}

static void
gimp_tool_path_end_change (GimpToolPath *path,
                           gboolean      success)
{
  GimpToolPathPrivate *private = path->private;

  private->have_undo   = FALSE;
  private->undo_motion = FALSE;

  g_signal_emit (path, path_signals[END_CHANGE], 0,
                 success);
}

static void
gimp_tool_path_vectors_visible (GimpVectors  *vectors,
                                GimpToolPath *path)
{
  GimpToolPathPrivate *private = path->private;

  gimp_canvas_item_set_visible (private->path,
                                ! gimp_item_get_visible (GIMP_ITEM (vectors)));
}

static void
gimp_tool_path_vectors_freeze (GimpVectors  *vectors,
                               GimpToolPath *path)
{
}

static void
gimp_tool_path_vectors_thaw (GimpVectors  *vectors,
                             GimpToolPath *path)
{
  /*  Ok, the vector might have changed externally (e.g. Undo) we need
   *  to validate our internal state.
   */
  gimp_tool_path_verify_state (path);
  gimp_tool_path_changed (GIMP_TOOL_WIDGET (path));
}

static void
gimp_tool_path_verify_state (GimpToolPath *path)
{
  GimpToolPathPrivate *private          = path->private;
  GimpStroke          *cur_stroke       = NULL;
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

  while ((cur_stroke = gimp_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      GList *anchors;
      GList *list;

      /* anchor handles */
      anchors = gimp_stroke_get_draw_anchors (cur_stroke);

      if (cur_stroke == private->cur_stroke)
        cur_stroke_valid = TRUE;

      for (list = anchors; list; list = g_list_next (list))
        {
          GimpAnchor *cur_anchor = list->data;

          if (cur_anchor == private->cur_anchor)
            cur_anchor_valid = TRUE;

          if (cur_anchor->type == GIMP_ANCHOR_ANCHOR &&
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

      anchors = gimp_stroke_get_draw_controls (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          GimpAnchor *cur_anchor = list->data;

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
gimp_tool_path_move_selected_anchors (GimpToolPath *path,
                                      gdouble       x,
                                      gdouble       y)
{
  GimpToolPathPrivate *private = path->private;
  GimpAnchor          *cur_anchor;
  GimpStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  GimpCoords           offset = { 0.0, };

  offset.x = x;
  offset.y = y;

  while ((cur_stroke = gimp_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      /* anchors */
      anchors = gimp_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = GIMP_ANCHOR (list->data);

          if (cur_anchor->selected)
            gimp_stroke_anchor_move_relative (cur_stroke,
                                              cur_anchor,
                                              &offset,
                                              GIMP_ANCHOR_FEATURE_NONE);
        }

      g_list_free (anchors);
    }
}

static void
gimp_tool_path_delete_selected_anchors (GimpToolPath *path)
{
  GimpToolPathPrivate *private = path->private;
  GimpAnchor          *cur_anchor;
  GimpStroke          *cur_stroke = NULL;
  GList               *anchors;
  GList               *list;
  gboolean             have_undo = FALSE;

  gimp_vectors_freeze (private->vectors);

  while ((cur_stroke = gimp_vectors_stroke_get_next (private->vectors,
                                                     cur_stroke)))
    {
      /* anchors */
      anchors = gimp_stroke_get_draw_anchors (cur_stroke);

      for (list = anchors; list; list = g_list_next (list))
        {
          cur_anchor = GIMP_ANCHOR (list->data);

          if (cur_anchor->selected)
            {
              if (! have_undo)
                {
                  gimp_tool_path_begin_change (path, _("Delete Anchors"));
                  have_undo = TRUE;
                }

              gimp_stroke_anchor_delete (cur_stroke, cur_anchor);

              if (gimp_stroke_is_empty (cur_stroke))
                {
                  gimp_vectors_stroke_remove (private->vectors, cur_stroke);
                  cur_stroke = NULL;
                }
            }
        }

      g_list_free (anchors);
    }

  if (have_undo)
    gimp_tool_path_end_change (path, TRUE);

  gimp_vectors_thaw (private->vectors);
}


/*  public functions  */

GimpToolWidget *
gimp_tool_path_new (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_PATH,
                       "shell", shell,
                       NULL);
}

void
gimp_tool_path_set_vectors (GimpToolPath *path,
                            GimpVectors  *vectors)
{
  GimpToolPathPrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_PATH (path));
  g_return_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors));

  private = path->private;

  if (vectors == private->vectors)
    return;

  if (private->vectors)
    {
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            gimp_tool_path_vectors_visible,
                                            path);
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            gimp_tool_path_vectors_freeze,
                                            path);
      g_signal_handlers_disconnect_by_func (private->vectors,
                                            gimp_tool_path_vectors_thaw,
                                            path);

      g_object_unref (private->vectors);
    }

  private->vectors  = vectors;
  private->function = VECTORS_FINISHED;
  gimp_tool_path_verify_state (path);

  if (private->vectors)
    {
      g_object_ref (private->vectors);

      g_signal_connect_object (private->vectors, "visibility-changed",
                               G_CALLBACK (gimp_tool_path_vectors_visible),
                               path, 0);
      g_signal_connect_object (private->vectors, "freeze",
                               G_CALLBACK (gimp_tool_path_vectors_freeze),
                               path, 0);
      g_signal_connect_object (private->vectors, "thaw",
                               G_CALLBACK (gimp_tool_path_vectors_thaw),
                               path, 0);
    }

  g_object_notify (G_OBJECT (path), "vectors");
}
