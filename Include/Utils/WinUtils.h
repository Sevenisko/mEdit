#pragma once

#include <Windows.h>

static HICON CreateIconFromPNG(const char* filename, int iconWidth, int iconHeight) {
    unsigned char* image = NULL;
    unsigned width, height;
    unsigned error = lodepng_decode32_file(&image, &width, &height, filename);
    if(error) {
        debugPrintf("LodePNG error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    // Resize if necessary (simple nearest-neighbor for simplicity)
    unsigned char* resizedImage = NULL;
    if(width != (unsigned)iconWidth || height != (unsigned)iconHeight) {
        resizedImage = (unsigned char*)malloc(iconWidth * iconHeight * 4);
        if(!resizedImage) {
            free(image);
            debugPrintf("Failed to allocate resized image\n");
            return NULL;
        }

        // Basic scaling (nearest-neighbor)
        for(int y = 0; y < iconHeight; y++) {
            for(int x = 0; x < iconWidth; x++) {
                int srcX = (x * width) / iconWidth;
                int srcY = (y * height) / iconHeight;
                int srcIndex = (srcY * width + srcX) * 4;
                int dstIndex = (y * iconWidth + x) * 4;
                resizedImage[dstIndex + 0] = image[srcIndex + 0]; // R
                resizedImage[dstIndex + 1] = image[srcIndex + 1]; // G
                resizedImage[dstIndex + 2] = image[srcIndex + 2]; // B
                resizedImage[dstIndex + 3] = image[srcIndex + 3]; // A
            }
        }
        free(image);
        image = resizedImage;
        width = iconWidth;
        height = iconHeight;
    }

    // Create HBITMAP for color data (32-bit ARGB)
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = iconWidth;
    bmi.bmiHeader.biHeight = -iconHeight; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    void* bitmapBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bitmapBits, NULL, 0);
    if(!hBitmap) {
        debugPrintf("Failed to create HBITMAP\n");
        free(image);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    // Copy RGBA to ARGB (swap R and B, include alpha)
    unsigned char* dst = (unsigned char*)bitmapBits;
    for(unsigned i = 0; i < width * height; i++) {
        dst[i * 4 + 0] = image[i * 4 + 2]; // B
        dst[i * 4 + 1] = image[i * 4 + 1]; // G
        dst[i * 4 + 2] = image[i * 4 + 0]; // R
        dst[i * 4 + 3] = image[i * 4 + 3]; // A
    }

    // Create monochrome mask bitmap
    HBITMAP hMask = CreateBitmap(iconWidth, iconHeight, 1, 1, NULL);
    if(!hMask) {
        debugPrintf("Failed to create mask bitmap\n");
        DeleteObject(hBitmap);
        free(image);
        ReleaseDC(NULL, hdcScreen);
        return NULL;
    }

    // Generate mask (alpha < 128 = transparent)
    HDC hdcColor = CreateCompatibleDC(hdcScreen);
    HDC hdcMask = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcColor, hBitmap);
    SelectObject(hdcMask, hMask);
    for(int y = 0; y < iconHeight; y++) {
        for(int x = 0; x < iconWidth; x++) {
            unsigned index = (y * iconWidth + x) * 4;
            BYTE alpha = image[index + 3];
            SetPixel(hdcMask, x, y, alpha < 128 ? RGB(255, 255, 255) : RGB(0, 0, 0));
        }
    }

    // Create ICONINFO structure
    ICONINFO iconInfo = {0};
    iconInfo.fIcon = TRUE; // Icon, not cursor
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = hMask;
    iconInfo.hbmColor = hBitmap;

    // Create HICON
    HICON hIcon = CreateIconIndirect(&iconInfo);

    // Clean up
    DeleteDC(hdcColor);
    DeleteDC(hdcMask);
    ReleaseDC(NULL, hdcScreen);
    DeleteObject(hBitmap);
    DeleteObject(hMask);
    free(image);

    if(!hIcon) { debugPrintf("Failed to create HICON\n"); }

    return hIcon;
}