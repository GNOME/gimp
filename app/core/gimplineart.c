/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2017 Sébastien Fourey & David Tchumperlé
 * Copyright (C) 2018 Jehan
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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp-parallel.h"
#include "gimp-priorities.h"
#include "gimp-utils.h" /* GIMP_TIMER */
#include "gimpasync.h"
#include "gimpcancelable.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimplineart.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprojection.h"
#include "gimpviewable.h"
#include "gimpwaitable.h"

#include "gimp-intl.h"

enum
{
  COMPUTING_START,
  COMPUTING_END,
  LAST_SIGNAL,
};

enum
{
  PROP_0,
  PROP_SELECT_TRANSPARENT,
  PROP_MAX_GROW,
  PROP_THRESHOLD,
  PROP_SPLINE_MAX_LEN,
  PROP_SEGMENT_MAX_LEN,
};

typedef struct _GimpLineArtPrivate GimpLineArtPrivate;

struct _GimpLineArtPrivate
{
  gboolean      frozen;
  gboolean      compute_after_thaw;

  GimpAsync    *async;

  gint          idle_id;

  GimpPickable *input;
  GeglBuffer   *closed;
  gfloat       *distmap;

  /* Used in the closing step. */
  gboolean      select_transparent;
  gdouble       threshold;
  gint          spline_max_len;
  gint          segment_max_len;
  gboolean      max_len_bound;

  /* Used in the grow step. */
  gint          max_grow;
};

typedef struct
{
  GeglBuffer  *buffer;

  gboolean     select_transparent;
  gdouble      threshold;
  gint         spline_max_len;
  gint         segment_max_len;
} LineArtData;

typedef struct
{
  GeglBuffer *closed;
  gfloat     *distmap;
} LineArtResult;

static int DeltaX[4] = {+1, -1, 0, 0};
static int DeltaY[4] = {0, 0, +1, -1};

static const GimpVector2 Direction2Normal[4] =
{
    {  1.0f,  0.0f },
    { -1.0f,  0.0f },
    {  0.0f,  1.0f },
    {  0.0f, -1.0f }
};

typedef enum _Direction
{
  XPlusDirection  = 0,
  XMinusDirection = 1,
  YPlusDirection  = 2,
  YMinusDirection = 3
} Direction;

typedef GimpVector2 Pixel;

typedef struct _SplineCandidate
{
  Pixel p1;
  Pixel p2;
  float quality;
} SplineCandidate;

typedef struct _Edgel
{
  gint      x, y;
  Direction direction;

  gfloat    x_normal;
  gfloat    y_normal;
  gfloat    curvature;
  guint     next, previous;
} Edgel;


static void            gimp_line_art_finalize                  (GObject               *object);
static void            gimp_line_art_set_property              (GObject                *object,
                                                                guint                   property_id,
                                                                const GValue           *value,
                                                                GParamSpec             *pspec);
static void            gimp_line_art_get_property              (GObject                *object,
                                                                guint                   property_id,
                                                                GValue                 *value,
                                                                GParamSpec             *pspec);

/* Functions for asynchronous computation. */

static void            gimp_line_art_compute                   (GimpLineArt            *line_art);
static void            gimp_line_art_compute_cb                (GimpAsync              *async,
                                                                GimpLineArt            *line_art);

static GimpAsync     * gimp_line_art_prepare_async             (GimpLineArt            *line_art,
                                                                gint                    priority);
static void            gimp_line_art_prepare_async_func        (GimpAsync              *async,
                                                                LineArtData            *data);
static LineArtData   * line_art_data_new                       (GeglBuffer             *buffer,
                                                                GimpLineArt            *line_art);
static void            line_art_data_free                      (LineArtData            *data);
static LineArtResult * line_art_result_new                     (GeglBuffer             *line_art,
                                                                gfloat                 *distmap);
static void            line_art_result_free                    (LineArtResult          *result);

static gboolean        gimp_line_art_idle                      (GimpLineArt            *line_art);
static void            gimp_line_art_input_invalidate_preview  (GimpViewable           *viewable,
                                                                GimpLineArt            *line_art);


/* All actual computation functions. */

static GeglBuffer    * gimp_line_art_close                     (GeglBuffer             *buffer,
                                                                gboolean                select_transparent,
                                                                gdouble                 stroke_threshold,
                                                                gint                    spline_max_length,
                                                                gint                    segment_max_length,
                                                                gint                    minimal_lineart_area,
                                                                gint                    normal_estimate_mask_size,
                                                                gfloat                  end_point_rate,
                                                                gfloat                  spline_max_angle,
                                                                gint                    end_point_connectivity,
                                                                gfloat                  spline_roundness,
                                                                gboolean                allow_self_intersections,
                                                                gint                    created_regions_significant_area,
                                                                gint                    created_regions_minimum_area,
                                                                gboolean                small_segments_from_spline_sources,
                                                                gfloat                **lineart_distmap,
                                                                GimpAsync              *async);

static void            gimp_lineart_denoise                    (GeglBuffer             *buffer,
                                                                int                     size,
                                                                GimpAsync              *async);
static void            gimp_lineart_compute_normals_curvatures (GeglBuffer             *mask,
                                                                gfloat                 *normals,
                                                                gfloat                 *curvatures,
                                                                gfloat                 *smoothed_curvatures,
                                                                int                     normal_estimate_mask_size,
                                                                GimpAsync              *async);
static gfloat        * gimp_lineart_get_smooth_curvatures      (GArray                 *edgelset,
                                                                GimpAsync              *async);
static GArray        * gimp_lineart_curvature_extremums        (gfloat                 *curvatures,
                                                                gfloat                 *smoothed_curvatures,
                                                                gint                    curvatures_width,
                                                                gint                    curvatures_height,
                                                                GimpAsync              *async);
static gint            gimp_spline_candidate_cmp               (const SplineCandidate  *a,
                                                                const SplineCandidate  *b,
                                                                gpointer                user_data);
static GList         * gimp_lineart_find_spline_candidates     (GArray                 *max_positions,
                                                                gfloat                 *normals,
                                                                gint                    width,
                                                                gint                    distance_threshold,
                                                                gfloat                  max_angle_deg,
                                                                GimpAsync              *async);

static GArray        * gimp_lineart_discrete_spline            (Pixel                   p0,
                                                                GimpVector2             n0,
                                                                Pixel                   p1,
                                                                GimpVector2             n1);

static gint            gimp_number_of_transitions               (GArray                 *pixels,
                                                                 GeglBuffer             *buffer);
static gboolean        gimp_line_art_allow_closure              (GeglBuffer             *mask,
                                                                 GArray                 *pixels,
                                                                 GList                 **fill_pixels,
                                                                 int                     significant_size,
                                                                 int                     minimum_size);
static GArray        * gimp_lineart_line_segment_until_hit      (const GeglBuffer       *buffer,
                                                                 Pixel                   start,
                                                                 GimpVector2             direction,
                                                                 int                     size);
static gfloat        * gimp_lineart_estimate_strokes_radii      (GeglBuffer             *mask,
                                                                 GimpAsync              *async);
static void            gimp_line_art_simple_fill                (GeglBuffer             *buffer,
                                                                 gint                    x,
                                                                 gint                    y);

/* Some callback-type functions. */

static guint           visited_hash_fun                         (Pixel                  *key);
static gboolean        visited_equal_fun                        (Pixel                  *e1,
                                                                 Pixel                  *e2);

static inline gboolean border_in_direction                      (GeglBuffer             *mask,
                                                                 Pixel                   p,
                                                                 int                     direction);
static inline GimpVector2 pair2normal                           (Pixel                   p,
                                                                 gfloat                 *normals,
                                                                 gint                    width);

/* Edgel */

static Edgel    * gimp_edgel_new                  (int                x,
                                                   int                y,
                                                   Direction          direction);
static void       gimp_edgel_init                 (Edgel             *edgel);
static void       gimp_edgel_clear                (Edgel            **edgel);
static int        gimp_edgel_cmp                  (const Edgel       *e1,
                                                   const Edgel       *e2);
static guint      edgel2index_hash_fun            (Edgel              *key);
static gboolean   edgel2index_equal_fun           (Edgel              *e1,
                                                   Edgel              *e2);

static glong      gimp_edgel_track_mark           (GeglBuffer         *mask,
                                                   Edgel               edgel,
                                                   long                size_limit);
static glong      gimp_edgel_region_area          (const GeglBuffer   *mask,
                                                   Edgel               start_edgel);

/* Edgel set */

static GArray   * gimp_edgelset_new               (GeglBuffer         *buffer,
                                                   GimpAsync          *async);
static void       gimp_edgelset_add               (GArray             *set,
                                                   int                 x,
                                                   int                 y,
                                                   Direction           direction,
                                                   GHashTable         *edgel2index);
static void       gimp_edgelset_init_normals      (GArray             *set);
static void       gimp_edgelset_smooth_normals    (GArray             *set,
                                                   int                 mask_size,
                                                   GimpAsync          *async);
static void       gimp_edgelset_compute_curvature (GArray             *set,
                                                   GimpAsync          *async);

static void       gimp_edgelset_build_graph       (GArray            *set,
                                                   GeglBuffer        *buffer,
                                                   GHashTable        *edgel2index,
                                                   GimpAsync         *async);
static void       gimp_edgelset_next8             (const GeglBuffer  *buffer,
                                                   Edgel             *it,
                                                   Edgel             *n);

G_DEFINE_TYPE_WITH_CODE (GimpLineArt, gimp_line_art, GIMP_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpLineArt))

static guint gimp_line_art_signals[LAST_SIGNAL] = { 0 };

