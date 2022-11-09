#ifndef COMMON_VAR_DEFINE_H_
#define COMMON_VAR_DEFINE_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#define DEFINE_MEMBER_BOOL(_NAME) \
public:                           \
    bool Get##_NAME() const       \
    {                             \
        return m_b##_NAME;        \
    }                             \
    void Set##_NAME(bool bValue)  \
    {                             \
        m_b##_NAME = bValue;      \
    }                             \
                                  \
private:                          \
    bool m_b##_NAME = false;

#define DEFINE_MEMBER_INT_TYPE(_TYPE, _NAME) \
public:                                      \
    _TYPE Get##_NAME() const                 \
    {                                        \
        return m_n##_NAME;                   \
    }                                        \
    void Set##_NAME(_TYPE nValue)            \
    {                                        \
        m_n##_NAME = nValue;                 \
    }                                        \
                                             \
private:                                     \
    _TYPE m_n##_NAME = 0;

#define DEFINE_MEMBER_INT8(_NAME) DEFINE_MEMBER_INT_TYPE(int8_t, _NAME)
#define DEFINE_MEMBER_UINT8(_NAME) DEFINE_MEMBER_INT_TYPE(uint8_t, _NAME)
#define DEFINE_MEMBER_INT16(_NAME) DEFINE_MEMBER_INT_TYPE(int16_t, _NAME)
#define DEFINE_MEMBER_UINT16(_NAME) DEFINE_MEMBER_INT_TYPE(uint16_t, _NAME)
#define DEFINE_MEMBER_INT32(_NAME) DEFINE_MEMBER_INT_TYPE(int32_t, _NAME)
#define DEFINE_MEMBER_UINT32(_NAME) DEFINE_MEMBER_INT_TYPE(uint32_t, _NAME)
#define DEFINE_MEMBER_INT64(_NAME) DEFINE_MEMBER_INT_TYPE(int64_t, _NAME)
#define DEFINE_MEMBER_UINT64(_NAME) DEFINE_MEMBER_INT_TYPE(uint64_t, _NAME)

#define DEFINE_MEMBER_STR(_NAME)                \
public:                                         \
    const std::string& Get##_NAME() const       \
    {                                           \
        return m_str##_NAME;                    \
    }                                           \
    std::string& Get##_NAME()                   \
    {                                           \
        return m_str##_NAME;                    \
    }                                           \
    void Set##_NAME(const std::string& val)     \
    {                                           \
        m_str##_NAME = val;                     \
    }                                           \
    void Set##_NAME(std::string&& val)          \
    {                                           \
        m_str##_NAME = std::move(val);          \
    }                                           \
    void Set##_NAME(const std::string_view& val)\
    {                                           \
        m_str##_NAME = std::move(val);          \
    }                                           \
                                                \
private:                                        \
    std::string m_str##_NAME;

#define DEFINE_MEMBER_VAR(_TYPE, _NAME) \
public:                                 \
    const _TYPE& Get##_NAME() const     \
    {                                   \
        return m_data##_NAME;           \
    }                                   \
    _TYPE& Get##_NAME()                 \
    {                                   \
        return m_data##_NAME;           \
    }                                   \
    void Set##_NAME(const _TYPE& val)   \
    {                                   \
        m_data##_NAME = val;            \
    }                                   \
    void Set##_NAME(_TYPE&& val)        \
    {                                   \
        m_data##_NAME = std::move(val); \
    }                                   \
    _TYPE* Mutable##_NAME()             \
    {                                   \
        return &m_data##_NAME;          \
    }                                   \
                                        \
private:                                \
    _TYPE m_data##_NAME;

#define DEFINE_MEMBER_SET(_TYPE, _NAME)           \
public:                                           \
    bool Exist##_NAME(const _TYPE& findKey) const \
    {                                             \
        auto itSet = m_set##_NAME.find(findKey);  \
        return itSet != m_set##_NAME.end();       \
    }                                             \
    void Set##_NAME(const _TYPE& findKey)         \
    {                                             \
        m_set##_NAME.insert(findKey);             \
    }                                             \
    void Set##_NAME(_TYPE&& findKey)              \
    {                                             \
        m_set##_NAME.insert(std::move(findKey));  \
    }                                             \
    void Remove##_NAME(const _TYPE& findKey)      \
    {                                             \
        m_set##_NAME.erase(findKey);              \
    }                                             \
    std::set<_TYPE>* Mutable##_NAME()             \
    {                                             \
        return &m_set##_NAME;                     \
    }                                             \
                                                  \
