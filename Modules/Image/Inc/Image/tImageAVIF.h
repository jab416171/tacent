// tImageAVIF.h
//
// This class knows how to load AVIF (AV1 Image File Format) files into tPixel arrays. These tPixels may be 'stolen'
// by the tPicture's constructor if an avif file is specified. After the array is stolen the tImageAVIF is invalid.
// This is purely for performance.
//
// Copyright (c) 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tString.h>
#include <Math/tColour.h>
#include <Image/tPixelFormat.h>
#include <Image/tBaseImage.h>
namespace tImage
{


class tImageAVIF : public tBaseImage
{
public:
	// Creates an invalid tImageAVIF. You must call Load manually.
	tImageAVIF()																										{ }
	tImageAVIF(const tString& avifFile)																					{ Load(avifFile); }

	// The data is copied out of avifFileInMemory. Go ahead and delete[] after if you want.
	tImageAVIF(const uint8* avifFileInMemory, int numBytes)																{ Load(avifFileInMemory, numBytes); }

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	tImageAVIF(tPixel4b* pixels, int width, int height, bool steal = false)												{ Set(pixels, width, height, steal); }

	// Sets from a single frame.
	tImageAVIF(tFrame* frame, bool steal = true)																		{ Set(frame, steal); }

	// Constructs from a tPicture.
	tImageAVIF(tPicture& picture, bool steal = true)																	{ Set(picture, steal); }

	virtual ~tImageAVIF()																								{ Clear(); }

	// Clears the current tImageAVIF before loading. Returns success. If false returned, object is invalid.
	bool Load(const tString& avifFile);
	bool Load(const uint8* avifFileInMemory, int numBytes);

	// This one sets from a supplied pixel array. If steal is true it takes ownership of the pixels pointer. Otherwise
	// it just copies the data out.
	bool Set(tPixel4b* pixels, int width, int height, bool steal = false) override;

	// Sets from a single frame.
	bool Set(tFrame*, bool steal = true) override;

	// Sets from a tPicture.
	bool Set(tPicture& picture, bool steal = true) override;

	// After this call no memory will be consumed by the object and it will be invalid.
	void Clear() override;
	bool IsValid() const override																						{ return Pixels ? true : false; }

	int GetWidth() const																								{ return Width; }
	int GetHeight() const																								{ return Height; }

	// All pixels must be opaque (alpha = 255) for this to return true.
	bool IsOpaque() const;

	// After this call you are the owner of the pixels and must eventually delete[] them. This tImageAVIF object is
	// invalid afterwards.
	tPixel4b* StealPixels();
	tFrame* GetFrame(bool steal = true) override;
	tPixel4b* GetPixels() const																							{ return Pixels; }

private:
	int Width						= 0;
	int Height						= 0;
	tPixel4b* Pixels				= nullptr;
};


// Implementation below this line.


inline void tImageAVIF::Clear()
{
	Width				= 0;
	Height				= 0;
	delete[]			Pixels;
	Pixels				= nullptr;

	tBaseImage::Clear();
}


}
