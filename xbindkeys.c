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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include "xbindkeys.h"
#include "keys.h"
#include "options.h"
#include "get_key.h"
#include "grab_key.h"
#include "winfo.h"

#ifdef GUILE_FLAG
#include <libguile.h>
#endif

#include <X11/XKBlib.h>


void end_it_all (Display * d);

static Display *start (char *);
static void event_loop (Display *);
static int *null_X_error (Display *, XErrorEvent *);
static void reload_rc_file (void);
static void catch_HUP_signal (int sig);
static void catch_CHLD_signal (int sig);
static void start_as_daemon (void);


Display *current_display;  // The current display

extern char rc_file[512];
#ifdef GUILE_FLAG
extern char rc_guile_file[512];
#endif
extern int poll_rc;

#define SLEEP_TIME 100

#ifndef GUILE_FLAG
int
main (int argc, char **argv)
#else
int argc_t; char** argv_t;
void
inner_main (int argc, char **argv)
#endif
{
  Display *d;

#ifdef GUILE_FLAG
  argc = argc_t;
  argv = argv_t;
#endif

  get_options (argc, argv);

  if (!rc_file_exist ())
    exit (-1);

  if (have_to_start_as_daemon && !have_to_show_binding && !have_to_get_binding)
    start_as_daemon ();

  if (!display_name)
    display_name = XDisplayName (NULL);

  show_options ();

  d = start (display_name);
  current_display = d;

  if (detectable_ar)
    {
      Bool supported_rtrn;
      XkbSetDetectableAutoRepeat(d, True, &supported_rtrn);

      if (!supported_rtrn)
	{
	  fprintf (stderr, "Could not set detectable autorepeat\n");
	}
    }

  get_offending_modifiers (d);

  if (have_to_get_binding)
    {
      get_key_binding (d, argv, argc);
      end_it_all (d);
      exit (0);
    }

#ifdef GUILE_FLAG
  if (get_rc_guile_file () != 0)
    {
#endif
      if (get_rc_file () != 0)
	{
	  end_it_all (d);
	  exit (-1);
	}
#ifdef GUILE_FLAG
    }
#endif


  if (have_to_show_binding)
    {
      show_key_binding (d);
      end_it_all (d);
      exit (0);
    }

  grab_keys (d);

  /* This: for restarting reading the RC file if get a HUP signal */
  signal (SIGHUP, catch_HUP_signal);
  /* and for reaping dead children */
  signal (SIGCHLD, catch_CHLD_signal);

  if (verbose)
    printf ("starting loop...\n");
  event_loop (d);

  if (verbose)
    printf ("ending...\n");
  end_it_all (d);

#ifndef GUILE_FLAG
  return (0);			/* not reached... */
#endif
}

#ifdef GUILE_FLAG
int
main (int argc, char** argv)
{
  //guile shouldn't steal our arguments! we already parse them!
  //so we put them in temporary variables.
  argv_t = argv;
  argc_t = argc;
  //printf("Starting in guile mode...\n"); //debugery!
  scm_boot_guile(0,(char**)NULL,(void *)inner_main,NULL);
  return 0; /* not reached ...*/
}
#endif



static Display *
start (char *display)
{
  Display *d;
  int screen;

  d = XOpenDisplay (display);
  if (!d)
    {
      fprintf (stderr,
	       "Could not open display, check shell DISPLAY variable, \
and export or setenv it!\n");
      exit (1);
    }


  XAllowEvents (d, AsyncBoth, CurrentTime);

  for (screen = 0; screen < ScreenCount (d); screen++)
    {
      XSelectInput (d, RootWindow (d, screen),
		    KeyPressMask | KeyReleaseMask | PointerMotionMask);
    }

  return (d);
}



void
end_it_all (Display * d)
{
  ungrab_all_keys (d);

  close_keys ();
  XCloseDisplay (d);
}



