/* viewtgxm.h header for all source files of 'viewtgxm' project. */

#if !defined VIEWTGXM_H
#define VIEWTGXM_H 

#include <Carbon/Carbon.h>

/* redundantly before what follows - to avoid _POSIX_SOURCE precomp warning */
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#if !defined (_POSIX_SOURCE)
#define _POSIX_SOURCE
#endif

#include "tileset.h"
#include <zlib.h>
#include <jpeglib.h> 

#define viewtgxmAppSignature 'c65y'

#if defined DEBUG_TRACE_ON
#define DBG_TRACE(args) fprintf args
#else 
#define DBG_TRACE(args) 
#endif

#define DRAW_PIXELS_A               1
#define DRAW_PIXELS_B               2
#define DRAW_GPSTRACE               4
#define DRAW_LATLNG                 8
#define DRAW_TILEBDY               16

#define INITIAL_MAP_POSITION_X    200
#define INITIAL_MAP_POSITION_Y     64

#define LINE_LENGTH            509         /* C standard (believe it or not) */

#define INFO_CONTINUE            1 
#define ALERT_CONTINUE           2
#define ALERT_TERMINATE          3

#define viewtgxmMenuId           1
#define fileMenuId               2
#define viewMenuId               3
#define GPSMenuId                4
#define infoMenuId               5

#define buildDateControlId     103

#define TILES_IN_MEM               64              /* tiles saved in memory */
#define LOAD_COUNTER_MAX   0xffffffff         /* maximum unsigned int value */

struct tileInMemory {                  /* corresponds to pixel buffer slots */
   int column, row;                              /* tile coordinates ("id") */
   unsigned loadCounter;              /* first-in-first-replaced (0: empty) */
   };

struct fileMapping {         /* POSIX file mapping params and tileset items */
   void *fileStart;          /* NULL if none open ("master file indicator") */
   int fileSize;                                                    /* size */
   int fd;                         /* "low level" open handle, internal use */
   };

struct viewtgxmControl {                    /* system-wide data, one viewer */
   WindowRef infoWindow;                                     /* info window */
   WindowRef aboutWindow;                                   /* about window */
   double screen_unit_x, screen_unit_y;               /* pixel size, meters */
   char tilesetFileName[FILENAME_MAX + 2];          /* tileset to be opened */
   NavEventUPP fnep;                         /* file-nav event callback ptr */
   };

struct mapControl {         /* map window specific data, one per map window */
   struct viewtgxmControl *vtc;            /* link back to system-wide data */
   WindowRef mapWindow;             /* doubly linked with main (map) window */
                                                              
   struct fileMapping fm;    /* tileset file mapping, and pointers into it: */
   const struct tsHdr *tsh;                         /* tile-set file header */
   const struct tsRect *tsrp;                     /* rectangular projection */
   const struct tsTile1 *tstt1;                 /* tile table, single layer */
   const struct tsTile2 *tstt2;                  /* tile table, both layers */
   const struct tsColor *tspltt;                         /* palette, if any */
   const struct tsColor *tstrsp;                    /* transparency, if any */
   const char *dscr_l1;                              /* description, line 1 */
   const char *dscr_l2;                              /* description, line 2 */
   int sLineSize;                            /* uncompressed scan line size */
   int tileSize;                                  /* uncompressed tile size */

   int drawFlags;                                  /* drawing: what to draw */
   Rect winRect;                        /* current window drawing rectangle */
   Point winOrig;                      /* port origin in global coordinates */
   struct tsPixel crcTile; /* ...its client rect. center in tileset coords. */
   int mouseWheelDirection;                 /* 0: North-South, 1: East-West */ 
   
   unsigned tileLoadCounter;                     /* used to stamp upon load */
   unsigned char *pixelBuffer;    /* for uncompressed tiles saved in memory */
   struct tileInMemory tile[TILES_IN_MEM]; /* description of tiles in above */

   };

/* viewtgxm_events.c - Event hanndlers */
pascal OSStatus appMenuCommand(EventHandlerCallRef, EventRef, void *);
pascal OSStatus aboutInfoClose(EventHandlerCallRef, EventRef, void *);
pascal void fileNavCallBack(NavEventCallbackMessage, NavCBRecPtr, void *);
pascal OSStatus windowSizeMove(EventHandlerCallRef, EventRef, void *);
pascal OSStatus drawWindowContent(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mapWindowClose(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mapLocationClick(EventHandlerCallRef, EventRef event, void *);
pascal OSStatus mouseWheelMoved(EventHandlerCallRef, EventRef, void *);
pascal OSStatus menuUpdate(EventHandlerCallRef, EventRef, void *);
pascal OSStatus mapMenuCommand(EventHandlerCallRef, EventRef, void *);                                 

/* viewtgxm_misc.c - Other / general viewtgxm functions */
struct viewtgxmControl *initViewtgxm(void);
struct viewtgxmControl *getVTC(void *);
int ancillaryWindows(IBNibRef);
void setBuildDate(WindowRef, char *);
char *makeMessage(char *, ...);
void putAlert(char *, int);
void closeViewtgxm(struct viewtgxmControl *);
void reportFile(struct mapControl *);
                 
/* viewtgxm_file.c - File manipulation functions */
int openMapWindow(IBNibRef);
int getTilesetFileName(void);
int loadTile(struct mapControl *, int, int);

/* viewtgxm_geometry.c - Geometry viewtgxm functions */
int windowLimits(const struct tsHdr *, const struct tsRect *, const struct Rect *, const struct tsPixel *, int *, int *, int *, int *);
int resizeWindow(struct mapControl *, struct Rect *); 
int positionWindow(struct mapControl *, struct tsLatLng *);
int windowTiles(struct mapControl *, int *, int *, int *, int *);
void win2ts(struct Rect *, struct tsPixel *, int, int, int *, int *);
void ts2win(struct Rect *, struct tsPixel *, int, int, int *, int *);
  
/* viewtgxm_graphics.c - Graphics functions viewtgxm functions */
void paintWindowTiles(WindowRef, struct mapControl *);
void paintTilePixels(WindowRef, unsigned char *, int, int, int, int);
void paintTile(WindowRef, unsigned char *, int, int, int, int);
                
/* viewtgxm_help.c - Other / general viewtgxm functions */
pascal OSStatus displayInfo(EventHandlerCallRef, EventRef, void *);
#endif
