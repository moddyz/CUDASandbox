#pragma once

#include "allocate.h"
#include "copy.h"
#include "residency.h"

#include <cuda_runtime.h>

/// \struct MemoryCompare
///
/// Template prototype for a memory comparison operation between two arrays.
template <Residency ResidencyT>
struct MemoryCompare
{
};

template <>
struct MemoryCompare<Host>
{
    template <typename ValueT>
    static inline bool Execute( size_t i_numElements, const ValueT* i_arrayA, const ValueT* i_arrayB )
    {
        ASSERT( i_arrayA != nullptr );
        ASSERT( i_arrayB != nullptr );

        for ( size_t index = 0; index < i_numElements; ++index )
        {
            if ( i_arrayA[ index ] != i_arrayB[ index ] )
            {
                return false;
            }
        }

        return true;
    }
};

template <>
struct MemoryCompare<CUDA>
{
    template <typename ValueT>
    static inline bool Execute( size_t i_numElements, const ValueT* i_arrayA, const ValueT* i_arrayB )
    {
        // MemoryAllocate host buffers.
        size_t  numBytes   = i_numElements * sizeof( ValueT );
        ValueT* hostArrayA = ( ValueT* ) MemoryAllocate<Host>::Execute( numBytes );
        if ( hostArrayA == nullptr )
        {
            return false;
        }
        ValueT* hostArrayB = ( ValueT* ) MemoryAllocate<Host>::Execute( numBytes );
        if ( hostArrayB == nullptr )
        {
            return false;
        }

        // Copy to host buffers.
        bool result = MemoryCopy<CUDA, Host>::Execute( numBytes, i_arrayA, hostArrayA );
        ASSERT( result );
        result = MemoryCopy<CUDA, Host>::Execute( numBytes, i_arrayB, hostArrayB );
        ASSERT( result );

        // Do comparison on Host.
        return MemoryCompare<Host>::Execute( i_numElements, hostArrayA, hostArrayB );
    }
};
