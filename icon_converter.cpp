/*  icon_converter.cpp – Convert PNG to Haiku icon resource
 *
 *  Usage: icon_converter <input.png> <output.rsrc>
 *
 *  Reads a PNG file and creates a Haiku resource file with
 *  vector icon (VICN) and bitmap icons (ICON, MICN).
 */

#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include <GraphicsDefs.h>
#include <IconUtils.h>
#include <Path.h>
#include <Entry.h>
#include <Mime.h>

#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.png> <output.rsrc>\n", argv[0]);
        return 1;
    }

    const char* inputPath = argv[1];
    const char* outputPath = argv[2];

    // Initialize BApplication (needed for Translation Kit)
    BApplication app("application/x-vnd.icon-converter");

    // Load the PNG
    BBitmap* bitmap = BTranslationUtils::GetBitmap(inputPath);
    if (bitmap == nullptr) {
        fprintf(stderr, "Error: Could not load PNG file: %s\n", inputPath);
        return 1;
    }

    printf("Loaded PNG: %dx%d\n", (int)bitmap->Bounds().Width() + 1, (int)bitmap->Bounds().Height() + 1);

    // Create large icon (64x64) and mini icon (16x16)
    BBitmap largeIcon(BRect(0, 0, 63, 63), B_BITMAP_NO_SERVER_LINK, B_RGBA32);
    BBitmap miniIcon(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK, B_RGBA32);

    // Scale the source bitmap to icon sizes
    // First, we need to create server-linked bitmaps for drawing
    BBitmap* tempLarge = new BBitmap(BRect(0, 0, 63, 63), B_RGBA32);
    BBitmap* tempMini = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);

    // Use BTranslationUtils to scale (it handles this via view drawing internally)
    // Actually, let's do manual scaling with a simple nearest-neighbor approach

    // Scale to 64x64
    uint8* srcBits = (uint8*)bitmap->Bits();
    uint8* dstBits = (uint8*)tempLarge->Bits();
    int srcW = bitmap->Bounds().IntegerWidth() + 1;
    int srcH = bitmap->Bounds().IntegerHeight() + 1;
    int srcBPR = bitmap->BytesPerRow();
    int dstBPR = tempLarge->BytesPerRow();

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int sx = (x * srcW) / 64;
            int sy = (y * srcH) / 64;
            uint8* src = srcBits + sy * srcBPR + sx * 4;
            uint8* dst = dstBits + y * dstBPR + x * 4;
            dst[0] = src[0];  // B
            dst[1] = src[1];  // G
            dst[2] = src[2];  // R
            dst[3] = src[3];  // A
        }
    }

    // Scale to 16x16
    dstBits = (uint8*)tempMini->Bits();
    dstBPR = tempMini->BytesPerRow();
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int sx = (x * srcW) / 16;
            int sy = (y * srcH) / 16;
            uint8* src = srcBits + sy * srcBPR + sx * 4;
            uint8* dst = dstBits + y * dstBPR + x * 4;
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
        }
    }

    // Create the resource file
    BFile file(outputPath, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) {
        fprintf(stderr, "Error: Could not create output file: %s\n", outputPath);
        delete bitmap;
        return 1;
    }

    BResources res(&file);

    // Add large icon (ICON type = 'ICON', ID 101)
    // The ICON format is B_CMAP8 (256 colors, 64x64 = 4096 bytes)
    // We need to convert RGBA32 to CMAP8
    BBitmap* cmapLarge = new BBitmap(BRect(0, 0, 63, 63), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
    BIconUtils::ConvertToCMAP8((uint8*)tempLarge->Bits(), 64, 64, tempLarge->BytesPerRow(), cmapLarge);
    res.AddResource('ICON', 101, cmapLarge->Bits(), (size_t)cmapLarge->BitsLength(), "Large icon");

    // Add mini icon (MICN type = 'MICN', ID 101)
    // MICN is 16x16 in B_CMAP8 = 256 bytes
    BBitmap* cmapMini = new BBitmap(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
    BIconUtils::ConvertToCMAP8((uint8*)tempMini->Bits(), 16, 16, tempMini->BytesPerRow(), cmapMini);
    res.AddResource('MICN', 101, cmapMini->Bits(), (size_t)cmapMini->BitsLength(), "Mini icon");

    // Also add the raw PNG data as a 'PNG ' resource so the app can use it
    // Read the PNG file
    FILE* pngFile = fopen(inputPath, "rb");
    if (pngFile) {
        fseek(pngFile, 0, SEEK_END);
        size_t pngSize = (size_t)ftell(pngFile);
        fseek(pngFile, 0, SEEK_SET);
        uint8* pngData = new uint8[pngSize];
        fread(pngData, 1, pngSize, pngFile);
        fclose(pngFile);
        res.AddResource('PNG ', 101, pngData, pngSize, "App icon PNG");
        delete[] pngData;
    }

    printf("Created icon resources: %s\n", outputPath);

    delete bitmap;
    delete tempLarge;
    delete tempMini;
    delete cmapLarge;
    delete cmapMini;

    return 0;
}
