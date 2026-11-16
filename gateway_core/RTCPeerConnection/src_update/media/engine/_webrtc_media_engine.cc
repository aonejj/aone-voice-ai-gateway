//////////////////////////////////////////////////////////////////////////
//
// author : kimi
// desc   : replace to  /media/engine/webrtc_media_engine.cc
//
//////////////////////////////////////////////////////////////////////////
#include "../../../rtc_pc_src/media/engine/RTCWebRTCVideoEngine.h"
#include "../../../rtc_pc_src/media/engine/RTCWebRTCVoiceEngine.h"

#include "_webrtc_media_engine.h"


namespace cricket {

std::unique_ptr<MediaEngineInterface> CreateMediaEngine(
	std::vector<AudioCodec> audio_decoder_codecs,
	std::vector<AudioCodec> audio_encoder_codecs,
	std::vector<VideoCodec> video_decoder_codecs,
	std::vector<VideoCodec> video_encoder_codecs) {
	auto video_engine = std::make_unique<RTCWebRtcVideoEngine>(
		video_decoder_codecs,
		video_encoder_codecs);

		RTC_LOG(LS_INFO) << "codec_trace CreateMediaEngine audio_encoder_codecs size "<<audio_encoder_codecs.size();
	auto audio_engine = std::make_unique<RTCWebRtcVoiceEngine>(
		audio_decoder_codecs,
		audio_encoder_codecs
		);

	return std::make_unique<CompositeMediaEngine>(
		std::move(audio_engine),
		std::move(video_engine));
}

}