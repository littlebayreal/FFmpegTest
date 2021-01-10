//
// Created by bei on 2021/1/10.
//
#include <jni.h>
#ifndef FFMPEGTEST_JAVACALLHELPER_H
#define FFMPEGTEST_JAVACALLHELPER_H
class JavaCallHelper{
public:
    JavaCallHelper(JavaVM* _javaVm,JNIEnv* _jniENV,jobject &_object);
    ~JavaCallHelper();

    void onPrepare(int thread);

    void onProgress(int thread, int progress);

    void onError(int thread , int code);
private:
    JavaVM* javaVm;
    JNIEnv* env;
    jobject jobj;
    //回调方法
    jmethodID jmid_prepare;
    jmethodID jmid_error;
    jmethodID jmid_progress;
};
#endif //FFMPEGTEST_JAVACALLHELPER_H
