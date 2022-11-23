/* MapObject 1.2.0 -- image filter plug-in for LIGMA
 *
 * Copyright (C) 1996-98 Tom Bech
 * Copyright (C) 1996-98 Federico Mena Quintero
 *
 * E-mail: tomb@ligma.org (Tom) or quartic@ligma.org (Federico)
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "map-object-ui.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"

#include "libligma/stdplugins-intl.h"


MapObjectValues mapvals;


typedef struct _Map      Map;
typedef struct _MapClass MapClass;

struct _Map
{
  LigmaPlugIn parent_instance;
};

struct _MapClass
{
  LigmaPlugInClass parent_class;
};


#define MAP_TYPE  (map_get_type ())
#define MAP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAP_TYPE, Map))

GType                   map_get_type         (void) G_GNUC_CONST;

static GList          * map_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * map_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * map_run              (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static void             set_default_settings (void);
static void             check_drawables      (LigmaDrawable         *drawable);


G_DEFINE_TYPE (Map, map, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (MAP_TYPE)
DEFINE_STD_SET_I18N


static void
map_class_init (MapClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = map_query_procedures;
  plug_in_class->create_procedure = map_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
map_init (Map *map)
{
}

static GList *
map_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
map_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      LigmaRGB white = { 1.0, 1.0, 1.0, 1.0 };

      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            map_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("Map _Object..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Map");

      ligma_procedure_set_documentation (procedure,
                                        _("Map the image to an object "
                                          "(plane, sphere, box or cylinder)"),
                                        "No help yet",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 1.2.0, July 16 1998");

      LIGMA_PROC_ARG_INT (procedure, "map-type",
                         "Map type",
                         "Type of mapping (0=plane, 1=sphere, 2=box, "
                         "3=cylinder)",
                         0, 2, MAP_PLANE,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "viewpoint-x",
                            "Viewpoint X",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "viewpoint-y",
                            "Viewpoint Y",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "viewpoint-z",
                            "Viewpoint Z",
                            "Position of viewpoint (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "position-x",
                            "Position X",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "position-y",
                            "Position Y",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "position-z",
                            "Position Z",
                            "Object position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "first-axis-x",
                            "First axis X",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "first-axis-y",
                            "First axis y",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "first-axis-z",
                            "First axis Z",
                            "First axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "second-axis-x",
                            "Second axis X",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "second-axis-y",
                            "Second axis Y",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "second-axis-z",
                            "Second axis Z",
                            "Second axis of object (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "rotation-angle-x",
                            "Rotation angle X",
                            "Rotation about X axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "rotation-angle-y",
                            "Rotation angle Y",
                            "Rotation about Y axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "rotation-angle-z",
                            "Rotation angle Z",
                            "Rotation about Z axis in degrees",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "light-type",
                         "Light type",
                         "Type of lightsource (0=point, 1=directional, 2=none)",
                         0, 2, POINT_LIGHT,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "light-color",
                         "Light color",
                         "Light source color",
                         TRUE, &white,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-x",
                            "Light position X",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-y",
                            "Light position Y",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-z",
                            "Light position Z",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 2.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-x",
                            "Light direction X",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-y",
                            "Light direction Y",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1.0,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-z",
                            "Light direction Z",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "ambient-intensity",
                            "Ambient intensity",
                            "Material ambient intensity",
                            0, 1, 0.3,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "diffuse-intensity",
                            "Diffuse intensity",
                            "Material diffuse intensity",
                            0, 1, 1.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "diffuse-reflectivity",
                            "Diffuse reflectivity",
                            "Material diffuse reflectivity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "specular-reflectivity",
                            "Specular reflectivity",
                            "Material specular reflectivity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "highlight",
                            "Highlight",
                            "Material highlight (note, it's exponential)",
                            0, G_MAXDOUBLE, 27.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "antialiasing",
                             "Antialiasing",
                             "Apply antialiasing",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "tiled",
                             "Tiled",
                             "Tile source image",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "new-image",
                             "New image",
                             "Create a new image",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "transparent-background",
                             "Transparent background",
                             "Make background transparent",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "radius",
                            "Radius",
                            "Sphere/cylinder radius (only used when "
                            "maptype=1 or 3)",
                            0, G_MAXDOUBLE, 0.25,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "x-scale",
                            "X scale",
                            "Box X size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "y-scale",
                            "Y scale",
                            "Box Y size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DOUBLE (procedure, "z-scale",
                            "Z scale",
                            "Box Z size",
                            0, G_MAXDOUBLE, 0.5,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "cylinder-length",
                            "Cylinder length",
                            "Cylinder length",
                            0, G_MAXDOUBLE, 0.25,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-front-drawable",
                              "Box front drawable",
                              "Box front face (set these to NULL if not used)",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-back-drawable",
                              "Box back drawable",
                              "Box back face",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-top-drawable",
                              "Box top drawable",
                              "Box top face",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-bottom-drawable",
                              "Box bottom drawable",
                              "Box bottom face",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-left-drawable",
                              "Box left drawable",
                              "Box left face",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "box-right-drawable",
                              "Box right drawable",
                              "Box right face",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DRAWABLE (procedure, "cyl-top-drawable",
                              "Cyl top drawable",
                              "Cylinder top face (set these to NULL if not used)",
                              TRUE,
                              G_PARAM_READWRITE);
      LIGMA_PROC_ARG_DRAWABLE (procedure, "cyl-bottom-drawable",
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

  ligma_vector3_set (&mapvals.viewpoint,  0.5, 0.5, 2.0);
  ligma_vector3_set (&mapvals.firstaxis,  1.0, 0.0, 0.0);
  ligma_vector3_set (&mapvals.secondaxis, 0.0, 1.0, 0.0);
  ligma_vector3_set (&mapvals.normal,     0.0, 0.0, 1.0);
  ligma_vector3_set (&mapvals.position,   0.5, 0.5, 0.0);
  ligma_vector3_set (&mapvals.lightsource.position,  -0.5, -0.5, 2.0);
  ligma_vector3_set (&mapvals.lightsource.direction, -1.0, -1.0, 1.0);
  ligma_vector3_set (&mapvals.scale,      0.5, 0.5, 0.5);

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
  ligma_rgba_set (&mapvals.lightsource.color, 1.0, 1.0, 1.0, 1.0);

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
check_drawables (LigmaDrawable *drawable)
{
  LigmaDrawable *map;
  gint          i;

  /* Check that boxmap images are valid */
  /* ================================== */

  for (i = 0; i < 6; i++)
    {
      map = ligma_drawable_get_by_id (mapvals.boxmap_id[i]);

      if (! map || ligma_drawable_is_gray (map))
        mapvals.boxmap_id[i] = ligma_item_get_id (LIGMA_ITEM (drawable));
    }

  /* Check that cylindermap images are valid */
  /* ======================================= */

  for (i = 0; i < 2; i++)
    {
     map = ligma_drawable_get_by_id (mapvals.cylindermap_id[i]);

     if (! map || ligma_drawable_is_gray (map))
        mapvals.cylindermap_id[i] = ligma_item_get_id (LIGMA_ITEM (drawable));
    }
}

