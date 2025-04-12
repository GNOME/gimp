/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
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

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"

#include "../operations-types.h"

#include "gimp-layer-modes.h"
#include "gimpoperationlayermode.h"
#include "gimpoperationlayermode-composite.h"


/* the maximum number of samples to process in one go.  used to limit
 * the size of the buffers we allocate on the stack.
 */
#define GIMP_COMPOSITE_BLEND_MAX_SAMPLES ((1 << 18) /* 256 KiB */  /      \
                                          16 /* bytes per pixel */ /      \
                                          2  /* max number of buffers */)

/* number of consecutive unblended samples (whose source or destination alpha
 * is zero) above which to split the blending process, in order to avoid
 * performing too many unnecessary conversions.
 */
#define GIMP_COMPOSITE_BLEND_SPLIT_THRESHOLD 32


enum
{
  PROP_0,
  PROP_LAYER_MODE,
  PROP_OPACITY,
  PROP_BLEND_SPACE,
  PROP_COMPOSITE_SPACE,
  PROP_COMPOSITE_MODE
};


typedef void (* CompositeFunc) (const gfloat *in,
                                const gfloat *layer,
                                const gfloat *comp,
                                const gfloat *mask,
                                float         opacity,
                                gfloat       *out,
                                gint          samples);


static void            gimp_operation_layer_mode_finalize            (GObject                *object);
static void            gimp_operation_layer_mode_set_property        (GObject                *object,
                                                                      guint                   property_id,
                                                                      const GValue           *value,
                                                                      GParamSpec             *pspec);
static void            gimp_operation_layer_mode_get_property        (GObject                *object,
                                                                      guint                   property_id,
                                                                      GValue                 *value,
                                                                      GParamSpec             *pspec);

static void            gimp_operation_layer_mode_prepare             (GeglOperation          *operation);
static GeglRectangle   gimp_operation_layer_mode_get_bounding_box    (GeglOperation          *operation);
static gboolean        gimp_operation_layer_mode_parent_process      (GeglOperation          *operation,
                                                                      GeglOperationContext   *context,
                                                                      const gchar            *output_prop,
                                                                      const GeglRectangle    *result,
                                                                      gint                    level);

static gboolean        gimp_operation_layer_mode_process             (GeglOperation          *operation,
                                                                      void                   *in,
                                                                      void                   *layer,
                                                                      void                   *mask,
                                                                      void                   *out,
                                                                      glong                   samples,
                                                                      const GeglRectangle    *roi,
                                                                      gint                    level);

static gboolean        gimp_operation_layer_mode_real_parent_process (GeglOperation          *operation,
                                                                      GeglOperationContext   *context,
                                                                      const gchar            *output_prop,
                                                                      const GeglRectangle    *result,
                                                                      gint                    level);
static gboolean        gimp_operation_layer_mode_real_process        (GeglOperation          *operation,
                                                                      void                   *in,
                                                                      void                   *layer,
                                                                      void                   *mask,
                                                                      void                   *out,
                                                                      glong                   samples,
                                                                      const GeglRectangle    *roi,
                                                                      gint                    level);

static gboolean        process_last_node                             (GeglOperation       *operation,
                                                                      void                *in,
                                                                      void                *layer,
                                                                      void                *mask,
                                                                      void                *out,
                                                                      glong                samples,
                                                                      const GeglRectangle *roi,
                                                                      gint                 level);

static void            gimp_operation_layer_mode_cache_fishes        (GimpOperationLayerMode  *op,
                                                                      const Babl              *preferred_format,
                                                                      const Babl             **out_format,
                                                                      const Babl             **composite_to_blend_fish,
                                                                      const Babl             **blend_to_composite_fish);


G_DEFINE_TYPE (GimpOperationLayerMode, gimp_operation_layer_mode,
               GEGL_TYPE_OPERATION_POINT_COMPOSER3)

#define parent_class gimp_operation_layer_mode_parent_class

