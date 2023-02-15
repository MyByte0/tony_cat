#ifndef COMMON_VAR_DEFINE_H_
#define COMMON_VAR_DEFINE_H_

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#define DEFINE_MEMBER_BOOL(_NAME, _ACCESS) \
public:                                    \
    bool Get##_NAME() const                \
    {                                      \
        return m_b##_NAME;                 \
    }                                      \
                                           \
    _ACCESS:                               \
    void Set##_NAME(bool bValue)           \
    {                                      \
        m_b##_NAME = bValue;               \
    }                                      \
                                           \
private:                                   \
    bool m_b##_NAME = false;

#define DEFINE_MEMBER_BOOL_PUBLIC(_NAME) DEFINE_MEMBER_BOOL(_NAME, public)
#define DEFINE_MEMBER_BOOL_PRIVATE(_NAME) DEFINE_MEMBER_BOOL(_NAME, private)

#define DEFINE_MEMBER_INT_TYPE(_TYPE, _NAME, _ACCESS) \
public:                                               \
    _TYPE Get##_NAME() const                          \
    {                                                 \
        return m_n##_NAME;                            \
    }                                                 \
                                                      \
    _ACCESS:                                          \
    void Set##_NAME(_TYPE nValue)                     \
    {                                                 \
        m_n##_NAME = nValue;                          \
    }                                                 \
                                                      \
private:                                              \
    _TYPE m_n##_NAME = 0;

#define DEFINE_MEMBER_INT_TYPE_PUBLIC(_TYPE, _NAME) DEFINE_MEMBER_INT_TYPE(_TYPE, _NAME, public)
#define DEFINE_MEMBER_INT_TYPE_PRIVATE(_TYPE, _NAME) DEFINE_MEMBER_INT_TYPE(_TYPE, _NAME, private)

#define DEFINE_MEMBER_INT8_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(int8_t, _NAME)
#define DEFINE_MEMBER_INT8_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(int8_t, _NAME)
#define DEFINE_MEMBER_UINT8_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(uint8_t, _NAME)
#define DEFINE_MEMBER_UINT8_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(uint8_t, _NAME)
#define DEFINE_MEMBER_INT16_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(int16_t, _NAME)
#define DEFINE_MEMBER_INT16_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(int16_t, _NAME)
#define DEFINE_MEMBER_UINT16_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(uint16_t, _NAME)
#define DEFINE_MEMBER_UINT16_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(uint16_t, _NAME)
#define DEFINE_MEMBER_INT32_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(int32_t, _NAME)
#define DEFINE_MEMBER_INT32_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(int32_t, _NAME)
#define DEFINE_MEMBER_UINT32_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(uint32_t, _NAME)
#define DEFINE_MEMBER_UINT32_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(uint32_t, _NAME)
#define DEFINE_MEMBER_INT64_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(int64_t, _NAME)
#define DEFINE_MEMBER_INT64_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(int64_t, _NAME)
#define DEFINE_MEMBER_UINT64_PUBLIC(_NAME) DEFINE_MEMBER_INT_TYPE_PUBLIC(uint64_t, _NAME)
#define DEFINE_MEMBER_UINT64_PRIVATE(_NAME) DEFINE_MEMBER_INT_TYPE_PRIVATE(uint64_t, _NAME)

#define DEFINE_MEMBER_STR(_NAME, _ACCESS)        \
public:                                          \
    const std::string& Get##_NAME() const        \
    {                                            \
        return m_str##_NAME;                     \
    }                                            \
                                                 \
    _ACCESS:                                     \
    std::string& Get##_NAME()                    \
    {                                            \
        return m_str##_NAME;                     \
    }                                            \
    void Set##_NAME(const std::string& val)      \
    {                                            \
        m_str##_NAME = val;                      \
    }                                            \
    void Set##_NAME(std::string&& val)           \
    {                                            \
        m_str##_NAME = std::move(val);           \
    }                                            \
    void Set##_NAME(const std::string_view& val) \
    {                                            \
        m_str##_NAME = std::move(val);           \
    }                                            \
                                                 \
private:                                         \
    std::string m_str##_NAME;

#define DEFINE_MEMBER_STR_PUBLIC(_NAME) DEFINE_MEMBER_STR(_NAME, public)
#define DEFINE_MEMBER_STR_PRIVATE(_NAME) DEFINE_MEMBER_STR(_NAME, private)

#define DEFINE_MEMBER_VAR(_TYPE, _NAME, _ACCESS) \
public:                                          \
    const _TYPE& Get##_NAME() const              \
    {                                            \
        return m_data##_NAME;                    \
    }                                            \
                                                 \
    _ACCESS:                                     \
    _TYPE& Get##_NAME()                          \
    {                                            \
        return m_data##_NAME;                    \
    }                                            \
    void Set##_NAME(const _TYPE& val)            \
    {                                            \
        m_data##_NAME = val;                     \
    }                                            \
    void Set##_NAME(_TYPE&& val)                 \
    {                                            \
        m_data##_NAME = std::move(val);          \
    }                                            \
    _TYPE* Mutable##_NAME()                      \
    {                                            \
        return &m_data##_NAME;                   \
    }                                            \
                                                 \
private:                                         \
    _TYPE m_data##_NAME;

