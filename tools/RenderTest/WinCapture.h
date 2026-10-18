#pragma once

// Windows-only helper for recording the plugin window (returns false elsewhere).
bool getClientScreenRect (void* hwnd, int& x, int& y, int& w, int& h);   // physical pixels