static LigmaValueArray *
map_run (LigmaProcedure        *procedure,
         LigmaRunMode           run_mode,
         LigmaImage            *_image,
         gint                  n_drawables,
         LigmaDrawable        **drawables,
         const LigmaValueArray *args,
         gpointer              run_data)
{
  LigmaDrawable *drawable;
  gint          i;

  gegl_init (NULL, NULL);

  image = _image;

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   ligma_procedure_get_name (procedure));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  set_default_settings ();

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &mapvals);
      check_drawables (drawable);

      if (! main_dialog (drawable))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_CANCEL,
                                                   NULL);
        }

      compute_image ();

      ligma_set_data (PLUG_IN_PROC, &mapvals, sizeof (MapObjectValues));
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &mapvals);
      check_drawables (drawable);

      if (! image_setup (drawable, FALSE))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_SUCCESS,
                                                   NULL);
        }

      compute_image ();
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      mapvals.maptype                 = LIGMA_VALUES_GET_INT     (args, 0);
      mapvals.viewpoint.x             = LIGMA_VALUES_GET_DOUBLE  (args, 1);
      mapvals.viewpoint.y             = LIGMA_VALUES_GET_DOUBLE  (args, 2);
      mapvals.viewpoint.z             = LIGMA_VALUES_GET_DOUBLE  (args, 3);
      mapvals.position.x              = LIGMA_VALUES_GET_DOUBLE  (args, 4);
      mapvals.position.y              = LIGMA_VALUES_GET_DOUBLE  (args, 5);
      mapvals.position.z              = LIGMA_VALUES_GET_DOUBLE  (args, 6);
      mapvals.firstaxis.x             = LIGMA_VALUES_GET_DOUBLE  (args, 7);
      mapvals.firstaxis.y             = LIGMA_VALUES_GET_DOUBLE  (args, 8);
      mapvals.firstaxis.z             = LIGMA_VALUES_GET_DOUBLE  (args, 9);
      mapvals.secondaxis.x            = LIGMA_VALUES_GET_DOUBLE  (args, 10);
      mapvals.secondaxis.y            = LIGMA_VALUES_GET_DOUBLE  (args, 11);
      mapvals.secondaxis.z            = LIGMA_VALUES_GET_DOUBLE  (args, 12);
      mapvals.alpha                   = LIGMA_VALUES_GET_DOUBLE  (args, 13);
      mapvals.beta                    = LIGMA_VALUES_GET_DOUBLE  (args, 14);
      mapvals.gamma                   = LIGMA_VALUES_GET_DOUBLE  (args, 15);
      mapvals.lightsource.type        = LIGMA_VALUES_GET_INT     (args, 16);

      LIGMA_VALUES_GET_RGB (args, 17, &mapvals.lightsource.color);

      mapvals.lightsource.position.x  = LIGMA_VALUES_GET_DOUBLE  (args, 18);
      mapvals.lightsource.position.y  = LIGMA_VALUES_GET_DOUBLE  (args, 19);
      mapvals.lightsource.position.z  = LIGMA_VALUES_GET_DOUBLE  (args, 20);
      mapvals.lightsource.direction.x = LIGMA_VALUES_GET_DOUBLE  (args, 21);
      mapvals.lightsource.direction.y = LIGMA_VALUES_GET_DOUBLE  (args, 22);
      mapvals.lightsource.direction.z = LIGMA_VALUES_GET_DOUBLE  (args, 23);
      mapvals.material.ambient_int    = LIGMA_VALUES_GET_DOUBLE  (args, 24);
      mapvals.material.diffuse_int    = LIGMA_VALUES_GET_DOUBLE  (args, 25);
      mapvals.material.diffuse_ref    = LIGMA_VALUES_GET_DOUBLE  (args, 26);
      mapvals.material.specular_ref   = LIGMA_VALUES_GET_DOUBLE  (args, 27);
      mapvals.material.highlight      = LIGMA_VALUES_GET_DOUBLE  (args, 28);
      mapvals.antialiasing            = LIGMA_VALUES_GET_BOOLEAN (args, 29);
      mapvals.tiled                   = LIGMA_VALUES_GET_BOOLEAN (args, 30);
      mapvals.create_new_image        = LIGMA_VALUES_GET_BOOLEAN (args, 31);
      mapvals.transparent_background  = LIGMA_VALUES_GET_BOOLEAN (args, 32);
      mapvals.radius                  =
      mapvals.cylinder_radius         = LIGMA_VALUES_GET_DOUBLE  (args, 33);
      mapvals.scale.x                 = LIGMA_VALUES_GET_DOUBLE  (args, 34);
      mapvals.scale.y                 = LIGMA_VALUES_GET_DOUBLE  (args, 35);
      mapvals.scale.z                 = LIGMA_VALUES_GET_DOUBLE  (args, 36);
      mapvals.cylinder_length         = LIGMA_VALUES_GET_DOUBLE  (args, 37);

      for (i = 0; i < 6; i++)
        {
          mapvals.boxmap_id[i] = LIGMA_VALUES_GET_DRAWABLE_ID (args, 38 + i);
        }

      for (i = 0; i < 2; i++)
        {
          mapvals.cylindermap_id[i] = LIGMA_VALUES_GET_DRAWABLE_ID (args, 44 + i);
        }

      check_drawables (drawable);

      if (! image_setup (drawable, FALSE))
        {
          return ligma_procedure_new_return_values (procedure,
                                                   LIGMA_PDB_SUCCESS,
                                                   NULL);
        }

      compute_image ();
      break;
    }

  if (run_mode != LIGMA_RUN_NONINTERACTIVE)
    ligma_displays_flush ();

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}
