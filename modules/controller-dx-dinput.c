/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * controller_dx_dinput.c
 * Copyright (C) 2004-2007 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Tor Lillqvist <tml@iki.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* When using gcc with the February 2007 version of the DirectX SDK,
 * at least, you need to fix a couple of duplicate typedefs in the
 * DirectX <dinput.h>. Unfortunately I can't include the diff here,
 * both for copyright reasons, and because that would confuse the C
 * lexer even if inside #if 0.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <math.h>

#include <windows.h>
#include <dinput.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#define GIMP_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libgimpwidgets/gimpcontroller.h"

#include "gimpinputdevicestore.h"

#include "libgimp/libgimp-intl.h"

enum
{
  PROP_0,
  PROP_DEVICE,
  PROP_DEVICE_STORE
};

#define NUM_EVENTS_PER_BUTTON 3 /* Button click, press and release */
#define NUM_EVENTS_PER_AXIS 2   /* Axis decrease and increase */
#define NUM_EVENTS_PER_SLIDER 2 /* Slider decrease and increase */
#define NUM_EVENTS_PER_POV 3    /* POV view vector X and Y view and return */

#define CONTROLLER_TYPE_DX_DINPUT            (controller_dx_dinput_get_type ())
#define CONTROLLER_DX_DINPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CONTROLLER_TYPE_DX_DINPUT, ControllerDXDInput))
#define CONTROLLER_DX_DINPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CONTROLLER_TYPE_DX_DINPUT, ControllerDXDInputClass))
#define CONTROLLER_IS_DX_DINPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CONTROLLER_TYPE_DX_DINPUT))
#define CONTROLLER_IS_DX_DINPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CONTROLLER_TYPE_DX_DINPUT))

typedef struct _DXDInputSource          DXDInputSource;
typedef struct _ControllerDXDInput      ControllerDXDInput;
typedef struct _ControllerDXDInputClass ControllerDXDInputClass;

struct _DXDInputSource
{
  GSource             source;
  ControllerDXDInput *controller;
};

struct _ControllerDXDInput
{
  GimpController        parent_instance;

  GimpInputDeviceStore *store;

  LPDIRECTINPUTDEVICE8W didevice8;

  HANDLE                event;

  GPollFD              *pollfd;

  gint                  num_buttons;
  gint                  num_button_events;

  gint                  num_axes;
  gint                  num_axis_events;

  gint                  num_sliders;
  gint                  num_slider_events;

  gint                  num_povs;
  gint                  num_pov_events;

  gint                  num_events;
  gchar               **event_names;
  gchar               **event_blurbs;

  DIDATAFORMAT         *format;

  guchar               *prevdata;

  gchar                *guid;
  GSource              *source;

  /* Variables used by enumeration callbacks only*/
  gint                  bei, bi, aei, ai, sei, si, pei, pi;
};

struct _ControllerDXDInputClass
{
  GimpControllerClass  parent_class;
};


GType         controller_dx_dinput_get_type     (void);

static void   dx_dinput_dispose                 (GObject        *object);
static void   dx_dinput_finalize                (GObject        *object);
static void   dx_dinput_set_property            (GObject        *object,
                                                 guint           property_id,
                                                 const GValue   *value,
                                                 GParamSpec     *pspec);
static void   dx_dinput_get_property            (GObject        *object,
                                                 guint           property_id,
                                                 GValue         *value,
                                                 GParamSpec     *pspec);

static gint          dx_dinput_get_n_events     (GimpController *controller);
static const gchar * dx_dinput_get_event_name   (GimpController *controller,
                                                 gint            event_id);
static const gchar * dx_dinput_get_event_blurb  (GimpController *controller,
                                                 gint            event_id);

static void          dx_dinput_device_changed   (ControllerDXDInput *controller,
                                                 const gchar        *guid);
static gboolean      dx_dinput_set_device       (ControllerDXDInput *controller,
                                                 const gchar        *guid);


static const GimpModuleInfo dx_dinput_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("DirectX DirectInput event controller"),
  "Tor Lillqvist <tml@iki.fi>",
  "v0.1",
  "(c) 2004-2007, released under the GPL",
  "2004-2007"
};


G_DEFINE_DYNAMIC_TYPE (ControllerDXDInput, controller_dx_dinput,
                       GIMP_TYPE_CONTROLLER)


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &dx_dinput_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  gimp_input_device_store_register_types (module);
  controller_dx_dinput_register_type (module);

  return TRUE;
}

