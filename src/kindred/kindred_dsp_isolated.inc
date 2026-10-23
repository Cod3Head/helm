/* Clear But Complex // Kindred - DSP compilation unit, part 2.
 *
 * Included once, at the end of src/synthesis/helm_voice_handler.cpp. These two Plaits files
 * cannot share a compilation unit with the rest of Plaits (kindred_dsp.inc): six_op_engine.cc
 * imports the fm namespace (whose Voice collides with plaits::Voice) and string_machine_engine.cc
 * redefines the chord engine's registration table. Mutable Instruments, MIT License.
 */

#include <cstdio>

// MSVC (Windows builds): the Mutable Instruments code is written for GCC.
#ifdef _MSC_VER
  #pragma push_macro("__attribute__")
  #define __attribute__(x)
  #ifndef M_PI
    #define M_PI 3.14159265358979323846
  #endif
  #pragma warning(push)
  #pragma warning(disable: 4244 4267 4305 4100 4189 4456 4458 4701)
#endif

#pragma push_macro("TEST")
#undef TEST
#define TEST
#include "plaits/dsp/engine2/six_op_engine.cc"
#include "plaits/dsp/engine2/string_machine_engine.cc"
#pragma pop_macro("TEST")
#ifdef _MSC_VER
  #pragma warning(pop)
  #pragma pop_macro("__attribute__")
#endif

