/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2013 Daniel Sabo
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

extern "C"
{

#include "paint-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimp-parallel.h"
#include "core/gimptempbuf.h"

#include "operations/layer-modes/gimpoperationlayermode.h"

#include "gimppaintcore-loops.h"

} /* extern "C" */


#define MIN_PARALLEL_SUB_SIZE 64
#define MIN_PARALLEL_SUB_AREA (MIN_PARALLEL_SUB_SIZE * MIN_PARALLEL_SUB_SIZE)


/* In order to avoid iterating over the same region of the same buffers
 * multiple times, when calling more than one of the paint-core loop functions
 * (hereafter referred to as "algorithms") in succession, we provide a single
 * function, gimp_paint_core_loops_process(), which can be used to perform
 * multiple algorithms in a row.  This function takes a pointer to a
 * GimpPaintCoreLoopsParams structure, providing the parameters for the
 * algorithms, and a GimpPaintCoreLoopsAlgorithm bitset, which specifies the
 * set of algorithms to run; currently, the algorithms are always run in a
 * fixed order.  For convenience, we provide public functions for the
 * individual algorithms, but they're merely wrappers around
 * gimp_paint_core_loops_process().
 *
 * We use some C++ magic to statically generate specialized versions of
 * gimp_paint_core_loops_process() for all possible combinations of algorithms,
 * and, where relevant, formats and input parameters, and to dispatch to the
 * correct version at runtime.
 *
 * To achieve this, each algorithm provides two components:
 *
 *   - The algorithm class template, which implements the algorithm, following
 *     a common interface.  See the AlgorithmBase class for a description of
 *     the interface.  Each algorithm class takes its base class as a template
 *     parameter, which allows us to construct a class hierarchy corresponding
 *     to a specific set of algorithms.  Some classes in the hierarchy are not
 *     algorithms themselves, but are rather helpers, which provide some
 *     functionality to the algorithms further down the hierarchy, such as
 *     access to specific buffers.
 *
 *   - A dispatch function, which takes the input parameters, the requested set
 *     of algorithms, the (type of) the current algorithm hierarchy, and a
 *     visitor object.  The function calls the visitor with a (potentially)
 *     modified hierarchy, depending on the input.  Ihe dispatch function for
 *     an algorithm checks if the requested set of algorithms contains a
 *     certain algorithm, adds the said algorithm to the hierarchy accordingly,
 *     and calls the visitor with the new hierarchy.  See the AlgorithmDispatch
 *     class, which provides a dispatch-function implementation which
 *     algorithms can use instead of providing their own dispatch function.
 *
 *     Helper classes in the hierarchy may also provide dispatch functions,
 *     which likewise modify the hierarchy based on the input parameters.  For
 *     example, the dispatch_paint_mask() function adds a certain PaintMask
 *     specialization to the hierarchy, depending on the format of the paint
 *     mask buffer; this can be used to specialize algorithms based on the mask
 *     format; an algorithm that depends on the paint mask may dispatch through
 *     this function, before modifying the hierarchy itself.
 *
 * The dispatch() function is used to construct an algorithm hierarchy by
 * dispatching through a list of functions.  gimp_paint_core_loops_process()
 * calls dispatch() with the full list of algorithm dispatch functions,
 * receiving in return the algorithm hierarchy matching the input.  It then
 * uses the algorithm interface to perform the actual processing.
 */


enum
{
  ALGORITHM_PAINT_BUF  = 1u << 31,
  ALGORITHM_PAINT_MASK = 1u << 30,
  ALGORITHM_STIPPLE    = 1u << 29
};


template <class T>
struct identity
{
  using type = T;
};


/* dispatch():
 *
 * Takes a list of dispatch function objects, and calls each of them, in order,
 * with the same 'params' and 'algorithms' parameters, passing 'algorithm' as
 * the input hierarchy to the first dispatch function, and passing the output
 * hierarchy of the previous dispatch function as the input hierarchy for the
 * next dispatch function.  Calls 'visitor' with the output hierarchy of the
 * last dispatch function.
 *
 * Each algorithm hierarchy should provide a 'filter' static data member, and
 * each dispatch function object should provide a 'mask' static data member.
 * If the bitwise-AND of the current hierarchy's 'filter' member and the
 * current dispatch function's 'mask' member is equal to 'mask', the dispatch
 * function is skipped.  This can be used to make sure that a class appears
 * only once in the hierarchy, even if its dispatch function is used multiple
 * times, or to prevent an algorithm from being dispatched, if it cannot be
 * used together with another algorithm.
 */

template <class Visitor,
          class Algorithm>
static inline void
dispatch (Visitor                         visitor,
          const GimpPaintCoreLoopsParams *params,
          GimpPaintCoreLoopsAlgorithm     algorithms,
          identity<Algorithm>             algorithm)
{
  visitor (algorithm);
}

template <class Algorithm,
          class Dispatch,
          gboolean = (Algorithm::filter & Dispatch::mask) == Dispatch::mask>
