/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef _DECODER_H
#define _DECODER_H

#include <GL/gl.h>
#include <list>
#include <inttypes.h>
#include "threading.h"
#include "graphics.h"
#include "flv.h"
extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace lightspark
{

class VideoDecoder
{
private:
	bool resizeGLBuffers;
protected:
	uint32_t frameWidth;
	uint32_t frameHeight;
	bool setSize(uint32_t w, uint32_t h);
public:
	VideoDecoder():resizeGLBuffers(false),frameWidth(0),frameHeight(0){}
	virtual ~VideoDecoder(){}
	virtual bool decodeData(uint8_t* data, uint32_t datalen)=0;
	virtual bool discardFrame()=0;
	//NOTE: the base implementation returns true if resizing of buffers should be done
	//This should be called in every derived implementation
	virtual bool copyFrameToTexture(TextureBuffer& tex)=0;
	uint32_t getWidth()
	{
		return frameWidth;
	}
	uint32_t getHeight()
	{
		return frameHeight;
	}
};

class FFMpegVideoDecoder: public VideoDecoder
{
private:
	class YUVBuffer
	{
	public:
		uint8_t* ch[3];
		YUVBuffer(){ch[0]=NULL;ch[1]=NULL;ch[2]=NULL;}
		~YUVBuffer()
		{
			if(ch[0])
			{
				free(ch[0]);
				free(ch[1]);
				free(ch[2]);
			}
		}
	};
	class YUVBufferGenerator
	{
	private:
		uint32_t bufferSize;
	public:
		YUVBufferGenerator(uint32_t b):bufferSize(b){}
		void init(YUVBuffer& buf) const;
	};
	GLuint videoBuffers[2];
	unsigned int curBuffer;
	AVCodecContext* codecContext;
	BlockingCircularQueue<YUVBuffer,10> buffers;
	Mutex mutex;
	bool initialized;
	AVFrame* frameIn;
	void copyFrameToBuffers(const AVFrame* frameIn);
	void setSize(uint32_t w, uint32_t h);
public:
	FFMpegVideoDecoder(uint8_t* initdata, uint32_t datalen);
	~FFMpegVideoDecoder();
	bool decodeData(uint8_t* data, uint32_t datalen);
	bool discardFrame();
	bool copyFrameToTexture(TextureBuffer& tex);
};

class AudioDecoder
{
protected:
	class FrameSamples
	{
	public:
		//Worst case buffer size, 1 second
		int16_t samples[AVCODEC_MAX_AUDIO_FRAME_SIZE/2];
		uint32_t len;
		FrameSamples():len(AVCODEC_MAX_AUDIO_FRAME_SIZE){}
	};
	std::list<FrameSamples> samplesList;
public:
	virtual ~AudioDecoder(){};
//	virtual bool discardFrame()=0;
	virtual bool decodeData(uint8_t* data, uint32_t datalen)=0;
};

class FFMpegAudioDecoder: public AudioDecoder
{
private:
	AVCodecContext* codecContext;
public:
	FFMpegAudioDecoder(FLV_AUDIO_CODEC codec, uint8_t* initdata, uint32_t datalen);
//	bool discardFrame();
	bool decodeData(uint8_t* data, uint32_t datalen);
};

};
#endif
