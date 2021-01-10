//
// Created by bei on 2021/1/10.
//
#include "JavaCallHelper.h"
#include "macro.h"
//初始化列表初始化参数
JavaCallHelper::JavaCallHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_obj) :javaVm(_javaVM),env(_env){
    //建立全局的引用对象jobj防止方法执行结束，内存被回收.
    jobj = env->NewGlobalRef(_obj);
    jclass jclassz = env->GetObjectClass(jobj);
    //ArtMethod error.
    jmid_error = env->GetMethodID(jclassz,"onError","(I)V");
    jmid_prepare = env->GetMethodID(jclassz,"onPrepare","()V");
    jmid_progress = env->GetMethodID(jclassz,"onProgress","(I)V");
}
//析构函数
JavaCallHelper::~JavaCallHelper() {

}

void JavaCallHelper::onPrepare(int thread) {
    //如果当前线程是子线程
    if(thread == THREAD_CHILD){
        JNIEnv* jniEnv;
        if((javaVm->AttachCurrentThread(&jniEnv , 0)) != JNI_OK){
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_prepare );
        //调用完成之后需要解绑.
        javaVm->DetachCurrentThread();
    }else{
        env->CallVoidMethod(jobj,jmid_prepare);
    }
}

void JavaCallHelper::onProgress(int thread, int progress) {

    if(thread == THREAD_CHILD){
        JNIEnv* jniEnv;
        if((javaVm->AttachCurrentThread(&jniEnv , 0)) != JNI_OK){
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_progress , progress);
        //调用完成之后需要解绑.
        javaVm->DetachCurrentThread();
    }else{
        env->CallVoidMethod(jobj,jmid_progress, progress);

    }
}

void JavaCallHelper::onError(int thread, int code) {

    if(thread == THREAD_CHILD){
        JNIEnv* jniEnv;
        if((javaVm->AttachCurrentThread(&jniEnv , 0)) != JNI_OK){
            return;
        }
        jniEnv->CallVoidMethod(jobj, jmid_error , code);
        //调用完成之后需要解绑.
        javaVm->DetachCurrentThread();
    }else{
        env->CallVoidMethod(jobj,jmid_error, code);

    }
}