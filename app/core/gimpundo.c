/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpmarshal.h"
#include "gimptempbuf.h"
#include "gimpundo.h"
#include "gimpundostack.h"

#include "gimp-priorities.h"

#include "gimp-intl.h"


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


static void          gimp_undo_constructed         (GObject             *object);
static void          gimp_undo_finalize            (GObject             *object);
static void          gimp_undo_set_property        (GObject             *object,
                                                    guint                property_id,
                                                    const GValue        *value,
                                                    GParamSpec          *pspec);
static void          gimp_undo_get_property        (GObject             *object,
                                                    guint                property_id,
                                                    GValue              *value,
                                                    GParamSpec          *pspec);

static gint64        gimp_undo_get_memsize         (GimpObject          *object,
                                                    gint64              *gui_size);

static gboolean      gimp_undo_get_popup_size      (GimpViewable        *viewable,
                                                    gint                 width,
                                                    gint                 height,
                                                    gboolean             dot_for_dot,
                                                    gint                *popup_width,
                                                    gint                *popup_height);
static GimpTempBuf * gimp_undo_get_new_preview     (GimpViewable        *viewable,
                                                    GimpContext         *context,
                                                    gint                 width,
                                                    gint                 height);

static void          gimp_undo_real_pop            (GimpUndo            *undo,
                                                    GimpUndoMode         undo_mode,
                                                    GimpUndoAccumulator *accum);
static void          gimp_undo_real_free           (GimpUndo            *undo,
                                                    GimpUndoMode         undo_mode);

static gboolean      gimp_undo_create_preview_idle (gpointer             data);
static void       gimp_undo_create_preview_private (GimpUndo            *undo,
                                                    GimpContext         *context);


