/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <time.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligmacontext.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmamarshal.h"
#include "ligmatempbuf.h"
#include "ligmaundo.h"
#include "ligmaundostack.h"

#include "ligma-priorities.h"

#include "ligma-intl.h"


enum
{
  POP,
  FREE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_TIME,
  PROP_UNDO_TYPE,
  PROP_DIRTY_MASK
};


static void          ligma_undo_constructed         (GObject             *object);
static void          ligma_undo_finalize            (GObject             *object);
static void          ligma_undo_set_property        (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void          ligma_undo_get_property        (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static gint64        ligma_undo_get_memsize         (LigmaObject          *object,
                                                    gint64              *gui_size);

static gboolean      ligma_undo_get_popup_size      (LigmaViewable        *viewable,
                                                    gint                 width,
                                                    gint                 height,
                                                    gboolean             dot_for_dot,
                                                    gint                *popup_width,
                                                    gint                *popup_height);
static LigmaTempBuf * ligma_undo_get_new_preview     (LigmaViewable        *viewable,
                                                    LigmaContext         *context,
                                                    gint                 width,
                                                    gint                 height);

static void          ligma_undo_real_pop            (LigmaUndo            *undo,
                                                    LigmaUndoMode         undo_mode,
                                                    LigmaUndoAccumulator *accum);
static void          ligma_undo_real_free           (LigmaUndo            *undo,
                                                    LigmaUndoMode         undo_mode);

static gboolean      ligma_undo_create_preview_idle (gpointer             data);
static void       ligma_undo_create_preview_private (LigmaUndo            *undo,
                                                    LigmaContext         *context);


G_DEFINE_TYPE (LigmaUndo, ligma_undo, LIGMA_TYPE_VIEWABLE)

#define parent_class ligma_undo_parent_class

static guint undo_signals[LAST_SIGNAL] = { 0 };


static void
ligma_undo_class_init (LigmaUndoClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);

  undo_signals[POP] =
    g_signal_new ("pop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaUndoClass, pop),
                  NULL, NULL,
                  ligma_marshal_VOID__ENUM_POINTER,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_UNDO_MODE,
                  G_TYPE_POINTER);

  undo_signals[FREE] =
    g_signal_new ("free",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaUndoClass, free),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_UNDO_MODE);

  object_class->constructed         = ligma_undo_constructed;
  object_class->finalize            = ligma_undo_finalize;
  object_class->set_property        = ligma_undo_set_property;
  object_class->get_property        = ligma_undo_get_property;

  ligma_object_class->get_memsize    = ligma_undo_get_memsize;

  viewable_class->default_icon_name = "edit-undo";
  viewable_class->get_popup_size    = ligma_undo_get_popup_size;
  viewable_class->get_new_preview   = ligma_undo_get_new_preview;

  klass->pop                        = ligma_undo_real_pop;
  klass->free                       = ligma_undo_real_free;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TIME,
                                   g_param_spec_uint ("time", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_UNDO_TYPE,
                                   g_param_spec_enum ("undo-type", NULL, NULL,
                                                      LIGMA_TYPE_UNDO_TYPE,
                                                      LIGMA_UNDO_GROUP_NONE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIRTY_MASK,
                                   g_param_spec_flags ("dirty-mask",
                                                       NULL, NULL,
                                                       LIGMA_TYPE_DIRTY_MASK,
                                                       LIGMA_DIRTY_NONE,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_undo_init (LigmaUndo *undo)
{
  undo->time = time (NULL);
}

static void
ligma_undo_constructed (GObject *object)
{
  LigmaUndo *undo = LIGMA_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_IMAGE (undo->image));
}

static void
ligma_undo_finalize (GObject *object)
{
  LigmaUndo *undo = LIGMA_UNDO (object);

  if (undo->preview_idle_id)
    {
      g_source_remove (undo->preview_idle_id);
      undo->preview_idle_id = 0;
    }

  g_clear_pointer (&undo->preview, ligma_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_undo_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaUndo *undo = LIGMA_UNDO (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      /* don't ref */
      undo->image = g_value_get_object (value);
      break;
    case PROP_TIME:
      undo->time = g_value_get_uint (value);
      break;
    case PROP_UNDO_TYPE:
      undo->undo_type = g_value_get_enum (value);
      break;
    case PROP_DIRTY_MASK:
      undo->dirty_mask = g_value_get_flags (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_undo_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaUndo *undo = LIGMA_UNDO (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, undo->image);
      break;
    case PROP_TIME:
      g_value_set_uint (value, undo->time);
      break;
    case PROP_UNDO_TYPE:
      g_value_set_enum (value, undo->undo_type);
      break;
    case PROP_DIRTY_MASK:
      g_value_set_flags (value, undo->dirty_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_undo_get_memsize (LigmaObject *object,
                       gint64     *gui_size)
{
  LigmaUndo *undo    = LIGMA_UNDO (object);
  gint64    memsize = 0;

  *gui_size += ligma_temp_buf_get_memsize (undo->preview);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_undo_get_popup_size (LigmaViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  LigmaUndo *undo = LIGMA_UNDO (viewable);

  if (undo->preview &&
      (ligma_temp_buf_get_width  (undo->preview) > width ||
       ligma_temp_buf_get_height (undo->preview) > height))
    {
      *popup_width  = ligma_temp_buf_get_width  (undo->preview);
      *popup_height = ligma_temp_buf_get_height (undo->preview);

      return TRUE;
    }

  return FALSE;
}

static LigmaTempBuf *
ligma_undo_get_new_preview (LigmaViewable *viewable,
                           LigmaContext  *context,
                           gint          width,
                           gint          height)
{
  LigmaUndo *undo = LIGMA_UNDO (viewable);

  if (undo->preview)
    {
      gint preview_width;
      gint preview_height;

      ligma_viewable_calc_preview_size (ligma_temp_buf_get_width  (undo->preview),
                                       ligma_temp_buf_get_height (undo->preview),
                                       width,
                                       height,
                                       TRUE, 1.0, 1.0,
                                       &preview_width,
                                       &preview_height,
                                       NULL);

      if (preview_width  < ligma_temp_buf_get_width  (undo->preview) &&
          preview_height < ligma_temp_buf_get_height (undo->preview))
        {
          return ligma_temp_buf_scale (undo->preview,
                                      preview_width, preview_height);
        }

      return ligma_temp_buf_copy (undo->preview);
    }

  return NULL;
}

static void
ligma_undo_real_pop (LigmaUndo            *undo,
                    LigmaUndoMode         undo_mode,
                    LigmaUndoAccumulator *accum)
{
}

static void
ligma_undo_real_free (LigmaUndo     *undo,
                     LigmaUndoMode  undo_mode)
{
}

void
ligma_undo_pop (LigmaUndo            *undo,
               LigmaUndoMode         undo_mode,
               LigmaUndoAccumulator *accum)
{
  g_return_if_fail (LIGMA_IS_UNDO (undo));
  g_return_if_fail (accum != NULL);

  if (undo->dirty_mask != LIGMA_DIRTY_NONE)
    {
      switch (undo_mode)
        {
        case LIGMA_UNDO_MODE_UNDO:
          ligma_image_clean (undo->image, undo->dirty_mask);
          break;

        case LIGMA_UNDO_MODE_REDO:
          ligma_image_dirty (undo->image, undo->dirty_mask);
          break;
        }
    }

  g_signal_emit (undo, undo_signals[POP], 0, undo_mode, accum);
}

void
ligma_undo_free (LigmaUndo     *undo,
                LigmaUndoMode  undo_mode)
{
  g_return_if_fail (LIGMA_IS_UNDO (undo));

  g_signal_emit (undo, undo_signals[FREE], 0, undo_mode);
}

typedef struct _LigmaUndoIdle LigmaUndoIdle;

struct _LigmaUndoIdle
{
  LigmaUndo    *undo;
  LigmaContext *context;
};

static void
ligma_undo_idle_free (LigmaUndoIdle *idle)
{
  if (idle->context)
    g_object_unref (idle->context);

  g_slice_free (LigmaUndoIdle, idle);
}

void
ligma_undo_create_preview (LigmaUndo    *undo,
                          LigmaContext *context,
                          gboolean     create_now)
{
  g_return_if_fail (LIGMA_IS_UNDO (undo));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  if (undo->preview || undo->preview_idle_id)
    return;

  if (create_now)
    {
      ligma_undo_create_preview_private (undo, context);
    }
  else
    {
      LigmaUndoIdle *idle = g_slice_new0 (LigmaUndoIdle);

      idle->undo = undo;

      if (context)
        idle->context = g_object_ref (context);

      undo->preview_idle_id =
        g_idle_add_full (LIGMA_PRIORITY_VIEWABLE_IDLE,
                         ligma_undo_create_preview_idle, idle,
                         (GDestroyNotify) ligma_undo_idle_free);
    }
}

static gboolean
ligma_undo_create_preview_idle (gpointer data)
{
  LigmaUndoIdle *idle   = data;
  LigmaUndoStack *stack = ligma_image_get_undo_stack (idle->undo->image);

  if (idle->undo == ligma_undo_stack_peek (stack))
    {
      ligma_undo_create_preview_private (idle->undo, idle->context);
    }

  idle->undo->preview_idle_id = 0;

  return FALSE;
}

static void
ligma_undo_create_preview_private (LigmaUndo    *undo,
                                  LigmaContext *context)
{
  LigmaImage    *image = undo->image;
  LigmaViewable *preview_viewable;
  LigmaViewSize  preview_size;
  gint          width;
  gint          height;

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_GROUP_IMAGE_QUICK_MASK:
    case LIGMA_UNDO_GROUP_MASK:
    case LIGMA_UNDO_MASK:
      preview_viewable = LIGMA_VIEWABLE (ligma_image_get_mask (image));
      break;

    default:
      preview_viewable = LIGMA_VIEWABLE (image);
      break;
    }

  preview_size = image->ligma->config->undo_preview_size;

  if (ligma_image_get_width  (image) <= preview_size &&
      ligma_image_get_height (image) <= preview_size)
    {
      width  = ligma_image_get_width  (image);
      height = ligma_image_get_height (image);
    }
  else
    {
      if (ligma_image_get_width (image) > ligma_image_get_height (image))
        {
          width  = preview_size;
          height = MAX (1, (ligma_image_get_height (image) * preview_size /
                            ligma_image_get_width (image)));
        }
      else
        {
          height = preview_size;
          width  = MAX (1, (ligma_image_get_width (image) * preview_size /
                            ligma_image_get_height (image)));
        }
    }

  undo->preview = ligma_viewable_get_new_preview (preview_viewable, context,
                                                 width, height);

  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (undo));
}

void
ligma_undo_refresh_preview (LigmaUndo    *undo,
                           LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_UNDO (undo));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  if (undo->preview_idle_id)
    return;

  if (undo->preview)
    {
      g_clear_pointer (&undo->preview, ligma_temp_buf_unref);
      ligma_undo_create_preview (undo, context, FALSE);
    }
}

const gchar *
ligma_undo_type_to_name (LigmaUndoType type)
{
  const gchar *desc;

  if (ligma_enum_get_value (LIGMA_TYPE_UNDO_TYPE, type, NULL, NULL, &desc, NULL))
    return desc;
  else
    return "";
}

gboolean
ligma_undo_is_weak (LigmaUndo *undo)
{
  if (! undo)
    return FALSE;

  switch (undo->undo_type)
    {
    case LIGMA_UNDO_GROUP_ITEM_VISIBILITY:
    case LIGMA_UNDO_GROUP_ITEM_PROPERTIES:
    case LIGMA_UNDO_GROUP_LAYER_APPLY_MASK:
    case LIGMA_UNDO_ITEM_VISIBILITY:
    case LIGMA_UNDO_LAYER_MODE:
    case LIGMA_UNDO_LAYER_OPACITY:
    case LIGMA_UNDO_LAYER_MASK_APPLY:
    case LIGMA_UNDO_LAYER_MASK_SHOW:
      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}

/**
 * ligma_undo_get_age:
 * @undo:
 *
 * Returns: the time in seconds since this undo item was created
 */
gint
ligma_undo_get_age (LigmaUndo *undo)
{
  guint now = time (NULL);

  g_return_val_if_fail (LIGMA_IS_UNDO (undo), 0);
  g_return_val_if_fail (now >= undo->time, 0);

  return now - undo->time;
}

/**
 * ligma_undo_reset_age:
 * @undo:
 *
 * Changes the creation time of this undo item to the current time.
 */
void
ligma_undo_reset_age (LigmaUndo *undo)
{
  g_return_if_fail (LIGMA_IS_UNDO (undo));

  undo->time = time (NULL);

  g_object_notify (G_OBJECT (undo), "time");
}
