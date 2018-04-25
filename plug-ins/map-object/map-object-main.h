#ifndef __MAPOBJECT_MAIN_H__
#define __MAPOBJECT_MAIN_H__

/* Defines and stuff */
/* ================= */

#define PLUG_IN_PROC   "plug-in-map-object"
#define PLUG_IN_BINARY "map-object"
#define PLUG_IN_ROLE   "gimp-map-object"

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
  gdouble  ambient_int;
  gdouble  diffuse_int;
  gdouble  diffuse_ref;
  gdouble  specular_ref;
  gdouble  highlight;
  GimpRGB  color;
} MaterialSettings;

typedef struct
{
  LightType    type;
  GimpVector3  position;
  GimpVector3  direction;
  GimpRGB      color;
  gdouble      intensity;
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
  gint create_new_layer;
  gint transparent_background;
  gint tiled;
  gint livepreview;
  gint showgrid;
  gint showcaps;

  gdouble zoom;
  gdouble alpha,beta,gamma;
  gdouble maxdepth;
  gdouble pixelthreshold;
  gdouble radius;
  gdouble cylinder_radius;
  gdouble cylinder_length;

  gint32 boxmap_id[6];
  gint32 cylindermap_id[2];

} MapObjectValues;

/* Externally visible variables */
/* ============================ */

extern MapObjectValues mapvals;

#endif  /* __MAPOBJECT_MAIN_H__ */
