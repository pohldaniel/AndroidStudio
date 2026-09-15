#pragma once

#include <memory>
#include <Physics/Physics.h>

#define FIXED_STEP 0.0166666666666667f
namespace Globals {
    extern std::unique_ptr<Physics> physics;
}