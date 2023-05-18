#include "Layer.H"
#include <cmath>
#include <cblas.h>
#include <sstream>
#include <fstream>
#include <cassert>
#include <iostream>

template<>
void Linear<float>::forward(const Tensor<float>& input, Tensor<float>& output){
    assert(input.dim_num() == 2);
    assert(output.dim_num() == 2);
    assert(input.dim(0) == output.dim(0));
    assert(input.dim(1) == in_features_);
    assert(out_features_ == output.dim(1));
    const float alpha = 1.f;
    const float beta = 1.f;
    int m = input.dim(0);
    int n = out_features_;
    int k = in_features_;
    const float* A = input.data();
    int lda = input.dim(1);
    const float* B = weights_.data();
    int ldb = out_features_;
    float*  C = output.data();
    int ldc = output.dim(1);
    #pragma omp parallel for
    for(int i = 0; i < m; ++i)
        std::copy(bias_.data(), bias_.data() + n, C + i * n);
    cblas_sgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,
        m,n,k,
        alpha,A,lda,
        B,ldb,
        beta,C,ldc);
}

template<>
void Linear<float>::load_parameters(const std::string& dir, int64_t layer_id){
    std::stringstream ss1,ss2;
    ss1 << dir << "/" << "linear_" << layer_id << "_weights_rowmajor_" << in_features_ << "_" << out_features_ << ".data";
    std::string weights_path = ss1.str();
    ss2 << dir << "/" << "linear_" << layer_id << "_bias_" << out_features_ << ".data";
    std::string bias_path = ss2.str();

    // weights
    std::ifstream weights_file(weights_path, std::ios::binary);
    if(!weights_file.is_open()){
        std::cerr << "open weights file error : " << weights_path << std::endl << std::flush;
        abort();
    }
    weights_file.read(reinterpret_cast<char*>(weights_.data()), weights_.bytes_num());
    weights_file.close();

    // bias
    std::ifstream bias_file(bias_path, std::ios::binary);
    if(!bias_file.is_open()){
        std::cerr << "open bias file error : " << bias_path << std::endl << std::flush;
        abort();
    }
    bias_file.read(reinterpret_cast<char*>(bias_.data()), bias_.bytes_num());
    bias_file.close();
}

template<>
void LinearGELU<float>::load_parameters(const std::string& dir, int64_t layer_id){
    std::stringstream ss1,ss2;
    ss1 << dir << "/" << "linear_" << layer_id << "_weights_rowmajor_" << in_features_ << "_" << out_features_ << ".data";
    std::string weights_path = ss1.str();
    ss2 << dir << "/" << "linear_" << layer_id << "_bias_" << out_features_ << ".data";
    std::string bias_path = ss2.str();
    
    // weights
    std::ifstream weights_file(weights_path, std::ios::binary);
    if(!weights_file.is_open()){
        std::cerr << "open weights file error : " << weights_path << std::endl << std::flush;
        abort();
    }
    weights_file.read(reinterpret_cast<char*>(weights_.data()), weights_.bytes_num());
    weights_file.close();

    // bias
    std::ifstream bias_file(bias_path, std::ios::binary);
    if(!bias_file.is_open()){
        std::cerr << "open bias file error : " << bias_path << std::endl << std::flush;
        abort();
    }
    bias_file.read(reinterpret_cast<char*>(bias_.data()), bias_.bytes_num());
    bias_file.close();
}


void gelu_navie(int64_t len, float* data){
    #pragma omp parallel for
    for(int64_t i = 0; i < len; ++i){
        float x = data[i];
        data[i] = 0.5 * x * (1.f + tanhf(sqrtf(2.f / M_PI) * (x + 0.044715f * powf(x, 3.f))));
    }
}

inline float tanhf_exp(float x){
    if(x > 8.f) return 1.;
    if(x < -8.f) return -1.;
    return 1.f - 2.f / (expf(2.f * x) + 1.f);
}

double tanh_exp(double x){
    return 1. - 2. / (exp(2. * x) + 1.);
}

void geluf_exp(int64_t len, float* data){
    const float const_1 = sqrtf(2.f / M_PI);
    const float const_2 = 0.044715f;
    const float one = 1.f;
    const float half = 0.5;

    #pragma omp parallel for
    for(int64_t i = 0; i < len; ++i){
        float x = data[i];
        data[i] = half * x * (one + tanhf_exp(const_1 * (x + const_2 * x * x * x)));
    }
}

void gelud_exp(int64_t len, float* data){
    const double const_1 = sqrtf(2. / M_PI);
    const double const_2 = 0.044715;
    const double one = 1.;
    const double half = 0.5;

    #pragma omp parallel for
    for(int64_t i = 0; i < len; ++i){
        double x = data[i];
        data[i] = half * x * (one + tanh_exp(const_1 * (x + const_2 * x * x * x)));
    }
}

// TODO 
void gelu_exp_sve(int64_t len, float* data){

}

template<>
void LinearGELU<float>::forward(const Tensor<float>& input, Tensor<float>& output){
    assert(input.dim_num() == 2);
    assert(output.dim_num() == 2);
    assert(input.dim(0) == output.dim(0));
    assert(input.dim(1) == in_features_);
    assert(out_features_ == output.dim(1));
    const float alpha = 1.f;
    const float beta = 1.f;
    int m = input.dim(0);
    int n = out_features_;
    int k = in_features_;
    const float* A = input.data();
    int lda = input.dim(1);
    const float* B = weights_.data();
    int ldb = out_features_;
    float*  C = output.data();
    int ldc = output.dim(1);
    #pragma omp parallel for
    for(int i = 0; i < m; ++i)
        std::copy(bias_.data(), bias_.data() + n, C + i * n);
    cblas_sgemm(CblasRowMajor,CblasNoTrans,CblasNoTrans,
        m,n,k,
        alpha,A,lda,
        B,ldb,
        beta,C,ldc);
    
    // GELU
    geluf_exp(output.element_num(), output.data());
}

template class Tensor<float>;
template class Linear<float>;
template class LinearGELU<float>;