struct dispatch_impl
{
  template <class    Visitor,
            class... DispatchRest>
  static void
  apply (Visitor                         visitor,
         const GimpPaintCoreLoopsParams *params,
         GimpPaintCoreLoopsAlgorithm     algorithms,
         identity<Algorithm>             algorithm,
         Dispatch                        disp,
         DispatchRest...                 disp_rest)
  {
    disp (
      [&] (auto algorithm)
      {
        dispatch (visitor, params, algorithms, algorithm, disp_rest...);
      },
      params, algorithms, algorithm);
  }
};

template <class Algorithm,
          class Dispatch>
struct dispatch_impl<Algorithm, Dispatch, TRUE>
{
  template <class    Visitor,
            class... DispatchRest>
  static void
  apply (Visitor                         visitor,
         const GimpPaintCoreLoopsParams *params,
         GimpPaintCoreLoopsAlgorithm     algorithms,
         identity<Algorithm>             algorithm,
         Dispatch                        disp,
         DispatchRest...                 disp_rest)
  {
    dispatch (visitor, params, algorithms, algorithm, disp_rest...);
  }
};

template <class    Visitor,
          class    Algorithm,
          class    Dispatch,
          class... DispatchRest>
static inline void
dispatch (Visitor                         visitor,
          const GimpPaintCoreLoopsParams *params,
          GimpPaintCoreLoopsAlgorithm     algorithms,
          identity<Algorithm>             algorithm,
          Dispatch                        disp,
          DispatchRest...                 disp_rest)
{
  dispatch_impl<Algorithm, Dispatch>::apply (
    visitor, params, algorithms, algorithm, disp, disp_rest...);
}


/* value_to_float():
 *
 * Converts a component value to float.
 */

static inline gfloat
value_to_float (guint8 value)
{
  return value / 255.0f;
}

static inline gfloat
value_to_float (gfloat value)
{
  return value;
}

template <class T>
static inline gfloat
value_to_float (T value) = delete;


/* AlgorithmBase:
 *
 * The base class of the algorithm hierarchy.
 */

struct AlgorithmBase
{
  /* Used to filter-out dispatch functions; see the description of dispatch().
   * Algorithms that redefine 'filter' should bitwise-OR their filter with that
   * of their base class.
   */
  static constexpr guint          filter                 = 0;

  /* See CanvasBufferIterator. */
  static constexpr gint           canvas_buffer_iterator = -1;
  static constexpr GeglAccessMode canvas_buffer_access   = {};

  /* The current number of iterators used by the hierarchy.  Algorithms should
   * use the 'n_iterators' value of their base class as the base-index for
   * their iterators, and redefine 'n_iterators' by adding the number of
   * iterators they use to this value.
   */
  static constexpr gint           n_iterators            = 0;

  /* Non-static data members should be initialized in the constructor, and
   * should not be further modified.
   */
  explicit
  AlgorithmBase (const GimpPaintCoreLoopsParams *params)
  {
  }

  /* Algorithms should store their dynamic state in the 'State' member class
   * template.  This template will be instantiated with the most-derived type
   * of the hierarchy, which allows an algorithm to depend on the properties of
   * its descendants.  Algorithms that provide their own 'State' class should
   * derive it from the 'State' class of their base class, passing 'Derived' as
   * the template argument.
   *
   * Algorithms can be run in parallel on multiple threads.  In this case, each
   * thread uses its own 'State' object, while the algorithm object itself is
   * either shared, or is a copy of a shared algorithm object.  Either way, the
   * algorithm object itself is immutable, while the state object is mutable.
   */
  template <class Derived>
  struct State
  {
  };

  /* The 'init()' function is called once per state object before processing
   * starts, and should initialize the state object, and, if necessary, the
   * iterator.
   *
   * 'params' is the same parameter struct passed to the constructor.  'state'
   * is the state object.  'iter' is the iterator; each distinct state object
   * uses a distinct iterator.  'roi' is the full region to be processed.
   * 'area' is the subregion to be processed by the current state object.
   *
   * An algorithm that overrides this function should call the 'init()'
   * function of its base class first, using the same arguments.
   */
  template <class Derived>
  void
  init (const GimpPaintCoreLoopsParams *params,
        State<Derived>                 *state,
        GeglBufferIterator             *iter,
        const GeglRectangle            *roi,
        const GeglRectangle            *area) const
  {
  }

  /* The 'init_step()' function is called once after each
   * 'gegl_buffer_iterator_next()' call, and should perform any necessary
   * initialization required before processing the current chunk.
   *
   * The parameters are the same as for 'init()'.
   *
   * An algorithm that overrides this function should call the 'init_step()'
   * function of its base class first, using the same arguments.
   */
  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area) const
  {
  }

  /* The 'process_row()' function is called for each row in the current chunk,
   * and should perform the actual processing.
   *
   * The parameters are the same as for 'init()', with the addition of 'y',
   * which is the current row.
   *
   * An algorithm that overrides this function should call the 'process_row()'
   * function of its base class first, using the same arguments.
   */
  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
  }
};