static void
adjust_display (XAnyEvent * xany)
{
  size_t envstr_size = strlen(DisplayString(xany->display)) + 8 + 1;
  char* envstr = malloc ( (envstr_size + 2) * sizeof (char) );
  XWindowAttributes attr;
  char* ptr;
  char buf[16];

  snprintf (envstr, envstr_size, "DISPLAY=%s", DisplayString(xany->display));

  XGetWindowAttributes (xany->display, xany->window,  &attr);

  if (verbose)
    printf ("got screen %d for window %x\n", XScreenNumberOfScreen(attr.screen), (unsigned int)xany->window );

  ptr = strchr (strchr (envstr, ':'), '.');
  if (ptr)
    *ptr = '\0';

  snprintf (buf, sizeof(buf), ".%i", XScreenNumberOfScreen(attr.screen));
  strncat (envstr, buf, 16);

  putenv (envstr);
}



/* If this key has a condition, is it met? */
static int
allow_key (Display * d, XEvent *e, int i, Window cwindow)
{
  int match_required;
  int match_found;
  int condition_satisfied;
  char *string;
  int n;

  if (keys[i].condition.required < CONDITION_MATCH)
    return 1;	/* no condition or guile func, so honour the event */

  match_required = keys[i].condition.required == CONDITION_MATCH;
  switch (keys[i].condition.match)
    {
    case MATCH_COMMAND:
      string = window_command (d, cwindow);
      break;
    case MATCH_CLASS:
      string = window_class (d, cwindow);
      break;
    case MATCH_NAME:
      string = window_name (d, cwindow);
      break;
    default:
      fprintf (stderr, "Invalid condition type %d\n", keys[i].condition.match);
      exit (2);
    }
  if (string == NULL)
    {
      fprintf (stderr, "Could not find string for window 0x%lx\n", cwindow);
      return 1;         /* Arbitrary, but treat as no condition */
    }
  else
    {
      if (verbose)
	printf ("i=%d String for window 0x%lx is %s\n", i, cwindow, string);
      n = regexec (&keys[i].condition.regexp, string, 0, NULL, 0);
      match_found = n == 0;
      condition_satisfied = match_found == match_required;
      if (verbose)
	printf ("match_required: %d  match_found: %d  condition_satisfied: %d\n",
		match_required, match_found, condition_satisfied);
      XFree(string);
      return condition_satisfied;
    }
}

static void
swallow_event(Display * d, XEvent * ev, int event_mode)
{
    XAllowEvents (d, event_mode, ev->xkey.time);
}

static void
passthru_event(Display * d, XEvent * ev, int event_mode)
{
    /* pass it through as if we never intercepted it */
    XAllowEvents (d, event_mode, ev->xkey.time);
    XFlush (d); /* don't forget! */
}



static int
execute_key (Display * d, Keys_t * key, XEvent * e, Window client_window)
{
  current_windowid = client_window;	/* accessible from scheme */
  print_key (d, key);
  adjust_display (&(e->xany));
  return start_command_key (key, client_window);
}

