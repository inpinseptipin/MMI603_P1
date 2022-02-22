// -----------------------------------------------------------------------------
//    ASPiK Plugin Kernel File:  plugincore.h
//
/**
    \file   plugincore.h
    \author Will Pirkle
    \date   17-September-2018
    \brief  base class interface file for ASPiK plugincore object
    		- http://www.aspikplugins.com
    		- http://www.willpirkle.com
*/
// -----------------------------------------------------------------------------
#ifndef __pluginCore_h__
#define __pluginCore_h__

#include "pluginbase.h"
#include "AudioEffect.h"

// **--0x7F1F--**

// --- Plugin Variables controlID Enumeration 

enum controlID {
	LPF1_Switch = 70,
	LPF1_FC = 71,
	LPF1_Q = 72,
	LPF2_Switch = 60,
	HPF1_Switch = 79,
	HPF2_Switch = 69,
	HPF1_FC = 77,
	HPF1_Q = 78,
	LPF2_FC = 61,
	LPF2_Q = 62,
	HPF2_Q = 68,
	HPF2_FC = 67,
	LPF1_Channel = 73,
	HPF1_Channel = 76,
	LPF2_Channel = 63,
	HPF2_Channel = 66,
	LPF1_Mix = 74,
	LPF2_Mix = 64,
	HPF2_Mix = 65,
	HPF1_Mix = 75,
	LPFMix = 54,
	HPFMix = 55,
	DCSwitch = 50,
	ZCSwitch = 51,
	TanHSwitch = 52,
	ATan2Switch = 53,
	DC = 40,
	ZC = 41,
	TanHDrive = 42,
	ATan2Drive = 43,
	DCMix = 30,
	ZCMix = 31,
	TanHMix = 32,
	ATan2Mix = 33,
	ATanSwitch = 56,
	ATanDrive = 46,
	ATanMix = 36,
	WaveRectifier = 57,
	WRMix = 37,
	MasterClean = 4,
	MasterDistortion = 5,
	BPF1_Switch = 20,
	BPF1_FC = 21,
	BPF1_Q = 22,
	BPF1_Mix = 23
};

	// **--0x0F1F--**

/**
\class PluginCore
\ingroup ASPiK-Core
\brief
The PluginCore object is the default PluginBase derived object for ASPiK projects.
Note that you are fre to change the name of this object (as long as you change it in the compiler settings, etc...)


PluginCore Operations:
- overrides the main processing functions from the base class
- performs reset operation on sub-modules
- processes audio
- processes messages for custom views
- performs pre and post processing functions on parameters and audio (if needed)

\author Will Pirkle http://www.willpirkle.com
\remark This object is included in Designing Audio Effects Plugins in C++ 2nd Ed. by Will Pirkle
\version Revision : 1.0
\date Date : 2018 / 09 / 7
*/
class PluginCore : public PluginBase
{
public:
    PluginCore();

	/** Destructor: empty in default version */
    virtual ~PluginCore(){}

	// --- PluginBase Overrides ---
	//
	/** this is the creation function for all plugin parameters */
	bool initPluginParameters();

	/** called when plugin is loaded, a new audio file is playing or sample rate changes */
	virtual bool reset(ResetInfo& resetInfo);

	/** one-time post creation init function; pluginInfo contains path to this plugin */
	virtual bool initialize(PluginInfo& _pluginInfo);

	/** preProcess: sync GUI parameters here; override if you don't want to use automatic variable-binding */
	virtual bool preProcessAudioBuffers(ProcessBufferInfo& processInfo);

	/** process frames of data (DEFAULT MODE) */
	virtual bool processAudioFrame(ProcessFrameInfo& processFrameInfo);

	/** Pre-process the block with: MIDI events for the block and parametet smoothing */
	virtual bool preProcessAudioBlock(IMidiEventQueue* midiEventQueue = nullptr);

	/** process sub-blocks of data (OPTIONAL MODE) */
	virtual bool processAudioBlock(ProcessBlockInfo& processBlockInfo);

