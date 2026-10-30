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

float edge(Vec3 a, Vec3 b, Vec3 p) {
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
void drawTriangle(vector<Color>& pixels, vector<float>& zbuffer, int width, int height,
	Vec3 A, Vec3 B, Vec3 C, Color cA, Color cB, Color cC){
	int minX = max(0, (int)min({ A.x, B.x, C.x }));
	int maxX = min(width - 1, (int)max({ A.x, B.x, C.x }));
	int minY = max(0, (int)min({ A.y, B.y, C.y }));
	int maxY = min(height - 1, (int)max({ A.y, B.y, C.y }));
	float area = edge(A, B, C);
	if (area == 0) return;//треугольник сплющен в линию
	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			Vec3 p = { x + 0.5f, y + 0.5f, 0 };//центр пикселя
			float w0 = edge(B, C, p);
			float w1 = edge(C, A, p);
			float w2 = edge(A, B, p);
			float alpha = w0 / area;
			float beta = w1 / area;
			float gamma = w2 / area;

			if (alpha >= 0 && beta >= 0 && gamma >= 0) {
				float z = alpha * A.z + beta * B.z + gamma * C.z;//глубина в этом пикселе
				int idx = y * width + x;
				if (z <= zbuffer[idx]) continue;//тут уже нарисовано что-то ближе, пропускаем
				zbuffer[idx] = z;//запоминаем новую глубину

				Color c;
				c.r = (unsigned char)(alpha * cA.r + beta * cB.r + gamma * cC.r);
				c.g = (unsigned char)(alpha * cA.g + beta * cB.g + gamma * cC.g);
				c.b = (unsigned char)(alpha * cA.b + beta * cB.b + gamma * cC.b);
				pixels[y * width + x] = c;
				
			}
		}
	}
}
//переводит точку модели (от -1 до 1) в пиксели экрана, z оставляем для глубины
Vec3 toScreen(Vec3 v, int width, int height) {
	return { (v.x + 1) * width / 2, (v.y + 1) * height / 2, v.z };
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
	vector<float> zbuffer(width * height, -1e9f);//изначально везде "бесконечно далеко"
	
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pixels[y * width + x] = { 30, 60, 150 };
		}
	}
	for (const Face& f : model.faces) {
		Vec3 A = toScreen(model.verts[f.v[0]], width, height);
		Vec3 B = toScreen(model.verts[f.v[1]], width, height);
		Vec3 C = toScreen(model.verts[f.v[2]], width, height);

		Color c = { (unsigned char)(rand() % 256), (unsigned char)(rand() % 256), (unsigned char)(rand() % 256) };
		drawTriangle(pixels, zbuffer, width, height, A, B, C, c, c, c);
	}
	stbi_flip_vertically_on_write(1);
	stbi_write_png("output.png", width, height, 3, pixels.data(), width * 3);
}
