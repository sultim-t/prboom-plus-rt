#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "d_event.h"
#include "i_axes.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "mn_engin.h"
#include "v_video.h"

#include "lprintf.h"

// Action Variables
//

int axis_forward_value=0;
int axis_side_value=0;
int axis_turn_value=0;
int axis_cycleweapon_value=0;
int axis_changeweapon_value=0;

//
// Actions List
//

typedef struct axisaction_s
{
  const char *name; // text description

  enum {            // type of action
    at_variable,
    at_function,
    at_conscmd,
  } type;

  union {           // variable or handler
    int *variable;
    void (*Handler)();
  } value;

  struct axisaction_s *next; // next action

} axisaction_t;

axisaction_t axisactions[] =
{
  {"axisforward", at_variable,  {&axis_forward_value}       },
  {"axisstrafe",  at_variable,  {&axis_side_value}          },
  {"axisturn",    at_variable,  {&axis_turn_value}          },
  {"axiscycle",   at_variable,  {&axis_cycleweapon_value}   },
  {"axischange",  at_variable,  {&axis_changeweapon_value}  }
};

const int num_axisactions = sizeof(axisactions) / sizeof(*axisactions);

// Console Bindings

axisaction_t *cons_axisactions = NULL;

// axis bindings

typedef struct {
  char *name;
  int value;
  int sensitivity;
  boolean invert;
  axisaction_t *binding;
} doomaxis_t;

doomaxis_t **axisbindings;

//
// G_InitAxisBindings
//
// Setup axis names and other details
//
void G_InitAxisBindings()
{
  int i,j;
  int number_devices;
  int number_axes;

  number_devices = I_GetNumberOfDevices();

  axisbindings = (doomaxis_t **) calloc(number_devices, sizeof(doomaxis_t *) );

  for (i=0; i<number_devices; i++) {
    number_axes = I_GetAxesForDevice(i);
    axisbindings[i] = (doomaxis_t *) calloc(number_axes, sizeof(doomaxis_t) );

    for (j=0; j<number_axes; j++) {
      axisbindings[i][j].name = (char *)malloc(15*sizeof(char));
      lprintf(LO_INFO,"dev%d/axis%d\n", i, j);
      sprintf(axisbindings[i][j].name, "dev%d/axis%d", i, j);
      axisbindings[i][j].value = 0;
      axisbindings[i][j].sensitivity = 50;
      axisbindings[i][j].invert = false;
      axisbindings[i][j].binding = NULL;
    }
  }
}

//
// G_AxisActionForName
//

static axisaction_t *G_AxisActionForName(const char *name)
{
  int i;
  axisaction_t *prev, *temp, *newaction;

  // sequential search
  for(i=0; i<num_axisactions; i++)
    if(!strcasecmp(name, axisactions[i].name) )
      return &axisactions[i];

  // check console axisactions
  
  if(cons_axisactions) {
    temp = cons_axisactions;
    while(temp)
    {
      if(!strcasecmp(name, temp->name) )
        return temp;

      temp = temp->next;
    }
  } else {
    // first time only -- initialize cons_axisactions
    cons_axisactions = (axisaction_t *)malloc(sizeof(axisaction_t));
    cons_axisactions->type = at_conscmd;
    cons_axisactions->name = strdup(name);
    cons_axisactions->next = NULL;

    return cons_axisactions;
  }

  // not in list -- add
  prev = NULL;
  temp = cons_axisactions;
  while(temp)
  {
    prev = temp;
    temp = temp->next;
  }
  newaction = (axisaction_t *)malloc(sizeof(axisaction_t));
  newaction->type = at_conscmd;
  newaction->name = strdup(name);
  newaction->next = NULL;

  if(prev)
    prev->next = newaction;

  return newaction;
}

//
// G_AxisForName
//
doomaxis_t *G_AxisForName(char *name)
{
  int device, axis;

  sscanf(name, "dev%d/axis%d", &device, &axis);

  if(device<=I_GetNumberOfDevices()) 
  {
    if(axis<=I_GetAxesForDevice(device))
      return &axisbindings[device][axis];
  }
  return NULL;
}

