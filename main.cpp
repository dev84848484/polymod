#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/log.h>

#define LOG_TAG "PolyMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

bool godMode = false;
bool speedHack = false;

void patchMemory(uintptr_t address, const char* hexBytes) {
    if (!address) return;
    
    size_t len = strlen(hexBytes);
    size_t byteCount = (len + 1) / 3;
    uint8_t* bytes = (uint8_t*)malloc(byteCount);
    
    for (size_t i = 0; i < byteCount; i++) {
        unsigned int byte;
        sscanf(hexBytes + (i * 3), "%2x", &byte);
        bytes[i] = (uint8_t)byte;
    }

    uintptr_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t pageStart = address & ~(pageSize - 1);
    mprotect((void*)pageStart, pageSize * 2, PROT_READ | PROT_WRITE | PROT_EXEC);

    memcpy((void*)address, bytes, byteCount);
    
    mprotect((void*)pageStart, pageSize * 2, PROT_READ | PROT_EXEC);
    
    free(bytes);
}

void* hack_thread(void*) {
    void* handle = nullptr;
    LOGI("Ждём загрузку libil2cpp.so...");
    
    do {
        handle = dlopen("libil2cpp.so", RTLD_LAZY);
        sleep(1);
    } while (!handle);

    uintptr_t base = (uintptr_t)handle;
    LOGI("libil2cpp.so загружен по адресу: %p", (void*)base);

    uintptr_t offset_godmode = 0x123456; 
    uintptr_t offset_speed   = 0x654321;

    while (true) {
        if (godMode) {
            patchMemory(base + offset_godmode, "20 00 80 D2 C0 03 5F D6");
            godMode = false;
            LOGI("Godmode применён.");
        }

        if (speedHack) {
            patchMemory(base + offset_speed, "00 00 A0 52 C0 03 5F D6");
            speedHack = false;
            LOGI("Speedhack применён.");
        }
        sleep(1); 
    }
    return nullptr;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("Мод загружается...");
    pthread_t pt;
    pthread_create(&pt, nullptr, hack_thread, nullptr);
    return JNI_VERSION_1_6;
}
