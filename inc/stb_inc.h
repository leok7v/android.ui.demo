#pragma once
#include "rt.h"
// see: https://github.com/nothings/stb
#define STBI_ASSERT(x) assert(x)
#define STBI_MALLOC    allocate
#define STBI_REALLOC   reallocate
#define STBI_FREE      deallocate