static void
controller_dx_dinput_class_init (ControllerDXDInputClass *klass)
{
  GimpControllerClass *controller_class = GIMP_CONTROLLER_CLASS (klass);
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);

  object_class->dispose            = dx_dinput_dispose;
  object_class->finalize           = dx_dinput_finalize;
  object_class->get_property       = dx_dinput_get_property;
  object_class->set_property       = dx_dinput_set_property;

  g_object_class_install_property (object_class, PROP_DEVICE,
                                   g_param_spec_string ("device",
                                                        _("Device:"),
                                                        _("The device to read DirectInput events from."),
                                                        NULL,
                                                        GIMP_CONFIG_PARAM_FLAGS));
  g_object_class_install_property (object_class, PROP_DEVICE_STORE,
                                   g_param_spec_object ("device-values",
                                                        NULL, NULL,
                                                        GIMP_TYPE_INPUT_DEVICE_STORE,
                                                        G_PARAM_READABLE));

  controller_class->name            = _("DirectX DirectInput");
  controller_class->help_id         = "gimp-controller-directx-directinput";
  controller_class->icon_name       = GIMP_ICON_CONTROLLER_LINUX_INPUT;

  controller_class->get_n_events    = dx_dinput_get_n_events;
  controller_class->get_event_name  = dx_dinput_get_event_name;
  controller_class->get_event_blurb = dx_dinput_get_event_blurb;
}

static void
controller_dx_dinput_class_finalize (ControllerDXDInputClass *klass)
{
}

static void
controller_dx_dinput_init (ControllerDXDInput *controller)
{
  controller->store = gimp_input_device_store_new ();

  if (controller->store)
    {
      g_signal_connect_swapped (controller->store, "device-added",
                                G_CALLBACK (dx_dinput_device_changed),
                                controller);
      g_signal_connect_swapped (controller->store, "device-removed",
                                G_CALLBACK (dx_dinput_device_changed),
                                controller);
    }
}

static void
dx_dinput_dispose (GObject *object)
{
  ControllerDXDInput *controller = CONTROLLER_DX_DINPUT (object);

  dx_dinput_set_device (controller, NULL);

  G_OBJECT_CLASS (controller_dx_dinput_parent_class)->dispose (object);
}

static void
dx_dinput_finalize (GObject *object)
{
  ControllerDXDInput *controller = CONTROLLER_DX_DINPUT (object);

  if (controller->source != NULL)
    {
      g_source_remove_poll (controller->source, controller->pollfd);
      g_source_remove (g_source_get_id (controller->source));
      g_source_unref (controller->source);
    }

  if (controller->didevice8)
    IDirectInputDevice8_SetEventNotification (controller->didevice8, NULL);

  if (controller->format != NULL)
    {
      g_free (controller->format->rgodf);
      g_free (controller->format);
      controller->format = NULL;
    }

  g_free (controller->prevdata);
  controller->prevdata = NULL;

  if (controller->event != NULL)
    {
      CloseHandle (controller->event);
      controller->event = 0;
    }

  if (controller->store)
    {
      g_object_unref (controller->store);
      controller->store = NULL;
    }

  G_OBJECT_CLASS (controller_dx_dinput_parent_class)->finalize (object);
}

