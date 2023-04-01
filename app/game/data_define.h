#ifndef GAME_DATA_DEFINE_H_
#define GAME_DATA_DEFINE_H_

#include <memory>
#include <string>
#include <vector>

#include "common/core_define.h"
#include "protoc/db_data.pb.h"

TONY_CAT_SPACE_BEGIN

typedef std::string USER_ID;

struct PlayerData {
    std::vector<Db::UserCount> user_count;
    // Db::UserZoneInfo user_zoneinfo;
    Db::UserBase user_base;
};

typedef std::shared_ptr<PlayerData> PlayerDataPtr;

TONY_CAT_SPACE_END

#endif  // GAME_DATA_DEFINE_H_