static void
gimp_line_art_class_init (GimpLineArtClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_line_art_signals[COMPUTING_START] =
    g_signal_new ("computing-start",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLineArtClass, computing_start),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  gimp_line_art_signals[COMPUTING_END] =
    g_signal_new ("computing-end",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpLineArtClass, computing_end),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize     = gimp_line_art_finalize;
  object_class->set_property = gimp_line_art_set_property;
  object_class->get_property = gimp_line_art_get_property;

  g_object_class_install_property (object_class, PROP_SELECT_TRANSPARENT,
                                   g_param_spec_boolean ("select-transparent",
                                                         _("Select transparent pixels instead of gray ones"),
                                                         _("Select transparent pixels instead of gray ones"),
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT | GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_THRESHOLD,
                                   g_param_spec_double ("threshold",
                                                        _("Line art detection threshold"),
                                                        _("Threshold to detect contour (higher values will include more pixels)"),
                                                        0.0, 1.0, 0.92,
                                                        G_PARAM_CONSTRUCT | GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MAX_GROW,
                                   g_param_spec_int ("max-grow",
                                                     _("Maximum growing size"),
                                                     _("Maximum number of pixels grown under the line art"),
                                                     1, 100, 3,
                                                     G_PARAM_CONSTRUCT | GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SPLINE_MAX_LEN,
                                   g_param_spec_int ("spline-max-length",
                                                     _("Maximum curved closing length"),
                                                     _("Maximum curved length (in pixels) to close the line art"),
                                                     0, 1000, 100,
                                                     G_PARAM_CONSTRUCT | GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SEGMENT_MAX_LEN,
                                   g_param_spec_int ("segment-max-length",
                                                     _("Maximum straight closing length"),
                                                     _("Maximum straight length (in pixels) to close the line art"),
                                                     0, 1000, 100,
                                                     G_PARAM_CONSTRUCT | GIMP_PARAM_READWRITE));
}

static void
gimp_line_art_init (GimpLineArt *line_art)
{
  line_art->priv = gimp_line_art_get_instance_private (line_art);
}

static void
gimp_line_art_finalize (GObject *object)
{
  GimpLineArt *line_art = GIMP_LINE_ART (object);

  line_art->priv->frozen = FALSE;

  gimp_line_art_set_input (line_art, NULL);
}

static void
gimp_line_art_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpLineArt *line_art = GIMP_LINE_ART (object);

  switch (property_id)
    {
    case PROP_SELECT_TRANSPARENT:
      if (line_art->priv->select_transparent != g_value_get_boolean (value))
        {
          line_art->priv->select_transparent = g_value_get_boolean (value);
          gimp_line_art_compute (line_art);
        }
      break;
    case PROP_MAX_GROW:
      line_art->priv->max_grow = g_value_get_int (value);
      break;
    case PROP_THRESHOLD:
      if (line_art->priv->threshold != g_value_get_double (value))
        {
          line_art->priv->threshold = g_value_get_double (value);
          gimp_line_art_compute (line_art);
        }
      break;
    case PROP_SPLINE_MAX_LEN:
      if (line_art->priv->spline_max_len != g_value_get_int (value))
        {
          line_art->priv->spline_max_len = g_value_get_int (value);
          if (line_art->priv->max_len_bound)
            line_art->priv->segment_max_len = line_art->priv->spline_max_len;
          gimp_line_art_compute (line_art);
        }
      break;
    case PROP_SEGMENT_MAX_LEN:
      if (line_art->priv->segment_max_len != g_value_get_int (value))
        {
          line_art->priv->segment_max_len = g_value_get_int (value);
          if (line_art->priv->max_len_bound)
            line_art->priv->spline_max_len = line_art->priv->segment_max_len;
          gimp_line_art_compute (line_art);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_line_art_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpLineArt *line_art = GIMP_LINE_ART (object);

  switch (property_id)
    {
    case PROP_SELECT_TRANSPARENT:
      g_value_set_boolean (value, line_art->priv->select_transparent);
      break;
    case PROP_MAX_GROW:
      g_value_set_int (value, line_art->priv->max_grow);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, line_art->priv->threshold);
      break;
    case PROP_SPLINE_MAX_LEN:
      g_value_set_int (value, line_art->priv->spline_max_len);
      break;
    case PROP_SEGMENT_MAX_LEN:
      g_value_set_int (value, line_art->priv->segment_max_len);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public functions */

GimpLineArt *
gimp_line_art_new (void)
{
  return g_object_new (GIMP_TYPE_LINE_ART,
                       NULL);
}

void
gimp_line_art_bind_gap_length (GimpLineArt *line_art,
                               gboolean     bound)
{
  line_art->priv->max_len_bound = bound;
}

void
gimp_line_art_set_input (GimpLineArt  *line_art,
                         GimpPickable *pickable)
{
  g_return_if_fail (pickable == NULL || GIMP_IS_VIEWABLE (pickable));

  if (pickable != line_art->priv->input)
    {
      if (line_art->priv->input)
        g_signal_handlers_disconnect_by_data (line_art->priv->input, line_art);

      g_set_object (&line_art->priv->input, pickable);

      gimp_line_art_compute (line_art);

      if (pickable)
        {
          g_signal_connect (pickable, "invalidate-preview",
                            G_CALLBACK (gimp_line_art_input_invalidate_preview),
                            line_art);
        }
    }
}

GimpPickable *
gimp_line_art_get_input (GimpLineArt *line_art)
{
  return line_art->priv->input;
}

void
gimp_line_art_freeze (GimpLineArt *line_art)
{
  g_return_if_fail (! line_art->priv->frozen);

  line_art->priv->frozen             = TRUE;
  line_art->priv->compute_after_thaw = FALSE;
}

void
gimp_line_art_thaw (GimpLineArt *line_art)
{
  g_return_if_fail (line_art->priv->frozen);

  line_art->priv->frozen = FALSE;
  if (line_art->priv->compute_after_thaw)
    {
      gimp_line_art_compute (line_art);
      line_art->priv->compute_after_thaw = FALSE;
    }
}

gboolean
gimp_line_art_is_frozen (GimpLineArt *line_art)
{
  return line_art->priv->frozen;
}

GeglBuffer *
gimp_line_art_get (GimpLineArt  *line_art,
                   gfloat      **distmap)
{
  g_return_val_if_fail (line_art->priv->input, NULL);

  if (line_art->priv->async)
    {
      gimp_waitable_wait (GIMP_WAITABLE (line_art->priv->async));
    }
  else if (! line_art->priv->closed)
    {
      gimp_line_art_compute (line_art);
      if (line_art->priv->async)
        gimp_waitable_wait (GIMP_WAITABLE (line_art->priv->async));
    }

  g_return_val_if_fail (line_art->priv->closed, NULL);

  if (distmap)
    *distmap = line_art->priv->distmap;

  return line_art->priv->closed;
}

/* Functions for asynchronous computation. */

static void
gimp_line_art_compute (GimpLineArt *line_art)
{
  if (line_art->priv->frozen)
    {
      line_art->priv->compute_after_thaw = TRUE;
      return;
    }

  if (line_art->priv->async)
    {
      /* we cancel the async, but don't wait for it to finish, since
       * it might take a while to respond.  instead gimp_line_art_compute_cb()
       * bails if the async has been canceled, to avoid accessing the line art.
       */
      g_signal_emit (line_art, gimp_line_art_signals[COMPUTING_END], 0);
      gimp_cancelable_cancel (GIMP_CANCELABLE (line_art->priv->async));
      g_clear_object (&line_art->priv->async);
    }

  if (line_art->priv->idle_id)
    {
      g_source_remove (line_art->priv->idle_id);
      line_art->priv->idle_id = 0;
    }

  g_clear_object (&line_art->priv->closed);
  g_clear_pointer (&line_art->priv->distmap, g_free);

  if (line_art->priv->input)
    {
      /* gimp_line_art_prepare_async() will flush the pickable, which
       * may trigger this signal handler, and will leak a line art (as
       * line_art->priv->async has not been set yet).
       */
      g_signal_handlers_block_by_func (
        line_art->priv->input,
        G_CALLBACK (gimp_line_art_input_invalidate_preview),
        line_art);
      line_art->priv->async = gimp_line_art_prepare_async (line_art, +1);
      g_signal_emit (line_art, gimp_line_art_signals[COMPUTING_START], 0);
      g_signal_handlers_unblock_by_func (
        line_art->priv->input,
        G_CALLBACK (gimp_line_art_input_invalidate_preview),
        line_art);

      gimp_async_add_callback_for_object (line_art->priv->async,
                                          (GimpAsyncCallback) gimp_line_art_compute_cb,
                                          line_art, line_art);
    }
}

static void
gimp_line_art_compute_cb (GimpAsync   *async,
                          GimpLineArt *line_art)
{
  if (gimp_async_is_canceled (async))
    return;

  if (gimp_async_is_finished (async))
    {
      LineArtResult *result;

      result = gimp_async_get_result (async);

      line_art->priv->closed  = g_object_ref (result->closed);
      line_art->priv->distmap = result->distmap;
      result->distmap  = NULL;
      g_signal_emit (line_art, gimp_line_art_signals[COMPUTING_END], 0);
    }

  g_clear_object (&line_art->priv->async);
}

static GimpAsync *
gimp_line_art_prepare_async (GimpLineArt *line_art,
                             gint         priority)
{
  GeglBuffer  *buffer;
  GimpAsync   *async;
  LineArtData *data;

  g_return_val_if_fail (GIMP_IS_PICKABLE (line_art->priv->input), NULL);

  gimp_pickable_flush (line_art->priv->input);

  buffer = gimp_gegl_buffer_dup (
    gimp_pickable_get_buffer (line_art->priv->input));

  data  = line_art_data_new (buffer, line_art);

  g_object_unref (buffer);

  async = gimp_parallel_run_async_full (
    priority,
    (GimpParallelRunAsyncFunc) gimp_line_art_prepare_async_func,
    data, (GDestroyNotify) line_art_data_free);

  return async;
}

static void
gimp_line_art_prepare_async_func (GimpAsync   *async,
                                  LineArtData *data)
{
  GeglBuffer *buffer;
  GeglBuffer *closed  = NULL;
  gfloat     *distmap = NULL;
  gint        buffer_x;
  gint        buffer_y;
  gboolean    has_alpha;
  gboolean    select_transparent = FALSE;

  has_alpha = babl_format_has_alpha (gegl_buffer_get_format (data->buffer));

  if (has_alpha)
    {
      if (data->select_transparent)
        {
          /*  don't select transparent regions if there are no fully
           *  transparent pixels.
           */
          GeglBufferIterator *gi;

          gi = gegl_buffer_iterator_new (data->buffer, NULL, 0,
                                         babl_format ("A u8"),
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);
          while (gegl_buffer_iterator_next (gi))
            {
              guint8 *p = (guint8*) gi->items[0].data;
              gint    k;

              if (gimp_async_is_canceled (async))
                {
                  gegl_buffer_iterator_stop (gi);

                  gimp_async_abort (async);

                  line_art_data_free (data);

                  return;
                }

              for (k = 0; k < gi->length; k++)
                {
                  if (! *p)
                    {
                      select_transparent = TRUE;
                      break;
                    }
                  p++;
                }
              if (select_transparent)
                break;
            }
          if (select_transparent)
            gegl_buffer_iterator_stop (gi);
        }
    }

  buffer   = data->buffer;
  buffer_x = gegl_buffer_get_x (data->buffer);
  buffer_y = gegl_buffer_get_y (data->buffer);

  if (buffer_x != 0 || buffer_y != 0)
    {
      buffer = g_object_new (GEGL_TYPE_BUFFER,
                             "source",  buffer,
                             "shift-x", buffer_x,
                             "shift-y", buffer_y,
                             NULL);
    }

  /* For smart selection, we generate a binarized image with close
   * regions, then run a composite selection with no threshold on
   * this intermediate buffer.
   */
  GIMP_TIMER_START();

  closed = gimp_line_art_close (buffer,
                                select_transparent,
                                data->threshold,
                                data->spline_max_len,
                                data->segment_max_len,
                                /*minimal_lineart_area,*/
                                5,
                                /*normal_estimate_mask_size,*/
                                5,
                                /*end_point_rate,*/
                                0.85,
                                /*spline_max_angle,*/
                                90.0,
                                /*end_point_connectivity,*/
                                2,
                                /*spline_roundness,*/
                                1.0,
                                /*allow_self_intersections,*/
                                TRUE,
                                /*created_regions_significant_area,*/
                                4,
                                /*created_regions_minimum_area,*/
                                100,
                                /*small_segments_from_spline_sources,*/
                                TRUE,
                                &distmap,
                                async);

  GIMP_TIMER_END("close line-art");

  if (buffer != data->buffer)
    g_object_unref (buffer);

  if (! gimp_async_is_stopped (async))
    {
      if (buffer_x != 0 || buffer_y != 0)
        {
          buffer = g_object_new (GEGL_TYPE_BUFFER,
                                 "source",  closed,
                                 "shift-x", -buffer_x,
                                 "shift-y", -buffer_y,
                                 NULL);

          g_object_unref (closed);

          closed = buffer;
        }

      gimp_async_finish_full (async,
                              line_art_result_new (closed, distmap),
                              (GDestroyNotify) line_art_result_free);
    }

  line_art_data_free (data);
}

static LineArtData *
line_art_data_new (GeglBuffer  *buffer,
                   GimpLineArt *line_art)
{
  LineArtData *data = g_slice_new (LineArtData);

  data->buffer             = g_object_ref (buffer);
  data->select_transparent = line_art->priv->select_transparent;
  data->threshold          = line_art->priv->threshold;
  data->spline_max_len     = line_art->priv->spline_max_len;
  data->segment_max_len    = line_art->priv->segment_max_len;

  return data;
}

static void
line_art_data_free (LineArtData *data)
{
  g_object_unref (data->buffer);

  g_slice_free (LineArtData, data);
}

static LineArtResult *
line_art_result_new (GeglBuffer *closed,
                     gfloat     *distmap)
{
  LineArtResult *data;

  data = g_slice_new (LineArtResult);
  data->closed  = closed;
  data->distmap = distmap;

  return data;
}

static void
line_art_result_free (LineArtResult *data)
{
  g_object_unref (data->closed);
  g_clear_pointer (&data->distmap, g_free);

  g_slice_free (LineArtResult, data);
}

static gboolean
gimp_line_art_idle (GimpLineArt *line_art)
{
  line_art->priv->idle_id = 0;

  gimp_line_art_compute (line_art);

  return G_SOURCE_REMOVE;
}

static void
gimp_line_art_input_invalidate_preview (GimpViewable *viewable,
                                        GimpLineArt  *line_art)
{
  if (! line_art->priv->idle_id)
    {
      line_art->priv->idle_id = g_idle_add_full (
        GIMP_PRIORITY_VIEWABLE_IDLE,
        (GSourceFunc) gimp_line_art_idle,
        line_art, NULL);
    }
}

/* All actual computation functions. */

/**
 * gimp_line_art_close:
 * @buffer: the input #GeglBuffer.
 * @select_transparent: whether we binarize the alpha channel or the
 *                      luminosity.
 * @stroke_threshold: [0-1] threshold value for detecting stroke pixels
 *                    (higher values will detect more stroke pixels).
 * @spline_max_length: the maximum length for creating splines between
 *                     end points.
 * @segment_max_length: the maximum length for creating segments
 *                      between end points. Unlike splines, segments
 *                      are straight lines.
 * @minimal_lineart_area: the minimum size in number pixels for area to
 *                        be considered as line art.
 * @normal_estimate_mask_size:
 * @end_point_rate: threshold to estimate if a curvature is an end-point
 *                  in [0-1] range value.
 * @spline_max_angle: the maximum angle between end point normals for
 *                    creating splines between them.
 * @end_point_connectivity:
 * @spline_roundness:
 * @allow_self_intersections: whether to allow created splines and
 *                            segments to intersect.
 * @created_regions_significant_area:
 * @created_regions_minimum_area:
 * @small_segments_from_spline_sources:
 * @closed_distmap: a distance map of the closed line art pixels.
 * @async: the #GimpAsync associated with the computation
 *
 * Creates a binarized version of the strokes of @buffer, detected either
 * with luminosity (light means background) or alpha values depending on
 * @select_transparent. This binary version of the strokes will have closed
 * regions allowing adequate selection of "nearly closed regions".
 * This algorithm is meant for digital painting (and in particular on the
 * sketch-only step), and therefore will likely produce unexpected results on
 * other types of input.
 *
 * The algorithm is the first step from the research paper "A Fast and
 * Efficient Semi-guided Algorithm for Flat Coloring Line-arts", by Sébastian
 * Fourey, David Tschumperlé, David Revoy.
 * https://hal.archives-ouvertes.fr/hal-01891876
 *
 * Returns: a new #GeglBuffer of format "Y u8" representing the
 *          binarized @line_art. If @lineart_distmap is not #NULL, a
 *          newly allocated float buffer is returned, which can be used
 *          for overflowing created masks later.
 */
static GeglBuffer *
gimp_line_art_close (GeglBuffer  *buffer,
                     gboolean     select_transparent,
                     gdouble      stroke_threshold,
                     gint         spline_max_length,
                     gint         segment_max_length,
                     gint         minimal_lineart_area,
                     gint         normal_estimate_mask_size,
                     gfloat       end_point_rate,
                     gfloat       spline_max_angle,
                     gint         end_point_connectivity,
                     gfloat       spline_roundness,
                     gboolean     allow_self_intersections,
                     gint         created_regions_significant_area,
                     gint         created_regions_minimum_area,
                     gboolean     small_segments_from_spline_sources,
                     gfloat     **closed_distmap,
                     GimpAsync   *async)
{
  const Babl         *gray_format;
  GeglBufferIterator *gi;
  GeglBuffer         *closed  = NULL;
  GeglBuffer         *strokes = NULL;
  guchar              max_value = 0;
  gint                width  = gegl_buffer_get_width (buffer);
  gint                height = gegl_buffer_get_height (buffer);
  gint                i;

  if (select_transparent)
    /* Keep alpha channel as gray levels */
    gray_format = babl_format ("A u8");
  else
    /* Keep luminance */
    gray_format = babl_format ("Y' u8");

  /* Transform the line art from any format to gray. */
  strokes = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                             gray_format);
  gimp_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, strokes, NULL);
  gegl_buffer_set_format (strokes, babl_format ("Y' u8"));

  if (! select_transparent)
    {
      /* Compute the biggest value */
      gi = gegl_buffer_iterator_new (strokes, NULL, 0, NULL,
                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
      while (gegl_buffer_iterator_next (gi))
        {
          guchar *data = (guchar*) gi->items[0].data;
          gint    k;

          if (gimp_async_is_canceled (async))
            {
              gegl_buffer_iterator_stop (gi);

              gimp_async_abort (async);

              goto end1;
            }

          for (k = 0; k < gi->length; k++)
            {
              if (*data > max_value)
                max_value = *data;
              data++;
            }
        }
    }

  /* Make the image binary: 1 is stroke, 0 background */
  gi = gegl_buffer_iterator_new (strokes, NULL, 0, NULL,
                                 GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (gi))
    {
      guchar *data = (guchar*) gi->items[0].data;
      gint    k;

      if (gimp_async_is_canceled (async))
        {
          gegl_buffer_iterator_stop (gi);

          gimp_async_abort (async);

          goto end1;
        }

      for (k = 0; k < gi->length; k++)
        {
          if (! select_transparent)
            /* Negate the value. */
            *data = max_value - *data;
          /* Apply a threshold. */
          if (*data > (guchar) (255.0f * (1.0f - stroke_threshold)))
            *data = 1;
          else
            *data = 0;
          data++;
        }
    }

  /* Denoise (remove small connected components) */
  gimp_lineart_denoise (strokes, minimal_lineart_area, async);
  if (gimp_async_is_stopped (async))
    goto end1;

  closed = g_object_ref (strokes);

  if (spline_max_length > 0 || segment_max_length > 0)
    {
      GArray     *keypoints           = NULL;
      GHashTable *visited             = NULL;
      gfloat     *radii               = NULL;
      gfloat     *normals             = NULL;
      gfloat     *curvatures          = NULL;
      gfloat     *smoothed_curvatures = NULL;
      gfloat      threshold;
      gfloat      clamped_threshold;
      GList      *fill_pixels         = NULL;
      GList      *iter;

      normals             = g_new0 (gfloat, width * height * 2);
      curvatures          = g_new0 (gfloat, width * height);
      smoothed_curvatures = g_new0 (gfloat, width * height);

      /* Estimate normals & curvature */
      gimp_lineart_compute_normals_curvatures (strokes, normals, curvatures,
                                               smoothed_curvatures,
                                               normal_estimate_mask_size,
                                               async);
      if (gimp_async_is_stopped (async))
        goto end2;

      radii = gimp_lineart_estimate_strokes_radii (strokes, async);
      if (gimp_async_is_stopped (async))
        goto end2;
      threshold = 1.0f - end_point_rate;
      clamped_threshold = MAX (0.25f, threshold);
      for (i = 0; i < width; i++)
        {
          gint j;

          if (gimp_async_is_canceled (async))
            {
              gimp_async_abort (async);

              goto end2;
            }

          for (j = 0; j < height; j++)
            {
              if (smoothed_curvatures[i + j * width] >= (threshold / MAX (1.0f, radii[i + j * width])) ||
                  curvatures[i + j * width] >= clamped_threshold)
                curvatures[i + j * width] = 1.0;
              else
                curvatures[i + j * width] = 0.0;
            }
        }
      g_clear_pointer (&radii, g_free);

      keypoints = gimp_lineart_curvature_extremums (curvatures, smoothed_curvatures,
                                                    width, height, async);
      if (gimp_async_is_stopped (async))
        goto end2;

      visited = g_hash_table_new_full ((GHashFunc) visited_hash_fun,
                                       (GEqualFunc) visited_equal_fun,
                                       (GDestroyNotify) g_free, NULL);

      if (spline_max_length > 0)
        {
          GList           *candidates;
          SplineCandidate *candidate;

          candidates = gimp_lineart_find_spline_candidates (keypoints, normals, width,
                                                            spline_max_length,
                                                            spline_max_angle,
                                                            async);
          if (gimp_async_is_stopped (async))
            goto end3;

          g_object_unref (closed);
          closed = gimp_gegl_buffer_dup (strokes);

          /* Draw splines */
          while (candidates)
            {
              Pixel    *p1;
              Pixel    *p2;
              gboolean  inserted = FALSE;

              if (gimp_async_is_canceled (async))
                {
                  gimp_async_abort (async);

                  goto end3;
                }

              p1 = g_new (Pixel, 1);
              p2 = g_new (Pixel, 1);

              candidate = (SplineCandidate *) candidates->data;
              p1->x = candidate->p1.x;
              p1->y = candidate->p1.y;
              p2->x = candidate->p2.x;
              p2->y = candidate->p2.y;

              g_free (candidate);
              candidates = g_list_delete_link (candidates, candidates);

              if ((! g_hash_table_contains (visited, p1) ||
                   GPOINTER_TO_INT (g_hash_table_lookup (visited, p1)) < end_point_connectivity) &&
                  (! g_hash_table_contains (visited, p2) ||
                   GPOINTER_TO_INT (g_hash_table_lookup (visited, p2)) < end_point_connectivity))
                {
                  GArray      *discrete_curve;
                  GimpVector2  vect1 = pair2normal (*p1, normals, width);
                  GimpVector2  vect2 = pair2normal (*p2, normals, width);
                  gfloat       distance = gimp_vector2_length_val (gimp_vector2_sub_val (*p1, *p2));
                  gint         transitions;

                  gimp_vector2_mul (&vect1, distance);
                  gimp_vector2_mul (&vect1, spline_roundness);
                  gimp_vector2_mul (&vect2, distance);
                  gimp_vector2_mul (&vect2, spline_roundness);

                  discrete_curve = gimp_lineart_discrete_spline (*p1, vect1, *p2, vect2);

                  transitions = allow_self_intersections ?
                    gimp_number_of_transitions (discrete_curve, strokes) :
                    gimp_number_of_transitions (discrete_curve, closed);

                  if (transitions == 2 &&
                      gimp_line_art_allow_closure (closed, discrete_curve,
                                                   &fill_pixels,
                                                   created_regions_significant_area,
                                                   created_regions_minimum_area - 1))
                    {
                      for (i = 0; i < discrete_curve->len; i++)
                        {
                          Pixel p = g_array_index (discrete_curve, Pixel, i);

                          if (p.x >= 0 && p.x < gegl_buffer_get_width (closed) &&
                              p.y >= 0 && p.y < gegl_buffer_get_height (closed))
                            {
                              guchar val = 2;

                              gegl_buffer_set (closed, GEGL_RECTANGLE ((gint) p.x, (gint) p.y, 1, 1), 0,
                                               NULL, &val, GEGL_AUTO_ROWSTRIDE);
                            }
                        }
                      g_hash_table_replace (visited, p1,
                                            GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p1)) + 1));
                      g_hash_table_replace (visited, p2,
                                            GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p2)) + 1));
                      inserted = TRUE;
                    }
                  g_array_free (discrete_curve, TRUE);
                }
              if (! inserted)
                {
                  g_free (p1);
                  g_free (p2);
                }
            }

 end3:
          g_list_free_full (candidates, g_free);

          if (gimp_async_is_stopped (async))
            goto end2;
        }

      g_clear_object (&strokes);

      /* Draw straight line segments */
      if (segment_max_length > 0)
        {
          Pixel *point;

          point = (Pixel *) keypoints->data;
          for (i = 0; i < keypoints->len; i++)
            {
              Pixel    *p;
              gboolean  inserted = FALSE;

              if (gimp_async_is_canceled (async))
                {
                  gimp_async_abort (async);

                  goto end2;
                }

              p  = g_new (Pixel, 1);
              *p = *point;

              if (! g_hash_table_contains (visited, p) ||
                  (small_segments_from_spline_sources &&
                   GPOINTER_TO_INT (g_hash_table_lookup (visited, p)) < end_point_connectivity))
                {
                  GArray *segment = gimp_lineart_line_segment_until_hit (closed, *point,
                                                                         pair2normal (*point, normals, width),
                                                                         segment_max_length);

                  if (segment->len &&
                      gimp_line_art_allow_closure (closed, segment, &fill_pixels,
                                                   created_regions_significant_area,
                                                   created_regions_minimum_area - 1))
                    {
                      gint j;

                      for (j = 0; j < segment->len; j++)
                        {
                          Pixel  p2 = g_array_index (segment, Pixel, j);
                          guchar val = 2;

                          gegl_buffer_set (closed, GEGL_RECTANGLE ((gint) p2.x, (gint) p2.y, 1, 1), 0,
                                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
                        }
                      g_hash_table_replace (visited, p,
                                            GINT_TO_POINTER (GPOINTER_TO_INT (g_hash_table_lookup (visited, p)) + 1));
                      inserted = TRUE;
                    }
                  g_array_free (segment, TRUE);
                }
              if (! inserted)
                g_free (p);
              point++;
            }
        }

      for (iter = fill_pixels; iter; iter = iter->next)
        {
          Pixel *p = iter->data;

          if (gimp_async_is_canceled (async))
            {
              gimp_async_abort (async);

              goto end2;
            }

          /* XXX A best approach would be to generalize
           * gimp_drawable_bucket_fill() to work on any buffer (the code
           * is already mostly there) rather than reimplementing a naive
           * bucket fill.
           * This is mostly a quick'n dirty first implementation which I
           * will improve later.
           */
          gimp_line_art_simple_fill (closed, (gint) p->x, (gint) p->y);
        }

 end2:
      g_list_free_full (fill_pixels, g_free);
      g_free (normals);
      g_free (curvatures);
      g_free (smoothed_curvatures);
      g_clear_pointer (&radii, g_free);
      if (keypoints)
        g_array_free (keypoints, TRUE);
      g_clear_pointer (&visited, g_hash_table_destroy);

      if (gimp_async_is_stopped (async))
        goto end1;
    }
  else
    {
      g_clear_object (&strokes);
    }

  if (closed_distmap)
    {
      GeglNode   *graph;
      GeglNode   *input;
      GeglNode   *op;

      /* Flooding needs a distance map for closed line art. */
      *closed_distmap = g_new (gfloat, width * height);

      graph = gegl_node_new ();
      input = gegl_node_new_child (graph,
                                   "operation", "gegl:buffer-source",
                                   "buffer", closed,
                                   NULL);
      op  = gegl_node_new_child (graph,
                                 "operation", "gegl:distance-transform",
                                 "metric",    GEGL_DISTANCE_METRIC_EUCLIDEAN,
                                 "normalize", FALSE,
                                 NULL);
      gegl_node_connect_to (input, "output",
                            op, "input");
      gegl_node_blit (op, 1.0, gegl_buffer_get_extent (closed),
                      NULL, *closed_distmap,
                      GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
      g_object_unref (graph);
    }

 end1:
  g_clear_object (&strokes);

  if (gimp_async_is_stopped (async))
    g_clear_object (&closed);

  return closed;
}

static void
gimp_lineart_denoise (GeglBuffer *buffer,
                      int         minimum_area,
                      GimpAsync  *async)
{
  /* Keep connected regions with significant area. */
  GArray   *region;
  GQueue   *q       = g_queue_new ();
  gint      width   = gegl_buffer_get_width (buffer);
  gint      height  = gegl_buffer_get_height (buffer);
  gboolean *visited = g_new0 (gboolean, width * height);
  gint      x, y;

  region = g_array_sized_new (TRUE, TRUE, sizeof (Pixel *), minimum_area);

  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x)
      {
        guchar has_stroke;

        if (gimp_async_is_canceled (async))
          {
            gimp_async_abort (async);

            goto end;
          }

        gegl_buffer_sample (buffer, x, y, NULL, &has_stroke, NULL,
                            GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        if (has_stroke && ! visited[x + y * width])
          {
            Pixel *p = g_new (Pixel, 1);
            gint   regionSize = 0;

            p->x = x;
            p->y = y;

            g_queue_push_tail (q, p);
            visited[x + y * width] = TRUE;

            while (! g_queue_is_empty (q))
              {
                Pixel *p;
                gint   p2x;
                gint   p2y;

                if (gimp_async_is_canceled (async))
                  {
                    gimp_async_abort (async);

                    goto end;
                  }

                p = (Pixel *) g_queue_pop_head (q);

                p2x = p->x + 1;
                p2y = p->y;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x +p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x + 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x - 1;
                p2y = p->y + 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }
                p2x = p->x + 1;
                p2y = p->y - 1;
                if (p2x >= 0 && p2x < width && p2y >= 0 && p2y < height)
                  {
                    gegl_buffer_sample (buffer, p2x, p2y, NULL, &has_stroke, NULL,
                                        GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                    if (has_stroke && ! visited[p2x + p2y * width])
                      {
                        Pixel *p2 = g_new (Pixel, 1);

                        p2->x = p2x;
                        p2->y = p2y;
                        g_queue_push_tail (q, p2);
                        visited[p2x + p2y * width] = TRUE;
                      }
                  }

                ++regionSize;
                if (regionSize < minimum_area)
                  g_array_append_val (region, *p);
                g_free (p);
              }
            if (regionSize < minimum_area)
              {
                Pixel *pixel = (Pixel *) region->data;
                gint   i = 0;

                for (; i < region->len; i++)
                  {
                    guchar val = 0;
                    gegl_buffer_set (buffer, GEGL_RECTANGLE (pixel->x, pixel->y, 1, 1), 0,
                                     NULL, &val, GEGL_AUTO_ROWSTRIDE);
                    pixel++;
                  }
              }
            g_array_remove_range (region, 0, region->len);
          }
      }

 end:
  g_array_free (region, TRUE);
  g_queue_free_full (q, g_free);
  g_free (visited);
}

static void
gimp_lineart_compute_normals_curvatures (GeglBuffer *mask,
                                         gfloat     *normals,
                                         gfloat     *curvatures,
                                         gfloat     *smoothed_curvatures,
                                         int         normal_estimate_mask_size,
                                         GimpAsync  *async)
{
  gfloat  *edgels_curvatures  = NULL;
  gfloat  *smoothed_curvature;
  GArray  *es                 = NULL;
  Edgel  **e;
  gint     width              = gegl_buffer_get_width (mask);

  es = gimp_edgelset_new (mask, async);
  if (gimp_async_is_stopped (async))
    goto end;

  e = (Edgel **) es->data;

  gimp_edgelset_smooth_normals (es, normal_estimate_mask_size, async);
  if (gimp_async_is_stopped (async))
    goto end;

  gimp_edgelset_compute_curvature (es, async);
  if (gimp_async_is_stopped (async))
    goto end;

  while (*e)
    {
      const float curvature = ((*e)->curvature > 0.0f) ? (*e)->curvature : 0.0f;
      const float w         = MAX (1e-8f, curvature * curvature);

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          goto end;
        }

      normals[((*e)->x + (*e)->y * width) * 2] += w * (*e)->x_normal;
      normals[((*e)->x + (*e)->y * width) * 2 + 1] += w * (*e)->y_normal;
      curvatures[(*e)->x + (*e)->y * width] = MAX (curvature,
                                                   curvatures[(*e)->x + (*e)->y * width]);
      e++;
    }
  for (int y = 0; y < gegl_buffer_get_height (mask); ++y)
    {
      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          goto end;
        }

      for (int x = 0; x < gegl_buffer_get_width (mask); ++x)
        {
          const float _angle = atan2f (normals[(x + y * width) * 2 + 1],
                                       normals[(x + y * width) * 2]);
          normals[(x + y * width) * 2] = cosf (_angle);
          normals[(x + y * width) * 2 + 1] = sinf (_angle);
        }
    }

  /* Smooth curvatures on edgels, then take maximum on each pixel. */
  edgels_curvatures = gimp_lineart_get_smooth_curvatures (es, async);
  if (gimp_async_is_stopped (async))
    goto end;

  smoothed_curvature = edgels_curvatures;

  e = (Edgel **) es->data;
  while (*e)
    {
      gfloat *pixel_curvature = &smoothed_curvatures[(*e)->x + (*e)->y * width];

      if (*pixel_curvature < *smoothed_curvature)
        *pixel_curvature = *smoothed_curvature;

      ++smoothed_curvature;
      e++;
    }

 end:
  g_free (edgels_curvatures);

  if (es)
    g_array_free (es, TRUE);
}