	/** This is the native buffer processing function; you may override and implement
	     it if you want to roll your own buffer or block procssing code */
	// virtual bool processAudioBuffers(ProcessBufferInfo& processBufferInfo);

	/** preProcess: do any post-buffer processing required; default operation is to send metering data to GUI  */
	virtual bool postProcessAudioBuffers(ProcessBufferInfo& processInfo);

	/** called by host plugin at top of buffer proccess; this alters parameters prior to variable binding operation  */
	virtual bool updatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo);

	/** called by host plugin at top of buffer proccess; this alters parameters prior to variable binding operation  */
	virtual bool updatePluginParameterNormalized(int32_t controlID, double normalizedValue, ParameterUpdateInfo& paramInfo);

	/** this can be called: 1) after bound variable has been updated or 2) after smoothing occurs  */
	virtual bool postUpdatePluginParameter(int32_t controlID, double controlValue, ParameterUpdateInfo& paramInfo);

	/** this is ony called when the user makes a GUI control change */
	virtual bool guiParameterChanged(int32_t controlID, double actualValue);

	/** processMessage: messaging system; currently used for custom/special GUI operations */
	virtual bool processMessage(MessageInfo& messageInfo);

	/** processMIDIEvent: MIDI event processing */
	virtual bool processMIDIEvent(midiEvent& event);

	/** specialized joystick servicing (currently not used) */
	virtual bool setVectorJoystickParameters(const VectorJoystickData& vectorJoysickData);

	/** create the presets */
	bool initPluginPresets();

	// --- example block processing template functions (OPTIONAL)
	//
	/** FX EXAMPLE: process audio by passing through */
	bool renderFXPassThrough(ProcessBlockInfo& blockInfo);

	/** SYNTH EXAMPLE: render a block of silence */
	bool renderSynthSilence(ProcessBlockInfo& blockInfo);

	// --- BEGIN USER VARIABLES AND FUNCTIONS -------------------------------------- //
	//	   Add your variables and methods here



	// --- END USER VARIABLES AND FUNCTIONS -------------------------------------- //

protected:

