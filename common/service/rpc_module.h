#ifndef COMMON_SERVICE_RPC_MODULE_H_
#define COMMON_SERVICE_RPC_MODULE_H_

#include "common/core_define.h"
#include "common/module_base.h"
#include "common/loop.h"
#include "net/net_session.h"
#include "net/net_pb_module.h"

#include <cstdint>
#include <unordered_map>

TONY_CAT_SPACE_BEGIN

class NetModule;
class XmlConfigModule;

class RpcModule : public ModuleBase {
public:
    RpcModule(ModuleManager* pModuleManager);
    virtual ~RpcModule();

    virtual void BeforeInit() override;
    

private:
    template<class _TypeMsgPacketHead, class _TypeMsgPacketBody>
    struct RpcContext
    {
        using PbFunction = std::function<void(Session::session_id_t sessionId, _TypeMsgPacketHead& packetHead, _TypeMsgPacketBody& packetBody)>;
        std::unordered_map<int64_t, PbFunction> mapLastCallback;
        std::unordered_map<int64_t, PbFunction> mapCurrentCallback;
    };

public:
    enum {
        kRpcTimeoutMillSeconds = 30000,
    };

    template<class _TypeMsgPacketHead, class _TypeMsgPacketBody, class _TypeHandler>
    void RpcRequest(Session::session_id_t destSessionId, _TypeMsgPacketHead& pbPacketHead, const _TypeMsgPacketBody& msgBody, const _TypeHandler& funcResult) {
        using GetFunctionMessage_t = typename NetPbModule::GetFunctionMessage_t<_TypeHandler>;
        using PbRspHeadType = typename GetFunctionMessage_t::TypeHead;
        using PbRspBodyType = typename GetFunctionMessage_t::TypeBody;
        using RspRpcContext = RpcContext<PbRspHeadType, PbRspBodyType>;
        using PbRspFunction = typename RspRpcContext::PbFunction;

        if (m_mapRpcContext.count(PbRspBodyType::msgid) == 0) {
            auto pRpcContext = new RspRpcContext();
            m_mapRpcContext[PbRspBodyType::msgid] = pRpcContext;

            m_pNetPbModule->RegisterHandle([this, pRpcContext, funcResult](Session::session_id_t sessionId, PbRspHeadType& packetHead, PbRspBodyType& packetBody) {
                auto nRspQueryId = packetHead.query_id();
                auto itContext = pRpcContext->mapCurrentCallback.find(nRspQueryId);
                if (itContext != pRpcContext->mapCurrentCallback.end()) {
                    auto& func = itContext->second;
                    if (nullptr != func) {
                        func(sessionId, packetHead, packetBody);
                    }

                    pRpcContext->mapCurrentCallback.erase(itContext);
                }
                else {
                    itContext = pRpcContext->mapLastCallback.find(nRspQueryId);
                    if (itContext != pRpcContext->mapLastCallback.end()) {
                        auto& func = itContext->second;
                        if (nullptr != func) {
                            func(sessionId, packetHead, packetBody);
                        }

                        pRpcContext->mapLastCallback.erase(itContext);
                    }
                    else {
                        LOG_ERROR("can not find rpc callback, query_id{}", nRspQueryId);
                    }
                }
            });

            Loop::GetCurrentThreadLoop().ExecEvery(kRpcTimeoutMillSeconds, [this, pRpcContext]() {
                for (auto& elemCallbacks : pRpcContext->mapLastCallback)
                {
                    auto nQueryId = elemCallbacks.first;
                    LOG_ERROR("rpc timeout, query_id{}", nQueryId);
                }
                pRpcContext->mapLastCallback.clear();
                pRpcContext->mapCurrentCallback.swap(pRpcContext->mapLastCallback);
            });
        }

        auto nCurrentQueryId = ++nQueryId;
        auto pRpcContext = (RspRpcContext*)(m_mapRpcContext[PbRspBodyType::msgid]);
        pRpcContext->mapCurrentCallback[nCurrentQueryId] = PbRspFunction(funcResult);
        pbPacketHead.set_query_id(nCurrentQueryId);

        m_pNetPbModule->SendPacket(destSessionId, pbPacketHead, msgBody);
    }

private:
    NetModule*          m_pNetModule = nullptr;
    NetPbModule*        m_pNetPbModule = nullptr;
    
    int64_t nQueryId = 0;
    std::unordered_map<uint32_t, void*> m_mapRpcContext;
};

TONY_CAT_SPACE_END

#endif  // COMMON_SERVICE_RPC_MODULE_H_
