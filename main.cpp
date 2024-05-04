/*

File: main.cpp
Author: Aldhinn Espinas
Description: This file contains the implementation of the proof of concept
    that mutex locks are still necessary for some matrix operations.

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

#include <gtest/gtest.h>
#include <atomic>
#include <vector>
#include <random>
#include <time.h>
#include <thread>
#include <functional>
#include <iostream>
#include <mutex>

#include "matrix.hpp"

/// @brief The test suite fixture class for this test.
class TestSuiteFixture : public ::testing::Test {
protected:
    /// @brief Run `modifyingTestVariableValues` in the background.
    inline void runTestVariableModifier() {
        // Halt if already running.
        if (_shouldModificationsContinue.load()) return;

        // Update state.
        _shouldModificationsContinue.store(true);
        // Bind the runner.
        ::std::thread runningThread(::std::bind(
            &TestSuiteFixture::modifyingTestVariableValues, this
        ));
        // Run even after runningThread gets out of scope.
        runningThread.detach();
    }

    /// @brief The task that would continuously modify the values of the test variables.
    inline void modifyingTestVariableValues() {
        while (_shouldModificationsContinue.load()) {
            // No other thread should modify the test variables from here on.
            ::std::lock_guard<::std::mutex> lock(_mutex);

            // Randomly determine the indices to modify.
            unsigned int rowIndex = static_cast<unsigned int>(::std::rand()) % 4;
            unsigned int colIndex = static_cast<unsigned int>(::std::rand()) % 4;
            // Randomly determine the value.
            double newValue = static_cast<double>(::std::rand() % 4);
            // Modify _mat1.
            _mat1(rowIndex, colIndex).store(newValue);

            // Randomly determine the indices to modify.
            rowIndex = static_cast<unsigned int>(::std::rand()) % 4;
            colIndex = static_cast<unsigned int>(::std::rand()) % 4;
            // Randomly determine the value.
            newValue = static_cast<double>(::std::rand() % 4);
            // Modify _mat2.
            _mat2(rowIndex, colIndex).store(newValue);
        }
    }

    /// @brief Calculate the accuracy of calculations.
    /// @return The accuracy in percentage.
    inline double calculateAccuracy() const {
        unsigned long correctCalculations = 0;
        // Iterate over the calculations and count the correct ones.
        for (const MultiplicationRecorder& calculation : _calculations) {
            if (calculation.isCorrect()) correctCalculations++;
        }
        return (static_cast<double>(correctCalculations) * 100.0) /
            (static_cast<double>(_calculations.size()));
    }

protected:
    /// @brief A test matrix variable.
    AtomicMatrix4x4 _mat1;
    /// @brief Another test matrix variable.
    AtomicMatrix4x4 _mat2;
    /// @brief The variable that contains whether the method,
    /// `modifyingTestVariableValues` should continue running.
    ::std::atomic<bool> _shouldModificationsContinue;
    /// @brief The collection of recorded calculations.
    ::std::vector<MultiplicationRecorder> _calculations;
    /// @brief The mutex object.
    ::std::mutex _mutex;

public:
    /// @brief The function that runs before each test case.
    inline void SetUp() override {
        // Initialize test variables.
        _mat1 = {
            {1.0, 2.0, 0.0, 1.0},
            {0.0, 1.0, 1.0, 0.0},
            {1.0, 1.0, 0.0, 2.0},
            {1.0, 0.0, 1.0, 0.0}
        };
        _mat2 = {
            {2.0, 2.0, 0.0, 1.0},
            {1.0, 1.0, 1.0, 2.0},
            {1.0, 1.0, 3.0, 2.0},
            {1.0, 2.0, 1.0, 1.0}
        };
        // Set to default state.
        _shouldModificationsContinue.store(false);
        // Seed the random number generator.
        ::std::srand(::std::time(NULL));
    }
    /// @brief The function that runs after each test case.
    inline void TearDown() override {
        // Stop `modifyingTestVariableValues` that's running on the background.
        _shouldModificationsContinue.store(false);
        // Clear out the calculations.
        _calculations.clear();
    }
};

TEST_F(TestSuiteFixture, runCalculationsWithoutThreadLocks) {
    // Run the modifier in the background.
    runTestVariableModifier();
    // The number of calculations cycles.
    const int CYCLES = 100000;

    for (int i = 0; i < CYCLES; i++) {
        // Record a calculation.
        MultiplicationRecorder calculation(
            _mat1, _mat2, _mat1 * _mat2
        );
        _calculations.push_back(calculation);
    }
    double accuracy = calculateAccuracy();
    GTEST_ASSERT_NE(accuracy, 100.0);

    ::std::cout << "Accuracy of calculations with only atomic calculations = "
        << accuracy << "%.\n";
}

TEST_F(TestSuiteFixture, runCalculationsWithThreadLocks) {
    // Run the modifier in the background.
    runTestVariableModifier();
    // The number of calculations cycles.
    const int CYCLES = 100000;

    for (int i = 0; i < CYCLES; i++) {
        // Lock the thread.
        ::std::lock_guard<::std::mutex> lock(_mutex);
        // Record a calculation.
        MultiplicationRecorder calculation(
            _mat1, _mat2, _mat1 * _mat2
        );
        _calculations.push_back(calculation);
    }
    double accuracy = calculateAccuracy();
    GTEST_ASSERT_EQ(accuracy, 100.0);

    ::std::cout << "Accuracy of calculations with thread locks = " << accuracy<< "%.\n";
}

TEST_F(TestSuiteFixture, verifyDotProductImplementationCorrectness) {
    // To verify,
    //
    // [ 1.0  2.0  0.0  1.0 ] [ 2.0  2.0  0.0  1.0 ]   [ 5.0  6.0  3.0  6.0 ]
    // [ 0.0  1.0  1.0  0.0 ] [ 1.0  1.0  1.0  2.0 ] = [ 2.0  2.0  4.0  4.0 ]
    // [ 1.0  1.0  0.0  2.0 ] [ 1.0  1.0  3.0  2.0 ]   [ 5.0  7.0  3.0  5.0 ]
    // [ 1.0  0.0  1.0  0.0 ] [ 1.0  2.0  1.0  1.0 ]   [ 3.0  3.0  3.0  3.0 ]
    //
    AtomicMatrix4x4 mat1DotMat2 = {
        {5.0, 6.0, 3.0, 6.0},
        {2.0, 2.0, 4.0, 4.0},
        {5.0, 7.0, 3.0, 5.0},
        {3.0, 3.0, 3.0, 3.0}
    };
    GTEST_ASSERT_EQ(_mat1 * _mat2, mat1DotMat2);

    // To verify,
    //
    // [ 2.0  2.0  0.0  1.0 ] [ 1.0  2.0  0.0  1.0 ]   [ 3.0  6.0  3.0  2.0 ]
    // [ 1.0  1.0  1.0  2.0 ] [ 0.0  1.0  1.0  0.0 ] = [ 4.0  4.0  3.0  3.0 ]
    // [ 1.0  1.0  3.0  2.0 ] [ 1.0  1.0  0.0  2.0 ]   [ 6.0  6.0  3.0  7.0 ]
    // [ 1.0  2.0  1.0  1.0 ] [ 1.0  0.0  1.0  0.0 ]   [ 3.0  5.0  3.0  3.0 ]
    //
    AtomicMatrix4x4 mat2DotMat1 = {
        {3.0, 6.0, 3.0, 2.0},
        {4.0, 4.0, 3.0, 3.0},
        {6.0, 6.0, 3.0, 7.0},
        {3.0, 5.0, 3.0, 3.0}
    };
    GTEST_ASSERT_EQ(_mat2 * _mat1, mat2DotMat1);
}

TEST_F(TestSuiteFixture, verifyCopyingImplementationCorrectness) {
    AtomicMatrix4x4 matrix = _mat1;
    GTEST_ASSERT_EQ(matrix, _mat1);

    matrix = _mat2;
    GTEST_ASSERT_EQ(matrix, _mat2);
}

TEST_F(TestSuiteFixture, verifyEqualityImplementationCorrectness) {
    GTEST_ASSERT_EQ(_mat1, _mat1);
    GTEST_ASSERT_EQ(_mat2, _mat2);
    GTEST_ASSERT_NE(_mat1, _mat2);
}