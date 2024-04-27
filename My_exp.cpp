#include <stdio.h>
#include <iostream>
#include <fstream>
#include <random>
#include <riscv-vector.h>
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
void FLOAT2INT(vfloat32m4_t f, vfloat32m4_t rf, vuint32m4_t fint, int _vl)
{
    vfloat32m4_t tmp = vfadd_vf_f32m4(f, 12582912.0f, _vl);
    rf = vfsub_vf_f32m4(tmp, 12582912.0f, _vl);
    fint = vreinterpret_v_f32m4_u32m4(tmp);

    fint = vand_vx_u32m4(fint, 0xFFFF, _vl);
}



vfloat32m4_t my_exp0(vfloat32m4_t x, int vl) {
    vfloat32m4_t y, kf;
    vuint32m4_t ki;


    float Log2 = (float)0x1.62e43p-1;
    float Log2h = (float)0xb.17200p-4;
    float Log2l = (float)0x1.7f7d1cf8p-20;
    float InvLog2 = (float)0x1.715476p0;

    // Here should be the tests for exceptional cases
    vfloat32m4_t x_mult_InvLog2 = vfmul_vf_f32m4(x, InvLog2, vl);
    FLOAT2INT(x_mult_InvLog2, kf, ki, vl);

    //y = (x - kf*Log2h) - kf*Log2l;
    vfloat32m4_t kf_mult_Log2h = vfmul_vf_f32m4(kf, Log2h, vl);
    vfloat32m4_t kf_mult_Log2l = vfmul_vf_f32m4(kf, Log2l, vl);
    vfloat32m4_t  x_sub_kflog2h = vfsub_vv_f32m4(x, kf_mult_Log2h, vl);
    y = vfsub_vv_f32m4(x_sub_kflog2h, kf_mult_Log2l, vl);

    vfloat32m4_t result;
    result = vfmul_vf_f32m4(y, (float)0x1.6850e4p-10, vl);
    result = vfadd_vf_f32m4(result, (float)0x1.123bccp-7, vl);
    result = vfmul_vv_f32m4(y, result, vl);
    result = vfadd_vf_f32m4(result, (float)0x1.555b98p-5, vl);
    result = vfmul_vv_f32m4(y, result, vl);
    result = vfadd_vf_f32m4(result, (float)0x1.55548ep-3, vl);
    result = vfmul_vv_f32m4(y, result, vl);
    result = vfadd_vf_f32m4(result, (float)0x1.fffff8p-2, vl);
    result = vfmul_vv_f32m4(y, result, vl);
    result = vfadd_vf_f32m4(result, (float)0x1p0, vl);
    result = vfmul_vv_f32m4(y, result, vl);
    result = vfadd_vf_f32m4(result, (float)0x1p0, vl);

    /* r.f = (float)0x1p0 + y * ((float)0x1p0
                        + y * ((float)0x1.fffff8p-2
                        + y * ((float)0x1.55548ep-3
                        + y * ((float)0x1.555b98p-5
                        + y * ((float)0x1.123bccp-7
                        + y *  (float)0x1.6850e4p-10)))));*/

                        //  r.i16[HI] += k << 7; //Exponent update
    vuint32m4_t resint = vreinterpret_v_f32m4_u32m4(result);

    vuint32m4_t tmp_exp = vand_vx_u32m4(resint, 0x7F800000u, vl);
    vuint32m4_t tmp_exp_srl = vsrl_vx_u32m4(tmp_exp, 23, vl);

    tmp_exp_srl = vadd_vv_u32m4(tmp_exp_srl, ki, vl); 
    vuint32m4_t tmp_exp_sll = vsll_vx_u32m4(tmp_exp_srl, 23, vl);
    vuint32m4_t resint_and_mas = vand_vx_u32m4(resint, 0x807FFFFFu, vl);
    resint = vor_vv_u32m4(resint_and_mas, tmp_exp_sll, vl);

    // vfloat32m4_t exp_vect = vreinterpret_v_u32m4_f32m4(resint);

     //return exp_vect;
    return result;
}

int main() 
{

    std::default_random_engine rd(0);//генератор случайных чисел
    std::uniform_real_distribution<float> dist1(-5.0f, 10.0f);
    float* S0 = new float[16];
    float* my_exponent = new float[16];
    float* exponent = new float[16];

    for (int i = 0; i < 16; i++)
    {

        S0[i] = dist1(rd);
    }

    vfloat32m4_t so = vle_v_f32m4(S0, 16);
    vfloat32m4_t exp_val = my_exp0(S0, 16);
    vse_v_f32m4(my_exponent, exp_val, 16);

    for (int i = 0; i < 16; i++)
    {
        exponent[i] = exp(S0[i]);
    }
    float* diff = new float[16];
    float max_diff = 0.0f;
    for (int i = 0; i < 16; i++)
    {
        diff[i] = abs(my_exponent[i]-exponent[i]);
        if (diff[i] > max_diff) max_diff = diff[i];
        printf("x=%.12ef\nmy_exp=%.12ef\nexpf=%.12ef\ndiff=%i  max_diff=%i\n",
            S0[i], my_exponent[i], exponent[i], diff[i], max_diff);
    }
 

      
            
   

    return 0;
}