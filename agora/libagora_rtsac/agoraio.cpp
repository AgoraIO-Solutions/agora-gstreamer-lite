#include "agoraio.h"

#include <stdbool.h>
#include <fstream>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/time.h>

//agora header files

#include "utilities.h"
#include "agoratype.h"
#include "agoralog.h"

#include "syncbuffer.h"
#include <sstream>
#include <list>

#define DEFAULT_CHANNEL_NAME "hello_demo"
#define DEFAULT_CERTIFACTE_FILENAME "certificate.bin"
#define DEFAULT_SEND_VIDEO_FILENAME "send_video.h264"
#define DEFAULT_SEND_AUDIO_FILENAME "send_audio_16k_1ch.pcm"
#define DEFAULT_8K_SEND_AUDIO_FILENAME "send_audio_8k_1ch.pcm"

#define DEFAULT_RECV_AUDIO_BASENAME "recv_audio"
#define DEFAULT_RECV_VIDEO_BASENAME "recv_video"
#define DEFAULT_SEND_VIDEO_FRAME_RATE (25)
#define DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE (100000)
#define DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE (1000000)
#define DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE (500000)
#define DEFAULT_SEND_AUDIO_FRAME_PERIOD_MS (20)
#define DEFAULT_PCM_SAMPLE_RATE (16000)
#define DEFAULT_PCM_CHANNEL_NUM (1)

#define LOGS(fmt, ...) fprintf(stdout, "" fmt "\n", ##__VA_ARGS__)
#define LOGD(fmt, ...) fprintf(stdout, "[DBG] " fmt "\n", ##__VA_ARGS__)
#define LOGI(fmt, ...) fprintf(stdout, "[INF] " fmt "\n", ##__VA_ARGS__)
#define LOGW(fmt, ...) fprintf(stdout, "[WRN] " fmt "\n", ##__VA_ARGS__)
#define LOGE(fmt, ...) fprintf(stdout, "[ERR] " fmt "\n", ##__VA_ARGS__)

AgoraIo::AgoraIo(const bool& verbose,
                event_fn fn,
			    void* userData,
                const int& in_audio_delay,
                const int& in_video_delay,
                const int& out_audio_delay,
                const int& out_video_delay,
                const bool& sendOnly,
                const bool& enableProxy,
                const int& proxyTimeout,
                const std::string& proxyIps,
                const bool& enableTranscode ):
 _verbose(verbose),
 _lastReceivedFrameTime(Now()),
 _currentVideoUser(""),
 _lastVideoUserSwitchTime(Now()),
 _isRunning(false),
 _isPaused(false),
 _eventfn(fn),
  _userEventData(userData),
 _outSyncBuffer(nullptr),
 _inSyncBuffer(nullptr),
 _in_audio_delay(in_audio_delay),
 _in_video_delay(in_video_delay),
 _out_audio_delay(out_audio_delay),
 _out_video_delay(out_video_delay),
 _videoOutFn(nullptr),
 _audioOutFn(nullptr),
 _lastTimeAudioReceived(Now()),
 _lastTimeVideoReceived(Now()),
 _videoOutFps(0),
 _videoInFps(0),
 _lastFpsPrintTime(Now()),
 _sendOnly(sendOnly),
 _lastSendTime(Now()),
 _enableProxy(enableProxy),
 _proxyConnectionTimeOut((proxyTimeout < 1 ) ? 10000 : proxyTimeout),
 _proxyIps(proxyIps)
{	
_activeUsers.clear();
}
typedef struct {
	const char *p_sdk_log_dir;

	const char *p_appid;
	const char *p_token;
	const char *p_channel;
	uint32_t uid;
	uint32_t area;

	// video related config
	video_data_type_e video_data_type;
	int send_video_frame_rate;
	const char *send_video_file_path;

	// audio related config
	audio_data_type_e audio_data_type;
	audio_codec_type_e audio_codec_type;
	const char *send_audio_file_path;

	uint32_t pcm_sample_rate;
	uint32_t pcm_channel_num;

	// advanced config
	bool send_video_generic_flag;
	bool enable_audio_mixer;
	bool receive_data_only;

	const char *license;
} app_config_t;

