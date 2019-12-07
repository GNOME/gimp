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


/* Global variables */
/* ================ */

MapObjectValues mapvals;

/******************/
/* Implementation */
/******************/

/*************************************/
/* Set parameters to standard values */
/*************************************/

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
check_drawables (gint32 drawable_id)
{
  gint i;

  /* Check that boxmap images are valid */
  /* ================================== */

  for (i = 0; i < 6; i++)
    {
      if (mapvals.boxmap_id[i] == -1 ||
          !gimp_item_is_valid (mapvals.boxmap_id[i]) ||
          gimp_drawable_is_gray (mapvals.boxmap_id[i]))
        mapvals.boxmap_id[i] = drawable_id;
    }

  /* Check that cylindermap images are valid */
  /* ======================================= */

  for (i = 0; i < 2; i++)
    {
      if (mapvals.cylindermap_id[i] == -1 ||
          !gimp_item_is_valid (mapvals.cylindermap_id[i]) ||
          gimp_drawable_is_gray (mapvals.cylindermap_id[i]))
        mapvals.cylindermap_id[i] = drawable_id;
    }
}

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",              "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",                 "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",              "Input drawable" },
    { GIMP_PDB_INT32,    "maptype",               "Type of mapping (0=plane,1=sphere,2=box,3=cylinder)" },
    { GIMP_PDB_FLOAT,    "viewpoint-x",           "Position of viewpoint (x,y,z)" },
    { GIMP_PDB_FLOAT,    "viewpoint-y",           "Position of viewpoint (x,y,z)" },
    { GIMP_PDB_FLOAT,    "viewpoint-z",           "Position of viewpoint (x,y,z)" },
    { GIMP_PDB_FLOAT,    "position-x",            "Object position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "position-y",            "Object position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "position-z",            "Object position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "firstaxis-x",           "First axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "firstaxis-y",           "First axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "firstaxis-z",           "First axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "secondaxis-x",          "Second axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "secondaxis-y",          "Second axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "secondaxis-z",          "Second axis of object [x,y,z]" },
    { GIMP_PDB_FLOAT,    "rotationangle-x",       "Rotation about X axis in degrees" },
    { GIMP_PDB_FLOAT,    "rotationangle-y",       "Rotation about Y axis in degrees" },
    { GIMP_PDB_FLOAT,    "rotationangle-z",       "Rotation about Z axis in degrees" },
    { GIMP_PDB_INT32,    "lighttype",             "Type of lightsource (0=point,1=directional,2=none)" },
    { GIMP_PDB_COLOR,    "lightcolor",            "Lightsource color (r,g,b)" },
    { GIMP_PDB_FLOAT,    "lightposition-x",       "Lightsource position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "lightposition-y",       "Lightsource position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "lightposition-z",       "Lightsource position (x,y,z)" },
    { GIMP_PDB_FLOAT,    "lightdirection-x",      "Lightsource direction [x,y,z]" },
    { GIMP_PDB_FLOAT,    "lightdirection-y",      "Lightsource direction [x,y,z]" },
    { GIMP_PDB_FLOAT,    "lightdirection-z",      "Lightsource direction [x,y,z]" },
    { GIMP_PDB_FLOAT,    "ambient_intensity",     "Material ambient intensity (0..1)" },
    { GIMP_PDB_FLOAT,    "diffuse_intensity",     "Material diffuse intensity (0..1)" },
    { GIMP_PDB_FLOAT,    "diffuse_reflectivity",  "Material diffuse reflectivity (0..1)" },
    { GIMP_PDB_FLOAT,    "specular_reflectivity", "Material specular reflectivity (0..1)" },
    { GIMP_PDB_FLOAT,    "highlight",             "Material highlight (0..->), note: it's exponential" },
    { GIMP_PDB_INT32,    "antialiasing",          "Apply antialiasing (TRUE/FALSE)" },
    { GIMP_PDB_INT32,    "tiled",                 "Tile source image (TRUE/FALSE)" },
    { GIMP_PDB_INT32,    "newimage",              "Create a new image (TRUE/FALSE)" },
    { GIMP_PDB_INT32,    "transparentbackground", "Make background transparent (TRUE/FALSE)" },
    { GIMP_PDB_FLOAT,    "radius",                "Sphere/cylinder radius (only used when maptype=1 or 3)" },
    { GIMP_PDB_FLOAT,    "x-scale",               "Box x size (0..->)" },
    { GIMP_PDB_FLOAT,    "y-scale",               "Box y size (0..->)" },
    { GIMP_PDB_FLOAT,    "z-scale",               "Box z size (0..->)"},
    { GIMP_PDB_FLOAT,    "cylinder-length",       "Cylinder length (0..->)"},
    { GIMP_PDB_DRAWABLE, "box-front-drawable",    "Box front face (set these to -1 if not used)" },
    { GIMP_PDB_DRAWABLE, "box-back-drawable",     "Box back face" },
    { GIMP_PDB_DRAWABLE, "box-top-drawable",      "Box top face" },
    { GIMP_PDB_DRAWABLE, "box-bottom-drawable",   "Box bottom face" },
    { GIMP_PDB_DRAWABLE, "box-left-drawable",     "Box left face" },
    { GIMP_PDB_DRAWABLE, "box-right-drawable",    "Box right face" },
    { GIMP_PDB_DRAWABLE, "cyl-top-drawable",      "Cylinder top face (set these to -1 if not used)" },
    { GIMP_PDB_DRAWABLE, "cyl-bottom-drawable",   "Cylinder bottom face" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Map the image to an object (plane, sphere, box or cylinder)"),
                          "No help yet",
                          "Tom Bech & Federico Mena Quintero",
                          "Tom Bech & Federico Mena Quintero",
                          "Version 1.2.0, July 16 1998",
                          N_("Map _Object..."),
                          "RGB*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Map");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpRunMode        run_mode;
  gint32             drawable_id;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint               i;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  /* Set default values */
  /* ================== */

  set_default_settings ();

  /* Get the specified drawable */
  /* ========================== */

  run_mode    = param[0].data.d_int32;
  image_id    = param[1].data.d_int32;
  drawable_id = param[2].data.d_int32;

  switch (run_mode)
    {
      case GIMP_RUN_INTERACTIVE:

        /* Possibly retrieve data */
        /* ====================== */

        gimp_get_data (PLUG_IN_PROC, &mapvals);
        check_drawables (drawable_id);
        if (main_dialog (drawable_id))
          {
            compute_image ();

            gimp_set_data (PLUG_IN_PROC, &mapvals, sizeof (MapObjectValues));
          }
        break;

      case GIMP_RUN_WITH_LAST_VALS:
        gimp_get_data (PLUG_IN_PROC, &mapvals);
        check_drawables (drawable_id);
        if (image_setup (drawable_id, FALSE))
          compute_image ();
        break;

      case GIMP_RUN_NONINTERACTIVE:
        if (nparams != 49)
          {
            status = GIMP_PDB_CALLING_ERROR;
          }
        else
          {
            mapvals.maptype                 = (MapType) param[3].data.d_int32;
            mapvals.viewpoint.x             = param[4].data.d_float;
            mapvals.viewpoint.y             = param[5].data.d_float;
            mapvals.viewpoint.z             = param[6].data.d_float;
            mapvals.position.x              = param[7].data.d_float;
            mapvals.position.y              = param[8].data.d_float;
            mapvals.position.z              = param[9].data.d_float;
            mapvals.firstaxis.x             = param[10].data.d_float;
            mapvals.firstaxis.y             = param[11].data.d_float;
            mapvals.firstaxis.z             = param[12].data.d_float;
            mapvals.secondaxis.x            = param[13].data.d_float;
            mapvals.secondaxis.y            = param[14].data.d_float;
            mapvals.secondaxis.z            = param[15].data.d_float;
            mapvals.alpha                   = param[16].data.d_float;
            mapvals.beta                    = param[17].data.d_float;
            mapvals.gamma                   = param[18].data.d_float;
            mapvals.lightsource.type        = (LightType) param[19].data.d_int32;
            mapvals.lightsource.color       = param[20].data.d_color;
            mapvals.lightsource.position.x  = param[21].data.d_float;
            mapvals.lightsource.position.y  = param[22].data.d_float;
            mapvals.lightsource.position.z  = param[23].data.d_float;
            mapvals.lightsource.direction.x = param[24].data.d_float;
            mapvals.lightsource.direction.y = param[25].data.d_float;
            mapvals.lightsource.direction.z = param[26].data.d_float;
            mapvals.material.ambient_int    = param[27].data.d_float;
            mapvals.material.diffuse_int    = param[28].data.d_float;
            mapvals.material.diffuse_ref    = param[29].data.d_float;
            mapvals.material.specular_ref   = param[30].data.d_float;
            mapvals.material.highlight      = param[31].data.d_float;
            mapvals.antialiasing            = (gint) param[32].data.d_int32;
            mapvals.tiled                   = (gint) param[33].data.d_int32;
            mapvals.create_new_image        = (gint) param[34].data.d_int32;
            mapvals.transparent_background  = (gint) param[35].data.d_int32;
            mapvals.radius                  = param[36].data.d_float;
            mapvals.cylinder_radius         = param[36].data.d_float;
            mapvals.scale.x                 = param[37].data.d_float;
            mapvals.scale.y                 = param[38].data.d_float;
            mapvals.scale.z                 = param[39].data.d_float;
            mapvals.cylinder_length         = param[40].data.d_float;

            for (i = 0; i < 6; i++)
              mapvals.boxmap_id[i] = param[41+i].data.d_drawable;

            for (i = 0; i < 2; i++)
              mapvals.cylindermap_id[i] = param[47+i].data.d_drawable;

            check_drawables (drawable_id);
            if (image_setup (drawable_id, FALSE))
              compute_image ();
          }
        break;
    }

  values[0].data.d_status = status;

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();
}

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()
