/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

/* deriveable type not included in gimp.h. */
#include "gimpresource.h"

/* GimpResource: base class for resources.
 *
 * Known subclasses are Font, Brush, Pattern, Palette, Gradient.
 * FUTURE: ? Dynamics, ToolPreset, ColorProfile, Color, ColorScale
 * (GimpParasite is NOT.)
 *
 * A *resource* is data that GIMP loads at runtime startup,
 * where the data is used by drawing tools.
 * The GimpContext holds a user's current choice of resources.
 * The GIMP core has the *resource* data.
 *
 * A resource has-a identifier.
 * Currently the identifier is a string, sometimes called a name.
 * The identifier is unique among instances(resource datas) loaded into GIMP.
 *
 * A user can change the set of resources installed with GIMP,
 * and edit or create new resources meaning datasets.
 * A user may attempt to install two different resources having the same name.
 * A user may uninstall a resource while there are still references to it in settings.
 *
 * FUTURE: the identifier is world unique, a UUID or an integer incremented by core.
 * Uniqueness is enforced by core.
 *
 * A GimpResource's identifier is serialized as a setting, i.e. a user's choice,
 * A serialization of a GimpResource is *not* the serialization of the
 * resource's underlying data e.g. not the pixels of a brush.
 *
 * The GimpResource identifier is opaque: you should only pass it around.
 * You should not assume it has any human readable meaning (although it does now.)
 * You should not assume that the "id" is a string.
 * The Resource class encapsulates the ID so in the future, the type may change.
 * PDB procedures that pass a string ID for a resource are obsolete.
 *
 * Usually a plugin lets a user choose a resource interactively
 * then sets it into a temporary context to affect subsequent operations.
 *
 * The Gimp architecture for plugins uses remote procedures,
 * and identically named classes on each side of the wire.
 * A GimpBrush class in core is not the same class as GimpBrush in libgimp.
 * a GimpResource on the libgimp side is a proxy.
 * There is no GimpResource class in core.
 *
 * One use of GimpResource and its subclasses
 * is as a held type of GParamSpecObject, used to declare the parameters of a  PDB procedure.
 * A GimpResource is serializable just for the purpose of serializing GimpProcedureConfig,
 * a "settings" i.e. a set of values for the arguments to a GimpProcedure.
 * A GimpResource just holds the id as a way to identify a *resource*
 * in calls to PDB procedures that ultimately access the core instance of the resource.

 * A GimpResource that has been serialized in a GimpConfig refers to a *resource*
 * that might not still exist in core in the set of loaded resources (files.)
 *
 * A GimpResource:
 *  - furnishes its ID as a property
 *  - serializes/deserializes itself (implements GimpConfigInterface)
 *
 * Some subclasses e.g. GimpFont are pure types.
 * That is, inheriting all its properties and methods from GimpResource.
 * Such a pure type exists just to distinguish (by the name of its type)
 * from other subclasses of GimpResource.
 *
 * Some subclasses have methods for getting/setting attributes of a resource.
 * Some subclasses have methods for creating, duplicating, and deleting resources.
 * Most methods are defined in the PDB (in .pdb files.)
 *
 * Internally, you may need to create a proxy object. Use:
 *    brush = g_object_new (GIMP_TYPE_BRUSH, NULL);
 *    g_object_set (GIMP_RESOURCE(brush), "id", "foo name", NULL);
 * This does NOT create the resource's data in core, only a reference to it.
 * When there is no underlying resource of that id (name) in core,
 * the brush is invalid.
 */

enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

/* Private structure definition. */
typedef struct
{
  char *id;
} GimpResourcePrivate;

static void     gimp_resource_finalize           (GObject      *object);

static void     gimp_resource_set_property       (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void     gimp_resource_get_property       (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

/* Implementation of the GimpConfigInterface */
static void     gimp_resource_config_iface_init (GimpConfigInterface *iface);

static gboolean gimp_resource_serialize          (GimpConfig       *config,
                                                  GimpConfigWriter *writer,
                                                  gpointer          data);
static gboolean gimp_resource_deserialize        (GimpConfig       *config,
                                                  GScanner         *scanner,
                                                  gint              nest_level,
                                                  gpointer          data);

/* The class type is both deriveable (has private) AND implements interface. */
G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GimpResource, gimp_resource, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (GimpResource)
                                  G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                         gimp_resource_config_iface_init))