G_DEFINE_TYPE (GimpUndo, gimp_undo, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_undo_parent_class

static guint undo_signals[LAST_SIGNAL] = { 0 };


static void
gimp_undo_class_init (GimpUndoClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  undo_signals[POP] =
    g_signal_new ("pop",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpUndoClass, pop),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM_POINTER,
                  G_TYPE_NONE, 2,
                  GIMP_TYPE_UNDO_MODE,
                  G_TYPE_POINTER);

  undo_signals[FREE] =
    g_signal_new ("free",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpUndoClass, free),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_UNDO_MODE);

  object_class->constructed         = gimp_undo_constructed;
  object_class->finalize            = gimp_undo_finalize;
  object_class->set_property        = gimp_undo_set_property;
  object_class->get_property        = gimp_undo_get_property;

  gimp_object_class->get_memsize    = gimp_undo_get_memsize;

  viewable_class->default_icon_name = "edit-undo";
  viewable_class->get_popup_size    = gimp_undo_get_popup_size;
  viewable_class->get_new_preview   = gimp_undo_get_new_preview;

  klass->pop                        = gimp_undo_real_pop;
  klass->free                       = gimp_undo_real_free;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_TIME,
                                   g_param_spec_uint ("time", NULL, NULL,
                                                      0, G_MAXUINT, 0,
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_UNDO_TYPE,
                                   g_param_spec_enum ("undo-type", NULL, NULL,
                                                      GIMP_TYPE_UNDO_TYPE,
                                                      GIMP_UNDO_GROUP_NONE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIRTY_MASK,
                                   g_param_spec_flags ("dirty-mask",
                                                       NULL, NULL,
                                                       GIMP_TYPE_DIRTY_MASK,
                                                       GIMP_DIRTY_NONE,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_undo_init (GimpUndo *undo)
{
  undo->time = time (NULL);
}

static void
gimp_undo_constructed (GObject *object)
{
  GimpUndo *undo = GIMP_UNDO (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_IMAGE (undo->image));
}

static void
gimp_undo_finalize (GObject *object)
{
  GimpUndo *undo = GIMP_UNDO (object);

  if (undo->preview_idle_id)
    {
      g_source_remove (undo->preview_idle_id);
      undo->preview_idle_id = 0;
    }

  g_clear_pointer (&undo->preview, gimp_temp_buf_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_undo_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpUndo *undo = GIMP_UNDO (object);

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
gimp_undo_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpUndo *undo = GIMP_UNDO (object);

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
gimp_undo_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpUndo *undo    = GIMP_UNDO (object);
  gint64    memsize = 0;

  *gui_size += gimp_temp_buf_get_memsize (undo->preview);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_undo_get_popup_size (GimpViewable *viewable,
                          gint          width,
                          gint          height,
                          gboolean      dot_for_dot,
                          gint         *popup_width,
                          gint         *popup_height)
{
  GimpUndo *undo = GIMP_UNDO (viewable);

  if (undo->preview &&
      (gimp_temp_buf_get_width  (undo->preview) > width ||
       gimp_temp_buf_get_height (undo->preview) > height))
    {
      *popup_width  = gimp_temp_buf_get_width  (undo->preview);
      *popup_height = gimp_temp_buf_get_height (undo->preview);

      return TRUE;
    }

  return FALSE;
}

static GimpTempBuf *
gimp_undo_get_new_preview (GimpViewable *viewable,
                           GimpContext  *context,
                           gint          width,
                           gint          height)
{
  GimpUndo *undo = GIMP_UNDO (viewable);

  if (undo->preview)
    {
      gint preview_width;
      gint preview_height;

      gimp_viewable_calc_preview_size (gimp_temp_buf_get_width  (undo->preview),
                                       gimp_temp_buf_get_height (undo->preview),
                                       width,
                                       height,
                                       TRUE, 1.0, 1.0,
                                       &preview_width,
                                       &preview_height,
                                       NULL);

      if (preview_width  < gimp_temp_buf_get_width  (undo->preview) &&
          preview_height < gimp_temp_buf_get_height (undo->preview))
        {
          return gimp_temp_buf_scale (undo->preview,
                                      preview_width, preview_height);
        }

      return gimp_temp_buf_copy (undo->preview);
    }

  return NULL;
}

static void
gimp_undo_real_pop (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
}

static void
gimp_undo_real_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
}

void
gimp_undo_pop (GimpUndo            *undo,
               GimpUndoMode         undo_mode,
               GimpUndoAccumulator *accum)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (accum != NULL);

  if (undo->dirty_mask != GIMP_DIRTY_NONE)
    {
      switch (undo_mode)
        {
        case GIMP_UNDO_MODE_UNDO:
          gimp_image_clean (undo->image, undo->dirty_mask);
          break;

        case GIMP_UNDO_MODE_REDO:
          gimp_image_dirty (undo->image, undo->dirty_mask);
          break;
        }
    }

  g_signal_emit (undo, undo_signals[POP], 0, undo_mode, accum);
}

void
gimp_undo_free (GimpUndo     *undo,
                GimpUndoMode  undo_mode)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));

  g_signal_emit (undo, undo_signals[FREE], 0, undo_mode);
}

typedef struct _GimpUndoIdle GimpUndoIdle;

struct _GimpUndoIdle
{
  GimpUndo    *undo;
  GimpContext *context;
};

static void
gimp_undo_idle_free (GimpUndoIdle *idle)
{
  if (idle->context)
    g_object_unref (idle->context);

  g_slice_free (GimpUndoIdle, idle);
}

void
gimp_undo_create_preview (GimpUndo    *undo,
                          GimpContext *context,
                          gboolean     create_now)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (undo->preview || undo->preview_idle_id)
    return;

  if (create_now)
    {
      gimp_undo_create_preview_private (undo, context);
    }
  else
    {
      GimpUndoIdle *idle = g_slice_new0 (GimpUndoIdle);

      idle->undo = undo;

      if (context)
        idle->context = g_object_ref (context);

      undo->preview_idle_id =
        g_idle_add_full (GIMP_PRIORITY_VIEWABLE_IDLE,
                         gimp_undo_create_preview_idle, idle,
                         (GDestroyNotify) gimp_undo_idle_free);
    }
}