private:
	//  **--0x07FD--**

	// --- Continuous Plugin Variables 
	float LPF1_FC = 0.f;
	float LPF1_Q = 0.f;
	float HPF1_FC = 0.f;
	float HPF1_Q = 0.f;
	float LPF2_FC = 0.f;
	float LPF2_Q = 0.f;
	float HPF2_Q = 0.f;
	float HPF2_FC = 0.f;
	float LPF1_Mix = 0.f;
	float LPF2_Mix = 0.f;
	float HPF2_Mix = 0.f;
	float HPF1_Mix = 0.f;
	float LPFMix = 0.f;
	float HPFMix = 0.f;
	float DC = 0.f;
	float ZC = 0.f;
	float TanHDrive = 0.f;
	float ATan2Drive = 0.f;
	float DCMix = 0.f;
	float ZCMix = 0.f;
	float TanHMix = 0.f;
	float ATan2Mix = 0.f;
	float ATanDrive = 0.f;
	float ATanMix = 0.f;
	float WRMix = 0.f;
	float MasterClean = 0.f;
	float MasterDistortion = 0.f;
	float BPF1_FC = 0.f;
	float BPF1_Q = 0.f;
	float BPF1_Mix = 0.f;

	// --- Discrete Plugin Variables 
	int LPF1_Switch = 0;
	enum class LPF1_SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(LPF1_SwitchEnum::SWITCH_OFF, LPF1_Switch)) etc... 

	int LPF2_Switch = 0;
	enum class LPF2_SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(LPF2_SwitchEnum::SWITCH_OFF, LPF2_Switch)) etc... 

	int HPF1_Switch = 0;
	enum class HPF1_SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(HPF1_SwitchEnum::SWITCH_OFF, HPF1_Switch)) etc... 

	int HPF2_Switch = 0;
	enum class HPF2_SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(HPF2_SwitchEnum::SWITCH_OFF, HPF2_Switch)) etc... 

	int LPF1_Channel = 0;
	enum class LPF1_ChannelEnum { Left,Right };	// to compare: if(compareEnumToInt(LPF1_ChannelEnum::Left, LPF1_Channel)) etc... 

	int HPF1_Channel = 0;
	enum class HPF1_ChannelEnum { Left,Right };	// to compare: if(compareEnumToInt(HPF1_ChannelEnum::Left, HPF1_Channel)) etc... 

	int LPF2_Channel = 0;
	enum class LPF2_ChannelEnum { Left,Right };	// to compare: if(compareEnumToInt(LPF2_ChannelEnum::Left, LPF2_Channel)) etc... 

	int HPF2_Channel = 0;
	enum class HPF2_ChannelEnum { Left,Right };	// to compare: if(compareEnumToInt(HPF2_ChannelEnum::Left, HPF2_Channel)) etc... 

	int DCSwitch = 0;
	enum class DCSwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(DCSwitchEnum::SWITCH_OFF, DCSwitch)) etc... 

	int ZCSwitch = 0;
	enum class ZCSwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(ZCSwitchEnum::SWITCH_OFF, ZCSwitch)) etc... 

	int TanHSwitch = 0;
	enum class TanHSwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(TanHSwitchEnum::SWITCH_OFF, TanHSwitch)) etc... 

	int ATan2Switch = 0;
	enum class ATan2SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(ATan2SwitchEnum::SWITCH_OFF, ATan2Switch)) etc... 

	int ATanSwitch = 0;
	enum class ATanSwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(ATanSwitchEnum::SWITCH_OFF, ATanSwitch)) etc... 

	int WaveRectifier = 0;
	enum class WaveRectifierEnum { None,HW,FW };	// to compare: if(compareEnumToInt(WaveRectifierEnum::None, WaveRectifier)) etc... 

	int BPF1_Switch = 0;
	enum class BPF1_SwitchEnum { SWITCH_OFF,SWITCH_ON };	// to compare: if(compareEnumToInt(BPF1_SwitchEnum::SWITCH_OFF, BPF1_Switch)) etc... 

	// **--0x1A7F--**
    // --- end member variables
	AuxPort::Effect<float, float> kernel;
	AuxPort::Frame<float> audioFrame;
public:
    /** static description: bundle folder name

	\return bundle folder name as a const char*
	*/
    static const char* getPluginBundleName();

    /** static description: name

	\return name as a const char*
	*/
    static const char* getPluginName();

	/** static description: short name

	\return short name as a const char*
	*/
	static const char* getShortPluginName();

	/** static description: vendor name

	\return vendor name as a const char*
	*/
	static const char* getVendorName();

	/** static description: URL

	\return URL as a const char*
	*/
	static const char* getVendorURL();

	/** static description: email

	\return email address as a const char*
	*/
	static const char* getVendorEmail();

	/** static description: Cocoa View Factory Name

	\return Cocoa View Factory Name as a const char*
	*/
	static const char* getAUCocoaViewFactoryName();

	/** static description: plugin type

	\return type (FX or Synth)
	*/
	static pluginType getPluginType();

	/** static description: VST3 GUID

	\return VST3 GUID as a const char*
	*/
	static const char* getVSTFUID();

	/** static description: 4-char code

	\return 4-char code as int
	*/
	static int32_t getFourCharCode();

	/** initalizer */
	bool initPluginDescriptors();

    /** Status Window Messages for hosts that can show it */
    void sendHostTextMessage(std::string messageString)
    {
        HostMessageInfo hostMessageInfo;
        hostMessageInfo.hostMessage = sendRAFXStatusWndText;
        hostMessageInfo.rafxStatusWndText.assign(messageString);
        if(pluginHostConnector)
            pluginHostConnector->sendHostMessage(hostMessageInfo);
    }

};




#endif /* defined(__pluginCore_h__) */
