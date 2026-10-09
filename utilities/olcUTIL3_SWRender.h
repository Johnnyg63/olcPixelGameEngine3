/*
	OneLoneCoder - Software Renderer v3.0
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	A collection of software rendering routines. Typically these model
	that of the basic Draw.xx() routines in PGE3, however they don't
	have to be olc::Pixel in olc::Image. As long as your container
	and element fulfil the conceptual requirements you can render
	anything into anything!


	License (OLC-3)
	~~~~~~~~~~~~~~~

	Copyright 2018 - 2026 OneLoneCoder.com

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions or derivations of source code must retain the above
	copyright notice, this list of conditions and the following disclaimer.

	2. Redistributions or derivative works in binary form must reproduce
	the above copyright notice. This list of conditions and the following
	disclaimer must be reproduced in the documentation and/or other
	materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	contributors may be used to endorse or promote products derived
	from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
	A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Links
	~~~~~
	YouTube:	https://www.youtube.com/javidx9
	Discord:	https://discord.gg/WhwHUMV
	Twitter:	https://www.twitter.com/javidx9
	Twitch:		https://www.twitch.tv/javidx9
	GitHub:		https://www.github.com/onelonecoder
	Homepage:	https://www.onelonecoder.com

	Author
	~~~~~~
	David Barr, aka javidx9, ©OneLoneCoder 2019 - 2026

	Version
	~~~~~~~

	3.0		+Initial Release

*/



#pragma once

#include <concepts>
#include <olcPixelGameEngine3.h>

namespace olc::utils
{
	template<typename grid, typename T>
	concept RenderSurface = requires(grid g, int x, int y, T t)
	{
		{ g.set(x, y, t) } -> std::same_as<bool>;
		{ g.get(x, y) } -> std::convertible_to<T>;
		{ g.width() } -> std::integral;
		{ g.height() } -> std::integral;
	};

	template<typename T>
	concept lerpable = requires(T a, T b, float t)
	{
		{ a * t } -> std::convertible_to<T>;
		{ b * t } -> std::convertible_to<T>;
		{ a + b } -> std::convertible_to<T>;
	};

	template<typename T> requires lerpable<T>
	T flerp(const T& a, const T& b, const float t)
	{
		return (b * t) + a * (1.0f - t);
	}

	template<class grid, class T> requires RenderSurface<grid, T>
	class SoftwareRenderer
	{	
	

	public:
		void SetTarget(grid& target)
		{
			pTarget = &target;
			vScanlines.resize(pTarget->height());
		}

		void SetWorldTransform(const olc::tf2d& trans)
		{
			transformAffine = trans;
		}

		olc::tf2d& GetWorldTransform()
		{
			return transformAffine;
		}

		void WorldReset()
		{
			transformAffine = olc::tf2d();
		}

		void WorldScale(const olc::vf2d& vScale)
		{
			transformAffine.scale(vScale);
		}

		void WorldOffset(const olc::vf2d& vOffset)
		{
			transformAffine.translate(vOffset);
		}

		void WorldRotate(const float& fTheta, const olc::vf2d& vPoint)
		{
			transformAffine.rotate(fTheta, vPoint);
		}


	public:


		bool Plot(const olc::vi2d& pos, const T& element)
		{
			const auto vTransformedPoints = transformAffine.forward<float>(pos);
			return PlotRaw(vTransformedPoints.x, vTransformedPoints.y, element);
		}

		bool PlotRaw(const int x, const int y, const T& element)
		{
			return pTarget->set(x, y, element);
		}

		void Clear(const T& element)
		{
			for (int y = 0; y < pTarget->height(); y++)
			{
				for (int x = 0; x < pTarget->width(); x++)
				{
					PlotRaw(x, y, element);
				}
			}
		}

		void Line(const olc::vf2d& p1, const olc::vf2d& p2,	const T& element)
		{
			const auto vTransformedPoints = transformAffine.forward<float>({ p1, p2 });
			swRasterShadedLine(
				vTransformedPoints[0],
				vTransformedPoints[1],
				element, element);
		}

		void Line(const olc::vf2d& p1, const olc::vf2d& p2, const T& element1, const T& element2)
		{
			const auto vTransformedPoints = transformAffine.forward<float>({ p1, p2 });
			swRasterShadedLine(
				vTransformedPoints[0],
				vTransformedPoints[1],
				element1, element2);
		}