static gfloat *
gimp_lineart_get_smooth_curvatures (GArray    *edgelset,
                                    GimpAsync *async)
{
  Edgel  **e;
  gfloat  *smoothed_curvatures = g_new0 (gfloat, edgelset->len);
  gfloat   weights[9];
  gfloat   smoothed_curvature;
  gfloat   weights_sum;
  gint     idx = 0;

  weights[0] = 1.0f;
  for (int i = 1; i <= 8; ++i)
    weights[i] = expf (-(i * i) / 30.0f);

  e = (Edgel **) edgelset->data;
  while (*e)
    {
      Edgel *edgel_before = g_array_index (edgelset, Edgel*, (*e)->previous);
      Edgel *edgel_after  = g_array_index (edgelset, Edgel*, (*e)->next);
      int    n = 5;
      int    i = 1;

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          g_free (smoothed_curvatures);

          return NULL;
        }

      smoothed_curvature = (*e)->curvature;
      weights_sum = weights[0];
      while (n-- && (edgel_after != edgel_before))
        {
          smoothed_curvature += weights[i] * edgel_before->curvature;
          smoothed_curvature += weights[i] * edgel_after->curvature;
          edgel_before = g_array_index (edgelset, Edgel*, edgel_before->previous);
          edgel_after  = g_array_index (edgelset, Edgel*, edgel_after->next);
          weights_sum += 2 * weights[i];
          i++;
        }
      smoothed_curvature /= weights_sum;
      smoothed_curvatures[idx++] = smoothed_curvature;

      e++;
    }

  return smoothed_curvatures;
}