//
// G_BindAxisToAction
//
static void G_BindAxisToAction(char *axis_name, const char *action_name)
{
  doomaxis_t *axis;
  axisaction_t *action;

  // get axis
  
  axis = G_AxisForName(axis_name);

  if (axis == NULL) {
    C_Printf("unknown axis '%s'\n", axis_name);
    return;
  }

  // get action

  action = G_AxisActionForName(action_name);

  if (action == NULL) {
    C_Printf("unknown action '%s'\n", action_name);
    return;
  }

  axis->binding = action;
}

//
// G_BoundAxes
//
const char *G_BoundAxes(char *actionname)
{
  int num_devices;
  int num_axes;
  int i, j;
  char *ret = NULL;

  axisaction_t *action = G_AxisActionForName(actionname);

  if (action == NULL)
    return "unknown action";

  num_devices = I_GetNumberOfDevices();

  for (i=0; i<num_devices; i++) {
    num_axes = I_GetAxesForDevice(i);
    for (j=0; j<num_axes; j++) {
      if (axisbindings[i][j].binding == action) {
        if (ret == NULL)
          strcat(ret, " + ");
        strcat(ret, axisbindings[i][j].name);
      }
    }
  }

  return (ret == NULL) ? ret : "none";
}

//
// G_AxisResponder
//
boolean G_AxisResponder (event_t *ev)
{
  if(ev->type == ev_axis) {
    int device = ev->data1;
    int axis = ev->data2;

    if(axisbindings[device][axis].binding != NULL) {
      switch (axisbindings[device][axis].binding->type) {
        case at_variable:
          if (axisbindings[device][axis].invert)
            *(axisbindings[device][axis].binding->value.variable) = ((-ev->data3) * axisbindings[device][axis].sensitivity) / 50;
          else
            *(axisbindings[device][axis].binding->value.variable) = (ev->data3 * axisbindings[device][axis].sensitivity) / 50;
          break;

        case at_function:
          axisbindings[device][axis].binding->value.Handler();
          break;

        case at_conscmd:
          C_RunTextCmd(axisbindings[device][axis].binding->name);
          break;

        default:
          break;
      }
    }
  }
  return true;
}

/*
 * Save bindings
 */

