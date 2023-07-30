#include "net_pb_module.h"

#include <ctime>

#include "common/config/xml_config_module.h"
#include "common/log/log_module.h"
#include "common/module_manager.h"
#include "common/net/net_module.h"
#include "common/service/service_government_module.h"
#include "common/utility/crc.h"

TONY_CAT_SPACE_BEGIN

NetPbModule* NetPbModule::m_pNetPbModule = nullptr;

NetPbModule::NetPbModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager) {}

NetPbModule::~NetPbModule() {}

NetPbModule* NetPbModule::GetInstance() { return NetPbModule::m_pNetPbModule; }

void NetPbModule::BeforeInit() {
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
    m_pServiceGovernmentModule =
        FIND_MODULE(m_pModuleManager, ServiceGovernmentModule);
    m_pXmlConfigModule = FIND_MODULE(m_pModuleManager, XmlConfigModule);

    m_workLoop = std::make_shared<LoopPool>();
    assert(NetPbModule::m_pNetPbModule == nullptr);
    NetPbModule::m_pNetPbModule = this;
}

void NetPbModule::OnInit() {
    auto nId = m_pServiceGovernmentModule->GetMineServerInfo().nServerConfigId;
    auto pServerListConfig =
        m_pXmlConfigModule->GetServerListConfigDataById(nId);
    if (pServerListConfig) {
        m_workLoop->Start(pServerListConfig->nNetThreadsNum);
    }
    // call by service grov now
    // Listen(m_pServiceGovernmentModule->GetServerIp(),
    //        m_pServiceGovernmentModule->GetServerPort());
}

void NetPbModule::AfterStop() { m_workLoop->Stop(); }

void NetPbModule::OnUpdate() {
    std::vector<std::function<void()>> vecGetFunction;
    {
        std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
        m_vecMsgFunction.swap(vecGetFunction);
    }

    std::for_each(vecGetFunction.begin(), vecGetFunction.end(),
                  [](const std::function<void()>& cb) { cb(); });
}

void NetPbModule::Listen(const std::string& strAddress, uint16_t addressPort) {
    AcceptorPtr pAcceper = std::make_shared<Acceptor>();
    pAcceper->Init(m_workLoop, m_pNetModule->GetDefaultListenLoop(), strAddress,
                   addressPort,
                   std::bind(&NetPbModule::ReadData, this,
                             std::placeholders::_1, std::placeholders::_2));
    m_pNetModule->Listen(pAcceper);
}

void NetPbModule::Connect(
    const std::string& strAddress, uint16_t addressPort,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/) {
    return m_pNetModule->Connect(
        strAddress, addressPort,
        std::bind(&NetPbModule::ReadData, this, std::placeholders::_1,
                  std::placeholders::_2),
        funOnSessionConnect, funOnSessionClose);
}

bool NetPbModule::ReadData(Session::session_id_t sessionId,
                           SessionBuffer& buf) {
    while (buf.GetReadableSize() >= kHeadLen) {
        auto readBuff = buf.GetReadData();
        uint32_t checkCode = 0, msgType = 0;
        size_t packetLen = 0;
        if constexpr (std::endian::native == std::endian::big) {
            checkCode = *(uint32_t*)readBuff;
            packetLen = *((uint32_t*)readBuff + 1);
            msgType = *((uint32_t*)readBuff + 2);
        } else {
            checkCode = SwapUint32(*(uint32_t*)readBuff);
            packetLen = SwapUint32(*((uint32_t*)readBuff + 1));
            msgType = SwapUint32(*((uint32_t*)readBuff + 2));
        }

        if (buf.GetReadableSize() < packetLen + kHeadLen) {
            break;
        }

        uint32_t remoteCheck =
            CheckCode(readBuff + sizeof(checkCode),
                      packetLen + kHeadLen - sizeof(checkCode));
        if (checkCode != remoteCheck) {
            LOG_ERROR(
                "check code error, remote code: {}, remote data check: {}",
                checkCode, remoteCheck);
            return false;
        }

        if (ReadPbPacket(sessionId, msgType, readBuff + kHeadLen, packetLen) ==
            false) {
            LOG_ERROR("read msg fail, msgType:{}", msgType);
            return false;
        }
        buf.RemoveData(packetLen + kHeadLen);
    }

    return true;
}

bool NetPbModule::WriteData(Session::session_id_t sessionId, uint32_t msgType,
                            const char* data, size_t length) {
    auto pSession = m_pNetModule->GetSessionInMainLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}, msgId:{}", sessionId,
                  msgType);
        return false;
    }
    if (length > 0xffffff) {
        return false;
    }

    uint8_t head[kHeadLen];
    if constexpr (std::endian::native == std::endian::big) {
        *((uint32_t*)head + 1) = (uint32_t)length;
        *((uint32_t*)head + 2) = msgType;
        *(uint32_t*)head =
            CheckCode((uint32_t*)head + 1, size_t(kHeadLen) - sizeof(uint32_t),
                      data, length);
    } else {
        *((uint32_t*)head + 1) = SwapUint32((uint32_t)length);
        *((uint32_t*)head + 2) = SwapUint32(msgType);
        *(uint32_t*)head = SwapUint32(
            CheckCode((uint32_t*)head + 1, size_t(kHeadLen) - sizeof(uint32_t),
                      data, length));
    }

    pSession->WriteAppend((const char*)head, sizeof(head), data, length);
    return m_pNetModule->SessionSend(pSession);
}