/* BasicDispatch:
 *
 * A class template implementing a simple dispatch function object, which adds
 * an algorithm to the hierarchy unconditionally.  'AlgorithmTemplate' is the
 * alogithm class template (usually a helper class, rather than an actual
 * algorithm), and 'Mask' is the dispatch function mask, as described in
 * 'dispatch()'.
 */

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask>
struct BasicDispatch
{
  static constexpr guint mask = Mask;

  template <class Visitor,
            class Algorithm>
  void
  operator () (Visitor                         visitor,
               const GimpPaintCoreLoopsParams *params,
               GimpPaintCoreLoopsAlgorithm     algorithms,
               identity<Algorithm>             algorithm) const
  {
    visitor (identity<AlgorithmTemplate<Algorithm>> ());
  }
};


/* AlgorithmDispatch:
 *
 * A class template implementing a dispatch function suitable for dispatching
 * algorithms.  'AlgorithmTemplate' is the algorithm class template, 'Mask' is
 * the dispatch function mask, as described in 'dispatch()', and 'Dependencies'
 * is a list of (types of) dispatch functions the algorithm depends on, usd as
 * explained below.
 *
 * 'AlgorithmDispatch' adds the algorithm to the hierarchy if it's included in
 * the set of requested algorithms; specifically, if the bitwise-AND of the
 * requested-algorithms bitset and of 'Mask' is equal to 'Mask'.
 *
 * Before adding the algorithm to the hierarchy, the hierarchy is augmented by
 * dispatching through the list of dependencies, in order.
 */

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask,
          class...                    Dependencies>
struct AlgorithmDispatch
{
  static constexpr guint mask = Mask;

  template <class Visitor,
            class Algorithm>
  void
  operator () (Visitor                         visitor,
               const GimpPaintCoreLoopsParams *params,
               GimpPaintCoreLoopsAlgorithm     algorithms,
               identity<Algorithm>             algorithm) const
  {
    if ((algorithms & mask) == mask)
      {
        dispatch (
          [&] (auto algorithm)
          {
            using NewAlgorithm = typename decltype (algorithm)::type;

            visitor (identity<AlgorithmTemplate<NewAlgorithm>> ());
          },
          params, algorithms, algorithm, Dependencies ()...);
      }
    else
      {
        visitor (algorithm);
      }
  }
};


/* PaintBuf, dispatch_paint_buf():
 *
 * An algorithm helper class, providing access to the paint buffer.  Algorithms
 * that use the paint buffer should specify 'dispatch_paint_buf()' as a
 * dependency, and access 'PaintBuf' members through their base type/subobject.
 */

template <class Base>
struct PaintBuf : Base
{
  /* Component type of the paint buffer. */
  using paint_type = gfloat;

  static constexpr guint filter = Base::filter | ALGORITHM_PAINT_BUF;

  /* Paint buffer stride, in 'paint_type' elements. */
  gint        paint_stride;
  /* Pointer to the start of the paint buffer data. */
  paint_type *paint_data;

  explicit
  PaintBuf (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    paint_stride = gimp_temp_buf_get_width (params->paint_buf) * 4;
    paint_data   = (paint_type *) gimp_temp_buf_get_data (params->paint_buf);
  }
};

static BasicDispatch<PaintBuf, ALGORITHM_PAINT_BUF> dispatch_paint_buf;


/* PaintMask, dispatch_paint_mask():
 *
 * An algorithm helper class, providing access to the paint mask.  Algorithms
 * that use the paint mask should specify 'dispatch_paint_mask()' as a
 * dependency, and access 'PaintMask' members through their base type/
 * subobject.
 */

template <class Base,
          class MaskType>
struct PaintMask : Base
{
  /* Component type of the paint mask. */
  using mask_type = MaskType;

  static constexpr guint filter = Base::filter | ALGORITHM_PAINT_MASK;

  /* Paint mask stride, in 'mask_type' elements. */
  gint             mask_stride;
  /* Pointer to the start of the paint mask data, taking the mask offset into
   * account.
   */
  const mask_type *mask_data;

  explicit
  PaintMask (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    mask_stride = gimp_temp_buf_get_width (params->paint_mask);
    mask_data   =
      (const mask_type *) gimp_temp_buf_get_data (params->paint_mask) +
      params->paint_mask_offset_y * mask_stride                       +
      params->paint_mask_offset_x;
  }
};

struct DispatchPaintMask
{
  static constexpr guint mask = ALGORITHM_PAINT_MASK;

  template <class Visitor,
            class Algorithm>
  void
  operator () (Visitor                         visitor,
               const GimpPaintCoreLoopsParams *params,
               GimpPaintCoreLoopsAlgorithm     algorithms,
               identity<Algorithm>             algorithm) const
  {
    const Babl *mask_format = gimp_temp_buf_get_format (params->paint_mask);

    if (mask_format == babl_format ("Y u8"))
      visitor (identity<PaintMask<Algorithm, guint8>> ());
    else if (mask_format == babl_format ("Y float"))
      visitor (identity<PaintMask<Algorithm, gfloat>> ());
    else
      g_warning ("Mask format not supported: %s", babl_get_name (mask_format));
  }
} static dispatch_paint_mask;


