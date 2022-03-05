#ifndef GAME_DATA_DEFINE_H_
#define GAME_DATA_DEFINE_H_

#include "common/core_define.h"

#include <memory>
#include <string>

TONY_CAT_SPACE_BEGIN

typedef std::string USER_ID;

struct PlayerData {
};

typedef std::shared_ptr<PlayerData> PlayerDataPtr;

TONY_CAT_SPACE_END

#endif // GAME_DATA_DEFINE_H_