static CompositeFunc composite_union                = gimp_operation_layer_mode_composite_union;
static CompositeFunc composite_clip_to_backdrop     = gimp_operation_layer_mode_composite_clip_to_backdrop;
static CompositeFunc composite_clip_to_layer        = gimp_operation_layer_mode_composite_clip_to_layer;
static CompositeFunc composite_intersection         = gimp_operation_layer_mode_composite_intersection;

static CompositeFunc composite_union_sub            = gimp_operation_layer_mode_composite_union_sub;
static CompositeFunc composite_clip_to_backdrop_sub = gimp_operation_layer_mode_composite_clip_to_backdrop_sub;
static CompositeFunc composite_clip_to_layer_sub    = gimp_operation_layer_mode_composite_clip_to_layer_sub;
static CompositeFunc composite_intersection_sub     = gimp_operation_layer_mode_composite_intersection_sub;


static void
gimp_operation_layer_mode_class_init (GimpOperationLayerModeClass *klass)
{
  GObjectClass                     *object_class          = G_OBJECT_CLASS (klass);
  GeglOperationClass               *operation_class       = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposer3Class *point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name", "gimp:layer-mode", NULL);

  object_class->finalize            = gimp_operation_layer_mode_finalize;
  object_class->set_property        = gimp_operation_layer_mode_set_property;
  object_class->get_property        = gimp_operation_layer_mode_get_property;

  operation_class->prepare          = gimp_operation_layer_mode_prepare;
  operation_class->get_bounding_box = gimp_operation_layer_mode_get_bounding_box;
  operation_class->process          = gimp_operation_layer_mode_parent_process;

  point_composer3_class->process    = gimp_operation_layer_mode_process;

  klass->parent_process             = gimp_operation_layer_mode_real_parent_process;
  klass->process                    = gimp_operation_layer_mode_real_process;
  klass->get_affected_region        = NULL;

  g_object_class_install_property (object_class, PROP_LAYER_MODE,
                                   g_param_spec_enum ("layer-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BLEND_SPACE,
                                   g_param_spec_enum ("blend-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));


  g_object_class_install_property (object_class, PROP_COMPOSITE_SPACE,
                                   g_param_spec_enum ("composite-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_enum ("composite-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COMPOSITE_MODE,
                                                      GIMP_LAYER_COMPOSITE_UNION,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));


#if COMPILE_SSE2_INTRINISICS
  if (gimp_cpu_accel_get_support () & GIMP_CPU_ACCEL_X86_SSE2)
    composite_clip_to_backdrop = gimp_operation_layer_mode_composite_clip_to_backdrop_sse2;
#endif
}

static void
gimp_operation_layer_mode_init (GimpOperationLayerMode *self)
{
  g_rw_lock_init (&self->cache_lock);
}

static void
gimp_operation_layer_mode_finalize (GObject *object)
{
  GimpOperationLayerMode *mode = GIMP_OPERATION_LAYER_MODE (object);

  g_rw_lock_clear (&mode->cache_lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_layer_mode_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LAYER_MODE:
      self->layer_mode = g_value_get_enum (value);
      break;

    case PROP_OPACITY:
      self->prop_opacity = g_value_get_double (value);
      break;

    case PROP_BLEND_SPACE:
      self->blend_space = g_value_get_enum (value);
      break;

    case PROP_COMPOSITE_SPACE:
      self->composite_space = g_value_get_enum (value);
      break;

    case PROP_COMPOSITE_MODE:
      self->prop_composite_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_layer_mode_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LAYER_MODE:
      g_value_set_enum (value, self->layer_mode);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, self->prop_opacity);
      break;

    case PROP_BLEND_SPACE:
      g_value_set_enum (value, self->blend_space);
      break;

    case PROP_COMPOSITE_SPACE:
      g_value_set_enum (value, self->composite_space);
      break;

    case PROP_COMPOSITE_MODE:
      g_value_set_enum (value, self->prop_composite_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_layer_mode_prepare (GeglOperation *operation)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (operation);
  const GeglRectangle    *input_extent;
  const GeglRectangle    *mask_extent;
  const Babl             *preferred_format;
  const Babl             *format;

  self->composite_mode = self->prop_composite_mode;

  if (self->composite_mode == GIMP_LAYER_COMPOSITE_AUTO)
    {
      self->composite_mode =
        gimp_layer_mode_get_composite_mode (self->layer_mode);

      g_warn_if_fail (self->composite_mode != GIMP_LAYER_COMPOSITE_AUTO);
    }

  self->function       = gimp_layer_mode_get_function       (self->layer_mode);
  self->blend_function = gimp_layer_mode_get_blend_function (self->layer_mode);

  input_extent = gegl_operation_source_get_bounding_box (operation, "input");
  mask_extent  = gegl_operation_source_get_bounding_box (operation, "aux2");

  /* if the input pad has data, work as usual. */
  if (input_extent && ! gegl_rectangle_is_empty (input_extent))
    {
      self->is_last_node = FALSE;

      preferred_format = gegl_operation_get_source_format (operation, "input");
    }
  /* otherwise, we're the last node (corresponding to the bottom layer).
   * in this case, we render the layer (as if) using UNION mode.
   */
  else
    {
      self->is_last_node = TRUE;

      /* if the layer mode doesn't affect the source, use a shortcut
       * function that only applies the opacity/mask to the layer.
       */
      if (! (gimp_operation_layer_mode_get_affected_region (self) &
             GIMP_LAYER_COMPOSITE_REGION_SOURCE))
        {
          self->function = process_last_node;
        }
      /* otherwise, use the original process function, but force the
       * composite mode to UNION.
       */
      else
        {
          self->composite_mode = GIMP_LAYER_COMPOSITE_UNION;
        }

      preferred_format = gegl_operation_get_source_format (operation, "aux");
    }

  self->has_mask = mask_extent && ! gegl_rectangle_is_empty (mask_extent);

  gimp_operation_layer_mode_cache_fishes (self, preferred_format, &format, NULL, NULL);

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "aux2",   babl_format_with_space ("Y float", format));
}

static GeglRectangle
gimp_operation_layer_mode_get_bounding_box (GeglOperation *op)
{
  GimpOperationLayerMode   *self     = (gpointer) op;
  GeglRectangle            *in_rect;
  GeglRectangle            *aux_rect;
  GeglRectangle            *aux2_rect;
  GeglRectangle             src_rect = {};
  GeglRectangle             dst_rect = {};
  GeglRectangle             result;
  GimpLayerCompositeRegion  included_region;

  in_rect   = gegl_operation_source_get_bounding_box (op, "input");
  aux_rect  = gegl_operation_source_get_bounding_box (op, "aux");
  aux2_rect = gegl_operation_source_get_bounding_box (op, "aux2");

  if (in_rect)
    dst_rect = *in_rect;

  if (aux_rect)
    {
      src_rect = *aux_rect;

      if (aux2_rect)
        gegl_rectangle_intersect (&src_rect, &src_rect, aux2_rect);
    }

  if (self->is_last_node)
    {
      included_region = GIMP_LAYER_COMPOSITE_REGION_SOURCE;
    }
  else
    {
      included_region = gimp_layer_mode_get_included_region (self->layer_mode,
                                                             self->composite_mode);
    }

  if (self->prop_opacity == 0.0)
    included_region &= ~GIMP_LAYER_COMPOSITE_REGION_SOURCE;

  gegl_rectangle_intersect (&result, &src_rect, &dst_rect);

  if (included_region & GIMP_LAYER_COMPOSITE_REGION_SOURCE)
    gegl_rectangle_bounding_box (&result, &result, &src_rect);

  if (included_region & GIMP_LAYER_COMPOSITE_REGION_DESTINATION)
    gegl_rectangle_bounding_box (&result, &result, &dst_rect);

  return result;
}

static gboolean
gimp_operation_layer_mode_parent_process (GeglOperation        *operation,
                                          GeglOperationContext *context,
                                          const gchar          *output_prop,
                                          const GeglRectangle  *result,
                                          gint                  level)
{
  GimpOperationLayerMode *point = GIMP_OPERATION_LAYER_MODE (operation);

  point->opacity = point->prop_opacity;

  /* if we have a mask, but it's not included in the output, pretend the
   * opacity is 0, so that we don't composite 'aux' over 'input' as if there
   * was no mask.
   */
  if (point->has_mask)
    {
      GObject  *mask;
      gboolean  has_mask;

      /* get the raw value.  this does not increase the reference count. */
      mask = gegl_operation_context_get_object (context, "aux2");

      /* disregard 'mask' if it's not included in the roi. */
      has_mask =
        mask &&
        gegl_rectangle_intersect (NULL,
                                  gegl_buffer_get_extent (GEGL_BUFFER (mask)),
                                  result);

      if (! has_mask)
        point->opacity = 0.0;
    }

  return GIMP_OPERATION_LAYER_MODE_GET_CLASS (point)->parent_process (
    operation, context, output_prop, result, level);
}

static gboolean
gimp_operation_layer_mode_process (GeglOperation       *operation,
                                   void                *in,
                                   void                *layer,
                                   void                *mask,
                                   void                *out,
                                   glong                samples,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  return ((GimpOperationLayerMode *) operation)->function (
    operation, in, layer, mask, out, samples, roi, level);
}

static gboolean
gimp_operation_layer_mode_real_parent_process (GeglOperation        *operation,
                                               GeglOperationContext *context,
                                               const gchar          *output_prop,
                                               const GeglRectangle  *result,
                                               gint                  level)
{
  GimpOperationLayerMode   *point = GIMP_OPERATION_LAYER_MODE (operation);
  GObject                  *input;
  GObject                  *aux;
  gboolean                  has_input;
  gboolean                  has_aux;
  GimpLayerCompositeRegion  included_region;

  /* get the raw values.  this does not increase the reference count. */
  input = gegl_operation_context_get_object (context, "input");
  aux   = gegl_operation_context_get_object (context, "aux");

  /* disregard 'input' if it's not included in the roi. */
  has_input =
    input &&
    gegl_rectangle_intersect (NULL,
                              gegl_buffer_get_extent (GEGL_BUFFER (input)),
                              result);

  /* disregard 'aux' if it's not included in the roi, or if it's fully
   * transparent.
   */
  has_aux =
    aux &&
    point->opacity != 0.0 &&
    gegl_rectangle_intersect (NULL,
                              gegl_buffer_get_extent (GEGL_BUFFER (aux)),
                              result);

  if (point->is_last_node)
    {
      included_region = GIMP_LAYER_COMPOSITE_REGION_SOURCE;
    }
  else
    {
      included_region = gimp_layer_mode_get_included_region (point->layer_mode,
                                                             point->composite_mode);
    }

  /* if there's no 'input' ... */
  if (! has_input)
    {
      /* ... and there's 'aux', and the composite mode includes it (or we're
       * the last node) ...
       */
      if (has_aux && (included_region & GIMP_LAYER_COMPOSITE_REGION_SOURCE))
        {
          GimpLayerCompositeRegion affected_region;

          affected_region =
            gimp_operation_layer_mode_get_affected_region (point);

          /* ... and the op doesn't otherwise affect 'aux', or changes its
           * alpha ...
           */
          if (! (affected_region & GIMP_LAYER_COMPOSITE_REGION_SOURCE) &&
              point->opacity == 1.0                                    &&
              ! gegl_operation_context_get_object (context, "aux2"))
            {
              /* pass 'aux' directly as output; */
              gegl_operation_context_set_object (context, "output", aux);
              return TRUE;
            }

          /* otherwise, if the op affects 'aux', or changes its alpha, process
           * it even though there's no 'input';
           */
        }
      /* otherwise, there's no 'aux', or the composite mode doesn't include it,
       * and so ...
       */
      else
        {
          /* ... the output is empty. */
          gegl_operation_context_set_object (context, "output", NULL);
          return TRUE;
        }
    }
  /* otherwise, if there's 'input' but no 'aux' ... */
  else if (! has_aux)
    {
      /* ... and the composite mode includes 'input' ... */
      if (included_region & GIMP_LAYER_COMPOSITE_REGION_DESTINATION)
        {
          GimpLayerCompositeRegion affected_region;

          affected_region =
            gimp_operation_layer_mode_get_affected_region (point);

          /* ... and the op doesn't otherwise affect 'input' ... */
          if (! (affected_region & GIMP_LAYER_COMPOSITE_REGION_DESTINATION))
            {
              /* pass 'input' directly as output; */
              gegl_operation_context_set_object (context, "output", input);
              return TRUE;
            }

          /* otherwise, if the op affects 'input', process it even though
           * there's no 'aux';
           */
        }

      /* otherwise, the output is fully transparent, but we process it anyway
       * to maintain the 'input' color values.
       */
    }

  /* FIXME: we don't actually handle the case where one of the inputs
   * is NULL -- it'll just segfault.  'input' is not expected to be NULL,
   * but 'aux' might be, currently.
   */
  if (! input || ! aux)
    {
      GObject *empty = G_OBJECT (gegl_buffer_new (NULL, NULL));

      if (! input) gegl_operation_context_set_object (context, "input", empty);
      if (! aux)   gegl_operation_context_set_object (context, "aux",   empty);

      if (! input && ! aux)
        gegl_object_set_has_forked (G_OBJECT (empty));

      g_object_unref (empty);
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}

static gboolean
gimp_operation_layer_mode_real_process (GeglOperation       *operation,
                                        void                *in_p,
                                        void                *layer_p,
                                        void                *mask_p,
                                        void                *out_p,
                                        glong                samples,
                                        const GeglRectangle *roi,
                                        gint                 level)
{
  GimpOperationLayerMode *layer_mode              = (gpointer) operation;
  gfloat                 *in                      = in_p;
  gfloat                 *out                     = out_p;
  gfloat                 *layer                   = layer_p;
  gfloat                 *mask                    = mask_p;
  gfloat                  opacity                 = layer_mode->opacity;
  GimpLayerCompositeMode  composite_mode          = layer_mode->composite_mode;
  GimpLayerModeBlendFunc  blend_function          = layer_mode->blend_function;
  gboolean                composite_needs_in_color;
  gfloat                 *blend_in;
  gfloat                 *blend_layer;
  gfloat                 *blend_out;
  const Babl             *composite_to_blend_fish = NULL;
  const Babl             *blend_to_composite_fish = NULL;

  /* make sure we don't process more than GIMP_COMPOSITE_BLEND_MAX_SAMPLES
   * at a time, so that we don't overflow the stack if we allocate buffers
   * on it.  note that this has to be done with a nested function call,
   * because alloca'd buffers remain for the duration of the stack frame.
   */
  while (samples > GIMP_COMPOSITE_BLEND_MAX_SAMPLES)
    {
      gimp_operation_layer_mode_real_process (operation,
                                              in, layer, mask, out,
                                              GIMP_COMPOSITE_BLEND_MAX_SAMPLES,
                                              roi, level);

      in      += 4 * GIMP_COMPOSITE_BLEND_MAX_SAMPLES;
      layer   += 4 * GIMP_COMPOSITE_BLEND_MAX_SAMPLES;
      if (mask)
        mask  +=     GIMP_COMPOSITE_BLEND_MAX_SAMPLES;
      out     += 4 * GIMP_COMPOSITE_BLEND_MAX_SAMPLES;

      samples -= GIMP_COMPOSITE_BLEND_MAX_SAMPLES;
    }

  composite_needs_in_color =
    composite_mode == GIMP_LAYER_COMPOSITE_UNION ||
    composite_mode == GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP;

  blend_in    = in;
  blend_layer = layer;
  blend_out   = out;

  /* Make sure the cache is set up from the start as the
   * operation's prepare() method may have not been run yet.
   */
  gimp_operation_layer_mode_cache_fishes (layer_mode, NULL, NULL,
                                          &composite_to_blend_fish,
                                          &blend_to_composite_fish);

  /* if we need to convert the samples between the composite and blend
   * spaces...
   */
  if (composite_to_blend_fish)
    {
      gint i;
      gint end;

      if (in != out || composite_needs_in_color)
        {
          /* don't convert input in-place if we're not doing in-place output,
           * or if we're going to need the original input for compositing.
           */
          blend_in = g_alloca (sizeof (gfloat) * 4 * samples);
        }
      blend_layer  = g_alloca (sizeof (gfloat) * 4 * samples);

      if (in == out) /* in-place detected, avoid clobbering since we need to
                        read 'in' for the compositing stage  */
        {
          if (blend_layer != layer)
            blend_out = blend_layer;
          else
            blend_out = g_alloca (sizeof (gfloat) * 4 * samples);
        }

      /* samples whose the source or destination alpha is zero are not blended,
       * and therefore do not need to be converted.  while it's generally
       * desirable to perform conversion and blending in bulk, when we have
       * more than a certain number of consecutive unblended samples, the cost
       * of converting them outweighs the cost of splitting the process around
       * them to avoid the conversion.
       */

      i   = ALPHA;
      end = 4 * samples + ALPHA;

      while (TRUE)
        {
          gint first;
          gint last;
          gint count;

          /* skip any unblended samples.  the color values of `blend_out` for
           * these samples are unconstrained, in particular, they may be NaN,
           * but the alpha values should generally be finite, and specifically
           * 0 when the source alpha is 0.
           */
          while (i < end && (in[i] == 0.0f || layer[i] == 0.0f))
            {
              blend_out[i] = 0.0f;
              i += 4;
            }

          /* stop if there are no more samples */
          if (i == end)
            break;

          /* otherwise, keep scanning the samples until we find
           * GIMP_COMPOSITE_BLEND_SPLIT_THRESHOLD consecutive unblended
           * samples.
           */

          first  = i;
          i     += 4;
          last   = i;

          while (i < end && i - last < 4 * GIMP_COMPOSITE_BLEND_SPLIT_THRESHOLD)
            {
              gboolean blended;

              blended = (in[i] != 0.0f && layer[i] != 0.0f);

              i += 4;
              if (blended)
                last = i;
            }

          /* convert and blend the samples in the range [first, last) */

          count  = (last - first) / 4;
          first -= ALPHA;

          babl_process (composite_to_blend_fish,
                        in + first, blend_in + first, count);
          babl_process (composite_to_blend_fish,
                        layer + first, blend_layer + first, count);

          blend_function (operation, blend_in + first, blend_layer + first,
                          blend_out + first, count);

          babl_process (blend_to_composite_fish,
                        blend_out + first, blend_out + first, count);

          /* make sure the alpha values of `blend_out` are valid for the
           * trailing unblended samples.
           */
          for (; last < i; last += 4)
            blend_out[last] = 0.0f;
        }
    }
  else
    {
      /* if both blending and compositing use the same color space, things are
       * much simpler.
       */

      if (in == out) /* in-place detected, avoid clobbering since we need to
                        read 'in' for the compositing stage  */
        {
          blend_out = g_alloca (sizeof (gfloat) * 4 * samples);
        }

      blend_function (operation, blend_in, blend_layer, blend_out, samples);
    }

  if (! gimp_layer_mode_is_subtractive (layer_mode->layer_mode))
    {
      switch (composite_mode)
        {
        case GIMP_LAYER_COMPOSITE_UNION:
        case GIMP_LAYER_COMPOSITE_AUTO:
          composite_union (in, layer, blend_out, mask, opacity,
                           out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
          composite_clip_to_backdrop (in, layer, blend_out, mask, opacity,
                                      out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
          composite_clip_to_layer (in, layer, blend_out, mask, opacity,
                                   out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_INTERSECTION:
          composite_intersection (in, layer, blend_out, mask, opacity,
                                  out, samples);
          break;
        }
    }
  else
    {
      switch (composite_mode)
        {
        case GIMP_LAYER_COMPOSITE_UNION:
        case GIMP_LAYER_COMPOSITE_AUTO:
          composite_union_sub (in, layer, blend_out, mask, opacity,
                               out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
          composite_clip_to_backdrop_sub (in, layer, blend_out, mask, opacity,
                                          out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER:
          composite_clip_to_layer_sub (in, layer, blend_out, mask, opacity,
                                       out, samples);
          break;

        case GIMP_LAYER_COMPOSITE_INTERSECTION:
          composite_intersection_sub (in, layer, blend_out, mask, opacity,
                                      out, samples);
          break;
        }
    }

  return TRUE;
}

static gboolean
process_last_node (GeglOperation       *operation,
                   void                *in_p,
                   void                *layer_p,
                   void                *mask_p,
                   void                *out_p,
                   glong                samples,
                   const GeglRectangle *roi,
                   gint                 level)
{
  gfloat *out     = out_p;
  gfloat *layer   = layer_p;
  gfloat *mask    = mask_p;
  gfloat  opacity = GIMP_OPERATION_LAYER_MODE (operation)->opacity;

  while (samples--)
    {
      memcpy (out, layer, 3 * sizeof (gfloat));

      out[ALPHA] = layer[ALPHA] * opacity;
      if (mask)
        out[ALPHA] *= *mask++;

      layer += 4;
      out   += 4;
    }

  return TRUE;
}

static void
gimp_operation_layer_mode_cache_fishes (GimpOperationLayerMode  *op,
                                        const Babl              *preferred_format,
                                        const Babl             **layer_mode_format,
                                        const Babl             **composite_to_blend_fish,
                                        const Babl             **blend_to_composite_fish)
{
  const Babl *format;
  gboolean    update_cache = FALSE;

  g_rw_lock_reader_lock (&op->cache_lock);

  gimp_assert (op->composite_space >= GIMP_LAYER_COLOR_SPACE_AUTO &&
               op->composite_space < GIMP_LAYER_COLOR_SPACE_LAST);
  gimp_assert (op->blend_space     >= GIMP_LAYER_COLOR_SPACE_AUTO &&
               op->blend_space     < GIMP_LAYER_COLOR_SPACE_LAST);

  /* XXX I am not why but some old code already had some assert that the
   * composite_space would not be AUTO when blend_space is not.
   */
  if (op->blend_space != GIMP_LAYER_COLOR_SPACE_AUTO)
    gimp_assert (op->composite_space != GIMP_LAYER_COLOR_SPACE_AUTO);

  if (! preferred_format)
    {
      const GeglRectangle *input_extent;

      input_extent = gegl_operation_source_get_bounding_box (GEGL_OPERATION (op), "input");

      if (input_extent && ! gegl_rectangle_is_empty (input_extent))
        preferred_format = gegl_operation_get_source_format (GEGL_OPERATION (op), "input");
      else
        preferred_format = gegl_operation_get_source_format (GEGL_OPERATION (op), "aux");
    }

  format = gimp_layer_mode_get_format (op->layer_mode,
                                       op->blend_space,
                                       op->composite_space,
                                       op->composite_mode,
                                       preferred_format);

  if (op->cached_fish_format != format)
    {
      g_rw_lock_reader_unlock (&op->cache_lock);
      update_cache = TRUE;
    }

  if (update_cache)
    {
      g_rw_lock_writer_lock (&op->cache_lock);

      /* We recheck because it is possible that in-between the point we
       * released the read lock and got our write lock, another thread
       * also modified the cache (to the same values).
       * No need to redo the same thing twice.
       */
      if (op->cached_fish_format != format)
        {
          op->cached_fish_format = format;

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1] =
            babl_fish (babl_format_with_space ("RGBA float", format),
                       babl_format_with_space ("R~G~B~A float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1] =
            babl_fish (babl_format_with_space ("RGBA float", format),
                       babl_format_with_space ("R'G'B'A float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1] =
            babl_fish (babl_format_with_space ("RGBA float", format),
                       babl_format_with_space ("CIE Lab alpha float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1] =
            babl_fish (babl_format_with_space("R'G'B'A float", format),
                       babl_format_with_space ( "RGBA float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1] =
            babl_fish (babl_format_with_space("R~G~B~A float", format),
                       babl_format_with_space ( "RGBA float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1] =
            babl_fish (babl_format_with_space("R~G~B~A float", format),
                       babl_format_with_space ( "CIE Lab alpha float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1] =
            babl_fish (babl_format_with_space("R'G'B'A float", format),
                       babl_format_with_space ( "CIE Lab alpha float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_LINEAR     - 1] =
            babl_fish (babl_format_with_space("CIE Lab alpha float", format),
                       babl_format_with_space ( "RGBA float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1] =
            babl_fish (babl_format_with_space("CIE Lab alpha float", format),
                       babl_format_with_space ( "R~G~B~A float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_LAB            - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1] =
            babl_fish (babl_format_with_space("CIE Lab alpha float", format),
                       babl_format_with_space ( "R'G'B'A float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1] =
            babl_fish (babl_format_with_space("R~G~B~A float", format),
                       babl_format_with_space ( "R'G'B'A float", format));

          op->space_fish
            /* from */ [GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR - 1]
            /* to   */ [GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL - 1] =
            babl_fish (babl_format_with_space("R'G'B'A float", format),
                       babl_format_with_space ( "R~G~B~A float", format));
        }
    }

  if (layer_mode_format)
    *layer_mode_format = format;
  if (composite_to_blend_fish)
    {
      if (op->blend_space != GIMP_LAYER_COLOR_SPACE_AUTO &&
          op->composite_space != GIMP_LAYER_COLOR_SPACE_AUTO)
        *composite_to_blend_fish = op->space_fish[op->composite_space - 1][op->blend_space - 1];
      else
        *composite_to_blend_fish = NULL;
    }
  if (blend_to_composite_fish)
    {
      if (op->blend_space != GIMP_LAYER_COLOR_SPACE_AUTO &&
          op->composite_space != GIMP_LAYER_COLOR_SPACE_AUTO)
        *blend_to_composite_fish = op->space_fish[op->blend_space - 1][op->composite_space - 1];
      else
        *blend_to_composite_fish = NULL;
    }

  if (update_cache)
    g_rw_lock_writer_unlock (&op->cache_lock);
  else
    g_rw_lock_reader_unlock (&op->cache_lock);
}


/*  public functions  */


GimpLayerCompositeRegion
gimp_operation_layer_mode_get_affected_region (GimpOperationLayerMode *layer_mode)
{
  GimpOperationLayerModeClass *klass;

  g_return_val_if_fail (GIMP_IS_OPERATION_LAYER_MODE (layer_mode),
                        GIMP_LAYER_COMPOSITE_REGION_INTERSECTION);

  klass = GIMP_OPERATION_LAYER_MODE_GET_CLASS (layer_mode);

  if (klass->get_affected_region)
    return klass->get_affected_region (layer_mode);

  return GIMP_LAYER_COMPOSITE_REGION_INTERSECTION;
}
