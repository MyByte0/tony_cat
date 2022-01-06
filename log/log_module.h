#ifndef COMMON_LOG_MODULE_H_
#define COMMON_LOG_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include <string>
#include <version>

#if __cpp_lib_format
#include <format>
#define _FORMAT_NAMESPACE_	std
#define FMT_STRING(_STR_TEXT)	_STR_TEXT
#else
#include <fmt/format.h>
#define _FORMAT_NAMESPACE_	fmt
#endif

#define STR_FORMAT(_STR_FMT_TEXT, ...)		_FORMAT_NAMESPACE_::format(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)
#define STR_FORMAT_N(_STR_FMT_TEXT,...)		_FORMAT_NAMESPACE_::format_to_n(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)
#define STR_FORMAT_TO(_STR_FMT_TEXT,...)	_FORMAT_NAMESPACE_::format_to(FMT_STRING(_STR_FMT_TEXT), ##__VA_ARGS__)

#include <glog/logging.h>


SER_NAME_SPACE_BEGIN

class LogModule:public ModuleBase {
public:
	LogModule(ModuleManager* pModuleManager);
	virtual ~LogModule();

	virtual void BeforeInit();

	virtual void AfterStop();

	virtual void OnUpdate();

	virtual void OnConfigChange();

public:
	enum class LOG_LEVEL_TYPE {
		LOG_LEVEL_DEBUG = 0,
		LOG_LEVEL_INFO,
		LOG_LEVEL_WARN,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_FATAL,
		LOG_LEVEL_MAX,
	};

public:
	void SetLogLevel(LOG_LEVEL_TYPE log_level);
	LOG_LEVEL_TYPE GetLogLevel() const;
	static LogModule* GetLogModuleInstance();

protected:
	static LogModule*				m_pLogModule;

	LOG_LEVEL_TYPE					m_log_level = LOG_LEVEL_TYPE::LOG_LEVEL_INFO;
};

SER_NAME_SPACE_END

#define LOG_DEBUG(_STR_FMT_TEXT, ...)  		do { DLOG_IF(INFO, LogModule::LOG_LEVEL_TYPE::LOG_LEVEL_DEBUG == LogModule::GetLogModuleInstance()->GetLogLevel()) \
												<< STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); } while(false)
#define LOG_INFO(_STR_FMT_TEXT, ...)  		do { LOG(INFO) 		<< STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); } while(false)
#define LOG_WARN(_STR_FMT_TEXT, ...)  		do { LOG(WARNING) 	<< STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); } while(false)
#define LOG_ERROR(_STR_FMT_TEXT, ...)  		do { LOG(ERROR) 	<< STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); } while(false)
#define LOG_FATAL(_STR_FMT_TEXT, ...)  		do { LOG(FATAL) 	<< STR_FORMAT(_STR_FMT_TEXT, ##__VA_ARGS__); } while(false)

#endif  // COMMON_LOG_MODULE_H_
