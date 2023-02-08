// ============================================================================
// Modifications copyright 2023 nTopology Inc. All Rights Reserved.
// ============================================================================
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// ============================================================================
// This file contains adaptations of MSVC 14.24 standard library templates
// in <algorithm> with some modifications. Available for download here:
// https://learn.microsoft.com/en-us/visualstudio/releases/2019/history
//
// Their output should be consistent with the implementations in MSVC 14.24 when run
// on different platforms and compilers. Use these instead of any standard library
// implementation if you want to avoid cross-platform differences.
//

#pragma once

#include <iterator>
#include <type_traits>

namespace stdfixed {

namespace {

template <class RanIt, class Pr>
inline void med3Unchecked(RanIt first, RanIt mid, RanIt last, Pr pred)
{
  // sort median of three elements to middle
  if (pred(*mid, *first)) {
    std::iter_swap(mid, first);
  }

  if (pred(*last, *mid)) {  // swap middle and last, then test first again
    std::iter_swap(last, mid);

    if (pred(*mid, *first)) {
      std::iter_swap(mid, first);
    }
  }
}

template <class RanIt, class Pr>
inline void guessMedianUnchecked(RanIt first, RanIt mid, RanIt last, Pr pred)
{
  // sort median element to middle
  using Diff       = typename std::iterator_traits<RanIt>::difference_type;
  const Diff count = last - first;
  if (40 < count) {                         // Tukey's ninther
    const Diff step    = (count + 1) >> 3;  // +1 can't overflow because range was made inclusive in caller
    const Diff twoStep = step << 1;         // note: intentionally discards low-order bit
    med3Unchecked(first, first + step, first + twoStep, pred);
    med3Unchecked(mid - step, mid, mid + step, pred);
    med3Unchecked(last - twoStep, last - step, last, pred);
    med3Unchecked(first + step, mid, last - step, pred);
  }
  else {
    med3Unchecked(first, mid, last, pred);
  }
}

template <class InIt>
constexpr InIt nextIter(InIt first)
{  // increment iterator
  return ++first;
}

template <class BidIt>
constexpr BidIt prevIter(BidIt first)
{  // decrement iterator
  return --first;
}

template <class RanIt, class Pr>
inline std::pair<RanIt, RanIt> partitionByMedianGuessUnchecked(RanIt first, RanIt last, Pr pred)
{
  // partition [first, last)
  RanIt mid = first + ((last - first) >> 1);  // shift for codegen
  guessMedianUnchecked(first, mid, prevIter(last), pred);
  RanIt pFirst = mid;
  RanIt pLast  = nextIter(pFirst);

  while (first < pFirst && !pred(*prevIter(pFirst), *pFirst) && !pred(*pFirst, *prevIter(pFirst))) {
    --pFirst;
  }

  while (pLast < last && !pred(*pLast, *pFirst) && !pred(*pFirst, *pLast)) {
    ++pLast;
  }

  RanIt gFirst = pLast;
  RanIt gLast  = pFirst;

  for (;;) {  // partition
    for (; gFirst < last; ++gFirst) {
      if (pred(*pFirst, *gFirst)) {
        continue;
      }
      else if (pred(*gFirst, *pFirst)) {
        break;
      }
      else if (pLast != gFirst) {
        std::iter_swap(pLast, gFirst);
        ++pLast;
      }
      else {
        ++pLast;
      }
    }

    for (; first < gLast; --gLast) {
      if (pred(*prevIter(gLast), *pFirst)) {
        continue;
      }
      else if (pred(*pFirst, *prevIter(gLast))) {
        break;
      }
      else if (--pFirst != prevIter(gLast)) {
        std::iter_swap(pFirst, prevIter(gLast));
      }
    }

    if (gLast == first && gFirst == last) {
      return std::pair<RanIt, RanIt>(pFirst, pLast);
    }

    if (gLast == first) {  // no room at bottom, rotate pivot upward
      if (pLast != gFirst) {
        std::iter_swap(pFirst, pLast);
      }

      ++pLast;
      std::iter_swap(pFirst, gFirst);
      ++pFirst;
      ++gFirst;
    }
    else if (gFirst == last) {  // no room at top, rotate pivot downward
      if (--gLast != --pFirst) {
        std::iter_swap(gLast, pFirst);
      }

      std::iter_swap(pFirst, --pLast);
    }
    else {
      std::iter_swap(gFirst, --gLast);
      ++gFirst;
    }
  }
}

template <class Fx>
struct RefFn
{  // pass function object by value as a reference
  template <class... Args>
  constexpr decltype(auto) operator()(Args&&... Vals)
  {  // forward function call operator
    if constexpr (std::is_member_pointer_v<Fx>) {
      return std::invoke(fn, std::forward<Args>(Vals)...);
    }
    else {
      return fn(std::forward<Args>(Vals)...);
    }
  }

