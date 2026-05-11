#pragma once

#include "nse_fo_structs.hpp"

#include<endian.h>
#include<cstdint>

namespace DSE::bswap{
    inline std::int16_t bswap16(std::int16_t v ) { return htobe16(v); }
    inline std::int32_t bswap32(std::int32_t v ) { return htobe32(v); }
    inline std::int64_t bswap64(std::int64_t v ) { return htobe64(v); }
}