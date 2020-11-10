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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

extern "C"
{

#include "paint-types.h"

#include "gegl/gimp-babl.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimptempbuf.h"

#include "operations/gimpoperationmaskcomponents.h"

#include "operations/layer-modes/gimpoperationlayermode.h"

#include "gimppaintcore-loops.h"

} /* extern "C" */


#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


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
  ALGORITHM_PAINT_BUF              = 1u << 31,
  ALGORITHM_PAINT_MASK             = 1u << 30,
  ALGORITHM_STIPPLE                = 1u << 29,
  ALGORITHM_COMP_MASK              = 1u << 28,
  ALGORITHM_TEMP_COMP_MASK         = 1u << 27,
  ALGORITHM_COMP_BUFFER            = 1u << 26,
  ALGORITHM_TEMP_COMP_BUFFER       = 1u << 25,
  ALGORITHM_CANVAS_BUFFER_ITERATOR = 1u << 24,
  ALGORITHM_MASK_BUFFER_ITERATOR   = 1u << 23
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
  static constexpr guint          filter          = 0;

  /* The current maximal number of iterators used by the hierarchy.  Algorithms
   * should redefine 'max_n_iterators' by adding the maximal number of
   * iterators they use to this value.
   */
  static constexpr gint           max_n_iterators = 0;

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
   * uses a distinct iterator; if the algorithm hierarchy doesn't use any
   * iterator, 'iter' may be NULL.  'roi' is the full region to be processed.
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
   * The parameters are the same as for 'init()', with the addition of 'rect',
   * which is the area of the current chunk.
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
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
  }

  /* The 'process_row()' function is called for each row in the current chunk,
   * and should perform the actual processing.
   *
   * The parameters are the same as for 'init_step()', with the addition of
   * 'y', which is the current row.
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
               const GeglRectangle            *rect,
               gint                            y) const
  {
  }

  /* The 'finalize_step()' function is called once per chunk after its
   * processing is done, and should finalize any chunk-specific resources of
   * the state object.
   *
   * 'params' is the same parameter struct passed to the constructor.  'state'
   * is the state object.
   *
   * An algorithm that overrides this function should call the
   * 'finalize_step()' function of its base class after performing its own
   * finalization, using the same arguments.
   */
  template <class Derived>
  void
  finalize_step (const GimpPaintCoreLoopsParams *params,
                 State<Derived>                 *state) const
  {
  }

  /* The 'finalize()' function is called once per state object after processing
   * is done, and should finalize the state object.
   *
   * 'params' is the same parameter struct passed to the constructor.  'state'
   * is the state object.
   *
   * An algorithm that overrides this function should call the 'finalize()'
   * function of its base class after performing its own finalization, using
   * the same arguments.
   */
  template <class Derived>
  void
  finalize (const GimpPaintCoreLoopsParams *params,
            State<Derived>                 *state) const
  {
  }
};


/* BasicDispatch:
 *
 * A class template implementing a simple dispatch function object, which adds
 * an algorithm to the hierarchy unconditionally.  'AlgorithmTemplate' is the
 * alogithm class template (usually a helper class, rather than an actual
 * algorithm), 'Mask' is the dispatch function mask, as described in
 * 'dispatch()', and 'Dependencies' is a list of (types of) dispatch functions
 * the algorithm depends on.
 *
 * Before adding the algorithm to the hierarchy, the hierarchy is augmented by
 * dispatching through the list of dependencies, in order.
 */

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask,
          class...                    Dependencies>
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
    dispatch (
      [&] (auto algorithm)
      {
        using NewAlgorithm = typename decltype (algorithm)::type;

        visitor (identity<AlgorithmTemplate<NewAlgorithm>> ());
      },
      params, algorithms, algorithm, Dependencies ()...);
  }
};

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask>
struct BasicDispatch<AlgorithmTemplate, Mask>
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
 * is a list of (types of) dispatch functions the algorithm depends on, used as
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


/* MandatoryAlgorithmDispatch:
 *
 * A class template implementing a dispatch function suitable for dispatching
 * algorithms that must be included in all hierarchies.  'AlgorithmTemplate' is
 * the algorithm class template, 'Mask' is the dispatch function mask, as
 * described in 'dispatch()', and 'Dependencies' is a list of (types of)
 * dispatch functions the algorithm depends on, used as explained below.
 *
 * 'MandatoryAlgorithmDispatch' verifies that the algorithm is included in the
 * set of requested algorithms (specifically, that the bitwise-AND of the
 * requested-algorithms bitset and of 'Mask' is equal to 'Mask'), and adds the
 * it to the hierarchy unconditionally.
 *
 * Before adding the algorithm to the hierarchy, the hierarchy is augmented by
 * dispatching through the list of dependencies, in order.
 */

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask,
          class...                    Dependencies>
