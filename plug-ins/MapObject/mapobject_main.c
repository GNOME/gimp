/*********************************************************************************/
/* MapObject 1.00 -- image filter plug-in for The Gimp program                   */
/* Copyright (C) 1996-98 Tom Bech                                                */
/* Copyright (C) 1996-98 Federico Mena Quintero                                  */
/*===============================================================================*/
/* E-mail: tomb@gimp.org (Tom) or quartic@gimp.org (Federico)                    */
/* You can contact the original The Gimp authors at gimp@xcf.berkeley.edu        */
/*===============================================================================*/
/* This program is free software; you can redistribute it and/or modify it under */
/* the terms of the GNU General Public License as published by the Free Software */
/* Foundation; either version 2 of the License, or (at your option) any later    */
/* version.                                                                      */
/*===============================================================================*/
/* This program is distributed in the hope that it will be useful, but WITHOUT   */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.*/
/*===============================================================================*/
/* You should have received a copy of the GNU General Public License along with  */
/* this program (read the "COPYING" file); if not, write to the Free Software    */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.    */
/*===============================================================================*/
/* In other words, you can't sue us for whatever happens while using this ;)     */
/*********************************************************************************/

#include "mapobject_main.h"

/* Global variables */
/* ================ */

MapObjectValues mapvals;

/******************/
/* Implementation */
/******************/

void mapobject_interactive    (GDrawable *drawable);
void mapobject_noninteractive (GDrawable *drawable);

/*************************************/
/* Set parameters to standard values */
/*************************************/

void set_default_settings(void)
{
  gck_vector3_set(&mapvals.viewpoint,  0.5,0.5,2.0);
  gck_vector3_set(&mapvals.firstaxis,  1.0,0.0,0.0);
  gck_vector3_set(&mapvals.secondaxis, 0.0,1.0,0.0);
  gck_vector3_set(&mapvals.normal,     0.0,0.0,1.0);
  gck_vector3_set(&mapvals.position,   0.5,0.5,0.0);
  gck_vector3_set(&mapvals.lightsource.position, -0.5,-0.5,2.0);
  gck_vector3_set(&mapvals.lightsource.direction, -1.0,-1.0,1.0);

  mapvals.maptype=MAP_PLANE;

  mapvals.pixeltreshold=0.25;
  mapvals.alpha=mapvals.beta=mapvals.gamma=0.0;
  mapvals.maxdepth=3.0;
  mapvals.radius=0.25;

  mapvals.preview_zoom_factor=0;

  mapvals.lightsource.type=POINT_LIGHT;

  mapvals.antialiasing=TRUE;
  mapvals.create_new_image=FALSE;
  mapvals.transparent_background=FALSE;
  mapvals.tiled=FALSE;
  mapvals.showgrid=FALSE;
  mapvals.tooltips_enabled=TRUE;

  mapvals.lightsource.intensity = 1.0;
  gck_rgb_set(&mapvals.lightsource.color,1.0,1.0,1.0);

  mapvals.material.ambient_int = 0.3;
  mapvals.material.diffuse_int = 1.0;
  mapvals.material.diffuse_ref = 0.5;
  mapvals.material.specular_ref = 0.5;
  mapvals.material.highlight = 27.0;
}

MAIN()