static gboolean
gimp_undo_create_preview_idle (gpointer data)
{
  GimpUndoIdle *idle   = data;
  GimpUndoStack *stack = gimp_image_get_undo_stack (idle->undo->image);

  if (idle->undo == gimp_undo_stack_peek (stack))
    {
      gimp_undo_create_preview_private (idle->undo, idle->context);
    }

  idle->undo->preview_idle_id = 0;

  return FALSE;
}

static void
gimp_undo_create_preview_private (GimpUndo    *undo,
                                  GimpContext *context)
{
  GimpImage    *image = undo->image;
  GimpViewable *preview_viewable;
  GimpViewSize  preview_size;
  gint          width;
  gint          height;

  switch (undo->undo_type)
    {
    case GIMP_UNDO_GROUP_IMAGE_QUICK_MASK:
    case GIMP_UNDO_GROUP_MASK:
    case GIMP_UNDO_MASK:
      preview_viewable = GIMP_VIEWABLE (gimp_image_get_mask (image));
      break;

    default:
      preview_viewable = GIMP_VIEWABLE (image);
      break;
    }

  preview_size = image->gimp->config->undo_preview_size;

  if (gimp_image_get_width  (image) <= preview_size &&
      gimp_image_get_height (image) <= preview_size)
    {
      width  = gimp_image_get_width  (image);
      height = gimp_image_get_height (image);
    }
  else
    {
      if (gimp_image_get_width (image) > gimp_image_get_height (image))
        {
          width  = preview_size;
          height = MAX (1, (gimp_image_get_height (image) * preview_size /
                            gimp_image_get_width (image)));
        }
      else
        {
          height = preview_size;
          width  = MAX (1, (gimp_image_get_width (image) * preview_size /
                            gimp_image_get_height (image)));
        }
    }

  undo->preview = gimp_viewable_get_new_preview (preview_viewable, context,
                                                 width, height);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (undo));
}

void
gimp_undo_refresh_preview (GimpUndo    *undo,
                           GimpContext *context)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  if (undo->preview_idle_id)
    return;

  if (undo->preview)
    {
      g_clear_pointer (&undo->preview, gimp_temp_buf_unref);
      gimp_undo_create_preview (undo, context, FALSE);
    }
}

const gchar *
gimp_undo_type_to_name (GimpUndoType type)
{
  const gchar *desc;

  if (gimp_enum_get_value (GIMP_TYPE_UNDO_TYPE, type, NULL, NULL, &desc, NULL))
    return desc;
  else
    return "";
}

gboolean
gimp_undo_is_weak (GimpUndo *undo)
{
  if (! undo)
    return FALSE;

  switch (undo->undo_type)
    {
    case GIMP_UNDO_GROUP_ITEM_VISIBILITY:
    case GIMP_UNDO_GROUP_ITEM_PROPERTIES:
    case GIMP_UNDO_GROUP_LAYER_APPLY_MASK:
    case GIMP_UNDO_ITEM_VISIBILITY:
    case GIMP_UNDO_LAYER_MODE:
    case GIMP_UNDO_LAYER_OPACITY:
    case GIMP_UNDO_LAYER_MASK_APPLY:
    case GIMP_UNDO_LAYER_MASK_SHOW:
      return TRUE;
      break;

    default:
      break;
    }

  return FALSE;
}

/**
 * gimp_undo_get_age:
 * @undo:
 *
 * Return value: the time in seconds since this undo item was created
 */
gint
gimp_undo_get_age (GimpUndo *undo)
{
  guint now = time (NULL);

  g_return_val_if_fail (GIMP_IS_UNDO (undo), 0);
  g_return_val_if_fail (now >= undo->time, 0);

  return now - undo->time;
}

/**
 * gimp_undo_reset_age:
 * @undo:
 *
 * Changes the creation time of this undo item to the current time.
 */
void
gimp_undo_reset_age (GimpUndo *undo)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));

  undo->time = time (NULL);

  g_object_notify (G_OBJECT (undo), "time");
}
