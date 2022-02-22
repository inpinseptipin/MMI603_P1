#ifndef AuxPort_AudioEffect_H
#define AuxPort_AudioEffect_H
#pragma once
/*
*			AuxPort Wrapper over Rackfx
			"I tried to make it easier" - inpinseptipin

			BSD 3-Clause License

			Copyright (c) 2022, Satyarth Arora, Teaching Assistant, University of Miami
			All rights reserved.

			Redistribution and use in source and binary forms, with or without
			modification, are permitted provided that the following conditions are met:

			1. Redistributions of source code must retain the above copyright notice, this
			   list of conditions and the following disclaimer.

			2. Redistributions in binary form must reproduce the above copyright notice,
			   this list of conditions and the following disclaimer in the documentation
			   and/or other materials provided with the distribution.

			3. Neither the name of the copyright holder nor the names of its
			   contributors may be used to endorse or promote products derived from
			   this software without specific prior written permission.

			THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
			AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
			IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
			DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
			FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
			DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
			SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
			CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
			OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
			OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#include <vector>
#include <math.h>
#include "plugincore.h"
#include "fxobjects.h"
namespace AuxPort
{
/*===================================================================================*/
/*
	[Class] Abstraction over AudioFilter class in FXObjects [Don't Mess with it]
*/
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
		bufferType process(const bufferType& frame,const int& shouldProcess = 0)
		{
			if (shouldProcess)
				return filter.processAudioSample(frame);
			else
				return frame;
		}
		~Filter() = default;
	private:
		AudioFilter filter;
		AudioFilterParameters filterParameters;

	};

