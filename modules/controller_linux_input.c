/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * controller_linux_input.c
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include <gtk/gtk.h>

#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpwidgets/gimpcontroller.h"

#include "libgimp/libgimp-intl.h"


enum
{
  PROP_0,
  PROP_DEVICE
};


#define CONTROLLER_TYPE_LINUX_INPUT            (controller_type)
#define CONTROLLER_LINUX_INPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInput))
#define CONTROLLER_LINUX_INPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONTROLLER_TYPE_LINUX_INPUT, ControllerLinuxInputClass))
#define CONTROLLER_IS_LINUX_INPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROLLER_TYPE_LINUX_INPUT))
#define CONTROLLER_IS_LINUX_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTROLLER_TYPE_LINUX_INPUT))


typedef struct _ControllerLinuxInput      ControllerLinuxInput;
typedef struct _ControllerLinuxInputClass ControllerLinuxInputClass;

struct _ControllerLinuxInput
{
  GimpController       parent_instance;

  gchar               *device;
  GIOChannel          *io;
};

struct _ControllerLinuxInputClass
{
  GimpControllerClass  parent_class;
};


typedef struct _LinuxInputEvent LinuxInputEvent;

struct _LinuxInputEvent
{
  __u16         type;
  __u16         code;
  const gchar  *name;
};


GType         linux_input_get_type     (GTypeModule    *module);
static void   linux_input_class_init   (ControllerLinuxInputClass *klass);
static void   linux_input_finalize     (GObject        *object);
static void   linux_input_set_property (GObject        *object,
                                        guint           property_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec);
static void   linux_input_get_property (GObject        *object,
                                        guint           property_id,
                                        GValue         *value,
                                        GParamSpec     *pspec);

static gint          linux_input_get_n_events   (GimpController *controller);
static const gchar * linux_input_get_event_name (GimpController *controller,
                                                 gint            event_id);

static gboolean      linux_input_set_device (ControllerLinuxInput *controller,
                                             const gchar          *device);
static gboolean      linux_input_read_event (GIOChannel           *io,
                                             GIOCondition          cond,
                                             gpointer              user_data);


static const GimpModuleInfo linux_input_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Linux input event controller"),
  "Sven Neumann <sven@gimp.org>",
  "v0.1",
  "(c) 2004, released under the GPL",
  "June 2004"
};


static const LinuxInputEvent input_events[] =
{
  { EV_KEY, BTN_0,      N_("Button 0")      },
  { EV_KEY, BTN_1,      N_("Button 1")      },
  { EV_KEY, BTN_2,      N_("Button 2")      },
  { EV_KEY, BTN_3,      N_("Button 3")      },
  { EV_KEY, BTN_4,      N_("Button 4")      },
  { EV_KEY, BTN_5,      N_("Button 5")      },
  { EV_KEY, BTN_6,      N_("Button 6")      },
  { EV_KEY, BTN_7,      N_("Button 7")      },
  { EV_KEY, BTN_8,      N_("Button 8")      },
  { EV_KEY, BTN_9,      N_("Button 9")      },
  { EV_KEY, BTN_LEFT,   N_("Button Left")   },
  { EV_KEY, BTN_RIGHT,  N_("Button Right")  },
  { EV_KEY, BTN_MIDDLE, N_("Button Middle") },
  { EV_REL, REL_DIAL,   N_("Dial (rel.)")   },
  { EV_REL, REL_WHEEL,  N_("Wheel (rel.)")  },
  { EV_ABS, ABS_WHEEL,  N_("Wheel (abs.)")  }
};

static GType                controller_type = 0;
static GimpControllerClass *parent_class    = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &linux_input_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  linux_input_get_type (module);

  return TRUE;
}


GType
linux_input_get_type (GTypeModule *module)
{
  if (! controller_type)
    {
      static const GTypeInfo controller_info =
      {
        sizeof (ControllerLinuxInputClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) linux_input_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (ControllerLinuxInput),
        0,              /* n_preallocs    */
        NULL            /* instance_init  */
      };

      controller_type = g_type_module_register_type (module,
                                                     GIMP_TYPE_CONTROLLER,
                                                     "ControllerLinuxInput",
                                                     &controller_info, 0);
    }

  return controller_type;
}

static void
linux_input_class_init (ControllerLinuxInputClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize           = linux_input_finalize;
  object_class->get_property       = linux_input_get_property;
  object_class->set_property       = linux_input_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  controller_class->name           = _("Linux Input Events");

  controller_class->get_n_events   = linux_input_get_n_events;
  controller_class->get_event_name = linux_input_get_event_name;
}

static void
linux_input_finalize (GObject *object)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  linux_input_set_device (controller, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
linux_input_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      linux_input_set_device (controller, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
linux_input_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ControllerLinuxInput *controller = CONTROLLER_LINUX_INPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      g_value_set_string (value, controller->device);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
linux_input_get_n_events (GimpController *controller)
{
  return G_N_ELEMENTS (input_events);
}

static const gchar *
linux_input_get_event_name (GimpController *controller,
                                      gint            event_id)
{
  if (event_id < 0 || event_id >= G_N_ELEMENTS (input_events))
    return NULL;

  return gettext (input_events[event_id].name);
}

static gboolean
linux_input_set_device (ControllerLinuxInput *controller,
                        const gchar          *device)
{
  if (controller->io)
    {
      g_io_channel_unref (controller->io);
      controller->io = NULL;
    }

  if (controller->device)
    g_free (controller->device);

  controller->device = g_strdup (device);

  if (device)
    {
      gint fd;

      fd = open (controller->device, O_RDONLY);
      if (fd >= 0)
        {
          controller->io = g_io_channel_unix_new (fd);
          g_io_channel_set_close_on_unref (controller->io, TRUE);
          g_io_channel_set_encoding (controller->io, NULL, NULL);

          g_io_add_watch (controller->io,
                          G_IO_IN,
                          linux_input_read_event,
                          controller);
          return TRUE;
        }
      else
        {
          g_printerr ("Cannot open device '%s': %s",
                      device, g_strerror (errno));
        }
    }

  return FALSE;
}

gboolean
linux_input_read_event (GIOChannel   *io,
                        GIOCondition  cond,
                        gpointer      user_data)
{
  struct input_event  ev;
  gint                n_bytes;
  gint                i;

  if (g_io_channel_read_chars (io,
                               (gchar *) &ev,
                               sizeof (struct input_event), &n_bytes,
                               NULL) == G_IO_STATUS_NORMAL &&
      n_bytes == sizeof (struct input_event))
    {
      for (i = 0; i < G_N_ELEMENTS (input_events); i++)
        {
          if (ev.type == input_events[i].type &&
              ev.code == input_events[i].code)
            {
              GimpController      *controller = GIMP_CONTROLLER (user_data);
              GimpControllerEvent  cevent;

              cevent.any.type     = GIMP_CONTROLLER_EVENT_TRIGGER;
              cevent.any.source   = controller;
              cevent.any.event_id = i;

              gimp_controller_event (controller, &cevent);

              break;
            }
        }
    }

  return TRUE;
}