/**
 * Keep one pixel per connected component of curvature extremums.
 */
static GArray *
gimp_lineart_curvature_extremums (gfloat    *curvatures,
                                  gfloat    *smoothed_curvatures,
                                  gint       width,
                                  gint       height,
                                  GimpAsync *async)
{
  gboolean *visited = g_new0 (gboolean, width * height);
  GQueue   *q       = g_queue_new ();
  GArray   *max_positions;

  max_positions = g_array_new (FALSE, TRUE, sizeof (Pixel));

  for (int y = 0; y < height; ++y)
    {
      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          goto end;
        }

      for (int x = 0; x < width; ++x)
        {
          if ((curvatures[x + y * width] > 0.0) && ! visited[x + y * width])
            {
              Pixel  *p = g_new (Pixel, 1);
              Pixel   max_smoothed_curvature_pixel;
              Pixel   max_raw_curvature_pixel;
              gfloat  max_smoothed_curvature;
              gfloat  max_raw_curvature;

              max_smoothed_curvature_pixel = gimp_vector2_new (-1.0, -1.0);
              max_smoothed_curvature       = 0.0f;

              max_raw_curvature_pixel = gimp_vector2_new (x, y);
              max_raw_curvature       = curvatures[x + y * width];

              p->x = x;
              p->y = y;
              g_queue_push_tail (q, p);
              visited[x + y * width] = TRUE;

              while (! g_queue_is_empty (q))
                {
                  gfloat sc;
                  gfloat c;
                  gint   p2x;
                  gint   p2y;

                  if (gimp_async_is_canceled (async))
                    {
                      gimp_async_abort (async);

                      goto end;
                    }

                  p  = (Pixel *) g_queue_pop_head (q);
                  sc = smoothed_curvatures[(gint) p->x + (gint) p->y * width];
                  c  = curvatures[(gint) p->x + (gint) p->y * width];

                  curvatures[(gint) p->x + (gint) p->y * width] = 0.0f;

                  p2x = (gint) p->x + 1;
                  p2y = (gint) p->y;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x - 1;
                  p2y = p->y;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x;
                  p2y = p->y - 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x;
                  p2y = p->y + 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x + 1;
                  p2y = p->y + 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x - 1;
                  p2y = p->y - 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x - 1;
                  p2y = p->y + 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  p2x = p->x + 1;
                  p2y = p->y - 1;
                  if (p2x >= 0 && p2x < width    &&
                      p2y >= 0 && p2y < height   &&
                      curvatures[p2x + p2y * width] > 0.0 &&
                      ! visited[p2x + p2y * width])
                    {
                      Pixel *p2 = g_new (Pixel, 1);

                      p2->x = p2x;
                      p2->y = p2y;
                      g_queue_push_tail (q, p2);
                      visited[p2x + p2y * width] = TRUE;
                    }

                  if (sc > max_smoothed_curvature)
                    {
                      max_smoothed_curvature_pixel = *p;
                      max_smoothed_curvature = sc;
                    }
                  if (c > max_raw_curvature)
                    {
                      max_raw_curvature_pixel = *p;
                      max_raw_curvature = c;
                    }
                  g_free (p);
                }
              if (max_smoothed_curvature > 0.0f)
                {
                  curvatures[(gint) max_smoothed_curvature_pixel.x + (gint) max_smoothed_curvature_pixel.y * width] = max_smoothed_curvature;
                  g_array_append_val (max_positions, max_smoothed_curvature_pixel);
                }
              else
                {
                  curvatures[(gint) max_raw_curvature_pixel.x + (gint) max_raw_curvature_pixel.y * width] = max_raw_curvature;
                  g_array_append_val (max_positions, max_raw_curvature_pixel);
                }
            }
        }
    }

 end:
  g_queue_free_full (q, g_free);
  g_free (visited);

  if (gimp_async_is_stopped (async))
    {
      g_array_free (max_positions, TRUE);
      max_positions = NULL;
    }

  return max_positions;
}

