#ifndef __MAPOBJECT_MAIN_H__
#define __MAPOBJECT_MAIN_H__

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <gck/gck.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "arcball.h"
#include "mapobject_ui.h"
#include "mapobject_image.h"
#include "mapobject_apply.h"
#include "mapobject_preview.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"

/* Defines and stuff */
/* ================= */

#define TILE_CACHE_SIZE 16

/* Typedefs */
/* ======== */

typedef enum
{
  POINT_LIGHT,
  DIRECTIONAL_LIGHT,
  NO_LIGHT
} LightType;

typedef enum
{
  MAP_PLANE,
  MAP_SPHERE,
  MAP_BOX,
  MAP_CYLINDER
} MapType;

/* Typedefs */
/* ======== */

typedef struct
{
  gdouble ambient_int;
  gdouble diffuse_int;
  gdouble diffuse_ref;
  gdouble specular_ref;
  gdouble highlight;
  GckRGB  color;
} MaterialSettings;

typedef struct
{
  LightType   type;
  GimpVector3 position;
  GimpVector3 direction;
  GckRGB      color;
  gdouble     intensity;
} LightSettings;

typedef struct
{
  GimpVector3   viewpoint,firstaxis,secondaxis,normal,position,scale;
  LightSettings lightsource;

  MaterialSettings material;
  MaterialSettings refmaterial;

  MapType maptype;

  gint antialiasing;
  gint create_new_image;
  gint transparent_background;
  gint tiled;
  gint showgrid;
  gint tooltips_enabled;
  gint showcaps;

  glong preview_zoom_factor;

  gdouble alpha,beta,gamma;
  gdouble maxdepth;
  gdouble pixeltreshold;
  gdouble radius;
  gdouble cylinder_radius;
  gdouble cylinder_length;

  gint32 boxmap_id[6];
  gint32 cylindermap_id[2];
  
} MapObjectValues;

/* Externally visible variables */
/* ============================ */

extern MapObjectValues mapvals;
extern GckRGB background;

#endif  /* __MAPOBJECT_MAIN_H__ */
