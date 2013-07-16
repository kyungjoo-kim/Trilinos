// @HEADER
// ***********************************************************************
//
//            Domi: Multidimensional Datastructures Package
//                 Copyright (2013) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact William F. Spotz (wfspotz@sandia.gov)
//
// ***********************************************************************
// @HEADER

// System includes
#include <string>
#include <stdlib.h>

// Domi includes
#include "Domi_Exceptions.hpp"
#include "Domi_Utils.hpp"

namespace Domi
{

Teuchos::Array< int >
regularizeAxisSizes(int numProcs,
                    int numDims,
                    const Teuchos::ArrayView< int > & axisSizes)
{
  // Allocate the return array, filled with the value -1
  Teuchos::Array< int > result(numDims, -1);
  // Copy the candidate array into the return array
  for (int axis = 0; axis < numDims && axis < axisSizes.size(); ++axis)
    result[axis] = axisSizes[axis];
  // Compute the block of processors accounted for, and the number of
  // unspecified axis sizes
  int block       = 1;
  int unspecified = 0;
  for (int axis = 0; axis < numDims; ++axis)
  {
    if (result[axis] <= 0)
      ++unspecified;
    else
      block *= result[axis];
  }
  // If all processor counts are specified, check the processor block
  // against the total number of processors and return
  if (unspecified == 0)
  {
    TEUCHOS_TEST_FOR_EXCEPTION(
      (block != numProcs),
      InvalidArgument,
      "Product of axis processor sizes does not "
      "equal total number of processors");
  }
  // For underspecified processor partitions, give the remainder to
  // the first unspecified axis and set all the rest to 1
  if (numProcs % block)
    throw InvalidArgument("Number of processors do not divide evenly");
  int quotient = numProcs / block;
  for (int axis = 0; axis < numDims; ++axis)
  {
    if (result[axis] <= 0)
    {
      result[axis] = quotient;
      quotient = 1;
    }
  }
  // Return the result
  return result;
}

////////////////////////////////////////////////////////////////////////

Teuchos::Array< int >
computeAxisRanks(int rank,
                 const Teuchos::ArrayView< int > & axisSizes)
{
  Teuchos::Array< int > result(axisSizes.size());
  int relRank = rank;
  int stride = 1;
  for (int axis = 0; axis < axisSizes.size()-1; ++axis)
    stride *= axisSizes[axis];
  for (int axis = axisSizes.size()-1; axis > 0; --axis)
  {
    result[axis] = relRank / stride;
    relRank      = relRank % stride;
    stride       = stride / axisSizes[axis-1];
  }
  result[0] = relRank;
  return result;
}

////////////////////////////////////////////////////////////////////////

Teuchos::Array< int >
computeAxisRanks(int rank,
                 int offset,
                 const Teuchos::ArrayView< int > & axisStrides)
{
  Teuchos::Array< int > result(axisStrides.size());
  int relRank = rank - offset;
  for (int axis = axisStrides.size()-1; axis >= 0; --axis)
  {
    result[axis] = relRank / axisStrides[axis];
    relRank      = relRank % axisStrides[axis];
  }
  return result;
}

////////////////////////////////////////////////////////////////////////

void splitStringOfIntsWithCommas(std::string data,
                                 Teuchos::Array< int > & result)
{
  result.clear();
  size_t current = 0;
  while (current < data.size())
  {
    size_t next = data.find(",", current);
    if (next == std::string::npos) next = data.size();
    result.push_back(atoi(data.substr(current, next-current).c_str()));
    current = next + 1;
  }
}

} // end namespace Domi
