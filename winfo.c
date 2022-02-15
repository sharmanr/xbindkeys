/* winfo -- window information */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

int debug = 0;

/* Find a window with given property starting from given window.
 * If this window doesn't have that property, check its children.
 * Return NULL if no suitable window was found (or error found).
 */
Window find_window_with_property (Display* display, Window window, Atom property)
{

  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  void *prop = NULL;
  int status;

  if (debug)
    printf ("find_window_with_property, looking at window 0x%lx\n", window);

  status = XGetWindowProperty (display, window,
			       property,
			       0,       /* offsetin property in 32-bit quantites */
			       0,      /* length in 32 bit quantities */
			       False,   /* do not delete! */
			       AnyPropertyType, &actual_type,
			       &actual_format, &nitems, &bytes_after,
			       (unsigned char**) &prop);
  if (status == BadWindow)
    {
      fprintf (stderr, "window id 0x%lx does not exists!", window);
      return 0;
    }
  else if (status != Success)
    {
      fprintf (stderr, "XGetWindowProperty failed!");
      return 0;
    }
  if (prop)
    XFree(prop);
  if (actual_type != None)
    {
      if (debug)
	printf("Found window!  Actual_type = %d\n", (int)actual_type);
      return window;
    }
  
  /* Not found,  check children. */

  Window root, parent;
  Window* children;
  unsigned int n;
  Window found_window;
  
  XQueryTree (display, window, &root, &parent, &children, &n);
  found_window = 0;
  if (children != NULL)
    {
      if (debug)
	printf ("There are %d children to check...\n", n);
      for (int i = 0; i < n; i++)
	{
	  if (debug)
	    printf ("  Checking window 0x%x\n", (unsigned int) children[i]);
	  found_window = find_window_with_property (display, children[i], property);
	  if (found_window)
	    {
	      if (debug)
		printf ("Found it! 0x%lx\n", found_window);
	      break;
	    }
	}
      XFree (children);
    }
  return found_window;
}

// ------------------------------------------------------------

/* Get a character property for a window.
 * If found, it must be XFree'd after use.
 * If not found (including errors), NULL is returned.
 */
char * char_property (Display* display, Window window, char * property)
{
  Atom atom;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  void *prop = NULL;
  int status;
  char *p;
  int i;
  
  atom = XInternAtom(display, property, False);
  if (atom == None) {
    fprintf(stderr, "Cannot intern atom %s\n", property);
    return NULL;
  }
  status = XGetWindowProperty (display, window,
			       atom,
			       0,       /* offsetin property in 32-bit quantites */
			       20,      /* length in 32 bit quantities */
			       False,   /* do not delete! */
			       AnyPropertyType, &actual_type,
			       &actual_format, &nitems, &bytes_after,
			       (unsigned char**) &prop);
  if (status == BadWindow)
    {
      fprintf (stderr, "window id 0x%lx does not exists!", window);
      return NULL;
    }
  else if (status != Success)
    {
      fprintf (stderr, "XGetWindowProperty failed!");
      return NULL;
    }
  /* Replace embedded NULs into commas, like xprop does. */
  p = prop;
  for (i = 0; i < (int)nitems - 1; i++)
    {
      if (*p == '\0')
	{
	  *p = ',';
	}
      p++;
    }
  return prop;
}

Window	client_window (Display* display, Window window)
{
  Atom atom;
  Window client;
  
  atom = XInternAtom(display, "WM_STATE", False);
  if (atom == None) {
    fprintf(stderr, "Cannot intern atom WM_STATE\n");
    return 0;
  }
  if (debug) 
    fprintf(stderr, "Looking for client window for windowid 0x%x\n", (int)window);
  client = find_window_with_property(display, window, atom);
  return client;
}



/* Return the WM_CLASS property which must be XFreed' or NULL */
char * window_class (Display* display, Window window)
{
  char *string;

  string = char_property (display, window, "WM_CLASS");
  return string;
}

char * window_name (Display* display, Window window)
{
  char *string;

  string = char_property (display, window, "WM_NAME");
  return string;
}

char * window_command (Display* display, Window window)
{
  char *string;

  string = char_property (display, window, "WM_COMMAND");
  return string;
}


