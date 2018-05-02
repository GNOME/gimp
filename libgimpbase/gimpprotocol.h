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
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PROTOCOL_H__
#define __GIMP_PROTOCOL_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* Increment every time the protocol changes
 */
#define GIMP_PROTOCOL_VERSION  0x0017


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


typedef struct _GPConfig        GPConfig;
typedef struct _GPTileReq       GPTileReq;
typedef struct _GPTileAck       GPTileAck;
typedef struct _GPTileData      GPTileData;
typedef struct _GPParam         GPParam;
typedef struct _GPParamDef      GPParamDef;
typedef struct _GPProcRun       GPProcRun;
typedef struct _GPProcReturn    GPProcReturn;
typedef struct _GPProcInstall   GPProcInstall;
typedef struct _GPProcUninstall GPProcUninstall;


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
  gint8    export_exif;
  gint8    export_xmp;
  gint8    export_iptc;
  gint8    show_tooltips;
  gint32   gdisp_ID;
  gchar   *app_name;
  gchar   *wm_class;
  gchar   *display_name;
  gint32   monitor_number;
  guint32  timestamp;
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

struct _GPParam
{
  guint32 type;

  union
  {
    gint32        d_int32;
    gint16        d_int16;
    guint8        d_int8;
    gdouble       d_float;
    gchar        *d_string;
    gint32       *d_int32array;
    gint16       *d_int16array;
    guint8       *d_int8array;
    gdouble      *d_floatarray;
    gchar       **d_stringarray;
    GimpRGB      *d_colorarray;
    GimpRGB       d_color;
    gint32        d_display;
    gint32        d_image;
    gint32        d_item;
    gint32        d_layer;
    gint32        d_channel;
    gint32        d_drawable;
    gint32        d_selection;
    gint32        d_boundary;
    gint32        d_vectors;
    gint32        d_status;
    GimpParasite  d_parasite;
  } data;
};

struct _GPParamDef
{
  guint32  type;
  gchar   *name;
  gchar   *description;
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
  gchar      *menu_path;
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

void      gp_params_destroy         (GPParam         *params,
                                     gint             nparams);

void      gp_lock                   (void);
void      gp_unlock                 (void);


G_END_DECLS

#endif /* __GIMP_PROTOCOL_H__ */
