// -----------------------------------------------------------------------------
//    ASPiK Plugin Kernel File:  plugincore.cpp
//
/**
    \file   plugincore.cpp
    \author Will Pirkle
    \date   17-September-2018
    \brief  Implementation file for PluginCore object
    		- http://www.aspikplugins.com
    		- http://www.willpirkle.com
*/
// -----------------------------------------------------------------------------
#include "plugincore.h"
#include "plugindescription.h"
#pragma warning (disable : 4244)

/**
\brief PluginCore constructor is launching pad for object initialization

Operations:
- initialize the plugin description (strings, codes, numbers, see initPluginDescriptors())
- setup the plugin's audio I/O channel support
- create the PluginParameter objects that represent the plugin parameters (see FX book if needed)
- create the presets
*/
PluginCore::PluginCore()
{
    // --- describe the plugin; call the helper to init the static parts you setup in plugindescription.h
    initPluginDescriptors();

    // --- default I/O combinations
	// --- for FX plugins
	if (getPluginType() == kFXPlugin)
	{
		addSupportedIOCombination({ kCFMono, kCFMono });
		addSupportedIOCombination({ kCFMono, kCFStereo });
		addSupportedIOCombination({ kCFStereo, kCFStereo });
	}
	else // --- synth plugins have no input, only output
	{
		addSupportedIOCombination({ kCFNone, kCFMono });
		addSupportedIOCombination({ kCFNone, kCFStereo });
	}

	// --- for sidechaining, we support mono and stereo inputs; auxOutputs reserved for future use
	addSupportedAuxIOCombination({ kCFMono, kCFNone });
	addSupportedAuxIOCombination({ kCFStereo, kCFNone });

	// --- create the parameters
    initPluginParameters();

    // --- create the presets
    initPluginPresets();
}

/**
\brief initialize object for a new run of audio; called just before audio streams

Operation:
- store sample rate and bit depth on audioProcDescriptor - this information is globally available to all core functions
- reset your member objects here

\param resetInfo structure of information about current audio format

\return true if operation succeeds, false otherwise
*/
bool PluginCore::reset(ResetInfo& resetInfo)
{
    // --- save for audio processing
    audioProcDescriptor.sampleRate = resetInfo.sampleRate;
    audioProcDescriptor.bitDepth = resetInfo.bitDepth;

    // --- other reset inits
    return PluginBase::reset(resetInfo);
}

/**
\brief one-time initialize function called after object creation and before the first reset( ) call

Operation:
- saves structure for the plugin to use; you can also load WAV files or state information here
*/
bool PluginCore::initialize(PluginInfo& pluginInfo)
{
	// --- add one-time init stuff here

	return true;
}

