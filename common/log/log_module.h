#ifndef COMMON_LOG_LOG_MODULE_H_
#define COMMON_LOG_LOG_MODULE_H_

#include <iostream>
#include <string>
#include <version>

#include "common/core_define.h"
#include "common/module_base.h"

#if __cpp_lib_format
#include <format>
#define _FORMAT_NAMESPACE_ std
#define FMT_STRING(_STR_TEXT) _STR_TEXT
#else
#include <fmt/format.h>
#define _FORMAT_NAMESPACE_ fmt
#endif

#define STR_FORMAT(_STR_FMT_TEXT, ...) \
    _FORMAT_NAMESPACE_::format(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)
#define STR_FORMAT_N(_STR_FMT_TEXT, ...) \
    _FORMAT_NAMESPACE_::format_to_n(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)
#define STR_FORMAT_TO(_STR_FMT_TEXT, ...) \
    _FORMAT_NAMESPACE_::format_to(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)

#include <glog/logging.h>

TONY_CAT_SPACE_BEGIN

class LogModule : public ModuleBase {
public:
    explicit LogModule(ModuleManager* pModuleManager);
    virtual ~LogModule();

    void BeforeInit() override;

    virtual void AfterStop();

    virtual void OnUpdate();

    virtual void OnLoadConfig();

public:
    enum class LOG_LEVEL_TYPE {
        LOG_LEVEL_TRACE = 0,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_FATAL,
        LOG_LEVEL_MAX,
    };

public:
    // SetLogName before BeforeInit
    void SetLogName(const std::string& strLogName);
    void SetLogLevel(LOG_LEVEL_TYPE log_level);
    LOG_LEVEL_TYPE GetLogLevel() const;
    static LogModule* GetInstance();

protected:
    static LogModule* m_pLogModule;

    std::string m_strLogName;
    LOG_LEVEL_TYPE m_eLogLevel = LOG_LEVEL_TYPE::LOG_LEVEL_TRACE;
};

TONY_CAT_SPACE_END

#define LOG_TRACE(_STR_FMT_TEXT, ...)                                    \
    do {                                                                 \
        if (LogModule::LOG_LEVEL_TYPE::LOG_LEVEL_TRACE >=                \
            LogModule::GetInstance()->GetLogLevel()) {                   \
            LOG(INFO) << "TRACE: "                                       \
                      << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__);       \
            std::cout << "TRACE: " << __FILE__ << ":" << __LINE__ << " " \
                      << __FUNCTION__ << " "                             \
                      << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__)        \
                      << std::endl;                                      \
        }                                                                \
    } while (false)

#define LOG_DEBUG(_STR_FMT_TEXT, ...)                              \
    do {                                                           \
        if (LogModule::LOG_LEVEL_TYPE::LOG_LEVEL_DEBUG >=          \
            LogModule::GetInstance()->GetLogLevel()) {             \
            LOG(INFO) << "DEBUG: "                                 \
                      << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); \
        }                                                          \
    } while (false)

#define LOG_INFO(_STR_FMT_TEXT, ...)                           \
    do {                                                       \
        LOG(INFO) << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); \
    } while (false)

#define LOG_WARN(_STR_FMT_TEXT, ...)                              \
    do {                                                          \
        LOG(WARNING) << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); \
    } while (false)

#define LOG_ERROR(_STR_FMT_TEXT, ...)                           \
    do {                                                        \
        LOG(ERROR) << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); \
    } while (false)

#define LOG_FATAL(_STR_FMT_TEXT, ...)                           \
    do {                                                        \
        LOG(FATAL) << STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); \
    } while (false)

#endif  // COMMON_LOG_LOG_MODULE_H_
