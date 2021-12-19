/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpapp.c
 * Copyright (C) 2021 Niels De Graef <nielsdegraef@gmail.com>
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

#include <config.h>

#include "gimpapp.h"

#include "libgimpbase/gimpbase.h"


enum
{
  PROP_0,
  PROP_GIMP,
  N_PROPS
};

struct _GimpApp
{
  GtkApplication parent_instance;

  Gimp          *gimp;

  gboolean       no_splash;
  gboolean       as_new;
  const gchar   *batch_interpreter;
  const gchar  **batch_commands;
};


static void   gimp_app_finalize      (GObject      *object);
static void   gimp_app_set_property  (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);
static void   gimp_app_get_property  (GObject      *object,
                                      guint         property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);


G_DEFINE_TYPE (GimpApp, gimp_app, GTK_TYPE_APPLICATION)

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_app_class_init (GimpAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->get_property = gimp_app_get_property;
  gobj_class->set_property = gimp_app_set_property;
  gobj_class->finalize = gimp_app_finalize;

  props[PROP_GIMP] =
    g_param_spec_object ("gimp", "GIMP", "GIMP root object",
                         GIMP_TYPE_GIMP,
                         GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobj_class, N_PROPS, props);
}

static void
gimp_app_init (GimpApp *self)
{
}

static void
gimp_app_finalize (GObject *object)
{
  GimpApp *self = GIMP_APP (object);

  g_clear_object (&self->gimp);

  G_OBJECT_CLASS (gimp_app_parent_class)->finalize (object);
}

static void
gimp_app_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  GimpApp *self = GIMP_APP (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, self->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_app_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  GimpApp *self = GIMP_APP (object);

  switch (property_id)
    {
    case PROP_GIMP:
      self->gimp = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

GApplication *
gimp_app_new (Gimp        *gimp,
              gboolean     no_splash,
              gboolean     as_new,
              const char  *batch_interpreter,
              const char **batch_commands)
{
  GimpApp *app;

  app = g_object_new (GIMP_TYPE_APP,
                      "gimp", gimp,
                      NULL);

  /* We shouldn't have to pass these externally, so I didn't bother making
   * GObject properties for them. In the end, they should just be parsed by
   * the GApplication code */
  app->no_splash = no_splash;
  app->as_new = as_new;
  app->batch_interpreter = batch_interpreter;
  app->batch_commands = batch_commands;

  return G_APPLICATION (app);
}

Gimp *
gimp_app_get_gimp (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), NULL);
  return self->gimp;
}

gboolean
gimp_app_get_no_splash (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), FALSE);
  return self->no_splash;
}

gboolean
gimp_app_get_as_new (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), FALSE);
  return self->as_new;
}

const char *
gimp_app_get_batch_interpreter (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), NULL);
  return self->batch_interpreter;
}

const char **
gimp_app_get_batch_commands (GimpApp *self)
{
  g_return_val_if_fail (GIMP_IS_APP (self), NULL);
  return self->batch_commands;
}
