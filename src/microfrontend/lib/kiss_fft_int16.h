/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_LITE_EXPERIMENTAL_MICROFRONTEND_LIB_KISS_FFT_INT16_H_
#define TENSORFLOW_LITE_EXPERIMENTAL_MICROFRONTEND_LIB_KISS_FFT_INT16_H_

#include "kiss_fft_common.h"

// Wrap 16-bit kiss fft in its own namespace. Enables us to link an application
// with different kiss fft resolutions (16/32 bit integer, float, double)
// without getting a linker error.
//
// Esp32Agent local change: the esp32 Arduino core's prebuilt
// libespressif__esp-tflite-micro.a already contains a FIXED_POINT=16 kissfft
// (TFLM's signal library) in namespace kiss_fft_fixed16, compiled against the
// kissfft headers the core ships. We declare the same namespace here and alias
// upstream's name to it, so the frontend links against that copy instead of a
// second vendored kiss_fft.c that could drift from the core's headers.
#define FIXED_POINT 16
namespace kiss_fft_fixed16 {
#include "kiss_fft.h"
#include "tools/kiss_fftr.h"
}  // namespace kiss_fft_fixed16
namespace kissfft_fixed16 = kiss_fft_fixed16;
#undef FIXED_POINT
#undef kiss_fft_scalar
#undef KISS_FFT_H

#endif  // TENSORFLOW_LITE_EXPERIMENTAL_MICROFRONTEND_LIB_KISS_FFT_INT16_H_