static gint
gimp_spline_candidate_cmp (const SplineCandidate *a,
                           const SplineCandidate *b,
                           gpointer               user_data)
{
  /* This comparison actually returns the opposite of common comparison
   * functions on purpose, as we want the first element on the list to
   * be the "bigger".
   */
  if (a->quality < b->quality)
    return 1;
  else if (a->quality > b->quality)
    return -1;
  else
    return 0;
}

static GList *
gimp_lineart_find_spline_candidates (GArray    *max_positions,
                                     gfloat    *normals,
                                     gint       width,
                                     gint       distance_threshold,
                                     gfloat     max_angle_deg,
                                     GimpAsync *async)
{
  GList       *candidates = NULL;
  const float  CosMin     = cosf (M_PI * (max_angle_deg / 180.0));
  gint         i;

  for (i = 0; i < max_positions->len; i++)
    {
      Pixel p1 = g_array_index (max_positions, Pixel, i);
      gint  j;

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          g_list_free_full (candidates, g_free);

          return NULL;
        }

      for (j = i + 1; j < max_positions->len; j++)
        {
          Pixel       p2 = g_array_index (max_positions, Pixel, j);
          const float distance = gimp_vector2_length_val (gimp_vector2_sub_val (p1, p2));

          if (distance <= distance_threshold)
            {
              GimpVector2 normalP1;
              GimpVector2 normalP2;
              GimpVector2 p1f;
              GimpVector2 p2f;
              GimpVector2 p1p2;
              float       cosN;
              float       qualityA;
              float       qualityB;
              float       qualityC;
              float       quality;

              normalP1 = gimp_vector2_new (normals[((gint) p1.x + (gint) p1.y * width) * 2],
                                           normals[((gint) p1.x + (gint) p1.y * width) * 2 + 1]);
              normalP2 = gimp_vector2_new (normals[((gint) p2.x + (gint) p2.y * width) * 2],
                                           normals[((gint) p2.x + (gint) p2.y * width) * 2 + 1]);
              p1f = gimp_vector2_new (p1.x, p1.y);
              p2f = gimp_vector2_new (p2.x, p2.y);
              p1p2 = gimp_vector2_sub_val (p2f, p1f);

              cosN = gimp_vector2_inner_product_val (normalP1, (gimp_vector2_neg_val (normalP2)));
              qualityA = MAX (0.0f, 1 - distance / distance_threshold);
              qualityB = MAX (0.0f,
                              (float) (gimp_vector2_inner_product_val (normalP1, p1p2) - gimp_vector2_inner_product_val (normalP2, p1p2)) /
                              distance);
              qualityC = MAX (0.0f, cosN - CosMin);
              quality = qualityA * qualityB * qualityC;
              if (quality > 0)
                {
                  SplineCandidate *candidate = g_new (SplineCandidate, 1);

                  candidate->p1      = p1;
                  candidate->p2      = p2;
                  candidate->quality = quality;

                  candidates = g_list_insert_sorted_with_data (candidates, candidate,
                                                               (GCompareDataFunc) gimp_spline_candidate_cmp,
                                                               NULL);
                }
            }
        }
    }
  return candidates;
}