/* Stipple, dispatch_stipple():
 *
 * An algorithm helper class, providing access to the 'stipple' parameter.
 * Algorithms that use the 'stipple' parameter should specify
 * 'dispatch_stipple()' as a dependency, and access 'Stipple' members through
 * their base type/subobject.
 */

template <class    Base,
          gboolean StippleFlag>
struct Stipple : Base
{
  static constexpr guint filter = Base::filter | ALGORITHM_STIPPLE;

  /* The value of the 'stipple' parameter, usable as a constant expression. */
  static constexpr gboolean stipple = StippleFlag;

  using Base::Base;
};

struct DispatchStipple
{
  static constexpr guint mask = ALGORITHM_STIPPLE;

  template <class Visitor,
            class Algorithm>
  void
  operator () (Visitor                         visitor,
               const GimpPaintCoreLoopsParams *params,
               GimpPaintCoreLoopsAlgorithm     algorithms,
               identity<Algorithm>             algorithm) const
  {
    if (params->stipple)
      visitor (identity<Stipple<Algorithm, TRUE>> ());
    else
      visitor (identity<Stipple<Algorithm, FALSE>> ());
  }
} static dispatch_stipple;


/* CanvasBufferIterator:
 *
 * An algorithm helper class, providing iterator-access to the canvas buffer.
 * Algorithms that iterate over the canvas buffer should derive from this
 * class, and access its members through their base type/subobject.
 *
 * 'Base' is the base class to use, which should normally be the base template-
 * parameter class passed to the algorithm.  'Access' specifies the desired
 * iterator access to the canvas buffer.
 */

template <class Base,
          guint Access>
struct CanvasBufferIterator : Base
{
  /* The iterator index of the canvas buffer. */
  static constexpr gint           canvas_buffer_iterator =
    Base::canvas_buffer_iterator < 0 ? Base::n_iterators :
                                       Base::canvas_buffer_iterator;
  /* Used internally. */
  static constexpr GeglAccessMode canvas_buffer_access   =
    (GeglAccessMode) (Base::canvas_buffer_access | Access);
  /* The total number of iterators used by the hierarchy, up to, and including,
   * the current class.
   */
  static constexpr gint           n_iterators            =
    Base::canvas_buffer_iterator < 0 ? Base::n_iterators + 1:
                                       Base::n_iterators;

  using Base::Base;

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  init (const GimpPaintCoreLoopsParams *params,
        State<Derived>                 *state,
        GeglBufferIterator             *iter,
        const GeglRectangle            *roi,
        const GeglRectangle            *area) const
  {
    Base::init (params, state, iter, roi, area);

    if (Base::canvas_buffer_iterator < 0)
      {
        gegl_buffer_iterator_add (iter, params->canvas_buffer, area, 0,
                                  babl_format ("Y float"),
                                  Derived::canvas_buffer_access,
                                  GEGL_ABYSS_NONE);
      }
  }
};


/* CombinePaintMaskToCanvasMaskToPaintBufAlpha,
 * dispatch_combine_paint_mask_to_canvas_mask_to_paint_buf_alpha():
 *
 * An algorithm class, providing an optimized version combining both the
 * COMBINE_PAINT_MASK_TO_CANVAS_MASK and the CANVAS_BUFFER_TO_PAINT_BUF_ALPHA
 * algorithms.  Used instead of the individual implementations, when both
 * algorithms are requested.
 */

template <class Base>
struct CombinePaintMaskToCanvasMaskToPaintBufAlpha :
  CanvasBufferIterator<Base, GEGL_BUFFER_READWRITE>
{
  using base_type = CanvasBufferIterator<Base, GEGL_BUFFER_READWRITE>;
  using mask_type = typename base_type::mask_type;

  static constexpr guint filter =
    base_type::filter                                                 |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_MASK |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA  |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER;

  using base_type::base_type;

  template <class Derived>
  struct State : base_type::template State<Derived>
  {
    gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area) const
  {
    base_type::init_step (params, state, iter, roi, area);

    state->canvas_pixel =
      (gfloat *) iter->data[base_type::canvas_buffer_iterator];
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
    base_type::process_row (params, state, iter, roi, area, y);

    gint             mask_offset  = (y              - roi->y) * this->mask_stride +
                                    (iter->roi[0].x - roi->x);
    const mask_type *mask_pixel   = &this->mask_data[mask_offset];
    gint             paint_offset = (y              - roi->y) * this->paint_stride +
                                    (iter->roi[0].x - roi->x) * 4;
    gfloat          *paint_pixel  = &this->paint_data[paint_offset];
    gint             x;

    for (x = 0; x < iter->roi[0].width; x++)
      {
        if (base_type::stipple)
          {
            state->canvas_pixel[0] += (1.0 - state->canvas_pixel[0])  *
                                      value_to_float (*mask_pixel)    *
                                      params->paint_opacity;
          }
        else
          {
            if (params->paint_opacity > state->canvas_pixel[0])
              {
                state->canvas_pixel[0] += (params->paint_opacity - state->canvas_pixel[0]) *
                                           value_to_float (*mask_pixel)                    *
                                           params->paint_opacity;
              }
          }

        paint_pixel[3] *= state->canvas_pixel[0];

        mask_pixel          += 1;
        state->canvas_pixel += 1;
        paint_pixel         += 4;
      }
  }
};

