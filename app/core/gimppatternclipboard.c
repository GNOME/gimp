/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppatternclipboard.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpchannel.h"
#include "gimppatternclipboard.h"
#include "gimpimage.h"
#include "gimppickable.h"
#include "gimpselection.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


#define PATTERN_MAX_SIZE 1024

enum
{
  PROP_0,
  PROP_GIMP
};


/*  local function prototypes  */

static void       gimp_pattern_clipboard_constructed  (GObject      *object);
static void       gimp_pattern_clipboard_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void       gimp_pattern_clipboard_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static GimpData * gimp_pattern_clipboard_duplicate    (GimpData     *data);

static void       gimp_pattern_clipboard_changed      (Gimp         *gimp,
                                                       GimpPattern  *pattern);


G_DEFINE_TYPE (GimpPatternClipboard, gimp_pattern_clipboard, GIMP_TYPE_PATTERN)

#define parent_class gimp_pattern_clipboard_parent_class


static void
gimp_pattern_clipboard_class_init (GimpPatternClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpDataClass *data_class   = GIMP_DATA_CLASS (klass);

  object_class->constructed  = gimp_pattern_clipboard_constructed;
  object_class->set_property = gimp_pattern_clipboard_set_property;
  object_class->get_property = gimp_pattern_clipboard_get_property;

  data_class->duplicate      = gimp_pattern_clipboard_duplicate;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_pattern_clipboard_init (GimpPatternClipboard *pattern)
{
}

static void
gimp_pattern_clipboard_constructed (GObject *object)
{
  GimpPatternClipboard *pattern = GIMP_PATTERN_CLIPBOARD (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (pattern->gimp));

  g_signal_connect_object (pattern->gimp, "clipboard-changed",
                           G_CALLBACK (gimp_pattern_clipboard_changed),
                           pattern, 0);

  gimp_pattern_clipboard_changed (pattern->gimp, GIMP_PATTERN (pattern));
}

static void
gimp_pattern_clipboard_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpPatternClipboard *pattern = GIMP_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_GIMP:
      pattern->gimp = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pattern_clipboard_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpPatternClipboard *pattern = GIMP_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, pattern->gimp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpData *
gimp_pattern_clipboard_duplicate (GimpData *data)
{
  GimpData *new = g_object_new (GIMP_TYPE_PATTERN, NULL);

  gimp_data_copy (new, data);

  return new;
}

GimpData *
gimp_pattern_clipboard_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_PATTERN_CLIPBOARD,
                       "name", _("Clipboard Image"),
                       "gimp", gimp,
                       NULL);
}


/*  private functions  */

static void
gimp_pattern_clipboard_changed (Gimp        *gimp,
                                GimpPattern *pattern)
{
  GimpObject *paste;
  GeglBuffer *buffer       = NULL;
  gboolean    unref_buffer = FALSE;

  g_clear_pointer (&pattern->mask, gimp_temp_buf_unref);

  paste = gimp_get_clipboard_object (gimp);

  if (GIMP_IS_IMAGE (paste))
    {
      GimpContext *context = gimp_get_user_context (gimp);

      gimp_pickable_flush (GIMP_PICKABLE (paste));
      if (context)
        {
          GimpChannel *mask = gimp_image_get_mask (GIMP_IMAGE (paste));

          if (! gimp_channel_is_empty (mask))
            {
              GList *pickables;
              gint   offset_x;
              gint   offset_y;

              pickables = g_list_prepend (NULL, GIMP_IMAGE (paste));
              buffer = gimp_selection_extract (GIMP_SELECTION (mask),
                                               pickables, context,
                                               FALSE, FALSE, FALSE,
                                               &offset_x, &offset_y,
                                               NULL);
              g_list_free (pickables);

              unref_buffer = TRUE;
            }
          else
            {
              buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (paste));
            }
        }
      else
        {
          buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (paste));
        }
    }
  else if (GIMP_IS_BUFFER (paste))
    {
      buffer = gimp_buffer_get_buffer (GIMP_BUFFER (paste));
    }

  if (buffer)
    {
      gint width  = MIN (gegl_buffer_get_width  (buffer), PATTERN_MAX_SIZE);
      gint height = MIN (gegl_buffer_get_height (buffer), PATTERN_MAX_SIZE);

      pattern->mask = gimp_temp_buf_new (width, height,
                                         gegl_buffer_get_format (buffer));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       NULL,
                       gimp_temp_buf_get_data (pattern->mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (unref_buffer)
        g_object_unref (buffer);
    }
  else
    {
      pattern->mask = gimp_temp_buf_new (16, 16, babl_format ("R'G'B' u8"));
      memset (gimp_temp_buf_get_data (pattern->mask), 255, 16 * 16 * 3);
    }

  gimp_data_dirty (GIMP_DATA (pattern));
}