struct MandatoryAlgorithmDispatch
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
    g_return_if_fail ((algorithms & Mask) == Mask);

    BasicDispatch<AlgorithmTemplate, Mask, Dependencies...> () (visitor,
                                                                params,
                                                                algorithms,
                                                                algorithm);
  }
};

/* SuppressedAlgorithmDispatch:
 *
 * A class template implementing a placeholder dispatch function suitable for
 * dispatching algorithms that are never included in any hierarchy.
 * 'AlgorithmTemplate' is the algorithm class template, 'Mask' is the dispatch
 * function mask, as described in 'dispatch()', and 'Dependencies' is a list of
 * (types of) dispatch functions the algorithm depends on.  Note that
 * 'AlgorithmTemplate' and 'Dependencies' are not actually used, and are merely
 * included for exposition.
 *
 * 'SuppressedAlgorithmDispatch' verifies that the algorithm is not included in
 * the set of requested algorithms (specifically, that the bitwise-AND of the
 * requested-algorithms bitset and of 'Mask' is not equal to 'Mask'), and
 * doesn't modify the hierarchy.
 */

template <template <class Base> class AlgorithmTemplate,
          guint                       Mask,
          class...                    Dependencies>
struct SuppressedAlgorithmDispatch
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
    g_return_if_fail ((algorithms & Mask) != Mask);

    visitor (algorithm);
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


/* CompMask, dispatch_comp_mask(), has_comp_mask(), comp_mask_data():
 *
 * An algorithm helper class, providing access to the mask used for
 * compositing.  When this class is part of the hierarchy, 'DoLayerBlend' uses
 * this buffer as the mask, instead of the input parameters' 'mask_buffer'.
 * Algorithms that use the compositing mask should specify
 * 'dispatch_comp_mask()' as a dependency, and access 'CompMask' members
 * through their base type/subobject.
 *
 * Note that 'CompMask' only provides *access* to the compositing mask, but
 * doesn't provide its actual *storage*.  This is the responsibility of the
 * algorithms that use 'CompMask'.  Algorithms that need temporary storage for
 * the compositing mask can use 'TempCompMask'.
 *
 * The 'has_comp_mask()' constexpr function determines if a given algorithm
 * hierarchy uses the compositing mask.
 *
 * The 'comp_mask_data()' function returns a pointer to the compositing mask
 * data for the current row if the hierarchy uses the compositing mask, or NULL
 * otherwise.
 */

template <class Base>
struct CompMask : Base
{
  /* Component type of the compositing mask. */
  using comp_mask_type = gfloat;

  static constexpr guint filter = Base::filter | ALGORITHM_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    /* Pointer to the compositing mask data for the current row. */
    comp_mask_type *comp_mask_data;
  };
};

static BasicDispatch<CompMask, ALGORITHM_COMP_MASK> dispatch_comp_mask;

template <class Base>
static constexpr gboolean
has_comp_mask (const CompMask<Base> *algorithm)
{
  return TRUE;
}

static constexpr gboolean
has_comp_mask (const AlgorithmBase *algorithm)
{
  return FALSE;
}

template <class Base,
          class State>
static gfloat *
comp_mask_data (const CompMask<Base> *algorithm,
                State                *state)
{
  return state->comp_mask_data;
}

template <class State>
static gfloat *
comp_mask_data (const AlgorithmBase *algorithm,
                State               *state)
{
  return NULL;
}


/* TempCompMask, dispatch_temp_comp_mask():
 *
 * An algorithm helper class, providing temporary storage for the compositing
 * mask.  Algorithms that need a temporary compositing mask should specify
 * 'dispatch_temp_comp_mask()' as a dependency, which itself includes
 * 'dispatch_comp_mask()' as a dependency.
 */

template <class Base>
struct TempCompMask : Base
{
  static constexpr guint filter = Base::filter | ALGORITHM_TEMP_COMP_MASK;

  using Base::Base;

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->comp_mask_data = gegl_scratch_new (gfloat, rect->width);
  }


  template <class Derived>
  void
  finalize_step (const GimpPaintCoreLoopsParams *params,
                 State<Derived>                 *state) const
  {
    gegl_scratch_free (state->comp_mask_data);

    Base::finalize_step (params, state);
  }
};

static BasicDispatch<
  TempCompMask,
  ALGORITHM_TEMP_COMP_MASK,
  decltype (dispatch_comp_mask)
> dispatch_temp_comp_mask;


