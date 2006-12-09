/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <time.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimpundo.h"
#include "gimpundostack.h"

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
  PROP_UNDO_TYPE,
  PROP_DIRTY_MASK,
  PROP_DATA,
  PROP_SIZE,
  PROP_POP_FUNC,
  PROP_FREE_FUNC
};


static GObject * gimp_undo_constructor         (GType                type,
                                                guint                n_params,
                                                GObjectConstructParam *params);
static void      gimp_undo_finalize            (GObject             *object);
static void      gimp_undo_set_property        (GObject             *object,
                                                guint                property_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);
static void      gimp_undo_get_property        (GObject             *object,
                                                guint                property_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);

static gint64    gimp_undo_get_memsize         (GimpObject          *object,
                                                gint64              *gui_size);

static gboolean  gimp_undo_get_popup_size      (GimpViewable        *viewable,
                                                gint                 width,
                                                gint                 height,
                                                gboolean             dot_for_dot,
                                                gint                *popup_width,
                                                gint                *popup_height);
static TempBuf * gimp_undo_get_new_preview     (GimpViewable        *viewable,
                                                GimpContext         *context,
                                                gint                 width,
                                                gint                 height);

static void      gimp_undo_real_pop            (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode,
                                                GimpUndoAccumulator *accum);
static void      gimp_undo_real_free           (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode);

static gboolean  gimp_undo_create_preview_idle (gpointer             data);
static void   gimp_undo_create_preview_private (GimpUndo            *undo,
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

  object_class->constructor        = gimp_undo_constructor;
  object_class->finalize           = gimp_undo_finalize;
  object_class->set_property       = gimp_undo_set_property;
  object_class->get_property       = gimp_undo_get_property;

  gimp_object_class->get_memsize   = gimp_undo_get_memsize;

  viewable_class->default_stock_id = "gtk-undo";
  viewable_class->get_popup_size   = gimp_undo_get_popup_size;
  viewable_class->get_new_preview  = gimp_undo_get_new_preview;

  klass->pop                       = gimp_undo_real_pop;
  klass->free                      = gimp_undo_real_free;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

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

  g_object_class_install_property (object_class, PROP_DATA,
                                   g_param_spec_pointer ("data", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_int64 ("size", NULL, NULL,
                                                       0, G_MAXINT64, 0,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_POP_FUNC,
                                   g_param_spec_pointer ("pop-func", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_FREE_FUNC,
                                   g_param_spec_pointer ("free-func", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_undo_init (GimpUndo *undo)
{
  undo->time = time (NULL);
}

static GObject *
gimp_undo_constructor (GType                  type,
                       guint                  n_params,
                       GObjectConstructParam *params)
{
  GObject  *object;
  GimpUndo *undo;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  undo = GIMP_UNDO (object);

  g_assert (GIMP_IS_IMAGE (undo->image));

  return object;
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

  if (undo->preview)
    {
      temp_buf_free (undo->preview);
      undo->preview = NULL;
    }

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
    case PROP_UNDO_TYPE:
      undo->undo_type = g_value_get_enum (value);
      break;
    case PROP_DIRTY_MASK:
      undo->dirty_mask = g_value_get_flags (value);
      break;
    case PROP_DATA:
      undo->data = g_value_get_pointer (value);
      break;
    case PROP_SIZE:
      undo->size = g_value_get_int64 (value);
      break;
    case PROP_POP_FUNC:
      undo->pop_func = g_value_get_pointer (value);
      break;
    case PROP_FREE_FUNC:
      undo->free_func = g_value_get_pointer (value);
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
    case PROP_UNDO_TYPE:
      g_value_set_enum (value, undo->undo_type);
      break;
    case PROP_DIRTY_MASK:
      g_value_set_flags (value, undo->dirty_mask);
      break;
    case PROP_DATA:
      g_value_set_pointer (value, undo->data);
      break;
    case PROP_SIZE:
      g_value_set_int64 (value, undo->size);
      break;
    case PROP_POP_FUNC:
      g_value_set_pointer (value, undo->pop_func);
      break;
    case PROP_FREE_FUNC:
      g_value_set_pointer (value, undo->free_func);
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
  gint64    memsize = undo->size;

  if (undo->preview)
    *gui_size += temp_buf_get_memsize (undo->preview);

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
      (undo->preview->width > width || undo->preview->height > height))
    {
      *popup_width  = undo->preview->width;
      *popup_height = undo->preview->height;

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
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

      gimp_viewable_calc_preview_size (undo->preview->width,
                                       undo->preview->height,
                                       width,
                                       height,
                                       TRUE, 1.0, 1.0,
                                       &preview_width,
                                       &preview_height,
                                       NULL);

      if (preview_width  < undo->preview->width &&
          preview_height < undo->preview->height)
        {
          return temp_buf_scale (undo->preview, preview_width, preview_height);
        }

      return temp_buf_copy (undo->preview, NULL);
    }

  return NULL;
}

static void
gimp_undo_real_pop (GimpUndo            *undo,
                    GimpUndoMode         undo_mode,
                    GimpUndoAccumulator *accum)
{
  if (undo->pop_func)
    undo->pop_func (undo, undo_mode, accum);
}

static void
gimp_undo_real_free (GimpUndo     *undo,
                     GimpUndoMode  undo_mode)
{
  if (undo->free_func)
    undo->free_func (undo, undo_mode);
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

  g_free (idle);
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
      GimpUndoIdle *idle = g_new0 (GimpUndoIdle, 1);

      idle->undo = undo;

      if (context)
        idle->context = g_object_ref (context);

      undo->preview_idle_id =
        g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                         gimp_undo_create_preview_idle,
                         idle, (GDestroyNotify) gimp_undo_idle_free);
    }
}

static gboolean
gimp_undo_create_preview_idle (gpointer data)
{
  GimpUndoIdle *idle = data;

  if (idle->undo == gimp_undo_stack_peek (idle->undo->image->undo_stack))
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

  if (image->width <= preview_size && image->height <= preview_size)
    {
      width  = image->width;
      height = image->height;
    }
  else
    {
      if (image->width > image->height)
        {
          width  = preview_size;
          height = MAX (1, (image->height * preview_size / image->width));
        }
      else
        {
          height = preview_size;
          width  = MAX (1, (image->width * preview_size / image->height));
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
      temp_buf_free (undo->preview);
      undo->preview = NULL;
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
  GimpUndoType type;

  if (! undo)
    return FALSE;

  type = undo->undo_type;

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
