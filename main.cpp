#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>

#include "quaternion.h"
using namespace std;

//Basic configuration
#define WIDTH 1920
#define HEIGHT 1080
#define SAMPLES 8
#define FRAMES 361
#define MAX_ITERATION 256
#define FILENAME "wow/frame"

#define SAMPLER_START_X -2.5
#define SAMPLER_SIZE_X 4
#define SAMPLER_SIZE_Y ((double) RENDER_SIZE_X / WIDTH * HEIGHT)
#define SAMPLER_START_Y (-SAMPLER_SIZE_Y / 2.0)
#define SAMPLER_COUNT_X WIDTH
#define SAMPLER_COUNT_Y HEIGHT
#define THREADS 5

#define RENDER_START_X -2.5
#define RENDER_SIZE_X 4
#define RENDER_SIZE_Y ((double) RENDER_SIZE_X / WIDTH * HEIGHT)
#define RENDER_START_Y (-RENDER_SIZE_Y / 2.0)

#define TRESHOLD_R MAX_ITERATION
#define TRESHOLD_G 64
#define TRESHOLD_B 16


double random01(){
	return ((double) rand() / (RAND_MAX));
}

struct Point {
	double x, y;
};

struct Rect {
	double x, y, w, h;
};

struct RGB {
	atomic<int> r, g, b;
};

struct RGBdouble {
	double r, g, b;
	RGBdouble(const RGB& rgb) : r(rgb.r), g(rgb.g), b(rgb.b) {}
};

void mandelbrotForPoint(RGB *counters, double t, Point p, int maxIteration, Rect render){

	double tEasing = t * t * 1.5;

	Quaternion c = Quaternion(p.x, p.y, (t < 0 ? tEasing : 0), (t > 0 ? tEasing : 0));
	Quaternion z = c;

	int iteration = 0;

	while(z.magnitude() < 5 && iteration < maxIteration){
		iteration++;
		z = z.multiply(z).add(c);

		int zw = (z.w - render.x) / render.w * WIDTH;
		int zi = (z.i - render.y) / render.h * HEIGHT;

		if(zw >= 0 && zw < WIDTH && zi >= 0 && zi < HEIGHT){
			if(iteration <= TRESHOLD_R) counters[zi * WIDTH + zw].r++;
			if(iteration <= TRESHOLD_G) counters[zi * WIDTH + zw].g++;
			if(iteration <= TRESHOLD_B) counters[zi * WIDTH + zw].b++;
		}
	}
}

// Function for converting "counters" to 0-255 RGB array
int* transformCounters(RGB* counters){
	int *pixels = new int[WIDTH * HEIGHT * 3];

	RGBdouble maxValues = counters[0];
	for(int i = 0; i < WIDTH * HEIGHT; i++){
		if(maxValues.r < counters[i].r) maxValues.r = counters[i].r;
		if(maxValues.g < counters[i].g) maxValues.g = counters[i].g;
		if(maxValues.b < counters[i].b) maxValues.b = counters[i].b;
	}

	int maxValue = max(max(maxValues.r, maxValues.g), maxValues.b);


	for(int y = 0; y < HEIGHT; y++){
		for(int x = 0; x < WIDTH; x++){
			
			RGBdouble color = counters[y * WIDTH + x];

			// HERE WE HAVE SOME CRAZY EXPERIMENTAL TRANSFORMS
			// SO THAT THE IMAGE HAS NICE COLORS

			color.r = sqrt(color.r); // sqrt reduces a little peaks of brightness
			color.g = sqrt(color.g);
			color.b = sqrt(color.b);

			color.r /= sqrt(maxValue);
			color.g /= sqrt(maxValue);
			color.b /= sqrt(maxValue);

			color.r = 1 - color.r;
			color.g = 1 - color.g;
			color.b = 1 - color.b;

			color.r *= color.r*color.r*color.r*color.r*color.r;
			color.g *= color.g*color.g*color.g*color.g*color.g*color.g;
			color.b *= color.b*color.b*color.b*color.b*color.b;

			color.r = 1 - color.r;
			color.g = 1 - color.g;
			color.b = 1 - color.b;

			color.r *= color.r;

			// END OF CRAZY STUFF

			int cr = max(0.0, min(255.0, color.r * 256));
			int cg = max(0.0, min(255.0, color.g * 256));
			int cb = max(0.0, min(255.0, color.b * 256));
			
			pixels[(y * WIDTH + x) * 3 + 0] = cr;
			pixels[(y * WIDTH + x) * 3 + 1] = cg;
			pixels[(y * WIDTH + x) * 3 + 2] = cb;
		}
	}

	return pixels;
}

void writeToFile(string filename, int* pixels){

	cout << "writing file " << filename << endl;

	ofstream file;
	file.open(filename);

	if(!file.is_open()){
		return;
	}

	file << "P3" << endl;
	file << WIDTH << " " << HEIGHT << endl;
	file << 255 << endl;

	for(int i = 0; i < WIDTH * HEIGHT; i++){

		file << pixels[i * 3] << " " << pixels[i * 3 + 1] << " " << pixels[i * 3 + 2] << endl;
	}

	file.close();

	//Automatic conversion to png using mogrify/imagemagick
	//system(("mogrify -format png " + filename).c_str());
	//system(("rm " + filename).c_str());
}

void drawFrame(string filename, double t, int maxIteration, Point *points, int pointsCount){


	Rect render = {RENDER_START_X, RENDER_START_Y, RENDER_SIZE_X, RENDER_SIZE_Y};
	double sampleDistX = (double) SAMPLER_SIZE_X / SAMPLER_COUNT_X;
	double sampleDistY = (double) SAMPLER_SIZE_Y / SAMPLER_COUNT_Y;

	RGB *counters = new RGB[WIDTH * HEIGHT]();

	thread *threads = new thread[THREADS];

	for(int j = 0; j < THREADS; j++){

		threads[j] = thread([&](int start){
			for(int i = start; i < SAMPLER_COUNT_X * SAMPLER_COUNT_Y; i += THREADS){
				
				int x = i / SAMPLER_COUNT_Y;
				int y = i % SAMPLER_COUNT_Y;

				if(x % 100 == 0 && y == 0) cout << "column " << x << " / " << SAMPLER_COUNT_X << endl;

				for(int k = 0; k < pointsCount; k++){
					Point p = {
						SAMPLER_START_X + (x + points[k].x) * sampleDistX,
						SAMPLER_START_Y + (y + points[k].y) * sampleDistY
					};
					mandelbrotForPoint(counters, t, p, maxIteration, render);
				}
			}
		}, j);

	}

	for(int j = 0; j < THREADS; j++){

		threads[j].join();

	}

	int *pixels = transformCounters(counters);

	writeToFile(filename, pixels);

	delete[] threads;
	delete[] pixels;
	delete[] counters;
}

int main(int argc, char **argv){

	Point points[SAMPLES];

	srand(time(0));

	for(int i = 0; i < SAMPLES; i++){
		points[i].x = random01();
		points[i].y = random01();
	}


	for(int i = 0; i < FRAMES; i++)
	{
		cout << "Rendering frame " << i << endl;
		auto start = chrono::steady_clock::now();

		double t = 2.0 * i / FRAMES - 1;

		ostringstream filename;
		filename << FILENAME << setfill('0') << setw(ceil(log10(FRAMES))) << i << ".ppm";

		drawFrame(filename.str(), t, MAX_ITERATION, points, SAMPLES);

		auto end = chrono::steady_clock::now();
		cout << "Frame " << i << " took " << chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0 << "s" << endl;
	}
	return 0;
}

