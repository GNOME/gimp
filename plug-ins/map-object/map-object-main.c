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
#define MAP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAP_TYPE, Map))

GType                   map_get_type         (void) G_GNUC_CONST;

static GList          * map_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * map_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * map_run              (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static void             set_default_settings (void);
static void             check_drawables      (GimpDrawable         *drawable);


G_DEFINE_TYPE (Map, map, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MAP_TYPE)


static void
map_class_init (MapClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = map_query_procedures;
  plug_in_class->create_procedure = map_create_procedure;
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
      GimpRGB white = { 1.0, 1.0, 1.0, 1.0 };

      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            map_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");

      gimp_procedure_set_menu_label (procedure, N_("Map _Object..."));
      gimp_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      gimp_procedure_set_documentation (procedure,
                                        N_("Map the image to an object "
                                           "(plane, sphere, box or cylinder)"),
                                        "No help yet",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 1.2.0, July 16 1998");

      GIMP_PROC_ARG_INT (procedure, "map-type",
                         "Map type",
                         "Type of mapping (0=plane, 1=sphere, 2=box, "
                         "3=cylinder)",
                         0, 2, MAP_PLANE,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "viewpoint-x",
                            "Viewpoint X",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "viewpoint-y",
                            "Viewpoint Y",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "viewpoint-z",
                            "Viewpoint Z",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "position-x",
                            "Position X",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "position-y",
                            "Position Y",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "position-z",
                            "Position Z",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "first-axis-x",
                            "First axis X",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "first-axis-y",
                            "First axis y",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "first-axis-z",
                            "First axis Z",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "second-axis-x",
                            "Second axis X",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "second-axis-y",
                            "Second axis Y",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "second-axis-z",
                            "Second axis Z",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "rotation-angle-x",
                            "Rotation angle X",
                            "Rotation about X axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "rotation-angle-y",
                            "Rotation angle Y",
                            "Rotation about Y axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "rotation-angle-z",
                            "Rotation angle Z",
                            "Rotation about Z axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "light-type",
                         "Light type",
                         "Type of lightsource (0=point, 1=directional, 2=none)",
                         0, 2, POINT_LIGHT,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_RGB (procedure, "light-color",
                         "Light color",
                         "Light source color",
                         TRUE, &white,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "light-position-x",
                            "Light position X",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "light-position-y",
                            "Light position Y",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "light-position-z",
                            "Light position Z",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "light-direction-x",
                            "Light direction X",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "light-direction-y",
                            "Light direction Y",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "light-direction-z",
                            "Light direction Z",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "ambient-intensity",
                            "Ambient intensity",
                            "Material ambient intensity",
                            0, 1, 0.3,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "diffuse-intensity",
                            "Diffuse intensity",
                            "Material diffuse intensity",
                            0, 1, 1.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "diffuse-reflectivity",
                            "Diffuse reflectivity",
                            "Material diffuse reflectivity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "specular-reflectivity",
                            "Specular reflectivity",
                            "Material specular reflectivity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "highlight",
                            "Highlight",
                            "Material highlight (note, it's exponential)",
                            0, G_MAXDOUBLE, 27.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "antialiasing",
                             "Antialiasing",
                             "Apply antialiasing",
                             TRUE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "tiled",
                             "Tiled",
                             "Tile source image",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "new-image",
                             "New image",
                             "Create a new image",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_BOOLEAN (procedure, "transparent-background",
                             "Transparent background",
                             "Make background transparent",
                             FALSE,
                             G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "radius",
                            "Radius",
                            "Sphere/cylinder radius (only used when "
                            "maptype=1 or 3)",
                            0, G_MAXDOUBLE, 0.25,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "x-scale",
                            "X scale",
                            "Box X size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "y-scale",
                            "Y scale",
                            "Box Y size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      GIMP_PROC_ARG_DOUBLE (procedure, "z-scale",
                            "Z scale",
                            "Box Z size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DOUBLE (procedure, "cylinder-length",
                            "Cylinder length",
                            "Cylinder length",
                            0, G_MAXDOUBLE, 0.25,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_DRAWABLE (procedure, "box-front-drawable",
                              "Box front drawable",
                              "Box front face (set these to NULL if not used)",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "box-back-drawable",
                              "Box back drawable",
                              "Box back face",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "box-top-drawable",
                              "Box top drawable",
                              "Box top face",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "box-bottom-drawable",
                              "Box bottom drawable",
                              "Box bottom face",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "box-left-drawable",
                              "Box left drawable",
                              "Box left face",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "box-right-drawable",
                              "Box right drawable",
                              "Box right face",
                              TRUE,
                              G_PARAM_READWRITE);

      GIMP_PROC_ARG_DRAWABLE (procedure, "cyl-top-drawable",
                              "Cyl top drawable",
                              "Cylinder top face (set these to NULL if not used)",
                              TRUE,
                              G_PARAM_READWRITE);
      GIMP_PROC_ARG_DRAWABLE (procedure, "cyl-bottom-drawable",
                              "Cyl bottom drawable",
                              "Cylinder bottom face",
                              TRUE,
                              G_PARAM_READWRITE);
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
  gimp_rgba_set (&mapvals.lightsource.color, 1.0, 1.0, 1.0, 1.0);

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
         GimpDrawable         *drawable,
         const GimpValueArray *args,
         gpointer              run_data)
{
  gint i;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  image = _image;

  set_default_settings ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &mapvals);
      check_drawables (drawable);

      if (! main_dialog (drawable))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_CANCEL,
                                                   NULL);
        }

      compute_image ();

      gimp_set_data (PLUG_IN_PROC, &mapvals, sizeof (MapObjectValues));
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &mapvals);
      check_drawables (drawable);

      if (! image_setup (drawable, FALSE))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_SUCCESS,
                                                   NULL);
        }

      compute_image ();
      break;

    case GIMP_RUN_NONINTERACTIVE:
      mapvals.maptype                 = GIMP_VALUES_GET_INT     (args, 0);
      mapvals.viewpoint.x             = GIMP_VALUES_GET_DOUBLE  (args, 1);
      mapvals.viewpoint.y             = GIMP_VALUES_GET_DOUBLE  (args, 2);
      mapvals.viewpoint.z             = GIMP_VALUES_GET_DOUBLE  (args, 3);
      mapvals.position.x              = GIMP_VALUES_GET_DOUBLE  (args, 4);
      mapvals.position.y              = GIMP_VALUES_GET_DOUBLE  (args, 5);
      mapvals.position.z              = GIMP_VALUES_GET_DOUBLE  (args, 6);
      mapvals.firstaxis.x             = GIMP_VALUES_GET_DOUBLE  (args, 7);
      mapvals.firstaxis.y             = GIMP_VALUES_GET_DOUBLE  (args, 8);
      mapvals.firstaxis.z             = GIMP_VALUES_GET_DOUBLE  (args, 9);
      mapvals.secondaxis.x            = GIMP_VALUES_GET_DOUBLE  (args, 10);
      mapvals.secondaxis.y            = GIMP_VALUES_GET_DOUBLE  (args, 11);
      mapvals.secondaxis.z            = GIMP_VALUES_GET_DOUBLE  (args, 12);
      mapvals.alpha                   = GIMP_VALUES_GET_DOUBLE  (args, 13);
      mapvals.beta                    = GIMP_VALUES_GET_DOUBLE  (args, 14);
      mapvals.gamma                   = GIMP_VALUES_GET_DOUBLE  (args, 15);
      mapvals.lightsource.type        = GIMP_VALUES_GET_INT     (args, 16);

      GIMP_VALUES_GET_RGB (args, 17, &mapvals.lightsource.color);

      mapvals.lightsource.position.x  = GIMP_VALUES_GET_DOUBLE  (args, 18);
      mapvals.lightsource.position.y  = GIMP_VALUES_GET_DOUBLE  (args, 19);
      mapvals.lightsource.position.z  = GIMP_VALUES_GET_DOUBLE  (args, 20);
      mapvals.lightsource.direction.x = GIMP_VALUES_GET_DOUBLE  (args, 21);
      mapvals.lightsource.direction.y = GIMP_VALUES_GET_DOUBLE  (args, 22);
      mapvals.lightsource.direction.z = GIMP_VALUES_GET_DOUBLE  (args, 23);
      mapvals.material.ambient_int    = GIMP_VALUES_GET_DOUBLE  (args, 24);
      mapvals.material.diffuse_int    = GIMP_VALUES_GET_DOUBLE  (args, 25);
      mapvals.material.diffuse_ref    = GIMP_VALUES_GET_DOUBLE  (args, 26);
      mapvals.material.specular_ref   = GIMP_VALUES_GET_DOUBLE  (args, 27);
      mapvals.material.highlight      = GIMP_VALUES_GET_DOUBLE  (args, 28);
      mapvals.antialiasing            = GIMP_VALUES_GET_BOOLEAN (args, 29);
      mapvals.tiled                   = GIMP_VALUES_GET_BOOLEAN (args, 30);
      mapvals.create_new_image        = GIMP_VALUES_GET_BOOLEAN (args, 31);
      mapvals.transparent_background  = GIMP_VALUES_GET_BOOLEAN (args, 32);
      mapvals.radius                  =
      mapvals.cylinder_radius         = GIMP_VALUES_GET_DOUBLE  (args, 33);
      mapvals.scale.x                 = GIMP_VALUES_GET_DOUBLE  (args, 34);
      mapvals.scale.y                 = GIMP_VALUES_GET_DOUBLE  (args, 35);
      mapvals.scale.z                 = GIMP_VALUES_GET_DOUBLE  (args, 36);
      mapvals.cylinder_length         = GIMP_VALUES_GET_DOUBLE  (args, 37);

      for (i = 0; i < 6; i++)
        {
          mapvals.boxmap_id[i] = GIMP_VALUES_GET_DRAWABLE_ID (args, 38 + i);
        }

      for (i = 0; i < 2; i++)
        {
          mapvals.cylindermap_id[i] = GIMP_VALUES_GET_DRAWABLE_ID (args, 44 + i);
        }

      check_drawables (drawable);

      if (! image_setup (drawable, FALSE))
        {
          return gimp_procedure_new_return_values (procedure,
                                                   GIMP_PDB_SUCCESS,
                                                   NULL);
        }

      compute_image ();
      break;
    }

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