		void Rect(const olc::vf2d& pos, const olc::vf2d& size, const T& col)
		{
			Rect(pos, size, col, col, col, col);
		}

		void Rect(const olc::vf2d& pos, const olc::vf2d& size, const T& colTL, const T& colTR, const T& colBL, const T& colBR)
		{
			Line(pos, { pos.x + size.x, pos.y }, colTL, colTR);
			Line({ pos.x + size.x, pos.y }, { pos.x + size.x, pos.y + size.y }, colTR, colBR);
			Line({ pos.x + size.x, pos.y + size.y }, { pos.x, pos.y + size.y }, colBR, colBL);
			Line({ pos.x, pos.y + size.y }, pos, colBL, colTL);
		}

		void FilledRect(const olc::vf2d& pos, const olc::vf2d& size, const T& col)
		{
			FilledRect(pos, size, col, col, col, col);
		}

		void FilledRect(const olc::vf2d& pos, const olc::vf2d& size, const T& colTL, const T& colTR, const T& colBL, const T& colBR)
		{
			const auto vTransformedPoints = transformAffine.forward<float>(
				{ pos,{ pos.x + size.x, pos.y },
				pos + size,{ pos.x, pos.y + size.y }
				});

			// Check if all one colour
			if (colTL == colBL && colTL == colTR && colTL == colBR)
			{
				// Check if axis aligned
				if (vTransformedPoints[0].y == vTransformedPoints[1].y &&
					vTransformedPoints[1].x == vTransformedPoints[2].x &&
					vTransformedPoints[2].y == vTransformedPoints[3].y &&
					vTransformedPoints[3].x == vTransformedPoints[0].x)
				{
					// Clip to target
					olc::vi2d p1 = vTransformedPoints[0].max({ 0,0 });
					olc::vi2d p2 = vTransformedPoints[2].min(pTarget->Size());

					// Draw filled rectangle
					for (int32_t y = p1.y; y < p2.y; y++)
						for (int32_t x = p1.x; x < p2.x; x++)
							PlotRaw(x, y, colTL);

					// Exit early
					return;
				}
			}

			// Fallback to general case rasteriser, where we split into two triangles
			swRasterShadedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[1],
				vTransformedPoints[2],
				colTL, colTR, colBR);
			swRasterShadedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[2],
				vTransformedPoints[3],
				colTL, colBR, colBL);
		}

		void TexturedRect(
			const olc::vf2d& pos, 
			const olc::vf2d& size,
			olc::Image& texture,
			const olc::vf2d& t1 = { 0,0 }, 
			const olc::vf2d& t2 = { 1,0 },
			const olc::vf2d& t3 = { 1,1 }, 
			const olc::vf2d& t4 = { 0,1 },
			const T& colTL = olc::Colour::WHITE, 
			const T& colTR = olc::Colour::WHITE, 
			const T& colBL = olc::Colour::WHITE, 
			const T& colBR = olc::Colour::WHITE)
		{
			const auto vTransformedPoints = transformAffine.forward<float>(
				{ pos,{ pos.x + size.x, pos.y },
				pos + size,{ pos.x, pos.y + size.y }
				});

			swRasterTexturedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[1],
				vTransformedPoints[2],
				colTL, colTR, colBR, 
				t1, t2, t3, texture);
			swRasterTexturedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[2],
				vTransformedPoints[3],
				colTL, colBR, colBL,
				t1, t3, t4, texture);
		}


		void Triangle(const olc::vf2d& p1, const olc::vf2d& p2, const olc::vf2d& p3, const T& col)
		{
			Triangle(p1, p2, p3, col, col, col);
		}

		void Triangle(const olc::vf2d& p1, const olc::vf2d& p2, const olc::vf2d& p3, const T& c1, const T& c2, const T& c3)
		{
			Line(p1, p2, c1, c2);
			Line(p2, p3, c2, c3);
			Line(p3, p1, c3, c1);
		}

		void FilledTriangle(const olc::vf2d& p1, const olc::vf2d& p2, const olc::vf2d& p3, const T& col)
		{
			FilledTriangle(p1, p2, p3, col, col, col);
		}

		void FilledTriangle(const olc::vf2d& p1, const olc::vf2d& p2, const olc::vf2d& p3, const T& c1, const T& c2, const T& c3)
		{
			const auto vTransformedPoints = transformAffine.forward<float>({ p1, p2, p3 });
			swRasterShadedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[1],
				vTransformedPoints[2],
				c1, c2, c3);
		}

		void TexturedTriangle(const olc::vf2d& p1, const olc::vf2d& p2, const olc::vf2d& p3, const T& c1, const T& c2, const T& c3, const olc::vf2d& t1, const olc::vf2d& t2, const olc::vf2d& t3, olc::Image& texture)
		{
			const auto vTransformedPoints = transformAffine.forward<float>({ p1, p2, p3 });
			swRasterTexturedTriangle(
				vTransformedPoints[0],
				vTransformedPoints[1],
				vTransformedPoints[2],
				c1, c2, c3,
				t1, t2, t3,
				texture);
		}


		void Circle(const olc::vf2d& p, float radius, const T& element, uint8_t mask = 0xFF)
		{
			const olc::vf2d vTransformedPoint = transformAffine.forwardRound<float>(p);
			const float fTransformedRadius = transformAffine.scale().x * radius;

			if (fTransformedRadius < 0 
				|| vTransformedPoint.x < -fTransformedRadius
				|| vTransformedPoint.y < -fTransformedRadius
				|| vTransformedPoint.x - pTarget->width() > fTransformedRadius
				|| vTransformedPoint.y - pTarget->height() > fTransformedRadius)
				return;

			if (fTransformedRadius > 0)
			{
				int x0 = 0;
				int y0 = int(fTransformedRadius);
				int d = 3 - 2 * y0;

				while (y0 >= x0) // only formulate 1/8 of circle
				{
					// Draw even octants
					if (mask & 0x01) PlotRaw(vTransformedPoint.x + x0, vTransformedPoint.y - y0, element);// Q6 - upper right right
					if (mask & 0x04) PlotRaw(vTransformedPoint.x + y0, vTransformedPoint.y + x0, element);// Q4 - lower lower right
					if (mask & 0x10) PlotRaw(vTransformedPoint.x - x0, vTransformedPoint.y + y0, element);// Q2 - lower left left
					if (mask & 0x40) PlotRaw(vTransformedPoint.x - y0, vTransformedPoint.y - x0, element);// Q0 - upper upper left
					if (x0 != 0 && x0 != y0)
					{
						if (mask & 0x02) PlotRaw(vTransformedPoint.x + y0, vTransformedPoint.y - x0, element);// Q7 - upper upper right
						if (mask & 0x08) PlotRaw(vTransformedPoint.x + x0, vTransformedPoint.y + y0, element);// Q5 - lower right right
						if (mask & 0x20) PlotRaw(vTransformedPoint.x - y0, vTransformedPoint.y + x0, element);// Q3 - lower lower left
						if (mask & 0x80) PlotRaw(vTransformedPoint.x - x0, vTransformedPoint.y - y0, element);// Q1 - upper left left
					}

					if (d < 0)
						d += 4 * x0++ + 6;
					else
						d += 4 * (x0++ - y0--) + 10;
				}
			}
			else
				PlotRaw(vTransformedPoint.x, vTransformedPoint.y, element);
		}


		void FilledCircle(const olc::vf2d& p, float radius, const T& element)
		{ 
			const olc::vf2d vTransformedPoint = transformAffine.forwardRound<float>(p);
			const float fTransformedRadius = transformAffine.scale().x * radius;

			if (fTransformedRadius < 0
				|| vTransformedPoint.x < -fTransformedRadius
				|| vTransformedPoint.y < -fTransformedRadius
				|| vTransformedPoint.x - pTarget->width() > fTransformedRadius
				|| vTransformedPoint.y - pTarget->height() > fTransformedRadius)
				return;

			if (fTransformedRadius > 0)
			{
				int x0 = 0;
				int y0 = int(fTransformedRadius);
				int d = 3 - 2 * y0;

				auto drawline = [&](int sx, int ex, int y)
					{
						for (int x = sx; x <= ex; x++)
							PlotRaw(x, y, element);
					};

				while (y0 >= x0)
				{
					drawline(vTransformedPoint.x - y0, vTransformedPoint.x + y0, vTransformedPoint.y - x0);
					if (x0 > 0)	drawline(vTransformedPoint.x - y0, vTransformedPoint.x + y0, vTransformedPoint.y + x0);

					if (d < 0)
						d += 4 * x0++ + 6;
					else
					{
						if (x0 != y0)
						{
							drawline(vTransformedPoint.x - x0, vTransformedPoint.x + x0, vTransformedPoint.y - y0);
							drawline(vTransformedPoint.x - x0, vTransformedPoint.x + x0, vTransformedPoint.y + y0);
						}
						d += 4 * (x0++ - y0--) + 10;
					}
				}
			}
			else
				PlotRaw(vTransformedPoint.x, vTransformedPoint.y, element);
		}



	protected:
		// Clips a line to a rectangular region, returns true if line is visible.
		// The returned weights correspond to distance along the line from v0 to v1
		bool swClipWeightedLine(olc::vf2d& v0, olc::vf2d& v1, const olc::vf2d& vMin, const olc::vf2d& vMax, float& w0, float& w1)
		{
			// Liang-Barsky line clipping algorithm adapted for weighted lines
			// https://en.wikipedia.org/wiki/Liang%E2%80%93Barsky_algorithm

			olc::vf2d diff = v1 - v0;

			float p[4] = { -diff.x, diff.x, -diff.y, diff.y };

			float q[4] =
			{
				v0.x - vMin.x,
				vMax.x - v0.x,
				v0.y - vMin.y,
				vMax.y - v0.y
			};

			// Weights are ideal to start, we'll contact them as we clip
			w0 = 0.0f;
			w1 = 1.0f;

			for (int i = 0; i < 4; i++)
			{
				if (p[i] == 0.0f)
				{
					if (q[i] < 0.0f)
						return false; // Line is parallel and outside the clipping boundary
				}
				else
				{
					float t = float(q[i]) / float(p[i]);
					if (p[i] < 0.0f)
					{
						if (t > w1)
							return false; // Line is outside the clipping boundary
						else if (t > w0)
							w0 = t;
					}
					else
					{
						if (t < w0)
							return false; // Line is outside the clipping boundary
						else if (t < w1)
							w1 = t;
					}
				}
			}

			if (w1 < w0)
				return false; // Line is outside the clipping boundary

			// Return new line segment ends
			olc::vf2d v = v0;
			v0 = v + (diff * w0);
			v1 = v + (diff * w1);

			// Line has visible pixels inside clipping boundary
			return true;
		}

		// Rasterises a shaded line in integer space
		void swRasterShadedLine(const olc::vi2d& v1, const olc::vi2d& v2, const T& c1, const T& c2)
		{
			// Lambda to draw a pixel gated by a pattern bit
			uint32_t pattern = 0xFFFFFFFF;
			auto rol = [&](void)
				{
					pattern = (pattern << 1) | (pattern >> 31);
					return pattern & 1;
				};

			// Clip line to draw target
			olc::vf2d clipped_p1 = v1;
			olc::vf2d clipped_p2 = v2;

			float w0 = 0, w1 = 1;
			if (!swClipWeightedLine(clipped_p1, clipped_p2, { 0,0 }, olc::vf2d(pTarget->width(), pTarget->height()), w0, w1))
				return;

			// Move to integer space
			olc::vi2d ip1 = clipped_p1;
			olc::vi2d ip2 = clipped_p2;
			olc::vi2d pixel;

			// Calculate deltas
			int dx = ip2.x - ip1.x;
			int dy = ip2.y - ip1.y;
			int absDx = std::abs(dx);
			int absDy = std::abs(dy);

			// Determine dominant axis
			bool xMajor = absDx >= absDy;
			int steps = xMajor ? absDx : absDy;

			// Handle degenerate case (single pixel)
			if (steps == 0)
			{
				PlotRaw(ip1.x, ip1.y, c1);
				return;
			}

			// Calculate step increments
			float xStep = float(dx) / float(steps);
			float yStep = float(dy) / float(steps);
			float colorStep = 1.0f / float(steps) * (w1 - w0);

			T cStart = flerp(c1, c2, w0);
			T cEnd = flerp(c1, c2, w1);



			// Starting position and color interpolation parameter
			float x = float(ip1.x);
			float y = float(ip1.y);
			float t = 0.0f;

			// Draw line pixel by pixel
			for (int i = 0; i <= steps; i++)
			{
				// Interpolate color
				//olc::Pixel col = olc::PixelLerp(c1, c2, t);
				T col = flerp(cStart, cEnd, t);

				// Plot pixel
				if (rol())
					PlotRaw((int)std::round(x), (int)std::round(y), col);

				// Step to next pixel
				x += xStep;
				y += yStep;
				t += colorStep;
			}
		}

		void swRasterShadedTriangle(const olc::vi2d& v1, const olc::vi2d& v2, const olc::vi2d& v3, const T& c1, const T& c2, const T& c3)
		{
			// Calculate barycentric outline
			auto [y_min, y_max] = swBaryFillTriangle(v1, v2, v3);

			// Now draw the scanlines
			for (int32_t y = y_min; y < y_max; y++)
			{
				const auto& scanline = vScanlines[y];

				int32_t xStart = scanline.nMin;
				int32_t xEnd = scanline.nMax;

				int32_t x_min = std::max(0, xStart);
				int32_t x_max = std::min(xEnd, pTarget->width());

				float fSpan = float(xEnd - xStart);
				float fSpanStep = fSpan > 0.0f ? 1.0f / fSpan : 0.0f;

				float b0_step = fSpanStep * (scanline.fBaryMax[0] - scanline.fBaryMin[0]);
				float b1_step = fSpanStep * (scanline.fBaryMax[1] - scanline.fBaryMin[1]);
				float b2_step = fSpanStep * (scanline.fBaryMax[2] - scanline.fBaryMin[2]);

				float b0 = scanline.fBaryMin[0];
				float b1 = scanline.fBaryMin[1];
				float b2 = scanline.fBaryMin[2];

				if (xStart < 0)
				{
					b0 = scanline.fBaryMin[0] + (-xStart * b0_step);
					b1 = scanline.fBaryMin[1] + (-xStart * b1_step);
					b2 = scanline.fBaryMin[2] + (-xStart * b2_step);
				}

				for (int32_t x = x_min; x < x_max; x++)
				{
					T col = c1 * b0 + c2 * b1 + c3 * b2;
					PlotRaw(x, y, col);
					b0 += b0_step;
					b1 += b1_step;
					b2 += b2_step;
				}
			}
		}

		void swRasterTexturedTriangle(const olc::vi2d& v1, const olc::vi2d& v2, const olc::vi2d& v3, const T& c1, const T& c2, const T& c3, const olc::vf2d& t1, const olc::vf2d& t2, const olc::vf2d& t3, grid& texture)
		{
			auto [y_min, y_max] = swBaryFillTriangle(v1, v2, v3);

			// Now draw the scanlines
			for (int32_t y = y_min; y < y_max; y++)
			{
				const auto& scanline = vScanlines[y];

				int32_t xStart = scanline.nMin;
				int32_t xEnd = scanline.nMax;

				int32_t x_min = std::max(0, xStart);
				int32_t x_max = std::min(xEnd, pTarget->width());

				float fSpan = float(xEnd - xStart);
				float fSpanStep = fSpan > 0.0f ? 1.0f / fSpan : 0.0f;

				float b0_step = fSpanStep * (scanline.fBaryMax[0] - scanline.fBaryMin[0]);
				float b1_step = fSpanStep * (scanline.fBaryMax[1] - scanline.fBaryMin[1]);
				float b2_step = fSpanStep * (scanline.fBaryMax[2] - scanline.fBaryMin[2]);

				float b0 = scanline.fBaryMin[0];
				float b1 = scanline.fBaryMin[1];
				float b2 = scanline.fBaryMin[2];

				if (xStart < 0)
				{
					b0 = scanline.fBaryMin[0] + (-xStart * b0_step);
					b1 = scanline.fBaryMin[1] + (-xStart * b1_step);
					b2 = scanline.fBaryMin[2] + (-xStart * b2_step);
				}

				float w =  texture.width();
				float h = texture.height();

				for (int32_t x = x_min; x < x_max; x++)
				{
					T col = c1 * b0 + c2 * b1 + c3 * b2;

					olc::vf2d uv = olc::vf2d(
						b0 * t1.x + b1 * t2.x + b2 * t3.x,
						b0 * t1.y + b1 * t2.y + b2 * t3.y);

					PlotRaw(x, y, texture.get(float(uv.x) * w, float(uv.y) * h));
					b0 += b0_step;
					b1 += b1_step;
					b2 += b2_step;
				}
			}

			return;
		}


	protected:
		grid* pTarget = nullptr;
		olc::tf2d transformAffine;

		struct Scanline
		{
			int32_t nMin = std::numeric_limits<int32_t>::max();
			int32_t nMax = std::numeric_limits<int32_t>::min();
			std::array<float, 3> fBaryMin;
			std::array<float, 3> fBaryMax;
		};
		std::vector<Scanline> vScanlines;

		// Fills scanline buffer with visible triangle extents and barycentric coordinates.
		// Returns vertical, visible extents of triangle scanlines
		std::pair<int, int> swBaryFillTriangle(const olc::vi2d& v1, const olc::vi2d& v2, const olc::vi2d& v3)
		{
			// Get height of triangle in whole pixels
			int32_t nMinY = std::min({ v1.y, v2.y, v3.y });
			int32_t nMaxY = std::max({ v1.y, v2.y, v3.y });
			int32_t nHeight = nMaxY - nMinY;

			if (nHeight <= 0)
				return { 0, 0 }; // Degenerate triangle

			// Scanline buffer is already allocated to be the max vertical size
			// of the draw target. Obviously it only represents visible scanlines
			// that are to be filled for the current triangle.

			// Get visible height of triangle
			int32_t y_min = std::max(0, nMinY);
			int32_t y_max = std::min(nMaxY, pTarget->height());

			// Zero out scanline buffer (by resetting min and max values)
			for (int32_t y = y_min; y < y_max; y++)
			{
				vScanlines[y].nMin = std::numeric_limits<int32_t>::max();
				vScanlines[y].nMax = std::numeric_limits<int32_t>::min();
			}

			// This function scans an edge of the triangle, updating
			// the scanline buffer with min/max extents and barycentric coords.
			// It returns the number of scanlines updated.
			auto scanEdge = [&](olc::vi2d p0, olc::vi2d p1, int id1, int id2) -> size_t
				{
					if (p0.y == p1.y)
						return 0;

					// Ensure p0.y < p1.y
					bool swapped = false;
					if (p0.y > p1.y)
					{
						std::swap(p0, p1);
						swapped = true;
					}

					// Cache edge step deltas
					int dy = p1.y - p0.y;
					float dx_step = (p1.x - p0.x) / float(dy);
					float dy_step = 1.0f / float(dy);
					float x = float(p0.x);

					// Rasterise edge - if pixel lies on visible scanline then
					// update the scanline bounds and barycentric coords
					size_t nScanline = 0;
					for (int y = p0.y; y <= p1.y; y++)
					{
						// If this pixel row is visible
						if (y >= 0 && y < vScanlines.size())
						{
							int ix = int(std::round(x));

							// interpolation along edge 
							// Note: We may need to do this differently when clipping
							float t = (y - p0.y) * dy_step;

							std::array<float, 3> bary = { 0.0f, 0.0f, 0.0f };

							// Set barycentric coords depending on edge direction
							if (swapped)
							{
								bary[id1] = t;
								bary[id2] = 1.0f - t;
							}
							else
							{
								bary[id1] = 1.0f - t;
								bary[id2] = t;
							}

							// Update scanline extents and barycentric coords
							if (ix < vScanlines[y].nMin)
							{
								vScanlines[y].nMin = ix;
								vScanlines[y].fBaryMin = bary;
							}

							if (ix > vScanlines[y].nMax)
							{
								vScanlines[y].nMax = ix;
								vScanlines[y].fBaryMax = bary;
							}

							nScanline++;
						}

						x += dx_step;
					}

					return nScanline;
				};

			// Rasterise triangle edges into scanline buffer
			scanEdge(v1, v2, 0, 1);
			scanEdge(v1, v3, 0, 2);
			scanEdge(v2, v3, 1, 2);

			return { y_min, y_max };
		}

	};


	typedef SoftwareRenderer<olc::Image, olc::Pixel> SWDraw;
}