/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLink
 * Copyright (C) 2019 Jehan
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimplink.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojection.h"

#include "file/file-open.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_FILE
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

struct _GimpLinkPrivate
{
  Gimp         *gimp;
  GFile        *file;
  GFileMonitor *monitor;

  gboolean      broken;
  guint         idle_changed_source;
};

static void       gimp_link_finalize       (GObject           *object);
static void       gimp_link_get_property   (GObject           *object,
                                            guint              property_id,
                                            GValue            *value,
                                            GParamSpec        *pspec);
static void       gimp_link_set_property   (GObject           *object,
                                            guint              property_id,
                                            const GValue      *value,
                                            GParamSpec        *pspec);

static void       gimp_link_file_changed   (GFileMonitor      *monitor,
                                            GFile             *file,
                                            GFile             *other_file,
                                            GFileMonitorEvent  event_type,
                                            GimpLink          *link);
static gboolean   gimp_link_emit_changed   (gpointer           data);

G_DEFINE_TYPE_WITH_PRIVATE (GimpLink, gimp_link, GIMP_TYPE_OBJECT)

#define parent_class gimp_link_parent_class

static guint link_signals[LAST_SIGNAL] = { 0 };

static void
gimp_link_class_init (GimpLinkClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);

  link_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLinkClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize     = gimp_link_finalize;
  object_class->get_property = gimp_link_get_property;
  object_class->set_property = gimp_link_set_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_link_init (GimpLink *link)
{
  link->p          = gimp_link_get_instance_private (link);
  link->p->gimp    = NULL;
  link->p->file    = NULL;
  link->p->monitor = NULL;
  link->p->broken  = TRUE;

  link->p->idle_changed_source = 0;
}

static void
gimp_link_finalize (GObject *object)
{
  GimpLink *link = GIMP_LINK (object);

  g_clear_object (&link->p->file);
  g_clear_object (&link->p->monitor);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_link_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  GimpLink *link = GIMP_LINK (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, link->p->gimp);
      break;
    case PROP_FILE:
      g_value_set_object (value, link->p->file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_link_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpLink *link = GIMP_LINK (object);

  switch (property_id)
    {
    case PROP_GIMP:
      link->p->gimp = g_value_get_object (value);
      break;
    case PROP_FILE:
      if (link->p->file)
        g_object_unref (link->p->file);
      link->p->file = g_value_dup_object (value);
      if (link->p->monitor)
        g_object_unref (link->p->monitor);
      if (link->p->file)
        {
          gchar *basename = g_file_get_basename (link->p->file);

          link->p->monitor = g_file_monitor_file (link->p->file, G_FILE_MONITOR_NONE, NULL, NULL);
          g_signal_connect (link->p->monitor, "changed",
                            G_CALLBACK (gimp_link_file_changed),
                            link);
          gimp_object_set_name_safe (GIMP_OBJECT (object), basename);
          g_free (basename);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_link_file_changed (GFileMonitor      *monitor,
                        GFile             *file,
                        GFile             *other_file,
                        GFileMonitorEvent  event_type,
                        GimpLink          *link)
{
  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
      if (link->p->idle_changed_source == 0)
        link->p->idle_changed_source = g_idle_add_full (G_PRIORITY_LOW,
                                                        gimp_link_emit_changed,
                                                        link, NULL);
      break;
    case G_FILE_MONITOR_EVENT_CREATED:
      g_signal_emit (link, link_signals[CHANGED], 0);
      break;
    case G_FILE_MONITOR_EVENT_DELETED:
      link->p->broken = TRUE;
      break;

    default:
      /* No need to signal for changes where nothing can be done anyway.
       * In particular a file deletion, the link is broken, yet we don't
       * want to re-render.
       * Don't emit either on G_FILE_MONITOR_EVENT_CHANGED because too
       * many such events may be emitted for a single file writing.
       */
      break;
    }

}

static gboolean
gimp_link_emit_changed (gpointer data)
{
  GimpLink *link = GIMP_LINK (data);

  g_signal_emit (link, link_signals[CHANGED], 0);
  link->p->idle_changed_source = 0;

  return G_SOURCE_REMOVE;
}

/*  public functions  */

/**
 * gimp_link_new:
 * @gimp: #Gimp object.
 * @file: a #GFile object.
 *
 * Creates a new text layer.
 *
 * Return value: a new #GimpLink or %NULL in case of a problem
 **/
GimpLink *
gimp_link_new (Gimp  *gimp,
               GFile *file)
{
  GimpObject *link;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  link = g_object_new (GIMP_TYPE_LINK,
                       "gimp", gimp,
                       "file", file,
                       NULL);

  return GIMP_LINK (link);
}

GFile *
gimp_link_get_file (GimpLink *link)
{
  g_return_val_if_fail (GIMP_IS_LINK (link), NULL);

  return link->p->file;
}

void
gimp_link_set_file (GimpLink *link,
                    GFile    *file)
{
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (link)));
  g_return_if_fail (G_IS_FILE (file) || file == NULL);

  if (file && g_file_equal (file, link->p->file))
    return;

  g_object_set (link,
                "file", file,
                NULL);
}

gboolean
gimp_link_is_broken (GimpLink *link,
                     gboolean  recheck)
{
  GeglBuffer *buffer;

  if (recheck)
    {
      buffer = gimp_link_get_buffer (link, NULL, NULL);
      g_clear_object (&buffer);
    }

  return link->p->broken;
}

GimpLink *
gimp_link_duplicate (GimpLink  *link)
{
  g_return_val_if_fail (GIMP_IS_LINK (link), NULL);

  return gimp_link_new (link->p->gimp, link->p->file);
}

GeglBuffer *
gimp_link_get_buffer (GimpLink      *link,
                      GimpProgress  *progress,
                      GError       **error)
{
  GeglBuffer *buffer = NULL;

  if (link->p->file)
    {
      GimpImage         *image;
      GimpPDBStatusType  status;
      const gchar       *mime_type = NULL;

      image = file_open_image (link->p->gimp,
                               gimp_get_user_context (link->p->gimp),
                               progress,
                               link->p->file,
                               link->p->width, link->p->height,
                               FALSE, NULL,
                               /* XXX We might want interactive opening
                                * for a first opening (when done through
                                * GUI), but not for every re-render.
                                */
                               GIMP_RUN_NONINTERACTIVE,
                               &status, &mime_type, error);

      if (image && status == GIMP_PDB_SUCCESS)
        {
          /* If we don't flush the projection first, the buffer may be empty.
           * I do wonder if the flushing and updating of the link could
           * not be multi-threaded with gimp_projection_flush() instead,
           * then notifying the update through signals. For very heavy
           * images, would it be a better UX? XXX
           */
          gimp_projection_flush_now (gimp_image_get_projection (image), TRUE);
          buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (image));
          g_object_ref (buffer);
        }

      /* Only keep the buffer, free the rest. */
      g_clear_object (&image);
    }

  link->p->broken = (buffer == NULL);

  return buffer;
}
