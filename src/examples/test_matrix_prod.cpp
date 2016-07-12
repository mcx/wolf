/**
 * \file test_matrix_prod.cpp
 *
 *  Created on: May 26, 2016
 *      \author: jsola
 */

#include "eigen3/Eigen/Dense"

//std includes
#include <ctime>
#include <iostream>
#include <iomanip>

#define DECLARE_MATRICES(s) \
        Matrix<double, s, s, RowMajor> R1, R2, Ro; \
        Matrix<double, s, s, ColMajor> C1, C2, Co;

#define INIT_MATRICES(s) \
        R1.setRandom(s, s);\
        R2.setRandom(s, s);\
        C1.setRandom(s, s);\
        C2.setRandom(s, s);\
        Ro.setRandom(s, s);\
        Co.setRandom(s, s);


#define LOOP_MATRIX(N,Mo,M1,M2) \
        for (int i = 0; i < N; i++) \
        { \
            Mo = M1 * M2; \
            M1(2,2) = Mo(2,2); \
        }

#define EVALUATE_MATRIX(N,Mo,M1,M2) \
        t0 = clock(); \
        LOOP_MATRIX(N,Mo,M1,M2) \
        t1 = clock(); \
        std::cout << std::setw(15) << Mo(2,2) << "\t";

#define EVALUATE_ALL \
        EVALUATE_MATRIX(N, Ro, R1, R2)\
        std::cout << "Time Ro = R * R: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Ro, R1, C2)\
        std::cout << "Time Ro = R * C: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Ro, C1, R2)\
        std::cout << "Time Ro = C * R: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Ro, C1, C2)\
        std::cout << "Time Ro = C * C: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Co, R1, R2)\
        std::cout << "Time Co = R * R: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Co, R1, C2)\
        std::cout << "Time Co = R * C: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Co, C1, R2)\
        std::cout << "Time Co = C * R: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;\
        EVALUATE_MATRIX(N, Co, C1, C2)\
        std::cout << "Time Co = C * C: " << (long double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N \
        << "ns <-- this is the Eigen default!" << std::endl;



/**
 * We multiply matrices and see how long it takes.
 * We compare different combinations of row-major and column-major to see which one is the fastest.
 * We can select the matrix size.
 */
int main()
{
    using namespace Eigen;

    int N = 100000;
    const int S = 6;
    Matrix<double, 16, S - 3 + 1> results;
    clock_t t0, t1;

    // All dynamic sizes
    {
        Matrix<double, Dynamic, Dynamic, RowMajor> R1, R2, Ro;
        Matrix<double, Dynamic, Dynamic, ColMajor> C1, C2, Co;

        for (int s = 3; s <= S; s++)
        {
            std::cout << "Timings for dynamic matrix product. R: row major matrix. C: column major matrix. " << s << "x"
                    << s << " matrices." << std::endl;

            INIT_MATRICES(s)
            EVALUATE_ALL

        }
    }
    // Statics, one by one
    {
        const int s = 3;
        std::cout << "Timings for static matrix product. R: row major matrix. C: column major matrix. " << s << "x" << s
                << " matrices." << std::endl;

        DECLARE_MATRICES(s)
        INIT_MATRICES(s)
        EVALUATE_ALL
    }
    {
        const int s = 4;
        std::cout << "Timings for static matrix product. R: row major matrix. C: column major matrix. " << s << "x" << s
                << " matrices." << std::endl;

        DECLARE_MATRICES(s)
        INIT_MATRICES(s)
        EVALUATE_ALL
    }
    {
        const int s = 5;
        std::cout << "Timings for static matrix product. R: row major matrix. C: column major matrix. " << s << "x" << s
                << " matrices." << std::endl;

        DECLARE_MATRICES(s)
        INIT_MATRICES(s)
        EVALUATE_ALL
    }
    {
        const int s = 6;
        std::cout << "Timings for static matrix product. R: row major matrix. C: column major matrix. " << s << "x" << s
                << " matrices." << std::endl;

        DECLARE_MATRICES(s)
        INIT_MATRICES(s)
        EVALUATE_ALL
    }

    std::cout << "Test q and R rotations" << std::endl;
    Eigen::Quaterniond q(Eigen::Vector4d::Random().normalized());
    Eigen::Matrix3d R = q.matrix();
    Eigen::Vector3d v; v << 1,2,3; double vn = v.norm();

    N *= 100;

    t0 = clock();
    for (int i = 0; i < N; i++)
    {
        v = R * v;
    }
    t1 = clock();
    std::cout << "Time w = R * v: " << (double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;
    std::cout << "v norm change: " << 10*logl((long double)v.norm()/(long double)vn) << " dB" << std::endl;

    v << 1,2,3;
    t0 = clock();
    for (int i = 0; i < N; i++)
    {
        v = q * v;
    }
    t1 = clock();
    std::cout << "Time w = q * v: " << (double)(t1 - t0) * 1e9 / CLOCKS_PER_SEC / N << "ns" << std::endl;
    std::cout << "v norm change: " << 10*logl((long double)v.norm()/(long double)vn) << " dB" << std::endl;
    return 0;
}

