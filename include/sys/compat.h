#pragma once
// Portable string alias without requiring Arduino headers in public includes.
// Always use std::string in headers; convert to/from Arduino String in .cpp files.
#include <string>
using Str = std::string;
