/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#include "gimpenums.h"
#include "gimpprotocol.h"
#include "gimpwire.h"
#include "parasite.h"
#include "parasiteP.h"
#include <stdio.h>


static void _gp_quit_read                (int fd, WireMessage *msg);
static void _gp_quit_write               (int fd, WireMessage *msg);
static void _gp_quit_destroy             (WireMessage *msg);
static void _gp_config_read              (int fd, WireMessage *msg);
static void _gp_config_write             (int fd, WireMessage *msg);
static void _gp_config_destroy           (WireMessage *msg);
static void _gp_tile_req_read            (int fd, WireMessage *msg);
static void _gp_tile_req_write           (int fd, WireMessage *msg);
static void _gp_tile_req_destroy         (WireMessage *msg);
static void _gp_tile_ack_read            (int fd, WireMessage *msg);
static void _gp_tile_ack_write           (int fd, WireMessage *msg);
static void _gp_tile_ack_destroy         (WireMessage *msg);
static void _gp_tile_data_read           (int fd, WireMessage *msg);
static void _gp_tile_data_write          (int fd, WireMessage *msg);
static void _gp_tile_data_destroy        (WireMessage *msg);
static void _gp_proc_run_read            (int fd, WireMessage *msg);
static void _gp_proc_run_write           (int fd, WireMessage *msg);
static void _gp_proc_run_destroy         (WireMessage *msg);
static void _gp_proc_return_read         (int fd, WireMessage *msg);
static void _gp_proc_return_write        (int fd, WireMessage *msg);
static void _gp_proc_return_destroy      (WireMessage *msg);
static void _gp_temp_proc_run_read       (int fd, WireMessage *msg);
static void _gp_temp_proc_run_write      (int fd, WireMessage *msg);
static void _gp_temp_proc_run_destroy    (WireMessage *msg);
static void _gp_temp_proc_return_read    (int fd, WireMessage *msg);
static void _gp_temp_proc_return_write   (int fd, WireMessage *msg);
static void _gp_temp_proc_return_destroy (WireMessage *msg);
static void _gp_proc_install_read        (int fd, WireMessage *msg);
static void _gp_proc_install_write       (int fd, WireMessage *msg);
static void _gp_proc_install_destroy     (WireMessage *msg);
static void _gp_proc_uninstall_read      (int fd, WireMessage *msg);
static void _gp_proc_uninstall_write     (int fd, WireMessage *msg);
static void _gp_proc_uninstall_destroy   (WireMessage *msg);
static void _gp_extension_ack_read       (int fd, WireMessage *msg);
static void _gp_extension_ack_write      (int fd, WireMessage *msg);
static void _gp_extension_ack_destroy    (WireMessage *msg);
static void _gp_params_read              (int fd, GPParam **params, guint *nparams);
static void _gp_params_write             (int fd, GPParam *params, int nparams);
       void _gp_params_destroy           (GPParam *params, int nparams);


