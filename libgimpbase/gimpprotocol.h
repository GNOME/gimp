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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PROTOCOL_H__
#define __GIMP_PROTOCOL_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* Increment every time the protocol changes
 */
#define GIMP_PROTOCOL_VERSION  0x0115


enum
{
  GP_QUIT,
  GP_CONFIG,
  GP_TILE_REQ,
  GP_TILE_ACK,
  GP_TILE_DATA,
  GP_PROC_RUN,
  GP_PROC_RETURN,
  GP_TEMP_PROC_RUN,
  GP_TEMP_PROC_RETURN,
  GP_PROC_INSTALL,
  GP_PROC_UNINSTALL,
  GP_EXTENSION_ACK,
  GP_HAS_INIT
};

typedef enum
{
  GP_PARAM_DEF_TYPE_DEFAULT,
  GP_PARAM_DEF_TYPE_INT,
  GP_PARAM_DEF_TYPE_UNIT,
  GP_PARAM_DEF_TYPE_ENUM,
  GP_PARAM_DEF_TYPE_CHOICE,
  GP_PARAM_DEF_TYPE_BOOLEAN,
  GP_PARAM_DEF_TYPE_DOUBLE,
  GP_PARAM_DEF_TYPE_STRING,
  GP_PARAM_DEF_TYPE_GEGL_COLOR,
  GP_PARAM_DEF_TYPE_ID,
  GP_PARAM_DEF_TYPE_ID_ARRAY,
  GP_PARAM_DEF_TYPE_EXPORT_OPTIONS,
  GP_PARAM_DEF_TYPE_RESOURCE,
  GP_PARAM_DEF_TYPE_FILE
} GPParamDefType;

typedef enum
{
  GP_PARAM_TYPE_INT,
  GP_PARAM_TYPE_DOUBLE,
  GP_PARAM_TYPE_STRING,
  GP_PARAM_TYPE_STRV,
  GP_PARAM_TYPE_BYTES,
  GP_PARAM_TYPE_FILE,
  GP_PARAM_TYPE_BABL_FORMAT,
  GP_PARAM_TYPE_GEGL_COLOR,
  GP_PARAM_TYPE_COLOR_ARRAY,
  GP_PARAM_TYPE_PARASITE,
  GP_PARAM_TYPE_ARRAY,
  GP_PARAM_TYPE_ID_ARRAY,
  GP_PARAM_TYPE_EXPORT_OPTIONS,
  GP_PARAM_TYPE_PARAM_DEF,
  GP_PARAM_TYPE_VALUE_ARRAY,
} GPParamType;


typedef struct _GPConfig                 GPConfig;
typedef struct _GPTileReq                GPTileReq;
typedef struct _GPTileAck                GPTileAck;
typedef struct _GPTileData               GPTileData;
typedef struct _GPParamDef               GPParamDef;
typedef struct _GPParamDefInt            GPParamDefInt;
typedef struct _GPParamDefUnit           GPParamDefUnit;
typedef struct _GPParamDefEnum           GPParamDefEnum;
typedef struct _GPParamDefBoolean        GPParamDefBoolean;
typedef struct _GPParamDefDouble         GPParamDefDouble;
typedef struct _GPParamDefString         GPParamDefString;
typedef struct _GPParamDefChoice         GPParamDefChoice;
typedef struct _GPParamStrv              GPParamStrv;
typedef struct _GPParamDefGeglColor      GPParamDefGeglColor;
typedef struct _GPParamDefID             GPParamDefID;
typedef struct _GPParamDefIDArray        GPParamDefIDArray;
typedef struct _GPParamDefResource       GPParamDefResource;
typedef struct _GPParamDefFile           GPParamDefFile;
typedef struct _GPParam                  GPParam;
typedef struct _GPParamArray             GPParamArray;
typedef struct _GPParamIDArray           GPParamIDArray;
typedef struct _GPParamFormat            GPParamFormat;
typedef struct _GPParamColor             GPParamColor;
typedef struct _GPParamColorArray        GPParamColorArray;
typedef struct _GPParamExportOptions     GPParamExportOptions;
typedef struct _GPParamValueArray        GPParamValueArray;
typedef struct _GPProcRun                GPProcRun;
typedef struct _GPProcReturn             GPProcReturn;
typedef struct _GPProcInstall            GPProcInstall;
typedef struct _GPProcUninstall          GPProcUninstall;


