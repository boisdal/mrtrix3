/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/10/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __algo_loop_h__
#define __algo_loop_h__

#include "apply.h"
#include "progressbar.h"
#include "stride.h"
#include "image_helpers.h"

namespace MR
{

  //! \cond skip


  namespace {

    struct set_pos {
      FORCE_INLINE set_pos (size_t axis, ssize_t index) : axis (axis), index (index) { }
      template <class ImageType> 
        FORCE_INLINE void operator() (ImageType& vox) { vox.index(axis) = index; }
      size_t axis;
      ssize_t index;
    };

    struct inc_pos {
      FORCE_INLINE inc_pos (size_t axis) : axis (axis) { }
      template <class ImageType> 
        FORCE_INLINE void operator() (ImageType& vox) { ++vox.index(axis); }
      size_t axis;
    };

  }

  //! \endcond

  /** \defgroup loop Looping functions
    @{ */


  struct LoopAlongSingleAxis {
    const size_t axis;

    template <class... ImageType>
      struct Run {
        const size_t axis;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        FORCE_INLINE Run (const size_t axis, const std::tuple<ImageType&...>& vox) : 
          axis (axis), vox (vox), size0 (std::get<0>(vox).size(axis)) { apply (set_pos (axis, 0), vox); }
        FORCE_INLINE operator bool() const { return std::get<0>(vox).index(axis) < size0; }
        FORCE_INLINE void operator++() const { apply (inc_pos (axis), vox); }
        FORCE_INLINE void operator++(int) const { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axis, std::tie (images...) }; }
  };

  struct LoopAlongSingleAxisProgress {
    const std::string text;
    const size_t axis;

    template <class... ImageType>
      struct Run {
        MR::ProgressBar progress;
        const size_t axis;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        FORCE_INLINE Run (const std::string& text, const size_t axis, const std::tuple<ImageType&...>& vox) : 
          progress (text, std::get<0>(vox).size(axis)), axis (axis), vox (vox), size0 (std::get<0>(vox).size(axis)) { apply (set_pos (axis, 0), vox); }
        FORCE_INLINE operator bool() const { return std::get<0>(vox).index(axis) < size0; }
        FORCE_INLINE void operator++() { apply (inc_pos (axis), vox); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axis, std::tie (images...) }; }
  };



  struct LoopAlongAxisRange {
    const size_t from, to;

    template <class... ImageType>
      struct Run {
        const size_t from, to;
        const std::tuple<ImageType&...> vox;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const size_t axis_from, const size_t axis_to, const std::tuple<ImageType&...>& vox) : 
          from (axis_from), to (axis_to ? axis_to : std::get<0>(vox).ndim()), vox (vox), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (size_t n = from; n < to; ++n)
              apply (set_pos (n, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          apply (set_pos (from, 0), vox);
          size_t axis = from+1;
          while (axis < to) {
            apply (inc_pos (axis), vox);
            if (std::get<0>(vox).index(axis) < std::get<0>(vox).size(axis))
              return;
            apply (set_pos (axis, 0), vox);
            ++axis;
          }
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { from, to, std::tie (images...) }; }
  };

  struct LoopAlongAxisRangeProgress : public LoopAlongAxisRange {
    const std::string text;
    LoopAlongAxisRangeProgress (const std::string& text, const size_t from, const size_t to) :
      LoopAlongAxisRange ({ from, to }), text (text) { }

    template <class... ImageType>
      struct Run : public LoopAlongAxisRange::Run<ImageType...> {
        MR::ProgressBar progress;
        FORCE_INLINE Run (const std::string& text, const size_t from, const size_t to, const std::tuple<ImageType&...>& vox) : 
          LoopAlongAxisRange::Run<ImageType...> (from, to, vox), progress (text, MR::voxel_count (std::get<0>(vox), from, to)) { }
        FORCE_INLINE void operator++() { LoopAlongAxisRange::Run<ImageType...>::operator++(); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, from, to, std::tie (images...) }; }
  };



  struct LoopAlongAxes {
    template <class... ImageType>
      FORCE_INLINE LoopAlongAxisRange::Run<ImageType...> operator() (ImageType&... images) const { return { 0, std::get<0>(std::tie(images...)).ndim(), std::tie (images...) }; }
  };

  struct LoopAlongAxesProgress {
    const std::string text;
    template <class... ImageType>
      FORCE_INLINE LoopAlongAxisRangeProgress::Run<ImageType...> operator() (ImageType&... images) const { return { text, 0, std::get<0>(std::tie(images...)).ndim(), std::tie (images...) }; }
  };



  struct LoopAlongStaticAxes {
    const std::initializer_list<size_t> axes;

    template <class... ImageType>
      struct Run {
        const std::initializer_list<size_t> axes;
        const std::tuple<ImageType&...> vox;
        const size_t from;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const std::initializer_list<size_t> axes, const std::tuple<ImageType&...>& vox) : 
          axes (axes), vox (vox), from (*axes.begin()), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (auto axis : axes)
              apply (set_pos (axis, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          apply (set_pos (from, 0), vox);
          auto axis = axes.begin()+1;
          while (axis != axes.end()) {
            apply (inc_pos (*axis), vox);
            if (std::get<0>(vox).index(*axis) < std::get<0>(vox).size(*axis))
              return;
            apply (set_pos (*axis, 0), vox);
            ++axis;
          } 
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axes, std::tie (images...) }; }
  };

