#include "net_pb_module.h"
#include "common/module_manager.h"
#include "common/utility/crc.h"
#include "log/log_module.h"
#include "net/net_module.h"

TONY_CAT_SPACE_BEGIN

NetPbModule::NetPbModule(ModuleManager* pModuleManager)
    : ModuleBase(pModuleManager)
{
}

NetPbModule::~NetPbModule() { }

void NetPbModule::BeforeInit()
{
    m_pNetModule = FIND_MODULE(m_pModuleManager, NetModule);
}

void NetPbModule::OnUpdate()
{
    std::vector<std::function<void()>> vecGetFunction;
    {
        std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
        m_vecMsgFunction.swap(vecGetFunction);
    }

    std::for_each(vecGetFunction.begin(), vecGetFunction.end(), [](std::function<void()>& cb) { cb(); });
}

void NetPbModule::Listen(const std::string& strAddress, uint16_t addressPort)
{
    m_pNetModule->Listen(strAddress, addressPort, std::bind(&NetPbModule::ReadData, this, std::placeholders::_1, std::placeholders::_2));
}

Session::session_id_t NetPbModule::Connect(const std::string& strAddress, uint16_t addressPort,
    const Session::FunSessionConnect& funOnSessionConnect /* = nullptr*/,
    const Session::FunSessionClose& funOnSessionClose /* = nullptr*/)
{
    return m_pNetModule->Connect(strAddress, addressPort, std::bind(&NetPbModule::ReadData, this, std::placeholders::_1, std::placeholders::_2),
        funOnSessionConnect, funOnSessionClose);
}

bool NetPbModule::ReadData(Session::session_id_t sessionId, SessionBuffer& buf)
{
    size_t readStart = 0;
    while (buf.GetReadableSize() >= enm_head_len) {
        auto readBuff = buf.GetReadData();
        uint32_t checkCode = 0, msgType = 0;
        size_t packetLen = 0;
        if constexpr (std::endian::native == std::endian::big) {
            packetLen = *(uint32_t*)readBuff;
            checkCode = *((uint32_t*)readBuff + 1);
            msgType = *((uint32_t*)readBuff + 2);
        } else {
            packetLen = SwapUint32(*(uint32_t*)readBuff);
            checkCode = SwapUint32(*((uint32_t*)readBuff + 1));
            msgType = SwapUint32(*((uint32_t*)readBuff + 2));
        }

        if (buf.GetReadableSize() < packetLen + enm_head_len) {
            break;
        }

        uint32_t remoteCheck = CheckCode(readBuff + enm_head_len, packetLen);
        if (checkCode != remoteCheck) {
            LOG_ERROR("check code error, remote code: {}, remote data check: {}", checkCode, remoteCheck);
            return false;
        }

        if (ReadPbPacket(sessionId, msgType, readBuff + enm_head_len, packetLen) == false) {
            LOG_ERROR("read msg fail, msgType:{}", msgType);
            return false;
        }
        buf.RemoveData(packetLen + enm_head_len);
    }

    return true;
}

bool NetPbModule::WriteData(Session::session_id_t sessionId, uint32_t msgType, const char* data, size_t length)
{
    auto pSession = m_pNetModule->GetSessionById(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}, msgId:{}", sessionId, msgType);
        return false;
    }
    if (length > 0xffffff) {
        return false;
    }

    uint32_t checkCode = CheckCode(data, length);
    uint8_t head[enm_head_len];
    if constexpr (std::endian::native == std::endian::big) {
        *(uint32_t*)head = (uint32_t)length;
        *((uint32_t*)head + 1) = checkCode;
        *((uint32_t*)head + 2) = msgType;
    } else {
        *(uint32_t*)head = SwapUint32((uint32_t)length);
        *((uint32_t*)head + 1) = SwapUint32(checkCode);
        *((uint32_t*)head + 2) = SwapUint32(msgType);
    }

    return pSession->WriteData((const char*)head, sizeof(head), data, length);
}

bool NetPbModule::WriteData(Session::session_id_t sessionId, uint32_t msgType,
    const char* dataHead, size_t lengthHead, const char* dataBody, size_t lengthBody)
{
    auto pSession = m_pNetModule->GetSessionById(sessionId);
    if (nullptr == pSession) {
        LOG_ERROR("send data error on sessionId:{}, msgId:{}", sessionId, msgType);
        return false;
    }
    if (lengthBody > 0xffffff) {
        return false;
    }
    if (lengthHead > enm_packet_head_max_len) {
        return false;
    }

    uint8_t head[enm_head_len + enm_packet_head_max_len];
    uint32_t checkCode = CheckCode(dataHead, lengthHead, dataBody, lengthBody);
    if constexpr (std::endian::native == std::endian::big) {
        *(uint32_t*)head = (uint32_t)(lengthHead + lengthBody);
        *((uint32_t*)head + 1) = checkCode;
        *((uint32_t*)head + 2) = msgType;
    } else {
        *(uint32_t*)head = SwapUint32((uint32_t)(lengthHead + lengthBody));
        *((uint32_t*)head + 1) = SwapUint32(checkCode);
        *((uint32_t*)head + 2) = SwapUint32(msgType);
    }

    memcpy((uint32_t*)head + 3, dataHead, lengthHead);
    return pSession->WriteData((const char*)head, sizeof(head) + lengthHead, dataBody, lengthBody);
}

void NetPbModule::RegisterPacketHandle(uint32_t msgType, FuncPacketHandleType func)
{
    if (nullptr == func) {
        return;
    }
    m_mapPackethandle[msgType] = func;
}

void NetPbModule::SetDefaultPacketHandle(FuncPacketHandleType func)
{
    m_funDefaultPacketHandle = func;
}

uint32_t NetPbModule::CheckCode(const char* data, size_t length)
{
    return CRC32(data, length);
}

uint32_t NetPbModule::CheckCode(const void* dataHead, size_t lenHead, const void* data, size_t len)
{
    return CRC32(dataHead, lenHead, data, len);
}

bool NetPbModule::ReadPbPacket(Session::session_id_t sessionId, uint32_t msgType, const char* data, std::size_t length)
{
    auto itMapPackethandle = m_mapPackethandle.find(msgType);
    if (itMapPackethandle != m_mapPackethandle.end()) {
        auto& funcCb = itMapPackethandle->second;
        funcCb(sessionId, msgType, data, length);
        return true;
    }

    if (nullptr != m_funDefaultPacketHandle) {
        m_funDefaultPacketHandle(sessionId, msgType, data, length);
        return true;
    }

    return false;
}

uint32_t NetPbModule::GetPacketHeadLength(const uint32_t* pData)
{
    uint32_t packetHeadLen = 0;
    if constexpr (std::endian::native == std::endian::big) {
        packetHeadLen = *pData;
    } else {
        packetHeadLen = SwapUint32(*pData);
    }
    return packetHeadLen;
}

void NetPbModule::SetPacketHeadLength(uint32_t* pData, uint32_t lenHead)
{
    if constexpr (std::endian::native == std::endian::big) {
        *(uint32_t*)pData = lenHead;
    } else {
        *(uint32_t*)pData = SwapUint32(lenHead);
    }
}

uint32_t NetPbModule::SwapUint32(uint32_t value)
{
    uint32_t swapValue = (value >> 24)
        | ((value & 0x00ff0000) >> 8)
        | ((value & 0x0000ff00) << 8)
        | (value << 24);

    return swapValue;
}

TONY_CAT_SPACE_END
