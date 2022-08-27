#ifndef APP_GAME_DATA_DEFINE_H_
#define APP_GAME_DATA_DEFINE_H_

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "common/core_define.h"
#include "protoc/db_data.pb.h"

TONY_CAT_SPACE_BEGIN

typedef std::string USER_ID;

class UserCount {
public:
    typedef std::shared_ptr<const Db::UserCount> ConstUserCountPtr;
    typedef std::shared_ptr<Db::UserCount> UserCountPtr;
    typedef std::map<int32_t, UserCountPtr> CountSubtypeMap;
    typedef std::map<int32_t, CountSubtypeMap> CountTypeMap;

    typedef std::unordered_map<int32_t, bool> CountSubtypeFlags;
    typedef std::unordered_map<int32_t, CountSubtypeFlags> CountTypeFlags;

    const ConstUserCountPtr GetData(int32_t count_type,
                                    int32_t count_subtype) const {
        auto itCountTypeMap = m_data.find(count_type);
        if (itCountTypeMap == m_data.end()) {
            return nullptr;
        }

        auto& countSubtypeMap = itCountTypeMap->second;
        auto itCountSubtypeMap = countSubtypeMap.find(count_subtype);
        if (itCountSubtypeMap == countSubtypeMap.end()) {
            return nullptr;
        }

        return itCountSubtypeMap->second;
    }

    UserCountPtr MutableData(int32_t count_type, int32_t count_subtype) {
        return m_data[count_type][count_subtype];
    }

    bool DelData(int32_t count_type, int32_t count_subtype) {
        auto itCountTypeMap = m_data.find(count_type);
        if (itCountTypeMap == m_data.end()) {
            return false;
        }

        auto& countSubtypeMap = itCountTypeMap->second;
        auto itCountSubtypeMap = countSubtypeMap.find(count_subtype);
        if (itCountSubtypeMap == countSubtypeMap.end()) {
            return false;
        }
        countSubtypeMap.erase(itCountSubtypeMap);
        return true;
    }

private:
    void ToModifyData(Db::KVData& saveData, Db::KVData& delData) {
        for (auto& [countType, countSubtypeflags] : m_flags) {
            for (auto& [countSubType, flag] : countSubtypeflags) {
                if (flag == true) {
                    *saveData.mutable_user_data()->add_user_counts() =
                        *m_data[countType][countSubType];
                } else {
                    auto pData = delData.mutable_user_data()->add_user_counts();
                    pData->set_count_type(countType);
                    pData->set_count_subtype(countSubType);
                }
            }
        }
        m_flags.clear();
    }

private:
    // count_type,count_subtype
    CountTypeMap m_data;
    CountTypeFlags m_flags;
};

class UserBase {
public:
    typedef std::shared_ptr<const Db::UserBase> ConstUserBasePtr;
    typedef std::shared_ptr<Db::UserBase> UserBasePtr;
    ConstUserBasePtr GetData() const { return m_data; }

    UserBasePtr& MutableData() { return m_data; }

private:
    void ToModifyData(Db::KVData& saveData, Db::KVData& delData) {
        if (m_flags == true) {
            *saveData.mutable_user_data()->mutable_user_base() = *m_data;
        }
    }

private:
    UserBasePtr m_data;
    bool m_flags;
};

struct PlayerData {
    std::vector<Db::UserCount> user_count;
    // Db::UserZoneInfo user_zoneinfo;
    Db::UserBase user_base;
};

typedef std::shared_ptr<PlayerData> PlayerDataPtr;

TONY_CAT_SPACE_END

#endif  // APP_GAME_DATA_DEFINE_H_
