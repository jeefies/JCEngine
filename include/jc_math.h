#ifndef _JCENGINE_MATH_H_
#define _JCENGINE_MATH_H_

#include <vector>

template<typename T>
class Matrix {
public:
    int n, m;
    std::vector<T> M;

    Matrix(int n, int m) : n(n), m(m) {
        M.assign(n * m, T());
    }

    Matrix(const std::vector<std::vector<int>> &V) {
        n = V.size(), m = V[0].size();
        M.resize(n * m);
        for (int i = 0; i < n; ++i)
            std::copy(V[i].begin(), V[i].end(), M.begin() + i * m);
    }

    const T& at(int x, int y) const { return M[x * m + y]; }
    T& at(int x, int y) { return M[x * m + y]; }

    Matrix<T> operator* (const Matrix &B) const {
        if (m != B.n) throw "Matrix Size Not Match!";
        Matrix<T> C(n, B.m);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < m; ++j)
                for (int k = 0; k < B.m; ++k)
                    C.at(i, k) += at(i, j) * B.at(j, k);
        return C;
    }

    Matrix<T> operator+ (const Matrix &B) const {
        if (n != B.n || m != B.m) throw "Matrix Size Not Match!";
        Matrix<T> C(n, m);
        for (int i = 0, s = n * m; i < s; ++i)
            C.M[i] = M[i] + B.M[i];
        return C;
    }

    Matrix<T>& operator*= (const Matrix &A) {
        Matrix<T> C = *this * A;
        n = C.n, m = C.m;
        swap(M, C.M);
        return *this;
    }

    Matrix<T>& operator+= (const Matrix &A) {
        if (A.n != n || A.m != m) throw "Matrix Size Not Match!";
        for (int i = 0, n = A.n * A.m; i < n; ++i)
            M[i] += A.M[i];
        return *this;
    }

    friend std::ostream& operator<<(std::ostream &os, const Matrix &A) {
        os << "{";
        for (int i = 0; i < A.n; ++i) {
            os << "{";
            for (int j = 0; j < A.m; ++j)
                os << A.at(i, j) << ", ";
            os << "}" << "\n "[i == A.n - 1] << " ";
        }
        return os << "}";
    }
};

#endif // _JCENGINE_MATH_H_