#ifndef COMMON_NET_NET_PB_MODULE_H_
#define COMMON_NET_NET_PB_MODULE_H_

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/loop/loop_coroutine.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_session.h"
#include "common/utility/spin_lock.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class ModuleManager;
class NetModule;
class ServiceGovernmentModule;
class XmlConfigModule;

// NetPacket:
// || 4 bytes packetLen(PbPacket Len) || 4 bytes checkCode(for PbPacket) || 4 bytes msgType || PbPacket ||
// the PbPacket in NetPacket:
// || 4 bytes PbPacketHead Len || PbPacketHead || PbPacketBody ||

class NetPbModule : public ModuleBase {
public:
    explicit NetPbModule(ModuleManager* pModuleManager);
    virtual ~NetPbModule();

    virtual void BeforeInit() override;
    virtual void OnInit() override;
    virtual void AfterStop() override;
    virtual void OnUpdate() override;

public:
    enum : int {
        kHeadLen = 12,
        kPacketHeadMaxLen = 16 * 1024,
        kPacketBodyMaxCache = 64 * 1024,
        kPacketTTLDefaultvalue = 30,
    };

    void Listen(const std::string& strAddress, uint16_t addressPort);
    Session::session_id_t Connect(const std::string& strAddress, uint16_t addressPort,
        const Session::FunSessionConnect& funOnSessionConnect = nullptr,
        const Session::FunSessionClose& funOnSessionClose = nullptr);
    bool ReadData(Session::session_id_t sessionId, SessionBuffer& buf);
    bool WriteData(Session::session_id_t sessionId, uint32_t msgType, const char* data, size_t length);
    bool WriteData(Session::session_id_t sessionId, uint32_t msgType, const char* dataHead,
        size_t lengthHead, const char* dataBody, size_t lengthBody);
    bool ReadPbPacket(Session::session_id_t sessionId, uint32_t msgType, const char* data, std::size_t length);

    typedef std::function<void(Session::session_id_t sessionId, uint32_t msgType, const char* data, std::size_t length)> FuncPacketHandleType;

    void RegisterPacketHandle(uint32_t msgType, FuncPacketHandleType func);
    void SetDefaultPacketHandle(FuncPacketHandleType func);

    template <typename _Fn>
    struct GetFunctionMessage;

    template <typename _TypeRet, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet(Session::session_id_t, _TypeHead&, _TypeMessage&)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn, typename _TypeRet, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet (_Fn::*)(Session::session_id_t, _TypeHead&, _TypeMessage&)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn, typename _TypeRet, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet (_Fn::*)(Session::session_id_t, _TypeHead&, _TypeMessage&) const> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn>
    struct GetFunctionMessage_t {
        using TypeHead = typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeHead;
        using TypeBody = typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeBody;
        using TypeRet = typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeRet;
    };

