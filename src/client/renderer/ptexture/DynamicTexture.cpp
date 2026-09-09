#include "DynamicTexture.h"

#include <cstring>
#include <cmath>
#include "../Textures.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/level/tile/FireTile.h"
#include "../../../util/Mth.h"

//
// DynamicTexture
//
DynamicTexture::DynamicTexture(int tex_)
:   tex(tex_),
	replicate(1)
{
	memset(pixels, 0, 16*16*4);
}

void DynamicTexture::bindTexture(Textures* tex) {
	tex->loadAndBindTexture("terrain.png");
}

//
// WaterTexture
// I was thinking of adding something simple (a simple frame copy from a
// "still water image sequence") every n:th tick for calm water, and shifting
// the rows of a texture for the running water. I might do that, but I got
// impressed over the java code, so I will try that first.. and I suspect they
// wont mix very good.
/*
WaterTexture::WaterTexture()
:   super(Tile::water->tex),
	_tick(0),
	_frame(0)
{
}

void WaterTexture::tick() {
}
*/

WaterTexture::WaterTexture()
:   super(Tile::water->tex),
	_tick(0),
	_frame(0)
{
	current = new float[16*16];
	next = new float[16*16];
	heat = new float[16*16];
	heata = new float[16*16];

	for (int i = 0; i < 256; ++i) {
		current[i] = 0;
		next[i] = 0;
		heat[i] = 0;
		heata[i] = 0;
	}
}

WaterTexture::~WaterTexture() {
	delete[] current;
	delete[] next;
	delete[] heat;
	delete[] heata;
}

void WaterTexture::tick()
{
	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			float pow = 0;
			for (int xx = x - 1; xx <= x + 1; xx++) {
				int xi = (xx) & 15;
				int yi = (y) & 15;
				pow += current[xi + yi * 16];
			}
			next[x + y * 16] = pow / 3.3f + heat[x + y * 16] * 0.8f;
		}

	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			heat[x + y * 16] += heata[x + y * 16] * 0.05f;

			if (heat[x + y * 16] < 0) heat[x + y * 16] = 0;
			heata[x + y * 16] -= 0.1f;
			if (Mth::random() < 0.05f) {
				heata[x + y * 16] = 0.5f;
			}
		}

	float* tmp = next;
	next = current;
	current = tmp;

	for (int i = 0; i < 256; i++) {
		float pow = current[i];
		if (pow > 1) pow = 1;
		if (pow < 0) pow = 0;

		float pp = pow * pow;

		int r = (int) (32 + pp * 32);
		int g = (int) (50 + pp * 64);
		int b = (int) (255);
		int a = (int) (146 + pp * 50);

		//if (anaglyph3d) {
		//	int rr = (r * 30 + g * 59 + b * 11) / 100;
		//	int gg = (r * 30 + g * 70) / (100);
		//	int bb = (r * 30 + b * 70) / (100);

		//	r = rr;
		//	g = gg;
		//	b = bb;
		//}

		pixels[i * 4 + 0] = r;
		pixels[i * 4 + 1] = g;
		pixels[i * 4 + 2] = b;
		pixels[i * 4 + 3] = a;
	}
}

//
// WaterSideTexture
//
WaterSideTexture::WaterSideTexture()
:   super(Tile::water->tex + 1),
	_tick(0),
	_frame(0),
	_tickCount(0)
{
	replicate = 2;

	current = new float[16*16];
	next = new float[16*16];
	heat = new float[16*16];
	heata = new float[16*16];

	for (int i = 0; i < 256; ++i) {
		current[i] = 0;
		next[i] = 0;
		heat[i] = 0;
		heata[i] = 0;
	}
}

WaterSideTexture::~WaterSideTexture() {
	delete[] current;
	delete[] next;
	delete[] heat;
	delete[] heata;
}

void WaterSideTexture::tick() {
	++_tickCount;
	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			float pow = 0;
			for (int xx = y - 2; xx <= y; xx++) {
				int xi = (x) & 15;
				int yi = (xx) & 15;
				pow += current[xi + yi * 16];
			}
			next[x + y * 16] = pow / 3.2f + heat[x + y * 16] * 0.8f;
		}

	for (int x = 0; x < 16; x++)
		for (int y = 0; y < 16; y++) {
			heat[x + y * 16] += heata[x + y * 16] * 0.05f;

			if (heat[x + y * 16] < 0) heat[x + y * 16] = 0;
			heata[x + y * 16] -= 0.3f;
			if (Mth::random() < 0.2) {
				heata[x + y * 16] = 0.5f;
			}
		}
	float* tmp = next;
	next = current;
	current = tmp;

	for (int i = 0; i < 256; i++) {
		float pow = current[(i - _tickCount * 16) & 255];
		if (pow > 1) pow = 1;
		if (pow < 0) pow = 0;

		float pp = pow * pow;

		int r = (int) (32 + pp * 32);
		int g = (int) (50 + pp * 64);
		int b = (int) (255);
		int a = (int) (146 + pp * 50);

		//if (anaglyph3d) {
		//	int rr = (r * 30 + g * 59 + b * 11) / 100;
		//	int gg = (r * 30 + g * 70) / (100);
		//	int bb = (r * 30 + b * 70) / (100);

		//	r = rr;
		//	g = gg;
		//	b = bb;
		//}

		pixels[i * 4 + 0] = r;
		pixels[i * 4 + 1] = g;
		pixels[i * 4 + 2] = b;
		pixels[i * 4 + 3] = a;
	}
}

