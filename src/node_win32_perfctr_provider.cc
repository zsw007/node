#define __INIT_node_perfctr_provider_IMP
#include "node_counters.h"
#include "node_win32_perfctr_provider.h"

#include <perflib.h>

#include "node_perfctr_provider.h"


typedef ULONG (WINAPI *PerfStartProviderExFunc)(
    __in LPGUID ProviderGuid,
    __in_opt PPERF_PROVIDER_CONTEXT ProviderContext,
    __out PHANDLE Provider);

typedef ULONG (WINAPI *PerfStopProviderFunc)(
    __in HANDLE ProviderHandle);

typedef ULONG (WINAPI *PerfSetCounterSetInfoFunc)(
    __in HANDLE ProviderHandle,
    __inout_bcount(TemplateSize) PPERF_COUNTERSET_INFO Template,
    __in ULONG TemplateSize);

typedef PPERF_COUNTERSET_INSTANCE (WINAPI *PerfCreateInstanceFunc)(
    __in HANDLE ProviderHandle,
    __in LPCGUID CounterSetGuid,
    __in PCWSTR Name,
    __in ULONG Id);

typedef ULONG (WINAPI *PerfDeleteInstanceFunc)(
    __in HANDLE Provider,
    __in PPERF_COUNTERSET_INSTANCE InstanceBlock);

typedef ULONG (WINAPI *PerfSetULongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONG Value);

typedef ULONG (WINAPI *PerfSetULongLongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONGLONG Value);

typedef ULONG (WINAPI *PerfIncrementULongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONG Value);

typedef ULONG (WINAPI *PerfIncrementULongLongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONGLONG Value);

typedef ULONG (WINAPI *PerfDecrementULongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONG Value);

typedef ULONG (WINAPI *PerfDecrementULongLongCounterValueFunc)(
    __in HANDLE Provider,
    __inout PPERF_COUNTERSET_INSTANCE Instance,
    __in ULONG CounterId,
    __in ULONGLONG Value);


HMODULE advapimod;
PerfStartProviderExFunc perfctr_startProvider;
PerfStopProviderFunc perfctr_stopProvider;
PerfSetCounterSetInfoFunc perfctr_setCounterSetInfo;
PerfCreateInstanceFunc perfctr_createInstance;
PerfDeleteInstanceFunc perfctr_deleteInstance;
PerfSetULongCounterValueFunc perfctr_setULongValue;
PerfSetULongLongCounterValueFunc perfctr_setULongLongValue;
PerfIncrementULongCounterValueFunc perfctr_incrementULongValue;
PerfIncrementULongLongCounterValueFunc perfctr_incrementULongLongValue;
PerfDecrementULongCounterValueFunc perfctr_decrementULongValue;
PerfDecrementULongLongCounterValueFunc perfctr_decrementULongLongValue;

PPERF_COUNTERSET_INSTANCE perfctr_instance;


#define NODE_COUNTER_HTTP_SERVER_REQUEST     1
#define NODE_COUNTER_HTTP_SERVER_RESPONSE    2
#define NODE_COUNTER_HTTP_CLIENT_REQUEST     3
#define NODE_COUNTER_HTTP_CLIENT_RESPONSE    4
#define NODE_COUNTER_SERVER_CONNS            5
#define NODE_COUNTER_NET_BYTES_SENT          6
#define NODE_COUNTER_NET_BYTES_RECV          7
#define NODE_COUNTER_GC_PERCENTTIME          8
#define NODE_COUNTER_PIPE_BYTES_SENT         9
#define NODE_COUNTER_PIPE_BYTES_RECV         10


