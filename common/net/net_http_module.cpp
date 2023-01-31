#include "net_http_module.h"

#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/service/service_government_module.h"
#include "common/utility/crc.h"

#include <ctime>

TONY_CAT_SPACE_BEGIN

static const std::string s_name_value_separator = ": ";
static const std::string s_crlf = "\r\n";

NetHttpModule* NetHttpModule::m_pNetHttpModule = nullptr;

NetHttpModule::NetHttpModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

NetHttpModule::~NetHttpModule() { }

NetHttpModule* NetHttpModule::GetInstance()
{
    return NetHttpModule::m_pNetHttpModule;
}

void NetHttpModule::BeforeInit()
{
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pServiceGovernmentModule = FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);

    assert(NetHttpModule::m_pNetHttpModule == nullptr);
    NetHttpModule::m_pNetHttpModule = this;
}

void NetHttpModule::OnUpdate()
{
    std::vector<std::function<void()>> vecGetFunction;
    {
        std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
        m_vecMsgFunction.swap(vecGetFunction);
    }

    std::for_each(vecGetFunction.begin(), vecGetFunction.end(), [](const std::function<void()>& cb) { cb(); });
}

void NetHttpModule::Listen(const std::string& strAddress, uint16_t addressPort)
{
    m_pNetModule->Listen(strAddress, addressPort,
        std::bind(&NetHttpModule::ReadData, this, std::placeholders::_1, std::placeholders::_2));
}

Session::session_id_t NetHttpModule::Connect(const std::string& strAddress, uint16_t addressPort,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/)
{
    return m_pNetModule->Connect(strAddress, addressPort,
        std::bind(&NetHttpModule::ReadData, this, std::placeholders::_1, std::placeholders::_2),
        funOnSessionConnect, funOnSessionClose);
}

bool NetHttpModule::ReadData(Session::session_id_t sessionId, SessionBuffer& buf)
{
    auto pSession = m_pNetModule->GetSessionById(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}", sessionId);
        return false;
    }

    auto pContext = reinterpret_cast<Http::RequestParser*>(pSession->GetProtoContext());
    if (nullptr == pContext) {
        pContext = new Http::RequestParser();
        pSession->SetSessionProtoContext(pContext,
            [](auto pProtoContext) { delete reinterpret_cast<Http::RequestParser*>(pProtoContext); });
    }
    auto readBuff = buf.GetReadData();
    auto readSize = buf.GetReadableSize();

    while (!buf.Empty()) {
        auto [result, read_len] = pContext->Parse(
            readBuff, readSize);

        if (result == Http::RequestParser::bad) {
            auto reply = Http::Reply::BadReply(Http::Reply::bad_request);
            WriteData(sessionId, reply);
            return false;
        }

        if (result == Http::RequestParser::good) {
            auto&& result = pContext->FetchResult();
            {
                std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
                m_vecMsgFunction.emplace_back(
                    [this, sessionId, result = std::move(result)]() mutable{ ReadPbPacket(sessionId, result); });
            }
            buf.RemoveData(read_len);
            pContext->Reset();
        }

        buf.RemoveData(read_len);
    };

    return true;
}

bool NetHttpModule::WriteData(Session::session_id_t sessionId, Http::Reply& respond)
{
    auto pSession = m_pNetModule->GetSessionById(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}", sessionId);
        return false;
    }

    pSession->WriteAppend(Http::Reply::ToErrorBody(respond.status));
    for (std::size_t i = 0; i < respond.headers.size(); ++i) {
        Http::Header& header = respond.headers[i];
        pSession->WriteAppend(header.name);
        pSession->WriteAppend(s_name_value_separator);
        pSession->WriteAppend(header.value);
        pSession->WriteAppend(s_crlf);
    }
    pSession->WriteAppend(s_crlf);
    pSession->WriteAppend(respond.content);
    pSession->DoWrite();
    return true;
}

void NetHttpModule::RegisterPacketHandle(std::string strHttpPath, FuncHttpHandleType func)
{
    if (nullptr == func) {
        return;
    }
    m_mapPackethandle[strHttpPath] = func;
}

void NetHttpModule::SetDefaultPacketHandle(FuncHttpHandleType func)
{
    m_funDefaultPacketHandle = func;
}

bool NetHttpModule::ReadPbPacket(Session::session_id_t sessionId, Http::Request& req)
{
    if (auto itMapPackethandle = m_mapPackethandle.find(req.uri); itMapPackethandle != m_mapPackethandle.end()) {
        auto& funcCb = itMapPackethandle->second;
        funcCb(sessionId, req);
        return true;
    }

    if (nullptr != m_funDefaultPacketHandle) {
        m_funDefaultPacketHandle(sessionId, req);
        return true;
    }

    return false;
}

TONY_CAT_SPACE_END