static void
event_loop (Display * d)
{
  XEvent e;
  int i;
  struct stat rc_file_info;
  time_t rc_file_changed = 0;
  int event_is_for_us;
  int condition_was_true;
  Window cwindow;	/* client window */
#ifdef GUILE_FLAG
  time_t rc_guile_file_changed = 0;
  struct stat rc_guile_file_info;
#endif


  XSetErrorHandler ((XErrorHandler) null_X_error);

  if (poll_rc)
    {
      stat(rc_file, &rc_file_info);
      rc_file_changed = rc_file_info.st_mtime;
#ifdef GUILE_FLAG
      stat (rc_guile_file, &rc_guile_file_info);
      rc_guile_file_changed = rc_guile_file_info.st_mtime;
#endif
    }

  while (True)
    {
      while(poll_rc && !XPending(d))
	{
	  // if the rc file has been modified, reload it
	  stat (rc_file, &rc_file_info);
#ifdef GUILE_FLAG
	  // if the rc guile file has been modified, reload it
	  stat (rc_guile_file, &rc_guile_file_info);
#endif

	  if (rc_file_info.st_mtime != rc_file_changed
#ifdef GUILE_FLAG
	      || rc_guile_file_info.st_mtime != rc_guile_file_changed
#endif
	      )
	    {
	      reload_rc_file ();
	      if (verbose)
		{
		  printf ("The configuration file has been modified, reload it\n");
		}
	      rc_file_changed = rc_file_info.st_mtime;
#ifdef GUILE_FLAG
	      rc_guile_file_changed = rc_guile_file_info.st_mtime;
#endif
	    }

	  usleep(SLEEP_TIME*1000);
	}

      XNextEvent (d, &e);

      switch (e.type)
	{
	case KeyPress:
	  cwindow = client_window (d, e.xkey.subwindow);
	  if (verbose)
	    {
	      printf ("Key press !\n");
	      printf ("e.xkey.keycode=%d\n", e.xkey.keycode);
	      printf ("e.xkey.state=%d\n", e.xkey.state);
	      printf ("e.xkey.subwindow is 0x%lx\n", e.xkey.subwindow);
	      printf ("client window = 0x%lx\n", cwindow);
	    }

	  e.xkey.state &= ~(numlock_mask | capslock_mask | scrolllock_mask);

	  condition_was_true = 0;
	  for (i = 0; i < nb_keys; i++)
	    {
	      event_is_for_us = 0;
	      if (keys[i].type == SYM && keys[i].event_type == PRESS)
		{
		  if (e.xkey.keycode == XKeysymToKeycode (d, keys[i].key.sym)
		      && e.xkey.state == keys[i].modifier)
		    event_is_for_us = 1;
		}
	      else
	      if (keys[i].type == CODE && keys[i].event_type == PRESS)
		{
		  if (e.xkey.keycode == keys[i].key.code
		      && e.xkey.state == keys[i].modifier)
		    event_is_for_us = 1;
		}
	      if (event_is_for_us)
		{
		  if (allow_key(d, &e, i, cwindow))
		    if (execute_key(d, &(keys[i]), &e, cwindow))
		      condition_was_true = 1;
		}
	    }
	  if (condition_was_true)
	    {
	      swallow_event(d, &e, AsyncKeyboard);
	    }
	  else
	    {
	      passthru_event(d, &e, ReplayKeyboard);
	    }
	  break;

	case KeyRelease:
	  cwindow = client_window (d, e.xkey.subwindow);
	  if (verbose)
	    {
	      printf ("Key release !\n");
	      printf ("e.xkey.keycode=%d\n", e.xkey.keycode);
	      printf ("e.xkey.state=%d\n", e.xkey.state);
	      printf ("client window = 0x%lx\n", cwindow);
	    }

	  e.xkey.state &= ~(numlock_mask | capslock_mask | scrolllock_mask);

	  condition_was_true = 0;
	  for (i = 0; i < nb_keys; i++)
	    {
	      event_is_for_us = 0;
	      if (keys[i].type == SYM && keys[i].event_type == RELEASE)
		{
		  if (e.xkey.keycode == XKeysymToKeycode (d, keys[i].key.sym)
		      && e.xkey.state == keys[i].modifier)
		    event_is_for_us = 1;
		}
	      else
	      if (keys[i].type == CODE && keys[i].event_type == RELEASE)
		{
		  if (e.xkey.keycode == keys[i].key.code
		      && e.xkey.state == keys[i].modifier)
		    event_is_for_us = 1;
		}
	      if (event_is_for_us)
		{
		  if (allow_key(d, &e, i, cwindow))
		    if (execute_key(d, &(keys[i]), &e, cwindow))
		      condition_was_true = 1;
		}
	    }
	  if (condition_was_true)
	    {
	      swallow_event(d, &e, AsyncKeyboard);
	    }
	  else
	    {
	      passthru_event(d, &e, ReplayKeyboard);
	    }
	  break;

	case ButtonPress:
	  cwindow = client_window (d, e.xbutton.subwindow);
	  if (verbose)
	    {
	      printf ("Button press !\n");
	      printf ("e.xbutton.button=%d\n", e.xbutton.button);
	      printf ("e.xbutton.state=%d\n", e.xbutton.state);
	      printf ("client window = 0x%lx\n", cwindow);
	    }

	  e.xbutton.state &= 0x1FFF & ~(numlock_mask | capslock_mask | scrolllock_mask
			       | Button1Mask | Button2Mask | Button3Mask
			       | Button4Mask | Button5Mask);

	  condition_was_true = 0;
	  for (i = 0; i < nb_keys; i++)
	    {
	      if (keys[i].type == BUTTON && keys[i].event_type == PRESS)
		{
		  if (e.xbutton.button == keys[i].key.button
		      && e.xbutton.state == keys[i].modifier)
		    {
                      //printf("Replay!!!\n");
                      //ungrab_all_keys(d);
                      //XPutBackEvent(d, &e);
                      //sleep(1);
                      //grab_keys(d);
		      if (allow_key(d, &e, i, cwindow))
			{
			  if (execute_key(d, &(keys[i]), &e, cwindow))
			    condition_was_true = 1;
			}
		    }
		}
	    }
	  if (condition_was_true)
	    swallow_event(d, &e, AsyncPointer);
	  else
	    passthru_event(d, &e, ReplayPointer);
	  break;

	case ButtonRelease:
	  cwindow = client_window (d, e.xbutton.subwindow);
	  if (verbose)
	    {
	      printf ("Button release !\n");
	      printf ("e.xbutton.button=%d\n", e.xbutton.button);
	      printf ("e.xbutton.state=%d\n", e.xbutton.state);
	      printf ("client window = 0x%lx\n", cwindow);
	    }

	  e.xbutton.state &= 0x1FFF & ~(numlock_mask | capslock_mask | scrolllock_mask
			       | Button1Mask | Button2Mask | Button3Mask
			       | Button4Mask | Button5Mask);

	  condition_was_true = 0;
	  for (i = 0; i < nb_keys; i++)
	    {
	      if (keys[i].type == BUTTON && keys[i].event_type == RELEASE)
		{
		  if (e.xbutton.button == keys[i].key.button
		      && e.xbutton.state == keys[i].modifier)
		    {
		      if (allow_key(d, &e, i, cwindow))
			{
			  if (execute_key(d, &(keys[i]), &e, cwindow))
			    condition_was_true = 1;
			}

		    }
		}
	    }
	  if (condition_was_true)
	    swallow_event(d, &e, AsyncPointer);
	  else
	    passthru_event(d, &e, ReplayPointer);
	  break;

	default:
	  break;
	}
    }				/*  infinite loop */
}



