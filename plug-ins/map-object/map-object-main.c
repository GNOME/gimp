/* MapObject 1.2.0 -- image filter plug-in for GIMP
 *
 * Copyright (C) 1996-98 Tom Bech
 * Copyright (C) 1996-98 Federico Mena Quintero
 *
 * E-mail: tomb@gimp.org (Tom) or quartic@gimp.org (Federico)
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "map-object-ui.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"

#include "libgimp/stdplugins-intl.h"


MapObjectValues mapvals;


typedef struct _Map      Map;
typedef struct _MapClass MapClass;

struct _Map
{
  GimpPlugIn parent_instance;
};

struct _MapClass
{
  GimpPlugInClass parent_class;
};


#define MAP_TYPE  (map_get_type ())
#define MAP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAP_TYPE, Map))

GType                   map_get_type         (void) G_GNUC_CONST;

static GList          * map_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * map_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * map_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable        **drawables,
                                              GimpProcedureConfig  *config,
                                              gpointer              run_data);

static void             set_default_settings (void);
static void             check_drawables      (GimpDrawable         *drawable);


G_DEFINE_TYPE (Map, map, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MAP_TYPE)
DEFINE_STD_SET_I18N


static void
map_class_init (MapClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = map_query_procedures;
  plug_in_class->create_procedure = map_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
map_init (Map *map)
{
}

static GList *
map_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
map_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      GeglColor *default_color;

      gegl_init (NULL, NULL);

      default_color = gegl_color_new ("white");

      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            map_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("Map _Object..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        _("Map the image to an object "
                                          "(plane, sphere, box or cylinder)"),
                                        "No help yet",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 1.2.0, July 16 1998");

      gimp_procedure_add_choice_argument (procedure, "map-type",
                                          _("Map _to"),
                                          _("Type of mapping"),
                                          gimp_choice_new_with_values ("map-plane",    MAP_PLANE,    _("Plane"),    NULL,
                                                                       "map-sphere",   MAP_SPHERE,   _("Sphere"),   NULL,
                                                                       "map-box",      MAP_BOX,      _("Box"),      NULL,
                                                                       "map-cylinder", MAP_CYLINDER, _("Cylinder"), NULL,
                                                                       NULL),
                                          "map-plane",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "viewpoint-x",
                                          _("X"),
                                          _("Position of viewpoint (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "viewpoint-y",
                                          _("Y"),
                                          _("Position of viewpoint (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "viewpoint-z",
                                          _("Z"),
                                          _("Position of viewpoint (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "position-x",
                                          _("Position X"),
                                          _("Object position (x,y,z)"),
                                          -1.0, 2.0, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "position-y",
                                          _("Position Y"),
                                          _("Object position (x,y,z)"),
                                          -1.0, 2.0, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "position-z",
                                          _("Position Z"),
                                          _("Object position (x,y,z)"),
                                          -1.0, 2.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "first-axis-x",
                                          _("X"),
                                          _("First axis of object (x,y,z)"),
                                          -1.0, 2.0, 1.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "first-axis-y",
                                          _("y"),
                                          _("First axis of object (x,y,z)"),
                                          -1.0, 2.0, 0.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "first-axis-z",
                                          _("Z"),
                                          _("First axis of object (x,y,z)"),
                                          -1.0, 2.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "second-axis-x",
                                          _("X"),
                                          _("Second axis of object (x,y,z)"),
                                          -1.0, 2.0, 0.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "second-axis-y",
                                          _("Y"),
                                          _("Second axis of object (x,y,z)"),
                                          -1.0, 2.0, 1.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "second-axis-z",
                                          _("Z"),
                                          _("Second axis of object (x,y,z)"),
                                          -1.0, 2.0, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "rotation-angle-x",
                                          _("Angle X"),
                                          _("Rotation about X axis in degrees"),
                                          -360, 360, 0.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "rotation-angle-y",
                                          _("Angle Y"),
                                          _("Rotation about Y axis in degrees"),
                                          -360, 360, 0.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "rotation-angle-z",
                                          _("Angle Z"),
                                          _("Rotation about Z axis in degrees"),
                                          -360, 360, 0.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "light-type",
                                          _("Light source type"),
                                          _("Type of lightsource"),
                                          gimp_choice_new_with_values ("point-light",       POINT_LIGHT,       _("Point Light"),       NULL,
                                                                       "directional-light", DIRECTIONAL_LIGHT, _("Directional Light"), NULL,
                                                                       "no-light",          NO_LIGHT,          _("No Light"),          NULL,
                                                                       NULL),
                                          "point-light",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "light-color",
                                         _("Light source _color"),
                                         _("Light source color"),
                                         TRUE, default_color,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-position-x",
                                          _("Light position X"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "light-position-y",
                                          _("Light position Y"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "light-position-z",
                                          _("Light position Z"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-direction-x",
                                          _("Light direction X"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "light-direction-y",
                                          _("Light direction Y"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "light-direction-z",
                                          _("Light direction Z"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "ambient-intensity",
                                          _("Ambie_nt"),
                                         _("Material ambient intensity"),
                                          0, 1, 0.3,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "diffuse-intensity",
                                          _("D_iffuse"),
                                          _("Material diffuse intensity"),
                                          0, 1, 1.0,
                                          G_PARAM_READWRITE);

      /* Reflectivity */
      gimp_procedure_add_double_argument (procedure, "diffuse-reflectivity",
                                          _("Di_ffuse"),
                                          _("Material diffuse reflectivity"),
                                          0, 1, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "specular-reflectivity",
                                          _("Spec_ular"),
                                          _("Material specular reflectivity"),
                                          0, 1, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "highlight",
                                          _("Highligh_t"),
                                          _("Material highlight "
                                            "(note, it's exponential)"),
                                          0, G_MAXDOUBLE, 27.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "antialiasing",
                                           _("Antialia_sing"),
                                           _("Apply antialiasing"),
                                           TRUE,
                                           G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "depth",
                                          _("_Depth"),
                                          _("Antialiasing quality. Higher is better, "
                                            "but slower"),
                                          1.0, 5.0, 3.0,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "threshold",
                                          _("Thr_eshold"),
                                          _("Stop when pixel differences are smaller than "
                                            "this value"),
                                          0.001, 1000.0, 0.250,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "tiled",
                                           _("_Tile source image"),
                                           _("Tile source image"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "new-image",
                                           _("Create _new image"),
                                           _("Create a new image"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "new-layer",
                                           _("Create ne_w layer"),
                                           _("Create a new layer when applying filter"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "transparent-background",
                                           _("Transparent bac_kground"),
                                           _("Make background transparent"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      /* Sphere Options */
      gimp_procedure_add_double_argument (procedure, "sphere-radius",
                                          _("Radi_us"),
                                          _("Sphere radius"),
                                          0, G_MAXDOUBLE, 0.25,
                                          G_PARAM_READWRITE);

      /* Box Options */
      gimp_procedure_add_drawable_argument (procedure, "box-front-drawable",
                                            _("Fro_nt"),
                                            _("Box front face "
                                              "(set this to NULL if not used)"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "box-back-drawable",
                                            _("B_ack"),
                                            _("Box back face"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "box-top-drawable",
                                            _("To_p"),
                                            _("Box top face"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "box-bottom-drawable",
                                            _("Bo_ttom"),
                                            _("Box bottom face"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "box-left-drawable",
                                            _("Le_ft"),
                                            _("Box left face"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "box-right-drawable",
                                            _("Ri_ght"),
                                            _("Box right face"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "x-scale",
                                          _("Scale X"),
                                          _("Box X size"),
                                          0, G_MAXDOUBLE, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "y-scale",
                                          _("Scale Y"),
                                          _("Box Y size"),
                                          0, G_MAXDOUBLE, 0.5,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "z-scale",
                                          _("Scale Z"),
                                          _("Box Z size"),
                                          0, G_MAXDOUBLE, 0.5,
                                          G_PARAM_READWRITE);

      /* Cylinder options */
      gimp_procedure_add_drawable_argument (procedure, "cyl-top-drawable",
                                            _("_Top"),
                                            _("Cylinder top face "
                                              "(set this to NULL if not used)"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_drawable_argument (procedure, "cyl-bottom-drawable",
                                            _("_Bottom"),
                                            _("Cylinder bottom face "
                                              "(set this to NULL if not used)"),
                                            TRUE,
                                            G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "cylinder-radius",
                                          _("Radi_us"),
                                          _("Cylinder radius"),
                                          0, G_MAXDOUBLE, 0.25,
                                          G_PARAM_READWRITE);
      gimp_procedure_add_double_argument (procedure, "cylinder-length",
                                          _("Cylin_der length"),
                                          _("Cylinder length"),
                                          0, G_MAXDOUBLE, 0.25,
                                          G_PARAM_READWRITE);

      g_object_unref (default_color);
    }

  return procedure;
}

static void
set_default_settings (void)
{
  gint i;

  gimp_vector3_set (&mapvals.viewpoint,  0.5, 0.5, 2.0);
  gimp_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
  gimp_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
  gimp_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);
  gimp_vector3_set (&mapvals.position,   0.5, 0.5, 0.0);
  gimp_vector3_set (&mapvals.lightsource.position,  -0.5, -0.5, 2.0);
  gimp_vector3_set (&mapvals.lightsource.direction, -1.0, -1.0, 1.0);
  gimp_vector3_set (&mapvals.scale,      0.5, 0.5, 0.5);

  mapvals.maptype = MAP_PLANE;

  mapvals.pixelthreshold  = 0.25;
  mapvals.alpha           = 0.0;
  mapvals.beta            = 0.0;
  mapvals.gamma           = 0.0;
  mapvals.maxdepth        = 3.0;
  mapvals.radius          = 0.25;
  mapvals.cylinder_radius = 0.25;
  mapvals.cylinder_length = 1.0;

  mapvals.zoom             = 1.0;
  mapvals.lightsource.type = POINT_LIGHT;

  mapvals.antialiasing           = TRUE;
  mapvals.create_new_image       = FALSE;
  mapvals.create_new_layer       = FALSE;
  mapvals.transparent_background = FALSE;
  mapvals.tiled                  = FALSE;
  mapvals.livepreview            = FALSE;
  mapvals.showgrid               = TRUE;

  mapvals.lightsource.intensity = 1.0;
  for (i = 0; i < 4; i++)
    mapvals.lightsource.color[i] = 1.0;

  mapvals.material.ambient_int  = 0.3;
  mapvals.material.diffuse_int  = 1.0;
  mapvals.material.diffuse_ref  = 0.5;
  mapvals.material.specular_ref = 0.5;
  mapvals.material.highlight    = 27.0;

  for (i = 0; i < 6; i++)
    mapvals.boxmap_id[i] = -1;

  for (i = 0; i < 2; i++)
    mapvals.cylindermap_id[i] = -1;
}

static void
check_drawables (GimpDrawable *drawable)
{
  GimpDrawable *map;
  gint          i;

  /* Check that boxmap images are valid */
  /* ================================== */

  for (i = 0; i < 6; i++)
    {
      map = gimp_drawable_get_by_id (mapvals.boxmap_id[i]);

      if (! map || gimp_drawable_is_gray (map))
        mapvals.boxmap_id[i] = gimp_item_get_id (GIMP_ITEM (drawable));
    }

  /* Check that cylindermap images are valid */
  /* ======================================= */

  for (i = 0; i < 2; i++)
    {
     map = gimp_drawable_get_by_id (mapvals.cylindermap_id[i]);

     if (! map || gimp_drawable_is_gray (map))
        mapvals.cylindermap_id[i] = gimp_item_get_id (GIMP_ITEM (drawable));
    }
}

static GimpValueArray *
map_run (GimpProcedure        *procedure,
         GimpRunMode           run_mode,
         GimpImage            *_image,
         GimpDrawable        **drawables,
         GimpProcedureConfig  *config,
         gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  image = _image;

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  set_default_settings ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      check_drawables (drawable);

      if (! main_dialog (procedure, config, drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }

      copy_from_config (config);
      compute_image ();
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      check_drawables (drawable);

      if (! image_setup (drawable, FALSE, config))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_SUCCESS,
                                                   NULL);
        }

      copy_from_config (config);
      compute_image ();
      break;

    case GIMP_RUN_NONINTERACTIVE:
      check_drawables (drawable);

      if (! image_setup (drawable, FALSE, config))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_SUCCESS,
                                                   NULL);
        }

      copy_from_config (config);
      compute_image ();
      break;
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