static AlgorithmDispatch<
  CombinePaintMaskToCanvasMaskToPaintBufAlpha,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_MASK |
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA,
  decltype (dispatch_paint_buf),
  decltype (dispatch_paint_mask),
  decltype (dispatch_stipple)
>
dispatch_combine_paint_mask_to_canvas_mask_to_paint_buf_alpha;


/* CombinePaintMaskToCanvasMask, dispatch_combine_paint_mask_to_canvas_mask():
 *
 * An algorithm class, implementing the COMBINE_PAINT_MASK_TO_CANVAS_MASK
 * algorithm.
 */

template <class Base>
struct CombinePaintMaskToCanvasMask :
  CanvasBufferIterator<Base, GEGL_BUFFER_READWRITE>
{
  using base_type = CanvasBufferIterator<Base, GEGL_BUFFER_READWRITE>;
  using mask_type = typename base_type::mask_type;

  static constexpr guint filter =
    base_type::filter                                                 |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_MASK |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER;

  using base_type::base_type;

  template <class Derived>
  struct State : base_type::template State<Derived>
  {
    gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area) const
  {
    base_type::init_step (params, state, iter, roi, area);

    state->canvas_pixel =
      (gfloat *) iter->data[base_type::canvas_buffer_iterator];
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
    base_type::process_row (params, state, iter, roi, area, y);

    gint             mask_offset = (y              - roi->y) * this->mask_stride +
                                   (iter->roi[0].x - roi->x);
    const mask_type *mask_pixel  = &this->mask_data[mask_offset];
    gint             x;

    for (x = 0; x < iter->roi[0].width; x++)
      {
        if (base_type::stipple)
          {
            state->canvas_pixel[0] += (1.0 - state->canvas_pixel[0]) *
                                      value_to_float (*mask_pixel)   *
                                      params->paint_opacity;
          }
        else
          {
            if (params->paint_opacity > state->canvas_pixel[0])
              {
                state->canvas_pixel[0] += (params->paint_opacity - state->canvas_pixel[0]) *
                                          value_to_float (*mask_pixel)                     *
                                          params->paint_opacity;
              }
          }

        mask_pixel          += 1;
        state->canvas_pixel += 1;
      }
  }
};

static AlgorithmDispatch<
  CombinePaintMaskToCanvasMask,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_MASK,
  decltype (dispatch_paint_mask),
  decltype (dispatch_stipple)
>
dispatch_combine_paint_mask_to_canvas_mask;


/* CanvasBufferToPaintBufAlpha, dispatch_canvas_buffer_to_paint_buf_alpha():
 *
 * An algorithm class, implementing the CANVAS_BUFFER_TO_PAINT_BUF_ALPHA
 * algorithm.
 */

template <class Base>
struct CanvasBufferToPaintBufAlpha : CanvasBufferIterator<Base,
                                                          GEGL_BUFFER_READ>
{
  using base_type = CanvasBufferIterator<Base, GEGL_BUFFER_READ>;

  static constexpr guint filter =
    base_type::filter                                                |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER;

  using base_type::base_type;

  template <class Derived>
  struct State : base_type::template State<Derived>
  {
    const gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area) const
  {
    base_type::init_step (params, state, iter, roi, area);

    state->canvas_pixel =
      (const gfloat *) iter->data[base_type::canvas_buffer_iterator];
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
    base_type::process_row (params, state, iter, roi, area, y);

    /* Copy the canvas buffer in rect to the paint buffer's alpha channel */

    gint    paint_offset = (y              - roi->y) * this->paint_stride +
                           (iter->roi[0].x - roi->x) * 4;
    gfloat *paint_pixel  = &this->paint_data[paint_offset];
    gint    x;

    for (x = 0; x < iter->roi[0].width; x++)
      {
        paint_pixel[3] *= *state->canvas_pixel;

        state->canvas_pixel += 1;
        paint_pixel         += 4;
      }
  }
};

static AlgorithmDispatch<
  CanvasBufferToPaintBufAlpha,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA,
  decltype (dispatch_paint_buf)
>
dispatch_canvas_buffer_to_paint_buf_alpha;


/* PaintMaskToPaintBuffer, dispatch_paint_mask_to_paint_buffer():
 *
 * An algorithm class, implementing the PAINT_MASK_TO_PAINT_BUFFER algorithm.
 */

