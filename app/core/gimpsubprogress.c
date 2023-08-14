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

#include <glib-object.h>

#include "core-types.h"

#include "core/gimpsubprogress.h"
#include "core/gimpprogress.h"


enum
{
  PROP_0,
  PROP_PROGRESS
};


static void           gimp_sub_progress_iface_init    (GimpProgressInterface *iface);

static void           gimp_sub_progress_finalize      (GObject             *object);
static void           gimp_sub_progress_set_property  (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void           gimp_sub_progress_get_property  (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);

static GimpProgress * gimp_sub_progress_start         (GimpProgress        *progress,
                                                       gboolean             cancellable,
                                                       const gchar         *message);
static void           gimp_sub_progress_end           (GimpProgress        *progress);
static gboolean       gimp_sub_progress_is_active     (GimpProgress        *progress);
static void           gimp_sub_progress_set_text      (GimpProgress        *progress,
                                                       const gchar         *message);
static void           gimp_sub_progress_set_value     (GimpProgress        *progress,
                                                       gdouble              percentage);
static gdouble        gimp_sub_progress_get_value     (GimpProgress        *progress);
static void           gimp_sub_progress_pulse         (GimpProgress        *progress);
static GBytes       * gimp_sub_progress_get_window_id (GimpProgress        *progress);
static gboolean       gimp_sub_progress_message       (GimpProgress        *progress,
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

  object_class->finalize     = gimp_sub_progress_finalize;
  object_class->set_property = gimp_sub_progress_set_property;
  object_class->get_property = gimp_sub_progress_get_property;

  g_object_class_install_property (object_class, PROP_PROGRESS,
                                   g_param_spec_object ("progress",
                                                        NULL, NULL,
                                                        GIMP_TYPE_PROGRESS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
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

  g_clear_object (&sub->progress);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_sub_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start         = gimp_sub_progress_start;
  iface->end           = gimp_sub_progress_end;
  iface->is_active     = gimp_sub_progress_is_active;
  iface->set_text      = gimp_sub_progress_set_text;
  iface->set_value     = gimp_sub_progress_set_value;
  iface->get_value     = gimp_sub_progress_get_value;
  iface->pulse         = gimp_sub_progress_pulse;
  iface->get_window_id = gimp_sub_progress_get_window_id;
  iface->message       = gimp_sub_progress_message;
}

static void
gimp_sub_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      g_return_if_fail (sub->progress == NULL);
      sub->progress = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sub_progress_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PROGRESS:
      g_value_set_object (value, sub->progress);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpProgress *
gimp_sub_progress_start (GimpProgress *progress,
                         gboolean      cancellable,
                         const gchar  *message)
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

static GBytes *
gimp_sub_progress_get_window_id (GimpProgress *progress)
{
  GimpSubProgress *sub = GIMP_SUB_PROGRESS (progress);

  if (sub->progress)
    return gimp_progress_get_window_id (sub->progress);

  return NULL;
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

/**
 * gimp_sub_progress_new:
 * @progress: parent progress or %NULL
 *
 * GimpSubProgress implements the GimpProgress interface and can be
 * used wherever a GimpProgress is needed. It maps progress
 * information to a sub-range of its parent @progress. This is useful
 * when an action breaks down into multiple sub-actions that itself
 * need a #GimpProgress pointer. See gimp_image_scale() for an example.
 *
 * Returns: a new #GimpProgress object
 */
GimpProgress *
gimp_sub_progress_new (GimpProgress *progress)
{
  GimpSubProgress *sub;

  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);

  sub = g_object_new (GIMP_TYPE_SUB_PROGRESS,
                      "progress", progress,
                      NULL);

  return GIMP_PROGRESS (sub);
}

/**
 * gimp_sub_progress_set_range:
 * @start: start value of range on the parent process
 * @end:   end value of range on the parent process
 *
 * Sets a range on the parent progress that this @progress should be
 * mapped to.
 */
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

/**
 * gimp_sub_progress_set_step:
 * @index:     step index
 * @num_steps: number of steps
 *
 * A more convenient form of gimp_sub_progress_set_range().
 */
void
gimp_sub_progress_set_step (GimpSubProgress *progress,
                            gint             index,
                            gint             num_steps)
{
  g_return_if_fail (GIMP_IS_SUB_PROGRESS (progress));
  g_return_if_fail (index < num_steps && num_steps > 0);

  progress->start = (gdouble) index / num_steps;
  progress->end   = (gdouble) (index + 1) / num_steps;
}