void
gp_init ()
{
  wire_register (GP_QUIT,
		 _gp_quit_read,
		 _gp_quit_write,
		 _gp_quit_destroy);
  wire_register (GP_CONFIG,
		 _gp_config_read,
		 _gp_config_write,
		 _gp_config_destroy);
  wire_register (GP_TILE_REQ,
		 _gp_tile_req_read,
		 _gp_tile_req_write,
		 _gp_tile_req_destroy);
  wire_register (GP_TILE_ACK,
		 _gp_tile_ack_read,
		 _gp_tile_ack_write,
		 _gp_tile_ack_destroy);
  wire_register (GP_TILE_DATA,
		 _gp_tile_data_read,
		 _gp_tile_data_write,
		 _gp_tile_data_destroy);
  wire_register (GP_PROC_RUN,
		 _gp_proc_run_read,
		 _gp_proc_run_write,
		 _gp_proc_run_destroy);
  wire_register (GP_PROC_RETURN,
		 _gp_proc_return_read,
		 _gp_proc_return_write,
		 _gp_proc_return_destroy);
  wire_register (GP_TEMP_PROC_RUN,
		 _gp_temp_proc_run_read,
		 _gp_temp_proc_run_write,
		 _gp_temp_proc_run_destroy);
  wire_register (GP_TEMP_PROC_RETURN,
		 _gp_temp_proc_return_read,
		 _gp_temp_proc_return_write,
		 _gp_temp_proc_return_destroy);
  wire_register (GP_PROC_INSTALL,
		 _gp_proc_install_read,
		 _gp_proc_install_write,
		 _gp_proc_install_destroy);
  wire_register (GP_PROC_UNINSTALL,
		 _gp_proc_uninstall_read,
		 _gp_proc_uninstall_write,
		 _gp_proc_uninstall_destroy);
  wire_register (GP_EXTENSION_ACK,
		 _gp_extension_ack_read,
		 _gp_extension_ack_write,
		 _gp_extension_ack_destroy);
}

