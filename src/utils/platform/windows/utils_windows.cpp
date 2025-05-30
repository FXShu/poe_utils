#include "utils.hh"
#include <windows.h>
cv::Mat utils::screenshot(void) {
	/* capture primary screen */
	HDC hScreenDC = GetDC(nullptr);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
	SelectObject(hMemoryDC, hOldBitmap);

	BITMAP bmpScreen;
	GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

	cv::Mat mat(height, width, CV_8UC4); // 4 channels: BGRA
	GetBitmapBits(hBitmap, bmpScreen.bmHeight * bmpScreen.bmWidthBytes, mat.data);

	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(nullptr, hScreenDC);

	cv::Mat matBGR;
	cv::cvtColor(mat, matBGR, cv::COLOR_BGRA2BGR);
	return matBGR;
}