static GArray *
gimp_lineart_discrete_spline (Pixel       p0,
                              GimpVector2 n0,
                              Pixel       p1,
                              GimpVector2 n1)
{
  GArray       *points = g_array_new (FALSE, TRUE, sizeof (Pixel));
  const double  a0 = 2 * p0.x - 2 * p1.x + n0.x - n1.x;
  const double  b0 = -3 * p0.x + 3 * p1.x - 2 * n0.x + n1.x;
  const double  c0 = n0.x;
  const double  d0 = p0.x;
  const double  a1 = 2 * p0.y - 2 * p1.y + n0.y - n1.y;
  const double  b1 = -3 * p0.y + 3 * p1.y - 2 * n0.y + n1.y;
  const double  c1 = n0.y;
  const double  d1 = p0.y;

  double        t = 0.0;
  const double  dtMin = 1.0 / MAX (fabs (p0.x - p1.x), fabs (p0.y - p1.y));
  Pixel         point = gimp_vector2_new ((gint) round (d0), (gint) round (d1));

  g_array_append_val (points, point);

  while (t <= 1.0)
    {
      const double t2 = t * t;
      const double t3 = t * t2;
      double       dx;
      double       dy;
      Pixel        p = gimp_vector2_new ((gint) round (a0 * t3 + b0 * t2 + c0 * t + d0),
                                         (gint) round (a1 * t3 + b1 * t2 + c1 * t + d1));

      /* create gimp_vector2_neq () ? */
      if (g_array_index (points, Pixel, points->len - 1).x != p.x ||
          g_array_index (points, Pixel, points->len - 1).y != p.y)
        {
          g_array_append_val (points, p);
        }
      dx = fabs (3 * a0 * t * t + 2 * b0 * t + c0) + 1e-8;
      dy = fabs (3 * a1 * t * t + 2 * b1 * t + c1) + 1e-8;
      t += MIN (dtMin, 0.75 / MAX (dx, dy));
    }
  if (g_array_index (points, Pixel, points->len - 1).x != p1.x ||
      g_array_index (points, Pixel, points->len - 1).y != p1.y)
    {
      g_array_append_val (points, p1);
    }
  return points;
}

