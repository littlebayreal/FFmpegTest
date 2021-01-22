//
// Created by GNNBEI on 2021/1/22.
//
#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {
    this->vm = vm;
    this->env = env;
    this->instance = env->NewGlobalRef(instance);

    jclass jclazz = env->GetObjectClass(instance);
    onPrepareMethodID = env->GetMethodID(jclazz,"onPrepare","(I)V");
}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(instance);
}

void JavaCallHelper::onPrepare(int threadID,int isSuccess) {
    if (threadID==THREAD_MAIN){
        env->CallVoidMethod(instance,onPrepareMethodID);
    } else{
        JNIEnv *env;
        if (vm->AttachCurrentThread(&env,0)!=JNI_OK){
            return;
        }
        env->CallVoidMethod(instance,onPrepareMethodID,isSuccess);
        vm->DetachCurrentThread();
    }
}
