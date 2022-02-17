#pragma once
#ifndef FX_H
#define FX_H
namespace AuxPort
{

/*===================================================================================*/
/*
	[Class] Abstraction over AudioFilter class in FXObjects [Don't Mess with it]
*/
/*===================================================================================*/
	template<class bufferType, class effectType>
	class Filter
	{
	public:
		Filter() = default;
		Filter(const Filter& filter) = default;
		void setFilterType(const filterAlgorithm& type)
		{
			filterParameters.algorithm = type;
		}
		void setParameters(const effectType& centerFrequency, const effectType& QFactor, const effectType& boostCut)
		{
			filterParameters.Q = QFactor;
			filterParameters.fc = centerFrequency;
			filterParameters.boostCut_dB = boostCut;
			filter.setParameters(filterParameters);
		}
		bufferType process(const bufferType& frame)
		{
			return filter.processAudioSample(frame);
		}
		~Filter() = default;
	private:
		AudioFilter filter;
		AudioFilterParameters filterParameters;

	};

	template<class bufferType>
	class FullWave
	{
	public:
		FullWave() = default;
		~FullWave() = default;
		double process(const bufferType& frame,bool toProcess)
		{
			if (toProcess)
			{
				if (frame > 0 && previousFrame <= 0)
				{
					previousFrame = frame;
					previousProcessedFrame = 0;
					return 0;
				}
				else
				{
					bufferType newFrame;
					newFrame = previousProcessedFrame + previousFrame;
					previousProcessedFrame = newFrame;
					previousFrame = frame;
					return newFrame;
				}
			}
			else
				return frame;
		}
	private:
		bufferType previousFrame;
		bufferType previousProcessedFrame;
	};
	

	template<class bufferType>
	class FX
	{
	public:
		static void DCOffset(bufferType& frame, const double& offset, bool state, const double& mix)
		{
			if (state)
			{
				frame += offset;
				frame *= mix;
			}

		}

		static void PreGain(bufferType& frame, const float& preGain)
		{
			frame *= preGain;
		}

		static void PostGain(bufferType& frame, const double& postGain)
		{
			frame *= postGain;
		}

		static void ZeroCrossing(bufferType& frame, const double& threshold, bool state, const double& mix)
		{
			if (state)
			{
				if (abs(frame) < threshold)
				{
					frame = 0;
				}
				else
				{
					frame *= mix;
				}
			}

		}

		static void ArcTan1(bufferType& frame, const double& alpha, bool state, const double& mix)
		{
			if (state)
				frame = mix * atan(alpha * frame);
		}

		static void ArcTan2(bufferType& frame, const double& alpha, bool state, const double& mix)
		{
			if (state)
				frame = mix * (2 / kPi) * atan(alpha * frame);
		}

		static void Fullwave(bufferType& frame, bool state)
		{
			if (state)
			{
				if (frame < 0)
					frame *= -1;
			}
		}

		static void Halfwave(bufferType& frame, bool state)
		{
			if (state)
			{
				if (frame < 0)
					frame = 0;
			}
		}

		static void ArcTanH(bufferType& frame, const double& alpha, bool state, const double& mix)
		{
			if (state)
			{
				frame = mix * atanh(alpha * frame);
			}

		}

		static bufferType accumulate(const std::vector<bufferType> items)
		{
			bufferType sum = 0;
			for (size_t i = 0; i < items.size(); i++)
				sum += items[i];
			return sum;
		}
		static bufferType stereoToMono(const bufferType& left, const bufferType& right)
		{
			return left + right;
		}
	};
}
#endif