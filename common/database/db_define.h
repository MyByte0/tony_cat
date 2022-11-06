
#ifndef DB_DEFINE_H_
#define DB_DEFINE_H_

#include "common/core_define.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

TONY_CAT_SPACE_BEGIN


struct DbPbFieldKey {
    DbPbFieldKey() {
		mapMesssageKeys[std::string("Db.UserCount")] = { 
            std::string("count_type"),
            std::string("count_subtype")
        };
		mapMesssageKeys[std::string("Db.ClientCache")] = { 
            std::string("cache_type")
        };
    }

    std::unordered_map<std::string, std::unordered_set<std::string>> mapMesssageKeys;
};

TONY_CAT_SPACE_END

#endif // DB_DEFINE_H_
