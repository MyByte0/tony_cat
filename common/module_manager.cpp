#include "module_manager.h"

#include <algorithm>

TONY_CAT_SPACE_BEGIN

ModuleManager::ModuleManager() { }

ModuleManager::~ModuleManager()
{
    for (auto it = m_vecModules.rbegin(); it != m_vecModules.rend(); ++it) {
        auto& moduleInfo = *it;
        delete moduleInfo.pModule;
    }
}

bool ModuleManager::RegisterModule(const std::string& strModuleName, ModuleBase* pModule)
{
    auto it = std::find_if(m_vecModules.begin(), m_vecModules.end(), [&](const ModuleInfo& moduleInfo) { return moduleInfo.strModuleName == strModuleName; });

    if (it != m_vecModules.end()) {
        return false;
    }

    m_vecModules.emplace_back(ModuleInfo { strModuleName, pModule });
    return true;
}

ModuleBase* ModuleManager::FindModule(const std::string& strModuleName)
{
    auto it = std::find_if(m_vecModules.begin(), m_vecModules.end(), [&](const ModuleInfo& moduleInfo) { return moduleInfo.strModuleName == strModuleName; });

    if (it == m_vecModules.end()) {
        return nullptr;
    }

    return it->pModule;
}

void ModuleManager::Init()
{
    m_loop.InitThreadlocalLoop();

    for (auto it = m_vecModules.begin(); it != m_vecModules.end(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->BeforeInit();
    }

    for (auto it = m_vecModules.begin(); it != m_vecModules.end(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->OnInit();
    }

    for (auto it = m_vecModules.begin(); it != m_vecModules.end(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->AfterInit();
    }
}

void ModuleManager::Stop()
{
    m_onStop = true;
}

void ModuleManager::StopModules()
{
    for (auto it = m_vecModules.rbegin(); it != m_vecModules.rend(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->BeforeStop();
    }

    for (auto it = m_vecModules.rbegin(); it != m_vecModules.rend(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->OnStop();
    }

    for (auto it = m_vecModules.rbegin(); it != m_vecModules.rend(); ++it) {
        auto& moduleInfo = *it;
        moduleInfo.pModule->AfterStop();
    }
}

void ModuleManager::LoadConfig()
{
    for (auto& moduleInfo : m_vecModules) {
        moduleInfo.pModule->OnLoadConfig();
    }

    for (auto& moduleInfo : m_vecModules) {
        moduleInfo.pModule->AfterLoadConfig();
    }
}

void ModuleManager::UpdateModules()
{
    if (true == m_onStop) [[unlikely]] {
        StopModules();
        Loop::Cancel(m_updateTimerHandler);
        m_updateTimerHandler = nullptr;
        m_loop.Stop();
        m_onStop = false;
        return;
    }

    for (auto& moduleInfo : m_vecModules) {
        moduleInfo.pModule->OnUpdate();
    }
}

void ModuleManager::Run()
{
    assert(m_updateTimerHandler == nullptr);
    m_updateTimerHandler = m_loop.ExecEvery(m_nframe, [this]() { UpdateModules(); });

    m_loop.RunInThread();
}

Loop& ModuleManager::GetMainLoop()
{
    return m_loop;
}

bool ModuleManager::OnMainLoop()
{
    return &ModuleManager::GetMainLoop() == &Loop::GetCurrentThreadLoop();
}

TONY_CAT_SPACE_END
