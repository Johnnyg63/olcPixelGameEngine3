/*
	olc::PixelGameEngine3 Example - Software Rendering

	Draws transformed and textured graphics entirely with the CPU

	Licenced under the OLC-3 License
*/


// Define OLC_PGE3_APPLICATION to include the implementation of 
// the Pixel Game Engine as part of this translation unit
#define OLC_PGE3_APPLICATION
#include "../olcPixelGameEngine3.h"
#include "../utilities/olcUTIL3_SWRender.h"

struct TextSurface
{
	TextSurface()
	{
		sSurface.resize(vSize.area(), '-');
	}
	
	std::string sSurface;
	olc::vi2d vSize = { 32, 30 };

	bool set(int x, int y, char c)
	{
		if (x >= 0 && y >= 0 && x < vSize.x && y < vSize.y)
		{
			sSurface[y * vSize.x + x] = c;
			return true;
		}
		else
			return false;
	}

	char get(int x, int y)
	{
		if (x >= 0 && y >= 0 && x < vSize.x && y < vSize.y)
		{
			return sSurface[y * vSize.x + x];
		}
		else
			return 'x';
	}

	int width() { return vSize.x; }
	int height() { return vSize.y; }
};



class Example_SoftwareRenderer : public olc::PixelGameEngine
{
public:
	Example_SoftwareRenderer()
	{
		sAppName = "Example - Lines";
	}

public:
	// Called once at the start, so create things here
	bool OnUserCreate() override
	{
		// Nothing to do here, so return true
		CreateImageFromFile(imgTest, "./assets/sanity_texture.png");
		return true;
	}

	// Called every frame, so update things here
	bool OnUserUpdate(float fElapsedTime) override
	{

		draw.Clear(olc::Colour::CYAN);
		draw.Pixel({ 0,0 },olc::Colour::GREEN);

		DrawSW.SetTarget(draw.GetTarget());
		
		DrawSW.Clear(olc::Colour::VERY_DARK_BLUE);
				
		DrawSW.Plot({ 4, 4 }, olc::Colour::RED);

		DrawSW.Line({ 10,4 }, mouse.GetPosition(), olc::Colour::MAGENTA, olc::Colour::VERY_DARK_BLUE);

		DrawSW.FilledRect({ 10,22 }, { 32, 32 }, olc::Colour::BLACK);

		DrawSW.FilledRect({ 6, 18 }, { 32, 32 },
			olc::Colour::RED, olc::Colour::YELLOW,
			olc::Colour::GREEN, olc::Colour::MAGENTA);

		DrawSW.Rect({ 6, 18 }, { 32, 32 }, olc::Colour::WHITE);

		



		DrawSW.WorldOffset({ 68, 68 });
		DrawSW.WorldRotate(TotalTimeElapsed(), { 0 ,0 });
		DrawSW.FilledRect({ -16, -16 }, { 32, 32 }, olc::Colour::BLACK);
		
		DrawSW.WorldReset();
		DrawSW.WorldOffset({ 64, 64 });
		DrawSW.WorldRotate(TotalTimeElapsed(), { 0 ,0 });
		DrawSW.FilledRect({ -16, -16 }, { 32, 32 },
			olc::Colour::RED, olc::Colour::YELLOW,
			olc::Colour::GREEN, olc::Colour::MAGENTA);

		DrawSW.Rect({ -16, -16 }, { 32, 32 }, olc::Colour::WHITE);


		DrawSW.WorldReset();
		DrawSW.WorldOffset({ 128, 68 });
		DrawSW.WorldRotate(TotalTimeElapsed(), { 0 ,0 });
		DrawSW.WorldScale({ 2, 2 });
		DrawSW.FilledRect({ -16, -16 }, { 32, 32 }, olc::Colour::BLACK);

		DrawSW.WorldReset();
		DrawSW.WorldOffset({ 124, 64 });
		DrawSW.WorldRotate(TotalTimeElapsed(), { 0 ,0 });
		DrawSW.WorldScale({ 2, 2 });
		DrawSW.TexturedRect({ -16, -16 }, { 32, 32 },
			imgTest);

		



		DrawSW.WorldReset();
		DrawSW.TexturedTriangle(
			{ 20,180 }, 
			mouse.GetPosition(), 
			{ 250, 230 }, 
			olc::Colour::RED, 
			olc::Colour::GREEN, 
			olc::Colour::BLUE,
			{ 0,0 },
			{ 1,0 },
			{ 1,1 }, imgTest);



		DrawSW.Circle(mouse.GetPosition(), 20, olc::Colour::GREEN);
		DrawSW.FilledCircle(mouse.GetPosition(), 10, olc::Colour::CYAN);


		//// Clear
		//TextDraw.SetTarget(textSurface);
		//
		//
		//TextDraw.Clear('-');


		//TextDraw.FilledTriangle(
		//	{ 20 / 8, 180 / 8 },
		//	mouse.GetPosition() / 8,
		//	{ 250 / 8, 230 / 8 },
		//	'O');


		//TextDraw.Line({ 10 / 8,10 / 8 }, mouse.GetPosition() / 8, 'A');


		//// Draw TextSurface to Screen
		//for (int y = 0; y < textSurface.height(); y++)
		//{
		//	for (int x = 0; x < textSurface.width(); x++)
		//	{
		//		draw.String({ x * 8.0f, y * 8.0f }, std::string(1, textSurface.get(x, y)));
		//	}
		//}

		// Successful frame
		return true;
	}

protected:
	olc::utils::SWDraw DrawSW;
	olc::Image imgTest;

	TextSurface textSurface;
	olc::utils::SoftwareRenderer<TextSurface, char> TextDraw;
};


// Main entry point for the application
int main()
{
	// Construct demo application
	Example_SoftwareRenderer demo;

	// Create "screen" of 256x240 "pixels"
	// with a pixel size of 4x4 actual screen pixels
	if (demo.Construct({ 256, 240 }, { 4, 4 }))
	{
		// Start the application
		demo.Start();
	}

	return 0;
}