/* columbus.h */

#if !defined COLUMBUS_H
#define COLUMBUS_H

#include <Carbon/Carbon.h>
/* redundantly before hipp*.h - to avoid _POSIX_SOURCE precomp warning */
#include <unistd.h>

#if !defined (_POSIX_SOURCE)
#define _POSIX_SOURCE
#endif

#include "hipparch.h"
#include "hippttys.h"
#include "hippdraw.h"

#define COAST_SGM_FN     "coast.sgm"
#define COAST_VTX_FN     "coast.vtx"
#define WGS_84                   34
#define DISP_UNDEF            32767
#define MIN_SCALE             750e6                    /* small-scale limit */
#define MAX_SCALE               5e6                    /* large-scale limit */

#define INITIAL_PROJECTION  WWVIEW
#define INITIAL_SCALE          100.0e6
#define INITIAL_LAT              0.0
#define INITIAL_LNG            -90.0

#define columbusAppSignature 'c65x'

#define buildDateControlId     103

#define columbusMenuId           1
#define windowsMenuId            2
#define projectionMenuId         3
#define scaleMenuId              4
#define infoMenuId               5

struct columbusControl {              /* system-wide data, one per Columbus */
   WindowRef infoWindow;                                     /* info window */
   WindowRef aboutWindow;                                   /* about window */
   double screen_unit_x, screen_unit_y;               /* pixel size, meters */
   struct ellprms ellprm;                           /* ellipsoid parameters */
   cstLnFph cstsFp;                             /* coastline file, segments */
   cstLnFph cstvFp;                             /* coastline file, vertices */
   };

struct mapControl {         /* map window specific data, one per map window */
   struct columbusControl *cd;               /* pointer to system-wide data */
   WindowRef mapWindow;             /* doubly linked with main (map) window */
   Rect winRect;                        /* current window drawing rectangle */
   Point winOrig;                      /* port origin in global coordinates */
   struct window_blk *wdw;                         /* Hipparchus map-window */
   };

/* All functions starting with a capital letter (for example,
   'CreateNibReference') are Apple; those starting with lover-case
   letters are Hipparchus Library functions if their prefix is
   h1 to h7 (for example, 'h4_SetEllipsoidParameters'), and Hipparchus
   Auxiliarry Library if the prefix is h0 or h9 (for example,
   'h9_InitWindow'). All other functions that start with lower-case
   letters (for example, 'initColumbus') belong to the Columbus
   application. Their source is found in one of the files below:
*/

/* ccdata.c - Columbus initialization and termination functions */
int initColumbus(void);
const struct columbusControl *ccDP(void);
void closeColumbus(void);

/* events.c - Carbon "event handler" functions */
pascal OSStatus windowUpdateSizePosition(EventHandlerCallRef, EventRef, void *);
pascal OSStatus drawWindowContent(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mapWindowClose(EventHandlerCallRef, EventRef, void *);
pascal OSStatus aboutInfoClose(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mouseWheelMoved(EventHandlerCallRef, EventRef event, void *);
pascal OSStatus mapLocationClick(EventHandlerCallRef, EventRef event, void *);
pascal OSStatus menuUpdate(EventHandlerCallRef, EventRef, void *);
pascal OSStatus appMenuCommand(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mapMenuCommand(EventHandlerCallRef, EventRef, void *);

/* geometry.c - Map geometry (projection, scale, window-size) functions */
int initWindowMap(WindowRef);
int updateGeometrySize(struct mapControl *, int, int);
int updateGeometryProjection(struct mapControl *, int);
int updateGeometryScale(struct mapControl *, double);
int updateGeometryCenter(struct mapControl *, int, int);
int mapGeometry(struct mapControl *, int, struct s_vct3 *, double, int, int);
void setGridDensity(double, int *, int *);

/* cristof.c - Other / general Columbus functions */
int newMapWindow(IBNibRef);
int ancillaryWindows(IBNibRef);
void setBuildDate(WindowRef, char *);
void setMapWindowTitle(WindowRef, int);
void unexpectedError(char *, int);

/* info.c - display info strings */
pascal OSStatus displayInfo(EventHandlerCallRef, EventRef, void *);

/* Hipparchus QuichDraw Extensions (in hqdx.c) are prototyped in hippgui.h */
#endif