template <class Base>
struct PaintMaskToPaintBuffer : Base
{
  using mask_type = typename Base::mask_type;

  static constexpr guint filter =
    Base::filter |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER;

  explicit
  PaintMaskToPaintBuffer (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    /* Validate that the paint buffer is within the bounds of the paint mask */
    g_return_if_fail (gimp_temp_buf_get_width (params->paint_buf) <=
                      gimp_temp_buf_get_width (params->paint_mask) -
                      params->paint_mask_offset_x);
    g_return_if_fail (gimp_temp_buf_get_height (params->paint_buf) <=
                      gimp_temp_buf_get_height (params->paint_mask) -
                      params->paint_mask_offset_y);
  }

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, y);

    gint             paint_offset = (y              - roi->y) * this->paint_stride +
                                    (iter->roi[0].x - roi->x) * 4;
    gfloat          *paint_pixel  = &this->paint_data[paint_offset];
    gint             mask_offset  = (y              - roi->y) * this->mask_stride +
                                    (iter->roi[0].x - roi->x);
    const mask_type *mask_pixel   = &this->mask_data[mask_offset];
    gint             x;

    for (x = 0; x < iter->roi[0].width; x++)
      {
        paint_pixel[3] *= value_to_float (*mask_pixel) * params->paint_opacity;

        mask_pixel  += 1;
        paint_pixel += 4;
      }
  }
};

static AlgorithmDispatch<
  PaintMaskToPaintBuffer,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER,
  decltype (dispatch_paint_buf),
  decltype (dispatch_paint_mask)
>
dispatch_paint_mask_to_paint_buffer;


/* DoLayerBlend, dispatch_do_layer_blend():
 *
 * An algorithm class, implementing the DO_LAYER_BLEND algorithm.
 */

template <class Base>
struct DoLayerBlend : Base
{
  static constexpr guint filter =
    Base::filter |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND;

  static constexpr gint iterator_base = Base::n_iterators;
  static constexpr gint n_iterators   = Base::n_iterators + 3;

  const Babl             *iterator_format;
  GimpOperationLayerMode  layer_mode;

  explicit
  DoLayerBlend (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    layer_mode.layer_mode          = params->paint_mode;
    layer_mode.opacity             = params->image_opacity;
    layer_mode.function            = gimp_layer_mode_get_function (params->paint_mode);
    layer_mode.blend_function      = gimp_layer_mode_get_blend_function (params->paint_mode);
    layer_mode.blend_space         = gimp_layer_mode_get_blend_space (params->paint_mode);
    layer_mode.composite_space     = gimp_layer_mode_get_composite_space (params->paint_mode);
    layer_mode.composite_mode      = gimp_layer_mode_get_paint_composite_mode (params->paint_mode);
    layer_mode.real_composite_mode = layer_mode.composite_mode;

    iterator_format = gimp_layer_mode_get_format (params->paint_mode,
                                                  layer_mode.composite_space,
                                                  layer_mode.blend_space,
                                                  gimp_temp_buf_get_format (params->paint_buf));

    g_return_if_fail (gimp_temp_buf_get_format (params->paint_buf) == iterator_format);
  }

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    GeglRectangle  process_roi;

    gfloat        *out_pixel;
    gfloat        *in_pixel;
    gfloat        *mask_pixel;
    gfloat        *paint_pixel;
  };

  template <class Derived>
  void
  init (const GimpPaintCoreLoopsParams *params,
        State<Derived>                 *state,
        GeglBufferIterator             *iter,
        const GeglRectangle            *roi,
        const GeglRectangle            *area) const
  {
    Base::init (params, state, iter, roi, area);

    GeglRectangle mask_area = *area;

    mask_area.x -= params->mask_offset_x;
    mask_area.y -= params->mask_offset_y;

    gegl_buffer_iterator_add (iter, params->dest_buffer, area, 0,
                              iterator_format,
                              GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

    gegl_buffer_iterator_add (iter, params->src_buffer, area, 0,
                              iterator_format,
                              GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

    if (params->mask_buffer)
      {
        gegl_buffer_iterator_add (iter, params->mask_buffer, &mask_area, 0,
                                  babl_format ("Y float"),
                                  GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      }
  }

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area) const
  {
    Base::init_step (params, state, iter, roi, area);

    state->out_pixel  = (gfloat *) iter->data[iterator_base + 0];
    state->in_pixel   = (gfloat *) iter->data[iterator_base + 1];
    state->mask_pixel = NULL;

    state->paint_pixel = this->paint_data                               +
                         (iter->roi[0].y - roi->y) * this->paint_stride +
                         (iter->roi[0].x - roi->x) * 4;

    if (params->mask_buffer)
      state->mask_pixel = (gfloat *) iter->data[iterator_base + 2];

    state->process_roi.x      = iter->roi[0].x;
    state->process_roi.width  = iter->roi[0].width;
    state->process_roi.height = 1;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, y);

    state->process_roi.y = y;

    layer_mode.function ((GeglOperation*) &layer_mode,
                         state->in_pixel,
                         state->paint_pixel,
                         state->mask_pixel,
                         state->out_pixel,
                         iter->roi[0].width,
                         &state->process_roi,
                         0);

    state->in_pixel     += iter->roi[0].width * 4;
    state->out_pixel    += iter->roi[0].width * 4;
    if (params->mask_buffer)
      state->mask_pixel += iter->roi[0].width;
    state->paint_pixel  += this->paint_stride;
  }
};