//
// FireTexture
// Port of the classic fire cellular automaton: heat rises from the bottom
// row, spreads/decays upwards, and is mapped to a yellow->red->transparent
// palette. Two instances (offset 0/1) feed the two fire texture slots.
//
FireTexture::FireTexture(int offset)
:	super(Tile::fire->tex + offset * 16)
{
	current = new float[20 * 20];
	next = new float[20 * 20];
	for (int i = 0; i < 20 * 20; ++i) {
		current[i] = 0;
		next[i] = 0;
	}
}

FireTexture::~FireTexture() {
	delete[] current;
	delete[] next;
}

void FireTexture::tick()
{
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 20; y++) {
			int reach = 18;
			float pow = current[x + ((y + 1) % 20) * 16] * reach;
			for (int xx = x - 1; xx <= x + 1; xx++) {
				for (int yy = y; yy <= y + 1; yy++) {
					if (xx >= 0 && yy >= 0 && xx < 16 && yy < 20) {
						pow += current[xx + yy * 16];
					}
					reach++;
				}
			}
			next[x + y * 16] = pow / (reach * 1.06f);
			if (y >= 19) {
				next[x + y * 16] = (float)(Mth::random() * Mth::random() * Mth::random() * 4 + Mth::random() * 0.1f + 0.2f);
			}
		}
	}

	float* tmp = next;
	next = current;
	current = tmp;

	for (int i = 0; i < 256; i++) {
		float pow = current[i] * 1.8f;
		if (pow > 1) pow = 1;
		if (pow < 0) pow = 0;

		int r = (int)(pow * 155 + 100);
		int g = (int)(pow * pow * 255);
		int b = (int)(pow * pow * pow * pow * pow * pow * pow * pow * pow * pow * 255);
		int a = 255;
		if (pow < 0.5f) a = 0;

		pixels[i * 4 + 0] = r;
		pixels[i * 4 + 1] = g;
		pixels[i * 4 + 2] = b;
		pixels[i * 4 + 3] = a;
	}
}

//
// PortalTexture
// Swirling purple portal: 32 precomputed frames of two overlaid spirals.
//
PortalTexture::PortalTexture()
:	super(Tile::portalTile->tex),
	_tick(0)
{
	frames = new unsigned char[32 * 16 * 16 * 4];
	for (int frame = 0; frame < 32; frame++) {
		for (int x = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				float pow = 0;
				for (int layer = 0; layer < 2; layer++) {
					float xo = (float)(x - 8) / 16.0f;
					float yo = (float)(y - 8) / 16.0f;
					float dist = (float)Mth::sqrt(xo * xo + yo * yo) * 2;
					float sr = (float)(dist * (1.0f + layer * 0.5f) * 3.0f);
					float ang = (float)atan2(yo, xo) * 4.0f;
					float ph = (float)(frame / 32.0f * 6.2831855f);
					float v = (float)(sin(ang + sr - ph * (layer == 0 ? 1 : -1)) * 0.5f + 0.5f);
					pow += v * v * 0.5f / (1.0f + dist);
				}
				pow += (float)(Mth::random() * 0.1f);
				if (pow > 1) pow = 1;
				if (pow < 0) pow = 0;

				int r = (int)(pow * pow * 200 + 55);
				int g = (int)(pow * pow * pow * pow * 255);
				int b = (int)(pow * 100 + 155);
				int a = (int)(pow * 100 + 100);

				int i = (frame * 256 + x + y * 16) * 4;
				frames[i + 0] = r;
				frames[i + 1] = g;
				frames[i + 2] = b;
				frames[i + 3] = a;
			}
		}
	}
}

PortalTexture::~PortalTexture() {
	delete[] frames;
}

void PortalTexture::tick()
{
	_tick++;
	int frame = (_tick / 2) & 31;
	memcpy(pixels, frames + frame * 256 * 4, 256 * 4);
}
