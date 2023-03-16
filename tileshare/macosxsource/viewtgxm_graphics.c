/* viewtgxm_graphics.c */

#include "viewtgxm.h"
/* ======================================================================== */
void paintWindowTiles(WindowRef window,               /* window to paint to */
                      struct mapControl *mc) {          /* map control data */
   int i, j;
   int iWest, iEast, jSouth, jNorth;                   /* tile index bounds */
   int iXDest, iYDest;                           /* destination coordinates */
   int ixt;                                      /* tile pixel buffer index */
   CGrafPtr port;                                            /* window port */
   CGRect dr;                                      /* destination rectangle */
   CGColorSpaceRef cs;                              /* color specifications */
   CGContextRef gc;                              /* window graphics context */
   CGDataProviderRef pr;                           /* image "data provider" */
   CGImageRef ti;                                          /* tileset image */
   OSStatus status;
/* ------------------------------------------------------------------------ */
   windowTiles(mc, &iWest, &iEast, &jSouth, &jNorth);
   DBG_TRACE((stderr, "Painting, edge tiles: columns:%d to %d, rows: %d to %d\n", iWest, iEast, jSouth, jNorth));

   port = GetWindowPort(window);
/* SetPort(port); */

   cs = CGColorSpaceCreateDeviceRGB();
   status = QDBeginCGContext(port, &gc);
   dr.size.width = (float)(mc->tsh->tileWidth);        /* tile size as seen */
   dr.size.height = (float)(mc->tsh->tileHeight);           /* by CGContext */
/* status = SyncCGContextOriginWithPort(gc, port); */      
 
   for (j = jSouth; j <= jNorth; j++) {
      for (i = iWest; i <= iEast; i++) {       /* tile i, j must be painted */
/*       DBG_TRACE((stderr, "Tile column: %d row: %d.\n", i, j)); */
         for (ixt = 0; ixt < TILES_IN_MEM; ixt++) {     /* is it in memory? */
            if (mc->tile[ixt].loadCounter == 0) continue;    /* virgin slot */
            if ((mc->tile[ixt].column == i) && (mc->tile[ixt].row == j)) break; /* found it */
            }
         if (ixt == TILES_IN_MEM) ixt = loadTile(mc, i, j); /* get it from file */
         if (ixt == -1) {                 /* unexpectedly, failed to do so? */
            putAlert(makeMessage("Unexpected error: unable to access tile %d %d", i, j), ALERT_CONTINUE); 
            status = QDEndCGContext(port, &gc);
            CGColorSpaceRelease(cs);
            return;
            }
/*       DBG_TRACE((stderr, "from memory buffer slot: %d", ixt)); */
         ts2win(&mc->winRect, &mc->crcTile, i * mc->tsh->tileWidth, j * mc->tsh->tileHeight + mc->tsh->tileHeight, &iXDest, &iYDest);
         dr.origin.x = (float)iXDest;
/*       Destination y coordinates are "normal" window relative; they must be adjusted for CGContext */ 
         dr.origin.y = (float)(mc->winRect.top + mc->winRect.bottom - mc->tsh->tileHeight - iYDest);
/*       DBG_TRACE((stderr, "row %3d iYDest %3d GCy %g.\n", j, iYDest, dr.origin.y)); */ 
         pr = CGDataProviderCreateWithData(NULL, mc->pixelBuffer + ixt * mc->tileSize, mc->tileSize, NULL);
         ti = CGImageCreate(mc->tsh->tileWidth, mc->tsh->tileHeight, 8, 24, mc->sLineSize, cs, kCGImageAlphaNone, pr, NULL, 0, kCGRenderingIntentDefault);
         CGDataProviderRelease(pr);
         CGContextDrawImage(gc, dr, ti);
/*       paintTilePixels(window, mc->pixelBuffer + ixt * mc->tileSize, mc->tsh->tileWidth, mc->tsh->tileHeight, iXDest, iYDest); */
         CGImageRelease(ti);
         }
      }
   
   CGContextSynchronize(gc);                           /* is this necessary */
   status = QDEndCGContext(port, &gc);
   CGColorSpaceRelease(cs);
   return;
   }
/* ======================================================================== */
/* just for reference and testing purposes. Must disable CGContext to work. */
void paintTilePixels(WindowRef window,                /* window to paint to */
               unsigned char *pu,  	 /* tile to paint bytes, rgbrgbrgrgb... */
               int width, int height,            /* size of above in pixels */ 
               int x, int y) {                           /* painting origin */
   int i, j;            
   struct RGBColor px;                
/* ------------------------------------------------------------------------ */
   for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
         px.red = 256 * *pu++;
         px.green = 256 * *pu++;
         px.blue = 256 * *pu++;
         SetCPixel(x + j, y + i, &px);
         }
      }
   }
/* ======================================================================== */
