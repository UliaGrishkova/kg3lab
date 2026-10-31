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
#include <cmath>

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
//скалярное произведение: насколько две стрелки смотрят в одну сторону
float dot(Vec3 a, Vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

//делает длину вектора равной 1, направление не меняется
Vec3 normalize(Vec3 v) {
	float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	return { v.x / len, v.y / len, v.z / len };
}
//разность точек: стрелка из b в a
Vec3 sub(Vec3 a, Vec3 b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

//векторное произведение: стрелка, перпендикулярная a и b
Vec3 cross(Vec3 a, Vec3 b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

struct Vec4 {
	float x, y, z, w;//точка в 3D + четвёртое число для матриц
};

struct Mat4 {
	float m[4][4];//m[строка][столбец]
};
struct Face {
	int v[3];//номера трёх вершин
	int vt[3];//номера текстурных координат
	int vn[3];//номера нормалей (для освещения)
};

struct Model {
	vector<Vec3> verts;//все вершины из строк "v"
	vector<Face> faces;//все треугольники из строк "f"
	vector<Vec3> norms;//все нормали из строк "vn"
};
Mat4 identityMatrix() {
	Mat4 r = {};//все элементы 0
	for (int i = 0; i < 4; i++) r.m[i][i] = 1;//единицы на диагонали
	return r;
}

Vec4 mul(Mat4 a, Vec4 v) {
	return {
		a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z + a.m[0][3] * v.w,
		a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z + a.m[1][3] * v.w,
		a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z + a.m[2][3] * v.w,
		a.m[3][0] * v.x + a.m[3][1] * v.y + a.m[3][2] * v.z + a.m[3][3] * v.w
	};
}
//склеивает два преобразования в одно: сначала b, потом a
Mat4 mul(Mat4 a, Mat4 b) {
	Mat4 r = {};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			for (int k = 0; k < 4; k++)
				r.m[i][j] += a.m[i][k] * b.m[k][j];
	return r;
}
//матрица вида: переводит мир в систему координат камеры
Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
	Vec3 z = normalize(sub(eye, target));//назад: от цели к камере
	Vec3 x = normalize(cross(up, z));//вправо
	Vec3 y = cross(z, x);//вверх для камеры

	Mat4 r = identityMatrix();
	r.m[0][0] = x.x; r.m[0][1] = x.y; r.m[0][2] = x.z; r.m[0][3] = -dot(x, eye);
	r.m[1][0] = y.x; r.m[1][1] = y.y; r.m[1][2] = y.z; r.m[1][3] = -dot(y, eye);
	r.m[2][0] = z.x; r.m[2][1] = z.y; r.m[2][2] = z.z; r.m[2][3] = -dot(z, eye);
	return r;
}
//матрица перспективы: дальнее становится меньше
//fov — угол обзора в градусах, aspect — ширина/высота экрана
Mat4 perspective(float fov, float aspect, float nearPlane, float farPlane) {
	float f = 1.0f / tan(fov * 3.14159265f / 180 / 2);//градусы в радианы, половина угла
	Mat4 r = {};
	r.m[0][0] = f / aspect;
	r.m[1][1] = f;
	r.m[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
	r.m[2][3] = 2 * farPlane * nearPlane / (nearPlane - farPlane);
	r.m[3][2] = -1;//вот тут в w кладётся расстояние до камеры
	return r;
}
//камера: где стоит, куда смотрит и где у неё верх
struct Camera {
	Vec3 position;
	Vec3 target;
	Vec3 up;
	float fov = 60;//угол обзора в градусах
	float aspect = 1;//ширина/высота картинки
	float nearPlane = 0.1f;//ближе этого не рисуем
	float farPlane = 100;//дальше этого не рисуем

	//матрица вида для этой камеры
	Mat4 view() const {
		return lookAt(position, target, up);
	}
	//матрица перспективы для этой камеры
	Mat4 projection() const {
		return perspective(fov, aspect, nearPlane, farPlane);
	}
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
		else if (type == "vn") {
			Vec3 n;
			s >> n.x >> n.y >> n.z;
			model.norms.push_back(n);
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
//пиксельный шейдер: считает цвет точки по модели освещения Фонга
//n нормаль, pos где точка в мире, lightDir направление на свет, camPos где камера
Color phongShader(Vec3 n, Vec3 pos, Vec3 lightDir, Vec3 camPos) {
	float ambient = 0.1f;//фоновый свет, чтобы тени не были чёрными

	float nl = dot(n, lightDir);
	float diffuse = max(0.0f, nl);//насколько точка повёрнута к свету

	Vec3 r = { 2 * nl * n.x - lightDir.x, 2 * nl * n.y - lightDir.y, 2 * nl * n.z - lightDir.z };//отражённый луч света
	Vec3 viewDir = normalize(sub(camPos, pos));//из точки на камеру
	float specular = pow(max(0.0f, dot(r, viewDir)), 32);//блик: чем больше степень, тем он меньше и резче
	if (nl <= 0) specular = 0;//на теневой стороне блика не бывает

	float light = ambient + 0.8f * diffuse + 0.4f * specular;//складываем всё вместе
	light = min(1.0f, light);//больше 1 нельзя, иначе цвет "перепрыгнет" через 255
	unsigned char v = (unsigned char)(255 * light);
	return { v, v, v };
}
void drawTriangle(vector<Color>& pixels, vector<float>& zbuffer, int width, int height,
	Vec3 A, Vec3 B, Vec3 C, Vec3 nA, Vec3 nB, Vec3 nC,
	Vec3 wA, Vec3 wB, Vec3 wC, Vec3 lightDir, Vec3 camPos) {
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

				//нормаль в этом пикселе смешиваем из нормалей вершин
				Vec3 n = {
				 alpha * nA.x + beta * nB.x + gamma * nC.x,
				 alpha * nA.y + beta * nB.y + gamma * nC.y,
				 alpha * nA.z + beta * nB.z + gamma * nC.z
				};
				n = normalize(n);//после смешивания длина уже не 1, исправляем

				//где эта точка находится в мире, смешиваем из вершин
				Vec3 pos = {
				 alpha * wA.x + beta * wB.x + gamma * wC.x,
				 alpha * wA.y + beta * wB.y + gamma * wC.y,
				 alpha * wA.z + beta * wB.z + gamma * wC.z
				};
				pixels[idx] = phongShader(n, pos, lightDir, camPos);
				
			}
		}
	}
}
//точку модели через матрицы камеры переводит в пиксели экрана
Vec3 toScreen(Vec3 v, Mat4 mvp, int width, int height) {
	Vec4 clip = mul(mvp, Vec4{ v.x, v.y, v.z, 1 });//применяем все матрицы сразу
	Vec3 ndc = { clip.x / clip.w, clip.y / clip.w, clip.z / clip.w };//перспективное деление
	return {
		(ndc.x + 1) * width / 2,
		(ndc.y + 1) * height / 2,
		-ndc.z//минус: у нас в z-буфере больше значит ближе, а тут наоборот
	};
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
	Vec3 lightDir = normalize({ 1, 1, 1 });//свет спереди, сверху и справа
	Camera cam = { { 0, 0, 3 }, { 0, 0, 0 }, { 0, 1, 0 } };//стоит на 3 спереди, смотрит в центр
	Mat4 mvp = mul(cam.projection(), cam.view());//сначала вид, потом перспектива
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pixels[y * width + x] = { 30, 60, 150 };
		}
	}
	for (const Face& f : model.faces) {
		Vec3 A = toScreen(model.verts[f.v[0]], mvp, width, height);
		Vec3 B = toScreen(model.verts[f.v[1]], mvp, width, height);
		Vec3 C = toScreen(model.verts[f.v[2]], mvp, width, height);

		drawTriangle(pixels, zbuffer, width, height, A, B, C,
			model.norms[f.vn[0]], model.norms[f.vn[1]], model.norms[f.vn[2]],
			model.verts[f.v[0]], model.verts[f.v[1]], model.verts[f.v[2]],
			lightDir, cam.position);

	}
	stbi_flip_vertically_on_write(1);
	stbi_write_png("output.png", width, height, 3, pixels.data(), width * 3);
}
