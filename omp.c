#define PNG_NO_SETJMP

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <math.h>
int main(int argc, char** argv) {
    int num_threads = strtol(argv[1], 0, 10);
    double left = strtod(argv[2], 0);
    double right = strtod(argv[3], 0);
    double lower = strtod(argv[4], 0);
    double upper = strtod(argv[5], 0);
    int width = strtol(argv[6], 0, 10);
    int height = strtol(argv[7], 0, 10);
    const char* filename = argv[8];

    int* image = (int*)malloc(width * height * sizeof(int));
	
	double avgH = (upper - lower) / height;
	double avgW = (right - left) / width;
	
	int idx=(height*0.004);
    #pragma omp parallel for schedule(dynamic) num_threads(num_threads) collapse(2)
    for (int j = idx; j < height; ++j) {        
        for (int i = 0; i < width; ++i) {
			double y0 = j * avgH + lower;
            double x0 = i * avgW + left;

            int repeats = 0;
            double x = 0;
            double y = 0;
            double length_squared = 0;
			
			double p = sqrt((x0 - 0.25) * (x0 - 0.25) + y0 * y0);
			double q = (x0 + 1) * (x0 + 1) + y0 * y0;
			if ((x0 < p - p * p * 2 + 0.25 || q < 0.0625)) 
				repeats = 10000;
			else
				while (repeats < 10000 && length_squared < 4) {
					double temp = x * x - y * y + x0;
					y = 2 * x * y + y0;
					x = temp;
					length_squared = x * x + y * y;
					++repeats;
				}
            image[j * width + i] = repeats;
        }
    }

	FILE* fp = fopen(filename, "wb");
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    size_t row_size = 3 * width * sizeof(png_byte);
    png_bytep row = (png_bytep)malloc(row_size);
    for (int y = 0; y < height; ++y) {
        memset(row, 0, row_size);
		int tp = (height - 1 - y) * width;
        #pragma omp parallel for schedule(dynamic) num_threads(num_threads) 
		for (int x = 0; x < width; ++x) {
			int p = image[tp + x];
            png_bytep color = row + x * 3;
            if (p != 10000) {
                if (p & 16) {
                    color[0] = 240;
                    color[1] = color[2] = ((p & 0xf) << 4);
                } else {
                    color[0] = ((p & 0xf) << 4);
                }
            }
        }
        png_write_row(png_ptr, row);
    }
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}