  struct LoopAlongStaticAxesProgress : public LoopAlongStaticAxes {
    const std::string text;
    LoopAlongStaticAxesProgress (const std::string& text, const std::initializer_list<size_t> axes) : 
      LoopAlongStaticAxes ({ axes }), text (text) { }

    template <class... ImageType>
      struct Run : public LoopAlongStaticAxes::Run<ImageType...> {
        MR::ProgressBar progress;
        FORCE_INLINE Run (const std::string& text, const std::initializer_list<size_t> axes, const std::tuple<ImageType&...>& vox) : 
          LoopAlongStaticAxes::Run<ImageType...> (axes, vox), progress (text, MR::voxel_count (std::get<0>(vox), axes)) { }
        FORCE_INLINE void operator++() { LoopAlongStaticAxes::Run<ImageType...>::operator++(); ++progress; }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axes, std::tie (images...) }; }
  };



  struct LoopAlongDynamicAxes {
    const std::vector<size_t> axes;

    template <class... ImageType>
      struct Run {
        const std::vector<size_t> axes;
        const std::tuple<ImageType&...> vox;
        const size_t from;
        const ssize_t size0;
        bool ok;
        FORCE_INLINE Run (const std::vector<size_t>& axes, const std::tuple<ImageType&...>& vox) : 
          axes (axes), vox (vox), from (axes[0]), size0 (std::get<0>(vox).size(from)), ok (true) { 
            for (auto axis : axes)
              apply (set_pos (axis, 0), vox); 
          }
        FORCE_INLINE operator bool() const { return ok; }
        FORCE_INLINE void operator++() { 
          apply (inc_pos (from), vox); 
          if (std::get<0>(vox).index(from) < size0)
            return;

          auto axis = axes.cbegin()+1;
          while (axis != axes.cend()) {
            apply (set_pos (*(axis-1), 0), vox);
            apply (inc_pos (*axis), vox);
            if (std::get<0>(vox).index(*axis) < std::get<0>(vox).size(*axis)) 
              return;
            ++axis;
          }
          ok = false;
        }
        FORCE_INLINE void operator++(int) { operator++(); }
      };

