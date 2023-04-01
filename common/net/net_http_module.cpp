#include "net_http_module.h"

#include <ctime>
#include <memory>
#include <utility>
#include <vector>

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/service/service_government_module.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

static const std::string s_name_value_separator = ": ";
static const std::string s_crlf = "\r\n";

NetHttpModule* NetHttpModule::m_pNetHttpModule = nullptr;

NetHttpModule::NetHttpModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

NetHttpModule::~NetHttpModule() {}

NetHttpModule* NetHttpModule::GetInstance() {
    return NetHttpModule::m_pNetHttpModule;
}

void NetHttpModule::BeforeInit() {
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pServiceGovernmentModule =
        FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_workLoop = std::make_shared<LoopPool>();
    assert(NetHttpModule::m_pNetHttpModule == nullptr);
    NetHttpModule::m_pNetHttpModule = this;
}

void NetHttpModule::OnInit() {
    auto nId = m_pServiceGovernmentModule->GetMineServerInfo().nServerConfigId;
    auto pServerListConfig =
        m_pXmlConfigModule->GetServerListConfigDataById(nId);
    if (pServerListConfig) {
        m_workLoop->Start(pServerListConfig->nHttpThreadsNum);
        std::string strIp;
        int32_t nPort;
        m_pServiceGovernmentModule->AddressToIpPort(
            pServerListConfig->strHttpIp, strIp, nPort);
        Listen(strIp, nPort);
    }
}

void NetHttpModule::AfterStop() { m_workLoop->Stop(); }

void NetHttpModule::OnUpdate() {
    std::vector<std::function<void()>> vecGetFunction;
    {
        std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
        m_vecMsgFunction.swap(vecGetFunction);
    }

    std::for_each(vecGetFunction.begin(), vecGetFunction.end(),
                  [](const std::function<void()>& cb) { cb(); });
}

void NetHttpModule::Listen(const std::string& strAddress,
                           uint16_t addressPort) {
    AcceptorPtr pAcceper = std::make_shared<Acceptor>();
    pAcceper->Init(m_workLoop, m_pNetModule->GetDefaultListenLoop(), strAddress,
                   addressPort,
                   std::bind(&NetHttpModule::ReadData, this,
                             std::placeholders::_1, std::placeholders::_2));
    m_pNetModule->Listen(pAcceper);
}

Session::session_id_t NetHttpModule::Connect(
    const std::string& strAddress, uint16_t addressPort,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/) {
    return m_pNetModule->Connect(
        strAddress, addressPort,
        std::bind(&NetHttpModule::ReadData, this, std::placeholders::_1,
                  std::placeholders::_2),
        funOnSessionConnect, funOnSessionClose);
}

bool NetHttpModule::ReadData(Session::session_id_t sessionId,
                             SessionBuffer& buf) {
    auto pSession = m_pNetModule->GetSessionInNetLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}", sessionId);
        return false;
    }

    auto pContext =
        reinterpret_cast<Http::RequestParser*>(pSession->GetSessionContext());
    if (nullptr == pContext) {
        pContext = new Http::RequestParser();
        pSession->SetSessionProtoContext(pContext, [this](auto pProtoContext) {
            m_pModuleManager->GetMainLoop().Exec([this, pProtoContext]() {
                delete reinterpret_cast<Http::RequestParser*>(pProtoContext);
            });
        });
    }
    auto readBuff = buf.GetReadData();
    auto readSize = buf.GetReadableSize();

    while (!buf.Empty()) {
        auto [result, read_len] = pContext->Parse(readBuff, readSize);

        if (result == Http::RequestParser::ResultType::bad) {
            auto reply =
                Http::Reply::BadReply(Http::Reply::HttpStatusCode::bad_request);
            WriteData(sessionId, reply);
            m_pNetModule->SessionSend(pSession);
            return false;
        }

        if (result == Http::RequestParser::ResultType::good) {
            auto&& http_request = pContext->FetchResult();
            {
                std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
                m_vecMsgFunction.emplace_back(
                    [this, sessionId,
                     http_request = std::move(http_request)]() mutable {
                        ReadHttpPacket(sessionId, http_request);
                    });
            }
            pContext->Reset();
        }

        buf.RemoveData(read_len);
    }

    return true;
}

bool NetHttpModule::WriteData(Session::session_id_t sessionId,
                              Http::Reply& respond) {
    auto pSession = m_pNetModule->GetSessionInMainLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}", sessionId);
        return false;
    }

    pSession->WriteAppend(Http::Reply::ToErrorHead(respond.statusCode));
    for (auto& [name, value] : respond.headers) {
        pSession->WriteAppend(name);
        pSession->WriteAppend(s_name_value_separator);
        pSession->WriteAppend(value);
        pSession->WriteAppend(s_crlf);
    }
    pSession->WriteAppend(s_crlf);
    pSession->WriteAppend(respond.content);
    m_pNetModule->SessionSend(pSession);
    return true;
}

void NetHttpModule::RegisterPacketHandle(std::string strHttpPath,
                                         FuncHttpHandleType func) {
    if (nullptr == func) {
        return;
    }
    m_mapPackethandle[strHttpPath] = func;
}

void NetHttpModule::SetDefaultPacketHandle(FuncHttpHandleType func) {
    m_funDefaultPacketHandle = func;
}

bool NetHttpModule::ReadHttpPacket(Session::session_id_t sessionId,
                                   Http::Request& req) {
    if (auto itMapPackethandle = m_mapPackethandle.find(req.path);
        itMapPackethandle != m_mapPackethandle.end()) {
        const auto& funcCb = itMapPackethandle->second;
        funcCb(sessionId, req);
        return true;
    }

    if (nullptr != m_funDefaultPacketHandle) {
        m_funDefaultPacketHandle(sessionId, req);
        return true;
    }

    Http::Reply replyError;
    replyError.statusCode = Http::Reply::HttpStatusCode::not_found;
    LOG_WARN("send not found to sessionID: {}", sessionId);
    WriteData(sessionId, replyError);
    m_pNetModule->CloseInMainLoop(sessionId);
    return false;
}

TONY_CAT_SPACE_END
