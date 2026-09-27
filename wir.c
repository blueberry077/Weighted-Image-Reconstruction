/*

	Weighted Image Reconstruction
	
	AUTHOR Marc-Daniel DALEBA
	DATE 2026-09-27
	DESC
		This program takes an input image, places random
		points of the image and uses weighted interpoliation
		to fill the gaps between previously places points.

*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WIN_WID (300)
#define WIN_HEI (400)

#define P_EXPONENT (3)            // Change the p exponent for influence (i: 3)
#define INI_POINTS (10000)        // Change the number of initial points (i: 10000)
#define PIXELS_PER_FRAMES (200)   // Change the number of pixels processed during a frame (i: 200)

LRESULT CALLBACK WindowProc(HWND h, UINT m, WPARAM w, LPARAM l);

typedef struct pt {
	int x, y;
	uint32_t color;
} pt;

BITMAPINFO bmi;
static HBITMAP hbitm;
uint32_t *dibits;
uint32_t *rbuf;
pt PTS[INI_POINTS];

float dist(int x0, int y0, int x1, int y1)
{
	float x = x0-x1;
	float y = y0-y1;
	return sqrt(x*x + y*y);
}

int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR lpCmdLine, int nShowCmd)
{
	// create window
	WNDCLASS wc = {};
	wc.lpszClassName = "Reconstruct Image";
	wc.hInstance = hInst;
	wc.lpfnWndProc = WindowProc;
	RegisterClass(&wc);
	
	RECT rc = {0, 0, WIN_WID, WIN_HEI};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindow(
		"Reconstruct Image", "Weighted Image Reconstruction", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right-rc.left, rc.bottom-rc.top,
		NULL, NULL, hInst, NULL);
	if (!hwnd) {
		return -1;
	}
	ShowWindow(hwnd, SW_NORMAL);
	
	// init video
	HDC hdc = GetDC(NULL);
	ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WIN_WID;
    bmi.bmiHeader.biHeight = -WIN_HEI;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
	hbitm = CreateDIBSection(
		hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&dibits, NULL, 0);
	ReleaseDC(NULL, hdc);
	if (!hbitm) {
		DestroyWindow(hwnd);
		return -1;
	}
	
	// load image & copy
	int w,h,n;
	unsigned char *data = stbi_load("parrots.png", &w, &h, &n, 4);
	if (!data) {
		DeleteObject(hbitm);
		DestroyWindow(hwnd);
		return -1;
	}
	int i = 0;
	uint32_t *data32 = (uint32_t*)data;
	for (i = 0; i < w*h; i++) {
		data32[i] =
			(data[(i*4)]   << 16) |
			(data[(i*4)+1] << 8 ) |
			(data[(i*4)+2]      ) |
			(data[(i*4)+3] << 24);
	}
	i = 0;
	for (int dy = 0; dy < h; ++dy) {
		for (int dx = 0; dx < w; ++dx) {
			dibits[dx + dy * WIN_WID] = data32[i];
			i++;
		}
	}
	
	// allocate reconstruction buffer & points
	size_t imgsize = w*h*4;
	rbuf = (uint32_t*)malloc(imgsize);
	if (!rbuf) {
		stbi_image_free(data);
		DeleteObject(hbitm);
		DestroyWindow(hwnd);
		return -1;
	}
	ZeroMemory(rbuf, imgsize);
	
	srand(time(NULL));
	for (int i = 0; i < INI_POINTS; ++i) {
		int x = rand()%300;
		int y = rand()%200;
		pt PT;
		PT.x = x;
		PT.y = y;
		PT.color = data32[x + y * WIN_WID];
		PTS[i] = PT;
	}
	stbi_image_free(data);
	
	// loop
	MSG msg = {};
	int running = 1;
	int x=0, y=0;
	while (running) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				running = 0;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// new pixel
		for (int i = 0; i < PIXELS_PER_FRAMES; ++i) {
			if (x != 0 || y < 200) {
				uint32_t color = 0;
				float d_j = 0.0f;
				for (int j = 0; j < INI_POINTS; ++j) {
					float d = dist(x, y, PTS[j].x, PTS[j].y);
					if (d != 0.0f) {
						d_j += 1.0f/(pow(d,P_EXPONENT));
					}
				}
				float r = 0.0f;
				float g = 0.0f;
				float b = 0.0f;
				int brk=0;
				for (int j = 0; j < INI_POINTS; ++j) {
					float d_i = dist(x, y, PTS[j].x, PTS[j].y);

					if (d_i == 0.0f) {
						color = PTS[j].color;
						brk=1;
						break;
					}

					float w_i = (1.0f / (pow(d_i,P_EXPONENT))) / d_j;
					uint32_t c = PTS[j].color;

					r += w_i * ((c >> 16) & 0xFF);
					g += w_i * ((c >> 8)  & 0xFF);
					b += w_i * (c & 0xFF);
				}
				if (!brk) {
					color = ((uint32_t)r << 16) |
							((uint32_t)g << 8) |
							(uint32_t)b;
				}
				rbuf[x + y * WIN_WID] = color;
				x++;
				if (x >= WIN_WID) {
					y++;
					x = 0;
				}
			}
		}
		
		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
		Sleep(16);
	}
	free(rbuf);
	DeleteObject(hbitm);
	DestroyWindow(hwnd);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		case WM_PAINT: {
			// copy rbuf
			int i = 0;
			for (int dy = 0; dy < 200; ++dy) {
				for (int dx = 0; dx < 300; ++dx) {
					dibits[dx + (dy + 200) * WIN_WID] = rbuf[i];
					i++;
				}
			}
		
			// draw
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(h, &ps);
			
			HDC memory_dc = CreateCompatibleDC(hdc);
			HBITMAP old_bitmap = SelectObject(memory_dc, hbitm);
			BitBlt(
				hdc, 0, 0, WIN_WID, WIN_HEI,
				memory_dc, 0, 0, SRCCOPY
			);
			SelectObject(memory_dc, old_bitmap);
			DeleteDC(memory_dc);
			
			EndPaint(h, &ps);
			break;
		}
		default:
			return DefWindowProc(h, m, w, l);
	}
	return 0;
}