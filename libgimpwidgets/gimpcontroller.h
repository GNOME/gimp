/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacontroller.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef LIGMA_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#error LigmaController is unstable API under construction
#endif

#ifndef __LIGMA_CONTROLLER_H__
#define __LIGMA_CONTROLLER_H__

G_BEGIN_DECLS

/* For information look at the html documentation */


/**
 * LigmaControllerEventType:
 * @LIGMA_CONTROLLER_EVENT_TRIGGER: the event is a simple trigger
 * @LIGMA_CONTROLLER_EVENT_VALUE:   the event carries a double value
 *
 * Event types for #LigmaController.
 **/
typedef enum
{
  LIGMA_CONTROLLER_EVENT_TRIGGER,
  LIGMA_CONTROLLER_EVENT_VALUE
} LigmaControllerEventType;


typedef struct _LigmaControllerEventAny     LigmaControllerEventAny;
typedef struct _LigmaControllerEventTrigger LigmaControllerEventTrigger;
typedef struct _LigmaControllerEventValue   LigmaControllerEventValue;
typedef union  _LigmaControllerEvent        LigmaControllerEvent;

/**
 * LigmaControllerEventAny:
 * @type:     The event's #LigmaControllerEventType
 * @source:   The event's source #LigmaController
 * @event_id: The event's ID
 *
 * Generic controller event. Every event has these three members at the
 * beginning of its struct
 **/
struct _LigmaControllerEventAny
{
  LigmaControllerEventType  type;
  LigmaController          *source;
  gint                     event_id;
};

/**
 * LigmaControllerEventTrigger:
 * @type:     The event's #LigmaControllerEventType
 * @source:   The event's source #LigmaController
 * @event_id: The event's ID
 *
 * Trigger controller event.
 **/
struct _LigmaControllerEventTrigger
{
  LigmaControllerEventType  type;
  LigmaController          *source;
  gint                     event_id;
};

/**
 * LigmaControllerEventValue:
 * @type:     The event's #LigmaControllerEventType
 * @source:   The event's source #LigmaController
 * @event_id: The event's ID
 * @value:    The event's value
 *
 * Value controller event.
 **/
struct _LigmaControllerEventValue
{
  LigmaControllerEventType  type;
  LigmaController          *source;
  gint                     event_id;
  GValue                   value;
};

/**
 * LigmaControllerEvent:
 * @type:    The event type
 * @any:     LigmaControllerEventAny
 * @trigger: LigmaControllerEventTrigger
 * @value:   LigmaControllerEventValue
 *
 * A union to hjold all event event types
 **/
union _LigmaControllerEvent
{
  LigmaControllerEventType    type;
  LigmaControllerEventAny     any;
  LigmaControllerEventTrigger trigger;
  LigmaControllerEventValue   value;
};


#define LIGMA_TYPE_CONTROLLER            (ligma_controller_get_type ())
#define LIGMA_CONTROLLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER, LigmaController))
#define LIGMA_CONTROLLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER, LigmaControllerClass))
#define LIGMA_IS_CONTROLLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER))
#define LIGMA_IS_CONTROLLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER))
#define LIGMA_CONTROLLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER, LigmaControllerClass))


typedef struct _LigmaControllerPrivate LigmaControllerPrivate;
typedef struct _LigmaControllerClass   LigmaControllerClass;

struct _LigmaController
{
  GObject                parent_instance;

  LigmaControllerPrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  gchar    *name;
  gchar    *state;
};

struct _LigmaControllerClass
{
  GObjectClass  parent_class;

  const gchar  *name;
  const gchar  *help_domain;
  const gchar  *help_id;
  const gchar  *icon_name;

  /*  virtual functions  */
  gint          (* get_n_events)    (LigmaController            *controller);
  const gchar * (* get_event_name)  (LigmaController            *controller,
                                     gint                       event_id);
  const gchar * (* get_event_blurb) (LigmaController            *controller,
                                     gint                       event_id);

  /*  signals  */
  gboolean      (* event)           (LigmaController            *controller,
                                     const LigmaControllerEvent *event);

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType            ligma_controller_get_type        (void) G_GNUC_CONST;
LigmaController * ligma_controller_new             (GType           controller_type);

gint             ligma_controller_get_n_events    (LigmaController *controller);
const gchar    * ligma_controller_get_event_name  (LigmaController *controller,
                                                  gint            event_id);
const gchar    * ligma_controller_get_event_blurb (LigmaController *controller,
                                                  gint            event_id);


/*  protected  */

gboolean         ligma_controller_event (LigmaController            *controller,
                                        const LigmaControllerEvent *event);


G_END_DECLS

#endif /* __LIGMA_CONTROLLER_H__ */
