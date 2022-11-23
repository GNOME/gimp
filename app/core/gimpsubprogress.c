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

#include <glib-object.h>

#include "core-types.h"

#include "core/ligmasubprogress.h"
#include "core/ligmaprogress.h"


enum
{
  PROP_0,
  PROP_PROGRESS
};


static void           ligma_sub_progress_iface_init    (LigmaProgressInterface *iface);

static void           ligma_sub_progress_finalize      (GObject             *object);
static void           ligma_sub_progress_set_property  (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void           ligma_sub_progress_get_property  (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);

static LigmaProgress * ligma_sub_progress_start         (LigmaProgress        *progress,
                                                       gboolean             cancellable,
                                                       const gchar         *message);
static void           ligma_sub_progress_end           (LigmaProgress        *progress);
static gboolean       ligma_sub_progress_is_active     (LigmaProgress        *progress);
static void           ligma_sub_progress_set_text      (LigmaProgress        *progress,
                                                       const gchar         *message);
static void           ligma_sub_progress_set_value     (LigmaProgress        *progress,
                                                       gdouble              percentage);
static gdouble        ligma_sub_progress_get_value     (LigmaProgress        *progress);
static void           ligma_sub_progress_pulse         (LigmaProgress        *progress);
static guint32        ligma_sub_progress_get_window_id (LigmaProgress        *progress);
static gboolean       ligma_sub_progress_message       (LigmaProgress        *progress,
                                                       Ligma                *ligma,
                                                       LigmaMessageSeverity  severity,
                                                       const gchar         *domain,
                                                       const gchar         *message);


G_DEFINE_TYPE_WITH_CODE (LigmaSubProgress, ligma_sub_progress, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_sub_progress_iface_init))

#define parent_class ligma_sub_progress_parent_class


static void
ligma_sub_progress_class_init (LigmaSubProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_sub_progress_finalize;
  object_class->set_property = ligma_sub_progress_set_property;
  object_class->get_property = ligma_sub_progress_get_property;

  g_object_class_install_property (object_class, PROP_PROGRESS,
                                   g_param_spec_object ("progress",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_PROGRESS,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_sub_progress_init (LigmaSubProgress *sub)
{
  sub->progress = NULL;
  sub->start    = 0.0;
  sub->end      = 1.0;
}

static void
ligma_sub_progress_finalize (GObject *object)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (object);

  g_clear_object (&sub->progress);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_sub_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start         = ligma_sub_progress_start;
  iface->end           = ligma_sub_progress_end;
  iface->is_active     = ligma_sub_progress_is_active;
  iface->set_text      = ligma_sub_progress_set_text;
  iface->set_value     = ligma_sub_progress_set_value;
  iface->get_value     = ligma_sub_progress_get_value;
  iface->pulse         = ligma_sub_progress_pulse;
  iface->get_window_id = ligma_sub_progress_get_window_id;
  iface->message       = ligma_sub_progress_message;
}

static void
ligma_sub_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (object);

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
ligma_sub_progress_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (object);

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

static LigmaProgress *
ligma_sub_progress_start (LigmaProgress *progress,
                         gboolean      cancellable,
                         const gchar  *message)
{
  /* does nothing */
  return NULL;
}

static void
ligma_sub_progress_end (LigmaProgress *progress)
{
  /* does nothing */
}

static gboolean
ligma_sub_progress_is_active (LigmaProgress *progress)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    return ligma_progress_is_active (sub->progress);

  return FALSE;
}

static void
ligma_sub_progress_set_text (LigmaProgress *progress,
                            const gchar  *message)
{
  /* does nothing */
}

static void
ligma_sub_progress_set_value (LigmaProgress *progress,
                             gdouble       percentage)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    ligma_progress_set_value (sub->progress,
                             sub->start + percentage * (sub->end - sub->start));
}

static gdouble
ligma_sub_progress_get_value (LigmaProgress *progress)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    return ligma_progress_get_value (sub->progress);

  return 0.0;
}

static void
ligma_sub_progress_pulse (LigmaProgress *progress)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    ligma_progress_pulse (sub->progress);
}

static guint32
ligma_sub_progress_get_window_id (LigmaProgress *progress)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    return ligma_progress_get_window_id (sub->progress);

  return 0;
}

static gboolean
ligma_sub_progress_message (LigmaProgress        *progress,
                           Ligma                *ligma,
                           LigmaMessageSeverity  severity,
                           const gchar         *domain,
                           const gchar         *message)
{
  LigmaSubProgress *sub = LIGMA_SUB_PROGRESS (progress);

  if (sub->progress)
    return ligma_progress_message (sub->progress,
                                  ligma, severity, domain, message);

  return FALSE;
}

/**
 * ligma_sub_progress_new:
 * @progress: parent progress or %NULL
 *
 * LigmaSubProgress implements the LigmaProgress interface and can be
 * used wherever a LigmaProgress is needed. It maps progress
 * information to a sub-range of its parent @progress. This is useful
 * when an action breaks down into multiple sub-actions that itself
 * need a #LigmaProgress pointer. See ligma_image_scale() for an example.
 *
 * Returns: a new #LigmaProgress object
 */
LigmaProgress *
ligma_sub_progress_new (LigmaProgress *progress)
{
  LigmaSubProgress *sub;

  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), NULL);

  sub = g_object_new (LIGMA_TYPE_SUB_PROGRESS,
                      "progress", progress,
                      NULL);

  return LIGMA_PROGRESS (sub);
}

/**
 * ligma_sub_progress_set_range:
 * @start: start value of range on the parent process
 * @end:   end value of range on the parent process
 *
 * Sets a range on the parent progress that this @progress should be
 * mapped to.
 */
void
ligma_sub_progress_set_range (LigmaSubProgress *progress,
                             gdouble          start,
                             gdouble          end)
{
  g_return_if_fail (LIGMA_IS_SUB_PROGRESS (progress));
  g_return_if_fail (start < end);

  progress->start = start;
  progress->end   = end;
}

/**
 * ligma_sub_progress_set_step:
 * @index:     step index
 * @num_steps: number of steps
 *
 * A more convenient form of ligma_sub_progress_set_range().
 */
void
ligma_sub_progress_set_step (LigmaSubProgress *progress,
                            gint             index,
                            gint             num_steps)
{
  g_return_if_fail (LIGMA_IS_SUB_PROGRESS (progress));
  g_return_if_fail (index < num_steps && num_steps > 0);

  progress->start = (gdouble) index / num_steps;
  progress->end   = (gdouble) (index + 1) / num_steps;
}