/* CompBuffer, dispatch_comp_buffer(), has_comp_buffer(), comp_buffer_data():
 *
 * An algorithm helper class, providing access to the output buffer used for
 * compositing.  When this class is part of the hierarchy, 'DoLayerBlend' uses
 * this buffer as the output buffer, instead of the input parameters'
 * 'dest_buffer'.  Algorithms that use the compositing buffer should specify
 * 'dispatch_comp_buffer()' as a dependency, and access 'CompBuffer' members
 * through their base type/subobject.
 *
 * Note that 'CompBuffer' only provides *access* to the compositing buffer, but
 * doesn't provide its actual *storage*.  This is the responsibility of the
 * algorithms that use 'CompBuffer'.  Algorithms that need temporary storage
 * for the compositing buffer can use 'TempCompBuffer'.
 *
 * The 'has_comp_buffer()' constexpr function determines if a given algorithm
 * hierarchy uses the compositing buffer.
 *
 * The 'comp_buffer_data()' function returns a pointer to the compositing
 * buffer data for the current row if the hierarchy uses the compositing
 * buffer, or NULL otherwise.
 */

template <class Base>
struct CompBuffer : Base
{
  /* Component type of the compositing buffer. */
  using comp_buffer_type = gfloat;

  static constexpr guint filter = Base::filter | ALGORITHM_COMP_BUFFER;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    /* Pointer to the compositing buffer data for the current row. */
    comp_buffer_type *comp_buffer_data;
  };
};

static BasicDispatch<CompBuffer, ALGORITHM_COMP_BUFFER> dispatch_comp_buffer;

template <class Base>
static constexpr gboolean
has_comp_buffer (const CompBuffer<Base> *algorithm)
{
  return TRUE;
}

static constexpr gboolean
has_comp_buffer (const AlgorithmBase *algorithm)
{
  return FALSE;
}

template <class Base,
          class State>
static gfloat *
comp_buffer_data (const CompBuffer<Base> *algorithm,
                  State                  *state)
{
  return state->comp_buffer_data;
}

template <class State>
static gfloat *
comp_buffer_data (const AlgorithmBase *algorithm,
                  State               *state)
{
  return NULL;
}


/* TempCompBuffer, dispatch_temp_comp_buffer():
 *
 * An algorithm helper class, providing temporary storage for the compositing
 * buffer.  Algorithms that need a temporary compositing buffer should specify
 * 'dispatch_temp_comp_buffer()' as a dependency, which itself includes
 * 'dispatch_comp_buffer()' as a dependency.
 */

template <class Base>
struct TempCompBuffer : Base
{
  static constexpr guint filter = Base::filter | ALGORITHM_TEMP_COMP_BUFFER;

  using Base::Base;

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->comp_buffer_data = gegl_scratch_new (gfloat, 4 * rect->width);
  }


  template <class Derived>
  void
  finalize_step (const GimpPaintCoreLoopsParams *params,
                 State<Derived>                 *state) const
  {
    gegl_scratch_free (state->comp_buffer_data);

    Base::finalize_step (params, state);
  }
};

static BasicDispatch<
  TempCompBuffer,
  ALGORITHM_TEMP_COMP_BUFFER,
  decltype (dispatch_comp_buffer)
> dispatch_temp_comp_buffer;


/* CanvasBufferIterator, DispatchCanvasBufferIterator:
 *
 * An algorithm helper class, providing iterator-access to the canvas buffer.
 * Algorithms that iterate over the canvas buffer should specify
 * 'DispatchCanvasBufferIterator<Access>' as a dependency, where 'Access' is
 * the desired access mode to the canvas buffer, and access its members through
 * their base type/subobject.
 */

template <class    Base,
          guint    Access,
          gboolean First>
struct CanvasBufferIterator;

template <class Base,
          guint Access>
static constexpr gboolean
canvas_buffer_iterator_is_first (CanvasBufferIterator<Base, Access, TRUE> *algorithm)
{
  return FALSE;
}

static constexpr gboolean
canvas_buffer_iterator_is_first (AlgorithmBase *algorithm)
{
  return TRUE;
}

template <class    Base,
          guint    Access,
          gboolean First = canvas_buffer_iterator_is_first ((Base *) NULL)>
struct CanvasBufferIterator : Base
{
  /* The combined canvas-buffer access mode used by the hierarchy, up to, and
   * including, the current class.
   */
  static constexpr GeglAccessMode canvas_buffer_access =
    (GeglAccessMode) (Base::canvas_buffer_access | Access);

  using Base::Base;
};

template <class Base,
          guint Access>
struct CanvasBufferIterator<Base, Access, TRUE> : Base
{
  /* The combined canvas-buffer access mode used by the hierarchy, up to, and
   * including, the current class.
   */
  static constexpr GeglAccessMode canvas_buffer_access =
    (GeglAccessMode) Access;

  static constexpr gint           max_n_iterators      =
    Base::max_n_iterators + 1;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gint canvas_buffer_iterator;
  };

  template <class Derived>
  void
  init (const GimpPaintCoreLoopsParams *params,
        State<Derived>                 *state,
        GeglBufferIterator             *iter,
        const GeglRectangle            *roi,
        const GeglRectangle            *area) const
  {
    state->canvas_buffer_iterator = gegl_buffer_iterator_add (
      iter, params->canvas_buffer, area, 0, babl_format ("Y float"),
      Derived::canvas_buffer_access, GEGL_ABYSS_NONE);

    /* initialize the base class *after* initializing the iterator, to make
     * sure that canvas_buffer is the primary buffer of the iterator, if no
     * subclass added an iterator first.
     */
    Base::init (params, state, iter, roi, area);
  }
};

