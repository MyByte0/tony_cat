#ifndef COMMON_SERVICE_RPC_MODULE_H_
#define COMMON_SERVICE_RPC_MODULE_H_

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"

#include <cstdint>
#include <functional>
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
    enum : int {
        kRpcTimeoutMillSeconds = 12000,
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
        // packetRespond::msg_id == packetRequest::msg_id + 1
        static_assert(PbRspBodyType::msgid == _TypeMsgPacketBody::msgid + 1, "respond message type error");

        // _TypeMsgPacketBody first call, register handle to NetPbModule
        if (m_mapRpcContext.count(msgId) == 0) {
            auto pRpcContext = new RspRpcContext();
            m_mapRpcContext[msgId] = pRpcContext;
            m_mapRpcContextDeleter[msgId] = [](void* pData) {
                auto pContext = reinterpret_cast<RspRpcContext*>(pData);
                delete pContext;
            };

            m_pNetPbModule->RegisterHandle([this, pRpcContext](Session::session_id_t sessionId, PbRspHeadType& packetHead, PbRspBodyType& packetBody) {
                // find callBack of the query_id, and call
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
                        LOG_ERROR("can not find rpc callback, query_id:{}", nRspQueryId);
                    }
                }
            });

            // on timeout on mapLastCallback, and swap with mapCurrentCallback
            Loop::GetCurrentThreadLoop().ExecEvery(kRpcTimeoutMillSeconds, [this, pRpcContext, msgId]() {
                for (auto& elemCallbacks : pRpcContext->mapLastCallback) {
                    auto nQueryId = elemCallbacks.first;
                    LOG_ERROR("rpc timeout, msgid:{}, query_id:{}", msgId, nQueryId);
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

        // store cb by query_id
        auto nCurrentQueryId = GenQueryId();
        auto pRpcContext = reinterpret_cast<RspRpcContext*>(m_mapRpcContext[msgId]);
        pRpcContext->mapCurrentCallback[nCurrentQueryId] = PbRspFunction(funcResult);
        pbPacketHead.set_query_id(nCurrentQueryId);

        m_pNetPbModule->SendPacket(destSessionId, pbPacketHead, msgBody);
    }

    struct AwaitableRpcRequest {
        template <class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeMsgPacketBodyRsp>
        AwaitableRpcRequest(Session::session_id_t destSessionId, _TypeMsgPacketHead& pbPacketHead, _TypeMsgPacketBody& msgBody,
            Session::session_id_t& rspSessionId, _TypeMsgPacketHead& rspHead, _TypeMsgPacketBodyRsp& rspMsg)
            : m_funSuspend([this, destSessionId, &pbPacketHead, &msgBody, &rspSessionId, &rspHead, &rspMsg](std::coroutine_handle<> handle) {
                RpcModule::GetInstance()->RpcRequest(destSessionId, pbPacketHead, msgBody,
                    [this, handle, &rspSessionId, &rspHead, &rspMsg](Session::session_id_t sessionId, _TypeMsgPacketHead& pbHead, _TypeMsgPacketBodyRsp& pbRsp) {
                        rspSessionId = sessionId;
                        rspHead = pbHead;
                        rspMsg = pbRsp;
                        handle.resume();
                    });
            }) {};

        bool await_ready() const { return false; }
        auto await_resume() { return; }
        void await_suspend(std::coroutine_handle<> handle)
        {
            m_funSuspend(handle);
        }

        std::function<void(std::coroutine_handle<>)> m_funSuspend;
    };

private:
    int64_t GenQueryId();
    static RpcModule* GetInstance();

private:
    static RpcModule* m_pRpcModule;
    NetPbModule* m_pNetPbModule = nullptr;

    int64_t m_nQueryId = 0;
    std::unordered_map<uint32_t, void*> m_mapRpcContext;
    std::unordered_map<uint32_t, std::function<void(void*)>> m_mapRpcContextDeleter;
};

TONY_CAT_SPACE_END

#endif // COMMON_SERVICE_RPC_MODULE_H_