void G_WriteAxisBindings(FILE* file)
{
  int i,j;
  int num_devices, num_axes;

  num_devices = I_GetNumberOfDevices();
  for(i=0; i<num_devices; i++)
  {
    num_axes = I_GetAxesForDevice(i);
    for(j=0; j<num_axes; j++)
    {
      if(axisbindings[i][j].binding)
      {
        fprintf(file, "bindaxis %s %s\n",
                axisbindings[i][j].name,
                axisbindings[i][j].binding->name);
      }
      if(axisbindings[i][j].invert)
      {
        fprintf(file, "invertaxis %s\n",
                axisbindings[i][j].name);
      }
      if(axisbindings[i][j].sensitivity != 50)
      {
        fprintf(file, "axissensitivity %s %i\n",
                axisbindings[i][j].name, axisbindings[i][j].sensitivity);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////
//        MENU STUFF
//
//  Binding Selection Widget

static char *binding_action;  // Name of Action we are editing

//
// G_BindAxisDrawer
//
void G_BindAxisDrawer()
{
  char temp[100];
  int wid, height;

  // draw the menu in the background
  MN_DrawMenu(current_menu);

  // create message
  strcpy(temp, "\n -= move desired axis away from center, then press a key =- \n");

  wid = V_StringWidth(temp, 0);
  height = V_StringHeight(temp);

  // draw box

  // write text in box
  V_WriteText(temp,
      (320 - wid) / 2,
      (200 - height) /2,
      0);
}

//
// G_BindAxisResponder
//
boolean G_BindAxisResponder(event_t *ev)
{
  char *max_axis = NULL;
  int i,j;
  int num_devices, num_axes;
  int max_value;

  if(ev->type == ev_axis) {
    if (ev->data1 == 0) // We treat the mouse differently
      axisbindings[0][ev->data2].value += ev->data3;
    else
      axisbindings[ev->data1][ev->data2].value = ev->data3;
  }

  // got a key - Identify and bind to max axis, then close box
  
  if(ev->type == ev_keydown) {
    if(ev->data1 == KEYD_ESCAPE) {  // CANCEL
      current_menuwidget = NULL;
      return true;
    } else {
      max_value = 0;
      num_devices = I_GetNumberOfDevices();

      for(i=0; i<num_devices; i++) {
        num_axes = I_GetAxesForDevice(i);

        for(j=0; j<num_axes; j++) {
          if(axisbindings[i][j].value >= max_value)
            max_axis = axisbindings[i][j].name;
        }
      }
    }
  }

  current_menuwidget = NULL;

  G_BindAxisToAction(max_axis, binding_action);

  return true;
}

menuwidget_t axisbindings_widget = {G_BindAxisDrawer, G_BindAxisResponder};

//
// G_EditBinding
//
void G_EditAxisBinding(char *action) {
  current_menuwidget = &axisbindings_widget;
  binding_action = action;
}

///////////////////////////////////////////////////////////////////////
//          CONSOLE COMMANDS
//

CONSOLE_COMMAND(bindaxis,0) {
  if(c_argc >= 2) {
    G_BindAxisToAction(c_argv[0], c_argv[1]);
  } else if(c_argc == 1) {
    doomaxis_t *axis = G_AxisForName(c_argv[0]);
    if(axis == NULL)
      C_Printf("no such axis!\n");
    else {
      if(axis->binding != NULL)
        C_Printf("%s bound to %s\n", axis->name, axis->binding->name);
      else
        C_Printf("%s not bound\n", axis->name);
    }
  } else
    C_Printf("usage: bindaxis axis action\n");
}

CONSOLE_COMMAND(listaxisactions, 0) {
  int i;
  for(i=0; i<num_axisactions; i++)
    C_Printf("%s\n", axisactions[i].name);
}

CONSOLE_COMMAND(listaxes, 0) {
  int i,j;
  int num_devices, num_axes;

  num_devices = I_GetNumberOfDevices();

  for (i=0; i<num_devices; i++) {
    C_Printf("%s:\n", I_GetDeviceName(i));
    num_axes = I_GetAxesForDevice(i);

    for (j=0;j<num_axes;j++) {
      C_Printf("%s, ", axisbindings[i][j].name);
      if( (j+1)%4 == 0 )
        C_Printf("\n");
    }
    C_Printf("\n");
  }
}

CONSOLE_COMMAND(invertaxis, 0) {
  doomaxis_t *axis;

  if (c_argc != 1) {
    C_Printf("usage: invertaxis axis\n");
    return;
  }

  if ( (axis = G_AxisForName(c_argv[0])) != NULL ) {
    axis->invert = !axis->invert;
    if (axis->invert)
	C_Printf("axis %s inverted, was normal\n", c_argv[0]);
    else
	C_Printf("axis %s normal, was inverted\n", c_argv[0]);
  } else
    C_Printf("unknown axis %s\n", c_argv[0]);
}

CONSOLE_COMMAND(axissensitivity, 0) {
  doomaxis_t *axis;

  if (c_argc != 2) {
    C_Printf("usage: axissensitivity axis value\n");
    return;
  }

  if ( (axis = G_AxisForName(c_argv[0])) != NULL ) {
    axis->sensitivity = atoi(c_argv[1]);
    C_Printf("axis %s sensitivity now %i\n", c_argv[0], axis->sensitivity);
  } else
    C_Printf("unknown axis %s\n", c_argv[0]);
}

CONSOLE_COMMAND(unbindaxis, 0) {
  doomaxis_t *axis;

  if (c_argc != 1) {
    C_Printf("usage: unbindaxis axis\n");
    return;
  }

  if ( (axis = G_AxisForName(c_argv[0])) != NULL ) {
    C_Printf("unbound axis %s\n", c_argv[0]);
    axis->binding = NULL;
  } else
    C_Printf("unknown axis %s\n", c_argv[0]);
}

CONSOLE_COMMAND(unbindallaxes, 0) {
  int i,j;
  int num_devices, num_axes;

  C_Printf("clearing all key bindings\n");

  num_devices = I_GetNumberOfDevices();

  for(i=0; i<num_devices; i++) {
    num_axes = I_GetAxesForDevice(i);
    for(j=0; j<num_axes; j++)
      axisbindings[i][j].binding = NULL;
  }
}

void G_BindAxes_AddCommands() {
  C_AddCommand(bindaxis);
  C_AddCommand(listaxisactions);
  C_AddCommand(listaxes);
  C_AddCommand(invertaxis);
  C_AddCommand(axissensitivity);
  C_AddCommand(unbindaxis);
  C_AddCommand(unbindallaxes);
}
