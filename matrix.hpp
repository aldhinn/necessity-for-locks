/*

File: matrix.hpp
Author: Aldhinn Espinas
Description: This file contains matrix class declarations.

License: MIT License

Copyright (c) 2024-Present Aldhinn Espinas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#if !defined(MATRIX_HEADER_FILE)
#define MATRIX_HEADER_FILE

#include <atomic>
#include <initializer_list>
#include <stdexcept>
#include <cstring>

/// @brief A description of a 4x4 matrix containing atomic values.
class AtomicMatrix4x4 final {
public:
    inline AtomicMatrix4x4(const ::std::initializer_list<::std::initializer_list<double>>& values = {}) {
        if (values.size() > 4) {
            throw ::std::out_of_range("Cannot initialize a 4x4 matrix with more than 4 row vectors.");
        }

        // Row index tracker.
        int rowIndex = 0;
        for (const ::std::initializer_list<double>& rowVector : values) {
            if (rowVector.size() > 4) {
                throw ::std::out_of_range("Cannot initialize a 4x4 matrix with a vector with more than 4 elements.");
            }
            // Column index tracker.
            int colIndex = 0;
            for (double element : rowVector) {
                _data[rowIndex][colIndex].store(element);
                // Update column index.
                colIndex++;
            }
            // Fill with zeros for the rest of the index unfilled.
            for (; colIndex < 4; colIndex++) {
                _data[rowIndex][colIndex].store(double());
            }

            // Update row index.
            rowIndex++;
        }
        // Fill with zeros for the rest of the row vectors unfilled.
        for (; rowIndex < 4; rowIndex++) {
            for (int colIndex = 0; colIndex < 4; colIndex++) {
                _data[rowIndex][colIndex].store(double());
            }
            // Update row index.
            rowIndex++;
        }
    }

    /// @brief Get the reference to the element in the specified index.
    /// @param rowIndex The row-index of the element to be accessed.
    /// @param colIndex The column-index of the element to be accessed.
    /// @return The reference at the specified index.
    inline ::std::atomic<double>& operator()(unsigned int rowIndex, unsigned int colIndex) {
        if (rowIndex >= 4 || colIndex >= 4) {
            throw ::std::out_of_range("Invalid index.");
        }
        return _data[rowIndex][colIndex];
    }
    /// @brief Get the const reference to the element in the specified index.
    /// @param rowIndex The row-index of the element to be accessed.
    /// @param colIndex The column-index of the element to be accessed.
    /// @return The const reference at the specified index.
    inline const ::std::atomic<double>& operator()(unsigned int rowIndex, unsigned int colIndex) const {
        if (rowIndex >= 4 || colIndex >= 4) {
            throw ::std::out_of_range("Invalid index.");
        }
        return _data[rowIndex][colIndex];
    }

    /// @brief Copy constructor.
    /// @param other The other instance where data is being copied from.
    inline AtomicMatrix4x4(const AtomicMatrix4x4& other) {
        for (int rowIndex = 0; rowIndex < 4; rowIndex++) {
            for (int colIndex = 0; colIndex < 4; colIndex++) {
                _data[rowIndex][colIndex].store(other._data[rowIndex][colIndex].load());
            }
        }
    }
    /// @brief Copy assignment operator.
    /// @param other The other instance where data is being copied from.
    /// @return The reference to this matrix.
    inline AtomicMatrix4x4& operator=(const AtomicMatrix4x4& other) {
        for (int rowIndex = 0; rowIndex < 4; rowIndex++) {
            for (int colIndex = 0; colIndex < 4; colIndex++) {
                _data[rowIndex][colIndex].store(other._data[rowIndex][colIndex].load());
            }
        }
        return *this;
    }

private:
    /// @brief The container for the matrix components.
    ::std::atomic<double> _data[4][4];
};

/// @brief The dot product operation.
/// @param leftMat The left hand-side matrix.
/// @param rightMat The right hand-side matrix.
/// @return The dot product.
inline AtomicMatrix4x4 operator*(const AtomicMatrix4x4& leftMat, const AtomicMatrix4x4& rightMat) {
    AtomicMatrix4x4 dotProductMatrix;
    for (int rowIndex = 0; rowIndex < 4; rowIndex++) {
        for (int colIndex = 0; colIndex < 4; colIndex++) {
            // By definition, the j^th row and k^th column of the product matrix
            // corresponds to the dot product of the j^th row vector of leftMat
            // and the k^th column vector of rightMat.
            double dotProduct = 0.0;
            for (int i = 0; i < 4; i++) {
                dotProduct += leftMat(rowIndex, i).load() * rightMat(i, colIndex);
            }
            dotProductMatrix(rowIndex, colIndex).store(dotProduct);
        }
    }

    return dotProductMatrix;
}

/// @brief The equality comparator.
/// @param leftMat The left hand-side matrix.
/// @param rightMat The right hand-side matrix.
/// @return The equality value.
inline bool operator==(const AtomicMatrix4x4& leftMat, const AtomicMatrix4x4& rightMat) {
    // Iterate over each others's value to find inequalities.
    for (int rowIndex = 0; rowIndex < 4; rowIndex++) {
        for (int colIndex = 0; colIndex < 4; colIndex++) {
            // If there is one pair of elements that don't match,
            // the matrices are automatically deemed to be unequal.
            if (leftMat(rowIndex, colIndex).load() != rightMat(rowIndex, colIndex).load())
                return false;
        }
    }
    // Iterated over all elements and none were unequal, therefore both of these matrices are equal.
    return true;
}

/// @brief The inequality comparator.
/// @param leftMat The left hand-side matrix.
/// @param rightMat The right hand-side matrix.
/// @return The inequality value.
inline bool operator!=(const AtomicMatrix4x4& leftMat, const AtomicMatrix4x4& rightMat) {
    return !(leftMat == rightMat);
}

/// @brief The object that records and evaluates a matrix multiplication..
class MultiplicationRecorder {
public:
    /// @brief Init list constructor.
    /// @param leftMat The left hand-side matrix.
    /// @param rightMat The right hand-side matrix.
    /// @param dotProduct The dot product.
    inline MultiplicationRecorder(
        AtomicMatrix4x4 leftMat, AtomicMatrix4x4 rightMat, AtomicMatrix4x4 dotProduct
    ) : _leftMat(leftMat), _rightMat(rightMat), _dotProduct(dotProduct) {}

    /// @brief Determines if the calculation is correct.
    inline bool isCorrect() const {
        return _dotProduct == _leftMat * _rightMat;
    }

    /// @brief Copy constructor. 
    /// @param other The other instance where data is being copied from.
    inline MultiplicationRecorder(const MultiplicationRecorder& other) :
    _leftMat(other._leftMat), _rightMat(other._rightMat), _dotProduct(other._dotProduct) {}
    /// @brief Copy assignment operator.
    /// @param other The other instance where data is being copied from.
    /// @return The reference to this instance.
    inline MultiplicationRecorder& operator=(const MultiplicationRecorder& other) {
        _leftMat = other._leftMat;
        _rightMat = other._rightMat;
        _dotProduct = other._dotProduct;
        return *this;
    }
private:
    /// @brief The left hand-side matrix.
    AtomicMatrix4x4 _leftMat;
    /// @brief The right hand-side matrix.
    AtomicMatrix4x4 _rightMat;
    /// @brief The dot product.
    AtomicMatrix4x4 _dotProduct;
};

#endif
// End of file.
// DO NOT WRITE BEYOND HERE.