#define DEFINE_MEMBER_VAR_PUBLIC(_TYPE, _NAME) DEFINE_MEMBER_VAR(_TYPE, _NAME, public)
#define DEFINE_MEMBER_VAR_PRIVATE(_TYPE, _NAME) DEFINE_MEMBER_VAR(_TYPE, _NAME, private)

#define DEFINE_MEMBER_SET(_TYPE, _NAME, _ACCESS)  \
public:                                           \
    bool Exist##_NAME(const _TYPE& findKey) const \
    {                                             \
        auto itSet = m_set##_NAME.find(findKey);  \
        return itSet != m_set##_NAME.end();       \
    }                                             \
                                                  \
    _ACCESS:                                      \
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

#define DEFINE_MEMBER_SET_PUBLIC(_TYPE, _NAME) DEFINE_MEMBER_SET(_TYPE, _NAME, public)
#define DEFINE_MEMBER_SET_PRIVATE(_TYPE, _NAME) DEFINE_MEMBER_SET(_TYPE, _NAME, private)

#define DEFINE_MEMBER_UNORDERED_SET(_TYPE, _NAME, _ACCESS) \
public:                                                    \
    bool Exist##_NAME(const _TYPE& findKey) const          \
    {                                                      \
        auto itSet = m_set##_NAME.find(findKey);           \
        return itSet != m_set##_NAME.end();                \
    }                                                      \
                                                           \
    _ACCESS:                                               \
    void Set##_NAME(const _TYPE& findKey)                  \
    {                                                      \
        m_set##_NAME.insert(findKey);                      \
    }                                                      \
    void Set##_NAME(_TYPE&& findKey)                       \
    {                                                      \
        m_set##_NAME.insert(std::move(findKey));           \
    }                                                      \
    void Remove##_NAME(const _TYPE& findKey)               \
    {                                                      \
        m_set##_NAME.erase(findKey);                       \
    }                                                      \
    std::unordered_set<_TYPE>* Mutable##_NAME()            \
    {                                                      \
        return &m_set##_NAME;                              \
    }                                                      \
                                                           \
private:                                                   \
    std::unordered_set<_TYPE> m_set##_NAME;

#define DEFINE_MEMBER_UNORDERED_SET_PUBLIC(_TYPE, _NAME) \
    DEFINE_MEMBER_UNORDERED_SET(_TYPE, _NAME, public)
#define DEFINE_MEMBER_UNORDERED_SET_PRIVATE(_TYPE, _NAME) \
    DEFINE_MEMBER_UNORDERED_SET(_TYPE, _NAME, private)

#define DEFINE_MEMBER_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, _ACCESS)       \
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
                                                                      \
    _ACCESS:                                                          \
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

#define DEFINE_MEMBER_MAP_PUBLIC(_TYPE_KEY, _TYPE_VAL, _NAME) \
    DEFINE_MEMBER_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, public)
#define DEFINE_MEMBER_MAP_PRIVATE(_TYPE_KEY, _TYPE_VAL, _NAME) \
    DEFINE_MEMBER_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, private)

#define DEFINE_MEMBER_UNORDERED_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, _ACCESS) \
public:                                                                   \
    const _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey) const           \
    {                                                                     \
        auto itMap = m_map##_NAME.find(findKey);                          \
        if (itMap == m_map##_NAME.end()) {                                \
            return nullptr;                                               \
        }                                                                 \
        return &(itMap->second);                                          \
    }                                                                     \
    _TYPE_VAL* Get##_NAME(const _TYPE_KEY& findKey)                       \
    {                                                                     \
        auto itMap = m_map##_NAME.find(findKey);                          \
        if (itMap == m_map##_NAME.end()) {                                \
            return nullptr;                                               \
        }                                                                 \
        return &(itMap->second);                                          \
    }                                                                     \
                                                                          \
    _ACCESS:                                                              \
    void Set##_NAME(const _TYPE_KEY& findKey, const _TYPE_VAL& value)     \
    {                                                                     \
        m_map##_NAME[findKey] = value;                                    \
    }                                                                     \
    void Set##_NAME(const _TYPE_KEY& findKey, _TYPE_VAL&& value)          \
    {                                                                     \
        m_map##_NAME[findKey] = std::move(value);                         \
    }                                                                     \
    void Remove##_NAME(const _TYPE_KEY& findKey)                          \
    {                                                                     \
        m_map##_NAME.erase(findKey);                                      \
    }                                                                     \
    std::unordered_map<_TYPE_KEY, _TYPE_VAL>* Mutable##_NAME()            \
    {                                                                     \
        return &m_map##_NAME;                                             \
    }                                                                     \
                                                                          \
private:                                                                  \
    std::unordered_map<_TYPE_KEY, _TYPE_VAL> m_map##_NAME;

#define DEFINE_MEMBER_UNORDERED_MAP_PUBLIC(_TYPE_KEY, _TYPE_VAL, _NAME) \
    DEFINE_MEMBER_UNORDERED_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, public)
#define DEFINE_MEMBER_UNORDERED_MAP_PRIVATE(_TYPE_KEY, _TYPE_VAL, _NAME) \
    DEFINE_MEMBER_UNORDERED_MAP(_TYPE_KEY, _TYPE_VAL, _NAME, private)

#endif // COMMON_VAR_DEFINE_H_