static AlgorithmDispatch<
  DoLayerBlend,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND,
  decltype (dispatch_paint_buf)
>
dispatch_do_layer_blend;


/* gimp_paint_core_loops_process():
 *
 * Performs the set of algorithms requested in 'algorithms', specified as a
 * bitwise-OR of 'GimpPaintCoreLoopsAlgorithm' values, given the set of
 * parameters 'params'.
 *
 * Note that the order in which the algorithms are performed is currently
 * fixed, and follows their order of appearance in the
 * 'GimpPaintCoreLoopsAlgorithm' enum.
 */

void
gimp_paint_core_loops_process (const GimpPaintCoreLoopsParams *params,
                               GimpPaintCoreLoopsAlgorithm     algorithms)
{
  GeglRectangle roi;

  if (params->paint_buf)
    {
      roi.x      = params->paint_buf_offset_x;
      roi.y      = params->paint_buf_offset_y;
      roi.width  = gimp_temp_buf_get_width  (params->paint_buf);
      roi.height = gimp_temp_buf_get_height (params->paint_buf);
    }
  else
    {
      roi.x      = params->paint_buf_offset_x;
      roi.y      = params->paint_buf_offset_y;
      roi.width  = gimp_temp_buf_get_width (params->paint_mask) -
                   params->paint_mask_offset_x;
      roi.height = gimp_temp_buf_get_height (params->paint_mask) -
                   params->paint_mask_offset_y;
    }

  dispatch (
    [&] (auto algorithm_type)
    {
      using Algorithm = typename decltype (algorithm_type)::type;
      using State     = typename Algorithm::template State<Algorithm>;

      Algorithm algorithm (params);

      gimp_parallel_distribute_area (&roi, MIN_PARALLEL_SUB_AREA,
                                     [=] (const GeglRectangle *area)
        {
          State state;
          gint  y;

          if (Algorithm::n_iterators > 0)
            {
              GeglBufferIterator *iter;

              iter = gegl_buffer_iterator_empty_new ();

              algorithm.init (params, &state, iter, &roi, area);

              while (gegl_buffer_iterator_next (iter))
                {
                  algorithm.init_step (params, &state, iter, &roi, area);

                  for (y = 0; y < iter->roi[0].height; y++)
                    {
                      algorithm.process_row (params, &state,
                                             iter, &roi, area,
                                             iter->roi[0].y + y);
                    }
                }
            }
          else
            {
              GeglBufferIterator iter;

              iter.roi[0] = *area;

              algorithm.init      (params, &state, &iter, &roi, area);
              algorithm.init_step (params, &state, &iter, &roi, area);

              for (y = 0; y < iter.roi[0].height; y++)
                {
                  algorithm.process_row (params, &state,
                                         &iter, &roi, area,
                                         iter.roi[0].y + y);
                }
            }
        });
    },
    params, algorithms, identity<AlgorithmBase> (),
    dispatch_combine_paint_mask_to_canvas_mask_to_paint_buf_alpha,
    dispatch_combine_paint_mask_to_canvas_mask,
    dispatch_canvas_buffer_to_paint_buf_alpha,
    dispatch_paint_mask_to_paint_buffer,
    dispatch_do_layer_blend);
}


/* combine_paint_mask_to_canvas_mask():
 *
 * A convenience wrapper around 'gimp_paint_core_loops_process()', performing
 * just the COMBINE_PAINT_MASK_TO_CANVAS_MASK algorithm.
 */

void
combine_paint_mask_to_canvas_mask (const GimpTempBuf *paint_mask,
                                   gint               mask_x_offset,
                                   gint               mask_y_offset,
                                   GeglBuffer        *canvas_buffer,
                                   gint               x_offset,
                                   gint               y_offset,
                                   gfloat             opacity,
                                   gboolean           stipple)
{
  GimpPaintCoreLoopsParams params = {};

  params.canvas_buffer       = canvas_buffer;

  params.paint_buf_offset_x  = x_offset;
  params.paint_buf_offset_y  = y_offset;

  params.paint_mask          = paint_mask;
  params.paint_mask_offset_x = mask_x_offset;
  params.paint_mask_offset_y = mask_y_offset;

  params.stipple             = stipple;

  params.paint_opacity       = opacity;

  gimp_paint_core_loops_process (
    &params,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_MASK);
}


