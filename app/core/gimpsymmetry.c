/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-nodes.h"

#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-symmetry.h"
#include "gimpitem.h"
#include "gimpsymmetry.h"

#include "gimp-intl.h"


enum
{
  STROKES_UPDATED,
  GUI_PARAM_CHANGED,
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_ACTIVE,
  PROP_VERSION,
};


/* Local function prototypes */

static void       gimp_symmetry_finalize            (GObject      *object);
static void       gimp_symmetry_set_property        (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void       gimp_symmetry_get_property        (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void       gimp_symmetry_real_update_strokes (GimpSymmetry *sym,
                                                     GimpDrawable *drawable,
                                                     GimpCoords   *origin);
static void       gimp_symmetry_real_get_transform  (GimpSymmetry *sym,
                                                     gint          stroke,
                                                     gdouble      *angle,
                                                     gboolean     *reflect);
static gboolean   gimp_symmetry_real_update_version (GimpSymmetry *sym);


G_DEFINE_TYPE_WITH_CODE (GimpSymmetry, gimp_symmetry, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_symmetry_parent_class

static guint gimp_symmetry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_symmetry_class_init (GimpSymmetryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* This signal should likely be emitted at the end of
   * update_strokes() if stroke coordinates were changed.
   */
  gimp_symmetry_signals[STROKES_UPDATED] =
    g_signal_new ("strokes-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, GIMP_TYPE_IMAGE);

  /* This signal should be emitted when you request a change in the
   * settings UI. For instance adding some settings (therefore having
   * a dynamic UI), or changing scale min/max extremes, etc.
   */
  gimp_symmetry_signals[GUI_PARAM_CHANGED] =
    g_signal_new ("gui-param-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, GIMP_TYPE_IMAGE);

  gimp_symmetry_signals[ACTIVE_CHANGED] =
    g_signal_new ("active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpSymmetryClass, active_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize     = gimp_symmetry_finalize;
  object_class->set_property = gimp_symmetry_set_property;
  object_class->get_property = gimp_symmetry_get_property;

  klass->label               = _("None");
  klass->update_strokes      = gimp_symmetry_real_update_strokes;
  klass->get_transform       = gimp_symmetry_real_get_transform;
  klass->active_changed      = NULL;
  klass->update_version      = gimp_symmetry_real_update_version;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        GIMP_TYPE_IMAGE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ACTIVE,
                            "active",
                            _("Active"),
                            _("Activate symmetry painting"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_VERSION,
                        "version",
                        "Symmetry version",
                        "Version of the symmetry object",
                        -1, G_MAXINT, 0,
                        GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_symmetry_init (GimpSymmetry *sym)
{
}

static void
gimp_symmetry_finalize (GObject *object)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (object);

  gimp_symmetry_clear_origin (sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_symmetry_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      sym->image = g_value_get_object (value);
      break;
    case PROP_ACTIVE:
      sym->active = g_value_get_boolean (value);
      g_signal_emit (sym, gimp_symmetry_signals[ACTIVE_CHANGED], 0,
                     sym->active);
      break;
    case PROP_VERSION:
      sym->version = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_symmetry_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, sym->image);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, sym->active);
      break;
    case PROP_VERSION:
      g_value_set_int (value, sym->version);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_symmetry_real_update_strokes (GimpSymmetry *sym,
                                   GimpDrawable *drawable,
                                   GimpCoords   *origin)
{
  /* The basic symmetry just uses the origin as is. */
  sym->strokes = g_list_prepend (sym->strokes,
                                 g_memdup (origin, sizeof (GimpCoords)));
}

static void
gimp_symmetry_real_get_transform (GimpSymmetry *sym,
                                  gint          stroke,
                                  gdouble      *angle,
                                  gboolean     *reflect)
{
  /* The basic symmetry does nothing, since no transformation of the
   * brush painting happen. */
}

static gboolean
gimp_symmetry_real_update_version (GimpSymmetry *symmetry)
{
  /* Currently all symmetries are at version 0. So all this check has to
   * do is verify that we are at version 0.
   * If one of the child symmetry bumps its version, it will have to
   * override the update_version() virtual function and do any necessary
   * update there (for instance new properties, modified properties, or
   * whatnot).
   */
  gint version;

  g_object_get (symmetry,
                "version", &version,
                NULL);

  return (version == 0);
}

/***** Public Functions *****/

/**
 * gimp_symmetry_set_stateful:
 * @sym:      the #GimpSymmetry
 * @stateful: whether the symmetry should be stateful or stateless.
 *
 * By default, symmetry is made stateless, which means in particular
 * that the size of points can change from one stroke to the next, and
 * in particular you cannot map the coordinates from a stroke to the
 * next. I.e. stroke N at time T+1 is not necessarily the continuation
 * of stroke N at time T.
 * To obtain corresponding strokes, stateful tools, such as MyPaint
 * brushes or the ink tool, need to run this function. They should reset
 * to stateless behavior once finished painting.
 *
 * One of the first consequence of being stateful is that the number of
 * strokes cannot be changed, so more strokes than possible on canvas
 * may be computed, and oppositely it will be possible to end up in
 * cases with missing strokes (e.g. a tiling, theoretically infinite,
 * won't be for the ink tool if one draws too far out of canvas).
 **/
void
gimp_symmetry_set_stateful (GimpSymmetry *symmetry,
                            gboolean      stateful)
{
  symmetry->stateful = stateful;
}

/**
 * gimp_symmetry_set_origin:
 * @sym:      the #GimpSymmetry
 * @drawable: the #GimpDrawable where painting will happen
 * @origin:   new base coordinates.
 *
 * Set the symmetry to new origin coordinates and drawable.
 **/
void
gimp_symmetry_set_origin (GimpSymmetry *sym,
                          GimpDrawable *drawable,
                          GimpCoords   *origin)
{
  g_return_if_fail (GIMP_IS_SYMMETRY (sym));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_get_image (GIMP_ITEM (drawable)) == sym->image);

  if (drawable != sym->drawable)
    {
      if (sym->drawable)
        g_object_unref (sym->drawable);
      sym->drawable = g_object_ref (drawable);
    }

  if (origin != sym->origin)
    {
      g_free (sym->origin);
      sym->origin = g_memdup (origin, sizeof (GimpCoords));
    }

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;

  GIMP_SYMMETRY_GET_CLASS (sym)->update_strokes (sym, drawable, origin);
}

/**
 * gimp_symmetry_clear_origin:
 * @sym: the #GimpSymmetry
 *
 * Clear the symmetry's origin coordinates and drawable.
 **/
void
gimp_symmetry_clear_origin (GimpSymmetry *sym)
{
  g_return_if_fail (GIMP_IS_SYMMETRY (sym));

  g_clear_object (&sym->drawable);

  g_clear_pointer (&sym->origin, g_free);

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;
}

/**
 * gimp_symmetry_get_origin:
 * @sym: the #GimpSymmetry
 *
 * Returns: the origin stroke coordinates.
 * The returned value is owned by the #GimpSymmetry and must not be freed.
 **/
GimpCoords *
gimp_symmetry_get_origin (GimpSymmetry *sym)
{
  g_return_val_if_fail (GIMP_IS_SYMMETRY (sym), NULL);

  return sym->origin;
}

/**
 * gimp_symmetry_get_size:
 * @sym: the #GimpSymmetry
 *
 * Returns: the total number of strokes.
 **/
gint
gimp_symmetry_get_size (GimpSymmetry *sym)
{
  g_return_val_if_fail (GIMP_IS_SYMMETRY (sym), 0);

  return g_list_length (sym->strokes);
}

/**
 * gimp_symmetry_get_coords:
 * @sym:    the #GimpSymmetry
 * @stroke: the stroke number
 *
 * Returns: the coordinates of the stroke number @stroke.
 * The returned value is owned by the #GimpSymmetry and must not be freed.
 **/
GimpCoords *
gimp_symmetry_get_coords (GimpSymmetry *sym,
                          gint          stroke)
{
  g_return_val_if_fail (GIMP_IS_SYMMETRY (sym), NULL);

  return g_list_nth_data (sym->strokes, stroke);
}

/**
 * gimp_symmetry_get_transform:
 * @sym:     the #GimpSymmetry
 * @stroke:  the stroke number
 * @angle:   output pointer to the transformation rotation angle,
 *           in degrees (ccw)
 * @reflect: output pointer to the transformation reflection flag
 *
 * Returns: the transformation to apply to the paint content for stroke
 * number @stroke.  The transformation is comprised of rotation, possibly
 * followed by horizontal reflection, around the stroke coordinates.
 **/
void
gimp_symmetry_get_transform (GimpSymmetry *sym,
                             gint          stroke,
                             gdouble      *angle,
                             gboolean     *reflect)
{
  g_return_if_fail (GIMP_IS_SYMMETRY (sym));
  g_return_if_fail (angle != NULL);
  g_return_if_fail (reflect != NULL);

  *angle   = 0.0;
  *reflect = FALSE;

  GIMP_SYMMETRY_GET_CLASS (sym)->get_transform (sym,
                                                stroke,
                                                angle,
                                                reflect);
}

/**
 * gimp_symmetry_get_matrix:
 * @sym:     the #GimpSymmetry
 * @stroke:  the stroke number
 * @matrix:  output pointer to the transformation matrix
 *
 * Returns: the transformation matrix to apply to the paint content for stroke
 * number @stroke.
 **/
void
gimp_symmetry_get_matrix (GimpSymmetry *sym,
                          gint          stroke,
                          GimpMatrix3  *matrix)
{
  gdouble  angle;
  gboolean reflect;

  g_return_if_fail (GIMP_IS_SYMMETRY (sym));
  g_return_if_fail (matrix != NULL);

  gimp_symmetry_get_transform (sym, stroke, &angle, &reflect);

  gimp_matrix3_identity (matrix);
  gimp_matrix3_rotate (matrix, -gimp_deg_to_rad (angle));
  if (reflect)
    gimp_matrix3_scale (matrix, -1.0, 1.0);
}

/**
 * gimp_symmetry_get_operation:
 * @sym:          the #GimpSymmetry
 * @stroke:       the stroke number
 *
 * Returns: the transformation operation to apply to the paint content for
 * stroke number @stroke, or NULL for the identity transformation.
 *
 * The returned #GeglNode should be freed by the caller.
 **/
GeglNode *
gimp_symmetry_get_operation (GimpSymmetry *sym,
                             gint          stroke)
{
  GimpMatrix3 matrix;

  g_return_val_if_fail (GIMP_IS_SYMMETRY (sym), NULL);

  gimp_symmetry_get_matrix (sym, stroke, &matrix);

  if (gimp_matrix3_is_identity (&matrix))
    return NULL;

  return gimp_gegl_create_transform_node (&matrix);
}

/*
 * gimp_symmetry_parasite_name:
 * @type: the #GimpSymmetry's #GType
 *
 * Returns: a newly allocated string.
 */
gchar *
gimp_symmetry_parasite_name (GType type)
{
  return g_strconcat ("gimp-image-symmetry:", g_type_name (type), NULL);
}

GimpParasite *
gimp_symmetry_to_parasite (const GimpSymmetry *sym)
{
  GimpParasite *parasite;
  gchar        *parasite_name;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_SYMMETRY (sym), NULL);

  str = gimp_config_serialize_to_string (GIMP_CONFIG (sym), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite_name = gimp_symmetry_parasite_name (G_TYPE_FROM_INSTANCE (sym));
  parasite = gimp_parasite_new (parasite_name,
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (parasite_name);
  g_free (str);

  return parasite;
}

GimpSymmetry *
gimp_symmetry_from_parasite (const GimpParasite *parasite,
                             GimpImage          *image,
                             GType               type)
{
  GimpSymmetry    *symmetry;
  gchar           *parasite_name;
  const gchar     *str;
  GError          *error = NULL;

  parasite_name = gimp_symmetry_parasite_name (type);

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                parasite_name) == 0,
                        NULL);

  str = gimp_parasite_data (parasite);

  if (! str)
    {
      g_warning ("Empty symmetry parasite \"%s\"", parasite_name);

      return NULL;
    }

  symmetry = gimp_image_symmetry_new (image, type);

  g_object_set (symmetry,
                "version", -1,
                NULL);

  if (! gimp_config_deserialize_string (GIMP_CONFIG (symmetry),
                                        str,
                                        gimp_parasite_data_size (parasite),
                                        NULL,
                                        &error))
    {
      g_printerr ("Failed to deserialize symmetry parasite: %s\n"
                  "\t- parasite name: %s\n\t- parasite data: %s\n",
                  error->message, parasite_name, str);
      g_error_free (error);

      g_object_unref (symmetry);
      symmetry = NULL;
    }
  g_free (parasite_name);

  if (symmetry)
    {
      gint version;

      g_object_get (symmetry,
                    "version", &version,
                    NULL);
      if (version == -1)
        {
          /* If version has not been updated, let's assume this parasite was
           * not representing symmetry settings.
           */
          g_object_unref (symmetry);
          symmetry = NULL;
        }
      else if (GIMP_SYMMETRY_GET_CLASS (symmetry)->update_version (symmetry) &&
               ! GIMP_SYMMETRY_GET_CLASS (symmetry)->update_version (symmetry))
        {
          g_object_unref (symmetry);
          symmetry = NULL;
        }
    }

  return symmetry;
}