private:                                          \
    std::set<_TYPE> m_set##_NAME;

#define DEFINE_MEMBER_UNORDERED_SET(_TYPE, _NAME) \
public:                                           \
    bool Exist##_NAME(const _TYPE& findKey) const \
    {                                             \
        auto itSet = m_set##_NAME.find(findKey);  \
        return itSet != m_set##_NAME.end();       \
    }                                             \
    void Set##_NAME(const _TYPE& findKey)         \
    {                                             \
        m_set##_NAME.insert(findKey);             \
    }                                             \
    void Set##_NAME(_TYPE&& findKey)              \
    {                                             \
        m_set##_NAME.insert(std::move(findKey));  \
    }                                             \
    void Remove##_NAME(const _TYPE& findKey)      \
    {                                             \
        m_set##_NAME.erase(findKey);              \
    }                                             \
    std::unordered_set<_TYPE>* Mutable##_NAME()   \
    {                                             \
        return &m_set##_NAME;                     \
    }                                             \
                                                  \
private:                                          \
    std::unordered_set<_TYPE> m_set##_NAME;

#define DEFINE_MEMBER_MAP(_TYPE_KEY, _TYPE_VAL, _NAME)                \
public:                                                               \
    const _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey) const       \
    {                                                                 \
        auto itMap = m_map##_NAME.find(findKey);                      \
        if (itMap == m_map##_NAME.end()) {                            \
            return nullptr;                                           \
        }                                                             \
        return &(itMap->second);                                      \
    }                                                                 \
    _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey)                   \
    {                                                                 \
        auto itMap = m_map##_NAME.find(findKey);                      \
        if (itMap == m_map##_NAME.end()) {                            \
            return nullptr;                                           \
        }                                                             \
        return &(itMap->second);                                      \
    }                                                                 \
    void Set##_NAME(const _TYPE_KEY& findKey, const _TYPE_VAL& value) \
    {                                                                 \
        m_map##_NAME[findKey] = value;                                \
    }                                                                 \
    void Set##_NAME(const _TYPE_KEY& findKey, _TYPE_VAL&& value)      \
    {                                                                 \
        m_map##_NAME[findKey] = std::move(value);                     \
    }                                                                 \
    void Remove##_NAME(const _TYPE_KEY& findKey)                      \
    {                                                                 \
        m_map##_NAME.erase(findKey);                                  \
    }                                                                 \
    std::map<_TYPE_KEY, _TYPE_VAL>* Mutable##_NAME()                  \
    {                                                                 \
        return &m_map##_NAME;                                         \
    }                                                                 \
                                                                      \
private:                                                              \
    std::map<_TYPE_KEY, _TYPE_VAL> m_map##_NAME;

#define DEFINE_MEMBER_UNORDERED_MAP(_TYPE_KEY, _TYPE_VAL, _NAME)      \
public:                                                               \
    const _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey) const       \
    {                                                                 \
        auto itMap = m_map##_NAME.find(findKey);                      \
        if (itMap == m_map##_NAME.end()) {                            \
            return nullptr;                                           \
        }                                                             \
        return &(itMap->second);                                      \
    }                                                                 \
    _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey)                   \
    {                                                                 \
        auto itMap = m_map##_NAME.find(findKey);                      \
        if (itMap == m_map##_NAME.end()) {                            \
            return nullptr;                                           \
        }                                                             \
        return &(itMap->second);                                      \
    }                                                                 \
    void Set##_NAME(const _TYPE_KEY& findKey, const _TYPE_VAL& value) \
    {                                                                 \
        m_map##_NAME[findKey] = value;                                \
    }                                                                 \
    void Set##_NAME(const _TYPE_KEY& findKey, _TYPE_VAL&& value)      \
    {                                                                 \
        m_map##_NAME[findKey] = std::move(value);                     \
    }                                                                 \
    void Remove##_NAME(const _TYPE_KEY& findKey)                      \
    {                                                                 \
        m_map##_NAME.erase(findKey);                                  \
    }                                                                 \
    std::unordered_map<_TYPE_KEY, _TYPE_VAL>* Mutable##_NAME()        \
    {                                                                 \
        return &m_map##_NAME;                                         \
    }                                                                 \
                                                                      \
private:                                                              \
    std::unordered_map<_TYPE_KEY, _TYPE_VAL> m_map##_NAME;

#endif // COMMON_VAR_DEFINE_H_