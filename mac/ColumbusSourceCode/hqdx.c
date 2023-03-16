/* This file implements "Hipparchus QuickDraw Extensions".
   This application needs only one: a polyline. Others TBI.
 */
#include "columbus.h"

/* ====================================================================== */
void HQDXsetpixrgb(struct dpxl *pixa, 
                   unsigned short r, 
                   unsigned short g, 
                   unsigned short b) {
   struct RGBColor c;
/* ---------------------------------------------------------------------- */
   c.red = r;
   c.green = g;
   c.blue = b;
   SetCPixel(pixa->pxl_x, pixa->pxl_y, &c);
   }
/* ====================================================================== */
void HQDXsetpixel(struct dpxl *pixa, 
                  long c) {
   }
/* ====================================================================== */
void HQDXpolyline(struct dpxl *pixa, int n) {
   MoveTo(pixa->pxl_x, pixa->pxl_y);
   while(--n) {
      pixa++;
      LineTo(pixa->pxl_x, pixa->pxl_y);   
      }
   }
/* ====================================================================== */
void HQDXfill3(struct dpxl *pixa) { /* no triangle fills done in Columbus */
   }
/* ====================================================================== */
void HQDXfill4(struct dpxl *pixa) {       /* no trapezoidal fills, either */
   }
/* ====================================================================== */
void HQDXfillpoly(struct dpxl *pixa, int n) { /* no polygon fills, either */
   }
/* ====================================================================== */