typedef struct {
	app_config_t config;

	void *video_file_parser;
	void *video_file_writer;

	void *audio_file_parser;
	void *audio_file_writer;

	connection_id_t conn_id;
	bool b_stop_flag;
	bool b_connected_flag;
} app_t;

static app_t g_app = {
    .config = {
        // common config
        .p_sdk_log_dir              = "io.agora.rtc_sdk",
        .p_appid                    = "d6b10eddcf514ca79254ed90936fdcef",
        .p_token                    = "",
        .p_channel                  = DEFAULT_CHANNEL_NAME,
        .uid                        = 0,
        .area                       = AREA_CODE_GLOB,

        // video related config
        .video_data_type            = VIDEO_DATA_TYPE_H264,
        .send_video_frame_rate      = DEFAULT_SEND_VIDEO_FRAME_RATE,
        .send_video_file_path       = DEFAULT_SEND_VIDEO_FILENAME,

        // audio related config
        .audio_data_type            = AUDIO_DATA_TYPE_PCM,
        .audio_codec_type           = AUDIO_CODEC_TYPE_OPUS,
        .send_audio_file_path       = DEFAULT_SEND_AUDIO_FILENAME,

        // pcm related config
        .pcm_sample_rate            = DEFAULT_PCM_SAMPLE_RATE,
        .pcm_channel_num            = DEFAULT_PCM_CHANNEL_NUM,

        // advanced config
        .send_video_generic_flag    = false,
        .enable_audio_mixer         = false,
        .receive_data_only          = false,

        .license                    = "eyJzaWduIjoiUGxtdmY4Y1k2eEROK3U1TTBLdVlsQm8yMzJiT0Y1TVh1dGxCVXZOR1AwcmJwN0NDVkZNNlRLT0Qrc3Z3Vmk1L3VGTWxPVG4zY1BQZGRIQTEwOGlJTWhFVGViZjY3Vzdxam5TckFxVG1rcmh1WmFFMnBUWDg2NXd0QjZrVDJSbnBtTVgxTGdia21sdXh0eVZheWVSVkNXT1preW1Mb1JqMk1xRjgrTG4wZ0k2SG54djFYYjVmNWFDZndPMUpHLzhVSHBkTnJwSndtN0NBdXFQWVQ5Z1hlUzhVZnI5UnErLyt6d0VOblIxQ0tteVpiSGVsQWMwYmI3aTJMSHZYRTcvZG1YVWxoRGxUandTVGRHQU9Wa1J1d241MGZIWnVKcnA3NHFGUm5GYS9UWjhobDNjT240dHpuTGU2MXBTMGJxbXFYbnU5cS9rVVJ4SVZVYnErekNraE53PT0iLCJjdXN0b20iOiIxMTAwNzciLCJjcmVkZW50aWFsIjoiNGE1ZmEyZWVkZGQyYjJjOWM3YzQxOTlhYWQ0ODIwNWI5YjBjOWJiMTllYWVmZGVhNDk5NmFhOTlmNmRkOTI1MiIsImR1ZSI6IjIwNzEwNTI3In0=",
    },

    .video_file_parser      = NULL,
    .video_file_writer      = NULL,
    .audio_file_parser      = NULL,
    .audio_file_writer      = NULL,

    .b_stop_flag            = false,
    .b_connected_flag       = false,
};

void AgoraIo::receiveVideoFrame(const uint userId, 
                                const uint8_t* buffer,
                                const size_t& length,
                                const int& isKeyFrame,
                                const uint64_t& ts){

    //do not read video if the pipeline is in pause state
    if(_isPaused) return;

    if(_inSyncBuffer!=nullptr && _isRunning){
        _inSyncBuffer->addVideo(buffer, length, isKeyFrame, ts);
    }
}

