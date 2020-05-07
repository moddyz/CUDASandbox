#pragma once

#include <cuda_runtime.h>
#include <stdlib.h>

/// \file cudaUtils.h
///
/// A set of useful utilities for CUDA programming.

/// \macro CUDA_CHECK_ERROR_CONTINUE
///
/// Check the error status.  On non-success, log to stderr and continue exceution.
#define CUDA_CHECK_ERROR_CONTINUE( val )                                                                               \
    CUDAUtils::CheckError< CUDAUtils::ErrorSeverity::Continue >( ( val ), #val, __FILE__, __LINE__ )

/// \macro CUDA_CHECK_ERROR_FATAL
///
/// Check the error status.  On non-success, log to stderr and exit the program.
#define CUDA_CHECK_ERROR_FATAL( val )                                                                                  \
    CUDAUtils::CheckError< CUDAUtils::ErrorSeverity::Fatal >( ( val ), #val, __FILE__, __LINE__ )

namespace CUDAUtils
{
/// \enum ErrorSeverity
///
/// Severity of CUDA error.
enum class ErrorSeverity : char
{
    Continue = 0,
    Fatal = 1
};

template < ErrorSeverity ErrorSeverityValue >
void CheckError( cudaError_t i_error, const char* i_function, const char* i_file, int i_line )
{
    if ( i_error != cudaSuccess )
    {
        fprintf( stderr,
                 "CUDA error at %s:%d code=%d(%s) \"%s\" \n",
                 i_file,
                 i_line,
                 static_cast< unsigned int >( i_error ),
                 cudaGetErrorName( i_error ),
                 i_function );

        if constexpr ( ErrorSeverityValue == ErrorSeverity::Fatal )
        {
            exit( EXIT_FAILURE );
        }
    }
}

} // namespace CUDAUtils

