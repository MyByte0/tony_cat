#ifndef COMMON_NET_NET_PB_MODULE_H_
#define COMMON_NET_NET_PB_MODULE_H_

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/core_define.h"
#include "common/log/log_module.h"
#include "common/loop/loop_coroutine.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_memory_pool.h"
#include "common/net/net_session.h"
#include "common/utility/spin_lock.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

class ModuleManager;
class NetModule;
class ServiceGovernmentModule;
class XmlConfigModule;

// NetPacket:
// || 4 bytes checkCode(for PbPacket) || 4 bytes packetLen(PbPacket Len) || 4
// bytes msgType || PbPacket
//////////////////////////////////////////
// the PbPacket in NetPacket:
// || 4 bytes PbPacketHead Len || PbPacketHead || PbPacketBody ||

class NetPbModule : public ModuleBase {
public:
    explicit NetPbModule(ModuleManager* pModuleManager);
    virtual ~NetPbModule();

    void BeforeInit() override;
    void OnInit() override;
    void AfterStop() override;
    void OnUpdate() override;

public:
    enum : int {
        kHeadLen = 12,
        kPacketHeadMaxLen = 16 * 1024,
        kPacketBodyMaxCache = 64 * 1024,
        kPacketTTLDefaultvalue = 30,
    };

    void Listen(const std::string& strAddress, uint16_t addressPort);
    void Connect(
        const std::string& strAddress, uint16_t addressPort,
        const Session::FunSessionConnect& funOnSessionConnect = nullptr,
        const Session::FunSessionClose& funOnSessionClose = nullptr);
    bool ReadData(Session::session_id_t sessionId, SessionBuffer& buf);

    typedef std::function<void(Session::session_id_t sessionId,
                               uint32_t msgType, const char* data,
                               std::size_t length)>
        FuncPacketHandleType;

    void RegisterPacketHandle(uint32_t msgType, FuncPacketHandleType func);
    void SetDefaultPacketHandle(FuncPacketHandleType func);

public:
    template <typename _Fn>
    struct GetFunctionMessage;

    template <typename _TypeRet, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet(
        Session::session_id_t, NetMemoryPool::PacketNode<_TypeHead>,
        NetMemoryPool::PacketNode<_TypeMessage>)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn, typename _TypeRet, typename _TypeHead,
              typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet (_Fn::*)(
        Session::session_id_t, NetMemoryPool::PacketNode<_TypeHead>,
        NetMemoryPool::PacketNode<_TypeMessage>)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn, typename _TypeRet, typename _TypeHead,
              typename _TypeMessage>
    struct GetFunctionMessage<_TypeRet (_Fn::*)(
        Session::session_id_t, NetMemoryPool::PacketNode<_TypeHead>,
        NetMemoryPool::PacketNode<_TypeMessage>) const> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
        using TypeRet = _TypeRet;
    };

    template <typename _Fn>
    struct GetFunctionMessage_t {
        using TypeHead =
            typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeHead;
        using TypeBody =
            typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeBody;
        using TypeRet =
            typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeRet;
    };

    template <class _TypeMsgPacketHead>
    bool PaserPbHead(const char* data, std::size_t length,
                     _TypeMsgPacketHead& pbHead, const char*& nextData,
                     std::size_t& nextLen) {
        uint32_t pbPacketHeadLen = 0;
        if (length < sizeof pbPacketHeadLen) [[unlikely]] {
            LOG_ERROR("paser head length:{}", length);
            return false;
        }
        // read PacketHeadLength
        pbPacketHeadLen =
            GetPacketHeadLength(reinterpret_cast<const uint32_t*>(data));
        if (length < pbPacketHeadLen + sizeof pbPacketHeadLen) [[unlikely]] {
            LOG_ERROR("paser head length:{}, PacketHeadLen:{}", length,
                      pbPacketHeadLen);
            return false;
        }
        assert(length < UINT32_MAX);
        // compelete PacketBodyLen
        uint32_t pbPacketBodyLen =
            uint32_t(length) - pbPacketHeadLen - sizeof pbPacketHeadLen;
        const char* pData = data + sizeof pbPacketHeadLen;
        if (false == pbHead.ParseFromArray(pData, pbPacketHeadLen))
            [[unlikely]] {
            LOG_ERROR("paser pbHead packet");
            return false;
        }
        pData += pbPacketHeadLen;

        nextData = pData;
        nextLen = pbPacketBodyLen;
        return true;
    }

