#ifndef COMMON_SERVICE_RPC_MODULE_H_
#define COMMON_SERVICE_RPC_MODULE_H_

#include "common/core_define.h"
#include "common/loop_pool.h"
#include "common/module_base.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "protocol/server_base.pb.h"

#include <cstdint>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;

class RpcModule : public ModuleBase {
public:
    RpcModule(ModuleManager* pModuleManager);
    virtual ~RpcModule();

    virtual void BeforeInit() override;
    virtual void AfterStop() override;

private:
    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    struct RpcContext {
        using PbFunction = std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>;
        std::unordered_map<int64_t, PbFunction> mapLastCallback;
        std::unordered_map<int64_t, PbFunction> mapCurrentCallback;
    };

public:
    enum {
        kRpcTimeoutMillSeconds = 20000,
    };

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeHandler>
    void RpcRequest(Session::session_id_t destSessionId, _TypeMsgPacketHead& pbPacketHead, const _TypeMsgPacketBody& msgBody, const _TypeHandler& funcResult)
    {
        using GetFunctionMessage_t = typename NetPbModule::GetFunctionMessage_t<_TypeHandler>;
        using PbRspHeadType = typename GetFunctionMessage_t::TypeHead;
        using PbRspBodyType = typename GetFunctionMessage_t::TypeBody;
        using RspRpcContext = RpcContext<PbRspHeadType, PbRspBodyType>;
        using PbRspFunction = typename RspRpcContext::PbFunction;
        uint32_t msgId = PbRspBodyType::msgid;
        static_assert(PbRspBodyType::msgid == _TypeMsgPacketBody::msgid + 1, "respond message type error");

        if (m_mapRpcContext.count(msgId) == 0) {
            auto pRpcContext = new RspRpcContext();
            m_mapRpcContext[msgId] = pRpcContext;
            m_mapRpcContextDeleter[msgId] = [](void* pData) {
                auto pContext = reinterpret_cast<RspRpcContext*>(pData);
                delete pContext;
            };

            m_pNetPbModule->RegisterHandle([this, pRpcContext](Session::session_id_t sessionId, PbRspHeadType& packetHead, PbRspBodyType& packetBody) {
                auto nRspQueryId = packetHead.query_id();
                auto itContext = pRpcContext->mapCurrentCallback.find(nRspQueryId);
                if (itContext != pRpcContext->mapCurrentCallback.end()) {
                    auto& func = itContext->second;
                    if (nullptr != func) {
                        func(sessionId, packetHead, packetBody);
                    }

                    pRpcContext->mapCurrentCallback.erase(itContext);
                } else {
                    itContext = pRpcContext->mapLastCallback.find(nRspQueryId);
                    if (itContext != pRpcContext->mapLastCallback.end()) {
                        auto& func = itContext->second;
                        if (nullptr != func) {
                            func(sessionId, packetHead, packetBody);
                        }

                        pRpcContext->mapLastCallback.erase(itContext);
                    } else {
                        LOG_ERROR("can not find rpc callback, query_id{}", nRspQueryId);
                    }
                }
            });

            Loop::GetCurrentThreadLoop().ExecEvery(kRpcTimeoutMillSeconds, [this, pRpcContext, msgId]() {
                for (auto& elemCallbacks : pRpcContext->mapLastCallback) {
                    auto nQueryId = elemCallbacks.first;
                    LOG_ERROR("rpc timeout, msgid:{}, query_id{}", msgId, nQueryId);
                    auto& func = elemCallbacks.second;
                    Session::session_id_t sessionId = 0;
                    static PbRspHeadType pbRspHead;
                    static PbRspBodyType pbRspBody;
                    pbRspHead.set_error_code(Pb::SSMessageCode::ss_msg_timeout);
                    func(sessionId, pbRspHead, pbRspBody);
                }
                pRpcContext->mapLastCallback.clear();
                pRpcContext->mapCurrentCallback.swap(pRpcContext->mapLastCallback);
            });
        }

        auto nCurrentQueryId = GenQueryId();
        auto pRpcContext = reinterpret_cast<RspRpcContext*>(m_mapRpcContext[msgId]);
        pRpcContext->mapCurrentCallback[nCurrentQueryId] = PbRspFunction(funcResult);
        pbPacketHead.set_query_id(nCurrentQueryId);

        m_pNetPbModule->SendPacket(destSessionId, pbPacketHead, msgBody);
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeMsgPacketBodyRsp>
    struct RequestAwaitable {
        RequestAwaitable(RpcModule* pRpcModule, Session::session_id_t destSessionId, _TypeMsgPacketHead& pbPacketHead, _TypeMsgPacketBody& msgBody)
            : m_pRpcModule(pRpcModule)
            , m_destSessionId(destSessionId)
            , m_pbPacketHead(pbPacketHead)
            , m_msgBody(msgBody)
            , m_tupResult() {};

        bool await_ready() const { return false; }
        auto await_resume() { return m_tupResult; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_pRpcModule->RpcRequest(m_destSessionId, m_pbPacketHead, m_msgBody,
                [this, handle](Session::session_id_t sessionId, _TypeMsgPacketHead& pbHead, _TypeMsgPacketBodyRsp& pbRsp) {
                    m_tupResult = std::tuple<Session::session_id_t, _TypeMsgPacketHead, _TypeMsgPacketBodyRsp>(sessionId, pbHead, pbRsp);
                    handle.resume();
                });
        }

        RpcModule* m_pRpcModule;
        Session::session_id_t m_destSessionId;
        _TypeMsgPacketHead& m_pbPacketHead;
        _TypeMsgPacketBody& m_msgBody;
        std::tuple<Session::session_id_t, _TypeMsgPacketHead, _TypeMsgPacketBodyRsp> m_tupResult;
    };

private:
    int64_t GenQueryId();

private:
    NetPbModule* m_pNetPbModule = nullptr;

    int64_t m_nQueryId = 0;
    std::unordered_map<uint32_t, void*> m_mapRpcContext;
    std::unordered_map<uint32_t, std::function<void(void*)>> m_mapRpcContextDeleter;
};

TONY_CAT_SPACE_END

#endif // COMMON_SERVICE_RPC_MODULE_H_
