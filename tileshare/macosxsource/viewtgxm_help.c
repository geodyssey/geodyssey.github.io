/* viewtgxm_help.c - draw info text in info window */

#include "viewtgxm.h"

static char para1[] =
 "The purpose of this program (named 'viewtgxm') is to provide sample "
 "Mac OS X (Carbon) source code that performs graphical rendering and "
 "navigation of scanned map files in .tgxm format "
 "(also known as 'tilesets').";

static char para2[] =
 "The functionality of current version is limited to opening a number "
 "of map windows and allowing the operator to roam the territory "
 "covered by the tileset.";
  
static char para3[] =
 "To open a new map window use the File - Open... menu entry or its "
 "keyboard shortcut, then select the tileset file.";

static char para4[] =
 "If you click anywhere in the window, the map will be re-centered so "
 "that the clicked location becomes a new map center-point. Mouse wheel "
 "can also be used to scroll the window over the tileset. (Pressing "
 "the wheel will flip-flop the scroll direction from North/South to "
 "East/West and vice versa).";

static char para5[] =
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
   DBG_TRACE((stderr, "Info display requested, window: %d.\n", (int)userData));

   port = GetWindowPort((WindowRef)userData);
   SetPort(port);
   GetPortBounds(port, &rect);
   
/* GetFNum("\pPalatino", &fNum); */
   GetFNum("\pPapyrus", &fNum);
   TextFont(fNum);
   TextFace(bold);
   TextSize(18);
   MoveTo(110, 36);
   DrawString("\pTileSet Viewer");

   fNum = GetSysFont ();
   TextFont(fNum);
   TextFace(normal);
   TextSize(12);

   yc = 64;                                      /* vertical start position */
   xm = 16;                                                       /* margin */
   xl = rect.right - rect.left - 2 * xm;                 /* text line width */
   yl = 16;                                                 /* line "depth" */   
   yc = yl/2 + displayPara(para1, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para2, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para3, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para4, xm, yc, xl, yl);
   yc = yl/2 + displayPara(para5, xm, yc, xl, yl);
 
   TextFace(bold);
   MoveTo(xm, yc + yl);
   DrawString("\pPlease visit:  http://www.geodyssey.com/tileshare .");
    
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
