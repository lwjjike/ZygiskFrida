#include "inject.h"

#include <unistd.h>

#include <chrono>
#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "config.h"
#include "log.h"
#include "child_gating.h"
#include "xdl.h"
#include "remapper.h"

#ifdef __LP64__
constexpr const char* kZygoteNiceName = "zygote64";
constexpr const char* nextLoadSo = "/data/local/tmp/xbs.fgg/system/lib64/libxbs.so";
#else
constexpr const char* kZygoteNiceName = "zygote";
constexpr const char* nextLoadSo = "/data/local/tmp/xbs.fgg/system/lib/libxbs.so";
#endif

static std::string get_process_name() {
    auto path = "/proc/self/cmdline";

    std::ifstream file(path);
    std::stringstream buffer;

    buffer << file.rdbuf();
    return buffer.str();
}

static void wait_for_init(std::string const &app_name) {
    LOGI("Wait for process to complete init");

    // wait until the process is renamed to the package name
    while (get_process_name().find(app_name) == std::string::npos) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // additional tolerance for the init to complete after process rename
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    LOGI("Process init completed");
}

static void delay_start_up(uint64_t start_up_delay_ms) {
    if (start_up_delay_ms <= 0) {
        return;
    }

    LOGI("Waiting for configured start up delay %" PRIu64"ms", start_up_delay_ms);

    int countdown = 0;
    uint64_t delay = start_up_delay_ms;

    for (int i = 0; i < 10 && delay > 1000; i++) {
        delay -= 1000;
        countdown++;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

    for (int i = countdown; i > 0; i--) {
        LOGI("Injecting libs in %d seconds", i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void inject_lib(std::string const &lib_path, std::string const &logContext) {
    auto *handle = xdl_open(lib_path.c_str(), XDL_TRY_FORCE_LOAD);
    if (handle) {
        LOGI("%sInjected %s with handle %p", logContext.c_str(), lib_path.c_str(), handle);
        remap_lib(lib_path);
        return;
    }

    auto xdl_err = dlerror();

    handle = dlopen(lib_path.c_str(), RTLD_NOW);
    if (handle) {
        LOGI("%sInjected %s with handle %p (dlopen)", logContext.c_str(), lib_path.c_str(), handle);
        remap_lib(lib_path);
        return;
    }

    auto dl_err = dlerror();

//    LOGE("%sFailed to inject %s (xdl_open): %s", logContext.c_str(), lib_path.c_str(), xdl_err);
    LOGE("%sFailed to inject %s (dlopen): %s", logContext.c_str(), lib_path.c_str(), dl_err);
}

static void inject_libs(target_config const &cfg) {
    // We need to wait for process initialization to complete.
    // Loading the gadget before that will freeze the process
    // before the init has completed. This make the process
    // undiscoverable or otherwise cause issue attaching.
    wait_for_init(cfg.app_name);

    if (cfg.child_gating.enabled) {
        enable_child_gating(cfg.child_gating);
    }

    delay_start_up(cfg.start_up_delay_ms);

    // 开始注入so
    char command[256];
    std::string package_name = cfg.app_name;
    std::string inject_mode = cfg.inject_mode;  // 获取注入模式 script或者listen

    if (inject_mode == std::string("script")) {  // script模式
        // 拷贝Frida脚本
        sprintf(command, "cp /data/local/tmp/script.js /data/data/%s/", package_name.c_str());
        system(command);

        // 写入libhan.config.so
        sprintf(command, "echo '{\n  \"interaction\": {\n    \"type\": \"script\",\n    \"path\": \"/data/data/%s/script.js\"\n  }\n}' > /data/data/%s/libhan.config.so", package_name.c_str(), package_name.c_str());
        system(command);

        // 拷贝libhan.so
        sprintf(command, "cp %s /data/data/%s/", nextLoadSo, package_name.c_str());
        system(command);

        // 注入libhan.so
        std::string lib_path = std::string("/data/data/").append(package_name).append("/libhan.so");
        LOGI("Injecting %s with mode %s", lib_path.c_str(), inject_mode.c_str());
        inject_lib(lib_path, "");
    } else if (inject_mode == std::string("listen")) {  // listen模式
        LOGI("Injecting %s with mode %s", nextLoadSo, inject_mode.c_str());
        inject_lib(std::string(nextLoadSo), "");
    } else {
        LOGE("%s mode is not supported", inject_mode.c_str());
    }

    // for (auto &lib_path : cfg.injected_libraries) {
    //     LOGI("Injecting %s", lib_path.c_str());
    //     inject_lib(lib_path, "");
    // }
}

bool check_and_inject(std::string const &app_name) {
    std::string module_dir = std::string("/data/local/tmp/xbs.fgg");

    std::optional<target_config> cfg = load_config(module_dir, app_name);
    if (!cfg.has_value()) {
        return false;
    }

    LOGI("App detected: %s", app_name.c_str());
    LOGI("PID: %d", getpid());


    auto target_config = cfg.value();
    if (!target_config.enabled) {
        LOGI("Injection disabled for %s", app_name.c_str());
        return false;
    }

    std::thread inject_thread(inject_libs, target_config);
    inject_thread.detach();

    return true;
}
