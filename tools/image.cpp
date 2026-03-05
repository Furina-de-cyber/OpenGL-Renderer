#include "image.h"

void SaveRenderTarget(
	int width,
	int height,
	GLenum attachmentPos,
	const std::string& path,
	EOutputFormat format,
	EYFlip yFlip
)
{
	glReadBuffer(attachmentPos);

	// -----------------------------
	// 1. 读取 GPU 浮点 RT
	// -----------------------------
	std::vector<float> pixels(width * height * 4);
	glReadPixels(
		0, 0,
		width, height,
		GL_RGBA,
		GL_FLOAT,
		pixels.data()
	);

	// -----------------------------
	// 2. Y 轴翻转（如果需要）
	// OpenGL: 左下角为 (0,0)
	// 图像:   左上角为 (0,0)
	// -----------------------------
	if (yFlip == EYFlip::FlipY)
	{
		const int rowStride = width * 4;
		std::vector<float> tempRow(rowStride);

		for (int y = 0; y < height / 2; ++y)
		{
			float* rowA = pixels.data() + y * rowStride;
			float* rowB = pixels.data() + (height - 1 - y) * rowStride;

			std::memcpy(tempRow.data(), rowA, rowStride * sizeof(float));
			std::memcpy(rowA, rowB, rowStride * sizeof(float));
			std::memcpy(rowB, tempRow.data(), rowStride * sizeof(float));
		}
	}

	// -----------------------------
	// 3. 根据输出格式保存
	// -----------------------------
	if (format == EOutputFormat::HDR_FLOAT)
	{
		// stb HDR 期望 RGB float
		std::vector<float> hdrRGB(width * height * 3);

		for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3)
		{
			hdrRGB[j + 0] = pixels[i + 0];
			hdrRGB[j + 1] = pixels[i + 1];
			hdrRGB[j + 2] = pixels[i + 2];
		}

		stbi_write_hdr(
			path.c_str(),
			width,
			height,
			3,
			hdrRGB.data()
		);
	}
	else
	{
		// -----------------------------
		// LDR：HDR → 8bit
		// -----------------------------
		std::vector<unsigned char> ldr(width * height * 4);

		for (int i = 0; i < width * height * 4; i += 4)
		{
			glm::vec3 hdr(
				pixels[i + 0],
				pixels[i + 1],
				pixels[i + 2]
			);

			// 可选 tone mapping
			// hdr = hdr / (hdr + glm::vec3(1.0f)); // Reinhard
			hdr = glm::clamp(hdr, 0.0f, 1.0f);

			ldr[i + 0] = static_cast<unsigned char>(hdr.r * 255.0f);
			ldr[i + 1] = static_cast<unsigned char>(hdr.g * 255.0f);
			ldr[i + 2] = static_cast<unsigned char>(hdr.b * 255.0f);
			ldr[i + 3] = 255;
		}

		stbi_write_png(
			path.c_str(),
			width,
			height,
			4,
			ldr.data(),
			width * 4
		);
	}
}