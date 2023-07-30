#include "log_module.h"

#include <glog/logging.h>

#include <filesystem>

TONY_CAT_SPACE_BEGIN

LogModule* LogModule::m_pLogModule = nullptr;

LogModule::LogModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

LogModule::~LogModule() {}

LogModule* LogModule::GetInstance() { return m_pLogModule; }

void LogModule::BeforeInit() {
    std::string logPath = "../log";
    std::filesystem::create_directory(logPath.c_str());

    google::InitGoogleLogging("ServerTest");

    google::SetLogDestination(google::GLOG_FATAL,
                              STR_FORMAT("{}/{}Fatal_", logPath, m_strLogName).c_str());
    google::SetLogDestination(google::GLOG_ERROR,
                              STR_FORMAT("{}/{}Error_", logPath, m_strLogName).c_str());
    google::SetLogDestination(google::GLOG_WARNING,
                              STR_FORMAT("{}/{}Warning_", logPath, m_strLogName).c_str());
    google::SetLogDestination(google::GLOG_INFO,
                              STR_FORMAT("{}/{}Info_", logPath, m_strLogName).c_str());

    google::SetLogFilenameExtension(".log");

    fLI::FLAGS_minloglevel = 0;

    m_pLogModule = this;
}

void LogModule::AfterStop() {
    LOG_INFO("log finish");
    google::FlushLogFiles(google::GLOG_INFO);
}

void LogModule::OnUpdate() {}

void LogModule::OnLoadConfig() {}

void LogModule::SetLogName(const std::string& strLogName) {
    m_strLogName = strLogName;
}

void LogModule::SetLogLevel(LOG_LEVEL_TYPE eLogLevel) {
    m_eLogLevel = eLogLevel;
    if (LOG_LEVEL_TYPE::LOG_LEVEL_TRACE == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_DEBUG == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_INFO == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_WARN == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_WARNING;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_ERROR == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_ERROR;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_FATAL == m_eLogLevel) {
        fLI::FLAGS_minloglevel = google::GLOG_FATAL;
    }
}

LogModule::LOG_LEVEL_TYPE LogModule::GetLogLevel() const {
    return m_pLogModule->m_eLogLevel;
}

TONY_CAT_SPACE_END