    template <class _TypeMsgPacketHead>
    bool PaserPbHead(const char* data, std::size_t length, _TypeMsgPacketHead& pbHead, const char*& nextData, std::size_t& nextLen)
    {
        uint32_t pbPacketHeadLen = 0;
        if (length < sizeof pbPacketHeadLen) [[unlikely]] {
            return false;
        }
        // read PacketHeadLength
        pbPacketHeadLen = GetPacketHeadLength(reinterpret_cast<const uint32_t*>(data));
        if (length < pbPacketHeadLen + sizeof pbPacketHeadLen) [[unlikely]] {
            return false;
        }
        assert(length < UINT32_MAX);
        // compelete PacketBodyLen
        uint32_t pbPacketBodyLen = uint32_t(length) - pbPacketHeadLen - sizeof pbPacketHeadLen;
        const char* pData = data + sizeof pbPacketHeadLen;
        if (false == pbHead.ParseFromArray(pData, pbPacketHeadLen)) [[unlikely]] {
            return false;
        }
        pData += pbPacketHeadLen;

        nextData = pData;
        nextLen = pbPacketBodyLen;
        return true;
    }

private:
    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeRet>
    FuncPacketHandleType GeneratePaserPacketFunc(
        const std::function<_TypeRet(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>& funReadPacket)
    {
        return FuncPacketHandleType([this, funReadPacket](Session::session_id_t sessionId, uint32_t msgType, const char* data, std::size_t length) {
            const char* pData = nullptr;
            std::size_t pbPacketBodyLen = 0;
            // paser packet
            _TypeMsgPacketHead msgPacketHead;
            if (false == PaserPbHead(data, length, msgPacketHead, pData, pbPacketBodyLen)) [[unlikely]] {
                LOG_ERROR("recv pb packet head error, sessionId:{} type:{}", sessionId, msgType);
                return false;
            }
            if (!CheckHeadTTLError(msgPacketHead)) {
                LOG_ERROR("recv pb packet ttl, sessionId:{} msgId:{}", sessionId, msgType);
                return false;
            }

            _TypeMsgPacketBody msgPacketBody;
            if (false == msgPacketBody.ParseFromArray(pData, int(pbPacketBodyLen))) [[unlikely]] {
                LOG_ERROR("recv pb packet body error, sessionId:{} type:{}", sessionId, msgType);
                return false;
            }

            // for module update() call eventFunction
            std::function<void()> funNetEvent =
                [sessionId, msgPacketHead(std::move(msgPacketHead)), msgPacketBody(std::move(msgPacketBody)), funReadPacket(std::move(funReadPacket))]() mutable {
                    funReadPacket(sessionId, msgPacketHead, msgPacketBody);
                };
            {
                std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
                m_vecMsgFunction.emplace_back(std::move(funNetEvent));
            }

            return true;
        });
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeRet>
    void RegisterHandleHelper(uint32_t msgType,
        const std::function<_TypeRet(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>& func)
    {
        RegisterPacketHandle(msgType, GeneratePaserPacketFunc<_TypeMsgPacketHead, _TypeMsgPacketBody, _TypeRet>(func));
    }

public:
    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    bool SendPacket(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)
    {
        FillHeadCommon(packetHead);
        uint32_t msgType = _TypeMsgPacketBody::msgid;
        char buffHead[sizeof(uint32_t) + kPacketHeadMaxLen + kPacketBodyMaxCache];
        char* pData = buffHead;

        // Serialize packetHead
        uint32_t pbPacketHeadLen = static_cast<uint32_t>(packetHead.ByteSizeLong());
        SetPacketHeadLength(reinterpret_cast<uint32_t*>(pData), pbPacketHeadLen);
        pData += sizeof pbPacketHeadLen;
        packetHead.SerializeToArray(pData, pbPacketHeadLen);
        pData += pbPacketHeadLen;
        uint32_t pbPacketBodyLen = static_cast<uint32_t>(packetBody.ByteSizeLong());

        // Serialize packetBody
        if (pbPacketBodyLen + pbPacketHeadLen + sizeof(uint32_t) < sizeof buffHead) {
            // enough room to store in buff
            packetBody.SerializeToArray(pData, pbPacketBodyLen);
            pData += pbPacketBodyLen;
            return WriteData(sessionId, msgType, buffHead, pData - buffHead);
        } else {
            // msg body store in string
            std::string strPacketBody;
            packetBody.SerializeToString(&strPacketBody);
            return WriteData(sessionId, msgType, buffHead, pbPacketHeadLen + sizeof(uint32_t), strPacketBody.data(), strPacketBody.size());
        }
    }

    template <class _TypeMsgPacketHead>
    bool SendPacket(Session::session_id_t sessionId, uint32_t msgType, _TypeMsgPacketHead& packetHead, const char* data, std::size_t length)
    {
        FillHeadCommon(packetHead);
        char buffHead[sizeof(uint32_t) + kPacketHeadMaxLen + kPacketBodyMaxCache];
        char* pData = buffHead;
        // Serialize packetHead
        uint32_t pbPacketHeadLen = static_cast<uint32_t>(packetHead.ByteSizeLong());
        SetPacketHeadLength(reinterpret_cast<uint32_t*>(pData), pbPacketHeadLen);
        pData += sizeof pbPacketHeadLen;
        packetHead.SerializeToArray(pData, pbPacketHeadLen);
        pData += pbPacketHeadLen;
        return WriteData(sessionId, msgType, buffHead, pbPacketHeadLen + sizeof(uint32_t), data, length);
    }

    template <class _TypeHandler>
    void RegisterHandle(const _TypeHandler& func)
    {
        using PbHeadType = typename GetFunctionMessage_t<_TypeHandler>::TypeHead;
        using PbBodyType = typename GetFunctionMessage_t<_TypeHandler>::TypeBody;
        using PbFunRet = typename GetFunctionMessage_t<_TypeHandler>::TypeRet;
        uint32_t msgType = PbBodyType::msgid;
        RegisterHandleHelper<PbHeadType, PbBodyType, PbFunRet>(msgType, func);
    }

    // Async Callback Funtion
    template <class _TypeClass, class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeRet>
    void RegisterHandle(_TypeClass* pClass,
        _TypeRet (_TypeClass::*func)(Session::session_id_t, _TypeMsgPacketHead&, _TypeMsgPacketBody&))
    {
        std::function<_TypeRet(Session::session_id_t, _TypeMsgPacketHead&, _TypeMsgPacketBody&)> funHandle
            = std::bind(func, pClass, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        RegisterHandle(funHandle);
    }

public:
    struct AwaitableConnect {
        AwaitableConnect(const std::string& strIP, uint16_t nPort)
            : m_strIP(strIP)
            , m_nPort(nPort)
            , m_nSessionId(0) {};
        bool await_ready() const { return false; }
        auto await_resume() { return m_nSessionId; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            NetPbModule::GetInstance()->Connect(m_strIP, m_nPort, [this, handle](auto nSessionId, auto bSuccess) {
                if (bSuccess) {
                    m_nSessionId = nSessionId;
                }
                handle.resume();
            });
        }

    private:
        const std::string& m_strIP;
        uint16_t m_nPort;
        Session::session_id_t m_nSessionId;
    };

private:
    static NetPbModule* GetInstance();

protected:
    uint32_t GetPacketHeadLength(const uint32_t* pData);
    void SetPacketHeadLength(uint32_t* pData, uint32_t lenHead);
    uint32_t CheckCode(const char* data, size_t length);
    uint32_t CheckCode(const void* dataHead, size_t lenHead, const void* data, size_t len);
    uint32_t SwapUint32(uint32_t value);

private:
    void FillHeadCommon(Pb::ClientHead& packetHead);
    void FillHeadCommon(Pb::ServerHead& packetHead);
    bool CheckHeadTTLError(Pb::ClientHead& packetHead);
    bool CheckHeadTTLError(Pb::ServerHead& packetHead);
    void GenTransId(std::string& strTransId);

protected:
    std::unordered_map<uint32_t, FuncPacketHandleType> m_mapPackethandle;
    FuncPacketHandleType m_funDefaultPacketHandle;
    LoopPoolPtr m_workLoop = nullptr;

    SpinLock m_lockMsgFunction;
    std::vector<std::function<void()>> m_vecMsgFunction;

protected:
    NetModule* m_pNetModule = nullptr;
    ServiceGovernmentModule* m_pServiceGovernmentModule = nullptr;
    XmlConfigModule* m_pXmlConfigModule = nullptr;

private:
    static NetPbModule* m_pNetPbModule;
};

TONY_CAT_SPACE_END

#endif // COMMON_NET_NET_PB_MODULE_H_
