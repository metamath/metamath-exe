/*****************************************************************************/
/*        Copyright (C) 2005  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#include <string.h>

/***********************************/
/*  Macintosh interface functions  */
/***********************************/

#ifdef THINK_C

#include "mmmaci.h"

#define kBaseResID 128
#define kMoveToFront (WindowPtr)-1L

#define kHorizontalPixel 30
#define kVerticalPixel 50



/* Macintosh tool box initialization */
void ToolBoxInit(void)
{
  InitGraf(&thePort); /*??? Crashes console interface */
  InitFonts();
  InitWindows();
  InitMenus();
  TEInit();
  InitDialogs(nil);
  InitCursor();
}

/* Macintosh window initialization */
void WindowInit(void)
{
  WindowPtr window;
  window = GetNewWindow(kBaseResID, nil, kMoveToFront);
  if (window == nil)
  {
    SysBeep(10); /* Couldn't load the WIND resource!!! */
    ExitToShell();
  }

  ShowWindow(window);
  SetPort(window);
  /* MoveTo(kHorizontalPixel, kVerticalPixel); */
  /* DrawString("\pHello, world!"); */
}

/* Draw the opening window */
void DrawMyPicture(void)
{
  Rect pictureRect;
  WindowPtr window;
  PicHandle picture;

  window = FrontWindow();
  pictureRect = window->portRect;
  picture = GetPicture(kBaseResID);

  if (picture == nil) {
    SysBeep(10); /* Couldn't load the PICT resource!!! */
    ExitToShell();
  }

  CenterPict(picture, &pictureRect);
  DrawPicture(picture, &pictureRect);
}

/* Center picture */
void CenterPict(PicHandle picture, Rect *destRectPtr)
{
  Rect windRect, pictRect;
  windRect = *destRectPtr;
  pictRect = (**(picture)).picFrame;
  OffsetRect(&pictRect, windRect.left - pictRect.left,
      windRect.top - pictRect.top);
  OffsetRect(&pictRect, (windRect.right - pictRect.right)/2,
     (windRect.bottom - pictRect.bottom)/2);
  *destRectPtr = pictRect;
}

#endif /* end ifdef THINK_C */
