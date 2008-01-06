/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 Calvin Williamson
 *           2005, 2006 Øyvind Kolås
 */

#ifndef __GEGL_OPERATION_H__
#define __GEGL_OPERATION_H__

#include <glib-object.h>
#include <babl/babl.h>
#include "gegl-types.h"
#include "buffer/gegl-buffer-types.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION            (gegl_operation_get_type ())
#define GEGL_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION, GeglOperation))
#define GEGL_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION, GeglOperationClass))
#define GEGL_IS_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION))
#define GEGL_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION))
#define GEGL_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION, GeglOperationClass))

#define MAX_PADS        16
#define MAX_INPUT_PADS  MAX_PADS
#define MAX_OUTPUT_PADS MAX_PADS

typedef struct _GeglOperationClass GeglOperationClass;

struct _GeglOperation
{
  GObject parent_instance;

  /*< private >*/
  GeglNode *node;  /* the node that this operation object is communicated
                      with through */
};

struct _GeglOperationClass
{
  GObjectClass  parent_class;

  const gchar      *name;        /* name used to refer to this type of
                                    operation in GEGL */
  gchar            *description; /* textual description of the operation */
  char             *categories;  /* a colon seperated list of categories */


  gboolean          no_cache;    /* do not create a cache for this operation */

  /* attach this operation with a GeglNode, override this if you are creating a
   * GeglGraph, it is already defined for Filters/Sources/Composers.
   */
  void            (*attach)               (GeglOperation *operation);

  /* called as a refresh before any of the region needs getters, used in
   * the area base class for instance.
   */
  void            (*tickle)               (GeglOperation *operation);

  /* prepare the node for processing (all properties will be set) override this
   * if you are creating a meta operation (using the node as a GeglGraph).
   */
  void            (*prepare)              (GeglOperation *operation);

  /* Returns a bounding rectangle for the data that is defined by this op. (is
   * already implemented in GeglOperationPointFilter and
   * GeglOperationPointComposer, GeglOperationAreaFilter base classes.
   */
  GeglRectangle   (*get_defined_region)   (GeglOperation *operation);

  /* Computes the region in output (same affected rect assumed for all outputs)
   * when a given region has changed on an input. Used to aggregate dirt in the
   * graph. A default implementation of this, if not provided should probably
   * be to report that the entire defined region is dirtied.
   */
  GeglRectangle   (*compute_affected_region)  (GeglOperation       *operation,
                                               const gchar         *input_pad,
                                               const GeglRectangle *input_region);

  /* computes the rectangle needed to be correctly computed in a buffer
   * on the named input_pad, for a given result rectangle
   */
  GeglRectangle   (*compute_input_request) (GeglOperation       *operation,
                                            const gchar         *input_pad,
                                            const GeglRectangle *roi);

  /* Adjust result rect, adapts the rectangle used for computing results.
   * (useful for global operations like contrast stretching, as well as
   * file loaders to force caching of the full raster).
   */
  GeglRectangle   (*adjust_result_region)  (GeglOperation       *operation,
                                            const GeglRectangle *roi);

  /* Returns the node providing data for a specific location
   */
  GeglNode*       (*detect)                (GeglOperation       *operation,
                                            gint                 x,
                                            gint                 y);

  gboolean        (*process)               (GeglOperation       *operation,
                                            GeglNodeContext     *context,
                                            const gchar         *output_pad,
                                            const GeglRectangle *result_rect);
};

/* returns|registers the gtype for GeglOperation */
GType           gegl_operation_get_type             (void) G_GNUC_CONST;

/* retrieves the bounding box of a connected input */
GeglRectangle * gegl_operation_source_get_defined_region (GeglOperation *operation,
                                                          const gchar   *pad_name);


/* sets the ROI needed to be computed on one of the sources */
void            gegl_operation_set_source_region    (GeglOperation       *operation,
                                                     gpointer             context_id,
                                                     const gchar         *pad_name,
                                                     const GeglRectangle *region);


/* virtual method invokers that depends only on the set properties of a
 * operation|node
 */

/* retrieves the node providing data to a named input pad */
GeglNode      * gegl_operation_get_source_node      (GeglOperation *operation,
                                                     const gchar   *pad_name);
GeglRectangle   gegl_operation_compute_affected_region (GeglOperation *operation,
                                                     const gchar   *input_pad,
                                                     const GeglRectangle *input_region);
GeglRectangle   gegl_operation_get_defined_region   (GeglOperation *operation);
GeglRectangle   gegl_operation_adjust_result_region (GeglOperation *operation,
                                                     const GeglRectangle *roi);

GeglRectangle   gegl_operation_compute_input_request(GeglOperation *operation,
                                                     const gchar   *input_pad,
                                                     const GeglRectangle *roi);

GeglNode       *gegl_operation_detect               (GeglOperation *operation,
                                                     gint           x,
                                                     gint           y);


/* virtual method invokers that change behavior based on the roi being computed,
 * needs a context_id being based that is used for storing context data.
 */

void            gegl_operation_attach               (GeglOperation       *operation,
                                                     GeglNode            *node);
void            gegl_operation_prepare              (GeglOperation       *operation);
gboolean        gegl_operation_process              (GeglOperation       *operation,
                                                     GeglNodeContext     *context,
                                                     const gchar         *output_pad,
                                                     const GeglRectangle *result_rect);

gchar        ** gegl_list_operations                (guint *n_operations_p);
GParamSpec   ** gegl_list_properties                (const gchar *operation_type,
                                                     guint       *n_properties_p);

/* set the name of an operation, transforms all occurences of "_" into "-" */
void       gegl_operation_class_set_name            (GeglOperationClass *operation,
                                                     const gchar        *name);

/* create a pad for a specified property for this operation, this method is
 * to be called from the attach method of operations, most operations do not
 * have to care about this since a super class like filter, sink, source or
 * composer already does so.
 */
void       gegl_operation_create_pad                (GeglOperation *operation,
                                                     GParamSpec    *param_spec);

/* specify the bablformat for a pad on this operation (XXX: document when
 * this is legal, at the moment, only used internally in some ops,. but might
 * turn into a global mechanism) */
void       gegl_operation_set_format                (GeglOperation *operation,
                                                     const gchar   *pad_name,
                                                     Babl          *format);

/* Used to look up the gtype when changing the type of operation associated
 * a GeglNode using just a string with the registered name.
 */
GType      gegl_operation_gtype_from_name           (const gchar *name);

gboolean gegl_operation_calc_source_regions  (GeglOperation *operation,
                                              gpointer       context_id);

void     gegl_operation_vector_prop_changed  (GeglVector    *vector,
                                              GeglOperation *operation);

void     gegl_extension_handler_cleanup      (void);

void     gegl_operation_gtype_cleanup        (void);

G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
