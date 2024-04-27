#include <stdio.h>
#include <inttypes.h>
#include <iostream>
//#include <gmp.h>
//#include <mpfr.h>
#include <fstream>
#include <random>
float* readDataFromFile(const char* filename, int& size) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file " << filename << std::endl;
        return nullptr;
    }

    // Определение размера массива данных
    file >> size;

    // Выделение памяти под динамический массив
    float* data = new float[size];

    // Считывание данных из файла в массив
    for (int i = 0; i < size; ++i) {
        file >> data[i];
    }

    file.close();
    return data;
}
void update_exponent(float& num, int k) {
    // Первым делом, получим представление числа в виде целого для доступа к экспоненте
    uint32_t* num_as_int = reinterpret_cast<uint32_t*>(&num);

    // Извлечение экспоненты
    uint32_t exponent = (*num_as_int & 0x7F800000) >> 23;

    // Обновление экспоненты
    exponent += k;

    // Проверка на переполнение
  /*  if (exponent >= 255)
        exponent = 255;
    else if (exponent <= 0)
        exponent = 0;
        */
        // Установка обновленной экспоненты
    *num_as_int = (*num_as_int & 0x807FFFFFu) | (exponent << 23);
}
typedef union {
    float f;
    uint16_t i16[2];//bigendian xranenieФ
    uint32_t i32;
} binary32;

#define HI 1
#define LO 0

#define FLOAT2INT(_ri, _rf, x) \
    { binary32 t;        \
      t.f = (x + 12582912.0f); \
      _rf = (t.f - 12582912.0f);\
      _ri = t.i16[LO]; }


void float_to_int(float& x, float& rf, uint32_t& fint)
{
    float tmp = (x + 12582912.0f);
    rf = tmp - 12582912.0f;
    fint = reinterpret_cast<uint32_t&>(tmp);
    fint = (fint & 0xFFFF);
}
float my_exp(float x) {
    float y, kf;
    uint32_t ki;
    float Log2 = (float)0x1.62e43p-1;
    float Log2h = (float)0xb.17200p-4;
    float Log2l = (float)0x1.7f7d1cf8p-20;
    float InvLog2 = (float)0x1.715476p0;
    // Here should be the tests for exceptional cases
    float X = x * InvLog2;
    float_to_int(X, kf, ki);

    //y = x - kf * Log2;
    y = (x - kf * Log2h) - kf * Log2l;

    float result = (float)0x1p0 + y * ((float)0x1p0
        + y * ((float)0x1.fffff8p-2
            + y * ((float)0x1.55548ep-3
                + y * ((float)0x1.555b98p-5
                    + y * ((float)0x1.123bccp-7
                        + y * (float)0x1.6850e4p-10)))));
    uint32_t resint = reinterpret_cast<float&>(result);


    update_exponent(result, ki);
    return result;
}

float my_exp0(float x) {
    float y, kf;
    int16_t k;
    binary32 r;

    float Log2 = (float)0x1.62e43p-1;
    float Log2h = (float)0xb.17200p-4;
    float Log2l = (float)0x1.7f7d1cf8p-20;
    float InvLog2 = (float)0x1.715476p0;

    // Here should be the tests for exceptional cases
    FLOAT2INT(k, kf, x * InvLog2);

    //y = x - kf * Log2;
    y = (x - kf * Log2h) - kf * Log2l;

    r.f = (float)0x1p0 + y * ((float)0x1p0
        + y * ((float)0x1.fffff8p-2
            + y * ((float)0x1.55548ep-3
                + y * ((float)0x1.555b98p-5
                    + y * ((float)0x1.123bccp-7
                        + y * (float)0x1.6850e4p-10)))));
    std::cout << "r.f = " << r.f << std::endl;

    r.i16[HI] += k << 7; //Exponent update
    return r.f;
}

int main() {
    binary32 ref, r;
    float x;
    int32_t diff, max_diff = 0;

    r.f = (float)0x2.c5c85fdf473dep8;//0x2.c5c85fp3f; // Начальное значение для генерации случайных чисел
    ref.f = 0x1.0p-10f; // Ограничивающее значение

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> distrib(ref.i32, r.i32); // Равномерное распределение

    for (int i = 0; i < 10; i++) {
        // r.i32 = distrib(gen);
        x = r.f;

        r.f = my_exp0(x);



        ref.f = exp(x);

        diff = r.i32 - ref.i32; // Сравнение результатов

        if (abs(diff) > max_diff) {
            max_diff = abs(diff);
        }

      
            // printf("x=%.12ef\nmy_exp=%.12ef\nexpf=%.12ef\ndiff=%f  max_diff=%f\n",
                        //x, r.f, ref.f, diff, max_diff);
   printf("x=%.12ef\nmy_exp=%.12ef\nexpf=%.12ef\ndiff=%i  max_diff=%i\n",
                x, r.f, ref.f, diff, max_diff);
    }

    return 0;
}