int
gp_quit_write (int fd)
{
  WireMessage msg;

  msg.type = GP_QUIT;
  msg.data = NULL;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_config_write (int       fd,
		 GPConfig *config)
{
  WireMessage msg;

  msg.type = GP_CONFIG;
  msg.data = config;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_tile_req_write (int        fd,
		   GPTileReq *tile_req)
{
  WireMessage msg;

  msg.type = GP_TILE_REQ;
  msg.data = tile_req;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_tile_ack_write (int fd)
{
  WireMessage msg;

  msg.type = GP_TILE_ACK;
  msg.data = NULL;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_tile_data_write (int         fd,
		    GPTileData *tile_data)
{
  WireMessage msg;

  msg.type = GP_TILE_DATA;
  msg.data = tile_data;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_proc_run_write (int        fd,
		   GPProcRun *proc_run)
{
  WireMessage msg;

  msg.type = GP_PROC_RUN;
  msg.data = proc_run;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_proc_return_write (int           fd,
		      GPProcReturn *proc_return)
{
  WireMessage msg;

  msg.type = GP_PROC_RETURN;
  msg.data = proc_return;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_temp_proc_run_write (int        fd,
			GPProcRun *proc_run)
{
  WireMessage msg;

  msg.type = GP_TEMP_PROC_RUN;
  msg.data = proc_run;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_temp_proc_return_write (int           fd,
			   GPProcReturn *proc_return)
{
  WireMessage msg;

  msg.type = GP_TEMP_PROC_RETURN;
  msg.data = proc_return;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_proc_install_write (int            fd,
		       GPProcInstall *proc_install)
{
  WireMessage msg;

  msg.type = GP_PROC_INSTALL;
  msg.data = proc_install;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_proc_uninstall_write (int              fd,
			 GPProcUninstall *proc_uninstall)
{
  WireMessage msg;

  msg.type = GP_PROC_UNINSTALL;
  msg.data = proc_uninstall;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}

int
gp_extension_ack_write (int fd)
{
  WireMessage msg;

  msg.type = GP_EXTENSION_ACK;
  msg.data = NULL;

  if (!wire_write_msg (fd, &msg))
    return FALSE;
  if (!wire_flush (fd))
    return FALSE;
  return TRUE;
}


static void
_gp_quit_read (int fd, WireMessage *msg)
{
}

static void
_gp_quit_write (int fd, WireMessage *msg)
{
}

static void
_gp_quit_destroy (WireMessage *msg)
{
}

static void
_gp_config_read (int fd, WireMessage *msg)
{
  GPConfig *config;

  config = g_new (GPConfig, 1);
  if (!wire_read_int32 (fd, &config->version, 1))
    return;
  if (!wire_read_int32 (fd, &config->tile_width, 1))
    return;
  if (!wire_read_int32 (fd, &config->tile_height, 1))
    return;
  if (!wire_read_int32 (fd, (guint32*) &config->shm_ID, 1))
    return;
  if (!wire_read_double (fd, &config->gamma, 1))
    return;
  if (!wire_read_int8 (fd, (guint8*) &config->install_cmap, 1))
    return;
  if (!wire_read_int8 (fd, (guint8*) config->color_cube, 3))
    return;
  if (!wire_read_int32 (fd, (guint32*) &config->gdisp_ID, 1))
    return;

  msg->data = config;
}

static void
_gp_config_write (int fd, WireMessage *msg)
{
  GPConfig *config;

  config = msg->data;
  if (!wire_write_int32 (fd, &config->version, 1))
    return;
  if (!wire_write_int32 (fd, &config->tile_width, 1))
    return;
  if (!wire_write_int32 (fd, &config->tile_height, 1))
    return;
  if (!wire_write_int32 (fd, (guint32*) &config->shm_ID, 1))
    return;
  if (!wire_write_double (fd, &config->gamma, 1))
    return;
  if (!wire_write_int8 (fd, (guint8*) &config->install_cmap, 1))
    return;
  if (!wire_write_int8 (fd, (guint8*) config->color_cube, 3))
    return;
  if (!wire_write_int32 (fd, (guint32*) &config->gdisp_ID, 1))
    return;
}

static void
_gp_config_destroy (WireMessage *msg)
{
  g_free (msg->data);
}

static void
_gp_tile_req_read (int fd, WireMessage *msg)
{
  GPTileReq *tile_req;

  tile_req = g_new (GPTileReq, 1);
  if (!wire_read_int32 (fd, (guint32*) &tile_req->drawable_ID, 1))
    return;
  if (!wire_read_int32 (fd, &tile_req->tile_num, 1))
    return;
  if (!wire_read_int32 (fd, &tile_req->shadow, 1))
    return;

  msg->data = tile_req;
}

static void
_gp_tile_req_write (int fd, WireMessage *msg)
{
  GPTileReq *tile_req;

  tile_req = msg->data;
  if (!wire_write_int32 (fd, (guint32*) &tile_req->drawable_ID, 1))
    return;
  if (!wire_write_int32 (fd, &tile_req->tile_num, 1))
    return;
  if (!wire_write_int32 (fd, &tile_req->shadow, 1))
    return;
}

static void
_gp_tile_req_destroy (WireMessage *msg)
{
  g_free (msg->data);
}

static void
_gp_tile_ack_read (int fd, WireMessage *msg)
{
}

static void
_gp_tile_ack_write (int fd, WireMessage *msg)
{
}

static void
_gp_tile_ack_destroy (WireMessage *msg)
{
}

static void
_gp_tile_data_read (int fd, WireMessage *msg)
{
  GPTileData *tile_data;
  guint length;

  tile_data = g_new (GPTileData, 1);
  if (!wire_read_int32 (fd, (guint32*) &tile_data->drawable_ID, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->tile_num, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->shadow, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->bpp, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->width, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->height, 1))
    return;
  if (!wire_read_int32 (fd, &tile_data->use_shm, 1))
    return;
  tile_data->data = NULL;

  if (!tile_data->use_shm)
    {
      length = tile_data->width * tile_data->height * tile_data->bpp;
      tile_data->data = g_new (guchar, length);
      if (!wire_read_int8 (fd, (guint8*) tile_data->data, length))
	return;
    }

  msg->data = tile_data;
}

static void
_gp_tile_data_write (int fd, WireMessage *msg)
{
  GPTileData *tile_data;
  guint length;

  tile_data = msg->data;
  if (!wire_write_int32 (fd, (guint32*) &tile_data->drawable_ID, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->tile_num, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->shadow, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->bpp, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->width, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->height, 1))
    return;
  if (!wire_write_int32 (fd, &tile_data->use_shm, 1))
    return;

  if (!tile_data->use_shm)
    {
      length = tile_data->width * tile_data->height * tile_data->bpp;
      if (!wire_write_int8 (fd, (guint8*) tile_data->data, length))
	return;
    }
}

static void
_gp_tile_data_destroy (WireMessage *msg)
{
  GPTileData *tile_data;

  tile_data = msg->data;
  if (tile_data->data)
    g_free (tile_data->data);
  g_free (tile_data);
}

static void
_gp_proc_run_read (int fd, WireMessage *msg)
{
  GPProcRun *proc_run;

  proc_run = g_new (GPProcRun, 1);
  if (!wire_read_string (fd, &proc_run->name, 1))
    {
      g_free (proc_run);
      return;
    }
  _gp_params_read (fd, &proc_run->params, (guint*) &proc_run->nparams);

  msg->data = proc_run;
}

static void
_gp_proc_run_write (int fd, WireMessage *msg)
{
  GPProcRun *proc_run;

  proc_run = msg->data;

  if (!wire_write_string (fd, &proc_run->name, 1))
    return;
  _gp_params_write (fd, proc_run->params, proc_run->nparams);
}

static void
_gp_proc_run_destroy (WireMessage *msg)
{
  GPProcRun *proc_run;

  proc_run = msg->data;
  _gp_params_destroy (proc_run->params, proc_run->nparams);
  g_free (proc_run->name);
  g_free (proc_run);
}

static void
_gp_proc_return_read (int fd, WireMessage *msg)
{
  GPProcReturn *proc_return;

  proc_return = g_new (GPProcReturn, 1);
  if (!wire_read_string (fd, &proc_return->name, 1))
    return;
  _gp_params_read (fd, &proc_return->params, (guint*) &proc_return->nparams);

  msg->data = proc_return;
}

static void
_gp_proc_return_write (int fd, WireMessage *msg)
{
  GPProcReturn *proc_return;

  proc_return = msg->data;

  if (!wire_write_string (fd, &proc_return->name, 1))
    return;
  _gp_params_write (fd, proc_return->params, proc_return->nparams);
}

static void
_gp_proc_return_destroy (WireMessage *msg)
{
  GPProcReturn *proc_return;

  proc_return = msg->data;
  _gp_params_destroy (proc_return->params, proc_return->nparams);
  g_free (proc_return);
}

static void
_gp_temp_proc_run_read (int fd, WireMessage *msg)
{
  _gp_proc_run_read (fd, msg);
}

static void
_gp_temp_proc_run_write (int fd, WireMessage *msg)
{
  _gp_proc_run_write (fd, msg);
}

static void
_gp_temp_proc_run_destroy (WireMessage *msg)
{
  _gp_proc_run_destroy (msg);
}

static void
_gp_temp_proc_return_read (int fd, WireMessage *msg)
{
  _gp_proc_return_read (fd, msg);
}

static void
_gp_temp_proc_return_write (int fd, WireMessage *msg)
{
  _gp_proc_return_write (fd, msg);
}

static void
_gp_temp_proc_return_destroy (WireMessage *msg)
{
  _gp_proc_return_destroy (msg);
}

static void
_gp_proc_install_read (int fd, WireMessage *msg)
{
  GPProcInstall *proc_install;
  int i;

  proc_install = g_new (GPProcInstall, 1);

  if (!wire_read_string (fd, &proc_install->name, 1))
    return;
  if (!wire_read_string (fd, &proc_install->blurb, 1))
    return;
  if (!wire_read_string (fd, &proc_install->help, 1))
    return;
  if (!wire_read_string (fd, &proc_install->author, 1))
    return;
  if (!wire_read_string (fd, &proc_install->copyright, 1))
    return;
  if (!wire_read_string (fd, &proc_install->date, 1))
    return;
  if (!wire_read_string (fd, &proc_install->menu_path, 1))
    return;
  if (!wire_read_string (fd, &proc_install->image_types, 1))
    return;

  if (!wire_read_int32 (fd, &proc_install->type, 1))
    return;
  if (!wire_read_int32 (fd, &proc_install->nparams, 1))
    return;
  if (!wire_read_int32 (fd, &proc_install->nreturn_vals, 1))
    return;

  proc_install->params = g_new (GPParamDef, proc_install->nparams);
  proc_install->return_vals = g_new (GPParamDef, proc_install->nreturn_vals);

  for (i = 0; i < proc_install->nparams; i++)
    {
      if (!wire_read_int32 (fd, (guint32*) &proc_install->params[i].type, 1))
	return;
      if (!wire_read_string (fd, &proc_install->params[i].name, 1))
	return;
      if (!wire_read_string (fd, &proc_install->params[i].description, 1))
	return;
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (!wire_read_int32 (fd, (guint32*) &proc_install->return_vals[i].type, 1))
	return;
      if (!wire_read_string (fd, &proc_install->return_vals[i].name, 1))
	return;
      if (!wire_read_string (fd, &proc_install->return_vals[i].description, 1))
	return;
    }

  msg->data = proc_install;
}

static void
_gp_proc_install_write (int fd, WireMessage *msg)
{
  GPProcInstall *proc_install;
  int i;

  proc_install = msg->data;

  if (!wire_write_string (fd, &proc_install->name, 1))
    return;
  if (!wire_write_string (fd, &proc_install->blurb, 1))
    return;
  if (!wire_write_string (fd, &proc_install->help, 1))
    return;
  if (!wire_write_string (fd, &proc_install->author, 1))
    return;
  if (!wire_write_string (fd, &proc_install->copyright, 1))
    return;
  if (!wire_write_string (fd, &proc_install->date, 1))
    return;
  if (!wire_write_string (fd, &proc_install->menu_path, 1))
    return;
  if (!wire_write_string (fd, &proc_install->image_types, 1))
    return;

  if (!wire_write_int32 (fd, &proc_install->type, 1))
    return;
  if (!wire_write_int32 (fd, &proc_install->nparams, 1))
    return;
  if (!wire_write_int32 (fd, &proc_install->nreturn_vals, 1))
    return;

  for (i = 0; i < proc_install->nparams; i++)
    {
      if (!wire_write_int32 (fd, (guint32*) &proc_install->params[i].type, 1))
	return;
      if (!wire_write_string (fd, &proc_install->params[i].name, 1))
	return;
      if (!wire_write_string (fd, &proc_install->params[i].description, 1))
	return;
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      if (!wire_write_int32 (fd, (guint32*) &proc_install->return_vals[i].type, 1))
	return;
      if (!wire_write_string (fd, &proc_install->return_vals[i].name, 1))
	return;
      if (!wire_write_string (fd, &proc_install->return_vals[i].description, 1))
	return;
    }
}

static void
_gp_proc_install_destroy (WireMessage *msg)
{
  GPProcInstall *proc_install;
  int i;

  proc_install = msg->data;

  g_free (proc_install->name);
  g_free (proc_install->blurb);
  g_free (proc_install->help);
  g_free (proc_install->author);
  g_free (proc_install->copyright);
  g_free (proc_install->date);
  g_free (proc_install->menu_path);
  g_free (proc_install->image_types);

  for (i = 0; i < proc_install->nparams; i++)
    {
      g_free (proc_install->params[i].name);
      g_free (proc_install->params[i].description);
    }

  for (i = 0; i < proc_install->nreturn_vals; i++)
    {
      g_free (proc_install->return_vals[i].name);
      g_free (proc_install->return_vals[i].description);
    }

  g_free (proc_install->params);
  g_free (proc_install->return_vals);
  g_free (proc_install);
}

static void
_gp_proc_uninstall_read (int fd, WireMessage *msg)
{
  GPProcUninstall *proc_uninstall;

  proc_uninstall = g_new (GPProcUninstall, 1);

  if (!wire_read_string (fd, &proc_uninstall->name, 1))
    return;

  msg->data = proc_uninstall;
}

static void
_gp_proc_uninstall_write (int fd, WireMessage *msg)
{
  GPProcUninstall *proc_uninstall;

  proc_uninstall = msg->data;

  if (!wire_write_string (fd, &proc_uninstall->name, 1))
    return;
}

static void
_gp_proc_uninstall_destroy (WireMessage *msg)
{
  GPProcUninstall *proc_uninstall;

  proc_uninstall = msg->data;

  g_free (proc_uninstall->name);
  g_free (proc_uninstall);
}

static void
_gp_params_read (int fd, GPParam **params, guint *nparams)
{
  int i;

  if (!wire_read_int32 (fd, (guint32*) nparams, 1))
    return;

  if (*nparams == 0)
    {
      *params = NULL;
      return;
    }

  *params = g_new (GPParam, *nparams);

  for (i = 0; i < *nparams; i++)
    {
      if (!wire_read_int32 (fd, (guint32*) &(*params)[i].type, 1))
	return;

      switch ((*params)[i].type)
	{
	case PARAM_INT32:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_int32, 1))
	    return;
	  break;
	case PARAM_INT16:
	  if (!wire_read_int16 (fd, (guint16*) &(*params)[i].data.d_int16, 1))
	    return;
	  break;
	case PARAM_INT8:
	  if (!wire_read_int8 (fd, (guint8*) &(*params)[i].data.d_int8, 1))
	    return;
	  break;
        case PARAM_FLOAT:
	  if (!wire_read_double (fd, &(*params)[i].data.d_float, 1))
	    return;
          break;
        case PARAM_STRING:
	  if (!wire_read_string (fd, &(*params)[i].data.d_string, 1))
	    return;
          break;
        case PARAM_INT32ARRAY:
	  (*params)[i].data.d_int32array = g_new (gint32, (*params)[i-1].data.d_int32);
	  if (!wire_read_int32 (fd, (guint32*) (*params)[i].data.d_int32array,
				(*params)[i-1].data.d_int32))
	    return;
          break;
        case PARAM_INT16ARRAY:
	  (*params)[i].data.d_int16array = g_new (gint16, (*params)[i-1].data.d_int32);
	  if (!wire_read_int16 (fd, (guint16*) (*params)[i].data.d_int16array,
				(*params)[i-1].data.d_int32))
	    return;
          break;
        case PARAM_INT8ARRAY:
	  (*params)[i].data.d_int8array = g_new (gint8, (*params)[i-1].data.d_int32);
	  if (!wire_read_int8 (fd, (guint8*) (*params)[i].data.d_int8array,
			       (*params)[i-1].data.d_int32))
	    return;
          break;
        case PARAM_FLOATARRAY:
	  (*params)[i].data.d_floatarray = g_new (gdouble, (*params)[i-1].data.d_int32);
	  if (!wire_read_double (fd, (*params)[i].data.d_floatarray,
				 (*params)[i-1].data.d_int32))
	    return;
          break;
        case PARAM_STRINGARRAY:
	  (*params)[i].data.d_stringarray = g_new (gchar*, (*params)[i-1].data.d_int32);
	  if (!wire_read_string (fd, (*params)[i].data.d_stringarray,
				 (*params)[i-1].data.d_int32))
	    return;
          break;
        case PARAM_COLOR:
	  if (!wire_read_int8 (fd, (guint8*) &(*params)[i].data.d_color.red, 1))
	    return;
	  if (!wire_read_int8 (fd, (guint8*) &(*params)[i].data.d_color.green, 1))
	    return;
	  if (!wire_read_int8 (fd, (guint8*) &(*params)[i].data.d_color.blue, 1))
	    return;
          break;
        case PARAM_REGION:
          break;
        case PARAM_DISPLAY:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_display, 1))
	    return;
          break;
        case PARAM_IMAGE:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_image, 1))
	    return;
          break;
        case PARAM_LAYER:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_layer, 1))
	    return;
          break;
        case PARAM_CHANNEL:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_channel, 1))
	    return;
          break;
        case PARAM_DRAWABLE:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_drawable, 1))
	    return;
          break;
        case PARAM_SELECTION:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_selection, 1))
	    return;
          break;
        case PARAM_BOUNDARY:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_boundary, 1))
	    return;
          break;
        case PARAM_PATH:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_path, 1))
	    return;
          break;
        case PARAM_PARASITE:
	{
	  if (!wire_read_string (fd, &(*params)[i].data.d_parasite.name, 1))
	    return;
	  if ((*params)[i].data.d_parasite.name == NULL)
	  { /* we have a null parasite */
	    (*params)[i].data.d_parasite.data = NULL;
	    break;
	  }
	  if (!wire_read_int32 (fd, &((*params)[i].data.d_parasite.flags), 1))
	    return;
	  if (!wire_read_int32 (fd, &((*params)[i].data.d_parasite.size), 1))
	    return;
	  if ((*params)[i].data.d_parasite.size > 0)
	  {
	    (*params)[i].data.d_parasite.data = g_malloc((*params)[i].data.d_parasite.size);
	    if (!wire_read_int8 (fd, (*params)[i].data.d_parasite.data,
				 (*params)[i].data.d_parasite.size))
	    {
	      g_free((*params)[i].data.d_parasite.data);
	      (*params)[i].data.d_parasite.data = NULL;
	      return;
	    }
	  }
	  else
	    (*params)[i].data.d_parasite.data = NULL;
	} break;
        case PARAM_STATUS:
	  if (!wire_read_int32 (fd, (guint32*) &(*params)[i].data.d_status, 1))
	    return;
          break;
	case PARAM_END:
	  break;
	}
    }
}

