package com.sziti.pushstream;

class BeiPush {
   static {
       System.loadLibrary("push_stream");
   }
   public native void init();
   public native void start(String path);
   public native void stop();
   public native void release();
   public native void pushVideo(byte[] data,int width,int height,boolean needRotate,int degree);
   public native void pushAudio(byte[] data);
   public native void setVideoEncoderInfo(int width, int height, int fps, int bitrate);
   public native void setAudioEncoderInfo(int sampleRateInHz, int channels);
   public native int getInputSamples();
}
