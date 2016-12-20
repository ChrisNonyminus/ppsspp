// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.



// This is not really hardware, it's a software audio mixer running on the Media Engine.
// From the perspective of a PSP app though, it might as well be.

#pragma once

#include "Common/CommonTypes.h"
#include "Core/HW/BufferQueue.h"
#include "Core/HW/SasReverb.h"

class PointerWrap;

enum {
	PSP_SAS_VOICES_MAX = 32,

	PSP_SAS_PITCH_MIN = 0x0000,
	PSP_SAS_PITCH_BASE = 0x1000,
	PSP_SAS_PITCH_BASE_SHIFT = 12,
	PSP_SAS_PITCH_MAX = 0x4000,

	PSP_SAS_VOL_MAX = 0x1000,

	PSP_SAS_ADSR_CURVE_MODE_LINEAR_INCREASE = 0,
	PSP_SAS_ADSR_CURVE_MODE_LINEAR_DECREASE = 1,
	PSP_SAS_ADSR_CURVE_MODE_LINEAR_BENT = 2,
	PSP_SAS_ADSR_CURVE_MODE_EXPONENT_DECREASE = 3,
	PSP_SAS_ADSR_CURVE_MODE_EXPONENT_INCREASE = 4,
	PSP_SAS_ADSR_CURVE_MODE_DIRECT = 5,

	PSP_SAS_ADSR_ATTACK = 1,
	PSP_SAS_ADSR_DECAY = 2,
	PSP_SAS_ADSR_SUSTAIN = 4,
	PSP_SAS_ADSR_RELEASE = 8,

	PSP_SAS_ENVELOPE_HEIGHT_MAX = 0x40000000,
	PSP_SAS_ENVELOPE_FREQ_MAX = 0x7FFFFFFF,

	PSP_SAS_EFFECT_TYPE_OFF = -1,
	PSP_SAS_EFFECT_TYPE_ROOM = 0,
	PSP_SAS_EFFECT_TYPE_STUDIO_SMALL= 1,
	PSP_SAS_EFFECT_TYPE_STUDIO_MEDIUM = 2,
	PSP_SAS_EFFECT_TYPE_STUDIO_LARGE = 3,
	PSP_SAS_EFFECT_TYPE_HALL = 4,
	PSP_SAS_EFFECT_TYPE_SPACE = 5,
	PSP_SAS_EFFECT_TYPE_ECHO = 6,
	PSP_SAS_EFFECT_TYPE_DELAY = 7,
	PSP_SAS_EFFECT_TYPE_PIPE = 8,
	PSP_SAS_EFFECT_TYPE_MAX = 8,

	PSP_SAS_OUTPUTMODE_MIXED = 0,
	PSP_SAS_OUTPUTMODE_RAW = 1,
};

struct WaveformEffect {
	int type;
	int delay;
	int feedback;
	int leftVol;
	int rightVol;
	int isDryOn;
	int isWetOn;
};

enum VoiceType {
	VOICETYPE_OFF,
	VOICETYPE_VAG,  // default
	VOICETYPE_NOISE,
	VOICETYPE_TRIWAVE,  // are these used? there are functions for them (sceSetTriangularWave)
	VOICETYPE_PULSEWAVE,
	VOICETYPE_PCM,
	VOICETYPE_ATRAC3,
};

// VAG is a Sony ADPCM audio compression format, which goes all the way back to the PSX.
// It compresses 28 16-bit samples into a block of 16 bytes.
class VagDecoder {
public:
	VagDecoder() : data_(0), read_(0), end_(true) {
		memset(samples, 0, sizeof(samples));
	}
	void Start(u32 dataPtr, u32 vagSize, bool loopEnabled);

	void GetSamples(s16 *outSamples, int numSamples);

	void DecodeBlock(u8 *&readp);
	bool End() const { return end_; }

	void DoState(PointerWrap &p);

	u32 GetReadPtr() const { return read_; }

private:
	s16 samples[28];
	int curSample;

	u32 data_;
	u32 read_;
	int curBlock_;
	int loopStartBlock_;
	int numBlocks_;

	// rolling state. start at 0, should probably reset to 0 on loops?
	int s_1;
	int s_2;

	bool loopEnabled_;
	bool loopAtNextBlock_;
	bool end_;
};

class SasAtrac3 {
public:
	SasAtrac3() : contextAddr_(0), atracID_(-1), sampleQueue_(0), end_(false) {}
	~SasAtrac3() { if (sampleQueue_) delete sampleQueue_; }
	int setContext(u32 context);
	void getNextSamples(s16 *outbuf, int wantedSamples);
	int addStreamData(u32 bufPtr, u32 addbytes);
	void DoState(PointerWrap &p);
	bool End() const {
		return end_;
	}

private:
	u32 contextAddr_;
	int atracID_;
	BufferQueue *sampleQueue_;
	bool end_;
};