template <guint Access>
struct DispatchCanvasBufferIteratorHelper
{
  template <class Base>
  using algorithm_template = CanvasBufferIterator<Base, Access>;
};

template <guint Access>
using DispatchCanvasBufferIterator = BasicDispatch<
  DispatchCanvasBufferIteratorHelper<Access>::template algorithm_template,
  ALGORITHM_CANVAS_BUFFER_ITERATOR
>;


/* MaskBufferIterator, mask_buffer_iterator_dispatch(),
 * has_mask_buffer_iterator():
 *
 * An algorithm helper class, providing read-only iterator-access to the mask
 * buffer.  Algorithms that iterate over the mask buffer should specify
 * 'dispatch_mask_buffer_iterator()' as a dependency, and access
 * 'MaskBufferIterator' members through their base type/subobject.
 *
 * The 'has_mask_buffer_iterator()' constexpr function determines if a given
 * algorithm hierarchy uses has a mask-buffer iterator.
 *
 * The 'mask_buffer_iterator()' function returns the index of the mask-buffer
 * iterator if the hierarchy has one, or -1 otherwise.
 */

template <class Base>
struct MaskBufferIterator : Base
{
  static constexpr guint filter = Base::filter | ALGORITHM_MASK_BUFFER_ITERATOR;

  static constexpr gint max_n_iterators = Base::max_n_iterators + 1;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gint mask_buffer_iterator;
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

    state->mask_buffer_iterator = gegl_buffer_iterator_add (
      iter, params->mask_buffer, &mask_area, 0, babl_format ("Y float"),
      GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  }
};

struct DispatchMaskBufferIterator
{
  static constexpr guint mask = ALGORITHM_MASK_BUFFER_ITERATOR;

  template <class Visitor,
            class Algorithm>
  void
  operator () (Visitor                         visitor,
               const GimpPaintCoreLoopsParams *params,
               GimpPaintCoreLoopsAlgorithm     algorithms,
               identity<Algorithm>             algorithm) const
  {
    if (params->mask_buffer)
      visitor (identity<MaskBufferIterator<Algorithm>> ());
    else
      visitor (algorithm);
  }
} static dispatch_mask_buffer_iterator;

template <class Base>
static constexpr gboolean
has_mask_buffer_iterator (const MaskBufferIterator<Base> *algorithm)
{
  return TRUE;
}

static constexpr gboolean
has_mask_buffer_iterator (const AlgorithmBase *algorithm)
{
  return FALSE;
}

template <class Base,
          class State>
static gint
mask_buffer_iterator (const MaskBufferIterator<Base> *algorithm,
                      State                          *state)
{
  return state->mask_buffer_iterator;
}

template <class State>
static gint
mask_buffer_iterator (const AlgorithmBase *algorithm,
                      State               *state)
{
  return -1;
}


/* CombinePaintMaskToCanvasBufferToPaintBufAlpha,
 * dispatch_combine_paint_mask_to_canvas_buffer_to_paint_buf_alpha():
 *
 * An algorithm class, providing an optimized version combining both the
 * COMBINE_PAINT_MASK_TO_CANVAS_BUFFER and the CANVAS_BUFFER_TO_PAINT_BUF_ALPHA
 * algorithms.  Used instead of the individual implementations, when both
 * algorithms are requested.
 */

template <class Base>
struct CombinePaintMaskToCanvasBufferToPaintBufAlpha : Base
{
  using mask_type = typename Base::mask_type;

  static constexpr guint filter =
    Base::filter                                                        |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA    |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA       |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK          |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->canvas_pixel =
      (gfloat *) iter->items[state->canvas_buffer_iterator].data;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gint             mask_offset  = (y       - roi->y) * this->mask_stride +
                                    (rect->x - roi->x);
    const mask_type *mask_pixel   = &this->mask_data[mask_offset];
    gint             paint_offset = (y       - roi->y) * this->paint_stride +
                                    (rect->x - roi->x) * 4;
    gfloat          *paint_pixel  = &this->paint_data[paint_offset];
    gint             x;

