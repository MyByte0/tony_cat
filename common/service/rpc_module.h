#ifndef COMMON_SERVICE_RPC_MODULE_H_
#define COMMON_SERVICE_RPC_MODULE_H_

#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include "common/core_define.h"
#include "common/loop/loop_pool.h"
#include "common/module_base.h"
#include "common/module_manager.h"
#include "common/net/net_pb_module.h"
#include "common/net/net_session.h"
#include "protocol/client_base.pb.h"
#include "protocol/server_base.pb.h"

TONY_CAT_SPACE_BEGIN

class XmlConfigModule;

class RpcModule : public ModuleBase {
public:
    explicit RpcModule(ModuleManager* pModuleManager);
    virtual ~RpcModule();

    void BeforeInit() override;
    void OnStop() override;

private:
    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    struct RpcContext {
        using PbFunction = std::function<void(Session::session_id_t sessionId,
                                              _TypeMsgPacketHead& packetHead,
                                              _TypeMsgPacketBody& packetBody)>;
        std::unordered_map<int64_t, PbFunction> mapLastCallback;
        std::unordered_map<int64_t, PbFunction> mapCurrentCallback;
    };

public:
    enum : int {
        kRpcTimeoutMillSeconds = 15000,
    };

    template <class _TypeRpcContext>
    auto InitRpcContextMap(uint32_t msgId) {
        auto pRpcContext = new _TypeRpcContext();
        m_mapRpcContext[msgId] = pRpcContext;
        m_mapRpcContextDeleter[msgId] = [](void* pData) {
            auto pContext = reinterpret_cast<_TypeRpcContext*>(pData);
            delete pContext;
        };
        return pRpcContext;
    }

    template <class _TypeRpcContext>
    auto GetRpcContext(uint32_t msgId) {
        return reinterpret_cast<_TypeRpcContext*>(m_mapRpcContext[msgId]);
    }

    template <class _TypePbRspHead, class _TypePbRspBody, class _TypeRpcContext>
    void RegisterNetHandle() {
        m_pNetPbModule->RegisterHandle([this](Session::session_id_t sessionId,
                                              _TypePbRspHead& headRsp,
                                              _TypePbRspBody& bodyRsp) {
            assert(m_pModuleManager->OnMainLoop());
            // find callBack of the query_id, and call
            auto pRpcContext =
                GetRpcContext<_TypeRpcContext>(_TypePbRspBody::msgid);
            auto nRspQueryId = headRsp.query_id();
            if (auto itContext =
                    pRpcContext->mapCurrentCallback.find(nRspQueryId);
                itContext != pRpcContext->mapCurrentCallback.end()) {
                if (auto func = std::move(itContext->second); nullptr != func) {
                    pRpcContext->mapCurrentCallback.erase(itContext);
                    func(sessionId, headRsp, bodyRsp);
                }
            } else {
                if (itContext = pRpcContext->mapLastCallback.find(nRspQueryId);
                    itContext != pRpcContext->mapLastCallback.end()) {
                    if (auto func = std::move(itContext->second);
                        nullptr != func) {
                        pRpcContext->mapLastCallback.erase(itContext);
                        func(sessionId, headRsp, bodyRsp);
                    }
                } else {
                    LOG_ERROR(
                        "can not find rpc callback, session:{}, query_id:{}",
                        sessionId, nRspQueryId);
                }
            }
        });
    }

    template <class _TypePbRspHead, class _TypePbRspBody, class _TypeRpcContext>
    void RegisterTimeoutHandle() {
        // on timeout on mapLastCallback, and swap with mapCurrentCallback
        Loop::GetCurrentThreadLoop().ExecEvery(
            kRpcTimeoutMillSeconds, [this]() {
                uint32_t msgId = _TypePbRspBody::msgid;
                auto pRpcContext = GetRpcContext<_TypeRpcContext>(msgId);
                for (const auto& elemCallbacks : pRpcContext->mapLastCallback) {
                    auto nQueryId = elemCallbacks.first;
                    LOG_ERROR("rpc timeout, msgid:{}, query_id:{}", msgId,
                              nQueryId);
                    const auto& func = elemCallbacks.second;
                    Session::session_id_t sessionId = 0;
                    static _TypePbRspHead pbRspHead;
                    static _TypePbRspBody pbRspBody;
                    pbRspHead.set_error_code(Pb::SSMessageCode::ss_msg_timeout);
                    func(sessionId, pbRspHead, pbRspBody);
                }
                pRpcContext->mapLastCallback.clear();
                pRpcContext->mapCurrentCallback.swap(
                    pRpcContext->mapLastCallback);
            });
    }

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBody,
              class _TypeHandler>
    void RpcRequest(Session::session_id_t destSessionId,
                    _TypeMsgPacketHead& pbPacketHead,
                    const _TypeMsgPacketBody& msgBody,
                    const _TypeHandler& funcResult) {
        using GetFunctionMessage_t =
            typename NetPbModule::GetFunctionMessage_t<_TypeHandler>;
        using PbRspHeadType = typename GetFunctionMessage_t::TypeHead;
        using PbRspBodyType = typename GetFunctionMessage_t::TypeBody;
        using RspRpcContext = RpcContext<PbRspHeadType, PbRspBodyType>;
        using PbRspFunction = typename RspRpcContext::PbFunction;
        uint32_t msgId = PbRspBodyType::msgid;
        // packetRespond::msg_id == packetRequest::msg_id + 1
        static_assert(PbRspBodyType::msgid == _TypeMsgPacketBody::msgid + 1,
                      "respond message type error");
        assert(m_pModuleManager->OnMainLoop());

        // _TypeMsgPacketBody first call, register handle to NetPbModule
        if (m_mapRpcContext.count(msgId) == 0) {
            auto pRpcContext = InitRpcContextMap<RspRpcContext>(msgId);
            RegisterNetHandle<PbRspHeadType, PbRspBodyType, RspRpcContext>();
            RegisterTimeoutHandle<PbRspHeadType, PbRspBodyType,
                                  RspRpcContext>();
        }

        // store cb by query_id
        auto nCurrentQueryId = GenQueryId();
        LOG_TRACE("request meseage:{}. query_id:{}", _TypeMsgPacketBody::msgid,
                  nCurrentQueryId);
        auto pRpcContext = GetRpcContext<RspRpcContext>(msgId);
        pRpcContext->mapCurrentCallback.emplace(nCurrentQueryId,
                                                PbRspFunction(funcResult));
        pbPacketHead.set_query_id(nCurrentQueryId);
        m_pNetPbModule->SendPacket(destSessionId, pbPacketHead, msgBody);
    }

