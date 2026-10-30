// software3lab.cpp: определяет точку входа для приложения.
//
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>
#include "software3lab.h"
#include <algorithm>

using namespace std;

struct Color {
	unsigned char r, g, b;   // красный, зелёный, синий: от 0 до 255
};
struct Vec2 {
	float x, y;
};

float edge(Vec2 a, Vec2 b, Vec2 p) {
	return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);//с какой стороны отрезка находится точка p относительно отрезка ab
}
void drawTriangle(vector<Color>& pixels, int width, int height,
	Vec2 A, Vec2 B, Vec2 C, Color color)
{
	int minX = (int)min({ A.x, B.x, C.x });
	int maxX = (int)max({ A.x, B.x, C.x });
	int minY = (int)min({ A.y, B.y, C.y });
	int maxY = (int)max({ A.y, B.y, C.y });
	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			Vec2 p = { x + 0.5f, y + 0.5f };   // центр пикселя

			float w0 = edge(B, C, p);
			float w1 = edge(C, A, p);
			float w2 = edge(A, B, p);

			if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
				pixels[y * width + x] = color;
			}
		}
	}
}
int main()
{
	const int width = 800;
	const int height = 800;
	vector<Color> pixels(width * height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pixels[y * width + x] = { 30, 60, 150 };
		}
	}
	pixels[10 * width + 10] = { 255, 255, 255 };
	drawTriangle(pixels, width, height,
		{ 100, 100 }, { 700, 150 }, { 400, 700 },
		{ 255, 200, 0 });
	stbi_flip_vertically_on_write(1);
	stbi_write_png("output.png", width, height, 3, pixels.data(), width * 3);
}