class ADSREnvelope {
public:
	ADSREnvelope();
	void SetSimpleEnvelope(u32 ADSREnv1, u32 ADSREnv2);

	void WalkCurve(int type, int rate);

	void KeyOn();
	void KeyOff();
	void End();

	inline void Step();

	int GetHeight() const {
		return height_ > (s64)PSP_SAS_ENVELOPE_HEIGHT_MAX ? PSP_SAS_ENVELOPE_HEIGHT_MAX : height_;
	}
	bool NeedsKeyOn() const {
		return state_ == STATE_KEYON;
	}
	bool HasEnded() const {
		return state_ == STATE_OFF;
	}

	int attackRate;
	int decayRate;
	int sustainRate;
	int releaseRate;
	int attackType;
	int decayType;
	int sustainType;
	int sustainLevel;
	int releaseType;

	void DoState(PointerWrap &p);

private:
	// Actual PSP values.
	enum ADSRState {
		// Okay, this one isn't a real value but it might be.
		STATE_KEYON_STEP = -42,

		STATE_KEYON = -2,
		STATE_OFF = -1,
		STATE_ATTACK = 0,
		STATE_DECAY = 1,
		STATE_SUSTAIN = 2,
		STATE_RELEASE = 3,
	};
	void SetState(ADSRState state);

	ADSRState state_;
	s64 height_;  // s64 to avoid having to care about overflow when calculating. TODO: this should be fine as s32
};

// A SAS voice.
// TODO: Look into pre-decoding the VAG samples on SetVoice instead of decoding them on the fly.
// It's not very likely that games encode VAG dynamically.
struct SasVoice {
	SasVoice()
		: playing(false),
			paused(false),
			on(false),
			type(VOICETYPE_OFF),
			vagAddr(0),
			vagSize(0),
			pcmAddr(0),
			pcmSize(0),
			pcmIndex(0),
			pcmLoopPos(0),
			sampleRate(44100),
			sampleFrac(0),
			pitch(PSP_SAS_PITCH_BASE),
			loop(false),
			noiseFreq(0),
			volumeLeft(PSP_SAS_VOL_MAX),
			volumeRight(PSP_SAS_VOL_MAX),
			effectLeft(PSP_SAS_VOL_MAX),
			effectRight(PSP_SAS_VOL_MAX) {
		memset(resampleHist, 0, sizeof(resampleHist));
	}

	void Reset();
	void KeyOn();
	void KeyOff();
	void ChangedParams(bool changedVag);

	void DoState(PointerWrap &p);

	void ReadSamples(s16 *output, int numSamples);
	bool HaveSamplesEnded() const;

	bool playing;
	bool paused;  // a voice can be playing AND paused. In that case, it won't play.
	bool on;   // key-on, key-off.

	VoiceType type;

	u32 vagAddr;
	u32 vagSize;
	u32 pcmAddr;
	int pcmSize;
	int pcmIndex;
	int pcmLoopPos;
	int sampleRate;

	uint32_t sampleFrac;
	int pitch;
	bool loop;

	int noiseFreq;

	int volumeLeft;
	int volumeRight;

	// volume to "Send" (audio-lingo) to the effects processing engine, like reverb
	int effectLeft;
	int effectRight;
	s16 resampleHist[2];

	ADSREnvelope envelope;

	VagDecoder vag;
	SasAtrac3 atrac3;
};

class SasInstance {
public:
	SasInstance();
	~SasInstance();

	void ClearGrainSize();
	void SetGrainSize(int newGrainSize);
	int GetGrainSize() const { return grainSize; }
	int EstimateMixUs();

	int maxVoices;
	int sampleRate;
	int outputMode;

	int *mixBuffer;
	int *sendBuffer;
	s16 *sendBufferDownsampled;
	s16 *sendBufferProcessed;

	FILE *audioDump;

	void Mix(u32 outAddr, u32 inAddr = 0, int leftVol = 0, int rightVol = 0);
	void MixVoice(SasVoice &voice);

	// Applies reverb to send buffer, according to waveformEffect.
	void ApplyWaveformEffect();
	void SetWaveformEffectType(int type);
	void WriteMixedOutput(s16 *outp, const s16 *inp, int leftVol, int rightVol);

	void GetDebugText(char *text, size_t bufsize);

	void DoState(PointerWrap &p);

	SasVoice voices[PSP_SAS_VOICES_MAX];
	WaveformEffect waveformEffect;

private:
	SasReverb reverb_;
	int grainSize;
};