bool NetPbModule::WriteData(Session::session_id_t sessionId, uint32_t msgType,
                            const char* dataHead, size_t lengthHead,
                            const char* dataBody, size_t lengthBody) {
    auto pSession = m_pNetModule->GetSessionInMainLoop(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}, msgId:{}", sessionId,
                  msgType);
        return false;
    }
    if (lengthBody > 0xffffff) {
        return false;
    }
    if (lengthHead > kPacketHeadMaxLen) {
        return false;
    }

    uint8_t head[kHeadLen + kPacketHeadMaxLen];
    memcpy((uint32_t*)head + 3, dataHead, lengthHead);
    if constexpr (std::endian::native == std::endian::big) {
        *((uint32_t*)head + 1) = (uint32_t)(lengthHead + lengthBody);
        *((uint32_t*)head + 2) = msgType;
        *(uint32_t*)head = CheckCode((uint32_t*)head + 1,
                                     lengthHead + kHeadLen - sizeof(uint32_t),
                                     dataBody, lengthBody);
    } else {
        *((uint32_t*)head + 1) =
            SwapUint32((uint32_t)(lengthHead + lengthBody));
        *((uint32_t*)head + 2) = SwapUint32(msgType);
        *(uint32_t*)head = SwapUint32(CheckCode(
            (uint32_t*)head + 1, lengthHead + kHeadLen - sizeof(uint32_t),
            dataBody, lengthBody));
    }

    pSession->WriteAppend((const char*)head, sizeof(head) + lengthHead,
                          dataBody, lengthBody);
    return m_pNetModule->SessionSend(pSession);
}

void NetPbModule::RegisterPacketHandle(uint32_t msgType,
                                       FuncPacketHandleType func) {
    if (nullptr == func) {
        return;
    }
    m_mapPackethandle[msgType] = func;
}

void NetPbModule::SetDefaultPacketHandle(FuncPacketHandleType func) {
    m_funDefaultPacketHandle = func;
}

uint32_t NetPbModule::CheckCode(const char* data, size_t length) {
    return CRC32(data, length);
}

uint32_t NetPbModule::CheckCode(const void* dataHead, size_t lenHead,
                                const void* data, size_t len) {
    return CRC32(dataHead, lenHead, data, len);
}

bool NetPbModule::ReadPbPacket(Session::session_id_t sessionId,
                               uint32_t msgType, const char* data,
                               std::size_t length) {
    if (auto itMapPackethandle = m_mapPackethandle.find(msgType);
        itMapPackethandle != m_mapPackethandle.end()) {
        const auto& funcCb = itMapPackethandle->second;
        funcCb(sessionId, msgType, data, length);
        return true;
    }

    if (nullptr != m_funDefaultPacketHandle) {
        m_funDefaultPacketHandle(sessionId, msgType, data, length);
        return true;
    }

    return false;
}

uint32_t NetPbModule::GetPacketHeadLength(const uint32_t* pData) {
    uint32_t packetHeadLen = 0;
    if constexpr (std::endian::native == std::endian::big) {
        packetHeadLen = *pData;
    } else {
        packetHeadLen = SwapUint32(*pData);
    }
    return packetHeadLen;
}

void NetPbModule::SetPacketHeadLength(uint32_t* pData, uint32_t lenHead) {
    if constexpr (std::endian::native == std::endian::big) {
        *(uint32_t*)pData = lenHead;
    } else {
        *(uint32_t*)pData = SwapUint32(lenHead);
    }
}

uint32_t NetPbModule::SwapUint32(uint32_t value) {
    uint32_t swapValue = (value >> 24) | ((value & 0x00ff0000) >> 8) |
                         ((value & 0x0000ff00) << 8) | (value << 24);

    return swapValue;
}

void NetPbModule::FillHeadCommon(Pb::ClientHead& packetHead) {}

void NetPbModule::FillHeadCommon(Pb::ServerHead& packetHead) {
    // set sent server info
    packetHead.set_src_server_type(m_pServiceGovernmentModule->GetServerType());
    packetHead.set_src_server_index(m_pServiceGovernmentModule->GetServerId());

    // set request ttl
    if (packetHead.ttl() == 0) {
        packetHead.set_ttl(kPacketTTLDefaultvalue);
    } else {
        packetHead.set_ttl(packetHead.ttl() - 1);
    }

    // set teansId
    if (packetHead.trans_id().empty()) {
        GenTransId(*packetHead.mutable_trans_id());
    }
}

bool NetPbModule::CheckHeadTTLError(Pb::ClientHead& packetHead) { return true; }

bool NetPbModule::CheckHeadTTLError(Pb::ServerHead& packetHead) {
    bool bPass = packetHead.ttl() > 0;
    if (!bPass) {
        LOG_ERROR("check ttl error, serverType:{},ServerId:{},transId:{}",
                  packetHead.src_server_type(), packetHead.src_server_index(),
                  packetHead.trans_id());
    }
    return bPass;
}

void NetPbModule::GenTransId(std::string& strTransId) {
    strTransId.clear();
    static std::string strTransIdPrefix =
        std::to_string(m_pServiceGovernmentModule->GetServerType())
            .append(" - ")
            .append(std::to_string(m_pServiceGovernmentModule->GetServerId()))
            .append(std::to_string(time(nullptr)));
    static int64_t nSuffix = 0;
    ++nSuffix;
    strTransId.append(strTransIdPrefix).append(std::to_string(nSuffix));
}

TONY_CAT_SPACE_END