/* canvas_buffer_to_paint_buf_alpha():
 *
 * A convenience wrapper around 'gimp_paint_core_loops_process()', performing
 * just the CANVAS_BUFFER_TO_PAINT_BUF_ALPHA algorithm.
 */

void
canvas_buffer_to_paint_buf_alpha (GimpTempBuf  *paint_buf,
                                  GeglBuffer   *canvas_buffer,
                                  gint          x_offset,
                                  gint          y_offset)
{
  GimpPaintCoreLoopsParams params = {};

  params.canvas_buffer      = canvas_buffer;

  params.paint_buf          = paint_buf;
  params.paint_buf_offset_x = x_offset;
  params.paint_buf_offset_y = y_offset;

  gimp_paint_core_loops_process (
    &params,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA);
}


/* paint_mask_to_paint_buffer():
 *
 * A convenience wrapper around 'gimp_paint_core_loops_process()', performing
 * just the PAINT_MASK_TO_PAINT_BUFFER algorithm.
 */

void
paint_mask_to_paint_buffer (const GimpTempBuf  *paint_mask,
                            gint                mask_x_offset,
                            gint                mask_y_offset,
                            GimpTempBuf        *paint_buf,
                            gfloat              paint_opacity)
{
  GimpPaintCoreLoopsParams params = {};

  params.paint_buf           = paint_buf;

  params.paint_mask          = paint_mask;
  params.paint_mask_offset_x = mask_x_offset;
  params.paint_mask_offset_y = mask_y_offset;

  params.paint_opacity       = paint_opacity;

  gimp_paint_core_loops_process (
    &params,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUFFER);
}


/* do_layer_blend():
 *
 * A convenience wrapper around 'gimp_paint_core_loops_process()', performing
 * just the DO_LAYER_BLEND algorithm.
 */

void
do_layer_blend (GeglBuffer    *src_buffer,
                GeglBuffer    *dst_buffer,
                GimpTempBuf   *paint_buf,
                GeglBuffer    *mask_buffer,
                gfloat         opacity,
                gint           x_offset,
                gint           y_offset,
                gint           mask_x_offset,
                gint           mask_y_offset,
                GimpLayerMode  paint_mode)
{
  GimpPaintCoreLoopsParams params = {};

  params.paint_buf          = paint_buf;
  params.paint_buf_offset_x = x_offset;
  params.paint_buf_offset_y = y_offset;

  params.src_buffer         = src_buffer;
  params.dest_buffer        = dst_buffer;

  params.mask_buffer        = mask_buffer;
  params.mask_offset_x      = mask_x_offset;
  params.mask_offset_y      = mask_y_offset;

  params.image_opacity      = opacity;

  params.paint_mode         = paint_mode;

  gimp_paint_core_loops_process (
    &params,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND);
}


/* mask_components_onto():
 *
 * Copies the contents of 'src_buffer' and 'aux_buffer' into 'dst_buffer', over
 * 'roi'.  Components set in 'mask' are copied from 'aux_buffer', while those
 * not set in 'mask' are copied from 'src_buffer'.  'linear_mode' specifies
 * whether to iterate over the buffers use a linear format.  It should match
 * the linear mode of the painted-to drawable, to avoid modifying masked-out
 * components.
 *
 * Note that we don't integrate this function into the rest of the algorithm
 * framework, since it uses a (potentially) different format when iterating
 * over the buffers than the rest of the algorithms.
 */

void
mask_components_onto (GeglBuffer        *src_buffer,
                      GeglBuffer        *aux_buffer,
                      GeglBuffer        *dst_buffer,
                      GeglRectangle     *roi,
                      GimpComponentMask  mask,
                      gboolean           linear_mode)
{
  const Babl *iterator_format;

  if (linear_mode)
    iterator_format = babl_format ("RGBA float");
  else
    iterator_format = babl_format ("R'G'B'A float");

  gimp_parallel_distribute_area (roi, MIN_PARALLEL_SUB_AREA,
                                 [=] (const GeglRectangle *area)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (dst_buffer, area, 0,
                                       iterator_format,
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, src_buffer, area, 0,
                                iterator_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (iter, aux_buffer, area, 0,
                                iterator_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *dest    = (gfloat *)iter->data[0];
          gfloat *src     = (gfloat *)iter->data[1];
          gfloat *aux     = (gfloat *)iter->data[2];
          glong   samples = iter->length;

          while (samples--)
            {
              dest[RED]   = (mask & GIMP_COMPONENT_MASK_RED)   ? aux[RED]   : src[RED];
              dest[GREEN] = (mask & GIMP_COMPONENT_MASK_GREEN) ? aux[GREEN] : src[GREEN];
              dest[BLUE]  = (mask & GIMP_COMPONENT_MASK_BLUE)  ? aux[BLUE]  : src[BLUE];
              dest[ALPHA] = (mask & GIMP_COMPONENT_MASK_ALPHA) ? aux[ALPHA] : src[ALPHA];

              src  += 4;
              aux  += 4;
              dest += 4;
            }
        }
    });
}