static void
_gp_params_write (int fd, GPParam *params, int nparams)
{
  int i;

  if (!wire_write_int32 (fd, (guint32*) &nparams, 1))
    return;

  for (i = 0; i < nparams; i++)
    {
      if (!wire_write_int32 (fd, (guint32*) &params[i].type, 1))
	return;

      switch (params[i].type)
	{
	case PARAM_INT32:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_int32, 1))
	    return;
	  break;
	case PARAM_INT16:
	  if (!wire_write_int16 (fd, (guint16*) &params[i].data.d_int16, 1))
	    return;
	  break;
	case PARAM_INT8:
	  if (!wire_write_int8 (fd, (guint8*) &params[i].data.d_int8, 1))
	    return;
	  break;
        case PARAM_FLOAT:
	  if (!wire_write_double (fd, &params[i].data.d_float, 1))
	    return;
          break;
        case PARAM_STRING:
	  if (!wire_write_string (fd, &params[i].data.d_string, 1))
	    return;
          break;
        case PARAM_INT32ARRAY:
	  if (!wire_write_int32 (fd, (guint32*) params[i].data.d_int32array,
				 params[i-1].data.d_int32))
	    return;
          break;
        case PARAM_INT16ARRAY:
	  if (!wire_write_int16 (fd, (guint16*) params[i].data.d_int16array,
				 params[i-1].data.d_int32))
	    return;
          break;
        case PARAM_INT8ARRAY:
	  if (!wire_write_int8 (fd, (guint8*) params[i].data.d_int8array,
				params[i-1].data.d_int32))
	    return;
          break;
        case PARAM_FLOATARRAY:
	  if (!wire_write_double (fd, params[i].data.d_floatarray,
				  params[i-1].data.d_int32))
	    return;
          break;
        case PARAM_STRINGARRAY:
	  if (!wire_write_string (fd, params[i].data.d_stringarray,
				  params[i-1].data.d_int32))
	    return;
          break;
        case PARAM_COLOR:
	  if (!wire_write_int8 (fd, (guint8*) &params[i].data.d_color.red, 1))
	    return;
	  if (!wire_write_int8 (fd, (guint8*) &params[i].data.d_color.green, 1))
	    return;
	  if (!wire_write_int8 (fd, (guint8*) &params[i].data.d_color.blue, 1))
	    return;
          break;
        case PARAM_REGION:
          break;
        case PARAM_DISPLAY:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_display, 1))
	    return;
          break;
        case PARAM_IMAGE:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_image, 1))
	    return;
          break;
        case PARAM_LAYER:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_layer, 1))
	    return;
          break;
        case PARAM_CHANNEL:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_channel, 1))
	    return;
          break;
        case PARAM_DRAWABLE:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_drawable, 1))
	    return;
          break;
        case PARAM_SELECTION:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_selection, 1))
	    return;
          break;
        case PARAM_BOUNDARY:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_boundary, 1))
	    return;
          break;
        case PARAM_PATH:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_path, 1))
	    return;
          break;
        case PARAM_PARASITE:
	{
	  Parasite *p = (Parasite *)&params[i].data.d_parasite;
	  if (p->name == NULL)
	  { /* write a null string to signifly a null parasite */
	    wire_write_string (fd,  &p->name, 1);
	    break;
	  }
	  if (!wire_write_string (fd, &p->name, 1))
	    return;
	  if (!wire_write_int32 (fd, &p->flags, 1))
	    return;
	  if (!wire_write_int32 (fd, &p->size, 1))
	    return;
	  if (p->size > 0)
	  {
	    if (!wire_write_int8 (fd, p->data, p->size))
	      return;
	  }
	} break;
        case PARAM_STATUS:
	  if (!wire_write_int32 (fd, (guint32*) &params[i].data.d_status, 1))
	    return;
          break;
	case PARAM_END:
	  break;
	}
    }
}

