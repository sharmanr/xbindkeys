/***************************************************************************
        xbindkeys : a program to bind keys to commands under X11.
                           -------------------
    begin                : Sat Oct 13 14:11:34 CEST 2001
    copyright            : (C) 2001 by Philippe Brochard
    email                : hocwp@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keys.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/keysym.h>
#include <regex.h>
#include "xbindkeys.h"
#include "keys.h"
#include "config.h"
#include "options.h"
#include "grab_key.h"


#define STR_KEY_LEN 1000

int nb_keys;
Keys_t *keys;

extern char rc_file[512];

Condition_t no_condition = { CONDITION_NONE };
Condition_t func_condition = { CONDITION_SCM_FUNC };

static char *modifier_string[] = { "Control", "Shift", "Alt", "Mod2",
  "Mod3", "Mod4", "Mod5"
};

char *condition_strings[] = { "None", "Match", "Exclude" };
char *match_strings[] = { "None", "File", "Class", "Name"};

int
init_keys (void)
{
  nb_keys = 0;
  keys = NULL;

  return 0;
}

void
close_keys (void)
{
  int i;

  for (i = 0; i < nb_keys; i++)
    free_key (&keys[i]);

  if (keys != NULL)
    {
      free (keys);
      keys = NULL;
    }

  nb_keys = 0;
}



int
add_key (KeyType_t type, EventType_t event_type, KeySym keysym, KeyCode keycode,
	 unsigned int button, unsigned int modifier, char *command, SCM function,
	 Condition_t condition)
{
  Keys_t *keys_bis = NULL;
  int i;

  if (keysym == 0 && keycode == 0 && button == 0)
    {
      fprintf (stderr, "Warning: unknown key in RC file : %s\n", rc_file);
      return (-1);
    }

  /* make new array keys_bis */
  keys_bis = (Keys_t *) malloc ((nb_keys + 1) * sizeof (Keys_t));
  if (keys_bis == NULL)
    return (-1);

  if (keys != NULL)
    {
      /* copy keys in keys_bis */
      for (i = 0; i < nb_keys; i++)
	{
	  keys_bis[i] = keys[i];
	}

      /* delete old keys array */
      free (keys);
    }

  /* make keys_bis as keys */
  keys = keys_bis;

  modifier &= ~(numlock_mask | capslock_mask | scrolllock_mask);

#ifdef GUILE_FLAG
  if(function) scm_gc_protect_object(function);
#endif
  /* set key */
  if (type == SYM)
    {
      set_keysym (&keys[nb_keys], event_type, keysym, modifier, command, function);
    }
  else if (type == BUTTON)
    {
      set_button (&keys[nb_keys], event_type, button, modifier, command, function);
    }
  else
    {
      set_keycode (&keys[nb_keys], event_type, keycode, modifier, command, function);
    }

  keys[nb_keys].condition = condition;
    
  /* new key */
  nb_keys += 1;

  return (0);
}






void
show_key_binding (Display * d)
{
  int i;
  int last_verbose = verbose;

  verbose = 1;

  for (i = 0; i < nb_keys; i++)
    {
      print_key (d, &keys[i]);
    }

  verbose = last_verbose;
}



void
modifier_to_string (unsigned int modifier, char *str)
{
  str[0] = '\0';

  if (modifier & ControlMask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[0], STR_KEY_LEN);
    }

  if (modifier & ShiftMask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[1], STR_KEY_LEN);
    }


  if (modifier & Mod1Mask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[2], STR_KEY_LEN);
    }

  if (modifier & Mod2Mask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[3], STR_KEY_LEN);
    }

  if (modifier & Mod3Mask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[4], STR_KEY_LEN);
    }

  if (modifier & Mod4Mask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[5], STR_KEY_LEN);
    }

  if (modifier & Mod5Mask)
    {
      if (str[0])
	strncat (str, "+", STR_KEY_LEN);
      strncat (str, modifier_string[6], STR_KEY_LEN);
    }
}


void
print_key (Display * d, Keys_t * key)
{
  char str[STR_KEY_LEN];
  int keysyms_per_keycode_return;

  if (verbose)
    {
      if (key->type == SYM)
	{
	  modifier_to_string (key->modifier, str);

	  printf ("\"%s\"\n    %s%s%s%s\n",
		  key->command != NULL ? key->command : ( key->function != 0 ? "(Scheme function)" : "NoCommand" ),
		  key->event_type == PRESS ? "" : "Release + ",
		  str, str[0] ? " + " : "", XKeysymToString (key->key.sym));
	}
      else if (key->type == BUTTON)
	{
	  printf ("\"%s\"\n    %sm:0x%x + b:%d   (mouse)\n",
		  key->command != NULL ? key->command : ( key->function != 0 ? "(Scheme function)" : "NoCommand" ),
		  key->event_type == PRESS ? "" : "Release + ",
		  key->modifier, key->key.button);
	}
      else
	{
	  printf ("\"%s\"\n    %sm:0x%x + c:%d\n",
		  key->command != NULL ? key->command : ( key->function != 0 ? "(Scheme function)" : "NoCommand" ),
		  key->event_type == PRESS ? "" : "Release + ",
		  key->modifier, key->key.code);
	  if (d != NULL)
	    {
	      modifier_to_string (key->modifier, str);
	      printf ("    %s%s%s%s\n",
		      str,
		      str[0] ? " + " : "",
		      key->event_type == PRESS ? "" : "Release + ",
		      (XKeysymToString (*XGetKeyboardMapping(d, key->key.code, 1, &keysyms_per_keycode_return)) != NULL) ?
                      XKeysymToString (*XGetKeyboardMapping(d, key->key.code, 1, &keysyms_per_keycode_return)) : "NoSymbol");
	    }
	}
      if (key->condition.required != CONDITION_NONE)
	{
	  if (key->condition.required == CONDITION_SCM_FUNC)
	    printf("Condition: guile function\n");
	  else
	    printf ("Condition:  %s must %s %s\n",
		    match_strings[key->condition.match],
		    key->condition.required == CONDITION_MATCH ? "match" : "not match",
		    key->condition.string);
	}
    }
}


