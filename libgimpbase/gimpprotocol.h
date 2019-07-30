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
#define GIMP_PROTOCOL_VERSION  0x0106


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
  GP_PARAM_TYPE_INT,
  GP_PARAM_TYPE_FLOAT,
  GP_PARAM_TYPE_STRING,
  GP_PARAM_TYPE_COLOR,
  GP_PARAM_TYPE_PARASITE,
  GP_PARAM_TYPE_ARRAY,
  GP_PARAM_TYPE_STRING_ARRAY
} GPParamType;

typedef enum
{
  GP_PARAM_DEF_TYPE_DEFAULT,
  GP_PARAM_DEF_TYPE_INT,
  GP_PARAM_DEF_TYPE_UNIT,
  GP_PARAM_DEF_TYPE_ENUM,
  GP_PARAM_DEF_TYPE_BOOLEAN,
  GP_PARAM_DEF_TYPE_FLOAT,
  GP_PARAM_DEF_TYPE_STRING,
  GP_PARAM_DEF_TYPE_COLOR,
  GP_PARAM_DEF_TYPE_ID
} GPParamDefType;


typedef struct _GPConfig           GPConfig;
typedef struct _GPTileReq          GPTileReq;
typedef struct _GPTileAck          GPTileAck;
typedef struct _GPTileData         GPTileData;
typedef struct _GPParam            GPParam;
typedef struct _GPParamArray       GPParamArray;
typedef struct _GPParamStringArray GPParamStringArray;
typedef struct _GPParamDef         GPParamDef;
typedef struct _GPParamDefInt      GPParamDefInt;
typedef struct _GPParamDefUnit     GPParamDefUnit;
typedef struct _GPParamDefEnum     GPParamDefEnum;
typedef struct _GPParamDefBoolean  GPParamDefBoolean;
typedef struct _GPParamDefFloat    GPParamDefFloat;
typedef struct _GPParamDefString   GPParamDefString;
typedef struct _GPParamDefColor    GPParamDefColor;
typedef struct _GPParamDefID       GPParamDefID;
typedef struct _GPProcRun          GPProcRun;
typedef struct _GPProcReturn       GPProcReturn;
typedef struct _GPProcInstall      GPProcInstall;
typedef struct _GPProcUninstall    GPProcUninstall;


struct _GPConfig
{
  guint32  tile_width;
  guint32  tile_height;
  gint32   shm_ID;
  gint8    check_size;
  gint8    check_type;
  gint8    show_help_button;
  gint8    use_cpu_accel;
  gint8    use_opencl;
  gint8    export_profile;
  gint8    export_exif;
  gint8    export_xmp;
  gint8    export_iptc;
  gint32   gdisp_ID;
  gchar   *app_name;
  gchar   *wm_class;
  gchar   *display_name;
  gint32   monitor_number;
  guint32  timestamp;
  gchar   *icon_theme_dir;
  guint64  tile_cache_size;
  gchar   *swap_path;
  gint32   num_processors;
};

struct _GPTileReq
{
  gint32   drawable_ID;
  guint32  tile_num;
  guint32  shadow;
};

struct _GPTileData
{
  gint32   drawable_ID;
  guint32  tile_num;
  guint32  shadow;
  guint32  bpp;
  guint32  width;
  guint32  height;
  guint32  use_shm;
  guchar  *data;
};

struct _GPParamArray
{
  guint32  size;
  guint8  *data;
};

struct _GPParamStringArray
{
  guint32   size;
  gchar   **data;
};

struct _GPParam
{
  GPParamType  param_type;
  gchar       *type_name;

  union
  {
    gint32              d_int;
    gdouble             d_float;
    gchar              *d_string;
    GimpRGB             d_color;
    GimpParasite        d_parasite;
    GPParamArray        d_array;
    GPParamStringArray  d_string_array;
  } data;
};

struct _GPParamDefInt
{
  gint32 min_val;
  gint32 max_val;
  gint32 default_val;
};

struct _GPParamDefUnit
{
  gint32 allow_pixels;
  gint32 allow_percent;
  gint32 default_val;
};

struct _GPParamDefEnum
{
  gchar  *type_name;
  gint32  default_val;
};

struct _GPParamDefBoolean
{
  gint32 default_val;
};

struct _GPParamDefFloat
{
  gdouble min_val;
  gdouble max_val;
  gdouble default_val;
};

struct _GPParamDefString
{
  gint32  allow_non_utf8;
  gint32  null_ok;
  gint32  non_empty;
  gchar  *default_val;
};

struct _GPParamDefColor
{
  gint32  has_alpha;
  GimpRGB default_val;
};

struct _GPParamDefID
{
  gint32 none_ok;
};

struct _GPParamDef
{
  GPParamDefType  param_def_type;
  gchar          *type_name;
  gchar          *name;
  gchar          *nick;
  gchar          *blurb;

  union
  {
    GPParamDefInt     m_int;
    GPParamDefUnit    m_unit;
    GPParamDefEnum    m_enum;
    GPParamDefBoolean m_boolean;
    GPParamDefFloat   m_float;
    GPParamDefString  m_string;
    GPParamDefColor   m_color;
    GPParamDefID      m_id;
  } meta;
};

struct _GPProcRun
{
  gchar   *name;
  guint32  nparams;
  GPParam *params;
};

struct _GPProcReturn
{
  gchar   *name;
  guint32  nparams;
  GPParam *params;
};

struct _GPProcInstall
{
  gchar      *name;
  gchar      *blurb;
  gchar      *help;
  gchar      *author;
  gchar      *copyright;
  gchar      *date;
  gchar      *menu_label;
  gchar      *image_types;
  guint32     type;
  guint32     nparams;
  guint32     nreturn_vals;
  GPParamDef *params;
  GPParamDef *return_vals;
};

struct _GPProcUninstall
{
  gchar *name;
};


void      gp_init                   (void);
void      gp_lock                   (void);
void      gp_unlock                 (void);

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