static int *
null_X_error (Display * d, XErrorEvent * e)
{
  static int already = 0;

  /* The warning is displayed only once */
  if (already != 0)
    return (NULL);
  already = 1;

  printf ("\n*** Warning ***\n");
  printf ("Please verify that there is not another program running\n");
  printf ("which captures one of the keys captured by xbindkeys.\n");
  printf ("It seems that there is a conflict, and xbindkeys can't\n");
  printf ("grab all the keys defined in its configuration file.\n");

/*   end_it_all (d); */

/*   exit (-1); */

  return (NULL);
}



static void
reload_rc_file (void)
{
  int min, max;
  int screen;

  XDisplayKeycodes (current_display, &min, &max);

  if (verbose)
    printf ("Reload RC file\n");

  for (screen = 0; screen < ScreenCount (current_display); screen++)
    {
      XUngrabKey (current_display, AnyKey, AnyModifier, RootWindow (current_display, screen));
    }
  close_keys ();

#ifdef GUILE_FLAG
  if (get_rc_guile_file () != 0)
    {
#endif
      if (get_rc_file () != 0)
	{
	  end_it_all (current_display);
	  exit (-1);
	}
#ifdef GUILE_FLAG
    }
#endif

  grab_keys (current_display);
}


static void
catch_HUP_signal (int sig)
{
  reload_rc_file ();
}


static void
catch_CHLD_signal (int sig)
{
  pid_t child;

  /*   If more than one child exits at approximately the same time, the signals */
  /*   may get merged. Handle this case correctly. */
  while ((child = waitpid(-1, NULL, WNOHANG)) > 0)
    {
      if (verbose)
	printf ("Catch CHLD signal -> pid %i terminated\n", child);
    }
}





static void
start_as_daemon (void)
{
  pid_t pid;
  int i;

  pid = fork ();
  if (pid < 0)
    {
      perror ("Could not fork");
    }
  if (pid > 0)
    {
      exit (EXIT_SUCCESS);
    }

  setsid ();

  pid = fork ();
  if (pid < 0)
    {
      perror ("Could not fork");
    }
  if (pid > 0)
    {
      exit (EXIT_SUCCESS);
    }

  for (i = getdtablesize (); i >= 0; i--)
    {
      close (i);
    }

  i = open ("/dev/null", O_RDWR);
  dup (i);
  dup (i);

/*   umask (022); */
/*   chdir ("/tmp"); */
}
