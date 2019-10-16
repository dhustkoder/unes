#include <Windows.h>
#include "log.h"
#include "internal_video.h"

static HWND hwnd;
static const wchar_t WND_CLASS_NAME[]  = L"unes";
static PAINTSTRUCT paintstruct;
static MSG msg;

static LRESULT window_proc_clbk(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
    switch(msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
			return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

BOOL init_video_system(HINSTANCE hInstance)
{
	// Register the window class.
	
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));

	wc.lpfnWndProc   = window_proc_clbk;
	wc.hInstance     = hInstance;
	wc.lpszClassName = WND_CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.

	hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		WND_CLASS_NAME,                     // Window class
		"UNES",    // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		wc.hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL) {
		log_info("Failed to initialize WINDOW HWND");
		return 0;
	}

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);
	
	return 1;
}

void term_video_system(void)
{
	DestroyWindow(hwnd);
}


BOOL video_win_update(void)
{
	const DWORD ret = GetMessage(&msg, NULL, 0, 0);
	DispatchMessage(&msg);
	return ret != 0;
}

void video_start_frame(void)
{
	BeginPaint(hwnd, &paintstruct);
}

void video_end_frame(void)
{
	EndPaint(hwnd, &paintstruct);
}

