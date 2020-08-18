#pragma once

#include <iostream>
#include <cstring>


namespace kernel {

inline std::ostream& info()    { return std::cout; }
inline std::ostream& error()   { return std::cout; }
inline std::ostream& warning() { return std::cout; }

inline void hang() {}
inline void zero_memory(void* dst, size_t size) { std::memset(dst, 0, size); }
}