#define parent_class gimp_resource_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

/* Class construction */
static void
gimp_resource_class_init (GimpResourceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_debug("gimp_resource_class_init");

  object_class->finalize     = gimp_resource_finalize;

  object_class->set_property = gimp_resource_set_property;
  object_class->get_property = gimp_resource_get_property;

  props[PROP_ID] =
    g_param_spec_string ("id",
                         "The id",
                         "The id for internal use",
                         "unknown",
                         GIMP_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

/* Instance construction. */
static void
gimp_resource_init (GimpResource *self)
{
  /* Initialize private data to 0 */
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);
  priv->id = NULL;
  g_debug("gimp_resource_init");
}


/* The next comment annotates the class. */

/**
* GimpResource:
*
* Installable data having serializable ID.  Superclass of Font, Brush, etc.
**/


/**
 * gimp_resource_get_id:
 * @self: The resource.
 *
 * Returns an internal key to the store of resources in the core app.
 * The key can be invalid after a user uninstalls or deletes a resource.
 *
 * Returns: (transfer none): the resource's ID.
 *
 * Since: 3.0
 **/
gchar *
gimp_resource_get_id (GimpResource *self)
{
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);
  return self ? priv->id : "";
}


/* Two stage dispose/finalize.
 * We don't need dispose since no objects to unref. id is a string, not a GObject.
 * Some docs say must define both.
 */

static void
gimp_resource_finalize (GObject *self)
{
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (GIMP_RESOURCE(self));

  g_debug ("gimp_resource_finalize");

  /* Free string property. g_clear_pointer is safe if already freed. */
  g_clear_pointer (&priv->id, g_free);

  /* Chain up. */
  G_OBJECT_CLASS (parent_class)->finalize (self);
}


/*
 * Override the GObject methods for set/get property.
 * i.e. override g_object_set_property.
 */
static void
gimp_resource_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GimpResource *self = GIMP_RESOURCE (object);
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);

  g_debug ("gimp_resource_set_property");
  switch (property_id)
    {
    case PROP_ID:
      g_free (priv->id);
      priv->id = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_resource_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpResource *self = GIMP_RESOURCE (object);
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);

  g_debug ("gimp_resource_get_property");
  switch (property_id)
    {
    case PROP_ID:
      g_value_set_string (value, priv->id);
      /* Assert id string was copied into GValue. */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* config iface */

static void
gimp_resource_config_iface_init (GimpConfigInterface *iface)
{
  /* We don't implement the serialize_property methods. */
  iface->deserialize = gimp_resource_deserialize;
  iface->serialize   = gimp_resource_serialize;
}


/* Serialize the whole thing, which is the id.
 *
 * Requires the id is not NULL.
 * When id is NULL, writes nothing and returns FALSE.
 */
static gboolean
gimp_resource_serialize (GimpConfig       *config,
                         GimpConfigWriter *writer,
                         gpointer          data)  /* Unused. */
{
  /* Require config is-a GimpResource instance implementing Config iface. */
  GimpResource *self = GIMP_RESOURCE (config);
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);

  g_debug ("resource serialize");

  if (priv->id != NULL)
    {
      g_debug ("resource serialize: %s", priv->id);
      /* require the caller opened and will close writer.
       * Caller wrote the subclass type name "Gimp<Foo>"
       */
      gimp_config_writer_string (writer, priv->id);
      return TRUE;
    }
  else
    {
      g_debug ("resource serialize failed: NULL id");
      return FALSE;
    }
}


static gboolean
gimp_resource_deserialize (GimpConfig *config,
                           GScanner   *scanner,
                           gint        nest_level,
                           gpointer    data)
{
  gchar *id;
  GimpResource *self = GIMP_RESOURCE (config);
  GimpResourcePrivate *priv = gimp_resource_get_instance_private (self);

  g_debug ("resource deserialize");

  if (! gimp_scanner_parse_string (scanner, &id))
    {
      g_scanner_error (scanner,
                       "Fail scan string for resource");
      return FALSE;
    }
  else
    {
      g_debug ("resource deserialize: %s", id);
      priv->id = id;
    }

  return TRUE;
}