namespace node {


EXTERN_C DECLSPEC_SELECTANY HANDLE NodeCounterProvider = nullptr;

void InitPerfCountersWin32() {
  ULONG status;
  PERF_PROVIDER_CONTEXT providerContext;

  // create instance name using pid
#define INST_MAX_LEN       32
#define INST_PREFIX_LEN    5
#define INST_PREFIX        L"\x6e\x6f\x64\x65\x5f"

  wchar_t Inst[INST_MAX_LEN];
  DWORD pid = GetCurrentProcessId();
  wcscpy_s(Inst, INST_MAX_LEN, INST_PREFIX);
  _itow_s(pid, Inst + INST_PREFIX_LEN, INST_MAX_LEN - INST_PREFIX_LEN, 10);

  advapimod = LoadLibraryW(L"\x61\x64\x76\x61\x70\x69\x33\x32\x2e\x64\x6c\x6c");
  if (advapimod) {
    perfctr_startProvider = (PerfStartProviderExFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x53\x74\x61\x72\x74\x50\x72\x6f\x76\x69\x64\x65\x72\x45\x78");
    perfctr_stopProvider = (PerfStopProviderFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x53\x74\x6f\x70\x50\x72\x6f\x76\x69\x64\x65\x72");
    perfctr_setCounterSetInfo = (PerfSetCounterSetInfoFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x53\x65\x74\x43\x6f\x75\x6e\x74\x65\x72\x53\x65\x74\x49\x6e\x66\x6f");
    perfctr_createInstance = (PerfCreateInstanceFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x43\x72\x65\x61\x74\x65\x49\x6e\x73\x74\x61\x6e\x63\x65");
    perfctr_deleteInstance = (PerfDeleteInstanceFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x44\x65\x6c\x65\x74\x65\x49\x6e\x73\x74\x61\x6e\x63\x65");
    perfctr_setULongValue = (PerfSetULongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x53\x65\x74\x55\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");
    perfctr_setULongLongValue = (PerfSetULongLongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x53\x65\x74\x55\x4c\x6f\x6e\x67\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");
    perfctr_incrementULongValue = (PerfIncrementULongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x49\x6e\x63\x72\x65\x6d\x65\x6e\x74\x55\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");
    perfctr_incrementULongLongValue = (PerfIncrementULongLongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x49\x6e\x63\x72\x65\x6d\x65\x6e\x74\x55\x4c\x6f\x6e\x67\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");
    perfctr_decrementULongValue = (PerfDecrementULongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x44\x65\x63\x72\x65\x6d\x65\x6e\x74\x55\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");
    perfctr_decrementULongLongValue = (PerfDecrementULongLongCounterValueFunc)
      GetProcAddress(advapimod, "\x50\x65\x72\x66\x44\x65\x63\x72\x65\x6d\x65\x6e\x74\x55\x4c\x6f\x6e\x67\x4c\x6f\x6e\x67\x43\x6f\x75\x6e\x74\x65\x72\x56\x61\x6c\x75\x65");

    ZeroMemory(&providerContext, sizeof(providerContext));
    providerContext.ContextSize = sizeof(providerContext);

    if (!perfctr_startProvider ||
        !perfctr_setCounterSetInfo ||
        !perfctr_createInstance) {
      NodeCounterProvider = nullptr;
      return;
    }

    status = perfctr_startProvider(&NodeCounterSetGuid,
                                   &providerContext,
                                   &NodeCounterProvider);
    if (status != ERROR_SUCCESS) {
      NodeCounterProvider = nullptr;
      return;
    }

    status = perfctr_setCounterSetInfo(NodeCounterProvider,
                                       &NodeCounterSetInfo.CounterSet,
                                       sizeof(NodeCounterSetInfo.CounterSet));
    if (status != ERROR_SUCCESS) {
      perfctr_stopProvider(NodeCounterProvider);
      NodeCounterProvider = nullptr;
      return;
    }

    perfctr_instance = perfctr_createInstance(NodeCounterProvider,
                                              &NodeCounterSetGuid,
                                              Inst,
                                              1);
    if (perfctr_instance == nullptr) {
      perfctr_stopProvider(NodeCounterProvider);
      NodeCounterProvider = nullptr;
    }
  }
}


void TermPerfCountersWin32() {
  if (NodeCounterProvider != nullptr &&
    perfctr_stopProvider != nullptr) {
    perfctr_stopProvider(NodeCounterProvider);
    NodeCounterProvider = nullptr;
  }

  if (advapimod) {
    FreeLibrary(advapimod);
    advapimod = nullptr;
  }
}


void NODE_COUNT_HTTP_SERVER_REQUEST() {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongValue != nullptr) {
    perfctr_incrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_HTTP_SERVER_REQUEST,
                                1);
  }
}


void NODE_COUNT_HTTP_SERVER_RESPONSE() {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongValue != nullptr) {
    perfctr_incrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_HTTP_SERVER_RESPONSE,
                                1);
  }
}


void NODE_COUNT_HTTP_CLIENT_REQUEST() {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongValue != nullptr) {
    perfctr_incrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_HTTP_CLIENT_REQUEST,
                                1);
  }
}


void NODE_COUNT_HTTP_CLIENT_RESPONSE() {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongValue != nullptr) {
    perfctr_incrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_HTTP_CLIENT_RESPONSE,
                                1);
  }
}


void NODE_COUNT_SERVER_CONN_OPEN() {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongValue != nullptr) {
    perfctr_incrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_SERVER_CONNS,
                                1);
  }
}


void NODE_COUNT_SERVER_CONN_CLOSE() {
  if (NodeCounterProvider != nullptr &&
      perfctr_decrementULongValue != nullptr) {
    perfctr_decrementULongValue(NodeCounterProvider,
                                perfctr_instance,
                                NODE_COUNTER_SERVER_CONNS,
                                1);
  }
}


void NODE_COUNT_NET_BYTES_SENT(int bytes) {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongLongValue != nullptr) {
    perfctr_incrementULongLongValue(NodeCounterProvider,
                                    perfctr_instance,
                                    NODE_COUNTER_NET_BYTES_SENT,
                                    static_cast<ULONGLONG>(bytes));
  }
}


void NODE_COUNT_NET_BYTES_RECV(int bytes) {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongLongValue != nullptr) {
    perfctr_incrementULongLongValue(NodeCounterProvider,
                                    perfctr_instance,
                                    NODE_COUNTER_NET_BYTES_RECV,
                                    static_cast<ULONGLONG>(bytes));
  }
}


uint64_t NODE_COUNT_GET_GC_RAWTIME() {
  LARGE_INTEGER timegc;
  if (QueryPerformanceCounter(&timegc)) {
    return timegc.QuadPart;
  } else {
    return static_cast<uint64_t>(GetTickCount());
  }
}


void NODE_COUNT_GC_PERCENTTIME(unsigned int percent) {
  if (NodeCounterProvider != nullptr && perfctr_setULongValue != nullptr) {
    perfctr_setULongValue(NodeCounterProvider,
                          perfctr_instance,
                          NODE_COUNTER_GC_PERCENTTIME,
                          percent);
  }
}


void NODE_COUNT_PIPE_BYTES_SENT(int bytes) {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongLongValue != nullptr) {
    perfctr_incrementULongLongValue(NodeCounterProvider,
                                    perfctr_instance,
                                    NODE_COUNTER_PIPE_BYTES_SENT,
                                    static_cast<ULONGLONG>(bytes));
  }
}


void NODE_COUNT_PIPE_BYTES_RECV(int bytes) {
  if (NodeCounterProvider != nullptr &&
      perfctr_incrementULongLongValue != nullptr) {
    perfctr_incrementULongLongValue(NodeCounterProvider,
                                    perfctr_instance,
                                    NODE_COUNTER_PIPE_BYTES_RECV,
                                    static_cast<ULONGLONG>(bytes));
  }
}

}  // namespace node
