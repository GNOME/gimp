/* GIMP - The GNU Image Manipulation Program
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

#include "core/gimpsubprogress.h"
#include "core/gimpprogress.h"


static void        gimp_sub_progress_iface_init (GimpProgressInterface *iface);
static void        gimp_sub_progress_finalize   (GObject               *object);

static GimpProgress * gimp_sub_progress_start      (GimpProgress        *progress,
                                                    const gchar         *message,
                                                    gboolean             cancelable);
static void           gimp_sub_progress_end        (GimpProgress        *progress);
static gboolean       gimp_sub_progress_is_active  (GimpProgress        *progress);
static void           gimp_sub_progress_set_text   (GimpProgress        *progress,
                                                    const gchar         *message);
static void           gimp_sub_progress_set_value  (GimpProgress        *progress,
                                                    gdouble              percentage);
static gdouble        gimp_sub_progress_get_value  (GimpProgress        *progress);
static void           gimp_sub_progress_pulse      (GimpProgress        *progress);
static guint32        gimp_sub_progress_get_window (GimpProgress        *progress);
static gboolean       gimp_sub_progress_message    (GimpProgress        *progress,
                                                    Gimp                *gimp,
                                                    GimpMessageSeverity  severity,
                                                    const gchar         *domain,
                                                    const gchar         *message);


G_DEFINE_TYPE_WITH_CODE (GimpSubProgress, gimp_sub_progress, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_sub_progress_iface_init))

#define parent_class gimp_sub_progress_parent_class


static void
gimp_sub_progress_class_init (GimpSubProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_sub_progress_finalize;
}

static void
gimp_sub_progress_init (GimpSubProgress *sub)
{
  sub->progress = NULL;
  sub->start    = 0.0;
  sub->end      = 1.0;
}

static void
gimp_sub_progress_finalize (GObject *object)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (object);

  if (sub->progress)
    {
      g_object_unref (sub->progress);
      sub->progress = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_sub_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start      = gimp_sub_progress_start;
  iface->end        = gimp_sub_progress_end;
  iface->is_active  = gimp_sub_progress_is_active;
  iface->set_text   = gimp_sub_progress_set_text;
  iface->set_value  = gimp_sub_progress_set_value;
  iface->get_value  = gimp_sub_progress_get_value;
  iface->pulse      = gimp_sub_progress_pulse;
  iface->get_window = gimp_sub_progress_get_window;
  iface->message    = gimp_sub_progress_message;
}

static GimpProgress *
gimp_sub_progress_start (GimpProgress *progress,
                         const gchar  *message,
                         gboolean      cancelable)
{
  /* does nothing */
  return NULL;
}

static void
gimp_sub_progress_end (GimpProgress *progress)
{
  /* does nothing */
}

static gboolean
gimp_sub_progress_is_active (GimpProgress *progress)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    return gimp_progress_is_active (sub->progress);

  return FALSE;
}

static void
gimp_sub_progress_set_text (GimpProgress *progress,
                            const gchar  *message)
{
  /* does nothing */
}

static void
gimp_sub_progress_set_value (GimpProgress *progress,
                             gdouble       percentage)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    gimp_progress_set_value (sub->progress,
                             sub->start + percentage * (sub->end - sub->start));
}

static gdouble
gimp_sub_progress_get_value (GimpProgress *progress)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    return gimp_progress_get_value (sub->progress);

  return 0.0;
}

static void
gimp_sub_progress_pulse (GimpProgress *progress)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    gimp_progress_pulse (sub->progress);
}

static guint32
gimp_sub_progress_get_window (GimpProgress *progress)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    return gimp_progress_get_window (sub->progress);

  return 0;
}

static gboolean
gimp_sub_progress_message (GimpProgress        *progress,
                           Gimp                *gimp,
                           GimpMessageSeverity  severity,
                           const gchar         *domain,
                           const gchar         *message)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    return gimp_progress_message (sub->progress,
                                  gimp, severity, domain, message);

  return FALSE;
}

GimpProgress *
gimp_sub_progress_new (GimpProgress *progress)
{
  GimpSubProgress *sub;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), NULL);

  sub = g_object_new (GIMP_TYPE_SUB_PROGRESS, NULL);

  sub->progress = g_object_ref (progress);

  return GIMP_PROGRESS (sub);
}

void
gimp_sub_progress_set_range (GimpSubProgress *progress,
                             gdouble          start,
                             gdouble          end)
{
  g_return_if_fail (GIMP_IS_SUB_PROGRESS (progress));
  g_return_if_fail (start < end);

  progress->start = start;
  progress->end   = end;
}

void
gimp_sub_progress_set_steps (GimpSubProgress *progress,
                             gint             num,
                             gint             steps)
{
  g_return_if_fail (GIMP_IS_SUB_PROGRESS (progress));
  g_return_if_fail (num < steps && steps > 0);

  progress->start = (gdouble) num / steps;
  progress->end   = (gdouble) (num + 1) / steps;
}
