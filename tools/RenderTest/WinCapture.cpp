// Finds where a window's client area is on screen, so ffmpeg can record exactly that region.
#include "WinCapture.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

bool getClientScreenRect (void* hwnd, int& x, int& y, int& w, int& h)
{
    RECT rc;
    if (! GetClientRect ((HWND) hwnd, &rc))
        return false;
    POINT topLeft { 0, 0 };
    ClientToScreen ((HWND) hwnd, &topLeft);
    x = topLeft.x;
    y = topLeft.y;
    w = rc.right - rc.left;
    h = rc.bottom - rc.top;
    return true;
}
#else
bool getClientScreenRect (void*, int&, int&, int&, int&) { return false; }
#endif