    for (x = 0; x < rect->width; x++)
      {
        if (Base::stipple)
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

static SuppressedAlgorithmDispatch<
  CombinePaintMaskToCanvasBufferToPaintBufAlpha,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER |
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA,
  decltype (dispatch_paint_buf),
  decltype (dispatch_paint_mask),
  decltype (dispatch_stipple),
  DispatchCanvasBufferIterator<GEGL_BUFFER_READWRITE>
>
dispatch_combine_paint_mask_to_canvas_buffer_to_paint_buf_alpha;


/* CombinePaintMaskToCanvasBuffer,
 * dispatch_combine_paint_mask_to_canvas_buffer():
 *
 * An algorithm class, implementing the COMBINE_PAINT_MASK_TO_CANVAS_BUFFER
 * algorithm.
 */

template <class Base>
struct CombinePaintMaskToCanvasBuffer : Base
{
  using mask_type = typename Base::mask_type;

  static constexpr guint filter =
    Base::filter                                                        |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA    |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA       |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->canvas_pixel =
      (gfloat *) iter->items[state->canvas_buffer_iterator].data;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gint             mask_offset = (y       - roi->y) * this->mask_stride +
                                   (rect->x - roi->x);
    const mask_type *mask_pixel  = &this->mask_data[mask_offset];
    gint             x;

    for (x = 0; x < rect->width; x++)
      {
        if (Base::stipple)
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
  CombinePaintMaskToCanvasBuffer,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER,
  decltype (dispatch_paint_mask),
  decltype (dispatch_stipple),
  DispatchCanvasBufferIterator<GEGL_BUFFER_READWRITE>
>
dispatch_combine_paint_mask_to_canvas_buffer;


/* CanvasBufferToPaintBufAlpha, dispatch_canvas_buffer_to_paint_buf_alpha():
 *
 * An algorithm class, implementing the CANVAS_BUFFER_TO_PAINT_BUF_ALPHA
 * algorithm.
 */

template <class Base>
struct CanvasBufferToPaintBufAlpha : Base
{
  static constexpr guint filter =
    Base::filter                                                     |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA    |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK       |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    const gfloat *canvas_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->canvas_pixel =
      (const gfloat *) iter->items[state->canvas_buffer_iterator].data;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    /* Copy the canvas buffer in rect to the paint buffer's alpha channel */

    gint    paint_offset = (y       - roi->y) * this->paint_stride +
                           (rect->x - roi->x) * 4;
    gfloat *paint_pixel  = &this->paint_data[paint_offset];
    gint    x;

    for (x = 0; x < rect->width; x++)
      {
        paint_pixel[3] *= *state->canvas_pixel;

        state->canvas_pixel += 1;
        paint_pixel         += 4;
      }
  }
};

static SuppressedAlgorithmDispatch<
  CanvasBufferToPaintBufAlpha,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_PAINT_BUF_ALPHA,
  decltype (dispatch_paint_buf),
  DispatchCanvasBufferIterator<GEGL_BUFFER_READ>
>
dispatch_canvas_buffer_to_paint_buf_alpha;


/* PaintMaskToPaintBufAlpha, dispatch_paint_mask_to_paint_buf_alpha():
 *
 * An algorithm class, implementing the PAINT_MASK_TO_PAINT_BUF_ALPHA
 * algorithm.
 */

template <class Base>
struct PaintMaskToPaintBufAlpha : Base
{
  using mask_type = typename Base::mask_type;

  static constexpr guint filter =
    Base::filter |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK    |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  explicit
  PaintMaskToPaintBufAlpha (const GimpPaintCoreLoopsParams *params) :
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
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gint             paint_offset = (y       - roi->y) * this->paint_stride +
                                    (rect->x - roi->x) * 4;
    gfloat          *paint_pixel  = &this->paint_data[paint_offset];
    gint             mask_offset  = (y       - roi->y) * this->mask_stride +
                                    (rect->x - roi->x);
    const mask_type *mask_pixel   = &this->mask_data[mask_offset];
    gint             x;

    for (x = 0; x < rect->width; x++)
      {
        paint_pixel[3] *= value_to_float (*mask_pixel) * params->paint_opacity;

        mask_pixel  += 1;
        paint_pixel += 4;
      }
  }
};

static SuppressedAlgorithmDispatch<
  PaintMaskToPaintBufAlpha,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_PAINT_BUF_ALPHA,
  decltype (dispatch_paint_buf),
  decltype (dispatch_paint_mask)
>
dispatch_paint_mask_to_paint_buf_alpha;


/* CanvasBufferToCompMask, dispatch_canvas_buffer_to_comp_mask():
 *
 * An algorithm class, implementing the CANVAS_BUFFER_TO_COMP_MASK algorithm.
 */

template <class    Base,
          gboolean Direct>
struct CanvasBufferToCompMask : Base
{
  using comp_mask_type = typename Base::comp_mask_type;

  static constexpr guint filter =
    Base::filter                                               |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    const gfloat *canvas_pixel;
    const gfloat *mask_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->canvas_pixel =
      (const gfloat *) iter->items[state->canvas_buffer_iterator].data;
    state->mask_pixel   =
      (const gfloat *) iter->items[state->mask_buffer_iterator].data;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    comp_mask_type *comp_mask_pixel = state->comp_mask_data;
    gint            x;

    for (x = 0; x < rect->width; x++)
      {
        comp_mask_pixel[0] = state->canvas_pixel[0] * state->mask_pixel[0];

        comp_mask_pixel     += 1;
        state->canvas_pixel += 1;
        state->mask_pixel   += 1;
      }
  }
};

template <class Base>
struct CanvasBufferToCompMask<Base, TRUE> : Base
{
  static constexpr guint filter =
    Base::filter                                               |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->comp_mask_data =
      (gfloat *) iter->items[state->canvas_buffer_iterator].data - rect->width;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    state->comp_mask_data += rect->width;
  }
};