static void query(void)
{

  static GParamDef args[] =
    {
      { PARAM_INT32,      "run_mode",              "Interactive (0), non-interactive (1)" },
      { PARAM_IMAGE,      "image",                 "Input image" },
      { PARAM_DRAWABLE,   "drawable",              "Input drawable" },
      { PARAM_INT32,      "maptype",               "Type of mapping (0=plane,1=sphere)" },
      { PARAM_FLOAT, "viewpoint_x",                "Position of viewpoint (x,y,z)" },
      { PARAM_FLOAT, "viewpoint_y",                "Position of viewpoint (x,y,z)" },
      { PARAM_FLOAT, "viewpoint_z",                "Position of viewpoint (x,y,z)" },
      { PARAM_FLOAT, "position_x",              "Object position (x,y,z)" },
      { PARAM_FLOAT, "position_y",              "Object position (x,y,z)" },
      { PARAM_FLOAT, "position_z",              "Object position (x,y,z)" },
      { PARAM_FLOAT, "firstaxis_x",             "First axis of object [x,y,z]" },
      { PARAM_FLOAT, "firstaxis_y",             "First axis of object [x,y,z]" },
      { PARAM_FLOAT, "firstaxis_z",             "First axis of object [x,y,z]" },
      { PARAM_FLOAT, "secondaxis_x",            "Second axis of object [x,y,z]" },
      { PARAM_FLOAT, "secondaxis_y",            "Second axis of object [x,y,z]" },
      { PARAM_FLOAT, "secondaxis_z",            "Second axis of object [x,y,z]" },
      { PARAM_FLOAT, "rotationangle_x",         "Axis rotation (xy,xz,yz) in degrees" },
      { PARAM_FLOAT, "rotationangle_y",         "Axis rotation (xy,xz,yz) in degrees" },
      { PARAM_FLOAT, "rotationangle_z",         "Axis rotation (xy,xz,yz) in degrees" },
      { PARAM_INT32,      "lighttype",             "Type of lightsource (0=point,1=directional,3=none)" },
      { PARAM_COLOR,      "lightcolor",            "Lightsource color (r,g,b)" },
      { PARAM_FLOAT, "lightposition_x",         "Lightsource position (x,y,z)" },
      { PARAM_FLOAT, "lightposition_y",         "Lightsource position (x,y,z)" },
      { PARAM_FLOAT, "lightposition_z",         "Lightsource position (x,y,z)" },
      { PARAM_FLOAT, "lightdirection_x",        "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT, "lightdirection_y",        "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT, "lightdirection_z",        "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT,      "ambient_intensity",     "Material ambient intensity (0..1)" },
      { PARAM_FLOAT,      "diffuse_intensity",     "Material diffuse intensity (0..1)" },
      { PARAM_FLOAT,      "diffuse_reflectivity",  "Material diffuse reflectivity (0..1)" },
      { PARAM_FLOAT,      "specular_reflectivity", "Material specular reflectivity (0..1)" },
      { PARAM_FLOAT,      "highlight",             "Material highlight (0..->), note: it's expotential" },
      { PARAM_INT32,      "antialiasing",          "Apply antialiasing (TRUE/FALSE)" },
      { PARAM_INT32,      "tiled",                 "Tile source image (TRUE/FALSE)" },
      { PARAM_INT32,      "newimage",              "Create a new image (TRUE/FALSE)" },
      { PARAM_INT32,      "transparentbackground", "Make background transparent (TRUE/FALSE)" },
      { PARAM_FLOAT,      "radius",                "Sphere radius (only used when maptype=1)" }
    };

  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_map_object",
			  "Maps a picture to a object (plane, sphere)",
			  "No help yet",
			  "Tom Bech & Federico Mena Quintero",
			  "Tom Bech & Federico Mena Quintero",
			  "Version 1.00, March 15 1998",
			  "<Image>/Filters/Map/Map Object",
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run(gchar   *name,
                gint     nparams,
                GParam  *param,
                gint    *nreturn_vals,
                GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals = values;

  /* Set default values */
  /* ================== */

  set_default_settings();

  /* Get the specified drawable */
  /* ========================== */
  
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
      case RUN_INTERACTIVE:
        
        /* Possibly retrieve data */
        /* ====================== */
    
        gimp_get_data ("plug_in_map_object", &mapvals);
        mapobject_interactive(drawable);
        gimp_set_data("plug_in_map_object", &mapvals, sizeof(MapObjectValues));
        break;
      case RUN_WITH_LAST_VALS:
        gimp_get_data ("plug_in_map_object", &mapvals);
        image_setup(drawable,FALSE);
        compute_image();
        break;
      case RUN_NONINTERACTIVE:
        if (nparams != 37)
          status = STATUS_CALLING_ERROR;
        else if (status == STATUS_SUCCESS)
          {
            mapvals.maptype                 = (MapType)param[3].data.d_int32;
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
            mapvals.lightsource.type        = (LightType)param[19].data.d_int32;
            mapvals.lightsource.color.r     = param[20].data.d_color.red;
            mapvals.lightsource.color.g     = param[20].data.d_color.green;
            mapvals.lightsource.color.b     = param[20].data.d_color.blue;
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
            mapvals.antialiasing            = (gint)param[32].data.d_int32;
            mapvals.tiled                   = (gint)param[33].data.d_int32;
            mapvals.create_new_image        = (gint)param[34].data.d_int32;
            mapvals.transparent_background  = (gint)param[35].data.d_int32;
            mapvals.radius                  = param[36].data.d_float;

            image_setup(drawable, FALSE);
            compute_image();
          }
        break;
    }

  if (run_mode != RUN_NONINTERACTIVE)
     gimp_displays_flush();


  values[0].data.d_status = status;
  gimp_drawable_detach(drawable);
  	
}

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

void mapobject_interactive(GDrawable *drawable)
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("map_object");

  gdk_set_use_xshm(gimp_use_xshm());

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Set up ArcBall stuff */
  /* ==================== */

  /*ArcBall_Init(); */

  /* Create application window */
  /* ========================= */

  create_main_dialog();
  
  /* Prepare images */
  /* ============== */

  image_setup(drawable,TRUE);
  
  /* Gtk main event loop */
  /* =================== */
  
  gtk_main();
  gdk_flush();
}

