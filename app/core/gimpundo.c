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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpmarshal.h"
#include "gimpundo.h"
#include "gimpundostack.h"


enum
{
  POP,
  FREE,
  LAST_SIGNAL
};


static void      gimp_undo_class_init          (GimpUndoClass       *klass);
static void      gimp_undo_init                (GimpUndo            *undo);

static void      gimp_undo_finalize            (GObject             *object);

static gint64    gimp_undo_get_memsize         (GimpObject          *object,
                                                gint64              *gui_size);

static gboolean  gimp_undo_get_popup_size      (GimpViewable        *viewable,
                                                gint                 width,
                                                gint                 height,
                                                gboolean             dot_for_dot,
                                                gint                *popup_width,
                                                gint                *popup_height);
static TempBuf * gimp_undo_get_new_preview     (GimpViewable        *viewable,
                                                gint                 width,
                                                gint                 height);

static void      gimp_undo_real_pop            (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode,
                                                GimpUndoAccumulator *accum);
static void      gimp_undo_real_free           (GimpUndo            *undo,
                                                GimpUndoMode         undo_mode);

static gboolean  gimp_undo_create_preview_idle (gpointer             data);
static void   gimp_undo_create_preview_private (GimpUndo            *undo);


static guint undo_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;


GType
gimp_undo_get_type (void)
{
  static GType undo_type = 0;

  if (! undo_type)
    {
      static const GTypeInfo undo_info =
      {
        sizeof (GimpUndoClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_undo_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpUndo),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_undo_init,
      };

      undo_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					  "GimpUndo",
					  &undo_info, 0);
  }

  return undo_type;
}

static void
gimp_undo_class_init (GimpUndoClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

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

  object_class->finalize           = gimp_undo_finalize;

  gimp_object_class->get_memsize   = gimp_undo_get_memsize;

  viewable_class->default_stock_id = "gtk-undo";
  viewable_class->get_popup_size   = gimp_undo_get_popup_size;
  viewable_class->get_new_preview  = gimp_undo_get_new_preview;

  klass->pop                       = gimp_undo_real_pop;
  klass->free                      = gimp_undo_real_free;
}

static void
gimp_undo_init (GimpUndo *undo)
{
  undo->gimage        = NULL;
  undo->undo_type     = 0;
  undo->data          = NULL;
  undo->dirties_image = FALSE;
  undo->pop_func      = NULL;
  undo->free_func     = NULL;
  undo->preview       = NULL;
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

GimpUndo *
gimp_undo_new (GimpImage        *gimage,
               GimpUndoType      undo_type,
               const gchar      *name,
               gpointer          data,
               gint64            size,
               gboolean          dirties_image,
               GimpUndoPopFunc   pop_func,
               GimpUndoFreeFunc  free_func)
{
  GimpUndo *undo;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (size == 0 || data != NULL, NULL);

  if (! name)
    name = gimp_undo_type_to_name (undo_type);

  undo = g_object_new (GIMP_TYPE_UNDO,
                       "name", name,
                       NULL);

  undo->gimage        = gimage;
  undo->undo_type     = undo_type;
  undo->data          = data;
  undo->size          = size;
  undo->dirties_image = dirties_image ? TRUE : FALSE;
  undo->pop_func      = pop_func;
  undo->free_func     = free_func;

  return undo;
}

void
gimp_undo_pop (GimpUndo            *undo,
               GimpUndoMode         undo_mode,
               GimpUndoAccumulator *accum)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));
  g_return_if_fail (accum != NULL);

  g_signal_emit (undo, undo_signals[POP], 0, undo_mode, accum);

  if (undo->dirties_image)
    {
      switch (undo_mode)
        {
        case GIMP_UNDO_MODE_UNDO:
          gimp_image_clean (undo->gimage);
          break;

        case GIMP_UNDO_MODE_REDO:
          gimp_image_dirty (undo->gimage);
          break;
        }
    }
}

void
gimp_undo_free (GimpUndo     *undo,
                GimpUndoMode  undo_mode)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));

  g_signal_emit (undo, undo_signals[FREE], 0, undo_mode);
}

void
gimp_undo_create_preview (GimpUndo  *undo,
                          gboolean   create_now)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));

  if (undo->preview || undo->preview_idle_id)
    return;

  if (create_now)
    gimp_undo_create_preview_private (undo);
  else
    undo->preview_idle_id = g_idle_add (gimp_undo_create_preview_idle, undo);
}

static gboolean
gimp_undo_create_preview_idle (gpointer data)
{
  GimpUndo *undo = GIMP_UNDO (data);

  if (undo == gimp_undo_stack_peek (undo->gimage->undo_stack))
    {
      gimp_undo_create_preview_private (undo);
    }

  undo->preview_idle_id = 0;

  return FALSE;
}

static void
gimp_undo_create_preview_private (GimpUndo *undo)
{
  GimpImage       *image = undo->gimage;
  GimpViewable    *preview_viewable;
  GimpPreviewSize  preview_size;
  gint             width;
  gint             height;

  switch (undo->undo_type)
    {
    case GIMP_UNDO_GROUP_IMAGE_QMASK:
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

  undo->preview = gimp_viewable_get_new_preview (preview_viewable,
                                                 width, height);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (undo));
}

void
gimp_undo_refresh_preview (GimpUndo *undo)
{
  g_return_if_fail (GIMP_IS_UNDO (undo));

  if (undo->preview_idle_id)
    return;

  if (undo->preview)
    {
      temp_buf_free (undo->preview);
      undo->preview = NULL;
      gimp_undo_create_preview (undo, FALSE);
    }
}

const gchar *
gimp_undo_type_to_name (GimpUndoType type)
{
  static GEnumClass *enum_class = NULL;
  GEnumValue        *value;

  if (! enum_class)
    enum_class = g_type_class_ref (GIMP_TYPE_UNDO_TYPE);

  value = g_enum_get_value (enum_class, type);

  if (value)
    return value->value_name;

  return "";
}
