// tImageAVIF.cpp
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

#include <System/tFile.h>
#include "Image/tImageAVIF.h"
#include "Image/tPicture.h"
#include <avif/avif.h>
namespace tImage
{


bool tImageAVIF::Load(const tString& avifFile)
{
	Clear();

	if (tSystem::tGetFileType(avifFile) != tSystem::tFileType::AVIF)
		return false;

	if (!tSystem::tFileExists(avifFile))
		return false;

	int numBytes = 0;
	uint8* avifFileInMemory = tSystem::tLoadFile(avifFile, nullptr, &numBytes);
	bool success = Load(avifFileInMemory, numBytes);
	delete[] avifFileInMemory;

	return success;
}


bool tImageAVIF::Load(const uint8* avifFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !avifFileInMemory)
		return false;

	// Create decoder
	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
		return false;

	// Set I/O
	avifResult result = avifDecoderSetIOMemory(decoder, avifFileInMemory, numBytes);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		return false;
	}

	// Parse the image
	result = avifDecoderParse(decoder);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		return false;
	}

	// Get the next (first) image
	result = avifDecoderNextImage(decoder);
	if (result != AVIF_RESULT_OK)
	{
		avifDecoderDestroy(decoder);
		return false;
	}

	// Get image properties
	avifImage* image = decoder->image;
	Width = image->width;
	Height = image->height;

	if ((Width <= 0) || (Height <= 0))
	{
		avifDecoderDestroy(decoder);
		return false;
	}

	// Determine source pixel format
	bool hasAlpha = (image->alphaPlane != nullptr);
	PixelFormatSrc = hasAlpha ? tPixelFormat::R8G8B8A8 : tPixelFormat::R8G8B8;
	PixelFormat = tPixelFormat::R8G8B8A8;

	// @todo Add proper color profile detection based on image->colorPrimaries and image->transferCharacteristics.
	// Currently we assume sRGB which is correct for most AVIF files, but some may use different profiles.
	// For now, we default to sRGB to match the behavior of other image loaders in this library.
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	// Allocate pixel buffer
	Pixels = new tPixel4b[Width * Height];

	// Convert to RGBA
	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGBA;
	rgb.depth = 8;

	// Allocate RGB buffer
	avifRGBImageAllocatePixels(&rgb);

	// Convert from YUV to RGB
	result = avifImageYUVToRGB(image, &rgb);
	if (result != AVIF_RESULT_OK)
	{
		avifRGBImageFreePixels(&rgb);
		avifDecoderDestroy(decoder);
		delete[] Pixels;
		Pixels = nullptr;
		return false;
	}

	// Copy pixels (note: AVIF data is already in the correct orientation)
	// The RGB buffer contains RGBA pixels in row-major order
	int bytesPerRow = Width * 4;
	for (int y = 0; y < Height; y++)
	{
		// Reverse rows to match Tacent's convention (origin at bottom-left)
		tStd::tMemcpy(
			(uint8*)Pixels + ((Height - 1 - y) * bytesPerRow),
			rgb.pixels + (y * rgb.rowBytes),
			bytesPerRow
		);
	}

	// Clean up
	avifRGBImageFreePixels(&rgb);
	avifDecoderDestroy(decoder);

	return true;
}


bool tImageAVIF::Set(tPixel4b* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;

	if (steal)
	{
		Pixels = pixels;
	}
	else
	{
		Pixels = new tPixel4b[Width * Height];
		tStd::tMemcpy(Pixels, pixels, Width * Height * sizeof(tPixel4b));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	return true;
}


bool tImageAVIF::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	PixelFormatSrc = frame->PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;
	ColourProfileSrc = tColourProfile::sRGB;
	ColourProfile = tColourProfile::sRGB;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageAVIF::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	PixelFormatSrc = picture.PixelFormatSrc;
	PixelFormat = tPixelFormat::R8G8B8A8;

	tPixel4b* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	bool success = Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
	tAssert(success);
	return true;
}


tFrame* tImageAVIF::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame;
	frame->PixelFormatSrc = PixelFormatSrc;
	frame->Width = Width;
	frame->Height = Height;

	if (steal)
	{
		frame->Pixels = Pixels;
		Pixels = nullptr;
	}
	else
	{
		frame->Pixels = new tPixel4b[Width * Height];
		tStd::tMemcpy(frame->Pixels, Pixels, Width * Height * sizeof(tPixel4b));
	}

	if (steal)
		Clear();

	return frame;
}


bool tImageAVIF::IsOpaque() const
{
	if (!Pixels)
		return false;

	int numPixels = Width * Height;
	for (int p = 0; p < numPixels; p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel4b* tImageAVIF::StealPixels()
{
	tPixel4b* pixels = Pixels;
	Pixels = nullptr;
	Clear();
	return pixels;
}


}
