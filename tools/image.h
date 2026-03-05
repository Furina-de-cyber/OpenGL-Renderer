#pragma once

#include "../core.h"

enum class EOutputFormat
{
	LDR_8BIT,   // PNG
	HDR_FLOAT   // HDR
};

enum class EYFlip
{
	None,
	FlipY
};

void SaveRenderTarget(
	int width,
	int height,
	GLenum attachmentPos,
	const std::string& path,
	EOutputFormat format,
	EYFlip yFlip
);