void AgoraIo::receiveAudioFrame(const uint userId, 
                                const uint8_t* buffer,
                                const size_t& length,
                                const uint64_t& ts){

    //do not read audio if the pipeline is in pause state
    if(_isPaused ) return;

     if(_inSyncBuffer!=nullptr && _isRunning){
        _inSyncBuffer->addAudio(buffer, length, ts);
     } 
}

static void __on_join_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed)
{
  g_app.b_connected_flag = true;
  connection_info_t conn_info = { 0 };
  agora_rtc_get_connection_info(g_app.conn_id, &conn_info);
  LOGI("[conn-%u] Join the channel %s successfully, uid %u elapsed %d ms", conn_id, conn_info.channel_name,
           uid, elapsed);
}

static void __on_connection_lost(connection_id_t conn_id)
{
	g_app.b_connected_flag = false;
	LOGW("[conn-%u] Lost connection from the channel", conn_id);
}

static void __on_rejoin_channel_success(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
	g_app.b_connected_flag = true;
	LOGI("[conn-%u] Rejoin the channel successfully, uid %u elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_user_joined(connection_id_t conn_id, uint32_t uid, int elapsed_ms)
{
	LOGI("[conn-%u] Remote user \"%u\" has joined the channel, elapsed %d ms", conn_id, uid, elapsed_ms);
}

static void __on_user_offline(connection_id_t conn_id, uint32_t uid, int reason)
{
	LOGI("[conn-%u] Remote user \"%u\" has left the channel, reason %d", conn_id, uid, reason);
}

static void __on_user_mute_audio(connection_id_t conn_id, uint32_t uid, bool muted)
{
	LOGI("[conn-%u] audio: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_user_mute_video(connection_id_t conn_id, uint32_t uid, bool muted)
{
	LOGI("[conn-%u] video: uid=%u muted=%d", conn_id, uid, muted);
}

static void __on_error(connection_id_t conn_id, int code, const char *msg)
{
	if (code == ERR_SEND_VIDEO_OVER_BANDWIDTH_LIMIT) {
		LOGE("Not enough uplink bandwdith. Error msg \"%s\"", msg);
		return;
	}

	if (code == ERR_INVALID_APP_ID) {
		LOGE("Invalid App ID. Please double check. Error msg \"%s\"", msg);
	} else if (code == ERR_INVALID_CHANNEL_NAME) {
		LOGE("Invalid channel name for conn_id %u. Please double check. Error msg \"%s\"", conn_id, msg);
	} else if (code == ERR_INVALID_TOKEN || code == ERR_TOKEN_EXPIRED) {
		LOGE("Invalid token. Please double check. Error msg \"%s\"", msg);
	} else if (code == ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY) {
		LOGE("Dynamic token is enabled but is not provided. Error msg \"%s\"", msg);
	} else {
		LOGW("Error %d is captured. Error msg \"%s\"", code, msg);
	}

	g_app.b_stop_flag = true;
}

static void __on_audio_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts,
							const void *data, size_t len, const audio_frame_info_t *info_ptr)
{
	LOGD("[conn-%u] on_audio_data, uid %u sent_ts %u data_type %d, len %zu", conn_id, uid, sent_ts,
           info_ptr->data_type, len);
        //TODO
	//AgoraIo::receiveAudioFrame(uid, data, len, sent_ts);
	//write_file(g_app.audio_file_writer, info_ptr->data_type, data, len);
}

static void __on_mixed_audio_data(connection_id_t conn_id, const void *data, size_t len,
                                  const audio_frame_info_t *info_ptr)
{
	LOGD("[conn-%u] on_mixed_audio_data, data_type %d, len %zu", conn_id, info_ptr->data_type, len);
	//write_file(g_app.audio_file_writer, info_ptr->data_type, data, len);
}

static void __on_video_data(connection_id_t conn_id, const uint32_t uid, uint16_t sent_ts, const void *data, size_t len,
                            const video_frame_info_t *info_ptr)
{
	LOGD("[conn-%u] on_video_data: uid %u sent_ts %u data_type %d frame_type %d stream_type %d len %zu", conn_id,
           uid, sent_ts, info_ptr->data_type, info_ptr->frame_type, info_ptr->stream_type, len);
        //TODO
	//int isKeyFrame = (info_ptr->frame_type== VIDEO_FRAME_KEY)? 1: 0;
	//receiveVideoFrame(uid, data, len, isKeyFrame, sent_ts);
}

static void __on_target_bitrate_changed(connection_id_t conn_id, uint32_t target_bps)
{
	LOGI("[conn-%u] Bandwidth change detected. Please adjust encoder bitrate to %u kbps", conn_id,
           target_bps / 1000);
}

static void __on_key_frame_gen_req(connection_id_t conn_id, uint32_t uid, video_stream_type_e stream_type)
{
	LOGW("[conn-%u] Frame loss detected. Please notify the encoder to generate key frame immediately", conn_id);
}

static void app_init_event_handler(agora_rtc_event_handler_t *event_handler, app_config_t *config)
{
	event_handler->on_join_channel_success = __on_join_channel_success;
	event_handler->on_connection_lost = __on_connection_lost;
	event_handler->on_rejoin_channel_success = __on_rejoin_channel_success;
	event_handler->on_user_joined = __on_user_joined;
	event_handler->on_user_offline = __on_user_offline;
	event_handler->on_user_mute_audio = __on_user_mute_audio;
	event_handler->on_user_mute_video = __on_user_mute_video;
	event_handler->on_target_bitrate_changed = __on_target_bitrate_changed;
	event_handler->on_key_frame_gen_req = __on_key_frame_gen_req;
	event_handler->on_video_data = __on_video_data;
	event_handler->on_error = __on_error;

	if (config->enable_audio_mixer) {
		event_handler->on_mixed_audio_data = __on_mixed_audio_data;
	} else {
		event_handler->on_audio_data = __on_audio_data;
	}
}

static int app_send_video(const uint8_t * buffer,
                              uint64_t len,
                              int is_key_frame)
{
	app_config_t *config = &g_app.config;
	uint8_t stream_id = 0;

	video_frame_info_t info;
	info.frame_type = is_key_frame ? VIDEO_FRAME_KEY : VIDEO_FRAME_DELTA;
	info.frame_rate = (video_frame_rate_e) config->send_video_frame_rate;
	info.data_type = config->video_data_type;
	info.stream_type = VIDEO_STREAM_HIGH;
	// API: send vido data
	int rval = agora_rtc_send_video_data(g_app.conn_id, (const void *)buffer, len, &info);
	if (rval < 0) {
		LOGE("Failed to send video data, reason: %s", agora_rtc_err_2_str(rval));
		return -1;
	}

	return 0;
}

static int app_send_audio(const uint8_t * buffer, uint64_t len)
{
	app_config_t *config = &g_app.config;

	// API: send audio data
	audio_frame_info_t info;
	info.data_type = config->audio_data_type;
	int rval = agora_rtc_send_audio_data(g_app.conn_id, buffer, len, &info);
	if (rval < 0) {
		LOGE("Failed to send audio data, reason: %s", agora_rtc_err_2_str(rval));
		return -1;
	}

	return 0;
}

bool  AgoraIo::init(char* in_app_id, 
                        char* in_ch_id,
                        char* in_user_id,
                        bool is_audiouser,
                        bool enable_enc,
		                short enable_dual,
                        unsigned int  dual_vbr, 
			            unsigned short  dual_width,
                        unsigned short  dual_height,
                        unsigned short min_video_jb,
                        unsigned short dfps){

    if(!initAgoraService(in_app_id)){
        return false;
    }

    int rval;
    app_config_t *config = &g_app.config;
    std::string _userId=in_user_id;
    int token_len = strlen(config->p_token);
    const char *p_token = token_len == 0 ? NULL : config->p_token;
    rtc_channel_options_t channel_options;
    memset(&channel_options, 0, sizeof(channel_options));
    channel_options.auto_subscribe_audio = !_sendOnly;
    channel_options.auto_subscribe_video = !_sendOnly;
    //channel_options.enable_audio_mixer = config->enable_audio_mixer;
    agora_rtc_set_bwe_param(CONNECTION_ID_ALL, DEFAULT_BANDWIDTH_ESTIMATE_MIN_BITRATE,
							DEFAULT_BANDWIDTH_ESTIMATE_MAX_BITRATE,
							DEFAULT_BANDWIDTH_ESTIMATE_START_BITRATE);

    _connection = agora_rtc_create_connection(&g_app.conn_id);
    if (_connection < 0) {
        logMessage("Error creating connection to Agora SDK");
        return false;
    }

    std::cout<<" connecting to: "<<in_ch_id << "  " <<  _proxyConnectionTimeOut <<"  " << in_app_id << std::endl;
    rval = agora_rtc_join_channel(g_app.conn_id, in_ch_id, (unsigned int)atoi(in_user_id), p_token, &channel_options);
    //TODO add cloud proxy
#if 0
    if (!checkConnection() && _enableProxy) {
	_connection->disconnect();
        agora::base::IAgoraParameter* agoraParameter = _connection->getAgoraParameter();
        auto ipList=parseIpList();
        if (ipList.size() > 1) {
            auto ipListString=createProxyString(ipList);
            std::cout<< "Set proxy IPs  " << ipList.size() << std::endl;
            agoraParameter->setParameters(ipListString.c_str());
        } else {
            std::cout<< "Enable proxy with default access IPs " << std::endl;
            agoraParameter->setBool("rtc.enable_proxy", true);
        }
       	doConnect(in_app_id, in_ch_id, in_user_id);
    }
#endif
    if (rval < 0) {
	std::string str = config->p_channel;
        logMessage("Failed to join channel {}, reason: {}" + str +
			 std::to_string(rval));
	return false;
    }
    while (!g_app.b_connected_flag && !g_app.b_stop_flag) {
      usleep(10 * 1000);
    }

#if 0

    if(_sendOnly==false){
        h264FrameReceiver = std::make_shared<H264FrameReceiver>();
        _userObserver->setVideoEncodedImageReceiver(h264FrameReceiver.get());

        //video
        _receivedVideoFrames=std::make_shared<WorkQueue <Work_ptr> >();
        h264FrameReceiver->setOnVideoFrameReceivedFn([this](const uint userId, 
                                                    const uint8_t* buffer,
                                                    const size_t& length,
                                                    const int& isKeyFrame,
                                                    const uint64_t& ts){

            receiveVideoFrame(userId, buffer, length, isKeyFrame, ts);

        });

        //audio
        _receivedAudioFrames=std::make_shared<WorkQueue <Work_ptr> >();
        _pcmFrameObserver->setOnAudioFrameReceivedFn([this](const uint userId, 
                                                        const uint8_t* buffer,
                                                        const size_t& length,
                                                        const uint64_t& ts){

             receiveAudioFrame(userId, buffer, length,ts);
        });

        //connection observer: handles user join and leave
        _connectionObserver->setOnUserStateChanged([this](const std::string& userId,
                                                      const UserState& newState){
                    
                handleUserStateChange(userId, newState);

        });
    }
#endif
    //setup the in sync buffer ( AG sdk -> source)
    if (!_sendOnly) {
    _inSyncBuffer=std::make_shared<SyncBuffer>(_out_video_delay, _out_audio_delay, false);
    _inSyncBuffer->setVideoOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength,
                                         const bool& isKeyFrame){
  
        if(_videoOutFn!=nullptr){
            _videoOutFn(buffer, bufferLength, _videoOutUserData);
            _videoInFps++;
        }

    });

    _inSyncBuffer->setAudioOutFn([this](const uint8_t* buffer,
                                         const size_t& bufferLength){
        
        //TODO: we can print volume for each few seconds to make sure we got audio from the sdk
        //auto volume=calcVol((const int16_t*)buffer, bufferLength/2);
        //if(volume>0){
          // std::cout<<"audio volume: "<<volume<<std::endl;
       // }
        
        if(_audioOutFn!=nullptr){
            _audioOutFn(buffer, bufferLength, _audioOutUserData); 
        } 
          
    });

    _inSyncBuffer->start();
    }

    if (g_app.b_connected_flag == true)
    {
       std::string s(config->p_channel);
       logMessage("connected to channel" + s);
       _isRunning= true;
       _connected = true;
       return _connected;
    } else 
    {
       logMessage("Error connecting to channel");
       return false;
    }
}

bool AgoraIo::initAgoraService(const std::string& appid)
{
    app_config_t *config = &g_app.config;
    int rval;
    std::string str = agora_rtc_get_version();
    config->p_appid = appid.c_str();
    int appid_len = strlen(config->p_appid);
    const char *p_appid = (appid_len == 0 ? NULL : config->p_appid);
    
    logMessage("Agora RTSA SDK version: " + str);
    // TODO: API: verify license
#if 0
    	if(verifyLicense() != 0) {
      return false;
    }
#endif

    rtc_service_option_t service_opt = { 0 };
    agora_rtc_event_handler_t event_handler = { 0 };
	app_init_event_handler(&event_handler, config);
	service_opt.area_code = config->area;
	service_opt.log_cfg.log_path = config->p_sdk_log_dir;
        service_opt.log_cfg.log_level = RTC_LOG_INFO;
	snprintf(service_opt.license_value, sizeof(service_opt.license_value), "%s", config->license);
	str = p_appid;
    logMessage("init with appID: " + appid + str);
	rval = agora_rtc_init(p_appid, &event_handler, &service_opt);
	if (rval < 0) {
	  logMessage("Failed to initialize Agora sdk, reason: {}" + std::to_string(rval));
          return false;
	}
    return true;
}

int AgoraIo::sendVideo(const uint8_t * buffer,  
                              uint64_t len,
                              int is_key_frame,
                              long timestamp)
{
    logMessage("Agora sdk send video : %s" + std::to_string(buffer[0]));
    //LOGI("[len-%lu] [key frame: %d val %u]", len, is_key_frame, buffer[len/2] );
    //do nothing if we are in pause state
    if(_isPaused==true){
        return 0;
    }    
    if( _isRunning){
         app_send_video(buffer, len, is_key_frame);
	 // LOGI("real send [len-%lu] [key frame: %d val %u]", len, is_key_frame, buffer[len/2] );
    }

    _lastTimeVideoReceived=Now();

    showFps();
    return 0; //no errors
}

int AgoraIo::sendAudio(const uint8_t * buffer,  
                       uint64_t len,
                       long timestamp,
                       const long& duration)
{
    TimePoint nextAudiopacketTime=_lastTimeAudioReceived+std::chrono::milliseconds(duration);

    //do nothing if we are in pause state
    if(_isPaused==true){
	LOGI("Paused Audio real send [len-%lu] val %u", len, buffer[len/2] );
        return 0;
    }

    if(_isRunning){
	app_send_audio(buffer, len);
	LOGI("Audio real send [len-%lu] val %u", len, buffer[len/2] );
     }

    //block this thread loop until the duration of the current buffer is elapsed
    if(duration>0){
        std::this_thread::sleep_until(nextAudiopacketTime);
    }

    _lastTimeAudioReceived=Now();

    return 0;
}

void AgoraIo::setPaused(const bool& flag){

    _isPaused=flag;
    if(_isPaused==true){
    }
    else{
      _inSyncBuffer->clear();

    }
    agora_rtc_mute_local_audio(g_app.conn_id, flag);
    agora_rtc_mute_local_video(g_app.conn_id, flag);
}

void agora_log_message(const char* message){

   /*if(ctx->callConfig->useDetailedAudioLog()){
      logMessage(std::string(message));
   }*/
}

bool AgoraIo::doSendHighVideo(const uint8_t* buffer,  uint64_t len,int is_key_frame){

    return true;
}    
#if 0
bool AgoraIo::doSendAudio(const uint8_t* buffer,  uint64_t len){

  agora::rtc::EncodedAudioFrameInfo audioFrameInfo;
  audioFrameInfo.numberOfChannels =1; //TODO
  audioFrameInfo.sampleRateHz = 48000; //TODO
  audioFrameInfo.codec = agora::rtc::AUDIO_CODEC_OPUS;

  _audioSender->sendEncodedAudioFrame(buffer,len, audioFrameInfo);

  //auto diff=GetTimeDiff(_lastSendTime, Now());
  // logMessage("sent audo packet. diff time: "+std::to_string(diff));

  //_lastSendTime=Now();

  return true;
}
#endif

void AgoraIo::showFps(){

   if(_verbose){
        _videoOutFps++;
        if(_lastFpsPrintTime+std::chrono::milliseconds(1000)<=Now()){

            std::cout<<"Out video fps: "<<_videoOutFps<<std::endl;
            std::cout<<"In video fps: "<<_videoInFps<<std::endl;

            _videoInFps=0;
            _videoOutFps=0;

            _lastFpsPrintTime=Now();
        }
    }
}

void AgoraIo::disconnect(){

    logMessage("start agora disonnect ...");

   _isRunning=false;

   //tell the thread that we are finished
    _outSyncBuffer->stop();
    _inSyncBuffer->stop();

   std::this_thread::sleep_for(std::chrono::seconds(2));
   agora_rtc_leave_channel(g_app.conn_id);

   agora_rtc_destroy_connection(g_app.conn_id);

   _inSyncBuffer=nullptr;
   
   //delete context
   //connection=nullptr;

   //_service->release();
   //_service = nullptr;

   //h264FrameReceiver=nullptr;

   std::cout<<"agora disconnected\n";

   logMessage("Agora disonnected ");
}

void AgoraIo::setVideoOutFn(agora_media_out_fn videoOutFn, void* userData){
     _videoOutFn=videoOutFn;
     _videoOutUserData=userData;
 }

void AgoraIo::setAudioOutFn(agora_media_out_fn videoOutFn, void* userData){
     _audioOutFn=videoOutFn;
     _audioOutUserData=userData;
 }

void AgoraIo::setSendOnly(const bool& flag){
    _sendOnly=flag;
}

#if 0
std::list<std::string> AgoraIo::parseIpList(){

    std::stringstream ss(_proxyIps);

   std::list<std::string>  returnList;
   returnList.clear();

    while (ss.good()){
        std::string ip;
        getline(ss, ip, ',');
        returnList.emplace_back(ip);

        std::cout<<ip<<std::endl;
    }

  return returnList;
}

std::string AgoraIo::createProxyString(std::list<std::string> ipList){

    //TODO: this is a reference of how the proxy string looks like
    //agoraParameter->setParameters("{\"rtc.proxy_server\":[2, \"[\\\"128.1.77.34\\\", \\\"128.1.78.146\\\"]\", 0], \"rtc.enable_proxy\":true}");

    std::string ipListStr="\"[";
    bool addComma=false;
    for(const auto ip: ipList){

        if(addComma){
            ipListStr+=",";
        } 
        else{
            addComma=true;
        }

        ipListStr +="\\\""+ip+"\\\" ";
    }

    ipListStr+="]\", ";

    std::string proxyString="{\"rtc.proxy_server\":[2, "+
                             ipListStr +
                             "0], \"rtc.enable_proxy\":true}";

    return proxyString;
}
#endif
