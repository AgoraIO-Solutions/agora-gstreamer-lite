#ifndef _AGORA_IO_H_
#define _AGORA_IO_H_

#include <memory>
#include <chrono>
#include <functional>
#include <fstream>
#include <list>

#include "agoratype.h"
#include "context.h"

#include "agora_rtc_api.h"

class AgoraIo{

  public:
   AgoraIo(const bool& verbose, 
           event_fn fn,
			  void* userData,
           const int& in_audio_delay,
           const int& in_video_delay,
           const int& out_audio_delay,
           const int& out_video_delay,
           const bool& sendOnly=false,
           const bool& enableProxy=false,
           const int& proxyTimeout=0,
           const std::string& proxyIps="",
	   const bool& transcode=false);

   bool  init(char* in_app_id, 
               char* in_ch_id,
               char* in_user_id,
               bool is_audiouser,
               bool enable_enc,
               short enable_dual,
               unsigned int  dual_vbr, 
               unsigned short  dual_width,
               unsigned short  dual_height,
               unsigned short min_video_jb,
               unsigned short dfps);

    int sendVideo(const uint8_t * buffer,  
                        uint64_t len,
                        int is_key_frame,
                        long timestamp);

   int sendAudio(const uint8_t * buffer,  
                        uint64_t len,
                        long timestamp,
                        const long& duration=0);

#if 0
    void setOnAudioFrameReceivedFn(const OnNewAudioFrame_fn& fn);
    void setOnVideoFrameReceivedFn(const OnNewFrame_fn& fn);

    size_t getNextVideoFrame(uint8_t* data, 
                             size_t max_buffer_size,
                             int* is_key_frame,
                             uint64_t* ts);
                             
    size_t getNextAudioFrame(uint8_t* data, size_t max_buffer_size);
#endif

   void disconnect();

   void setPaused(const bool& flag);
#if 0
   //right now we support two params to the event
   void addEvent(const AgoraEventType& eventType, 
                  const std::string& userName,
                  const long& param1=0, 
                  const long& param2=0,
                  long* states=nullptr);

   void setEventFunction(event_fn fn, void* userData);

#endif   
   void setVideoOutFn(agora_media_out_fn videoOutFn, void* userData);
   void setAudioOutFn(agora_media_out_fn videoOutFn, void* userData);

   void setSendOnly(const bool& flag);
protected:

  bool initAgoraService(const std::string& appid);

  bool doConnect(char* in_app_id,
                 char* in_channel_id,
                 char* in_user_id);


 //retry to connect
 bool retryConnect(char* in_app_id,
                 char* in_channel_id,
                 char* in_user_id,
                 int timeout);   

  bool checkConnection();

  bool doSendHighVideo(const uint8_t* buffer,
                       uint64_t len,
                       int is_key_frame);

  bool doSendAudio( const uint8_t* buffer,  uint64_t len);

  void UpdatePredictedFps(const long& timestamp);

   //receiver events

   void receiveVideoFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length,
                           const int& isKeyFrame,
                           const uint64_t& ts);

   void receiveAudioFrame(const uint userId, 
                           const uint8_t* buffer,
                           const size_t& length,
                           const uint64_t& ts);

    void showFps();

    std::list<std::string> parseIpList();

    std::string createProxyString(std::list<std::string> ipList);

    int isIFrame(const uint8_t* packet, uint64_t len);

 private:

    WorkQueue_ptr                                 _receivedVideoFrames;
    WorkQueue_ptr                                 _receivedAudioFrames;

    bool                                          _verbose;

    TimePoint                                     _lastReceivedFrameTime;

    std::list<std::string>                         _activeUsers;
    std::string                                    _currentVideoUser;
    connection_id_t			           _connection;		
#if 0
    agora::base::IAgoraService*                     _service;
    agora::agora_refptr<agora::rtc::IRtcConnection> _connection;
    agora::rtc::RtcConnectionConfiguration          _rtcConfig;
#endif
    bool                                           _connected = false;

    TimePoint                                       _lastVideoUserSwitchTime;

    bool                                            _isRunning;

    TimePoint                                       _lastVideoSendTime;

    bool                                             _isPaused;

    event_fn                                         _eventfn;
    void*                                            _userEventData;

    //from the app to agora sdk
    SyncBuffer_ptr                                   _outSyncBuffer;
    SyncBuffer_ptr                                   _inSyncBuffer;

    int                                              _in_audio_delay;
    int                                              _in_video_delay;

    int                                              _out_audio_delay;
    int                                              _out_video_delay;

    agora_media_out_fn                                _videoOutFn;
    void*                                             _videoOutUserData;

    agora_media_out_fn                                _audioOutFn;
    void*                                             _audioOutUserData;

    TimePoint                                         _lastTimeAudioReceived;
    TimePoint                                         _lastTimeVideoReceived;

    std::shared_ptr<std::thread>                      _publishUnpublishCheckThread;

    bool                                              _isPublishingAudio;
    bool                                              _isPublishingVideo;

    int                                               _videoOutFps;
    int                                               _videoInFps;
    TimePoint                                         _lastFpsPrintTime;

    bool                                              _sendOnly;

    TimePoint                                         _lastSendTime;

    bool                                              _enableProxy;
    int                                               _proxyConnectionTimeOut; 
    std::string                                       _proxyIps;   

 };
#endif

