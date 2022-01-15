#ifndef NET_NET_PB_MODULE_H_
#define NET_NET_PB_MODULE_H_

#include "common/core_define.h"
#include "common/loop.h"
#include "common/loop_coroutine.h"
#include "common/loop_pool.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/utility/spin_lock.h"
#include "log/log_module.h"

#include "net_session.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <mutex>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetModule;

// NetPacket:
// || 4 bytes packetLen(PbPacket Len) || 4 bytes checkCode(for PbPacket) || 4 bytes msgType || PbPacket ||
// the PbPacket in NetPacket:
// || 4 bytes PbPacketHead Len || PbPacketHead || PbPacketBody ||

class NetPbModule : public ModuleBase {
public:
    NetPbModule(ModuleManager* pModuleManager);
    virtual ~NetPbModule();

    virtual void BeforeInit() override;

    virtual void OnUpdate() override;

public:
    enum {
        enm_head_len = 12,
        enm_packet_head_max_len = 64 * 1024,
        enm_packet_body_max_cache = 64 * 1024,
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

    template <typename _Ret, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_Ret(Session::session_id_t, _TypeHead&, _TypeMessage&)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
    };

    template <typename _Fn, typename _Ret, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_Ret (_Fn::*)(Session::session_id_t, _TypeHead&, _TypeMessage&)> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
    };

    template <typename _Fn, typename _Ret, typename _TypeHead, typename _TypeMessage>
    struct GetFunctionMessage<_Ret (_Fn::*)(Session::session_id_t, _TypeHead&, _TypeMessage&) const> {
        using TypeHead = _TypeHead;
        using TypeBody = _TypeMessage;
    };

    template <typename _Fn>
    struct GetFunctionMessage_t {
        using TypeHead = typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeHead;
        using TypeBody = typename GetFunctionMessage<decltype(&_Fn::operator())>::TypeBody;
    };

    template <class _TypeMsgPacketHead>
    bool PaserPbHead(const char* data, std::size_t length, _TypeMsgPacketHead& pbHead, const char*& nextData, std::size_t& nextLen)
    {
        uint32_t pbPacketHeadLen = 0;
        if (length < sizeof pbPacketHeadLen) [[unlikely]] {
            return false;
        }
        pbPacketHeadLen = GetPacketHeadLength(reinterpret_cast<const uint32_t*>(data));
        if (length < pbPacketHeadLen + sizeof pbPacketHeadLen) [[unlikely]] {
            return false;
        }
        assert(length < UINT32_MAX);
        const char* pData = data;
        uint32_t pbPacketBodyLen = uint32_t(length) - pbPacketHeadLen - sizeof pbPacketHeadLen;
        pData = data + sizeof pbPacketHeadLen;
        if (false == pbHead.ParseFromArray(pData, pbPacketHeadLen)) [[unlikely]] {
            return false;
        }
        pData += pbPacketHeadLen;

        nextData = pData;
        nextLen = pbPacketBodyLen;
        return true;
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    FuncPacketHandleType GenReadPacketFunc(
        std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>&& funCb)
    {
        return FuncPacketHandleType([this, funCb(std::move(funCb))](Session::session_id_t sessionId, uint32_t msgType, const char* data, std::size_t length) {
            const char* pData = nullptr;
            std::size_t pbPacketBodyLen = 0;
            _TypeMsgPacketHead msgPacketHead;
            if (false == PaserPbHead(data, length, msgPacketHead, pData, pbPacketBodyLen)) [[unlikely]] {
                LOG_ERROR("recv pb packet head error, sessionId:{} type:{}", sessionId, msgType);
                return false;
            }
            // \TODO maybe MsgPacket need use shared_ptr
            _TypeMsgPacketBody msgPacketBody;
            if (false == msgPacketBody.ParseFromArray(pData, int(pbPacketBodyLen))) [[unlikely]] {
                LOG_ERROR("recv pb packet body error, sessionId:{} type:{}", sessionId, msgType);
                return false;
            }

            std::function<void()> funCall = [sessionId, msgPacketHead(std::move(msgPacketHead)), msgPacketBody(std::move(msgPacketBody)), funCb(std::move(funCb))]() mutable { funCb(sessionId, msgPacketHead, msgPacketBody); };
            {
                std::lock_guard<SpinLock> lockGuard(m_lockMsgFunction);
                m_vecMsgFunction.emplace_back(std::move(funCall));
            }

            return true;
        });
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    FuncPacketHandleType GenReadPacketFunc(
        const std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>& funCb)
    {
        return GenReadPacketFunc(std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead & packetHead, _TypeMsgPacketBody & packetBody)>(funCb));
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    void RegisterHandleHelper(uint32_t msgType,
        const std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>& func)
    {
        RegisterPacketHandle(msgType, GenReadPacketFunc<_TypeMsgPacketHead, _TypeMsgPacketBody>(func));
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    bool SendPacket(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)
    {
        uint32_t msgType = _TypeMsgPacketBody::msgid;
        char head[sizeof(uint32_t) + enm_packet_head_max_len + enm_packet_body_max_cache];
        char* pData = head;
        uint32_t pbPacketHeadLen = static_cast<uint32_t>(packetHead.ByteSizeLong());
        SetPacketHeadLength(reinterpret_cast<uint32_t*>(pData), pbPacketHeadLen);
        pData += sizeof pbPacketHeadLen;
        packetHead.SerializeToArray(pData, pbPacketHeadLen);
        pData += pbPacketHeadLen;

        uint32_t pbPacketBodyLen = static_cast<uint32_t>(packetBody.ByteSizeLong());
        if (pbPacketBodyLen + pbPacketHeadLen + sizeof(uint32_t) < sizeof head) {
            packetBody.SerializeToArray(pData, pbPacketBodyLen);
            pData += pbPacketBodyLen;
            return WriteData(sessionId, msgType, head, pData - head);
        }

        std::string strPacketBody;
        packetBody.SerializeToString(&strPacketBody);
        return WriteData(sessionId, msgType, head, pbPacketHeadLen + sizeof(uint32_t), strPacketBody.data(), strPacketBody.size());
    }

    template <class _TypeMsgPacketHead>
    bool SendPacket(Session::session_id_t sessionId, uint32_t msgType, _TypeMsgPacketHead& packetHead, const char* data, std::size_t length)
    {
        char head[sizeof(uint32_t) + enm_packet_head_max_len + enm_packet_body_max_cache];
        char* pData = head;
        uint32_t pbPacketHeadLen = static_cast<uint32_t>(packetHead.ByteSizeLong());
        SetPacketHeadLength(reinterpret_cast<uint32_t*>(pData), pbPacketHeadLen);
        pData += sizeof pbPacketHeadLen;
        packetHead.SerializeToArray(pData, pbPacketHeadLen);
        pData += pbPacketHeadLen;
        return WriteData(sessionId, msgType, head, pbPacketHeadLen + sizeof(uint32_t), data, length);
    }

    template <class _TypeHandler>
    void RegisterHandle(const _TypeHandler& func)
    {
        using PbHeadType = typename GetFunctionMessage_t<_TypeHandler>::TypeHead;
        using PbBodyType = typename GetFunctionMessage_t<_TypeHandler>::TypeBody;
        uint32_t msgType = PbBodyType::msgid;
        RegisterHandleHelper<PbHeadType, PbBodyType>(msgType, func);
    }

    template <class _TypeClass, class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    void RegisterHandle(_TypeClass* pClass,
        void (_TypeClass::*func)(Session::session_id_t, _TypeMsgPacketHead&, _TypeMsgPacketBody&))
    {
        std::function<void(Session::session_id_t, _TypeMsgPacketHead&, _TypeMsgPacketBody&)> funHandle
            = std::bind(func, pClass, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        RegisterHandle(funHandle);
    }

protected:
    uint32_t GetPacketHeadLength(const uint32_t* pData);
    void SetPacketHeadLength(uint32_t* pData, uint32_t lenHead);
    uint32_t CheckCode(const char* data, size_t length);
    uint32_t CheckCode(const void* dataHead, size_t lenHead, const void* data, size_t len);
    uint32_t SwapUint32(uint32_t value);

protected:
    std::unordered_map<uint32_t, FuncPacketHandleType> m_mapPackethandle;
    FuncPacketHandleType m_funDefaultPacketHandle;

    SpinLock m_lockMsgFunction;
    std::vector<std::function<void()>> m_vecMsgFunction;

protected:
    NetModule* m_pNetModule = nullptr;
};

TONY_CAT_SPACE_END

#endif // NET_NET_PB_MODULE_H_
