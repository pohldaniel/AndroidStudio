#pragma once

#include <memory>
#include <Physics/Physics.h>

#define FIXED_STEP 0.0166666666666667f

class AndroidGamepad;
namespace Globals {
    extern std::unique_ptr<Physics> physics;
}