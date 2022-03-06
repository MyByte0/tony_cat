#include "log_module.h"

#include <filesystem>

#include <glog/logging.h>

TONY_CAT_SPACE_BEGIN

LogModule* LogModule::m_pLogModule = nullptr;

LogModule::LogModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

LogModule::~LogModule()
{
}

LogModule* LogModule::GetInstance()
{
    return m_pLogModule;
}

void LogModule::BeforeInit()
{
    std::string logPath = "../log";
    std::filesystem::create_directory(logPath.c_str());

    google::InitGoogleLogging("ServerTest");

    google::SetLogDestination(google::GLOG_FATAL, STR_FORMAT("{}/log_fatal_", logPath).c_str());
    google::SetLogDestination(google::GLOG_ERROR, STR_FORMAT("{}/log_error_", logPath).c_str());
    google::SetLogDestination(google::GLOG_WARNING, STR_FORMAT("{}/log_warning_", logPath).c_str());
    google::SetLogDestination(google::GLOG_INFO, STR_FORMAT("{}/log_info_", logPath).c_str());

    google::SetLogFilenameExtension(".log");

    fLI::FLAGS_minloglevel = 0;

    m_pLogModule = this;
}

void LogModule::AfterStop()
{
    LOG_INFO("log finish");
    google::FlushLogFiles(google::GLOG_INFO);
}

void LogModule::OnUpdate() { }

void LogModule::OnLoadConfig() { }

void LogModule::SetLogLevel(LOG_LEVEL_TYPE log_level)
{
    m_log_level = log_level;
    if (LOG_LEVEL_TYPE::LOG_LEVEL_TRACE == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_DEBUG == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_INFO == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_INFO;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_WARN == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_WARNING;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_ERROR == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_ERROR;
    } else if (LOG_LEVEL_TYPE::LOG_LEVEL_FATAL == m_log_level) {
        fLI::FLAGS_minloglevel = google::GLOG_FATAL;
    }
}

LogModule::LOG_LEVEL_TYPE LogModule::GetLogLevel() const
{
    return m_pLogModule->m_log_level;
}

TONY_CAT_SPACE_END