void
set_keysym (Keys_t * key, EventType_t event_type, KeySym keysym,
	    unsigned int modifier, char *command, SCM function)
{
	int size;

  key->type = SYM;
  key->event_type = event_type;
  key->key.sym = keysym;
  key->modifier = modifier;

  if (command != NULL)
	{
      size = (strlen (command) + 1) * sizeof (char);
      key->command = (char *) malloc (size);
      if (key->command != NULL)
        strncpy (key->command, command, size);
    }
  else
    {
      key->command = NULL;
    }

  key->function = function;
}

void
set_keycode (Keys_t * key, EventType_t event_type, KeyCode keycode,
	     unsigned int modifier, char *command, SCM function)
{
  int size;

  key->type = CODE;
  key->event_type = event_type;
  key->key.code = keycode;
  key->modifier = modifier;

  if (command != NULL)
	  {
      size = (strlen (command) + 1) * sizeof (char);
      key->command = (char *) malloc (size);
      if (key->command != NULL)
        strncpy (key->command, command, size);
    }
  else
    {
      key->command = NULL;
    }

  key->function = function;
}


void
set_button (Keys_t * key, EventType_t event_type, unsigned int button,
	    unsigned int modifier, char *command, SCM function)
{
  int size;

  key->type = BUTTON;
  key->event_type = event_type;
  key->key.button = button;
  key->modifier = modifier;

  if (command != NULL)
    {
      size = (strlen (command) + 1) * sizeof (char);
      key->command = (char *) malloc (size);
      if (key->command != NULL)
        strncpy (key->command, command, size);
    }
  else
    {
      key->command = NULL;
    }

  key->function = function;
}




void
free_key (Keys_t * key)
{
  key->type = SYM;
  key->event_type = PRESS;
  key->key.sym = 0;
  key->modifier = 0;

  if (key->command != NULL)
    free (key->command);

  if (key->condition.required >= CONDITION_MATCH)
    {
      free (key->condition.string);
    }
  
#ifdef GUILE_FLAG
  if(key->function) scm_gc_unprotect_object(key->function);
#endif
  key->function = 0;
}




int
remove_key (KeyType_t type, EventType_t event_type, KeySym keysym, KeyCode keycode,
	    unsigned int button, unsigned int modifier)
{
  int i, found_index = -1;
  Keys_t *keys_bis = NULL;

  if (keys == NULL)
    {
      return (-1);
    }

  modifier &= ~(numlock_mask | capslock_mask | scrolllock_mask);

  for (i = 0; i < nb_keys; i++)
    {
      if (keys[i].type == type &&
	  keys[i].event_type == event_type &&
	  keys[i].modifier == modifier &&
	  ((type == SYM && keys[i].key.sym == keysym) ||
	   (type == CODE && keys[i].key.code == keycode) ||
	   (type == BUTTON && keys[i].key.button == button)))
	found_index = i;
    }

  if (found_index != -1)
    {
      if (verbose)
	printf ("Removing key index %d\n", found_index);

      /* make new array keys_bis */
      keys_bis = (Keys_t *) malloc ((nb_keys - 1) * sizeof (Keys_t));
      if (keys_bis == NULL)
	return (-1);

      for (i = 0; i < found_index; i++)
	keys_bis[i] = keys[i];

      for (i = found_index + 1; i < nb_keys; i++)
	keys_bis[i - 1] = keys[i];

      free_key (&keys[found_index]);
      free (keys);

      /* make keys_bis as keys */
      keys = keys_bis;

      nb_keys -= 1;
    }

  return (0);
}


void
run_command (char *command)
{
#ifdef FORK_FLAG
  pid_t pid;

  if (verbose)
    printf ("Start program with fork+exec call\n");

  //  if (fork() == 0)
  //  execlp ("sh", "sh", "-c", key->command, NULL);
  if (!(pid = fork()))
    {
      setsid();
      switch (fork())
	{
	case 0: execlp ("sh", "sh", "-c", command, (char *) NULL);
	  break;
	default: _exit(0);
	  break;
	}
    }
  if (pid > 0)
    wait(NULL);
#else
  if (verbose)
    printf ("Start program with system call\n");

  system (command);
#endif
}


int
start_command_key (Keys_t * key, Window window)
{
  int rc = 1;
  SCM scm_rc;
  
  if (key->command == NULL)
    {
#ifdef GUILE_FLAG
      if (key->function != 0)
	{
	  if (verbose)
	    printf("start_command_key: window = 0x%lx\n", window);
	  if (key->condition.required == CONDITION_SCM_FUNC)
	    {
	      if (command_delay)
		usleep(command_delay * 1000);
	      scm_rc = scm_call_0 (key->function);
	      rc = scm_is_true(scm_rc);
	      if (verbose)
		printf("guile condition function returned %d\n", rc);
	    }
	  else
	    {
	      if (command_delay)
		usleep(command_delay * 1000);
	      scm_call_0 (key->function);
	    }
	}
#endif
      return rc;
    }

  if (command_delay)
    usleep(command_delay * 1000);
  run_command (key->command);
  
  return rc;
}