private:
    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody,
              class _TypeRet>
    FuncPacketHandleType GeneratePaserPacketFunc(
        const std::function<
            _TypeRet(Session::session_id_t sessionId,
                     NetMemoryPool::PacketNode<_TypeMsgPacketHead> packetHead,
                     NetMemoryPool::PacketNode<_TypeMsgPacketBody> packetBody)>&
            funReadPacket) {
        return FuncPacketHandleType(
            [this, funReadPacket](Session::session_id_t sessionId,
                                  uint32_t msgType, const char* data,
                                  std::size_t length) {
                const char* pData = nullptr;
                std::size_t pbPacketBodyLen = 0;
                // paser packet
                NetMemoryPool::PacketNode<_TypeMsgPacketHead> msgPacketHead =
                    NetMemoryPool::PacketCreate<_TypeMsgPacketHead>();
                if (false == PaserPbHead(data, length, *msgPacketHead, pData,
                                         pbPacketBodyLen)) [[unlikely]] {
                    LOG_ERROR("recv pb packet head error, sessionId:{} type:{}",
                              sessionId, msgType);
                    return false;
                }
                if (!CheckHeadTTLError(*msgPacketHead)) {
                    LOG_ERROR("recv pb packet ttl, sessionId:{} msgId:{}",
                              sessionId, msgType);
                    return false;
                }

                NetMemoryPool::PacketNode<_TypeMsgPacketBody> msgPacketBody =
                    NetMemoryPool::PacketCreate<_TypeMsgPacketBody>();
                if (false == msgPacketBody->ParseFromArray(
                                 pData, int(pbPacketBodyLen))) [[unlikely]] {
                    LOG_ERROR("recv pb packet body error, sessionId:{} type:{}",
                              sessionId, msgType);
                    return false;
                }

                // for module update() call eventFunction
                std::function<void()> funNetEvent =
                    [sessionId, msgPacketHead(std::move(msgPacketHead)),
                     msgPacketBody = std::move(msgPacketBody),
                     funReadPacket = std::move(funReadPacket)]() mutable {
                        funReadPacket(sessionId, msgPacketHead, msgPacketBody);
                    };
                {
                    std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
                    m_vecMsgFunction.emplace_back(std::move(funNetEvent));
                }

                return true;
            });
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody,
              class _TypeRet>
    void RegisterHandleHelper(
        uint32_t msgType,
        const std::function<_TypeRet(
            Session::session_id_t sessionId,
            NetMemoryPool::PacketNode<_TypeMsgPacketHead> packetHead,
            NetMemoryPool::PacketNode<_TypeMsgPacketBody> packetBody)>& func) {
        RegisterPacketHandle(
            msgType,
            GeneratePaserPacketFunc<_TypeMsgPacketHead, _TypeMsgPacketBody,
                                    _TypeRet>(func));
    }

    SessionPtr GetSessionInMainLoop(Session::session_id_t sessionId);

public:
    class StrData {
    public:
        StrData(const char* data, size_t len) : m_strData(data, len) {}
        ~StrData() = default;
        size_t ByteSizeLong() { return m_strData.size(); }
        bool SerializeToArray(char* data, size_t len) {
            if (len != m_strData.size()) {
                assert(false);
                return false;
            }
            std::copy(m_strData.data(), m_strData.data() + m_strData.size(),
                      data);
            return true;
        }

    private:
        std::string m_strData;
    };

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    bool SendPacket(Session::session_id_t sessionId,
                    NetMemoryPool::PacketNode<_TypeMsgPacketHead> packetHead,
                    NetMemoryPool::PacketNode<_TypeMsgPacketBody> packetBody) {
        LOG_TRACE("send packet sessionId:{}, queryId: {}", sessionId,
                  packetHead->query_id());
        auto pSession = GetSessionInMainLoop(sessionId);
        if (pSession == nullptr) {
            LOG_ERROR("send packet notfind session. sessionId:{}, queryId: {}",
                      sessionId, packetHead->query_id());
            return false;
        }

        FillHeadCommon(*packetHead);
        pSession->GetLoop().Exec([this, pSession, packetHead, packetBody]() {
            uint32_t msgType = _TypeMsgPacketBody::msgid;
            WriteDataOnSessionThread(pSession, msgType, packetHead, packetBody);
        });

        return true;
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    bool SendPacket(Session::session_id_t sessionId, int32_t msgType,
                    NetMemoryPool::PacketNode<_TypeMsgPacketHead> packetHead,
                    NetMemoryPool::PacketNode<_TypeMsgPacketBody> packetBody) {
        LOG_TRACE("send packet sessionId:{}, queryId: {}", sessionId,
                  packetHead->query_id());
        auto pSession = GetSessionInMainLoop(sessionId);
        if (pSession == nullptr) {
            LOG_ERROR("send packet notfind session. sessionId:{}, queryId: {}",
                      sessionId, packetHead->query_id());
            return false;
        }

        FillHeadCommon(*packetHead);
        pSession->GetLoop().Exec([this, pSession, msgType, packetHead,
                                  packetBody]() {
            WriteDataOnSessionThread(pSession, msgType, packetHead, packetBody);
        });

        return true;
    }

    template <class _TypeHandler>
    void RegisterHandle(const _TypeHandler& func) {
        using PbHeadType =
            typename GetFunctionMessage_t<_TypeHandler>::TypeHead;
        using PbBodyType =
            typename GetFunctionMessage_t<_TypeHandler>::TypeBody;
        using PbFunRet = typename GetFunctionMessage_t<_TypeHandler>::TypeRet;
        uint32_t msgType = PbBodyType::msgid;
        RegisterHandleHelper<PbHeadType, PbBodyType, PbFunRet>(msgType, func);
    }

    // Async Callback Funtion
    template <class _TypeClass, class _TypeMsgPacketHead,
              class _TypeMsgPacketBody, class _TypeRet>
    void RegisterHandle(_TypeClass* pClass,
                        _TypeRet (_TypeClass::*func)(
                            Session::session_id_t,
                            NetMemoryPool::PacketNode<_TypeMsgPacketHead>,
                            NetMemoryPool::PacketNode<_TypeMsgPacketBody>)) {
        std::function<_TypeRet(Session::session_id_t,
                               NetMemoryPool::PacketNode<_TypeMsgPacketHead>,
                               NetMemoryPool::PacketNode<_TypeMsgPacketBody>)>
            funHandle = std::bind(func, pClass, std::placeholders::_1,
                                  std::placeholders::_2, std::placeholders::_3);
        RegisterHandle(funHandle);
    }

public:
    struct AwaitableConnect {
        AwaitableConnect(const std::string& strIP, uint16_t nPort)
            : m_strIP(strIP), m_nPort(nPort), m_nSessionId(0) {}
        bool await_ready() const { return false; }
        auto await_resume() { return m_nSessionId; }
        void await_suspend(std::coroutine_handle<> handle) {
            NetPbModule::GetInstance()->Connect(
                m_strIP, m_nPort,
                [this, handle](auto nSessionId, auto bSuccess) {
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
    bool SessionSend(SessionPtr pSession);

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    bool WriteDataOnSessionThread(
        SessionPtr pSession, uint32_t msgType,
        NetMemoryPool::PacketNode<_TypeMsgPacketHead> packetHead,
        NetMemoryPool::PacketNode<_TypeMsgPacketBody> packetBody) {
        if (nullptr == pSession) {
            LOG_ERROR("send data error on msgId:{}", msgType);
            return false;
        }

        uint32_t pbPacketHeadLen =
            static_cast<uint32_t>(packetHead->ByteSizeLong());
        uint32_t pbPacketBodyLen =
            static_cast<uint32_t>(packetBody->ByteSizeLong());
        if (pbPacketHeadLen + pbPacketBodyLen > 0xffffff) {
            return false;
        }

        uint32_t nPbPacketLen =
            pbPacketHeadLen + pbPacketBodyLen + sizeof pbPacketHeadLen;
        uint32_t nAllDataLen = nPbPacketLen + kHeadLen;
        auto pData = pSession->GetWriteData(nAllDataLen);
        if (pData == nullptr) {
            return false;
        }

        // Serialize packet
        char* pLenPacketHead = pData + kHeadLen;
        *(reinterpret_cast<uint32_t*>(pLenPacketHead)) =
            SwapUint32(pbPacketHeadLen);
        char* pPacketHead = pData + kHeadLen + sizeof pbPacketHeadLen;
        packetHead->SerializeToArray(pPacketHead, pbPacketHeadLen);
        char* pPacketBody =
            pData + kHeadLen + sizeof pbPacketHeadLen + pbPacketHeadLen;
        packetBody->SerializeToArray(pPacketBody, pbPacketBodyLen);

        uint32_t* pHeadAlig = reinterpret_cast<uint32_t*>(pData);
        *(pHeadAlig + 1) = SwapUint32(nPbPacketLen);
        *(pHeadAlig + 2) = SwapUint32(msgType);
        *(pHeadAlig) =
            SwapUint32(CheckCode(reinterpret_cast<const char*>(pHeadAlig + 1),
                                 nPbPacketLen + kHeadLen - sizeof(uint32_t)));

        pSession->MoveWritePos(nAllDataLen);
        return SessionSend(pSession);
    }

    bool ReadPbPacket(Session::session_id_t sessionId, uint32_t msgType,
                      const char* data, std::size_t length);

private:
    static NetPbModule* GetInstance();

protected:
    uint32_t GetPacketHeadLength(const uint32_t* pData);
    void SetPacketHeadLength(uint32_t* pData, uint32_t lenHead);
    uint32_t CheckCode(const char* data, size_t length);
    uint32_t CheckCode(const void* dataHead, size_t lenHead, const void* data,
                       size_t len);
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

#endif  // COMMON_NET_NET_PB_MODULE_H_