static void
_gp_extension_ack_read (int fd, WireMessage *msg)
{
}

static void
_gp_extension_ack_write (int fd, WireMessage *msg)
{
}

static void
_gp_extension_ack_destroy (WireMessage *msg)
{
}

void
_gp_params_destroy (GPParam *params, int nparams)
{
  int count;
  int i, j;

  for (i = 0; i < nparams; i++)
    {
      switch (params[i].type)
	{
	case PARAM_INT32:
	case PARAM_INT16:
	case PARAM_INT8:
	case PARAM_FLOAT:
	case PARAM_COLOR:
	case PARAM_REGION:
	case PARAM_DISPLAY:
	case PARAM_IMAGE:
	case PARAM_LAYER:
	case PARAM_CHANNEL:
	case PARAM_DRAWABLE:
	case PARAM_SELECTION:
	case PARAM_BOUNDARY:
	case PARAM_PATH:
	case PARAM_STATUS:
	  break;

	case PARAM_STRING:
	  g_free (params[i].data.d_string);
	  break;
	case PARAM_INT32ARRAY:
	  g_free (params[i].data.d_int32array);
	  break;
	case PARAM_INT16ARRAY:
	  g_free (params[i].data.d_int16array);
	  break;
	case PARAM_INT8ARRAY:
	  g_free (params[i].data.d_int8array);
	  break;
	case PARAM_FLOATARRAY:
	  g_free (params[i].data.d_floatarray);
	  break;
	case PARAM_STRINGARRAY:
	  if ((i > 0) && (params[i-1].type == PARAM_INT32))
	    {
	      count = params[i-1].data.d_int32;
	      for (j = 0; j < count; j++)
		g_free (params[i].data.d_stringarray[j]);
	      g_free (params[i].data.d_stringarray);
	    }
	  break;
	case PARAM_PARASITE:
	  if (params[i].data.d_parasite.name)
	    g_free(params[i].data.d_parasite.name);
	  if (params[i].data.d_parasite.data)
	    g_free(params[i].data.d_parasite.data);
	  break;
	case PARAM_END:
	  break;
	}
    }
  g_free (params);
}