struct _GPConfig
{
  guint32  tile_width;
  guint32  tile_height;
  gint32   shm_id;
  gint8    check_size;
  gint8    check_type;
  gint8    show_help_button;
  gint8    use_cpu_accel;
  gint8    use_opencl;
  gint8    export_color_profile;
  gint8    export_comment;
  gint8    export_exif;
  gint8    export_xmp;
  gint8    export_iptc;
  gint8    update_metadata;
  gint32   default_display_id;
  gchar   *app_name;
  gchar   *wm_class;
  gchar   *display_name;
  gint32   monitor_number;
  guint32  timestamp;
  gchar   *icon_theme_dir;
  guint64  tile_cache_size;
  gchar   *swap_path;
  gchar   *swap_compression;
  gint32   num_processors;

  /* Since protocol version 0x0111:
   * These values are used to represent 2 GeglColor objects but we avoid
   * initializing GEGL in the wire protocol. This leads to extremely
   * slow startup when initializing every plug-in. So we store them as
   * bytes and encoding string and reconstruct the GeglColor in libgimp.
   */
  GBytes    *check_custom_color1;
  GBytes    *check_custom_icc1;
  gchar     *check_custom_encoding1;
  GBytes    *check_custom_color2;
  GBytes    *check_custom_icc2;
  gchar     *check_custom_encoding2;
};

struct _GPTileReq
{
  gint32   drawable_id;
  guint32  tile_num;
  guint32  shadow;
};

struct _GPTileData
{
  gint32   drawable_id;
  guint32  tile_num;
  guint32  shadow;
  guint32  bpp;
  guint32  width;
  guint32  height;
  guint32  use_shm;
  guchar  *data;
};

struct _GPParamDefInt
{
  gint64 min_val;
  gint64 max_val;
  gint64 default_val;
};

struct _GPParamDefUnit
{
  gint32 allow_pixels;
  gint32 allow_percent;
  gint32 default_val;
};

struct _GPParamDefEnum
{
  gint32 default_val;
};

struct _GPParamDefBoolean
{
  gint32 default_val;
};

struct _GPParamDefDouble
{
  gdouble min_val;
  gdouble max_val;
  gdouble default_val;
};

struct _GPParamDefString
{
  gchar *default_val;
};

struct _GPParamDefGeglColor
{
  gint32        has_alpha;
  GPParamColor *default_val;
};

struct _GPParamDefID
{
  gint32 none_ok;
};

struct _GPParamDefIDArray
{
  gchar *type_name;
};

struct _GPParamDefChoice
{
  GimpChoice *choice;
  gchar      *default_val;
};

struct _GPParamDefResource
{
  gint32 none_ok;
  gint32 default_to_context;
  gint32 default_resource_id;
};

struct _GPParamDefFile
{
  /* action is a GimpFileChooserAction casted to gint32. */
  gint32  action;
  gint32  none_ok;
  gchar  *default_uri;
};

struct _GPParamDef
{
  GPParamDefType  param_def_type;
  gchar          *type_name;
  gchar          *value_type_name;
  gchar          *name;
  gchar          *nick;
  gchar          *blurb;
  guint           flags;

  union
  {
    GPParamDefInt              m_int;
    GPParamDefUnit             m_unit;
    GPParamDefEnum             m_enum;
    GPParamDefBoolean          m_boolean;
    GPParamDefDouble           m_double;
    GPParamDefString           m_string;
    GPParamDefGeglColor        m_gegl_color;
    GPParamDefID               m_id;
    GPParamDefIDArray          m_id_array;
    GPParamDefChoice           m_choice;
    GPParamDefResource         m_resource;
    GPParamDefFile             m_file;
  } meta;
};

