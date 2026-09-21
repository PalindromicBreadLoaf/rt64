#pragma once

#include "plume_render_interface.h"

using namespace plume;

namespace RT64 {
#ifdef __SWITCH__
    static constexpr RenderFormat SwapChainFormat = RenderFormat::R8G8B8A8_UNORM;
#else
    static constexpr RenderFormat SwapChainFormat = RenderFormat::B8G8R8A8_UNORM;
#endif
};
