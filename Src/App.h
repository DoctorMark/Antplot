/******************************************************************************
** File:	App.h
**
** Notes:
*/

#define APP_VERSION_STRING	"1.0"

#define APP_SCREEN_WIDTH	1536
#define APP_SCREEN_HEIGHT	1024

extern HDC APP_hdc;

extern HWND APP_hWnd;

void APP_task(void);
void APP_paint(void);