static gint
gimp_number_of_transitions (GArray     *pixels,
                            GeglBuffer *buffer)
{
  int result = 0;

  if (pixels->len > 0)
    {
      Pixel    it = g_array_index (pixels, Pixel, 0);
      guchar   value;
      gboolean previous;
      gint     i;

      gegl_buffer_sample (buffer, (gint) it.x, (gint) it.y, NULL, &value, NULL,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      previous = (gboolean) value;

      /* Starts at the second element. */
      for (i = 1; i < pixels->len; i++)
        {
          it = g_array_index (pixels, Pixel, i);

          gegl_buffer_sample (buffer, (gint) it.x, (gint) it.y, NULL, &value, NULL,
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          result += ((gboolean) value != previous);
          previous = (gboolean) value;
        }
    }

  return result;
}

/**
 * gimp_line_art_allow_closure:
 * @mask: the current state of line art closure.
 * @pixels: the pixels of a candidate closure (spline or segment).
 * @fill_pixels: #GList of unsignificant pixels to bucket fill.
 * @significant_size: number of pixels for area to be considered
 *                    "significant".
 * @minimum_size: number of pixels for area to be allowed.
 *
 * Checks whether adding the set of points @pixels to @mask will create
 * 4-connected background regions whose size (i.e. number of pixels)
 * will be below @minimum_size. If it creates such small areas, the
 * function will refuse this candidate spline/segment, with the
 * exception of very small areas under @significant_size. These
 * micro-area are considered "unsignificant" and accepted (because they
 * can be created in some conditions, for instance when created curves
 * cross or start from a same endpoint), and one pixel for each
 * micro-area will be added to @fill_pixels to be later filled along
 * with the candidate pixels.
 *
 * Returns: #TRUE if @pixels should be added to @mask, #FALSE otherwise.
 */
static gboolean
gimp_line_art_allow_closure (GeglBuffer *mask,
                             GArray     *pixels,
                             GList     **fill_pixels,
                             int         significant_size,
                             int         minimum_size)
{
  /* A theorem from the paper is that a zone with more than
   * `2 * (@minimum_size - 1)` edgels (border pixels) will have more
   * than @minimum_size pixels.
   * Since we are following the edges of the area, we can therefore stop
   * earlier if we reach this number of edgels.
   */
  const glong max_edgel_count = 2 * (minimum_size + 1);

  Pixel      *p = (Pixel*) pixels->data;
  GList      *fp = NULL;
  gint        i;

  /* Mark pixels */
  for (i = 0; i < pixels->len; i++)
    {
      if (p->x >= 0 && p->x < gegl_buffer_get_width (mask) &&
          p->y >= 0 && p->y < gegl_buffer_get_height (mask))
        {
          guchar val;

          gegl_buffer_sample (mask, (gint) p->x, (gint) p->y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          val = val ? 3 : 2;
          gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p->x, (gint) p->y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
      p++;
    }

  for (i = 0; i < pixels->len; i++)
    {
      Pixel p = g_array_index (pixels, Pixel, i);

      for (int direction = 0; direction < 4; ++direction)
        {
          if (p.x >= 0 && p.x < gegl_buffer_get_width (mask) &&
              p.y >= 0 && p.y < gegl_buffer_get_height (mask) &&
              border_in_direction (mask, p, direction))
            {
              Edgel  e;
              guchar val;
              glong  count;
              glong  area;

              gegl_buffer_sample (mask, (gint) p.x, (gint) p.y, NULL, &val,
                                  NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
              if ((gboolean) (val & (4 << direction)))
                continue;

              gimp_edgel_init (&e);
              e.x = p.x;
              e.y = p.y;
              e.direction = direction;

              count = gimp_edgel_track_mark (mask, e, max_edgel_count);
              if ((count != -1) && (count <= max_edgel_count))
                {
                  area = gimp_edgel_region_area (mask, e);

                  if (area >= significant_size && area <= minimum_size)
                    {
                      gint j;

                      /* Remove marks */
                      for (j = 0; j < pixels->len; j++)
                        {
                          Pixel p2 = g_array_index (pixels, Pixel, j);

                          if (p2.x >= 0 && p2.x < gegl_buffer_get_width (mask) &&
                              p2.y >= 0 && p2.y < gegl_buffer_get_height (mask))
                            {
                              guchar val;

                              gegl_buffer_sample (mask, (gint) p2.x, (gint) p2.y, NULL, &val,
                                                  NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
                              val &= 1;
                              gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p2.x, (gint) p2.y, 1, 1), 0,
                                               NULL, &val, GEGL_AUTO_ROWSTRIDE);
                            }
                        }
                      g_list_free_full (fp, g_free);

                      return FALSE;
                    }
                  else if (area > 0 && area < significant_size)
                    {
                      Pixel *np = g_new (Pixel, 1);

                      np->x = direction == XPlusDirection ? p.x + 1 : (direction == XMinusDirection ? p.x - 1 : p.x);
                      np->y = direction == YPlusDirection ? p.y + 1 : (direction == YMinusDirection ? p.y - 1 : p.y);

                      if (np->x >= 0 && np->x < gegl_buffer_get_width (mask) &&
                          np->y >= 0 && np->y < gegl_buffer_get_height (mask))
                        fp = g_list_prepend (fp, np);
                      else
                        g_free (np);
                    }
                }
            }
        }
    }

  *fill_pixels = g_list_concat (*fill_pixels, fp);
  /* Remove marks */
  for (i = 0; i < pixels->len; i++)
    {
      Pixel p = g_array_index (pixels, Pixel, i);

      if (p.x >= 0 && p.x < gegl_buffer_get_width (mask) &&
          p.y >= 0 && p.y < gegl_buffer_get_height (mask))
        {
          guchar val;

          gegl_buffer_sample (mask, (gint) p.x, (gint) p.y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          val &= 1;
          gegl_buffer_set (mask, GEGL_RECTANGLE ((gint) p.x, (gint) p.y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
    }
  return TRUE;
}

static GArray *
gimp_lineart_line_segment_until_hit (const GeglBuffer *mask,
                                     Pixel             start,
                                     GimpVector2       direction,
                                     int               size)
{
  GeglBuffer  *buffer = (GeglBuffer *) mask;
  gboolean     out = FALSE;
  GArray      *points = g_array_new (FALSE, TRUE, sizeof (Pixel));
  int          tmax;
  GimpVector2  p0 = gimp_vector2_new (start.x, start.y);

  gimp_vector2_mul (&direction, (gdouble) size);
  direction.x = round (direction.x);
  direction.y = round (direction.y);

  tmax = MAX (abs ((int) direction.x), abs ((int) direction.y));

  for (int t = 0; t <= tmax; ++t)
    {
      GimpVector2 v = gimp_vector2_add_val (p0, gimp_vector2_mul_val (direction, (float)t / tmax));
      Pixel       p;

      p.x = (gint) round (v.x);
      p.y = (gint) round (v.y);
      if (p.x >= 0 && p.x < gegl_buffer_get_width (buffer) &&
          p.y >= 0 && p.y < gegl_buffer_get_height (buffer))
        {
          guchar val;
          gegl_buffer_sample (buffer, p.x, p.y, NULL, &val,
                              NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
          if (out && val)
            {
              return points;
            }
          out = ! val;
        }
      else if (out)
        {
          return points;
        }
      else
        {
          g_array_free (points, TRUE);
          return g_array_new (FALSE, TRUE, sizeof (Pixel));
        }
      g_array_append_val (points, p);
    }

  g_array_free (points, TRUE);
  return g_array_new (FALSE, TRUE, sizeof (Pixel));
}

static gfloat *
gimp_lineart_estimate_strokes_radii (GeglBuffer *mask,
                                     GimpAsync  *async)
{
  GeglBufferIterator *gi;
  gfloat             *dist;
  gfloat             *thickness;
  GeglNode           *graph;
  GeglNode           *input;
  GeglNode           *op;
  gint                width  = gegl_buffer_get_width (mask);
  gint                height = gegl_buffer_get_height (mask);

  /* Compute a distance map for the line art. */
  dist = g_new (gfloat, width * height);

  graph = gegl_node_new ();
  input = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer", mask,
                               NULL);
  op  = gegl_node_new_child (graph,
                             "operation", "gegl:distance-transform",
                             "metric",    GEGL_DISTANCE_METRIC_EUCLIDEAN,
                             "normalize", FALSE,
                             NULL);
  gegl_node_connect_to (input, "output", op, "input");
  gegl_node_blit (op, 1.0, gegl_buffer_get_extent (mask),
                  NULL, dist, GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
  g_object_unref (graph);

  thickness = g_new0 (gfloat, width * height);
  gi = gegl_buffer_iterator_new (mask, NULL, 0, NULL,
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (gi))
    {
      guint8 *m = (guint8*) gi->items[0].data;
      gint    startx = gi->items[0].roi.x;
      gint    starty = gi->items[0].roi.y;
      gint    endy   = starty + gi->items[0].roi.height;
      gint    endx   = startx + gi->items[0].roi.width;
      gint    x;
      gint    y;

      if (gimp_async_is_canceled (async))
        {
          gegl_buffer_iterator_stop (gi);

          gimp_async_abort (async);

          goto end;
        }

      for (y = starty; y < endy; y++)
        for (x = startx; x < endx; x++)
          {
            if (*m && dist[x + y * width] == 1.0)
              {
                gint     dx = x;
                gint     dy = y;
                gfloat   d  = 1.0;
                gfloat   nd;
                gboolean neighbour_thicker = TRUE;

                while (neighbour_thicker)
                  {
                    gint px = dx - 1;
                    gint py = dy - 1;
                    gint nx = dx + 1;
                    gint ny = dy + 1;

                    neighbour_thicker = FALSE;
                    if (px >= 0)
                      {
                        if ((nd = dist[px + dy * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (py >= 0 && (nd = dist[px + py * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            dy = py;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (ny < height && (nd = dist[px + ny * width]) > d)
                          {
                            d = nd;
                            dx = px;
                            dy = ny;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                      }
                    if (nx < width)
                      {
                        if ((nd = dist[nx + dy * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (py >= 0 && (nd = dist[nx + py * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            dy = py;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                        if (ny < height && (nd = dist[nx + ny * width]) > d)
                          {
                            d = nd;
                            dx = nx;
                            dy = ny;
                            neighbour_thicker = TRUE;
                            continue;
                          }
                      }
                    if (py > 0 && (nd = dist[dx + py * width]) > d)
                      {
                        d = nd;
                        dy = py;
                        neighbour_thicker = TRUE;
                        continue;
                      }
                    if (ny < height && (nd = dist[dx + ny * width]) > d)
                      {
                        d = nd;
                        dy = ny;
                        neighbour_thicker = TRUE;
                        continue;
                      }
                  }
                thickness[(gint) x + (gint) y * width] = d;
              }
            m++;
          }
    }

 end:
  g_free (dist);

  if (gimp_async_is_stopped (async))
    g_clear_pointer (&thickness, g_free);

  return thickness;
}

static void
gimp_line_art_simple_fill (GeglBuffer *buffer,
                           gint        x,
                           gint        y)
{
  guchar val;

  if (x < 0 || x >= gegl_buffer_get_width (buffer) ||
      y < 0 || y >= gegl_buffer_get_height (buffer))
    return;

  gegl_buffer_sample (buffer, x, y, NULL, &val,
                      NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  if (! val)
    {
      val = 1;
      gegl_buffer_set (buffer, GEGL_RECTANGLE (x, y, 1, 1), 0,
                       NULL, &val, GEGL_AUTO_ROWSTRIDE);
      gimp_line_art_simple_fill (buffer, x + 1, y);
      gimp_line_art_simple_fill (buffer, x - 1, y);
      gimp_line_art_simple_fill (buffer, x, y + 1);
      gimp_line_art_simple_fill (buffer, x, y - 1);
    }
}

static guint
visited_hash_fun (Pixel *key)
{
  /* Cantor pairing function. */
  return (key->x + key->y) * (key->x + key->y + 1) / 2 + key->y;
}

static gboolean
visited_equal_fun (Pixel *e1,
                   Pixel *e2)
{
  return (e1->x == e2->x && e1->y == e2->y);
}

static inline gboolean
border_in_direction (GeglBuffer *mask,
                     Pixel       p,
                     int         direction)
{
  gint px = (gint) p.x + DeltaX[direction];
  gint py = (gint) p.y + DeltaY[direction];

  if (px >= 0 && px < gegl_buffer_get_width (mask) &&
      py >= 0 && py < gegl_buffer_get_height (mask))
    {
      guchar val;

      gegl_buffer_sample (mask, px, py, NULL, &val,
                          NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      return ! ((gboolean) val);
    }
  return TRUE;
}

static inline GimpVector2
pair2normal (Pixel   p,
             gfloat *normals,
             gint    width)
{
  return gimp_vector2_new (normals[((gint) p.x + (gint) p.y * width) * 2],
                           normals[((gint) p.x + (gint) p.y * width) * 2 + 1]);
}
/* Edgel functions */

static Edgel *
gimp_edgel_new (int       x,
                int       y,
                Direction direction)
{
  Edgel *edgel = g_new (Edgel, 1);

  edgel->x         = x;
  edgel->y         = y;
  edgel->direction = direction;

  gimp_edgel_init (edgel);

  return edgel;
}

static void
gimp_edgel_init (Edgel *edgel)
{
  edgel->x_normal  = 0;
  edgel->y_normal  = 0;
  edgel->curvature = 0;
  edgel->next      = edgel->previous = G_MAXUINT;
}

static void
gimp_edgel_clear (Edgel **edgel)
{
  g_clear_pointer (edgel, g_free);
}

static int
gimp_edgel_cmp (const Edgel* e1,
                const Edgel* e2)
{
  gimp_assert (e1 && e2);

  if ((e1->x == e2->x) && (e1->y == e2->y) &&
      (e1->direction == e2->direction))
    return 0;
  else if ((e1->y < e2->y) || (e1->y == e2->y && e1->x < e2->x) ||
           (e1->y == e2->y && e1->x == e2->x && e1->direction < e2->direction))
    return -1;
  else
    return 1;
}

static guint
edgel2index_hash_fun (Edgel *key)
{
  /* Cantor pairing function.
   * Was not sure how to use the direction though. :-/
   */
  return (key->x + key->y) * (key->x + key->y + 1) / 2 + key->y * key->direction;
}

static gboolean
edgel2index_equal_fun (Edgel *e1,
                       Edgel *e2)
{
  return (e1->x == e2->x && e1->y == e2->y &&
          e1->direction == e2->direction);
}

/**
 * @mask;
 * @edgel:
 * @size_limit:
 *
 * Track a border, marking inner pixels with a bit corresponding to the
 * edgel traversed (4 << direction) for direction in {0,1,2,3}.
 * Stop tracking after @size_limit edgels have been visited.
 *
 * Returns: Number of visited edgels, or -1 if an already visited edgel
 *          has been encountered.
 */
static glong
gimp_edgel_track_mark (GeglBuffer *mask,
                       Edgel       edgel,
                       long        size_limit)
{
  Edgel start = edgel;
  long  count = 1;

  do
    {
      guchar val;

      gimp_edgelset_next8 (mask, &edgel, &edgel);
      gegl_buffer_sample (mask, edgel.x, edgel.y, NULL, &val,
                          NULL, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      if (val & 2)
        {
          /* Only mark pixels of the spline/segment */
          if (val & (4 << edgel.direction))
            return -1;

          /* Mark edgel in pixel (1 == In Mask, 2 == Spline/Segment) */
          val |= (4 << edgel.direction);
          gegl_buffer_set (mask, GEGL_RECTANGLE (edgel.x, edgel.y, 1, 1), 0,
                           NULL, &val, GEGL_AUTO_ROWSTRIDE);
        }
      if (gimp_edgel_cmp (&edgel, &start) != 0)
        ++count;
    }
  while (gimp_edgel_cmp (&edgel, &start) != 0 && count <= size_limit);

  return count;
}

/**
 * gimp_edgel_region_area:
 * @mask: current state of closed line art buffer.
 * @start_edgel: edgel to follow.
 *
 * Follows a line border, starting from @start_edgel to compute the area
 * enclosed by this border.
 * Unfortunately this may return a negative area when the line does not
 * close a zone. In this case, there is an uncertaincy on the size of
 * the created zone, and we should consider it a big size.
 *
 * Returns: the area enclosed by the followed line, or a negative value
 * if the zone is not closed (hence actual area unknown).
 */
static glong
gimp_edgel_region_area (const GeglBuffer *mask,
                        Edgel             start_edgel)
{
  Edgel edgel = start_edgel;
  glong area = 0;

  do
    {
      if (edgel.direction == XPlusDirection)
        area -= edgel.x;
      else if (edgel.direction == XMinusDirection)
        area += edgel.x - 1;

      gimp_edgelset_next8 (mask, &edgel, &edgel);
    }
  while (gimp_edgel_cmp (&edgel, &start_edgel) != 0);

  return area;
}

/* Edgel sets */

static GArray *
gimp_edgelset_new (GeglBuffer *buffer,
                   GimpAsync  *async)
{
  GeglBufferIterator *gi;
  GArray             *set;
  GHashTable         *edgel2index;
  gint                width  = gegl_buffer_get_width (buffer);
  gint                height = gegl_buffer_get_height (buffer);

  set = g_array_new (TRUE, TRUE, sizeof (Edgel *));
  g_array_set_clear_func (set, (GDestroyNotify) gimp_edgel_clear);

  if (width <= 1 || height <= 1)
    return set;

  edgel2index = g_hash_table_new ((GHashFunc) edgel2index_hash_fun,
                                  (GEqualFunc) edgel2index_equal_fun);

  gi = gegl_buffer_iterator_new (buffer, GEGL_RECTANGLE (0, 0, width, height),
                                 0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 5);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (0, -1, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (0, 1, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (-1, 0, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (gi, buffer, GEGL_RECTANGLE (1, 0, width, height),
                            0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (gi))
    {
      guint8 *p      = (guint8*) gi->items[0].data;
      guint8 *prevy  = (guint8*) gi->items[1].data;
      guint8 *nexty  = (guint8*) gi->items[2].data;
      guint8 *prevx  = (guint8*) gi->items[3].data;
      guint8 *nextx  = (guint8*) gi->items[4].data;
      gint    startx = gi->items[0].roi.x;
      gint    starty = gi->items[0].roi.y;
      gint    endy   = starty + gi->items[0].roi.height;
      gint    endx   = startx + gi->items[0].roi.width;
      gint    x;
      gint    y;

      if (gimp_async_is_canceled (async))
        {
          gegl_buffer_iterator_stop (gi);

          gimp_async_abort (async);

          goto end;
        }

      for (y = starty; y < endy; y++)
        for (x = startx; x < endx; x++)
          {
            if (*(p++))
              {
                if (! *prevy)
                  gimp_edgelset_add (set, x, y, YMinusDirection, edgel2index);
                if (! *nexty)
                  gimp_edgelset_add (set, x, y, YPlusDirection, edgel2index);
                if (! *prevx)
                  gimp_edgelset_add (set, x, y, XMinusDirection, edgel2index);
                if (! *nextx)
                  gimp_edgelset_add (set, x, y, XPlusDirection, edgel2index);
              }
            prevy++;
            nexty++;
            prevx++;
            nextx++;
          }
    }

  gimp_edgelset_build_graph (set, buffer, edgel2index, async);
  if (gimp_async_is_stopped (async))
    goto end;

  gimp_edgelset_init_normals (set);

 end:
  g_hash_table_destroy (edgel2index);

  if (gimp_async_is_stopped (async))
    {
      g_array_free (set, TRUE);
      set = NULL;
    }

  return set;
}

static void
gimp_edgelset_add (GArray     *set,
                   int         x,
                   int         y,
                   Direction   direction,
                   GHashTable *edgel2index)
{
  Edgel         *edgel    = gimp_edgel_new (x, y, direction);
  unsigned long  position = set->len;

  g_array_append_val (set, edgel);
  g_hash_table_insert (edgel2index, edgel, GUINT_TO_POINTER (position));
}

static void
gimp_edgelset_init_normals (GArray *set)
{
  Edgel **e = (Edgel**) set->data;

  while (*e)
    {
      GimpVector2 n = Direction2Normal[(*e)->direction];

      (*e)->x_normal = n.x;
      (*e)->y_normal = n.y;
      e++;
    }
}

static void
gimp_edgelset_smooth_normals (GArray    *set,
                              int        mask_size,
                              GimpAsync *async)
{
  const gfloat sigma = mask_size * 0.775;
  const gfloat den   = 2 * sigma * sigma;
  gfloat       weights[65];
  GimpVector2  smoothed_normal;
  gint         i;

  gimp_assert (mask_size <= 65);

  weights[0] = 1.0f;
  for (int i = 1; i <= mask_size; ++i)
    weights[i] = expf (-(i * i) / den);

  for (i = 0; i < set->len; i++)
    {
      Edgel *it           = g_array_index (set, Edgel*, i);
      Edgel *edgel_before = g_array_index (set, Edgel*, it->previous);
      Edgel *edgel_after  = g_array_index (set, Edgel*, it->next);
      int    n = mask_size;
      int    i = 1;

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          return;
        }

      smoothed_normal = Direction2Normal[it->direction];
      while (n-- && (edgel_after != edgel_before))
        {
          smoothed_normal = gimp_vector2_add_val (smoothed_normal,
                                                  gimp_vector2_mul_val (Direction2Normal[edgel_before->direction], weights[i]));
          smoothed_normal = gimp_vector2_add_val (smoothed_normal,
                                                  gimp_vector2_mul_val (Direction2Normal[edgel_after->direction], weights[i]));
          edgel_before = g_array_index (set, Edgel *, edgel_before->previous);
          edgel_after  = g_array_index (set, Edgel *, edgel_after->next);
          ++i;
        }
      gimp_vector2_normalize (&smoothed_normal);
      it->x_normal = smoothed_normal.x;
      it->y_normal = smoothed_normal.y;
    }
}

static void
gimp_edgelset_compute_curvature (GArray    *set,
                                 GimpAsync *async)
{
  gint i;

  for (i = 0; i < set->len; i++)
    {
      Edgel       *it       = g_array_index (set, Edgel*, i);
      Edgel       *previous = g_array_index (set, Edgel *, it->previous);
      Edgel       *next     = g_array_index (set, Edgel *, it->next);
      GimpVector2  n_prev   = gimp_vector2_new (previous->x_normal, previous->y_normal);
      GimpVector2  n_next   = gimp_vector2_new (next->x_normal, next->y_normal);
      GimpVector2  diff     = gimp_vector2_mul_val (gimp_vector2_sub_val (n_next, n_prev),
                                                    0.5);
      const float  c        = gimp_vector2_length_val (diff);
      const float  crossp   = n_prev.x * n_next.y - n_prev.y * n_next.x;

      it->curvature = (crossp > 0.0f) ? c : -c;

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          return;
        }
    }
}

static void
gimp_edgelset_build_graph (GArray     *set,
                           GeglBuffer *buffer,
                           GHashTable *edgel2index,
                           GimpAsync  *async)
{
  Edgel edgel;
  gint  i;

  for (i = 0; i < set->len; i++)
    {
      Edgel *neighbor;
      Edgel *it = g_array_index (set, Edgel *, i);
      guint  neighbor_pos;

      if (gimp_async_is_canceled (async))
        {
          gimp_async_abort (async);

          return;
        }

      gimp_edgelset_next8 (buffer, it, &edgel);

      gimp_assert (g_hash_table_contains (edgel2index, &edgel));
      neighbor_pos = GPOINTER_TO_UINT (g_hash_table_lookup (edgel2index, &edgel));
      it->next = neighbor_pos;
      neighbor = g_array_index (set, Edgel *, neighbor_pos);
      neighbor->previous = i;
    }
}

static void
gimp_edgelset_next8 (const GeglBuffer *buffer,
                     Edgel            *it,
                     Edgel            *n)
{
  guint8 pixels[9];

  n->x         = it->x;
  n->y         = it->y;
  n->direction = it->direction;

  gegl_buffer_get ((GeglBuffer *) buffer,
                   GEGL_RECTANGLE (n->x - 1, n->y - 1, 3, 3),
                   1.0, NULL, pixels, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);
  switch (n->direction)
    {
    case XPlusDirection:
      if (pixels[8])
        {
          ++(n->y);
          ++(n->x);
          n->direction = YMinusDirection;
        }
      else if (pixels[7])
        {
          ++(n->y);
        }
      else
        {
          n->direction = YPlusDirection;
        }
      break;
    case YMinusDirection:
      if (pixels[2])
        {
          ++(n->x);
          --(n->y);
          n->direction = XMinusDirection;
        }
      else if (pixels[5])
        {
          ++(n->x);
        }
      else
        {
          n->direction = XPlusDirection;
        }
      break;
    case XMinusDirection:
      if (pixels[0])
        {
          --(n->x);
          --(n->y);
          n->direction = YPlusDirection;
        }
      else if (pixels[1])
        {
          --(n->y);
        }
      else
        {
          n->direction = YMinusDirection;
        }
      break;
    case YPlusDirection:
      if (pixels[6])
        {
          --(n->x);
          ++(n->y);
          n->direction = XPlusDirection;
        }
      else if (pixels[3])
        {
          --(n->x);
        }
      else
        {
          n->direction = XMinusDirection;
        }
      break;
    default:
      g_return_if_reached ();
      break;
    }
}
