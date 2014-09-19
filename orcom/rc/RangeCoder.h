/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski

  Range coder algorithm based on 'Carryless rangecoder' by Dmitry Subbotin
*/

#ifndef H_RANGECODER
#define H_RANGECODER

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/BitMemory.h"


class RangeCoder
{
public:
	typedef uint64 Code;
	typedef uint32 Freq;

	static const uint32 CodeBits = 32;
	static const Freq TopValue = 0x00ffffffULL;
	static const Code Mask64 = 0xff00000000000000ULL;
	static const Freq Mask32 = 0xffffffffULL;

	RangeCoder()
		:	low(0)
		,	range(0)
	{}

protected:
	Code	low;
	Freq	range;
};


class RangeEncoder : private RangeCoder
{
public:
	RangeEncoder(BitMemoryWriter& byteStream_)
		:	byteStream(byteStream_)
	{}

	void Start()
	{
		low = 0;
		range = Mask32;
	}

	void EncodeFrequency(Freq symFreq_, Freq cumFreq_, Freq totalFreqSum_)
	{
		ASSERT(range > totalFreqSum_);
		range /= totalFreqSum_;
		low += range * cumFreq_;
		range *= symFreq_;

		while (range <= TopValue)
		{
			ASSERT(range != 0);
			if ((low ^ (low+range)) & Mask64)
			{
				Freq r = (Freq)low;
				range = (r | TopValue) - r;
			}
			byteStream.PutByte(low >> 56);
			low <<= 8, range <<= 8;
		}
	}

	void End()
	{
		for (int i = 0; i < 8; i++)
		{
			byteStream.PutByte(low >> 56);
			low <<= 8;
		}
	}

private:
	BitMemoryWriter& byteStream;
};


class RangeDecoder : private RangeCoder
{
public:
	RangeDecoder(BitMemoryReader& byteStream_)
		:	byteStream(byteStream_)
		,	buffer(0)
	{}

	void Start()
	{
		buffer = 0;
		for (uint32 i = 1; i <= 8; ++i)
		{
			buffer |= (Code)byteStream.GetByte() << (64 - i *8);
		}

		low = 0;
		range = Mask32;
	}

	Freq GetCumulativeFreq(Freq totalFreq_)
	{
		ASSERT(totalFreq_ != 0);
		return buffer / (range /= totalFreq_);
	}

	void UpdateFrequency(Freq symFreq_, Freq lowEnd_, Freq /*totalFreq_*/)
	{
		Freq r = lowEnd_*range;
		buffer -= r;
		low += r;
		range *= symFreq_;

		while (range <= TopValue)
		{
			if ((low ^ (low+range)) & Mask64)
			{
				Freq r = (Freq)low;
				range = (r | TopValue) - r;
			}

			buffer = (buffer << 8) + byteStream.GetByte();
			low <<= 8, range <<= 8;
		}
	}

	void End()
	{}

private:
	BitMemoryReader& byteStream;
	Code buffer;
};


#endif // H_RANGECODER