/**
\brief do anything needed prior to arrival of audio buffers

Operation:
- syncInBoundVariables when preProcessAudioBuffers is called, it is *guaranteed* that all GUI control change information
  has been applied to plugin parameters; this binds parameter changes to your underlying variables
- NOTE: postUpdatePluginParameter( ) will be called for all bound variables that are acutally updated; if you need to process
  them individually, do so in that function
- use this function to bulk-transfer the bound variable data into your plugin's member object variables

\param processInfo structure of information about *buffer* processing

\return true if operation succeeds, false otherwise
*/
bool PluginCore::preProcessAudioBuffers(ProcessBufferInfo& processInfo)
{
    // --- sync internal variables to GUI parameters; you can also do this manually if you don't
    //     want to use the auto-variable-binding
    syncInBoundVariables();

    return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	--- FRAME PROCESSING FUNCTION --- //
/*
	This is where you do your DSP processing on a PER-FRAME basis; this means that you are
	processing one sample per channel at a time, and is the default mechanism in ASPiK.

	You will get better performance is you process buffers or blocks instead, HOWEVER for
	some kinds of processing (e.g. ping pong delay) you must process on a per-sample
	basis anyway. This is also easier to understand for most newbies.

	NOTE:
	You can enable and disable frame/buffer procssing in the constructor of this object:

	// --- to process audio frames call:
	processAudioByFrames();

	// --- to process complete DAW buffers call:
	processAudioByBlocks(WANT_WHOLE_BUFFER);

	// --- to process sub-blocks of the incoming DAW buffer in 64 sample blocks call:
	processAudioByBlocks(DEFAULT_AUDIO_BLOCK_SIZE);

	// --- to process sub-blocks of size N, call:
	processAudioByBlocks(N);
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
\brief frame-processing method

Operation:
- decode the plugin type - for synth plugins, fill in the rendering code; for FX plugins, delete the if(synth) portion and add your processing code
- note that MIDI events are fired for each sample interval so that MIDI is tightly sunk with audio
- doSampleAccurateParameterUpdates will perform per-sample interval smoothing

\param processFrameInfo structure of information about *frame* processing

\return true if operation succeeds, false otherwise
*/
bool PluginCore::processAudioFrame(ProcessFrameInfo& processFrameInfo)
{
    // --- fire any MIDI events for this sample interval
    processFrameInfo.midiEventQueue->fireMidiEvents(processFrameInfo.currentFrame);

	// --- do per-frame smoothing
	doParameterSmoothing();

	// --- call your GUI update/cooking function here, now that smoothing has occurred
	//
	//     NOTE:
	//     updateParameters is the name used in Will Pirkle's books for the GUI update function
	//     you may name it what you like - this is where GUI control values are cooked
	//     for the DSP algorithm at hand
	// updateParameters();

    // --- decode the channelIOConfiguration and process accordingly
    //
	// --- Synth Plugin:
	// --- Synth Plugin --- remove for FX plugins
	if (getPluginType() == kSynthPlugin)
	{
		// --- output silence: change this with your signal render code
		processFrameInfo.audioOutputFrame[0] = 0.0;
		if (processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
			processFrameInfo.audioOutputFrame[1] = 0.0;

		return true;	/// processed
	}

    // --- FX Plugin:
    if(processFrameInfo.channelIOConfig.inputChannelFormat == kCFMono &&
       processFrameInfo.channelIOConfig.outputChannelFormat == kCFMono)
    {
		// --- pass through code: change this with your signal processing
		audioFrame.left = processFrameInfo.audioInputFrame[0];
		audioFrame.right = processFrameInfo.audioInputFrame[0];
		kernel.run(audioFrame);
		processFrameInfo.audioOutputFrame[0] = audioFrame.left;

        return true; /// processed
    }

    // --- Mono-In/Stereo-Out
    else if(processFrameInfo.channelIOConfig.inputChannelFormat == kCFMono &&
       processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
    {
		// --- pass through code: change this with your signal processing
		audioFrame.left = processFrameInfo.audioInputFrame[0];
		audioFrame.right = processFrameInfo.audioInputFrame[0];
		kernel.run(audioFrame);
        processFrameInfo.audioOutputFrame[0] = audioFrame.left;
        processFrameInfo.audioOutputFrame[1] = audioFrame.right;

        return true; /// processed
    }

    // --- Stereo-In/Stereo-Out
    else if(processFrameInfo.channelIOConfig.inputChannelFormat == kCFStereo &&
       processFrameInfo.channelIOConfig.outputChannelFormat == kCFStereo)
    {
		// --- pass through code: change this with your signal processing
		audioFrame.left = processFrameInfo.audioInputFrame[0];
		audioFrame.right = processFrameInfo.audioInputFrame[1];
		kernel.run(audioFrame);
        processFrameInfo.audioOutputFrame[0] = audioFrame.left;
        processFrameInfo.audioOutputFrame[1] = audioFrame.right;

        return true; /// processed
    }

    return false; /// NOT processed
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	--- BLOCK/BUFFER PRE-PROCESSING FUNCTION --- //
//      Only used when BLOCKS or BUFFERS are processed
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
\brief pre-process the audio block

Operation:
- fire MIDI events for the audio block; see processMIDIEvent( ) for the code that loads
  the vector on the ProcessBlockInfo structure

\param IMidiEventQueue ASPIK event queue of MIDI events for the entire buffer; this
       function only fires the MIDI events for this audio block

\return true if operation succeeds, false otherwise
*/
bool PluginCore::preProcessAudioBlock(IMidiEventQueue* midiEventQueue)
{
	// --- pre-process the block
	processBlockInfo.clearMidiEvents();

	// --- sample accurate parameter updates
	for (uint32_t sample = processBlockInfo.blockStartIndex;
		sample < processBlockInfo.blockStartIndex + processBlockInfo.blockSize;
		sample++)
	{
		// --- the MIDI handler will load up the vector in processBlockInfo
		if (midiEventQueue)
			midiEventQueue->fireMidiEvents(sample);
	}

	// --- this will do parameter smoothing ONLY ONCE AT THE TOP OF THE BLOCK PROCESSING
	//
	// --- to perform per-sample parameter smoothing, move this line of code, AND your updating
	//     functions (see updateParameters( ) in comment below) into the for( ) loop above
	//     NOTE: smoothing only once per block usually SAVES CPU cycles
	//           smoothing once per sample period usually EATS CPU cycles, potentially unnecessarily
	doParameterSmoothing();

	// --- call your GUI update/cooking function here, now that smoothing has occurred
	//
	//     NOTE:
	//     updateParameters is the name used in Will Pirkle's books for the GUI update function
	//     you may name it what you like - this is where GUI control values are cooked
	//     for the DSP algorithm at hand
	//     NOTE: updating (cooking) only once per block usually SAVES CPU cycles
	//           updating (cooking) once per sample period usually EATS CPU cycles, potentially unnecessarily
	// updateParameters();

	return true;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//	--- BLOCK/BUFFER PROCESSING FUNCTION --- //
/*
This is where you do your DSP processing on a PER-BLOCK basis; this means that you are
processing entire blocks at a time -- each audio channel has its own block to process.

A BUFFER is simply a single block of audio that is the size of the incoming buffer from the
DAW. A BLOCK is a sub-block portion of that buffer.

In the event that the incoming buffer is SMALLER than your requested audio block, the
entire buffer will be sent to this block processing function. This is also true when your
block size is not a divisor of the actual incoming buffer, OR when an incoming buffer
is partially filled (which is rare, but may happen under certain circumstances), resulting
in a "partial block" of data that is smaller than your requested block size.

NOTE:
You can enable and disable frame/buffer procssing in the constructor of this object:

// --- to process audio frames call:
processAudioByFrames();

// --- to process complete DAW buffers call:
processAudioByBlocks(WANT_WHOLE_BUFFER);

// --- to process sub-blocks of the incoming DAW buffer in 64 sample blocks call:
processAudioByBlocks(DEFAULT_AUDIO_BLOCK_SIZE);

// --- to process sub-blocks of size N, call:
processAudioByBlocks(N);
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/**
\brief block or buffer-processing method

Operation:
- process one block of audio data; see example functions for template code
- renderSynthSilence: render a block of 0.0 values (synth, silence when no notes are rendered)
- renderFXPassThrough: pass audio from input to output (FX)

\param processBlockInfo structure of information about *block* processing

\return true if operation succeeds, false otherwise
*/
bool PluginCore::processAudioBlock(ProcessBlockInfo& processBlockInfo)
{
	// --- FX or Synth Render
	//     call your block processing function here
	// --- Synth
	if (getPluginType() == kSynthPlugin)
		renderSynthSilence(processBlockInfo);

	// --- or FX
	else if (getPluginType() == kFXPlugin)
		renderFXPassThrough(processBlockInfo);

	return true;
}


/**
\brief
renders a block of silence (all 0.0 values) as an example
your synth code would render the synth using the MIDI messages and output buffers

Operation:
- process all MIDI events for the block
- perform render into block's audio buffers

\param blockInfo structure of information about *block* processing
\return true if operation succeeds, false otherwise
*/
bool PluginCore::renderSynthSilence(ProcessBlockInfo& blockInfo)
{
	// --- process all MIDI events in this block (same as SynthLab)
	uint32_t midiEvents = blockInfo.getMidiEventCount();
	for (uint32_t i = 0; i < midiEvents; i++)
	{
		// --- get the event
		midiEvent event = *blockInfo.getMidiEvent(i);

		// --- do something with it...
		// myMIDIMessageHandler(event); // <-- you write this
	}

	// --- render a block of audio; here it is silence but in your synth
	//     it will likely be dependent on the MIDI processing you just did above
	for (uint32_t sample = blockInfo.blockStartIndex, i = 0;
		 sample < blockInfo.blockStartIndex + blockInfo.blockSize;
		 sample++, i++)
	{
		// --- write outputs
		for (uint32_t channel = 0; channel < blockInfo.numAudioOutChannels; channel++)
		{
			// --- silence (or, your synthesized block of samples)
			blockInfo.outputs[channel][sample] = 0.0;
		}
	}
	return true;
}

/**
\brief
Renders pass-through code as an example; replace with meaningful DSP for audio goodness

Operation:
- loop over samples in block
- write inputs to outputs, per channel basis

\param blockInfo structure of information about *block* processing
\return true if operation succeeds, false otherwise
*/
bool PluginCore::renderFXPassThrough(ProcessBlockInfo& blockInfo)
{
	// --- block processing -- write to outputs
	for (uint32_t sample = blockInfo.blockStartIndex, i = 0;
		sample < blockInfo.blockStartIndex + blockInfo.blockSize;
		sample++, i++)
	{
		// --- handles multiple channels, but up to you for bookkeeping
		for (uint32_t channel = 0; channel < blockInfo.numAudioOutChannels; channel++)
		{
			// --- pass through code, or your processed FX version
			blockInfo.outputs[channel][sample] = blockInfo.inputs[channel][sample];
		}
	}
	return true;
}


/**
\brief do anything needed prior to arrival of audio buffers

Operation:
- updateOutBoundVariables sends metering data to the GUI meters

\param processInfo structure of information about *buffer* processing

\return true if operation succeeds, false otherwise
*/
bool PluginCore::postProcessAudioBuffers(ProcessBufferInfo& processInfo)
{
	// --- update outbound variables; currently this is meter data only, but could be extended
	//     in the future
	updateOutBoundVariables();

    return true;
}

/**
\brief update the PluginParameter's value based on GUI control, preset, or data smoothing (thread-safe)

Operation:
- update the parameter's value (with smoothing this initiates another smoothing process)
- call postUpdatePluginParameter to do any further processing

\param controlID the control ID value of the parameter being updated
\param controlValue the new control value
\param paramInfo structure of information about why this value is being udpated (e.g as a result of a preset being loaded vs. the top of a buffer process cycle)

\return true if operation succeeds, false otherwise
*/
bool PluginCore::updatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo)
{
    // --- use base class helper
    setPIParamValue(controlID, controlValue);

    // --- do any post-processing
    postUpdatePluginParameter(controlID, controlValue, paramInfo);

    return true; /// handled
}

/**
\brief update the PluginParameter's value based on *normlaized* GUI control, preset, or data smoothing (thread-safe)

Operation:
- update the parameter's value (with smoothing this initiates another smoothing process)
- call postUpdatePluginParameter to do any further processing

\param controlID the control ID value of the parameter being updated
\param normalizedValue the new control value in normalized form
\param paramInfo structure of information about why this value is being udpated (e.g as a result of a preset being loaded vs. the top of a buffer process cycle)

\return true if operation succeeds, false otherwise
*/
bool PluginCore::updatePluginParameterNormalized(int32_t controlID, double normalizedValue, ParameterUpdateInfo& paramInfo)
{
	// --- use base class helper, returns actual value
	double controlValue = setPIParamValueNormalized(controlID, normalizedValue, paramInfo.applyTaper);

	// --- do any post-processing
	postUpdatePluginParameter(controlID, controlValue, paramInfo);
	kernel.prepareToPlay(audioProcDescriptor.sampleRate);

	return true; /// handled
}

/**
\brief perform any operations after the plugin parameter has been updated; this is one paradigm for
	   transferring control information into vital plugin variables or member objects. If you use this
	   method you can decode the control ID and then do any cooking that is needed. NOTE: do not
	   overwrite bound variables here - this is ONLY for any extra cooking that is required to convert
	   the GUI data to meaninful coefficients or other specific modifiers.

\param controlID the control ID value of the parameter being updated
\param controlValue the new control value
\param paramInfo structure of information about why this value is being udpated (e.g as a result of a preset being loaded vs. the top of a buffer process cycle)

\return true if operation succeeds, false otherwise
*/
bool PluginCore::postUpdatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo)
{
    // --- now do any post update cooking; be careful with VST Sample Accurate automation
    //     If enabled, then make sure the cooking functions are short and efficient otherwise disable it
    //     for the Parameter involved
    /*switch(controlID)
    {
        case 0:
        {
            return true;    /// handled
        }

        default:
            return false;   /// not handled
    }*/

    return false;
}

/**
\brief has nothing to do with actual variable or updated variable (binding)

CAUTION:
- DO NOT update underlying variables here - this is only for sending GUI updates or letting you
  know that a parameter was changed; it should not change the state of your plugin.

WARNING:
- THIS IS NOT THE PREFERRED WAY TO LINK OR COMBINE CONTROLS TOGETHER. THE PROPER METHOD IS
  TO USE A CUSTOM SUB-CONTROLLER THAT IS PART OF THE GUI OBJECT AND CODE.
  SEE http://www.willpirkle.com for more information

\param controlID the control ID value of the parameter being updated
\param actualValue the new control value

\return true if operation succeeds, false otherwise
*/
bool PluginCore::guiParameterChanged(int32_t controlID, double actualValue)
{
	/*
	switch (controlID)
	{
		case controlID::<your control here>
		{

			return true; // handled
		}

		default:
			break;
	}*/

	return false; /// not handled
}

/**
\brief For Custom View and Custom Sub-Controller Operations

NOTES:
- this is for advanced users only to implement custom view and custom sub-controllers
- see the SDK for examples of use

\param messageInfo a structure containing information about the incoming message

\return true if operation succeeds, false otherwise
*/
bool PluginCore::processMessage(MessageInfo& messageInfo)
{
	// --- decode message
	switch (messageInfo.message)
	{
		// --- add customization appearance here
	case PLUGINGUI_DIDOPEN:
	{
		return false;
	}

	// --- NULL pointers so that we don't accidentally use them
	case PLUGINGUI_WILLCLOSE:
	{
		return false;
	}

	// --- update view; this will only be called if the GUI is actually open
	case PLUGINGUI_TIMERPING:
	{
		return false;
	}

	// --- register the custom view, grab the ICustomView interface
	case PLUGINGUI_REGISTER_CUSTOMVIEW:
	{

		return false;
	}

	case PLUGINGUI_REGISTER_SUBCONTROLLER:
	case PLUGINGUI_QUERY_HASUSERCUSTOM:
	case PLUGINGUI_USER_CUSTOMOPEN:
	case PLUGINGUI_USER_CUSTOMCLOSE:
	case PLUGINGUI_EXTERNAL_SET_NORMVALUE:
	case PLUGINGUI_EXTERNAL_SET_ACTUALVALUE:
	{

		return false;
	}

	default:
		break;
	}

	return false; /// not handled
}


/**
\brief process a MIDI event

NOTES:
- MIDI events are 100% sample accurate; this function will be called repeatedly for every MIDI message
- see the SDK for examples of use

\param event a structure containing the MIDI event data

\return true if operation succeeds, false otherwise
*/
bool PluginCore::processMIDIEvent(midiEvent& event)
{
	// --- IF PROCESSING AUDIO BLOCKS: push into vector for block processing
	if (!pluginDescriptor.processFrames)
	{
		processBlockInfo.pushMidiEvent(event);
		return true;
	}

	// --- IF PROCESSING AUDIO FRAMES: decode AND service this MIDI event here
	//     for sample accurate MIDI
	// myMIDIMessageHandler(event); // <-- you write this

	return true;
}

/**
\brief (for future use)

NOTES:
- MIDI events are 100% sample accurate; this function will be called repeatedly for every MIDI message
- see the SDK for examples of use

\param vectorJoysickData a structure containing joystick data

\return true if operation succeeds, false otherwise
*/
bool PluginCore::setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData)
{
	return true;
}

/**
\brief create all of your plugin parameters here

\return true if parameters were created, false if they already existed
*/
bool PluginCore::initPluginParameters()
{
	if (pluginParameterMap.size() > 0)
		return false;

	// --- Add your plugin parameter instantiation code bewtween these hex codes
	// **--0xDEA7--**


	// --- Declaration of Plugin Parameter Objects 
	PluginParameter* piParam = nullptr;

	// --- discrete control: LPF1_Switch
	piParam = new PluginParameter(controlID::LPF1_Switch, "LPF1_Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&LPF1_Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: LPF1_FC
	piParam = new PluginParameter(controlID::LPF1_FC, "LPF1_FC", "Units", controlVariableType::kFloat, 20.000000, 1000.000000, 500.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF1_FC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: LPF1_Q
	piParam = new PluginParameter(controlID::LPF1_Q, "LPF1_Q", "Units", controlVariableType::kFloat, 0.250000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF1_Q, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: LPF2_Switch
	piParam = new PluginParameter(controlID::LPF2_Switch, "LPF2_Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&LPF2_Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HPF1_Switch
	piParam = new PluginParameter(controlID::HPF1_Switch, "HPF1_Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&HPF1_Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HPF2_Switch
	piParam = new PluginParameter(controlID::HPF2_Switch, "HPF2_Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&HPF2_Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: HPF1_FC
	piParam = new PluginParameter(controlID::HPF1_FC, "HPF1_FC", "Units", controlVariableType::kFloat, 20.000000, 1000.000000, 500.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF1_FC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPF1_Q
	piParam = new PluginParameter(controlID::HPF1_Q, "HPF1_Q", "Units", controlVariableType::kFloat, 0.250000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF1_Q, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: LPF2_FC
	piParam = new PluginParameter(controlID::LPF2_FC, "LPF2_FC", "Units", controlVariableType::kFloat, 20.000000, 1000.000000, 500.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF2_FC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: LPF2_Q
	piParam = new PluginParameter(controlID::LPF2_Q, "LPF2_Q", "Units", controlVariableType::kFloat, 0.250000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF2_Q, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPF2_Q
	piParam = new PluginParameter(controlID::HPF2_Q, "HPF2_Q", "Units", controlVariableType::kFloat, 0.250000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF2_Q, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPF2_FC
	piParam = new PluginParameter(controlID::HPF2_FC, "HPF2_FC", "Units", controlVariableType::kFloat, 20.000000, 1000.000000, 500.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF2_FC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: LPF1_Channel
	piParam = new PluginParameter(controlID::LPF1_Channel, "LPF1_Channel", "Left,Right", "Left");
	piParam->setBoundVariable(&LPF1_Channel, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HPF1_Channel
	piParam = new PluginParameter(controlID::HPF1_Channel, "HPF1_Channel", "Left,Right", "Left");
	piParam->setBoundVariable(&HPF1_Channel, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: LPF2_Channel
	piParam = new PluginParameter(controlID::LPF2_Channel, "LPF2_Channel", "Left,Right", "Left");
	piParam->setBoundVariable(&LPF2_Channel, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: HPF2_Channel
	piParam = new PluginParameter(controlID::HPF2_Channel, "HPF2_Channel", "Left,Right", "Left");
	piParam->setBoundVariable(&HPF2_Channel, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: LPF1_Mix
	piParam = new PluginParameter(controlID::LPF1_Mix, "LPF1_Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF1_Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: LPF2_Mix
	piParam = new PluginParameter(controlID::LPF2_Mix, "LPF2_Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPF2_Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPF2_Mix
	piParam = new PluginParameter(controlID::HPF2_Mix, "HPF2_Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF2_Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPF1_Mix
	piParam = new PluginParameter(controlID::HPF1_Mix, "HPF1_Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPF1_Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: LPFMix
	piParam = new PluginParameter(controlID::LPFMix, "LPFMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&LPFMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: HPFMix
	piParam = new PluginParameter(controlID::HPFMix, "HPFMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&HPFMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: DCSwitch
	piParam = new PluginParameter(controlID::DCSwitch, "DCSwitch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&DCSwitch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: ZCSwitch
	piParam = new PluginParameter(controlID::ZCSwitch, "ZCSwitch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&ZCSwitch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: TanHSwitch
	piParam = new PluginParameter(controlID::TanHSwitch, "TanHSwitch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&TanHSwitch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- discrete control: ATan2Switch
	piParam = new PluginParameter(controlID::ATan2Switch, "ATan2Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&ATan2Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: DC
	piParam = new PluginParameter(controlID::DC, "DC", "Units", controlVariableType::kFloat, -1.000000, 1.000000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&DC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: ZC
	piParam = new PluginParameter(controlID::ZC, "ZC", "Units", controlVariableType::kFloat, 0.000000, 0.250000, 0.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ZC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: TanHDrive
	piParam = new PluginParameter(controlID::TanHDrive, "TanHDrive", "Units", controlVariableType::kFloat, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&TanHDrive, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: ATan2Drive
	piParam = new PluginParameter(controlID::ATan2Drive, "ATan2Drive", "Units", controlVariableType::kFloat, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ATan2Drive, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: DCMix
	piParam = new PluginParameter(controlID::DCMix, "DCMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&DCMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: ZCMix
	piParam = new PluginParameter(controlID::ZCMix, "ZCMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ZCMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: TanHMix
	piParam = new PluginParameter(controlID::TanHMix, "TanHMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&TanHMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: ATan2Mix
	piParam = new PluginParameter(controlID::ATan2Mix, "ATan2Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ATan2Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: ATanSwitch
	piParam = new PluginParameter(controlID::ATanSwitch, "ATanSwitch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&ATanSwitch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: ATanDrive
	piParam = new PluginParameter(controlID::ATanDrive, "ATanDrive", "Units", controlVariableType::kFloat, 1.000000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ATanDrive, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: ATanMix
	piParam = new PluginParameter(controlID::ATanMix, "ATanMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&ATanMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: WaveRectifier
	piParam = new PluginParameter(controlID::WaveRectifier, "WaveRectifier", "None,HW,FW", "None");
	piParam->setBoundVariable(&WaveRectifier, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: WRMix
	piParam = new PluginParameter(controlID::WRMix, "WRMix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&WRMix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: MasterClean
	piParam = new PluginParameter(controlID::MasterClean, "MasterClean", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&MasterClean, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: MasterDistortion
	piParam = new PluginParameter(controlID::MasterDistortion, "MasterDistortion", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&MasterDistortion, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- discrete control: BPF1_Switch
	piParam = new PluginParameter(controlID::BPF1_Switch, "BPF1_Switch", "SWITCH OFF,SWITCH ON", "SWITCH OFF");
	piParam->setBoundVariable(&BPF1_Switch, boundVariableType::kInt);
	piParam->setIsDiscreteSwitch(true);
	addPluginParameter(piParam);

	// --- continuous control: BPF1_FC
	piParam = new PluginParameter(controlID::BPF1_FC, "BPF1_FC", "Units", controlVariableType::kFloat, 20.000000, 1000.000000, 500.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&BPF1_FC, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: BPF1_Q
	piParam = new PluginParameter(controlID::BPF1_Q, "BPF1_Q", "Units", controlVariableType::kFloat, 0.250000, 10.000000, 1.000000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&BPF1_Q, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- continuous control: BPF1_Mix
	piParam = new PluginParameter(controlID::BPF1_Mix, "BPF1_Mix", "Units", controlVariableType::kFloat, 0.000000, 1.000000, 0.707000, taper::kLinearTaper);
	piParam->setParameterSmoothing(true);
	piParam->setSmoothingTimeMsec(20.00);
	piParam->setBoundVariable(&BPF1_Mix, boundVariableType::kFloat);
	addPluginParameter(piParam);

	// --- Aux Attributes
	AuxParameterAttribute auxAttribute;

	// --- RAFX GUI attributes
	// --- controlID::LPF1_Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741824);
	setParamAuxAttribute(controlID::LPF1_Switch, auxAttribute);

	// --- controlID::LPF1_FC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483664);
	setParamAuxAttribute(controlID::LPF1_FC, auxAttribute);

	// --- controlID::LPF1_Q
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483662);
	setParamAuxAttribute(controlID::LPF1_Q, auxAttribute);

	// --- controlID::LPF2_Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741824);
	setParamAuxAttribute(controlID::LPF2_Switch, auxAttribute);

	// --- controlID::HPF1_Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741824);
	setParamAuxAttribute(controlID::HPF1_Switch, auxAttribute);

	// --- controlID::HPF2_Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741824);
	setParamAuxAttribute(controlID::HPF2_Switch, auxAttribute);

	// --- controlID::HPF1_FC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483664);
	setParamAuxAttribute(controlID::HPF1_FC, auxAttribute);

	// --- controlID::HPF1_Q
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483662);
	setParamAuxAttribute(controlID::HPF1_Q, auxAttribute);

	// --- controlID::LPF2_FC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483664);
	setParamAuxAttribute(controlID::LPF2_FC, auxAttribute);

	// --- controlID::LPF2_Q
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483662);
	setParamAuxAttribute(controlID::LPF2_Q, auxAttribute);

	// --- controlID::HPF2_Q
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483662);
	setParamAuxAttribute(controlID::HPF2_Q, auxAttribute);

	// --- controlID::HPF2_FC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483664);
	setParamAuxAttribute(controlID::HPF2_FC, auxAttribute);

	// --- controlID::LPF1_Channel
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(805306368);
	setParamAuxAttribute(controlID::LPF1_Channel, auxAttribute);

	// --- controlID::HPF1_Channel
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(805306368);
	setParamAuxAttribute(controlID::HPF1_Channel, auxAttribute);

	// --- controlID::LPF2_Channel
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(805306368);
	setParamAuxAttribute(controlID::LPF2_Channel, auxAttribute);

	// --- controlID::HPF2_Channel
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(805306368);
	setParamAuxAttribute(controlID::HPF2_Channel, auxAttribute);

	// --- controlID::LPF1_Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483703);
	setParamAuxAttribute(controlID::LPF1_Mix, auxAttribute);

	// --- controlID::LPF2_Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483703);
	setParamAuxAttribute(controlID::LPF2_Mix, auxAttribute);

	// --- controlID::HPF2_Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483703);
	setParamAuxAttribute(controlID::HPF2_Mix, auxAttribute);

	// --- controlID::HPF1_Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483703);
	setParamAuxAttribute(controlID::HPF1_Mix, auxAttribute);

	// --- controlID::LPFMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483655);
	setParamAuxAttribute(controlID::LPFMix, auxAttribute);

	// --- controlID::HPFMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483655);
	setParamAuxAttribute(controlID::HPFMix, auxAttribute);

	// --- controlID::DCSwitch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741829);
	setParamAuxAttribute(controlID::DCSwitch, auxAttribute);

	// --- controlID::ZCSwitch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741829);
	setParamAuxAttribute(controlID::ZCSwitch, auxAttribute);

	// --- controlID::TanHSwitch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741829);
	setParamAuxAttribute(controlID::TanHSwitch, auxAttribute);

	// --- controlID::ATan2Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741829);
	setParamAuxAttribute(controlID::ATan2Switch, auxAttribute);

	// --- controlID::DC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483681);
	setParamAuxAttribute(controlID::DC, auxAttribute);

	// --- controlID::ZC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483681);
	setParamAuxAttribute(controlID::ZC, auxAttribute);

	// --- controlID::TanHDrive
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483681);
	setParamAuxAttribute(controlID::TanHDrive, auxAttribute);

	// --- controlID::ATan2Drive
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483681);
	setParamAuxAttribute(controlID::ATan2Drive, auxAttribute);

	// --- controlID::DCMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::DCMix, auxAttribute);

	// --- controlID::ZCMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::ZCMix, auxAttribute);

	// --- controlID::TanHMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::TanHMix, auxAttribute);

	// --- controlID::ATan2Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::ATan2Mix, auxAttribute);

	// --- controlID::ATanSwitch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741829);
	setParamAuxAttribute(controlID::ATanSwitch, auxAttribute);

	// --- controlID::ATanDrive
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483681);
	setParamAuxAttribute(controlID::ATanDrive, auxAttribute);

	// --- controlID::ATanMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::ATanMix, auxAttribute);

	// --- controlID::WaveRectifier
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(805306368);
	setParamAuxAttribute(controlID::WaveRectifier, auxAttribute);

	// --- controlID::WRMix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483692);
	setParamAuxAttribute(controlID::WRMix, auxAttribute);

	// --- controlID::MasterClean
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483704);
	setParamAuxAttribute(controlID::MasterClean, auxAttribute);

	// --- controlID::MasterDistortion
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483704);
	setParamAuxAttribute(controlID::MasterDistortion, auxAttribute);

	// --- controlID::BPF1_Switch
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(1073741824);
	setParamAuxAttribute(controlID::BPF1_Switch, auxAttribute);

	// --- controlID::BPF1_FC
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483664);
	setParamAuxAttribute(controlID::BPF1_FC, auxAttribute);

	// --- controlID::BPF1_Q
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483662);
	setParamAuxAttribute(controlID::BPF1_Q, auxAttribute);

	// --- controlID::BPF1_Mix
	auxAttribute.reset(auxGUIIdentifier::guiControlData);
	auxAttribute.setUintAttribute(2147483703);
	setParamAuxAttribute(controlID::BPF1_Mix, auxAttribute);


	// **--0xEDA5--**
	
	// --- BONUS Parameter
	// --- SCALE_GUI_SIZE
	PluginParameter* piParamBonus = new PluginParameter(SCALE_GUI_SIZE, "Scale GUI", "tiny,small,medium,normal,large,giant", "normal");
	addPluginParameter(piParamBonus);

	kernel.push(&LPF1_Switch, boundVariableType::kInt, controlID::LPF1_Switch);
	kernel.push(&LPF2_Switch, boundVariableType::kInt, controlID::LPF2_Switch);
	kernel.push(&HPF1_Switch, boundVariableType::kInt, controlID::HPF1_Switch);
	kernel.push(&HPF2_Switch, boundVariableType::kInt, controlID::HPF2_Switch);


	kernel.push(&LPF1_FC, boundVariableType::kFloat, controlID::LPF1_FC);
	kernel.push(&LPF2_FC, boundVariableType::kFloat, controlID::LPF2_FC);
	kernel.push(&HPF1_FC, boundVariableType::kFloat, controlID::HPF1_FC);
	kernel.push(&HPF2_FC, boundVariableType::kFloat, controlID::HPF2_FC);


	kernel.push(&LPF1_Q, boundVariableType::kFloat, controlID::LPF1_Q);
	kernel.push(&LPF2_Q, boundVariableType::kFloat, controlID::LPF2_Q);
	kernel.push(&HPF1_Q, boundVariableType::kFloat, controlID::HPF1_Q);
	kernel.push(&HPF2_Q, boundVariableType::kFloat, controlID::HPF2_Q);


	kernel.push(&LPF1_Channel, boundVariableType::kInt, controlID::LPF1_Channel);
	kernel.push(&LPF2_Channel, boundVariableType::kInt, controlID::LPF2_Channel);
	kernel.push(&HPF1_Channel, boundVariableType::kInt, controlID::HPF1_Channel);
	kernel.push(&HPF2_Channel, boundVariableType::kInt, controlID::HPF2_Channel);


	kernel.push(&LPF1_Mix, boundVariableType::kFloat, controlID::LPF1_Mix);
	kernel.push(&LPF2_Mix, boundVariableType::kFloat, controlID::LPF2_Mix);
	kernel.push(&HPF1_Mix, boundVariableType::kFloat, controlID::HPF1_Mix);
	kernel.push(&HPF2_Mix, boundVariableType::kFloat, controlID::HPF2_Mix);

	kernel.push(&LPFMix, boundVariableType::kFloat, controlID::LPFMix);
	kernel.push(&HPFMix, boundVariableType::kFloat, controlID::HPFMix);

	kernel.push(&DCSwitch, boundVariableType::kInt, controlID::DCSwitch);
	kernel.push(&DCMix, boundVariableType::kFloat, controlID::DCMix);
	kernel.push(&DC, boundVariableType::kFloat, controlID::DC);

	kernel.push(&ZCSwitch, boundVariableType::kInt, controlID::ZCSwitch);
	kernel.push(&ZCMix, boundVariableType::kFloat, controlID::ZCMix);
	kernel.push(&ZC, boundVariableType::kFloat, controlID::ZC);

	kernel.push(&TanHSwitch, boundVariableType::kInt, controlID::TanHSwitch);
	kernel.push(&TanHMix, boundVariableType::kFloat, controlID::TanHMix);
	kernel.push(&TanHDrive, boundVariableType::kFloat, controlID::TanHDrive);

	kernel.push(&ATan2Switch, boundVariableType::kInt, controlID::ATan2Switch);
	kernel.push(&ATan2Mix, boundVariableType::kFloat, controlID::ATan2Mix);
	kernel.push(&ATan2Drive, boundVariableType::kFloat, controlID::ATan2Drive);

	kernel.push(&ATanSwitch, boundVariableType::kInt, controlID::ATanSwitch);
	kernel.push(&ATanMix, boundVariableType::kFloat, controlID::ATanMix);
	kernel.push(&ATanDrive, boundVariableType::kFloat, controlID::ATanDrive);

	kernel.push(&WaveRectifier, boundVariableType::kInt, controlID::WaveRectifier);
	kernel.push(&WRMix, boundVariableType::kFloat, controlID::WRMix);

	kernel.push(&BPF1_FC, boundVariableType::kFloat, controlID::BPF1_FC);
	kernel.push(&BPF1_Q, boundVariableType::kFloat, controlID::BPF1_Q);
	kernel.push(&BPF1_Switch, boundVariableType::kInt, controlID::BPF1_Switch);
	kernel.push(&BPF1_Mix, boundVariableType::kFloat, controlID::BPF1_Mix);

	kernel.push(&MasterClean, boundVariableType::kFloat, controlID::MasterClean);
	kernel.push(&MasterDistortion, boundVariableType::kFloat, controlID::MasterDistortion);

	// --- create the super fast access array
	initPluginParameterArray();

	return true;
}

/**
\brief use this method to add new presets to the list

NOTES:
- see the SDK for examples of use
- for non RackAFX users that have large paramter counts, there is a secret GUI control you
  can enable to write C++ code into text files, one per preset. See the SDK or http://www.willpirkle.com for details

\return true if operation succeeds, false otherwise
*/
bool PluginCore::initPluginPresets()
{
	// **--0xFF7A--**

	// --- Plugin Presets 
	int index = 0;
	PresetInfo* preset = nullptr;

	// --- Preset: Factory Preset
	preset = new PresetInfo(index++, "Factory Preset");
	initPresetParameters(preset->presetParameters);
	setPresetParameter(preset->presetParameters, controlID::LPF1_Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF1_FC, 500.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF1_Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF2_Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF1_Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF2_Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF1_FC, 500.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF1_Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF2_FC, 500.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF2_Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF2_Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF2_FC, 500.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF1_Channel, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF1_Channel, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF2_Channel, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::HPF2_Channel, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::LPF1_Mix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::LPF2_Mix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::HPF2_Mix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::HPF1_Mix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::LPFMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::HPFMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::DCSwitch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::ZCSwitch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::TanHSwitch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::ATan2Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::DC, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::ZC, 0.000000);
	setPresetParameter(preset->presetParameters, controlID::TanHDrive, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::ATan2Drive, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::DCMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::ZCMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::TanHMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::ATan2Mix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::ATanSwitch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::ATanDrive, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::ATanMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::WaveRectifier, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::WRMix, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::MasterClean, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::MasterDistortion, 0.707000);
	setPresetParameter(preset->presetParameters, controlID::BPF1_Switch, -0.000000);
	setPresetParameter(preset->presetParameters, controlID::BPF1_FC, 500.000000);
	setPresetParameter(preset->presetParameters, controlID::BPF1_Q, 1.000000);
	setPresetParameter(preset->presetParameters, controlID::BPF1_Mix, 0.000000);
	addPreset(preset);


	// **--0xA7FF--**

    return true;
}

/**
\brief setup the plugin description strings, flags and codes; this is ordinarily done through the ASPiKreator or CMake

\return true if operation succeeds, false otherwise
*/
bool PluginCore::initPluginDescriptors()
{
	// --- setup audio procssing style
	//
	// --- kProcessFrames and kBlockSize are set in plugindescription.h
	//
	// --- true:  process audio frames --- less efficient, but easier to understand when starting out
	//     false: process audio blocks --- most efficient, but somewhat more complex code
	pluginDescriptor.processFrames = kProcessFrames;

	// --- for block processing (if pluginDescriptor.processFrame == false),
	//     this is the block size
	processBlockInfo.blockSize = kBlockSize;

    pluginDescriptor.pluginName = PluginCore::getPluginName();
    pluginDescriptor.shortPluginName = PluginCore::getShortPluginName();
    pluginDescriptor.vendorName = PluginCore::getVendorName();
    pluginDescriptor.pluginTypeCode = PluginCore::getPluginType();

	// --- describe the plugin attributes; set according to your needs
	pluginDescriptor.hasSidechain = kWantSidechain;
	pluginDescriptor.latencyInSamples = kLatencyInSamples;
	pluginDescriptor.tailTimeInMSec = kTailTimeMsec;
	pluginDescriptor.infiniteTailVST3 = kVSTInfiniteTail;

    // --- AAX
    apiSpecificInfo.aaxManufacturerID = kManufacturerID;
    apiSpecificInfo.aaxProductID = kAAXProductID;
    apiSpecificInfo.aaxBundleID = kAAXBundleID;  /* MacOS only: this MUST match the bundle identifier in your info.plist file */
    apiSpecificInfo.aaxEffectID = "aaxDeveloper.";
    apiSpecificInfo.aaxEffectID.append(PluginCore::getPluginName());
    apiSpecificInfo.aaxPluginCategoryCode = kAAXCategory;

    // --- AU
    apiSpecificInfo.auBundleID = kAUBundleID;   /* MacOS only: this MUST match the bundle identifier in your info.plist file */
    apiSpecificInfo.auBundleName = kAUBundleName;

    // --- VST3
    apiSpecificInfo.vst3FUID = PluginCore::getVSTFUID(); // OLE string format
    apiSpecificInfo.vst3BundleID = kVST3BundleID;/* MacOS only: this MUST match the bundle identifier in your info.plist file */
	apiSpecificInfo.enableVST3SampleAccurateAutomation = kVSTSAA;
	apiSpecificInfo.vst3SampleAccurateGranularity = kVST3SAAGranularity;

    // --- AU and AAX
    apiSpecificInfo.fourCharCode = PluginCore::getFourCharCode();

    return true;
}

// --- static functions required for VST3/AU only --------------------------------------------- //
const char* PluginCore::getPluginBundleName() { return getPluginDescBundleName(); }
const char* PluginCore::getPluginName(){ return kPluginName; }
const char* PluginCore::getShortPluginName(){ return kShortPluginName; }
const char* PluginCore::getVendorName(){ return kVendorName; }
const char* PluginCore::getVendorURL(){ return kVendorURL; }
const char* PluginCore::getVendorEmail(){ return kVendorEmail; }
const char* PluginCore::getAUCocoaViewFactoryName(){ return AU_COCOA_VIEWFACTORY_STRING; }
pluginType PluginCore::getPluginType(){ return kPluginType; }
const char* PluginCore::getVSTFUID(){ return kVSTFUID; }
int32_t PluginCore::getFourCharCode(){ return kFourCharCode; }
