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
	Vec2 A, Vec2 B, Vec2 C, Color cA, Color cB, Color cC)
{
	int minX = max(0, (int)min({ A.x, B.x, C.x }));
	int maxX = min(width - 1, (int)max({ A.x, B.x, C.x }));
	int minY = max(0, (int)min({ A.y, B.y, C.y }));
	int maxY = min(height - 1, (int)max({ A.y, B.y, C.y }));
	float area = edge(A, B, C);
	if (area == 0) return;   //треугольник сплющен в линию
	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			Vec2 p = { x + 0.5f, y + 0.5f };   // центр пикселя

			float w0 = edge(B, C, p);
			float w1 = edge(C, A, p);
			float w2 = edge(A, B, p);
			float alpha = w0 / area;
			float beta = w1 / area;
			float gamma = w2 / area;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				Color c;
				c.r = (unsigned char)(alpha * cA.r + beta * cB.r + gamma * cC.r);
				c.g = (unsigned char)(alpha * cA.g + beta * cB.g + gamma * cC.g);
				c.b = (unsigned char)(alpha * cA.b + beta * cB.b + gamma * cC.b);
				pixels[y * width + x] = c;
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
		{ 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 });
	stbi_flip_vertically_on_write(1);
	stbi_write_png("output.png", width, height, 3, pixels.data(), width * 3);
}
