/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprasterizable.c
 * Copyright (C) 2025 Jehan
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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimprasterizable.h"

#include "gimp-intl.h"


enum
{
  SET_RASTERIZED,
  LAST_SIGNAL
};


typedef struct _GimpRasterizablePrivate GimpRasterizablePrivate;

struct _GimpRasterizablePrivate
{
  gboolean rasterized;
};

#define GIMP_RASTERIZABLE_GET_PRIVATE(obj) (gimp_rasterizable_get_private ((GimpRasterizable *) (obj)))


static GimpRasterizablePrivate * gimp_rasterizable_get_private      (GimpRasterizable        *rasterizable);
static void                      gimp_rasterizable_private_finalize (GimpRasterizablePrivate *private);



G_DEFINE_INTERFACE (GimpRasterizable, gimp_rasterizable, GIMP_TYPE_DRAWABLE)


static guint gimp_rasterizable_signals[LAST_SIGNAL] = { 0, };


static void
gimp_rasterizable_default_init (GimpRasterizableInterface *iface)
{
  gimp_rasterizable_signals[SET_RASTERIZED] =
    g_signal_new ("set-rasterized",
                  GIMP_TYPE_RASTERIZABLE,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpRasterizableInterface, set_rasterized),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
}


/* Public functions */


/**
 * gimp_rasterizable_rasterize:
 * @rasterizable: an object that implements the %GimpRasterizable interface
 *
 * Rasterize @rasterizable.
 **/
void
gimp_rasterizable_rasterize (GimpRasterizable *rasterizable)
{
  GimpRasterizablePrivate *private;
  GimpImage               *image;
  gchar                   *undo_text;

  g_return_if_fail (GIMP_IS_RASTERIZABLE (rasterizable));
  g_return_if_fail (! gimp_rasterizable_is_rasterized (rasterizable));

  private = GIMP_RASTERIZABLE_GET_PRIVATE (rasterizable);
  image   = gimp_item_get_image (GIMP_ITEM (rasterizable));

  /* TRANSLATORS: the %s will be a type of item, i.e. "Text Layer". */
  undo_text = g_strdup_printf (_("Rasterize %s"),
                               GIMP_VIEWABLE_GET_CLASS (rasterizable)->default_name);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_PROPERTIES, undo_text);

  gimp_image_undo_push_rasterizable (image, NULL, rasterizable);

  private->rasterized = TRUE;
  g_signal_emit (rasterizable, gimp_rasterizable_signals[SET_RASTERIZED], 0, TRUE);

  gimp_image_undo_group_end (image);

  /* Triggers thumbnail update. */
  gimp_drawable_update_all (GIMP_DRAWABLE (rasterizable));
  /* Triggers contextual menu update. */
  gimp_image_flush (image);
}

/**
 * gimp_rasterizable_restore:
 * @rasterizable: an object that implements the %GimpRasterizable interface
 *
 * Revert the rasterization of @rasterizable.
 **/
void
gimp_rasterizable_restore (GimpRasterizable *rasterizable)
{
  GimpRasterizablePrivate *private;
  GimpImage               *image;
  gchar                   *undo_text;

  g_return_if_fail (GIMP_IS_RASTERIZABLE (rasterizable));
  g_return_if_fail (gimp_rasterizable_is_rasterized (rasterizable));

  private = GIMP_RASTERIZABLE_GET_PRIVATE (rasterizable);
  image   = gimp_item_get_image (GIMP_ITEM (rasterizable));

  /* TRANSLATORS: the %s will be a type of item, i.e. "Text Layer". */
  undo_text = g_strdup_printf (_("Revert Rasterize %s"),
                               GIMP_VIEWABLE_GET_CLASS (rasterizable)->default_name);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_PROPERTIES, undo_text);
  gimp_image_undo_push_rasterizable (image, NULL, rasterizable);
  gimp_image_undo_push_drawable_mod (image, NULL, GIMP_DRAWABLE (rasterizable), TRUE);

  private->rasterized = FALSE;
  g_signal_emit (rasterizable, gimp_rasterizable_signals[SET_RASTERIZED], 0, FALSE);

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
}

/**
 * gimp_rasterizable_is_rasterized:
 * @rasterizable: an object that implements the %GimpRasterizable interface
 *
 * Returns: whether @rasterizable is rasterized or not.
 **/
gboolean
gimp_rasterizable_is_rasterized (GimpRasterizable *rasterizable)
{
  GimpRasterizablePrivate *private;

  g_return_val_if_fail (GIMP_IS_RASTERIZABLE (rasterizable), FALSE);

  private = GIMP_RASTERIZABLE_GET_PRIVATE (rasterizable);

  return private->rasterized;
}

void
gimp_rasterizable_set_undo_rasterized (GimpRasterizable *rasterizable,
                                       gboolean          rasterized)
{
  GimpRasterizablePrivate *private;

  g_return_if_fail (GIMP_IS_RASTERIZABLE (rasterizable));

  private = GIMP_RASTERIZABLE_GET_PRIVATE (rasterizable);

  private->rasterized = rasterized;
}

/* Private functions */

static GimpRasterizablePrivate *
gimp_rasterizable_get_private (GimpRasterizable *rasterizable)
{
  GimpRasterizablePrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_RASTERIZABLE (rasterizable), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-rasterizable-private");

  private = g_object_get_qdata ((GObject *) rasterizable, private_key);

  if (! private)
    {
      private = g_slice_new0 (GimpRasterizablePrivate);

      g_object_set_qdata_full ((GObject *) rasterizable, private_key, private,
                               (GDestroyNotify) gimp_rasterizable_private_finalize);
    }

  return private;
}

static void
gimp_rasterizable_private_finalize (GimpRasterizablePrivate *private)
{
  g_slice_free (GimpRasterizablePrivate, private);
}
