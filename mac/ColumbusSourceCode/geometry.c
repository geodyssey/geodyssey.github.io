/* geometry.c: Hipparchus map geometry functions used by columbus program.  */
#include "columbus.h"
/* ======================================================================== */
int initWindowMap(WindowRef window) {
   Rect rect;
   CGrafPtr port;
   struct mapControl *mc;
   int istat;
   struct s_vct3      center;
   struct e_ltln ell_fl;
   const struct columbusControl *cc = ccDP();
   /* ------------------------------------------------------------------------ */
   mc = (struct mapControl *)GetWRefCon(window);
   port = GetWindowPort(window);
   GetPortBounds(port, &rect);
   mc->winRect = rect;
   /* fprintf(stderr, "Initial window size to %d %d %d %d\n", rect.top, rect.left, rect.bottom, rect.right); */
   SetPort(port);
   mc->winOrig.v = mc->winOrig.h = 0;
   LocalToGlobal(&mc->winOrig);
   /* fprintf(stderr, "Initial global window origin %d %d\n", mc->winOrig.v, mc->winOrig.h); */
   ell_fl.lat = DEG2RAD * INITIAL_LAT;
   ell_fl.lng = DEG2RAD * INITIAL_LNG;
   h4_EllipsoidToHpsph(&cc->ellprm, &ell_fl, &center);
   istat = mapGeometry(mc, INITIAL_PROJECTION, &center, INITIAL_SCALE, rect.right - rect.left, rect.bottom - rect.top);
   return(istat);
   }
/* ======================================================================== */
int updateGeometrySize(struct mapControl *mc, int ix, int iy) {
   int istat;
/* ------------------------------------------------------------------------ */
   istat = mapGeometry(mc, mc->wdw->projection, &mc->wdw->map_prm->center, mc->wdw->dpScale, ix, iy);
   if (istat) H0_Beep(30);
   return(istat);
   }
/* ======================================================================== */
int updateGeometryProjection(struct mapControl *mc, int projection) {
   int istat;
/* ------------------------------------------------------------------------ */
   istat = mapGeometry(mc, projection, &mc->wdw->map_prm->center, mc->wdw->dpScale, mc->winRect.right - mc->winRect.left, mc->winRect.bottom - mc->winRect.top);
   if (istat) H0_Beep(30);
   return(istat);
   }
/* ======================================================================== */
int updateGeometryCenter(struct mapControl *mc, int ix, int iy) {
   int istat;
   struct dpxl pixa;
   struct plpt plloc;
   struct s_vct3 center;
   int projection;
   /* ------------------------------------------------------------------------ */
   pixa.pxl_x = (disp_coord)ix;
   pixa.pxl_y = (disp_coord)iy;
   h9_DisplayToPlane(&mc->wdw->wd, &pixa, &plloc); /* from display to map plane */
   if (h9_UnMapPoint(mc->wdw->map_prm, mc->wdw->projection, &plloc, &center)) {
/*    fprintf(stderr, "Click in window, but outside map area.\n"); */
      H0_Beep(30);
      return(1);
      }
   projection = mc->wdw->projection;
/* if (projection == WWVIEW) projection = STEREO; */
   istat = mapGeometry(mc, projection, &center, mc->wdw->dpScale, mc->winRect.right - mc->winRect.left, mc->winRect.bottom - mc->winRect.top);
   if (istat) H0_Beep(30);
/* else updateProjectionMenu(mc); */
   return(istat);
   }
/* ======================================================================== */
int updateGeometryScale(struct mapControl *mc, double scaleFactor) {
   CGrafPtr port;
   int istat;
   int projection;
   double scale;
/* ------------------------------------------------------------------------ */
   port = GetWindowPort(mc->mapWindow);
   if (QDDone(port) == FALSE) {           /* mouse wheel can be turned fast */
      H0_Beep(30);
/*    fprintf(stderr, "Previous map drawing not finished?\n"); */
      return(-1);
      }
   scale = mc->wdw->dpScale * scaleFactor;
   projection = mc->wdw->projection;
   if (projection == WWVIEW) {
      if (scaleFactor > 1.0) {
         scale = mc->wdw->dpScale;
         projection = ORTHO;
         }
      else projection = STEREO;
      }
   istat = mapGeometry(mc, projection, &mc->wdw->map_prm->center, scale, mc->winRect.right - mc->winRect.left, mc->winRect.bottom - mc->winRect.top);
   if (istat) H0_Beep(30);
/* else updateProjectionMenu(mc); */
   return(istat);
   }
/* ======================================================================== */
/* Function to recalculate map geometry; if OK it will update window block. */
   int mapGeometry(struct mapControl *mc,                       /* map data */
                   int projection,                        /* map projection */
                   struct s_vct3 *center,          /* center/tangency point */
                   double scale,                               /* map scale */
                   int ix, int iy) {                          /* map window */

   struct window_blk *newWdw;
   struct d_rect wdw_rect;
   int    istat;
   const struct columbusControl *cc = ccDP();
/* ------------------------------------------------------------------------ */
   if ((ix < 8) || (iy < 8)) return(1);
   if ((scale > MIN_SCALE) || (scale < MAX_SCALE)) return(1);

   wdw_rect.x_left = 0;
   wdw_rect.x_right = ix - 1;
   wdw_rect.y_top = 0;
   wdw_rect.y_bottom = iy - 1;
   newWdw = h1_Malloc(H9_WINDOW_BLK_SIZE(H0_POINT_BUFF_SIZE));

   istat = h9_InitWindow(NULL, NULL, NULL, &cc->ellprm,
                         projection, 0, center, NULL, scale,
                         cc->screen_unit_x, cc->screen_unit_y,
                         &wdw_rect, H0_POINT_BUFF_SIZE, newWdw);

   if (istat) {
      h9_CloseWindow(newWdw);                       /* impossible geometry? */
      h1_Free(newWdw);
      }
   else {
      if (mc->wdw) {
         if (mc->wdw->projection != NOPROJECTION) h9_CloseWindow(mc->wdw);
         h1_Free(mc->wdw);
         }
      mc->wdw = newWdw;
      }
   return(istat);
   }
/* ======================================================================== */
void setGridDensity(double scale, int *isphi, int *islam) {
   if      (scale >= 30e6)  *isphi = 36000, *islam = 54000;
   else if (scale >= 10e6)  *isphi = 18000, *islam = 36000;
   else if (scale >= 5e6)   *isphi =  7200, *islam = 18000;
   else if (scale >= 1e6)   *isphi =  3600, *islam =  7200;
   else if (scale >= 500e3) *isphi =  1200, *islam =  3600;
   else if (scale >= 100e3) *isphi =   600, *islam =  1200;
   else if (scale >= 50e3)  *isphi =   300, *islam =   600;
   else if (scale >= 10e3)  *isphi =    60, *islam =   120;
   else if (scale >=  5e3)  *isphi =    30, *islam =    60;
   else                     *isphi =    10, *islam =    30;
   return;
   }
/* ======================================================================== */
