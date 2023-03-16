/* info.c - draw info text in info window */
#include "columbus.h"

static char para1[] =
 "The purpose of this program (named 'Columbus') is to provide sample source "
 "code that performs Mac OS X (Carbon) graphical rendering for an application "
 "based on the Hipparchus Geographical Toolkit.";

static char para2[] =
 "Columbus' functionality is limited to opening a number of map windows " 
 "and allowing the operator to change their projection, scale and location "
 "on the seamless, spheroidal planetary surface. In order to keep things simple, "
 "the only geographical object it displays is a set of world coastlines.";

static char para3[] =
 "To open a new map window use the Window - Create New... menu entry or its "
 "keyboard shortcut.";

static char para4[] =
 "To change map projection of the active map window, "
 "use an appropriate item in the Projection menu.";

static char para5[] =
 "To change scale by a factor of two use the Scale menu.";

static char para6[] =
 "If the program runs on a computer with a 'wheel' mouse, turning of the "
 "wheel will also change the scale by approximately 10% for each 'notch'. "
 "This provides an (almost) continuous Zoom-In/Zoom-Out map display. ";

static char para7[] =
 "If you click anywhere in the window (within the area covered by the map), "
 "the map will be re-centered so that the clicked location becomes - "
 "depending on the projection - a new map center-point or central meridian.";

static char para8[] =
 "A map window can be moved and resized like any other 'document' window.";

static int displayPara(char *, int, int, int, int);

/* ======================================================================== */
pascal OSStatus displayInfo(EventHandlerCallRef handlerRef,
                            EventRef event,
                            void *userData) {
   CGrafPtr port;
   Rect rect;
   short int fNum;
   int xl, xm, yl, yc;
/* ------------------------------------------------------------------------ */
   port = GetWindowPort((WindowRef)userData);
   SetPort(port);
   GetPortBounds(port, &rect);
   
   GetFNum("\pPalatino", &fNum);
   TextFont(fNum);
   TextFace(bold + italic);
   TextSize(18);
   MoveTo(102, 36);
   DrawString("\pHello (Round) World!");

   fNum = GetSysFont ();
   TextFont(fNum);
   TextFace(normal);
   TextSize(12);

   yc = 64;                                      /* vertical start position */
   xm = 12;                                                       /* margin */
   xl = rect.right - rect.left - xm - xm;                /* text line width */
   yl = 16;                                                 /* line "depth" */   
   yc = yl/2 + displayPara(para1, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para2, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para3, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para4, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para5, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para6, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para7, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para8, xm, yc, xl, yl);

   TextFace(bold);
   MoveTo(xm, yc);
   DrawString("\pFor details, please see: http://www.geodyssey.com/ .");
    
/* fprintf(stderr, "Making Info...\n"); */
   return(noErr);
   }
/* ======================================================================== */
/* A simple text-panel display function - future use for map text display.  */
static int displayPara(char *paraText,                   /* text to display */
                       int xo, int yo,                   /* start at origin */
                       int xl,                      /* line width in pixels */
                       int yl) {                              /* line depth */
   int i, j, jj, l, m;
   Point p = {1, 1};
/* ------------------------------------------------------------------------ */
/* fprintf(stderr, "Drawing paragraph, %d %d %d %d\n", xo, yo, xl, yl); */ 
   i = 0;
   l = strlen(paraText);              
   while (i < l) {                        /* draw lines while any text left */
      jj = i;                                   /* next line-break position */
      for (j = i; j <= l; j++) {     /* search for next space to break line */
         if (paraText[j] == ' ') {
            m = CharToPixel(paraText + i, j - i, (Fixed)0.0, j - i, (SInt16)0, onlyStyleRun, p, p);
/*          fprintf(stderr, "Line break check at %d, length %d (%d)\n", j, m, xl);  */
            if ((jj == i) || (m <= xl)) jj = j;
            if (m > xl) break;
            } 
         else if (paraText[j] == '\0') {
            jj = j;
            break;
            }
         }
/*    fprintf(stderr, "About to draw from %d to %d\n", i, jj); */ 
      MoveTo(xo, yo);
      DrawText(paraText, i, jj - i);
      i = jj + 1;
      yo += yl;
      }
   return(yo);                   /* Return vertical origin of the next line */
   }
/* ======================================================================== */