struct DispatchCanvasBufferToCompMask
{
  static constexpr guint mask =
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK;

  template <class Base>
  using AlgorithmDirect   = CanvasBufferToCompMask<Base, TRUE>;
  template <class Base>
  using AlgorithmIndirect = CanvasBufferToCompMask<Base, FALSE>;

  using DispatchDirect    = BasicDispatch<
    AlgorithmDirect,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK,
    decltype (dispatch_comp_mask)
  >;
  using DispatchIndirect  = BasicDispatch<
    AlgorithmIndirect,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK,
    decltype (dispatch_temp_comp_mask)
  >;

  template <class Algorithm,
            gboolean HasMaskBufferIterator = has_mask_buffer_iterator (
                                               (Algorithm *) NULL)>
  struct Dispatch : DispatchIndirect
  {
  };

  template <class Algorithm>
  struct Dispatch<Algorithm, FALSE> : DispatchDirect
  {
  };

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

            Dispatch<NewAlgorithm> () (visitor, params, algorithms, algorithm);
          },
          params, algorithms, algorithm,
          DispatchCanvasBufferIterator<GEGL_BUFFER_READ> (),
          dispatch_mask_buffer_iterator);
      }
    else
      {
        visitor (algorithm);
      }
  }
} static dispatch_canvas_buffer_to_comp_mask;


/* PaintMaskToCompMask, dispatch_paint_mask_to_comp_mask():
 *
 * An algorithm class, implementing the PAINT_MASK_TO_COMP_MASK algorithm.
 */

template <class    Base,
          gboolean Direct>
struct PaintMaskToCompMask : Base
{
  using mask_type      = typename Base::mask_type;
  using comp_mask_type = typename Base::comp_mask_type;

  static constexpr guint filter =
    Base::filter |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    const gfloat *mask_pixel;
  };

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    if (has_mask_buffer_iterator (this))
      {
        state->mask_pixel =
          (const gfloat *) iter->items[mask_buffer_iterator (this, state)].data;
      }
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gint             mask_offset = (y       - roi->y) * this->mask_stride +
                                   (rect->x - roi->x);
    const mask_type *mask_pixel  = &this->mask_data[mask_offset];
    comp_mask_type  *comp_mask_pixel = state->comp_mask_data;
    gint             x;

    if (has_mask_buffer_iterator (this))
      {
        for (x = 0; x < rect->width; x++)
          {
            comp_mask_pixel[0] = value_to_float (mask_pixel[0]) *
                                 state->mask_pixel[0]           *
                                 params->paint_opacity;

            comp_mask_pixel   += 1;
            mask_pixel        += 1;
            state->mask_pixel += 1;
          }
      }
    else
      {
        for (x = 0; x < rect->width; x++)
          {
            comp_mask_pixel[0] = value_to_float (mask_pixel[0]) *
                                 params->paint_opacity;

            comp_mask_pixel += 1;
            mask_pixel      += 1;
          }
      }
  }
};

template <class Base>
struct PaintMaskToCompMask<Base, TRUE> : Base
{
  using mask_type = typename Base::mask_type;

  static constexpr guint filter =
    Base::filter                                            |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK;

  using Base::Base;

  template <class Derived>
  using State = typename Base::template State<Derived>;

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    gint mask_offset = (rect->y - roi->y) * this->mask_stride +
                       (rect->x - roi->x);

    state->comp_mask_data = (mask_type *) &this->mask_data[mask_offset] -
                            this->mask_stride;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    state->comp_mask_data += this->mask_stride;
  }
};

struct DispatchPaintMaskToCompMask
{
  static constexpr guint mask =
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

  template <class Base>
  using AlgorithmDirect   = PaintMaskToCompMask<Base, TRUE>;
  template <class Base>
  using AlgorithmIndirect = PaintMaskToCompMask<Base, FALSE>;

  using DispatchDirect    = BasicDispatch<
    AlgorithmDirect,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK,
    decltype (dispatch_comp_mask)
  >;
  using DispatchIndirect  = BasicDispatch<
    AlgorithmIndirect,
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK,
    decltype (dispatch_temp_comp_mask)
  >;

