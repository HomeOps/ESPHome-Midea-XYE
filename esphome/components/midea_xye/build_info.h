#pragma once

#ifdef USE_ARDUINO

#ifndef MIDEA_XYE_BUILD_GIT_COMMIT
#define MIDEA_XYE_BUILD_GIT_COMMIT "unknown"
#endif

#ifndef MIDEA_XYE_BUILD_GIT_BRANCH
#define MIDEA_XYE_BUILD_GIT_BRANCH "unknown"
#endif

#ifndef MIDEA_XYE_BUILD_GIT_REMOTE
#define MIDEA_XYE_BUILD_GIT_REMOTE "unknown"
#endif

#ifndef MIDEA_XYE_BUILD_GIT_DIRTY
#define MIDEA_XYE_BUILD_GIT_DIRTY 0
#endif

namespace esphome {
namespace midea {
namespace xye {
namespace build_info {

static constexpr const char *GIT_COMMIT = MIDEA_XYE_BUILD_GIT_COMMIT;
static constexpr const char *GIT_BRANCH = MIDEA_XYE_BUILD_GIT_BRANCH;
static constexpr const char *GIT_REMOTE = MIDEA_XYE_BUILD_GIT_REMOTE;
static constexpr bool GIT_DIRTY = MIDEA_XYE_BUILD_GIT_DIRTY != 0;

}  // namespace build_info
}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