    template <class... ImageType>
      FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { axes, std::tie (images...) }; }
  };

  struct LoopAlongDynamicAxesProgress : public LoopAlongDynamicAxes {
      const std::string text;
      LoopAlongDynamicAxesProgress (const std::string& text, const std::vector<size_t>& axes) : 
        LoopAlongDynamicAxes ({ axes }), text (text) { }

      template <class... ImageType>
        struct Run : public LoopAlongDynamicAxes::Run<ImageType...> {
          MR::ProgressBar progress;
          FORCE_INLINE Run (const std::string& text, const std::vector<size_t>& axes, const std::tuple<ImageType&...>& vox) : 
            LoopAlongDynamicAxes::Run<ImageType...> (axes, vox), progress (text, MR::voxel_count (std::get<0>(vox), axes)) { }
          FORCE_INLINE void operator++() { LoopAlongDynamicAxes::Run<ImageType...>::operator++(); ++progress; }
          FORCE_INLINE void operator++(int) { operator++(); }
        };

      template <class... ImageType>
        FORCE_INLINE Run<ImageType...> operator() (ImageType&... images) const { return { text, axes, std::tie (images...) }; }
    };


  //! a class to loop over arbitrary numbers and orders of axes of a ImageType
  /*! This class can be used to loop over any number of axes of one of more
   * ImageType, in any specified order, within the same thread of execution
   * (for multi-threaded applications, see Image::ThreadedLoop). Its use is
   * essentially identical to that of the Loop class, with the difference
   * that axes can now be iterated over in any arbitrary order. This is best
   * illustrated with the following examples.
   *
   * \section strideorderloop Looping with smallest stride first
   * The looping strategy most likely to make most efficient use of the
   * memory infrastructure is one where the innermost loop iterates over the
   * axis with the smallest absolute stride, since voxels along this axis are
   * most likely to be adjacent. This is most likely to optimise both
   * throughput to and from system RAM or disk (which are typically optimised
   * for bursts of contiguous sections of memory), and CPU cache usage.
   *
   * The LoopInOrder class is designed to facilitate this. In the following
   * example, the ImageType of interest is passed as an argument to the
   * constructor, so that its strides can be used to compute the nesting
   * order for the loops over the corresponding axes. Here, we assume that
   * \a vox is a 3D ImageType (i.e. vox.ndim() == 3) with strides [ 2 -1 3 ]:
   * \code
   * float sum = 0.0;
   * for (auto i = Image::LoopInOrder().run (vox); i; ++i)
   *   sum += vox.value();
   * \endcode
   * This is equivalent to:
   * \code
   * float sum = 0.0;
   * for (vox.index(2) = 0; vox.index(2) < vox.size(2); ++vox.index(2))
   *   for (vox.index(0) = 0; vox.index(0) < vox.size(0); ++vox.index(0))
   *     for (vox.index(1) = 0; vox.index(1) < vox.size(1); ++vox.index(1))
   *       sum += vox.value();
   * \endcode
   */
  /* \section restrictedorderloop Looping over a specific range of axes
   * It is also possible to explicitly specify the range of axes to be looped
   * over. In the following example, the program will loop over each 3D
   * volume in the ImageType in turn using the Loop class, and use the
   * LoopInOrder class to iterate over the axes of each volume to ensure
   * efficient memory bandwidth use when each volume is being processed.
   * \code
   * // define inner loop to iterate over axes 0 to 2
   * LoopInOrder inner (vox, 0, 3);
   *
   * // outer loop iterates over axes 3 and above:
   * for (auto i = Loop(3).run (vox); i; ++i) {
   *   float sum = 0.0;
   *   for (auto j = inner.run (vox); j; ++j) {
   *     sum += vox.value();
   *   print ("total = " + str (sum) + "\n");
   * }
   * \endcode
   *
   * \section arbitraryorderloop Arbitrary order loop
   * It is also possible to specify the looping order explictly, as in the
   * following example:
   * \code
   * float value = 0.0;
   * std::vector<size_t> order = { 1, 0, 2 };
   *
   * LoopInOrder loop (vox, order);
   * for (auto i = loop.run (vox); i; ++i) 
   *   value += std::exp (-vox.value());
   * \endcode
   * This will iterate over the axes in the same order as the first example
   * above, irrespective of the strides of the ImageType.
   *
   * \section multiorderloop Looping over multiple ImageType objects:
   *
   * As with the Loop class, it is possible to loop over more than one
   * ImageType of the same dimensions, by passing any additional ImageType
   * objects to be looped over to the run() member function. For example,
   * this code snippet will copy the contents of the ImageType \a src into a
   * ImageType \a dest (assumed to have the same dimensions as \a src),
   * with the looping order optimised for the \a src ImageType:
   * \code
   * LoopInOrder loop (src);
   * for (auto i = loop.run(src, dest); i; ++i) 
    *   dest.value() = src.value();
    * \endcode
    */
   /* \section progressloopinroder Displaying progress status
    * As in the Loop class, the LoopInOrder object can also display its
    * progress as it proceeds, using the appropriate constructor. In the
    * following example, the program will display its progress as it averages
    * a ImageType:
    * \code
    * float sum = 0.0;
    *
    * LoopInOrder loop (vox, "averaging");
    * for (auto i = loop.run (vox); i; ++i)
    *   sum += vox.value();
    *
    * float average = sum / float (Image::voxel_count (vox));
    * print ("average = " + str (average) + "\n");
    * \endcode
    * The output would look something like this:
    * \code
    * myprogram: [100%] averaging
    * average = 23.42
    * \endcode
    */



  FORCE_INLINE LoopAlongAxes Loop () { return { }; }
  FORCE_INLINE LoopAlongAxesProgress Loop (const std::string& progress_message) { return { progress_message }; }
  FORCE_INLINE LoopAlongSingleAxis Loop (size_t axis) { return { axis }; }
  FORCE_INLINE LoopAlongSingleAxisProgress Loop (const std::string& progress_message, size_t axis) { return { progress_message, axis }; }
  FORCE_INLINE LoopAlongAxisRange Loop (size_t axis_from, size_t axis_to) { return { axis_from, axis_to }; }
  FORCE_INLINE LoopAlongAxisRangeProgress Loop (const std::string& progress_message, size_t axis_from, size_t axis_to) { return { progress_message, axis_from, axis_to }; }
  FORCE_INLINE LoopAlongStaticAxes Loop (std::initializer_list<size_t> axes) { return { axes }; }
  FORCE_INLINE LoopAlongStaticAxesProgress Loop (const std::string& progress_message, std::initializer_list<size_t> axes) { return { progress_message, axes }; }
  FORCE_INLINE LoopAlongDynamicAxes Loop (const std::vector<size_t>& axes) { return { axes }; }
  FORCE_INLINE LoopAlongDynamicAxesProgress Loop (const std::string& progress_message, const std::vector<size_t>& axes) { return { progress_message, axes }; }

  template <class ImageType>
    FORCE_INLINE LoopAlongDynamicAxes 
    Loop (const ImageType& source, size_t axis_from = 0, size_t axis_to = std::numeric_limits<size_t>::max(), 
        typename std::enable_if<std::is_class<ImageType>::value && !std::is_same<ImageType, std::string>::value, int>::type = 0) {
      return { Stride::order (source, axis_from, axis_to) }; 
    }
  template <class ImageType>
    FORCE_INLINE LoopAlongDynamicAxesProgress 
    Loop (const std::string& progress_message, const ImageType& source, size_t axis_from = 0, size_t axis_to = std::numeric_limits<size_t>::max(),
        typename std::enable_if<std::is_class<ImageType>::value && !std::is_same<ImageType, std::string>::value, int>::type = 0) {
      return { progress_message, Stride::order (source, axis_from, axis_to) }; 
     }


  //! @}
}

#endif


