/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BASE_ENUMS_H__
#define __GIMP_BASE_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_CHECK_SIZE (gimp_check_size_get_type ())

GType gimp_check_size_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHECK_SIZE_SMALL_CHECKS  = 0,  /*< desc="Small"  >*/
  GIMP_CHECK_SIZE_MEDIUM_CHECKS = 1,  /*< desc="Medium" >*/
  GIMP_CHECK_SIZE_LARGE_CHECKS  = 2   /*< desc="Large"  >*/
} GimpCheckSize;


#define GIMP_TYPE_CHECK_TYPE (gimp_check_type_get_type ())

GType gimp_check_type_get_type (void) G_GNUC_CONST;

typedef enum  /*< pdb-skip >*/
{
  GIMP_CHECK_TYPE_LIGHT_CHECKS = 0,  /*< desc="Light Checks"    >*/
  GIMP_CHECK_TYPE_GRAY_CHECKS  = 1,  /*< desc="Mid-Tone Checks" >*/
  GIMP_CHECK_TYPE_DARK_CHECKS  = 2,  /*< desc="Dark Checks"     >*/
  GIMP_CHECK_TYPE_WHITE_ONLY   = 3,  /*< desc="White Only"      >*/
  GIMP_CHECK_TYPE_GRAY_ONLY    = 4,  /*< desc="Gray Only"       >*/
  GIMP_CHECK_TYPE_BLACK_ONLY   = 5   /*< desc="Black Only"      >*/
} GimpCheckType;


#define GIMP_TYPE_IMAGE_BASE_TYPE (gimp_image_base_type_get_type ())

GType gimp_image_base_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB,     /*< desc="RGB color"     >*/
  GIMP_GRAY,    /*< desc="Grayscale"     >*/
  GIMP_INDEXED  /*< desc="Indexed color" >*/
} GimpImageBaseType;


#define GIMP_TYPE_IMAGE_TYPE (gimp_image_type_get_type ())

GType gimp_image_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_RGB_IMAGE,      /*< desc="RGB"             >*/
  GIMP_RGBA_IMAGE,     /*< desc="RGB-alpha"       >*/
  GIMP_GRAY_IMAGE,     /*< desc="Grayscale"       >*/
  GIMP_GRAYA_IMAGE,    /*< desc="Grayscale-alpha" >*/
  GIMP_INDEXED_IMAGE,  /*< desc="Indexed"         >*/
  GIMP_INDEXEDA_IMAGE  /*< desc="Indexed-alpha"   >*/
} GimpImageType;


typedef enum /*< skip >*/
{
  GIMP_UNIT_PIXEL   = 0,

  GIMP_UNIT_INCH    = 1,
  GIMP_UNIT_MM      = 2,
  GIMP_UNIT_POINT   = 3,
  GIMP_UNIT_PICA    = 4,

  GIMP_UNIT_END     = 5,

  GIMP_UNIT_PERCENT = 65536 /*< pdb-skip >*/
} GimpUnit;


#define GIMP_TYPE_PDB_ARG_TYPE (gimp_pdb_arg_type_get_type ())

GType gimp_pdb_arg_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_INT32,
  GIMP_PDB_INT16,
  GIMP_PDB_INT8,
  GIMP_PDB_FLOAT,
  GIMP_PDB_STRING,
  GIMP_PDB_INT32ARRAY,
  GIMP_PDB_INT16ARRAY,
  GIMP_PDB_INT8ARRAY,
  GIMP_PDB_FLOATARRAY,
  GIMP_PDB_STRINGARRAY,
  GIMP_PDB_COLOR,
  GIMP_PDB_REGION,
  GIMP_PDB_DISPLAY,
  GIMP_PDB_IMAGE,
  GIMP_PDB_LAYER,
  GIMP_PDB_CHANNEL,
  GIMP_PDB_DRAWABLE,
  GIMP_PDB_SELECTION,
  GIMP_PDB_BOUNDARY,
  GIMP_PDB_PATH,
  GIMP_PDB_PARASITE,
  GIMP_PDB_STATUS,
  GIMP_PDB_END
} GimpPDBArgType;


#define GIMP_TYPE_PDB_PROC_TYPE (gimp_pdb_proc_type_get_type ())

GType gimp_pdb_proc_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_INTERNAL,
  GIMP_PLUGIN,
  GIMP_EXTENSION,
  GIMP_TEMPORARY
} GimpPDBProcType;


#define GIMP_TYPE_PDB_STATUS_TYPE (gimp_pdb_status_type_get_type ())

GType gimp_pdb_status_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PDB_EXECUTION_ERROR,
  GIMP_PDB_CALLING_ERROR,
  GIMP_PDB_PASS_THROUGH,
  GIMP_PDB_SUCCESS,
  GIMP_PDB_CANCEL
} GimpPDBStatusType;


#define GIMP_TYPE_MESSAGE_HANDLER_TYPE (gimp_message_handler_type_get_type ())

GType gimp_message_handler_type_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_MESSAGE_BOX,
  GIMP_CONSOLE,
  GIMP_ERROR_CONSOLE
} GimpMessageHandlerType;


#define GIMP_TYPE_STACK_TRACE_MODE (gimp_stack_trace_mode_get_type ())

GType gimp_stack_trace_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_STACK_TRACE_NEVER,
  GIMP_STACK_TRACE_QUERY,
  GIMP_STACK_TRACE_ALWAYS
} GimpStackTraceMode;


#define GIMP_TYPE_PROGRESS_COMMAND (gimp_progress_command_get_type ())

GType gimp_progress_command_get_type (void) G_GNUC_CONST;

typedef enum
{
  GIMP_PROGRESS_COMMAND_START,
  GIMP_PROGRESS_COMMAND_END,
  GIMP_PROGRESS_COMMAND_SET_TEXT,
  GIMP_PROGRESS_COMMAND_SET_VALUE
} GimpProgressCommand;


G_END_DECLS

#endif  /* __GIMP_BASE_ENUMS_H__ */
