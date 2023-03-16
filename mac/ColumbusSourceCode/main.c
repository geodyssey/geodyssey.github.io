/* Columbus: A didactic program otlining the integration of
   Hipparchus Geographical Software toolkit into Mac OS X /
   Carbon GUI development environment.

   In order to concentrate on the essentials of window manipulation
   and graphical interface, the program employs only a subset of
   Hipparcus function and displays only the simplest of geographical
   objects: World coastlines, stored in a static coordinate file
   with its own internal spherical indexing. This requires two
   'resource' files: "coast.sgm" and "coast.vtx".

   (The technique used to perform this is very similar to the
   one outlined in Dr. Dobbs article - for a copy see
   http://www.geodyssey.com/papers/dobbs92.html.
   For a general introduction to Hipparchus see:
   http://www.geodyssey.com/papers/hlauto8.html).

   This program however includes all code elements that a Hipparchus
   application must put in place in order to establish an effective
   two-way coordinate pipeline between the ground and the display
   window in one of the Hipparchus canonical cartographic projections.
   The projections can be changed, window can be resized and centered
   on an arbitrary global location based on the screen pointer
   location, ground coordinates of pointed-to locations can be
   displayed, "unreasonable" geometry requests are detected and
   reported.

   The program source is structured as a Mac OS X Project Builder
   "project" and borrows from the "hello world" example supplied
   with it.

   See also comments in columbus.h
    
   Programmer: Hrvoje Lukatela, July 2002.
*/
#include "columbus.h"
     
int main(int argc, char* argv[]) {
   IBNibRef        nibRef;
   EventHandlerUPP handlerUPP;
   EventTargetRef  etRef;
   EventTypeSpec   eventSpec;
/* ------------------------------------------------------------------------ */
   if (initColumbus()) unexpectedError("Columbus initialization error", 1);
   if (CreateNibReference(CFSTR("main"), &nibRef)) unexpectedError("Unexpected program initialization error (1)", 1);
   if (SetMenuBarFromNib(nibRef, CFSTR("MenuBar"))) unexpectedError("Unexpected program initialization error (2)", 1);

   eventSpec.eventClass = kEventClassCommand;
   eventSpec.eventKind = kEventProcessCommand;
   handlerUPP = NewEventHandlerUPP(appMenuCommand);
   etRef = GetApplicationEventTarget();
   if (InstallEventHandler(etRef, handlerUPP, 1, &eventSpec, NULL, NULL)) unexpectedError("Unexpected program initialization error (3)", 1);

   eventSpec.eventClass = kEventClassMenu;
   eventSpec.eventKind = kEventMenuOpening;
   handlerUPP = NewEventHandlerUPP(menuUpdate);
   etRef = GetApplicationEventTarget();
   if (InstallEventHandler(etRef, handlerUPP, 1, &eventSpec, NULL, NULL)) unexpectedError("Unexpected program initialization error (4)", 1);
   
   if (newMapWindow(nibRef)) unexpectedError("Unexpected program initialization error (5)", 1);
   if (ancillaryWindows(nibRef)) unexpectedError("Unexpected program initialization error (6)", 1);
   DisposeNibReference(nibRef);

   RunApplicationEventLoop();
   closeColumbus();

   return(noErr);
   }