/*===================================================================================*/
/*
	[Struct] Simple Struct for handling two channel Frames [DON'T MESS WITH IT]
*/
	template<class bufferType>
	struct Frame
	{
		bufferType left;
		bufferType right;
	
	};

	template<class bufferType, class effectType>
	class Effect
	{
	public:
/*===================================================================================*/
/*
	[Constructor] Safely Allocates the memory
*/
		Effect<bufferType, effectType>() = default;
/*===================================================================================*/
/*
	[Destructor] Safely Deallocates the memory
*/
		~Effect<bufferType, effectType>() = default;
/*===================================================================================*/
/*
	[Copy Constructor] Safely Copies memory from one Effect Object to another
*/
		Effect<bufferType, effectType>(const Effect<bufferType, effectType>& kernel) = default;
/*===================================================================================*/
/*
	[Function] Set your Control Addresses (DONT MESS WITH IT)
*/
		void push(void* parameterAddress, const boundVariableType& dataType,int controlNumber)
		{
			_controls.push_back({parameterAddress,dataType,controlNumber});
		}
/*===================================================================================*/
/*
	[Function] Use this to update your FX objects before the processing block
*/
		void prepareToPlay(const bufferType& sampleRate)
		{
			/*
				Update Internal Parameters of your FX Objects here
			*/
			lpf1.setFilterType(filterAlgorithm::kButterLPF2);
			lpf1.setParameters(getControl(controlID::LPF1_FC), getControl(controlID::LPF1_Q), 1);

			lpf2.setFilterType(filterAlgorithm::kLPF2);
			lpf2.setParameters(getControl(controlID::LPF2_FC), getControl(controlID::LPF2_Q), 1);

			hpf1.setFilterType(filterAlgorithm::kButterHPF2);
			hpf1.setParameters(getControl(controlID::HPF1_FC), getControl(controlID::HPF1_Q), 1);

			hpf2.setFilterType(filterAlgorithm::kHPF2);
			hpf2.setParameters(getControl(controlID::HPF2_FC), getControl(controlID::HPF2_Q), 1);

			bpf1.setFilterType(filterAlgorithm::kBPF2);
			bpf1.setParameters(getControl(controlID::BPF1_FC), getControl(controlID::BPF1_Q),1);
			
		}
/*===================================================================================*/
/*
	[Function] Implement your Frame DSP Logic here
*/
		void run(Frame<bufferType>& frame)
		{	
			/*===================================================================================*/
			/*
				Making a copy of the input Frame (Dont Mess with it)
			*/
			/*===================================================================================*/

			bufferType leftChannel = frame.left;
			bufferType rightChannel = frame.right;
			/*===================================================================================*/
			/*===================================================================================*/
			/*
				Write DSP Algorithm here
			*/
			/*===================================================================================*/
			/*
				Start
			*/
			bufferType leftClean = frame.left;
			bufferType rightClean = frame.right;

			/*===================================================================================*/
			/*===================================================================================*/


			bufferType LowPass1 = getControl(controlID::LPF1_Channel) == 0 ? leftChannel : rightChannel;
			bufferType LowPass2 = getControl(controlID::LPF2_Channel) == 0 ? leftChannel : rightChannel;
			bufferType HighPass1 = getControl(controlID::HPF2_Channel) == 0 ? leftChannel : rightChannel;
			bufferType HighPass2 = getControl(controlID::HPF2_Channel) == 0 ? leftChannel : rightChannel;
			/*===================================================================================*/
			/*===================================================================================*/
			LowPass1 = getControl(controlID::LPF1_Mix) * lpf1.process(LowPass1,getControl(controlID::LPF1_Switch));
			LowPass2 = getControl(controlID::LPF2_Mix) * lpf2.process(LowPass2,getControl(controlID::LPF2_Switch));
			HighPass1 = getControl(controlID::HPF1_Mix) * hpf1.process(HighPass1,getControl(controlID::HPF1_Switch));
			HighPass2 = getControl(controlID::HPF2_Mix) * hpf2.process(HighPass2,getControl(controlID::HPF2_Switch));
			/*===================================================================================*/
			/*===================================================================================*/
			
			bufferType LowPass = getControl(controlID::LPFMix) * (LowPass1 + LowPass2);
			HighPass1 *= getControl(controlID::HPFMix);
			HighPass2 *= getControl(controlID::HPFMix);

			bufferType monoDistort = LowPass;
			FX<float, float>::DCOffset(monoDistort, getControl(controlID::DC), getControl(controlID::DCMix), getControl(controlID::DCSwitch));
			FX<float, float>::ZeroCrossing(monoDistort, getControl(controlID::ZC), getControl(controlID::ZCMix), getControl(controlID::ZCSwitch));
			FX<float, float>::TanH(monoDistort, getControl(controlID::TanHDrive), getControl(controlID::TanHMix), getControl(controlID::TanHSwitch));
			FX<float, float>::ATan2(monoDistort, getControl(controlID::ATan2Drive), getControl(controlID::ATan2Mix), getControl(controlID::ATan2Switch));
			FX<float, float>::ATan(monoDistort, getControl(controlID::ATanMix), getControl(controlID::ATanMix), getControl(controlID::ATanSwitch));
			FX<float, float>::rectify(monoDistort, getControl(controlID::WRMix), getControl(controlID::WaveRectifier));

			monoDistort = getControl(controlID::BPF1_Mix) * bpf1.process(monoDistort, getControl(controlID::BPF1_Switch));
			/*
				End
			*/
			/*===================================================================================*/
			/*===================================================================================*/
			/*
				Save your processed Samples back to the Frame (Dont Mess with it)
			*/
			/*===================================================================================*/
			bufferType leftEffect = getControl(controlID::MasterDistortion) * (monoDistort + HighPass1);
			bufferType rightEffect = getControl(controlID::MasterDistortion) * (monoDistort + HighPass2);
			leftClean *= getControl(controlID::MasterClean);
			rightClean *= getControl(controlID::MasterClean);
			frame.left = leftEffect + leftClean;
			frame.right = rightEffect + rightClean;
		}
	private:
		Filter<float, float> lpf1;
		Filter<float, float> lpf2;
		Filter<float, float> hpf1;
		Filter<float, float> hpf2;
		Filter<float, float> bpf1;
/*===================================================================================*/
/*
	[Function] Gets the Control from our nice dandy vector of pointers (DONT MESS WITH IT)
*/
		effectType getControl(const int& i)
		{
			Parameters* para;
			for (size_t j = 0; j < _controls.size(); j++)
			{
				para = &_controls[j];
				if (para->controlNumber == i)
				{
					if (para->_dataType == boundVariableType::kFloat)
						return *static_cast<float*>(para->_parameterAddress);
					if (para->_dataType == boundVariableType::kDouble)
						return *static_cast<double*>(para->_parameterAddress);
					if (para->_dataType == boundVariableType::kInt)
						return *static_cast<int*>(para->_parameterAddress);
					if (para->_dataType == boundVariableType::kUInt)
						return *static_cast<uint32_t*>(para->_parameterAddress);
				}
			}
		}
/*===================================================================================*/
/*
	[Function] Use this function to Update your meters
*/
		void setControlValue(const double& newValue,const int& i)
		{
			Parameters* para;
			for (size_t j = 0; j < _controls.size(); j++)
			{
				para = &_controls[j];
				if (para->controlNumber == i)
				{
					if (para->_dataType == boundVariableType::kFloat)
						*static_cast<float*>(para->_parameterAddress) = static_cast<float>(newValue);
					else if (para->_dataType == boundVariableType::kDouble)
						*static_cast<double*>(para->_parameterAddress) = newValue;
					else if (para->_dataType == boundVariableType::kInt)
						*static_cast<int*>(para->_parameterAddress) = static_cast<int>(newValue);
					else if (para->_dataType == boundVariableType::kUInt)
						*static_cast<uint32_t*>(para->_parameterAddress) = static_cast<uint32_t>(newValue);
					break;
				}

			}		
		}

		struct Parameters
		{
			void* _parameterAddress;
			boundVariableType _dataType;
			int controlNumber;
		};
		std::vector<Parameters> _controls;
		
	};


	template<class bufferType,class effectType>
	class FX
	{
	public:
		static void DCOffset(bufferType& frame, const effectType& DC, const effectType& mix, const int& state)
		{
			if (state)
			{
				frame += DC;
				frame *= mix;
			}
		}

		static void ZeroCrossing(bufferType& frame, const effectType& ZC, const effectType& mix, const int& state)
		{
			if (state)
			{
				if (abs(frame) < ZC)
					frame = 0;
				frame *= mix;
			}
		}

		static void TanH(bufferType& frame, const effectType& drive, const effectType& mix, const int& state)
		{
			if (state)
			{
				frame = tanh(drive * frame);
				frame *= mix;
			}
		}

		static void ATanH(bufferType& frame, const effectType& drive, const effectType& mix, const int& state)
		{
			if (state)
			{
				frame = atanh(drive * frame);
				frame *= mix;
			}
		}

		static void ATan2(bufferType& frame, const effectType& drive, const effectType& mix, const int& state)
		{
			if (state)
			{
				frame = (2 / kPi) * atan(drive * frame);
				frame *= mix;
			}
		}

		static void ATan(bufferType& frame, const effectType& drive, const effectType& mix, const int& state)
		{
			if (state)
			{
				frame = atan(drive * frame);
				frame *= mix;
			}
		}

		static void rectify(bufferType& frame, const effectType& mix, const int& state)
		{
			if (state == 1)
			{
				if (frame < 0)
					frame = 0;
				frame *= mix;
			}
			if (state == 2)
			{
				frame = abs(frame);
				frame *= mix;
			}
			
		}
	};

	

}
#endif
