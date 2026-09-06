#ifndef APPPLATFORM_EMSCRIPTEN_H__
#define APPPLATFORM_EMSCRIPTEN_H__

#include "AppPlatform.h"
#include "platform/log.h"
#include "client/renderer/gles.h"
#include "world/level/storage/FolderMethods.h"
#include <png.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <emscripten.h>

// Declarations of functions implemented in mc_platform.js (linked via --js-library)
extern "C" {
    extern void mcShowCreateWorldDialog();
    extern int  mcGetWorldStatus();
    extern char* mcGetWorldResult();  // caller must free()
    extern void mcSyncSaves();
}

class AppPlatform_emscripten: public AppPlatform
{
public:
    AppPlatform_emscripten() {}

	BinaryBlob readAssetFile(const std::string& filename) {
		FILE* fp = fopen(("/data/" + filename).c_str(), "rb");
		if (!fp) return BinaryBlob();
		int size = getRemainingFileSize(fp);
		BinaryBlob blob;
		blob.size = size;
		blob.data = new unsigned char[size];
		fread(blob.data, 1, size, fp);
		fclose(fp);
		return blob;
	}

    void saveScreenshot(const std::string& filename, int glWidth, int glHeight) {}

    TextureData loadTexture(const std::string& filename_, bool textureFolder)
	{
		TextureData out;
		std::string filename = textureFolder ? "/data/images/" + filename_ : filename_;
        FILE* fp = fopen(filename.c_str(), "rb");
		if (!fp) {
			LOGI("Couldn't find file: %s\n", filename.c_str());
			return out;
		}

		png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!pngPtr) { fclose(fp); return out; }

		png_infop infoPtr = png_create_info_struct(pngPtr);
		if (!infoPtr) { png_destroy_read_struct(&pngPtr, NULL, NULL); fclose(fp); return out; }

		png_init_io(pngPtr, fp);
		png_read_info(pngPtr, infoPtr);

		out.w = png_get_image_width(pngPtr, infoPtr);
		out.h = png_get_image_height(pngPtr, infoPtr);

		// Ensure RGBA output regardless of source format
		png_byte ct = png_get_color_type(pngPtr, infoPtr);
		png_byte bd = png_get_bit_depth(pngPtr, infoPtr);
		if (bd == 16) png_set_strip_16(pngPtr);
		if (ct == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(pngPtr);
		if (ct == PNG_COLOR_TYPE_GRAY && bd < 8) png_set_expand_gray_1_2_4_to_8(pngPtr);
		if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(pngPtr);
		if (ct == PNG_COLOR_TYPE_RGB || ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);
		if (ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(pngPtr);
		png_read_update_info(pngPtr, infoPtr);

		png_bytep* rowPtrs = new png_bytep[out.h];
		out.data = new unsigned char[4 * out.w * out.h];
		out.memoryHandledExternally = false;
		for (int i = 0; i < out.h; i++)
			rowPtrs[i] = (png_bytep)&out.data[i * 4 * out.w];
		png_read_image(pngPtr, rowPtrs);
		png_destroy_read_struct(&pngPtr, &infoPtr, (png_infopp)0);
		delete[] rowPtrs;
		fclose(fp);
		return out;
    }

    std::string getDateString(int s) {
        std::stringstream ss;
		ss << s;
		return ss.str();
	}

	virtual int checkLicense() { return 0; }

	// Return true so touch menus and touch gameplay are preserved.
	// WASD keyboard support is layered on top via TouchscreenInput's key handling.
	virtual bool supportsTouchscreen() { return true; }
	virtual bool hasBuyButtonWhenInvalidLicense() { return false; }

	// --- World Creation Dialog ---
	// Shows an HTML modal (via mc_platform.js) asking for world name and game mode.
	// The game polls getUserInputStatus() each tick until the user responds.
	virtual void createUserInput() {
		mcShowCreateWorldDialog();
	}

	virtual int getUserInputStatus() {
		return mcGetWorldStatus();
	}

	virtual StringVector getUserInput() {
		StringVector result;
		char* raw = mcGetWorldResult();
		if (raw) {
			// raw = "worldName|creative" or "worldName|survival"
			std::string fullStr(raw);
			free(raw);
			size_t sep = fullStr.find('|');
			if (sep != std::string::npos) {
				result.push_back(fullStr.substr(0, sep));   // [0] = name
				result.push_back(std::string());             // [1] = seed (empty = time-based)
				result.push_back(fullStr.substr(sep + 1));  // [2] = "creative"/"survival"
			} else {
				result.push_back(fullStr);
				result.push_back(std::string());
				result.push_back("creative");
			}
		}
		// Sync saves to IndexedDB after world is created
		mcSyncSaves();
		return result;
	}

	virtual int getScreenWidth();
	virtual int getScreenHeight();
	virtual float getPixelsPerMillimeter();
};

#endif /*APPPLATFORM_EMSCRIPTEN_H__*/
