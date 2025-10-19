#pragma once

#include <string>

namespace anisthesia {
struct Media;
}

namespace anisthesia::win::detail {

bool GetMediaFromSMTC(Media& media);

}  // namespace anisthesia::win::detail
