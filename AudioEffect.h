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
#include "FX.h"
namespace AuxPort
{


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
			lowPass.setFilterType(filterAlgorithm::kButterLPF2);
			lowPass.setParameters(getControl(controlID::lowPassFC), getControl(controlID::lowPass_Q), getControl(controlID::lpfBoost));


			highPass.setFilterType(filterAlgorithm::kButterHPF2);
			highPass.setParameters(getControl(controlID::highPassFC), getControl(controlID::highPassQ), getControl(controlID::hpfBoost));

			bandPass.setFilterType(filterAlgorithm::kBPF2);
			bandPass.setParameters(getControl(controlID::bandPassFC), getControl(controlID::bandPassQ), getControl(controlID::bandPassBoost));
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
			FX<float>::PreGain(leftChannel, getControl(controlID::preGain));
			FX<float>::PreGain(rightChannel, getControl(controlID::preGain));

			bufferType lowPassLeft = lowPass.process(leftChannel);
			bufferType lowPassRight = lowPass.process(rightChannel);

			bufferType monoLowPass = FX<float>::stereoToMono(lowPassLeft, lowPassRight);

			monoLowPass = fullWave.process(monoLowPass, getControl(controlID::fullWaveSwitch));
			monoLowPass = bandPass.process(monoLowPass);

			bufferType highPassLeft = highPass.process(leftChannel);
			bufferType highPassRight = highPass.process(rightChannel);

			


			
			bufferType sum = (getControl(controlID::A1)*FX<float>::accumulate({ highPassRight,monoLowPass })) + 
							 (getControl(controlID::A2)*FX<float>::accumulate({ monoLowPass,highPassLeft }));

			sum = FX<float>::accumulate({
											getControl(controlID::masterD) * sum,
											getControl(controlID::masterC) * FX<float>::stereoToMono(frame.left,frame.right)
										});

			leftChannel = sum;
			rightChannel = sum;
			/*
				End
			*/
			/*===================================================================================*/
			/*===================================================================================*/
			/*
				Save your processed Samples back to the Frame (Dont Mess with it)
			*/
			/*===================================================================================*/
			frame.left = leftChannel;
			frame.right = rightChannel;
		}
	private:
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

		Filter<float, float> lowPass;
		Filter<float, float> highPass;
		Filter<float, float> bandPass;
		FullWave<float> fullWave;
	};




	

}
#endif