    struct AwaitableRpcRequest {
        template <class _TypeMsgPacketHead, class _TypeMsgPacketBodyReq,
                  class _TypeMsgPacketBodyRsp>
        AwaitableRpcRequest(Session::session_id_t destSessionId,
                            _TypeMsgPacketHead& pbPacketHead,
                            _TypeMsgPacketBodyReq& msgBody,
                            Session::session_id_t& rspSessionId,
                            _TypeMsgPacketHead& rspHead,
                            _TypeMsgPacketBodyRsp& rspMsg)
            : m_funSuspend([this, destSessionId, &pbPacketHead, &msgBody,
                            &rspSessionId, &rspHead,
                            &rspMsg](std::coroutine_handle<> handle) {
                  RpcModule::GetInstance()->RpcRequest(
                      destSessionId, pbPacketHead, msgBody,
                      [this, handle, &rspSessionId, &rspHead, &rspMsg](
                          Session::session_id_t sessionId,
                          _TypeMsgPacketHead& pbHead,
                          _TypeMsgPacketBodyRsp& pbRsp) {
                          rspSessionId = sessionId;
                          rspHead = pbHead;
                          rspMsg = pbRsp;
                          handle.resume();
                      });
              }) {}

        bool await_ready() const { return false; }
        auto await_resume() { return; }
        void await_suspend(std::coroutine_handle<> handle) {
            m_funSuspend(handle);
        }

        std::function<void(std::coroutine_handle<>)> m_funSuspend;
    };

    template <class _TypeMsgPacketHead, class _TypeMsgPacketBodyReq,
              class _TypeMsgPacketBodyRsp>
    struct AwaitableRpcFetchHelper {
        AwaitableRpcFetchHelper(Session::session_id_t destSessionId,
                                _TypeMsgPacketHead& pbPacketHead,
                                _TypeMsgPacketBodyReq& msgBody)
            : destSessionId_(destSessionId),
              pbPacketHead_(pbPacketHead),
              msgBody_(msgBody){};

        bool await_ready() const { return false; }
        auto await_resume() {
            return std::tuple<Session::session_id_t, _TypeMsgPacketHead,
                              _TypeMsgPacketBodyRsp>{rspSessionId_, rspHead_,
                                                     rspMsg_};
        }
        void await_suspend(std::coroutine_handle<> handle) {
            RpcModule::GetInstance()->RpcRequest(
                destSessionId_, pbPacketHead_, msgBody_,
                [this, handle](Session::session_id_t sessionId,
                               _TypeMsgPacketHead& pbHead,
                               _TypeMsgPacketBodyRsp& pbRsp) {
                    rspSessionId_ = sessionId;
                    rspHead_ = pbHead;
                    rspMsg_ = pbRsp;
                    handle.resume();
                });
        }

        Session::session_id_t destSessionId_;
        _TypeMsgPacketHead& pbPacketHead_;
        _TypeMsgPacketBodyReq& msgBody_;

        Session::session_id_t rspSessionId_;
        _TypeMsgPacketHead rspHead_;
        _TypeMsgPacketBodyRsp rspMsg_;
    };

private:
    int64_t GenQueryId();
    static RpcModule* GetInstance();

private:
    static RpcModule* m_pRpcModule;
    NetPbModule* m_pNetPbModule = nullptr;

    int64_t m_nQueryId = 0;
    std::unordered_map<uint32_t, void*> m_mapRpcContext;
    std::unordered_map<uint32_t, std::function<void(void*)>>
        m_mapRpcContextDeleter;
};

#define AwaitableRpcFetch(_TypeMsgPacketBodyRsp, _destSessionId,          \
                          _pbPacketHead, _msgBody)                        \
    AwaitableRpcFetchHelper<std::remove_cvref_t<decltype(_pbPacketHead)>, \
                            std::remove_cvref_t<decltype(_msgBody)>,      \
                            std::remove_cvref_t<_TypeMsgPacketBodyRsp>>(  \
        _destSessionId, _pbPacketHead, _msgBody);

TONY_CAT_SPACE_END

#endif  // COMMON_SERVICE_RPC_MODULE_H_