  template <class Algorithm,
            class MaskType                 = typename Algorithm::mask_type,
            gboolean HasMaskBufferIterator = has_mask_buffer_iterator (
                                               (Algorithm *) NULL)>
  struct Dispatch : DispatchIndirect
  {
  };

  template <class Algorithm>
  struct Dispatch<Algorithm, gfloat, FALSE> : DispatchDirect
  {
  };

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

            if (params->paint_opacity == GIMP_OPACITY_OPAQUE)
              Dispatch<NewAlgorithm> () (visitor, params, algorithms, algorithm);
            else
              DispatchIndirect       () (visitor, params, algorithms, algorithm);
          },
          params, algorithms, algorithm,
          dispatch_paint_mask,
          dispatch_mask_buffer_iterator);
      }
    else
      {
        visitor (algorithm);
      }
  }
} static dispatch_paint_mask_to_comp_mask;


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

  static constexpr gint max_n_iterators = Base::max_n_iterators + 2;

  const Babl             *iterator_format;
  GimpOperationLayerMode *layer_mode = NULL;

  explicit
  DoLayerBlend (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    layer_mode = GIMP_OPERATION_LAYER_MODE (gimp_layer_mode_get_operation (params->paint_mode));
    layer_mode->opacity = params->image_opacity;

    iterator_format = gimp_layer_mode_get_format (params->paint_mode,
                                                  layer_mode->blend_space,
                                                  layer_mode->composite_space,
                                                  layer_mode->composite_mode,
                                                  gimp_temp_buf_get_format (params->paint_buf));

    g_return_if_fail (gimp_temp_buf_get_format (params->paint_buf) == iterator_format);
  }

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gint           iterator_base;

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
    state->iterator_base = gegl_buffer_iterator_add (iter, params->src_buffer,
                                                     area, 0, iterator_format,
                                                     GEGL_ACCESS_READ,
                                                     GEGL_ABYSS_NONE);

    if (! has_comp_buffer ((const Derived *) this))
      {
        gegl_buffer_iterator_add (iter, params->dest_buffer, area, 0,
                                  iterator_format,
                                  GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
      }

    /* initialize the base class *after* initializing the iterator, to make
     * sure that src_buffer is the primary buffer of the iterator, if no
     * subclass added an iterator first.
     */
    Base::init (params, state, iter, roi, area);
  }

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->in_pixel = (gfloat *) iter->items[state->iterator_base + 0].data;

    state->paint_pixel = this->paint_data                        +
                         (rect->y - roi->y) * this->paint_stride +
                         (rect->x - roi->x) * 4;

    if (! has_comp_mask (this) && has_mask_buffer_iterator (this))
      {
        state->mask_pixel =
          (gfloat *) iter->items[mask_buffer_iterator (this, state)].data;
      }

    if (! has_comp_buffer ((const Derived *) this))
      state->out_pixel = (gfloat *) iter->items[state->iterator_base + 1].data;

    state->process_roi.x      = rect->x;
    state->process_roi.width  = rect->width;
    state->process_roi.height = 1;
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gfloat *mask_pixel;
    gfloat *out_pixel;

    if (has_comp_mask (this))
      mask_pixel = comp_mask_data (this, state);
    else if (has_mask_buffer_iterator (this))
      mask_pixel = state->mask_pixel;
    else
      mask_pixel = NULL;

    if (! has_comp_buffer ((const Derived *) this))
      {
        out_pixel = state->out_pixel;
      }
    else
      {
        out_pixel = comp_buffer_data (
          (const Derived *) this,
          (typename Derived::template State<Derived> *) state);
      }

    state->process_roi.y = y;

    layer_mode->function ((GeglOperation*) layer_mode,
                          state->in_pixel,
                          state->paint_pixel,
                          mask_pixel,
                          out_pixel,
                          rect->width,
                          &state->process_roi,
                          0);

    state->in_pixel     += rect->width * 4;
    state->paint_pixel  += this->paint_stride;
    if (! has_comp_mask (this) && has_mask_buffer_iterator (this))
      state->mask_pixel += rect->width;
    if (! has_comp_buffer ((const Derived *) this))
      state->out_pixel  += rect->width * 4;
  }
};

static MandatoryAlgorithmDispatch<
  DoLayerBlend,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND,
  decltype (dispatch_paint_buf),
  decltype (dispatch_mask_buffer_iterator)
>
dispatch_do_layer_blend;


/* MaskComponents, dispatch_mask_components():
 *
 * An algorithm class, implementing the MASK_COMPONENTS algorithm.
 */

template <class Base>
struct MaskComponents : Base
{
  static constexpr guint filter =
    Base::filter |
    GIMP_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS;

  static constexpr gint max_n_iterators = Base::max_n_iterators + 1;

  const Babl *format;
  const Babl *comp_fish = NULL;

