// software3lab.cpp: определяет точку входа для приложения.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <vector>
#include "software3lab.h"
#include <algorithm>
#include <fstream>
#include <sstream>   
#include <string>
#include <iostream>  
#include <clocale>

using namespace std;

struct Color {
	unsigned char r, g, b;//красный, зелёный, синий: от 0 до 255
};
struct Vec2 {
	float x, y;
};
struct Vec3 {
	float x, y, z;//точка или направление в 3D
};

struct Face {
	int v[3];//номера трёх вершин
	int vt[3];//номера текстурных координат
	int vn[3];//номера нормалей (для освещения)
};

struct Model {
	vector<Vec3> verts;//все вершины из строк "v"
	vector<Face> faces;//все треугольники из строк "f"
};

float edge(Vec2 a, Vec2 b, Vec2 p) {
	return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);//с какой стороны отрезка находится точка p относительно отрезка ab
}
bool loadObj(const string& path, Model& model)
{
	ifstream file(path);
	if (!file) return false;//файл не открылся

	string line;
	while (getline(file, line)) {//читаем файл
		istringstream s(line);//превращаем строку в "поток"
		string type;
		s >> type;//первое слово "v", "vt", "vn", "f" или что-то ещё

		if (type == "v") {
			Vec3 v;
			s >> v.x >> v.y >> v.z;
			model.verts.push_back(v);//добавить в конец массива
		}
		else if (type == "f") {
			Face f;
			char slash;//будем "выбрасывать" символы '/'
			for (int i = 0; i < 3; i++) {
				s >> f.v[i] >> slash >> f.vt[i] >> slash >> f.vn[i];
				f.v[i]--;  f.vt[i]--;  f.vn[i]--;
			}
			model.faces.push_back(f);
		}
	}
	return true;
}
void drawTriangle(vector<Color>& pixels, int width, int height,
	Vec2 A, Vec2 B, Vec2 C, Color cA, Color cB, Color cC)
{
	int minX = max(0, (int)min({ A.x, B.x, C.x }));
	int maxX = min(width - 1, (int)max({ A.x, B.x, C.x }));
	int minY = max(0, (int)min({ A.y, B.y, C.y }));
	int maxY = min(height - 1, (int)max({ A.y, B.y, C.y }));
	float area = edge(A, B, C);
	if (area == 0) return;//треугольник сплющен в линию
	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			Vec2 p = { x + 0.5f, y + 0.5f };//центр пикселя

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
	setlocale(LC_ALL, "Russian");
	Model model;
	if (!loadObj(string(MODELS_DIR) + "african_head.obj", model)) {
		cout << "Не удалось открыть модель\n";
		return 1;
	}
	cout << "вершин:" << model.verts.size() << "\n";
	cout << "треугольников:" << model.faces.size() << "\n";
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