static void
dx_dinput_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ControllerDXDInput *controller = CONTROLLER_DX_DINPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      dx_dinput_set_device (controller, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
dx_dinput_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  ControllerDXDInput *controller = CONTROLLER_DX_DINPUT (object);

  switch (property_id)
    {
    case PROP_DEVICE:
      g_value_set_string (value, controller->guid);
      break;
    case PROP_DEVICE_STORE:
      g_value_set_object (value, controller->store);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
dx_dinput_get_n_events (GimpController *controller)
{
  ControllerDXDInput *cdxdi = (ControllerDXDInput *) controller;

  return cdxdi->num_events;
}

static const gchar *
dx_dinput_get_event_name (GimpController *controller,
                          gint            event_id)
{
  ControllerDXDInput *cdxdi = (ControllerDXDInput *) controller;

  if (event_id < 0 || event_id >= cdxdi->num_events)
    return NULL;

  return cdxdi->event_names[event_id];
}

static const gchar *
dx_dinput_get_event_blurb (GimpController *controller,
                           gint            event_id)
{
  ControllerDXDInput *cdxdi = (ControllerDXDInput *) controller;

  if (event_id < 0 || event_id >= cdxdi->num_events)
    return NULL;

  return cdxdi->event_blurbs[event_id];
}

static BOOL CALLBACK
count_objects (const DIDEVICEOBJECTINSTANCEW *doi,
               void                          *user_data)
{
  ControllerDXDInput *controller = (ControllerDXDInput *) user_data;
  HRESULT             hresult;
  DIPROPRANGE         range;

  if (doi->dwType & DIDFT_AXIS)
    {
      /* Set reported range to  physical range, not to give upper
       * level software any false impression of higher accuracy.
       */

      range.diph.dwSize = sizeof (DIPROPRANGE);
      range.diph.dwHeaderSize = sizeof (DIPROPHEADER);
      range.diph.dwObj = doi->dwType;
      range.diph.dwHow = DIPH_BYID;

      if (FAILED ((hresult = IDirectInputDevice8_GetProperty (controller->didevice8,
                                                              DIPROP_PHYSICALRANGE,
                                                              &range.diph))))
        g_warning ("IDirectInputDevice8::GetProperty failed: %s",
                   g_win32_error_message (hresult));
      else
        {
          if (FAILED ((hresult = IDirectInputDevice8_SetProperty (controller->didevice8,
                                                                  DIPROP_RANGE,
                                                                  &range.diph))))
            g_warning ("IDirectInputDevice8::SetProperty failed: %s",
                       g_win32_error_message (hresult));
        }
    }

  if (IsEqualGUID (&doi->guidType, &GUID_Button))
    controller->num_buttons++;
  else if (IsEqualGUID (&doi->guidType, &GUID_XAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_YAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_ZAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RxAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RyAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RzAxis))
    controller->num_axes++;
  else if (IsEqualGUID (&doi->guidType, &GUID_Slider))
    controller->num_sliders++;
  else if (IsEqualGUID (&doi->guidType, &GUID_POV))
    controller->num_povs++;

  return DIENUM_CONTINUE;
}

static BOOL CALLBACK
name_objects (const DIDEVICEOBJECTINSTANCEW *doi,
              void                          *user_data)
{
  ControllerDXDInput *controller = (ControllerDXDInput *) user_data;

  if (IsEqualGUID (&doi->guidType, &GUID_Button))
    {
      controller->event_names[controller->bei] = g_strdup_printf ("button-%d", controller->bi);
      controller->event_blurbs[controller->bei] = g_strdup_printf (_("Button %d"), controller->bi);
      controller->bei++;
      controller->event_names[controller->bei] = g_strdup_printf ("button-%d-press", controller->bi);
      controller->event_blurbs[controller->bei] = g_strdup_printf (_("Button %d Press"), controller->bi);
      controller->bei++;
      controller->event_names[controller->bei] = g_strdup_printf ("button-%d-release", controller->bi);
      controller->event_blurbs[controller->bei] = g_strdup_printf (_("Button %d Release"), controller->bi);
      controller->bei++;

      g_assert (controller->bi < controller->num_buttons);

      controller->bi++;
    }
  else if (IsEqualGUID (&doi->guidType, &GUID_XAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_YAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_ZAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RxAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RyAxis) ||
           IsEqualGUID (&doi->guidType, &GUID_RzAxis))
    {
      if (IsEqualGUID (&doi->guidType, &GUID_XAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("x-move-left");
          controller->event_blurbs[controller->aei] = g_strdup (_("X Move Left"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("x-move-right");
          controller->event_blurbs[controller->aei] = g_strdup (_("X Move Right"));
          controller->aei++;
        }
      else if (IsEqualGUID (&doi->guidType, &GUID_YAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("y-move-away");
          controller->event_blurbs[controller->aei] = g_strdup (_("Y Move Away"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("y-move-near");
          controller->event_blurbs[controller->aei] = g_strdup (_("Y Move Near"));
          controller->aei++;
        }
      else if (IsEqualGUID (&doi->guidType, &GUID_ZAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("z-move-up");
          controller->event_blurbs[controller->aei] = g_strdup (_("Z Move Up"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("z-move-down");
          controller->event_blurbs[controller->aei] = g_strdup (_("Z Move Down"));
          controller->aei++;
        }
      else if (IsEqualGUID (&doi->guidType, &GUID_RxAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("x-axis-tilt-away");
          controller->event_blurbs[controller->aei] = g_strdup (_("X Axis Tilt Away"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("x-axis-tilt-near");
          controller->event_blurbs[controller->aei] = g_strdup (_("X Axis Tilt Near"));
          controller->aei++;
        }
      else if (IsEqualGUID (&doi->guidType, &GUID_RyAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("y-axis-tilt-right");
          controller->event_blurbs[controller->aei] = g_strdup (_("Y Axis Tilt Right"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("y-axis-tilt-left");
          controller->event_blurbs[controller->aei] = g_strdup (_("Y Axis Tilt Left"));
          controller->aei++;
        }
      else if (IsEqualGUID (&doi->guidType, &GUID_RzAxis))
        {
          controller->event_names[controller->aei] = g_strdup ("z-axis-turn-left");
          controller->event_blurbs[controller->aei] = g_strdup (_("Z Axis Turn Left"));
          controller->aei++;
          controller->event_names[controller->aei] = g_strdup ("z-axis-turn-right");
          controller->event_blurbs[controller->aei] = g_strdup (_("Z Axis Turn Right"));
          controller->aei++;
        }

      g_assert (controller->ai < controller->num_axes);

      controller->ai++;
    }
  else if (IsEqualGUID (&doi->guidType, &GUID_Slider))
    {
      controller->event_names[controller->sei] = g_strdup_printf ("slider-%d-increase", controller->si);
      controller->event_blurbs[controller->sei] = g_strdup_printf (_("Slider %d Increase"), controller->si);
      controller->sei++;
      controller->event_names[controller->sei] = g_strdup_printf ("slider-%d-decrease", controller->si);
      controller->event_blurbs[controller->sei] = g_strdup_printf (_("Slider %d Decrease"), controller->si);
      controller->sei++;

      g_assert (controller->si < controller->num_sliders);

      controller->si++;
    }
  else if (IsEqualGUID (&doi->guidType, &GUID_POV))
    {
      controller->event_names[controller->pei] = g_strdup_printf ("pov-%d-x-view", controller->pi);
      controller->event_blurbs[controller->pei] = g_strdup_printf (_("POV %d X View"), controller->pi);
      controller->pei++;
      controller->event_names[controller->pei] = g_strdup_printf ("pov-%d-y-view", controller->pi);
      controller->event_blurbs[controller->pei] = g_strdup_printf (_("POV %d Y View"), controller->pi);
      controller->pei++;
      controller->event_names[controller->pei] = g_strdup_printf ("pov-%d-x-return", controller->pi);
      controller->event_blurbs[controller->pei] = g_strdup_printf (_("POV %d Return"), controller->pi);
      controller->pei++;

      g_assert (controller->pi < controller->num_povs);

      controller->pi++;
    }

  return DIENUM_CONTINUE;
}

static gboolean
dx_dinput_get_device_info (ControllerDXDInput *controller,
                           GError            **error)
{
  HRESULT hresult;

  controller->num_buttons =
    controller->num_axes =
    controller->num_sliders =
    controller->num_povs = 0;

  if (FAILED ((hresult = IDirectInputDevice8_EnumObjects (controller->didevice8,
                                                          count_objects,
                                                          controller,
                                                          DIDFT_AXIS|
                                                          DIDFT_BUTTON|
                                                          DIDFT_POV|
                                                          DIDFT_PSHBUTTON|
                                                          DIDFT_RELAXIS|
                                                          DIDFT_TGLBUTTON))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::EnumObjects failed: %s",
                   g_win32_error_message (hresult));
      return FALSE;
    }

  controller->num_button_events = controller->num_buttons*NUM_EVENTS_PER_BUTTON;
  controller->num_axis_events = controller->num_axes*NUM_EVENTS_PER_AXIS;
  controller->num_slider_events = controller->num_sliders*NUM_EVENTS_PER_SLIDER;
  controller->num_pov_events = controller->num_povs*NUM_EVENTS_PER_POV;

  controller->num_events =
    controller->num_button_events +
    controller->num_axis_events +
    controller->num_slider_events +
    controller->num_pov_events;

  controller->event_names = g_new (gchar *, controller->num_events);
  controller->event_blurbs = g_new (gchar *, controller->num_events);

  controller->bi = controller->ai = controller->si = controller->pi = 0;

  controller->bei = 0;
  controller->aei = controller->bei + controller->num_button_events;
  controller->sei = controller->aei + controller->num_axis_events;
  controller->pei = controller->sei + controller->num_slider_events;

  if (FAILED ((hresult = IDirectInputDevice8_EnumObjects (controller->didevice8,
                                                          name_objects,
                                                          controller,
                                                          DIDFT_AXIS|
                                                          DIDFT_BUTTON|
                                                          DIDFT_POV|
                                                          DIDFT_PSHBUTTON|
                                                          DIDFT_RELAXIS|
                                                          DIDFT_TGLBUTTON))))
    {
      g_free (controller->event_names);
      g_free (controller->event_blurbs);

      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::EnumObjects failed: %s",
                   g_win32_error_message (hresult));
      return FALSE;
    }


  g_assert (controller->bei == controller->num_button_events);
  g_assert (controller->aei == controller->bei + controller->num_axis_events);
  g_assert (controller->sei == controller->aei + controller->num_slider_events);
  g_assert (controller->pei == controller->sei + controller->num_pov_events);

  return TRUE;
}


static void
dx_dinput_device_changed (ControllerDXDInput *controller,
                          const gchar        *guid)
{
  if (controller->guid && strcmp (guid, controller->guid) == 0)
    {
      dx_dinput_set_device (controller, guid);
      g_object_notify (G_OBJECT (controller), "device");
    }
}

static gboolean
dx_dinput_event_prepare (GSource *source,
                         gint    *timeout)
{
  *timeout = -1;

  return FALSE;
}

static gboolean
dx_dinput_event_check (GSource *source)
{
  DXDInputSource *input = (DXDInputSource *) source;

  if (WaitForSingleObject (input->controller->event, 0) == WAIT_OBJECT_0)
    {
      ResetEvent (input->controller->event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
dx_dinput_event_dispatch (GSource     *source,
                          GSourceFunc  callback,
                          gpointer     user_data)
{
  ControllerDXDInput * const       input = ((DXDInputSource *) source)->controller;
  GimpController                  *controller = &input->parent_instance;
  const DIDATAFORMAT * const       format = input->format;
  const DIOBJECTDATAFORMAT        *rgodf = format->rgodf;
  guchar                          *data;
  gint                             i;
  GimpControllerEvent              cevent = { 0, };

  data = g_alloca (format->dwDataSize);

  if (FAILED (IDirectInputDevice8_GetDeviceState (input->didevice8,
                                                  format->dwDataSize,
                                                  data)))
    {
      return TRUE;
    }

  g_object_ref (controller);

  for (i = 0; i < input->num_buttons; i++)
    {
      if (input->prevdata[rgodf->dwOfs] != data[rgodf->dwOfs])
        {
          if (data[rgodf->dwOfs] & 0x80)
            {
              /* Click event, compatibility with Linux Input */
              cevent.any.type = GIMP_CONTROLLER_EVENT_TRIGGER;
              cevent.any.source = controller;
              cevent.any.event_id = i*NUM_EVENTS_PER_BUTTON;
              gimp_controller_event (controller, &cevent);

              /* Press event */
              cevent.any.event_id = i*NUM_EVENTS_PER_BUTTON + 1;
              gimp_controller_event (controller, &cevent);
            }
          else
            {
              /* Release event */
              cevent.any.type = GIMP_CONTROLLER_EVENT_TRIGGER;
              cevent.any.source = controller;
              cevent.any.event_id = i*NUM_EVENTS_PER_BUTTON + 2;
              gimp_controller_event (controller, &cevent);
            }
        }
      rgodf++;
    }

  for (i = 0; i < input->num_axes; i++)
    {
      LONG *prev = (LONG *)(input->prevdata+rgodf->dwOfs);
      LONG *curr = (LONG *)(data+rgodf->dwOfs);

      if (ABS (*prev - *curr) > 1)
        {
          cevent.any.type = GIMP_CONTROLLER_EVENT_VALUE;
          cevent.any.source = controller;
          cevent.any.event_id =
            input->num_button_events +
            i*NUM_EVENTS_PER_AXIS;
          g_value_init (&cevent.value.value, G_TYPE_DOUBLE);
          if (*curr - *prev < 0)
            {
              g_value_set_double (&cevent.value.value, *prev - *curr);
            }
          else
            {
              cevent.any.event_id++;
              g_value_set_double (&cevent.value.value, *curr - *prev);
            }
          gimp_controller_event (controller, &cevent);
          g_value_unset (&cevent.value.value);
        }
      else
        *curr = *prev;
      rgodf++;
    }

  for (i = 0; i < input->num_sliders; i++)
    {
      LONG *prev = (LONG *)(input->prevdata+rgodf->dwOfs);
      LONG *curr = (LONG *)(data+rgodf->dwOfs);

      if (ABS (*prev - *curr) > 1)
        {
          cevent.any.type = GIMP_CONTROLLER_EVENT_VALUE;
          cevent.any.source = controller;
          cevent.any.event_id =
            input->num_button_events +
            input->num_axis_events +
            i*NUM_EVENTS_PER_SLIDER;
          g_value_init (&cevent.value.value, G_TYPE_DOUBLE);
          if (*curr - *prev < 0)
            {
              g_value_set_double (&cevent.value.value, *prev - *curr);
            }
          else
            {
              cevent.any.event_id++;
              g_value_set_double (&cevent.value.value, *curr - *prev);
            }
          gimp_controller_event (controller, &cevent);
          g_value_unset (&cevent.value.value);
        }
      else
        *curr = *prev;
      rgodf++;
    }

  for (i = 0; i < input->num_povs; i++)
    {
      LONG prev = *((LONG *)(input->prevdata+rgodf->dwOfs));
      LONG curr = *((LONG *)(data+rgodf->dwOfs));
      double prevx, prevy;
      double currx, curry;

      if (prev != curr)
        {
          if (prev == -1)
            {
              prevx = 0.;
              prevy = 0.;
            }
          else
            {
              prevx = sin (prev/36000.*2.*G_PI);
              prevy = cos (prev/36000.*2.*G_PI);
            }
          if (curr == -1)
            {
              currx = 0.;
              curry = 0.;
            }
          else
            {
              currx = sin (curr/36000.*2.*G_PI);
              curry = cos (curr/36000.*2.*G_PI);
            }

          cevent.any.type = GIMP_CONTROLLER_EVENT_VALUE;
          cevent.any.source = controller;
          cevent.any.event_id =
            input->num_button_events +
            input->num_axis_events +
            input->num_slider_events +
            i*NUM_EVENTS_PER_POV;
          g_value_init (&cevent.value.value, G_TYPE_DOUBLE);
          g_value_set_double (&cevent.value.value, currx - prevx);
          gimp_controller_event (controller, &cevent);
          cevent.any.event_id++;
          g_value_set_double (&cevent.value.value, curry - prevy);
          gimp_controller_event (controller, &cevent);
          g_value_unset (&cevent.value.value);
          if (curr == -1)
            {
              cevent.any.type = GIMP_CONTROLLER_EVENT_TRIGGER;
              cevent.any.event_id++;
              gimp_controller_event (controller, &cevent);
            }
        }
      rgodf++;
    }

  g_assert (rgodf == format->rgodf + format->dwNumObjs);

  memmove (input->prevdata, data, format->dwDataSize);

  g_object_unref (controller);

  return TRUE;
}

static GSourceFuncs dx_dinput_event_funcs = {
  dx_dinput_event_prepare,
  dx_dinput_event_check,
  dx_dinput_event_dispatch,
  NULL
};


static void
dump_data_format (const DIDATAFORMAT *format)
{
#ifdef GIMP_UNSTABLE
  gint i;

  g_print ("dwSize: %ld\n", format->dwSize);
  g_print ("dwObjSize: %ld\n", format->dwObjSize);
  g_print ("dwFlags: ");
  if (format->dwFlags == DIDF_ABSAXIS)
    g_print ("DIDF_ABSAXIS");
  else if (format->dwFlags == DIDF_RELAXIS)
    g_print ("DIDF_RELAXIS");
  else
    g_print ("%#lx", format->dwFlags);
  g_print ("\n");
  g_print ("dwDataSize: %ld\n", format->dwDataSize);
  g_print ("dwNumObjs: %ld\n", format->dwNumObjs);

  for (i = 0; i < format->dwNumObjs; i++)
    {
      const DIOBJECTDATAFORMAT *oformat = (DIOBJECTDATAFORMAT *) (((char *) format->rgodf) + i*format->dwObjSize);
      unsigned char *guid;

      g_print ("Object %d:\n", i);
      if (oformat->pguid == NULL)
        g_print ("  pguid: NULL\n");
      else
        {
          UuidToString (oformat->pguid, &guid);
          g_print ("  pguid: %s\n", guid);
          RpcStringFree (&guid);
        }

      g_print ("  dwOfs: %ld\n", oformat->dwOfs);
      g_print ("  dwType: ");
      switch (DIDFT_GETTYPE (oformat->dwType))
        {
#define CASE(x) case DIDFT_##x: g_print ("DIDFT_"#x); break
        CASE (RELAXIS);
        CASE (ABSAXIS);
        CASE (AXIS);
        CASE (PSHBUTTON);
        CASE (TGLBUTTON);
        CASE (BUTTON);
        CASE (POV);
        CASE (COLLECTION);
        CASE (NODATA);
#undef CASE
        default: g_print ("%#x", DIDFT_GETTYPE (oformat->dwType));
        }
      g_print (" ");
      if (DIDFT_GETINSTANCE (oformat->dwType) == DIDFT_GETINSTANCE (DIDFT_ANYINSTANCE))
        g_print ("ANYINSTANCE");
      else
        g_print ("#%d", DIDFT_GETINSTANCE (oformat->dwType));
#define BIT(x) if (oformat->dwType & DIDFT_##x) g_print (" "#x)
      BIT (FFACTUATOR);
      BIT (FFEFFECTTRIGGER);
      BIT (OUTPUT);
      BIT (VENDORDEFINED);
      BIT (ALIAS);
      BIT (OPTIONAL);
#undef BIT
      g_print ("\n");
      g_print ("  dwFlags:");
      switch (oformat->dwFlags & DIDOI_ASPECTACCEL)
        {
        case DIDOI_ASPECTPOSITION: g_print (" DIDOI_ASPECTPOSITION"); break;
        case DIDOI_ASPECTVELOCITY: g_print (" DIDOI_ASPECTVELOCITY"); break;
        case DIDOI_ASPECTACCEL: g_print (" DIDOI_ASPECTACCEL"); break;
        }
      if (oformat->dwFlags & DIDOI_ASPECTFORCE)
        g_print (" DIDOI_ASPECTFORCE");
      g_print ("\n");
    }
#endif
}

static gboolean
dx_dinput_setup_events (ControllerDXDInput *controller,
                        GError            **error)
{
  HRESULT         hresult;
  DIPROPDWORD     dword;
  gint            i, k;
  DXDInputSource *source;

  if ((controller->event = CreateEvent (NULL, TRUE, FALSE, NULL)) == NULL)
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "CreateEvent failed: %s",
                   g_win32_error_message (GetLastError ()));
      return FALSE;
    }

  controller->format = g_new (DIDATAFORMAT, 1);
  controller->format->dwSize = sizeof (DIDATAFORMAT);
  controller->format->dwObjSize = sizeof (DIOBJECTDATAFORMAT);

  dword.diph.dwSize = sizeof (DIPROPDWORD);
  dword.diph.dwHeaderSize = sizeof (DIPROPHEADER);
  dword.diph.dwObj = 0;
  dword.diph.dwHow = DIPH_DEVICE;

  /* Get the axis mode so we can use the same in the format */
  if (FAILED ((hresult = IDirectInputDevice8_GetProperty (controller->didevice8,
                                                          DIPROP_AXISMODE,
                                                          &dword.diph))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::GetParameters failed: %s",
                   g_win32_error_message (hresult));
      goto fail0;
    }

  controller->format->dwFlags = dword.dwData + 1;

  controller->format->dwNumObjs =
    controller->num_buttons +
    controller->num_axes +
    controller->num_sliders +
    controller->num_povs;

  controller->format->rgodf = g_new (DIOBJECTDATAFORMAT, controller->format->dwNumObjs);

  k = 0;
  controller->format->dwDataSize = 0;

  for (i = 0; i < controller->num_buttons; i++)
    {
      controller->format->rgodf[k].pguid = NULL;
      controller->format->rgodf[k].dwOfs = controller->format->dwDataSize;
      controller->format->rgodf[k].dwType = DIDFT_BUTTON | DIDFT_MAKEINSTANCE (i);
      controller->format->rgodf[k].dwFlags = 0;
      controller->format->dwDataSize += 1;
      k++;
    }

  controller->format->dwDataSize = 4*((controller->format->dwDataSize + 3)/4);

  for (i = 0; i < controller->num_axes; i++)
    {
      controller->format->rgodf[k].pguid = NULL;
      controller->format->rgodf[k].dwOfs = controller->format->dwDataSize;
      controller->format->rgodf[k].dwType = DIDFT_AXIS | DIDFT_MAKEINSTANCE (i);
      controller->format->rgodf[k].dwFlags = DIDOI_ASPECTPOSITION;
      controller->format->dwDataSize += 4;
      k++;
    }

  for (i = 0; i < controller->num_sliders; i++)
    {
      controller->format->rgodf[k].pguid = NULL;
      controller->format->rgodf[k].dwOfs = controller->format->dwDataSize;
      controller->format->rgodf[k].dwType = DIDFT_AXIS | DIDFT_MAKEINSTANCE (i);
      controller->format->rgodf[k].dwFlags = DIDOI_ASPECTPOSITION;
      controller->format->dwDataSize += 4;
      k++;
    }

  for (i = 0; i < controller->num_povs; i++)
    {
      controller->format->rgodf[k].pguid = NULL;
      controller->format->rgodf[k].dwOfs = controller->format->dwDataSize;
      controller->format->rgodf[k].dwType = DIDFT_POV | DIDFT_MAKEINSTANCE (i);
      controller->format->rgodf[k].dwFlags = 0;
      controller->format->dwDataSize += 4;
      k++;
    }

  g_assert (k == controller->format->dwNumObjs);

  controller->format->dwDataSize = 4*((controller->format->dwDataSize + 3)/4);
  controller->prevdata = g_malloc (controller->format->dwDataSize);

  dump_data_format (controller->format);

  if (FAILED ((hresult = IDirectInputDevice8_SetDataFormat (controller->didevice8,
                                                            controller->format))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::SetDataFormat failed: %s",
                   g_win32_error_message (hresult));
      goto fail1;
    }

  if (FAILED ((hresult = IDirectInputDevice8_SetEventNotification (controller->didevice8,
                                                                   controller->event))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::SetEventNotification failed: %s",
                   g_win32_error_message (hresult));
      goto fail2;
    }

  if (FAILED ((hresult = IDirectInputDevice8_Acquire (controller->didevice8))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::Acquire failed: %s",
                   g_win32_error_message (hresult));
      goto fail2;
    }

  if (FAILED ((hresult = IDirectInputDevice8_GetDeviceState (controller->didevice8,
                                                             controller->format->dwDataSize,
                                                             controller->prevdata))))
    {
      g_set_error (error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
                   "IDirectInputDevice8::GetDeviceState failed: %s",
                   g_win32_error_message (hresult));
      goto fail2;
    }

  source = (DXDInputSource *) g_source_new (&dx_dinput_event_funcs,
                                            sizeof (DXDInputSource));
  source->controller = controller;
  controller->source = (GSource *) source;

  controller->pollfd = g_new (GPollFD, 1);

  controller->pollfd->fd = (int) controller->event;
  controller->pollfd->events = G_IO_IN;

  g_source_add_poll (&source->source, controller->pollfd);
  g_source_attach (&source->source, NULL);

  return TRUE;

 fail2:
  IDirectInputDevice8_SetEventNotification (controller->didevice8, NULL);
 fail1:
  g_free (controller->format->rgodf);
  g_free (controller->format);
  controller->format = NULL;
  g_free (controller->prevdata);
  controller->prevdata = NULL;
 fail0:
  CloseHandle (controller->event);
  controller->event = 0;

  return FALSE;
}

static gboolean
dx_dinput_set_device (ControllerDXDInput *controller,
                      const gchar        *guid)
{
  GError *error = NULL;

  if (controller->guid)
    g_free (controller->guid);

  controller->guid = g_strdup (guid);

  g_object_set (controller, "name", _("DirectInput Events"), NULL);

  if (controller->guid && strlen (controller->guid))
    {
      if (controller->store)
        controller->didevice8 =
          (LPDIRECTINPUTDEVICE8W) gimp_input_device_store_get_device_file (controller->store,
                                                                           controller->guid);
    }
  else
    {
      g_object_set (controller, "state", _("No device configured"), NULL);

      return FALSE;
    }

  if (controller->didevice8)
    {
      if (!dx_dinput_get_device_info (controller, &error) ||
          !dx_dinput_setup_events (controller, &error))
        {
          g_object_set (controller, "state", error->message, NULL);
          g_error_free (error);
        }
    }
  else if (controller->store)
    {
      error = gimp_input_device_store_get_error (controller->store);

      if (error)
        {
          g_object_set (controller, "state", error->message, NULL);
          g_error_free (error);
        }
      else
        {
          g_object_set (controller, "state", _("Device not available"), NULL);
        }
    }

  return FALSE;
}
