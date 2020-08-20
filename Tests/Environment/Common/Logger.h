#pragma once

#include <iostream>

namespace kernel {

inline std::ostream& info()    { return std::cout; }
inline std::ostream& error()   { return std::cout; }
inline std::ostream& warning() { return std::cout; }
}
