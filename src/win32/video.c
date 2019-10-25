#include <Windows.h>
#include "log.h"
#include "cpu.h"
#include "internal_video.h"




uint8_t unes_pad_states[2];


static joykey_t unes_key_map[] = {
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT
};

static uint8_t windows_key_map[] = {
	0x5A, // Z
	0x58, // X
	VK_SPACE,
	VK_RETURN,
	VK_UP,
	VK_DOWN,
	VK_LEFT,
	VK_RIGHT
};

static HWND hwnd_mainwin;
static WNDCLASS wc;
static PAINTSTRUCT paintstruct;
static MSG msg_mainwin;
static BOOL wm_destroy_request = 0;
static DWORD win_width;
static DWORD win_height;

static HDC hdc_mainwin;
static const BITMAPINFO bmi = {
	.bmiHeader = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biWidth = NES_SCR_WIDTH,
		.biHeight = -NES_SCR_HEIGHT,
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
		.biSizeImage = 0,
		.biXPelsPerMeter = 0,
		.biYPelsPerMeter = 0,
		.biClrUsed = 0,
		.biClrImportant = 0
	}
};

static const uint32_t nes_rgb[0x40] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

static uint32_t* rgb_buffer = NULL;


static void window_size_update(void)
{
	RECT rect;
	GetClientRect(hwnd_mainwin, &rect);
	win_width = rect.right - rect.left;
	win_height = rect.bottom - rect.top;
}

static void update_pad_states(const WPARAM vk, const key_state_t state)
{
	for (int i = 0; i < KEY_NKEYS; ++i) {
		if (windows_key_map[i] == vk) {
			const joykey_t key = unes_key_map[i];
			unes_pad_states[0] &= ~(0x01<<key);
			unes_pad_states[0] |= state<<key;
		}
	}
}


static LRESULT window_proc_clbk(HWND hwnd,
                                UINT msg,
                                WPARAM wParam,
                                LPARAM lParam)
{
	switch (msg) {
		case WM_SIZE:
			window_size_update();
			break;
		case WM_KEYDOWN:
			update_pad_states(wParam, KEYSTATE_DOWN);
			break;
		case WM_KEYUP:
			update_pad_states(wParam, KEYSTATE_UP);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			wm_destroy_request = 1;
			return WM_DESTROY;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

BOOL init_video_system(const HINSTANCE hInstance,
                       const int nCmdShow)
{
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc   = window_proc_clbk;
	wc.hInstance     = hInstance;
	wc.lpszClassName = "unes";

	RegisterClass(&wc);

	hwnd_mainwin = CreateWindowEx(
			0, wc.lpszClassName,
			"UNES", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, wc.hInstance, NULL);
	if (!hwnd_mainwin) {
		log_info("Failed to initialize WINDOW HWND");
		return 0;
	}

	ShowWindow(hwnd_mainwin, nCmdShow);
	UpdateWindow(hwnd_mainwin);
	window_size_update();

	hdc_mainwin = GetDC(hwnd_mainwin);

	rgb_buffer = malloc(sizeof(*rgb_buffer) * NES_SCR_WIDTH * NES_SCR_HEIGHT);

	return 1;
}

void term_video_system(void)
{
	free(rgb_buffer);
	DestroyWindow(hwnd_mainwin);
}

BOOL video_win_update(void)
{
	while (PeekMessageA(&msg_mainwin, hwnd_mainwin, 0, 0, PM_REMOVE))
		DispatchMessage(&msg_mainwin);

	return !wm_destroy_request;
}

void video_start_frame(void)
{
	BeginPaint(hwnd_mainwin, &paintstruct);
}

void video_internal_render(const uint8_t* const fb)
{
	// TODO: fix the upside down
	for (int y = 0; y < NES_SCR_HEIGHT; ++y) {
		for (int x = 0; x < NES_SCR_WIDTH; ++x) {
			rgb_buffer[
				NES_SCR_WIDTH * y + x
			] = nes_rgb[fb[y * NES_SCR_WIDTH + x]&0x3F];
		}
	}

	StretchDIBits(hdc_mainwin, 0, 0, win_width, win_height,
			0, 0, NES_SCR_WIDTH, NES_SCR_HEIGHT,
			rgb_buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

void video_end_frame(void)
{
	EndPaint(hwnd_mainwin, &paintstruct);
}

