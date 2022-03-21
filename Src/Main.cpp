// main.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "App.h"

HBITMAP win_hbm;

TCHAR window_class[] = _T(" ");


/******************************************************************************
*/
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK wndproc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		APP_paint();
		break;

	case WM_CLOSE:							// don't destroy it - just hide it
		ShowWindow(APP_hWnd, SW_HIDE);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

/******************************************************************************
** Function:	Console app entry point
**
** Notes:		
*/
int _tmain(int argc, _TCHAR* argv[])
{
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)wndproc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName = window_class;
	RegisterClass(&wc);

	APP_hWnd = CreateWindow(window_class, _T("Antplot"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, APP_SCREEN_WIDTH + 16, APP_SCREEN_HEIGHT + 16, NULL, NULL, wc.hInstance, NULL);

	// Get handle to display device context
	HDC hdc = GetDC(APP_hWnd);

	// Set up graphical display:
	APP_hdc = CreateCompatibleDC(hdc);
	win_hbm = CreateCompatibleBitmap(hdc, APP_SCREEN_WIDTH, APP_SCREEN_HEIGHT);
	(void)SelectObject(APP_hdc, win_hbm);

	DeleteDC(hdc);

	ShowWindow(APP_hWnd, SW_HIDE);
	UpdateWindow(APP_hWnd);

	APP_task();

	return 0;
}