struct _GPParamArray
{
  guint32  size;
  guint8  *data;
};

struct _GPParamIDArray
{
  gchar   *type_name;
  guint32  size;
  gint32  *data;
};

struct _GPParamFormat
{
  /* Transferring a BablFormat with the encoding and the ICC data. */
  gchar   *encoding;
  guint32  profile_size;
  guint8  *profile_data;
};

struct _GPParamColor
{
  guint32       size;
  guint8        data[40];

  GPParamFormat format;
};

struct _GPParamColorArray
{
  guint32       size;
  GPParamColor *colors;
};

struct _GPParamExportOptions
{
  /* XXX: this is an empty shell right now, because there are no export
   * options yet. The capabilities property doesn't need to be passed
   * through the wire because it is set by libgimp, not at run call.
   */
};

struct _GPParamValueArray
{
  guint32  n_values;
  GPParam *values;
};

struct _GPParam
{
  GPParamType  param_type;
  gchar       *type_name;

  union
  {
    gint32                 d_int;
    gdouble                d_double;
    gchar                 *d_string;
    gchar                **d_strv;
    GBytes                *d_bytes;
    GPParamFormat          d_format;
    GPParamColor           d_gegl_color;
    GPParamColorArray      d_color_array;
    GimpParasite           d_parasite;
    GPParamArray           d_array;
    GPParamIDArray         d_id_array;
    GPParamExportOptions   d_export_options;
    GPParamDef             d_param_def;
    GPParamValueArray      d_value_array;
  } data;
};

struct _GPProcRun
{
  gchar   *name;
  guint32  n_params;
  GPParam *params;
};

struct _GPProcReturn
{
  gchar   *name;
  guint32  n_params;
  GPParam *params;
};

struct _GPProcInstall
{
  gchar      *name;
  guint32     type;
  guint32     n_params;
  guint32     n_return_vals;
  GPParamDef *params;
  GPParamDef *return_vals;
};

struct _GPProcUninstall
{
  gchar *name;
};


void      gp_init                   (void);

gboolean  gp_quit_write             (GIOChannel      *channel,
                                     gpointer         user_data);
gboolean  gp_config_write           (GIOChannel      *channel,
                                     GPConfig        *config,
                                     gpointer         user_data);
gboolean  gp_tile_req_write         (GIOChannel      *channel,
                                     GPTileReq       *tile_req,
                                     gpointer         user_data);
gboolean  gp_tile_ack_write         (GIOChannel      *channel,
                                     gpointer         user_data);
gboolean  gp_tile_data_write        (GIOChannel      *channel,
                                     GPTileData      *tile_data,
                                     gpointer         user_data);
gboolean  gp_proc_run_write         (GIOChannel      *channel,
                                     GPProcRun       *proc_run,
                                     gpointer         user_data);
gboolean  gp_proc_return_write      (GIOChannel      *channel,
                                     GPProcReturn    *proc_return,
                                     gpointer         user_data);
gboolean  gp_temp_proc_run_write    (GIOChannel      *channel,
                                     GPProcRun       *proc_run,
                                     gpointer         user_data);
gboolean  gp_temp_proc_return_write (GIOChannel      *channel,
                                     GPProcReturn    *proc_return,
                                     gpointer         user_data);
gboolean  gp_proc_install_write     (GIOChannel      *channel,
                                     GPProcInstall   *proc_install,
                                     gpointer         user_data);
gboolean  gp_proc_uninstall_write   (GIOChannel      *channel,
                                     GPProcUninstall *proc_uninstall,
                                     gpointer         user_data);
gboolean  gp_extension_ack_write    (GIOChannel      *channel,
                                     gpointer         user_data);
gboolean  gp_has_init_write         (GIOChannel      *channel,
                                     gpointer         user_data);


G_END_DECLS

#endif /* __GIMP_PROTOCOL_H__ */