  explicit
  MaskComponents (const GimpPaintCoreLoopsParams *params) :
    Base (params)
  {
    format = gimp_operation_mask_components_get_format (
      gegl_buffer_get_format (params->dest_buffer));

    if (format != this->iterator_format)
      comp_fish = babl_fish (this->iterator_format, format);
  }

  template <class Derived>
  struct State : Base::template State<Derived>
  {
    gint    dest_buffer_iterator;

    guint8 *dest_pixel;
    guint8 *comp_pixel;
  };

  template <class Derived>
  void
  init (const GimpPaintCoreLoopsParams *params,
        State<Derived>                 *state,
        GeglBufferIterator             *iter,
        const GeglRectangle            *roi,
        const GeglRectangle            *area) const
  {
    state->dest_buffer_iterator = gegl_buffer_iterator_add (
      iter, params->dest_buffer, area, 0, format,
      GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

    /* initialize the base class *after* initializing the iterator, to make
     * sure that dest_buffer is the primary buffer of the iterator, if no
     * subclass added an iterator first.
     */
    Base::init (params, state, iter, roi, area);
  }

  template <class Derived>
  void
  init_step (const GimpPaintCoreLoopsParams *params,
             State<Derived>                 *state,
             GeglBufferIterator             *iter,
             const GeglRectangle            *roi,
             const GeglRectangle            *area,
             const GeglRectangle            *rect) const
  {
    Base::init_step (params, state, iter, roi, area, rect);

    state->dest_pixel =
      (guint8 *) iter->items[state->dest_buffer_iterator].data;

    if (comp_fish)
      {
        state->comp_pixel = (guint8 *) gegl_scratch_alloc (
          rect->width * babl_format_get_bytes_per_pixel (format));
      }
  }

  template <class Derived>
  void
  process_row (const GimpPaintCoreLoopsParams *params,
               State<Derived>                 *state,
               GeglBufferIterator             *iter,
               const GeglRectangle            *roi,
               const GeglRectangle            *area,
               const GeglRectangle            *rect,
               gint                            y) const
  {
    Base::process_row (params, state, iter, roi, area, rect, y);

    gpointer comp_pixel;

    if (comp_fish)
      {
        babl_process (comp_fish,
                      state->comp_buffer_data, state->comp_pixel,
                      rect->width);

        comp_pixel = state->comp_pixel;
      }
    else
      {
        comp_pixel = state->comp_buffer_data;
      }

    gimp_operation_mask_components_process (format,
                                            state->dest_pixel, comp_pixel,
                                            state->dest_pixel,
                                            rect->width, params->affect);

    state->dest_pixel += rect->width * babl_format_get_bytes_per_pixel (format);
  }

  template <class Derived>
  void
  finalize_step (const GimpPaintCoreLoopsParams *params,
                 State<Derived>                 *state) const
  {
    if (comp_fish)
      gegl_scratch_free (state->comp_pixel);

    Base::finalize_step (params, state);
  }
};

static AlgorithmDispatch<
  MaskComponents,
  GIMP_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS,
  decltype (dispatch_temp_comp_buffer)
>
dispatch_mask_components;


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

      gegl_parallel_distribute_area (
        &roi, PIXELS_PER_THREAD,
        [=] (const GeglRectangle *area)
        {
          State state;
          gint  y;

          if (Algorithm::max_n_iterators > 0)
            {
              GeglBufferIterator *iter;

              iter = gegl_buffer_iterator_empty_new (
                Algorithm::max_n_iterators);

              algorithm.init (params, &state, iter, &roi, area);

              while (gegl_buffer_iterator_next (iter))
                {
                  const GeglRectangle *rect = &iter->items[0].roi;

                  algorithm.init_step (params, &state, iter, &roi, area, rect);

                  for (y = 0; y < rect->height; y++)
                    {
                      algorithm.process_row (params, &state,
                                             iter, &roi, area, rect,
                                             rect->y + y);
                    }

                  algorithm.finalize_step (params, &state);
                }

              algorithm.finalize (params, &state);
            }
          else
            {
              algorithm.init      (params, &state, NULL, &roi, area);
              algorithm.init_step (params, &state, NULL, &roi, area, area);

              for (y = 0; y < area->height; y++)
                {
                  algorithm.process_row (params, &state,
                                         NULL, &roi, area, area,
                                         area->y + y);
                }

              algorithm.finalize_step (params, &state);
              algorithm.finalize      (params, &state);
            }
        });
    },
    params, algorithms, identity<AlgorithmBase> (),
    dispatch_combine_paint_mask_to_canvas_buffer_to_paint_buf_alpha,
    dispatch_combine_paint_mask_to_canvas_buffer,
    dispatch_canvas_buffer_to_paint_buf_alpha,
    dispatch_paint_mask_to_paint_buf_alpha,
    dispatch_canvas_buffer_to_comp_mask,
    dispatch_paint_mask_to_comp_mask,
    dispatch_do_layer_blend,
    dispatch_mask_components);
}