  Fx& fn;
};

template <class Fn>
constexpr bool passFunctorByValue_v = std::conjunction_v<std::bool_constant<sizeof(Fn) <= sizeof(void*)>,
                                                         std::is_trivially_copy_constructible<Fn>,
                                                         std::is_trivially_destructible<Fn>>;

template <class Fn, std::enable_if_t<passFunctorByValue_v<Fn>, int> = 0>  // TRANSITION, if constexpr
constexpr Fn passFn(Fn val)
{  // pass functor by value
  return val;
}

template <class Fn, std::enable_if_t<!passFunctorByValue_v<Fn>, int> = 0>
constexpr RefFn<Fn> passFn(Fn& val)
{  // pass functor by "reference"
  return {val};
}

template <class BidIt1, class BidIt2>
inline BidIt2 moveBackwardUnchecked(BidIt1 first, BidIt1 last, BidIt2 dest)
{
  // move [first, last) backwards to [..., dest)
  while (first != last) {
    *--dest = std::move(*--last);
  }

  return dest;
}

template <class BidIt, class Pr>
inline BidIt insertionSortUnchecked(const BidIt first, const BidIt last, Pr pred)
{
  using IterValue_t = typename std::iterator_traits<BidIt>::value_type;

  // insertion sort [first, last)
  if (first != last) {
    for (BidIt mid = first; ++mid != last;) {  // order next element
      BidIt       hole = mid;
      IterValue_t val  = std::move(*mid);

      if (pred(val, *first)) {  // found new earliest element, move to front
        moveBackwardUnchecked(first, mid, ++hole);
        *first = std::move(val);
      }
      else {  // look for insertion point after first
        for (BidIt prev = hole; pred(val, *--prev); hole = prev) {
          *hole = std::move(*prev);  // move hole down
        }

        *hole = std::move(val);  // insert element in hole
      }
    }
  }

  return last;
}

}  // namespace

/// MSVC std::nth_element equivalent.
template <class RanIt, class Pr>
inline void nthElement(RanIt first, RanIt nth, RanIt last, Pr pred)
{
  static constexpr int sISORTMAX = 32;

  // order Nth element
  auto       uFirst = first;
  const auto uNth   = nth;
  auto       uLast  = last;
  if (uNth == uLast) {
    return;  // nothing to do
  }

  while (sISORTMAX < uLast - uFirst) {  // divide and conquer, ordering partition containing Nth
    auto uMid = partitionByMedianGuessUnchecked(uFirst, uLast, passFn(pred));

    if (uMid.second <= uNth) {
      uFirst = uMid.second;
    }
    else if (uMid.first <= uNth) {
      return;  // nth is in the subrange of elements equal to the pivot; done
    }
    else {
      uLast = uMid.first;
    }
  }

  insertionSortUnchecked(uFirst, uLast, passFn(pred));  // sort any remainder
}

/// MSVC std::nth_element equivalent.
template <class RanIt>
inline void nthElement(RanIt first, RanIt nth, RanIt last)
{  // order Nth element
  nthElement(first, nth, last, std::less<> {});
}

}  // namespace stdfixed
