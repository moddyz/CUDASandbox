// CUDA.
#include <cuda_runtime.h>

// Relative.
#include "matrixArrayProduct.h"
#include "valueTypes.h"

// Utils.
#include <cudaUtils.h>

// Thirdparty.
#include <cxxopts.hpp>

/// Helper function for setting a value \p i_matrix for \p i_arraySize elements in \p o_matrices.
void SetMatrixArrayValue( const Mat4f& i_matrix, int i_arraySize, Mat4f* o_matrices )
{
    for ( int matrixIndex = 0; matrixIndex < i_arraySize; ++matrixIndex )
    {
        o_matrices[ matrixIndex ] = i_matrix;
    }
}

/// Helper function for checking that all the values in the two matrices arrays are equal.
void CheckMatrixArrays( const Mat4f* i_matrixA, const Mat4f* i_matrixB, int i_arraySize )
{
    for ( int matrixIndex = 0; matrixIndex < i_arraySize; ++matrixIndex )
    {
        if ( i_matrixA[ matrixIndex ] != i_matrixB[ matrixIndex ] )
        {
            fprintf( stderr,
                     "MatrixA[ %d ] != MatrixB[ %d ],\n%s != %s\n",
                     matrixIndex,
                     matrixIndex,
                     i_matrixA[ matrixIndex ].GetString().c_str(),
                     i_matrixB[ matrixIndex ].GetString().c_str() );
            return;
        }
    }
}

int main( int i_argc, char** i_argv )
{
    // Parse command line arguments.
    cxxopts::Options options( "cudaMatrixArrayOps",
                              "Testing various implementations of 4 by 4 matrix array operations." );
    options.add_options()( "n,arraySize", "Size of matrix array.", cxxopts::value< int >()->default_value( "10000" ) );
    auto result = options.parse( i_argc, i_argv );
    int arraySize = result[ "arraySize" ].as< int >();

    // Compute amount of memory to allocate.
    size_t numBytes = arraySize * sizeof( Mat4f );

    // Allocate host memory.
    Mat4f* matricesA = ( Mat4f* ) malloc( numBytes );
    Mat4f* matricesB = ( Mat4f* ) malloc( numBytes );
    Mat4f* matricesC = ( Mat4f* ) malloc( numBytes );
    Mat4f* matricesRef = ( Mat4f* ) malloc( numBytes );

    // Set host values.
    SetMatrixArrayValue( Mat4f::Identity(), arraySize, matricesA );
    SetMatrixArrayValue( Mat4f::Identity(), arraySize, matricesB );

    // Compute CPU output.
    MatrixArrayProduct_CPU( matricesA, matricesB, arraySize, matricesRef );

    // Allocate device memory.
    Mat4f* matricesADevice;
    Mat4f* matricesBDevice;
    Mat4f* matricesCDevice;
    CUDA_CHECK_ERROR_FATAL( cudaMalloc( ( void** ) &matricesADevice, numBytes ) );
    CUDA_CHECK_ERROR_FATAL( cudaMalloc( ( void** ) &matricesBDevice, numBytes ) );
    CUDA_CHECK_ERROR_FATAL( cudaMalloc( ( void** ) &matricesCDevice, numBytes ) );

    // Upload host memory -> device.
    CUDA_CHECK_ERROR_FATAL(
        cudaMemcpy( /*dst*/ matricesADevice, /*src*/ matricesA, numBytes, cudaMemcpyHostToDevice ) );
    CUDA_CHECK_ERROR_FATAL(
        cudaMemcpy( /*dst*/ matricesBDevice, /*src*/ matricesB, numBytes, cudaMemcpyHostToDevice ) );

    // Compute grid & block size based on array size.
    dim3 blockSize, gridSize;
    blockSize.x = 256;
    gridSize.x = ( arraySize + blockSize.x - 1 ) / blockSize.x;

    // Create events for timings.
    cudaEvent_t start, stop;
    CUDA_CHECK_ERROR_FATAL( cudaEventCreate( &start ) );
    CUDA_CHECK_ERROR_FATAL( cudaEventCreate( &stop ) );

    // Start timer.
    CUDA_CHECK_ERROR_FATAL( cudaEventRecord( start, 0 ) );

    // Execute kernel.
    void* args[] = {&matricesADevice, &matricesBDevice, &arraySize, &matricesCDevice};
    CUDA_CHECK_ERROR_FATAL(
        cudaLaunchKernel( ( void* ) MatrixArrayProduct_Naive, gridSize, blockSize, args, 0, nullptr ) );

    // Stop timer, and get elapsed time in milliseconds.
    CUDA_CHECK_ERROR_FATAL( cudaEventRecord( stop, 0 ) );
    CUDA_CHECK_ERROR_FATAL( cudaEventSynchronize( stop ) );
    float elapsedMs;
    CUDA_CHECK_ERROR_FATAL( cudaEventElapsedTime( &elapsedMs, start, stop ) );

    // Print results.
    printf( "MatrixArrayProduct_Naive:\n" );
    printf( "   Elapsed time:                   %f ms\n", elapsedMs );
    printf( "   Theoretical Bandwidth:          %f GB/s\n", CudaComputeTheoreticalMemoryBandwidth() );
    printf( "   Effective Bandwidth:            %f GB/s\n",
            CudaComputeEffectiveMemoryBandwidth( numBytes * 2, numBytes, elapsedMs ) );

    // Download computed matrices.
    CUDA_CHECK_ERROR_FATAL(
        cudaMemcpy( /*dst*/ matricesC, /*src*/ matricesCDevice, numBytes, cudaMemcpyDeviceToHost ) );

    CheckMatrixArrays( matricesC, matricesRef, arraySize );
}
