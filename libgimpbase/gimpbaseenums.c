
/* Generated data (by gimp-mkenums) */

#include "config.h"
#include <glib-object.h>
#include "gimpbasetypes.h"
#include "libgimp/libgimp-intl.h"

/* enumerations from "./gimpbaseenums.h" */
GType
gimp_check_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_SIZE_SMALL_CHECKS, "GIMP_CHECK_SIZE_SMALL_CHECKS", "small-checks" },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, "GIMP_CHECK_SIZE_MEDIUM_CHECKS", "medium-checks" },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, "GIMP_CHECK_SIZE_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHECK_SIZE_SMALL_CHECKS, N_("Small"), NULL },
    { GIMP_CHECK_SIZE_MEDIUM_CHECKS, N_("Medium"), NULL },
    { GIMP_CHECK_SIZE_LARGE_CHECKS, N_("Large"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCheckSize", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_check_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, "GIMP_CHECK_TYPE_LIGHT_CHECKS", "light-checks" },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, "GIMP_CHECK_TYPE_GRAY_CHECKS", "gray-checks" },
    { GIMP_CHECK_TYPE_DARK_CHECKS, "GIMP_CHECK_TYPE_DARK_CHECKS", "dark-checks" },
    { GIMP_CHECK_TYPE_WHITE_ONLY, "GIMP_CHECK_TYPE_WHITE_ONLY", "white-only" },
    { GIMP_CHECK_TYPE_GRAY_ONLY, "GIMP_CHECK_TYPE_GRAY_ONLY", "gray-only" },
    { GIMP_CHECK_TYPE_BLACK_ONLY, "GIMP_CHECK_TYPE_BLACK_ONLY", "black-only" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_CHECK_TYPE_LIGHT_CHECKS, N_("Light Checks"), NULL },
    { GIMP_CHECK_TYPE_GRAY_CHECKS, N_("Mid-Tone Checks"), NULL },
    { GIMP_CHECK_TYPE_DARK_CHECKS, N_("Dark Checks"), NULL },
    { GIMP_CHECK_TYPE_WHITE_ONLY, N_("White Only"), NULL },
    { GIMP_CHECK_TYPE_GRAY_ONLY, N_("Gray Only"), NULL },
    { GIMP_CHECK_TYPE_BLACK_ONLY, N_("Black Only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpCheckType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_image_base_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB, "GIMP_RGB", "rgb" },
    { GIMP_GRAY, "GIMP_GRAY", "gray" },
    { GIMP_INDEXED, "GIMP_INDEXED", "indexed" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RGB, N_("RGB color"), NULL },
    { GIMP_GRAY, N_("Grayscale"), NULL },
    { GIMP_INDEXED, N_("Indexed color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpImageBaseType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_image_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_RGB_IMAGE, "GIMP_RGB_IMAGE", "rgb-image" },
    { GIMP_RGBA_IMAGE, "GIMP_RGBA_IMAGE", "rgba-image" },
    { GIMP_GRAY_IMAGE, "GIMP_GRAY_IMAGE", "gray-image" },
    { GIMP_GRAYA_IMAGE, "GIMP_GRAYA_IMAGE", "graya-image" },
    { GIMP_INDEXED_IMAGE, "GIMP_INDEXED_IMAGE", "indexed-image" },
    { GIMP_INDEXEDA_IMAGE, "GIMP_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_RGB_IMAGE, N_("RGB"), NULL },
    { GIMP_RGBA_IMAGE, N_("RGB-alpha"), NULL },
    { GIMP_GRAY_IMAGE, N_("Grayscale"), NULL },
    { GIMP_GRAYA_IMAGE, N_("Grayscale-alpha"), NULL },
    { GIMP_INDEXED_IMAGE, N_("Indexed"), NULL },
    { GIMP_INDEXEDA_IMAGE, N_("Indexed-alpha"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpImageType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_arg_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PDB_INT32, "GIMP_PDB_INT32", "int32" },
    { GIMP_PDB_INT16, "GIMP_PDB_INT16", "int16" },
    { GIMP_PDB_INT8, "GIMP_PDB_INT8", "int8" },
    { GIMP_PDB_FLOAT, "GIMP_PDB_FLOAT", "float" },
    { GIMP_PDB_STRING, "GIMP_PDB_STRING", "string" },
    { GIMP_PDB_INT32ARRAY, "GIMP_PDB_INT32ARRAY", "int32array" },
    { GIMP_PDB_INT16ARRAY, "GIMP_PDB_INT16ARRAY", "int16array" },
    { GIMP_PDB_INT8ARRAY, "GIMP_PDB_INT8ARRAY", "int8array" },
    { GIMP_PDB_FLOATARRAY, "GIMP_PDB_FLOATARRAY", "floatarray" },
    { GIMP_PDB_STRINGARRAY, "GIMP_PDB_STRINGARRAY", "stringarray" },
    { GIMP_PDB_COLOR, "GIMP_PDB_COLOR", "color" },
    { GIMP_PDB_REGION, "GIMP_PDB_REGION", "region" },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", "display" },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", "image" },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", "layer" },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", "channel" },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", "drawable" },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", "selection" },
    { GIMP_PDB_BOUNDARY, "GIMP_PDB_BOUNDARY", "boundary" },
    { GIMP_PDB_PATH, "GIMP_PDB_PATH", "path" },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", "parasite" },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", "status" },
    { GIMP_PDB_END, "GIMP_PDB_END", "end" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_INT32, "GIMP_PDB_INT32", NULL },
    { GIMP_PDB_INT16, "GIMP_PDB_INT16", NULL },
    { GIMP_PDB_INT8, "GIMP_PDB_INT8", NULL },
    { GIMP_PDB_FLOAT, "GIMP_PDB_FLOAT", NULL },
    { GIMP_PDB_STRING, "GIMP_PDB_STRING", NULL },
    { GIMP_PDB_INT32ARRAY, "GIMP_PDB_INT32ARRAY", NULL },
    { GIMP_PDB_INT16ARRAY, "GIMP_PDB_INT16ARRAY", NULL },
    { GIMP_PDB_INT8ARRAY, "GIMP_PDB_INT8ARRAY", NULL },
    { GIMP_PDB_FLOATARRAY, "GIMP_PDB_FLOATARRAY", NULL },
    { GIMP_PDB_STRINGARRAY, "GIMP_PDB_STRINGARRAY", NULL },
    { GIMP_PDB_COLOR, "GIMP_PDB_COLOR", NULL },
    { GIMP_PDB_REGION, "GIMP_PDB_REGION", NULL },
    { GIMP_PDB_DISPLAY, "GIMP_PDB_DISPLAY", NULL },
    { GIMP_PDB_IMAGE, "GIMP_PDB_IMAGE", NULL },
    { GIMP_PDB_LAYER, "GIMP_PDB_LAYER", NULL },
    { GIMP_PDB_CHANNEL, "GIMP_PDB_CHANNEL", NULL },
    { GIMP_PDB_DRAWABLE, "GIMP_PDB_DRAWABLE", NULL },
    { GIMP_PDB_SELECTION, "GIMP_PDB_SELECTION", NULL },
    { GIMP_PDB_BOUNDARY, "GIMP_PDB_BOUNDARY", NULL },
    { GIMP_PDB_PATH, "GIMP_PDB_PATH", NULL },
    { GIMP_PDB_PARASITE, "GIMP_PDB_PARASITE", NULL },
    { GIMP_PDB_STATUS, "GIMP_PDB_STATUS", NULL },
    { GIMP_PDB_END, "GIMP_PDB_END", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBArgType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_proc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_INTERNAL, "GIMP_INTERNAL", "internal" },
    { GIMP_PLUGIN, "GIMP_PLUGIN", "plugin" },
    { GIMP_EXTENSION, "GIMP_EXTENSION", "extension" },
    { GIMP_TEMPORARY, "GIMP_TEMPORARY", "temporary" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_INTERNAL, "GIMP_INTERNAL", NULL },
    { GIMP_PLUGIN, "GIMP_PLUGIN", NULL },
    { GIMP_EXTENSION, "GIMP_EXTENSION", NULL },
    { GIMP_TEMPORARY, "GIMP_TEMPORARY", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBProcType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_pdb_status_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PDB_EXECUTION_ERROR, "GIMP_PDB_EXECUTION_ERROR", "execution-error" },
    { GIMP_PDB_CALLING_ERROR, "GIMP_PDB_CALLING_ERROR", "calling-error" },
    { GIMP_PDB_PASS_THROUGH, "GIMP_PDB_PASS_THROUGH", "pass-through" },
    { GIMP_PDB_SUCCESS, "GIMP_PDB_SUCCESS", "success" },
    { GIMP_PDB_CANCEL, "GIMP_PDB_CANCEL", "cancel" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PDB_EXECUTION_ERROR, "GIMP_PDB_EXECUTION_ERROR", NULL },
    { GIMP_PDB_CALLING_ERROR, "GIMP_PDB_CALLING_ERROR", NULL },
    { GIMP_PDB_PASS_THROUGH, "GIMP_PDB_PASS_THROUGH", NULL },
    { GIMP_PDB_SUCCESS, "GIMP_PDB_SUCCESS", NULL },
    { GIMP_PDB_CANCEL, "GIMP_PDB_CANCEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpPDBStatusType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_message_handler_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_MESSAGE_BOX, "GIMP_MESSAGE_BOX", "message-box" },
    { GIMP_CONSOLE, "GIMP_CONSOLE", "console" },
    { GIMP_ERROR_CONSOLE, "GIMP_ERROR_CONSOLE", "error-console" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_MESSAGE_BOX, "GIMP_MESSAGE_BOX", NULL },
    { GIMP_CONSOLE, "GIMP_CONSOLE", NULL },
    { GIMP_ERROR_CONSOLE, "GIMP_ERROR_CONSOLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpMessageHandlerType", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_stack_trace_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_STACK_TRACE_NEVER, "GIMP_STACK_TRACE_NEVER", "never" },
    { GIMP_STACK_TRACE_QUERY, "GIMP_STACK_TRACE_QUERY", "query" },
    { GIMP_STACK_TRACE_ALWAYS, "GIMP_STACK_TRACE_ALWAYS", "always" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_STACK_TRACE_NEVER, "GIMP_STACK_TRACE_NEVER", NULL },
    { GIMP_STACK_TRACE_QUERY, "GIMP_STACK_TRACE_QUERY", NULL },
    { GIMP_STACK_TRACE_ALWAYS, "GIMP_STACK_TRACE_ALWAYS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpStackTraceMode", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
gimp_progress_command_get_type (void)
{
  static const GEnumValue values[] =
  {
    { GIMP_PROGRESS_COMMAND_START, "GIMP_PROGRESS_COMMAND_START", "start" },
    { GIMP_PROGRESS_COMMAND_END, "GIMP_PROGRESS_COMMAND_END", "end" },
    { GIMP_PROGRESS_COMMAND_SET_TEXT, "GIMP_PROGRESS_COMMAND_SET_TEXT", "set-text" },
    { GIMP_PROGRESS_COMMAND_SET_VALUE, "GIMP_PROGRESS_COMMAND_SET_VALUE", "set-value" },
    { 0, NULL, NULL }
  };

  static const GimpEnumDesc descs[] =
  {
    { GIMP_PROGRESS_COMMAND_START, "GIMP_PROGRESS_COMMAND_START", NULL },
    { GIMP_PROGRESS_COMMAND_END, "GIMP_PROGRESS_COMMAND_END", NULL },
    { GIMP_PROGRESS_COMMAND_SET_TEXT, "GIMP_PROGRESS_COMMAND_SET_TEXT", NULL },
    { GIMP_PROGRESS_COMMAND_SET_VALUE, "GIMP_PROGRESS_COMMAND_SET_VALUE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (! type)
    {
      type = g_enum_register_static ("GimpProgressCommand", values);
      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");
      gimp_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

