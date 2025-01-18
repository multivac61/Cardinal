/*
 * DISTRHO Cardinal Plugin
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#include "rack.hpp"
#include "plugin.hpp"

#include "DistrhoUtils.hpp"

// Cardinal (built-in)
#include "Cardinal/src/plugin.hpp"

// Fundamental
extern Model* modelADSR;
extern Model* modelCompare;
extern Model* modelLFO;
extern Model* modelMerge;
extern Model* modelMidSide;
extern Model* modelNoise;
extern Model* modelQuantizer;
extern Model* modelRandom;
extern Model* modelScope;
extern Model* modelSplit;
extern Model* modelSum;
extern Model* modelVCA_1;
extern Model* modelVCF;
extern Model* modelVCMixer;
extern Model* modelVCO;
extern Model* modelCVMix;
extern Model* modelGates;
extern Model* modelMixer;
extern Model* modelMult;
extern Model* modelMutes;
extern Model* modelOctave;
extern Model* modelProcess;
extern Model* modelRescale;

// AnimatedCircuits
#include "AnimatedCircuits/src/plugin.hpp"

// Aria
extern Model* modelSpleet;
extern Model* modelSwerge;

// AudibleInstruments
#include "AudibleInstruments/src/plugin.hpp"

// AS
extern Model *modelStereoVUmeter;


// BogaudioModules - integrate theme/skin support
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define private public
#include "BogaudioModules/src/skins.hpp"
#undef private

// BogaudioModules
extern Model* modelAD;
extern Model* modelBogaudioLFO;
extern Model* modelBogaudioNoise;
extern Model* modelBogaudioVCA;
extern Model* modelBogaudioVCF;
extern Model* modelBogaudioVCO;
extern Model* modelOffset;
extern Model* modelSampleHold;
extern Model* modelSwitch;
extern Model* modelSwitch18;
extern Model* modelUnison;

#define modelMix4 modelBogaudioMix4
extern Model* modelFollow;
extern Model* modelMix4;
extern Model* modelPolyCon8;
extern Model* modelPressor;
extern Model* modelSlew;
extern Model* modelEQ;
extern Model* modelEightOne;
extern Model* modelSampleHold;
#undef modelMix4

// Bidoo
extern Model *modelTIARE;

// countmodula
extern Model* modelLightStrip;
extern Model* modelPolyChances;
#define modelComparator modelcountmodulaComparator
#define modelMixer modelcountmodulaMixer
#define modelFade modelcountmodulaFade
#define modelBusRoute modelcountmodulaBusRoute
#define modelArpeggiator modelcountmodulaArpeggiator
#define modelMult modelcountmodulaMult
#define modelStack modelcountmodulaStack
#define SmallKnob countmodulaSmallKnob
#define RotarySwitch countmodulaRotarySwitch
#include "countmodula/src/components/CountModulaKnobs.hpp"
#include "countmodula/src/components/CountModulaComponents.hpp"
#include "countmodula/src/components/CountModulaPushButtons.hpp"
#include "countmodula/src/components/StdComponentPositions.hpp"
#undef modelComparator
#undef modelMixer
#undef modelFade
#undef modelBusRoute
#undef modelArpeggiator
#undef modelMult
#undef modelStack
#undef RotarySwitch
#undef SmallKnob

// dbRackModules
extern Model* modelOFS;

// Hetrick
#define modelMidSide modelHetrickCVMidSide
#define InverterWidget HetrickCVInverterWidget
extern Model* modelDust;
extern Model* modelMidSide;
#undef InverterWidget
#undef modelMidSide

// MockbaModular
#include "MockbaModular/src/plugin.hpp"
#include "MockbaModular/src/MockbaModular.hpp"
#undef min
#define saveBack ignoreMockbaModular1
#define loadBack ignoreMockbaModular2
#include "MockbaModular/src/MockbaModular.cpp"
#undef saveBack
#undef loadBack
std::string loadBack(int) { return "res/Empty_gray.svg"; }

// MUS-X
namespace musx {
extern Model* modelFilter;
extern Model* modelOnePoleLP;
}

// MindMeldModular
extern Model *modelPatchMaster;
extern Model *modelPatchMasterBlank;

// Submarine Free
#include "Submarine/src/shared/components.hpp"
extern Model *modelTD202;
extern Model *modelTD410;

// surgext
#include "surgext/src/SurgeXT.h"
void surgext_rack_initialize();
void surgext_rack_update_theme();

// ValleyAudio
#include "ValleyAudio/src/Valley.hpp"

// Venom
#define modelBypass modelVenomBypass
#define modelLogic modelVenomLogic
#define YellowRedLight VenomYellowRedLight
#define YellowBlueLight VenomYellowBlueLight
#define DigitalDisplay VenomDigitalDisplay
#include "Venom/src/plugin.hpp"
#undef DigitalDisplay
#undef YellowRedLight
#undef YellowBlueLight
#undef modelBypass
#undef modelLogic


// known terminal modules
std::vector<Model*> hostTerminalModels;

// plugin instances
Plugin* pluginInstance__Cardinal;
Plugin* pluginInstance__Fundamental;
Plugin* pluginInstance__AnimatedCircuits;
Plugin* pluginInstance__Aria;
Plugin* pluginInstance__AS;
Plugin* pluginInstance__AudibleInstruments;
Plugin* pluginInstance__Bidoo;
Plugin* pluginInstance__BogaudioModules;
extern Plugin* pluginInstance__countmodula;
Plugin* pluginInstance__HetrickCV;
Plugin* pluginInstance__dbRackModules;
extern Plugin* pluginInstance__MindMeld;
Plugin* pluginInstance__MockbaModular;
Plugin* pluginInstance__MUS_X;
Plugin* pluginInstance__Submarine;
Plugin* pluginInstance__surgext;
Plugin* pluginInstance__ValleyAudio;
extern Plugin* pluginInstance__Venom;


namespace rack {

namespace asset {
std::string pluginManifest(const std::string& dirname);
std::string pluginPath(const std::string& dirname);
}

namespace plugin {

struct StaticPluginLoader {
    Plugin* const plugin;
    FILE* file;
    json_t* rootJ;

    StaticPluginLoader(Plugin* const p, const char* const name)
        : plugin(p),
          file(nullptr),
          rootJ(nullptr)
    {
#ifdef DEBUG
        DEBUG("Loading plugin module %s", name);
#endif

        p->path = asset::pluginPath(name);

        const std::string manifestFilename = asset::pluginManifest(name);

        if ((file = std::fopen(manifestFilename.c_str(), "r")) == nullptr)
        {
            d_stderr2("Manifest file %s does not exist", manifestFilename.c_str());
            return;
        }

        json_error_t error;
        if ((rootJ = json_loadf(file, 0, &error)) == nullptr)
        {
            d_stderr2("JSON parsing error at %s %d:%d %s", manifestFilename.c_str(), error.line, error.column, error.text);
            return;
        }

        // force ABI, we use static plugins so this doesnt matter as long as it builds
        json_t* const version = json_string((APP_VERSION_MAJOR + ".0").c_str());
        json_object_set(rootJ, "version", version);
        json_decref(version);

        // Load manifest
        p->fromJson(rootJ);

        // Reject plugin if slug already exists
        if (Plugin* const existingPlugin = getPlugin(p->slug))
            throw Exception("Plugin %s is already loaded, not attempting to load it again", p->slug.c_str());
    }

    ~StaticPluginLoader()
    {
        if (rootJ != nullptr)
        {
            // Load modules manifest
            json_t* const modulesJ = json_object_get(rootJ, "modules");
            plugin->modulesFromJson(modulesJ);

            json_decref(rootJ);
            plugins.push_back(plugin);
        }

        if (file != nullptr)
            std::fclose(file);
    }

    bool ok() const noexcept
    {
        return rootJ != nullptr;
    }

    void removeModule(const char* const slugToRemove) const noexcept
    {
        json_t* const modules = json_object_get(rootJ, "modules");
        DISTRHO_SAFE_ASSERT_RETURN(modules != nullptr,);

        size_t i;
        json_t* v;
        json_array_foreach(modules, i, v)
        {
            if (json_t* const slug = json_object_get(v, "slug"))
            {
                if (const char* const value = json_string_value(slug))
                {
                    if (std::strcmp(value, slugToRemove) == 0)
                    {
                        json_array_remove(modules, i);
                        break;
                    }
                }
            }
        }
    }
};

static void initStatic__Cardinal()
{
    Plugin* const p = new Plugin;
    pluginInstance__Cardinal = p;

    const StaticPluginLoader spl(p, "Cardinal");
    if (spl.ok())
    {
        p->addModel(modelHostAudio2);
        p->addModel(modelHostCV);
        p->addModel(modelHostMIDI);
        p->addModel(modelHostMIDICC);
        p->addModel(modelHostMIDIGate);
        p->addModel(modelHostMIDIMap);
        p->addModel(modelHostParameters);
        p->addModel(modelHostParametersMap);
        p->addModel(modelHostTime);
        p->addModel(modelTextEditor);
        /* TODO
       #ifdef HAVE_FFTW3F
        p->addModel(modelAudioToCVPitch);
       #else
        */
        spl.removeModule("AudioToCVPitch");
        /*
       #endif
        */
        spl.removeModule("AIDA-X");
        spl.removeModule("AudioFile");
        spl.removeModule("Blank");
        spl.removeModule("Carla");
        spl.removeModule("ExpanderInputMIDI");
        spl.removeModule("ExpanderOutputMIDI");
        spl.removeModule("HostAudio8");
        spl.removeModule("Ildaeil");
        spl.removeModule("MPV");
        spl.removeModule("SassyScope");
        spl.removeModule("glBars");

        hostTerminalModels = {
            modelHostAudio2,
            modelHostCV,
            modelHostMIDI,
            modelHostMIDICC,
            modelHostMIDIGate,
            modelHostMIDIMap,
            modelHostParameters,
            modelHostParametersMap,
            modelHostTime,
        };
    }
}

static void initStatic__Fundamental()
{
    Plugin* const p = new Plugin;
    pluginInstance__Fundamental = p;

    const StaticPluginLoader spl(p, "Fundamental");
    if (spl.ok())
    {
        p->addModel(modelADSR);
        p->addModel(modelCompare);
        p->addModel(modelLFO);
        p->addModel(modelMerge);
        p->addModel(modelMidSide);
        p->addModel(modelNoise);
        p->addModel(modelQuantizer);
        p->addModel(modelRandom);
        p->addModel(modelScope);
        p->addModel(modelSplit);
        p->addModel(modelSum);
        p->addModel(modelVCA_1);
        p->addModel(modelVCF);
        p->addModel(modelVCMixer);
        p->addModel(modelVCO);
        p->addModel(modelCVMix);
        p->addModel(modelGates);
        p->addModel(modelMixer);
        p->addModel(modelMult);
        p->addModel(modelMutes);
        p->addModel(modelOctave);
        p->addModel(modelProcess);
        p->addModel(modelRescale);
        spl.removeModule("8vert");
        spl.removeModule("Delay");
        spl.removeModule("LFO2");
        spl.removeModule("Pulses");
        spl.removeModule("SEQ3");
        spl.removeModule("SequentialSwitch1");
        spl.removeModule("SequentialSwitch2");
        spl.removeModule("VCA");
        spl.removeModule("VCO2");
        spl.removeModule("Fade");
        spl.removeModule("Logic");
        spl.removeModule("Push");
        spl.removeModule("RandomValues");
        spl.removeModule("SHASR");
        spl.removeModule("Unity");
        spl.removeModule("Viz");
    }
}

static void initStatic__AnimatedCircuits()
{
    Plugin* const p = new Plugin;
    pluginInstance__AnimatedCircuits = p;

    const StaticPluginLoader spl(p, "AnimatedCircuits");
    if (spl.ok())
    {
        p->addModel(model_AC_Folding);
        p->addModel(model_AC_LFold);
    }
}

static void initStatic__Aria()
{
    Plugin* const p = new Plugin;
    pluginInstance__Aria = p;

    const StaticPluginLoader spl(p, "AriaModules");
    if (spl.ok())
    {
        p->addModel(modelSpleet);
        p->addModel(modelSwerge);

        spl.removeModule("Aleister");
        spl.removeModule("Arcane");
        spl.removeModule("Atout");
        spl.removeModule("Blank");
        spl.removeModule("Darius");
        spl.removeModule("Grabby");
        spl.removeModule("Pokies4");
        spl.removeModule("Psychopump");
        spl.removeModule("Q");
        spl.removeModule("Qqqq");
        spl.removeModule("Quack");
        spl.removeModule("Quale");
        spl.removeModule("Rotatoes4");
        spl.removeModule("Smerge");
        spl.removeModule("Solomon16");
        spl.removeModule("Solomon4");
        spl.removeModule("Solomon8");
        spl.removeModule("Splirge");
        spl.removeModule("Splort");
        spl.removeModule("Undular");

    }
}

static void initStatic__AS()
{
    Plugin* const p = new Plugin;
    pluginInstance__AS = p;
    const StaticPluginLoader spl(p, "AS");
    if (spl.ok())
    {
        p->addModel(modelStereoVUmeter);

        spl.removeModule("ADSR");
        spl.removeModule("AtNuVrTr");
        spl.removeModule("BPMCalc");
        spl.removeModule("BPMCalc2");
        spl.removeModule("BPMClock");
        spl.removeModule("BlankPanel4");
        spl.removeModule("BlankPanel6");
        spl.removeModule("BlankPanel8");
        spl.removeModule("BlankPanelSpecial");
        spl.removeModule("Cv2T");
        spl.removeModule("DelayPlusFx");
        spl.removeModule("DelayPlusStereoFx");
        spl.removeModule("Flow");
        spl.removeModule("KillGate");
        spl.removeModule("LaunchGate");
        spl.removeModule("Merge2_5");
        spl.removeModule("Mixer2ch");
        spl.removeModule("Mixer4ch");
        spl.removeModule("Mixer8ch");
        spl.removeModule("MonoVUmeter");
        spl.removeModule("Multiple2_5");
        spl.removeModule("PhaserFx");
        spl.removeModule("QuadVCA");
        spl.removeModule("ReScale");
        spl.removeModule("ReverbFx");
        spl.removeModule("ReverbStereoFx");
        spl.removeModule("SEQ16");
        spl.removeModule("SawOSC");
        spl.removeModule("SignalDelay");
        spl.removeModule("SineOSC");
        spl.removeModule("Steps");
        spl.removeModule("SuperDriveFx");
        spl.removeModule("SuperDriveStereoFx");
        spl.removeModule("TremoloFx");
        spl.removeModule("TremoloStereoFx");
        spl.removeModule("TriLFO");
        spl.removeModule("TriggersMKI");
        spl.removeModule("TriggersMKII");
        spl.removeModule("TriggersMKIII");
        spl.removeModule("VCA");
        spl.removeModule("WaveShaper");
        spl.removeModule("WaveShaperStereo");
        spl.removeModule("ZeroCV2T");
    }
}

static void initStatic__AudibleInstruments()
{
    Plugin* const p = new Plugin;
    pluginInstance__AudibleInstruments = p;

    const StaticPluginLoader spl(p, "AudibleInstruments");
    if (spl.ok())
    {
        p->addModel(modelPlaits);

        spl.removeModule("Blinds");
        spl.removeModule("Braids");
        spl.removeModule("Branches");
        spl.removeModule("Clouds");
        spl.removeModule("Elements");
        spl.removeModule("Frames");
        spl.removeModule("Kinks");
        spl.removeModule("Links");
        spl.removeModule("Marbles");
        spl.removeModule("Rings");
        spl.removeModule("Ripples");
        spl.removeModule("Shades");
        spl.removeModule("Shelves");
        spl.removeModule("Stages");
        spl.removeModule("Streams");
        spl.removeModule("Tides");
        spl.removeModule("Tides2");
        spl.removeModule("Veils");
        spl.removeModule("Warps");
    }
}

static void initStatic__Bidoo()
{
    Plugin* const p = new Plugin;
    pluginInstance__Bidoo = p;

    const StaticPluginLoader spl(p, "Bidoo");
    if (spl.ok())
    {
       p->addModel(modelTIARE); 

       spl.removeModule("ACnE");
       spl.removeModule("BAFIS");
       spl.removeModule("BISTROT");
       spl.removeModule("BanCau");
       spl.removeModule("ChUTE");
       spl.removeModule("DIKTAT");
       spl.removeModule("DilEMO");
       spl.removeModule("EMILE");
       spl.removeModule("ENCORE");
       spl.removeModule("ENCORE-Expander");
       spl.removeModule("FREIN");
       spl.removeModule("ForK");
       spl.removeModule("HCTIP");
       spl.removeModule("HUITre");
       spl.removeModule("LoURdE");
       spl.removeModule("MAGMA");
       spl.removeModule("MOiRE");
       spl.removeModule("MS");
       spl.removeModule("MU");
       spl.removeModule("OAI");
       spl.removeModule("OUAIve");
       spl.removeModule("PILOT");
       spl.removeModule("POUPRE");
       spl.removeModule("RATEAU");
       spl.removeModule("REI");
       spl.removeModule("SIGMA");
       spl.removeModule("SPORE");
       spl.removeModule("VOID");
       spl.removeModule("ZOUMAI");
       spl.removeModule("ZOUMAI-Expander");
       spl.removeModule("antN");
       spl.removeModule("baR");
       spl.removeModule("bordL");
       spl.removeModule("cANARd");
       spl.removeModule("dFUZE");
       spl.removeModule("dTrOY");
       spl.removeModule("dUKe");
       spl.removeModule("eDsaroS");
       spl.removeModule("fLAME");
       spl.removeModule("lATe");
       spl.removeModule("lIMbO");
       spl.removeModule("lambda");
       spl.removeModule("liMonADe");
       spl.removeModule("mINIBar");
       spl.removeModule("pErCO");
       spl.removeModule("rabBIT");
       spl.removeModule("tOCAnTe");
       spl.removeModule("ziNC");
    }
}

static void initStatic__BogaudioModules()
{
    Plugin* const p = new Plugin;
    pluginInstance__BogaudioModules = p;

    const StaticPluginLoader spl(p, "BogaudioModules");
    if (spl.ok())
    {
        // Make sure to use match Cardinal theme
        Skins& skins(Skins::skins());
        skins._default = settings::preferDarkPanels ? "dark" : "light";

        p->addModel(modelAD);
        p->addModel(modelBogaudioLFO);
        p->addModel(modelBogaudioNoise);
        p->addModel(modelBogaudioVCA);
        p->addModel(modelBogaudioVCF);
        p->addModel(modelBogaudioVCO);
        p->addModel(modelOffset);
        p->addModel(modelSampleHold);
        p->addModel(modelSwitch);
        p->addModel(modelSwitch18);
        p->addModel(modelUnison);

        p->addModel(modelFollow);
#define modelMix4 modelBogaudioMix4
        p->addModel(modelMix4);
#undef modelMix4
        p->addModel(modelPolyCon8);
        p->addModel(modelPressor);
        p->addModel(modelSlew);
        p->addModel(modelEQ);
        p->addModel(modelEightOne);

        // cat plugins/BogaudioModules/plugin.json  | jq -r .modules[].slug - | sort
        spl.removeModule("Bogaudio-Additator");
        spl.removeModule("Bogaudio-AddrSeq");
        spl.removeModule("Bogaudio-AddrSeqX");
        spl.removeModule("Bogaudio-ADSR");
        spl.removeModule("Bogaudio-AMRM");
        spl.removeModule("Bogaudio-Analyzer");
        spl.removeModule("Bogaudio-AnalyzerXL");
        spl.removeModule("Bogaudio-Arp");
        spl.removeModule("Bogaudio-ASR");
        spl.removeModule("Bogaudio-Assign");
        spl.removeModule("Bogaudio-Blank3");
        spl.removeModule("Bogaudio-Blank6");
        spl.removeModule("Bogaudio-Bool");
        spl.removeModule("Bogaudio-Chirp");
        spl.removeModule("Bogaudio-Clpr");
        spl.removeModule("Bogaudio-Cmp");
        spl.removeModule("Bogaudio-CmpDist");
        spl.removeModule("Bogaudio-CVD");
        spl.removeModule("Bogaudio-DADSRH");
        spl.removeModule("Bogaudio-DADSRHPlus");
        spl.removeModule("Bogaudio-Detune");
        spl.removeModule("Bogaudio-DGate");
        spl.removeModule("Bogaudio-Edge");
        spl.removeModule("Bogaudio-EightFO");
        spl.removeModule("Bogaudio-EQS");
        spl.removeModule("Bogaudio-FFB");
        spl.removeModule("Bogaudio-FlipFlop");
        spl.removeModule("Bogaudio-FMOp");
        spl.removeModule("Bogaudio-FourFO");
        spl.removeModule("Bogaudio-FourMan");
        spl.removeModule("Bogaudio-Inv");
        spl.removeModule("Bogaudio-Lgsw");
        spl.removeModule("Bogaudio-LLFO");
        spl.removeModule("Bogaudio-LLPG");
        spl.removeModule("Bogaudio-Lmtr");
        spl.removeModule("Bogaudio-LPG");
        spl.removeModule("Bogaudio-LVCF");
        spl.removeModule("Bogaudio-LVCO");
        spl.removeModule("Bogaudio-Manual");
        spl.removeModule("Bogaudio-Matrix18");
        spl.removeModule("Bogaudio-Matrix44");
        spl.removeModule("Bogaudio-Matrix44Cvm");
        spl.removeModule("Bogaudio-Matrix81");
        spl.removeModule("Bogaudio-Matrix88");
        spl.removeModule("Bogaudio-Matrix88Cv");
        spl.removeModule("Bogaudio-Matrix88M");
        spl.removeModule("Bogaudio-MegaGate");
        spl.removeModule("Bogaudio-Mix1");
        spl.removeModule("Bogaudio-Mix2");
        spl.removeModule("Bogaudio-Mix4x");
        spl.removeModule("Bogaudio-Mix8");
        spl.removeModule("Bogaudio-Mix8x");
        spl.removeModule("Bogaudio-Mono");
        spl.removeModule("Bogaudio-Mult");
        spl.removeModule("Bogaudio-Mumix");
        spl.removeModule("Bogaudio-Mute8");
        spl.removeModule("Bogaudio-Nsgt");
        spl.removeModule("Bogaudio-OneEight");
        spl.removeModule("Bogaudio-Pan");
        spl.removeModule("Bogaudio-PEQ");
        spl.removeModule("Bogaudio-PEQ14");
        spl.removeModule("Bogaudio-PEQ14XF");
        spl.removeModule("Bogaudio-PEQ6");
        spl.removeModule("Bogaudio-PEQ6XF");
        spl.removeModule("Bogaudio-Pgmr");
        spl.removeModule("Bogaudio-PgmrX");
        spl.removeModule("Bogaudio-PolyCon");
        spl.removeModule("Bogaudio-PolyMult");
        spl.removeModule("Bogaudio-PolyOff16");
        spl.removeModule("Bogaudio-PolyOff8");
        spl.removeModule("Bogaudio-Pulse");
        spl.removeModule("Bogaudio-Ranalyzer");
        spl.removeModule("Bogaudio-Reftone");
        spl.removeModule("Bogaudio-RGate");
        spl.removeModule("Bogaudio-Shaper");
        spl.removeModule("Bogaudio-ShaperPlus");
        spl.removeModule("Bogaudio-Sine");
        spl.removeModule("Bogaudio-Stack");
        spl.removeModule("Bogaudio-Sums");
        spl.removeModule("Bogaudio-Switch1616");
        spl.removeModule("Bogaudio-Switch44");
        spl.removeModule("Bogaudio-Switch81");
        spl.removeModule("Bogaudio-Switch88");
        spl.removeModule("Bogaudio-UMix");
        spl.removeModule("Bogaudio-VCAmp");
        spl.removeModule("Bogaudio-VCM");
        spl.removeModule("Bogaudio-Velo");
        spl.removeModule("Bogaudio-Vish");
        spl.removeModule("Bogaudio-VU");
        spl.removeModule("Bogaudio-Walk");
        spl.removeModule("Bogaudio-Walk2");
        spl.removeModule("Bogaudio-XCO");
        spl.removeModule("Bogaudio-XFade");
    }
}

static void initStatic__countmodula()
{
    Plugin* const p = new Plugin;
    pluginInstance__countmodula = p;

    const StaticPluginLoader spl(p, "countmodula");
    if (spl.ok())
    {
        p->addModel(modelLightStrip);	
        p->addModel(modelPolyChances);	
        spl.removeModule("AnalogueShiftRegister");
        spl.removeModule("Arpeggiator");
        spl.removeModule("Attenuator");
        spl.removeModule("Attenuverter");
        spl.removeModule("BarGraph");
        spl.removeModule("BasicSequencer8");
        spl.removeModule("BinaryComparator");
        spl.removeModule("BinarySequencer");
        spl.removeModule("Blank12HP");
        spl.removeModule("Blank16HP");
        spl.removeModule("Blank20HP");
        spl.removeModule("Blank24HP");
        spl.removeModule("Blank2HP");
        spl.removeModule("Blank4HP");
        spl.removeModule("Blank8HP");
        spl.removeModule("BooleanAND");
        spl.removeModule("BooleanOR");
        spl.removeModule("BooleanVCNOT");
        spl.removeModule("BooleanXOR");
        spl.removeModule("Breakout");
        spl.removeModule("BurstGenerator");
        spl.removeModule("BurstGenerator64");
        spl.removeModule("BusRoute");
        spl.removeModule("BusRoute2");
        spl.removeModule("CVSpreader");
        spl.removeModule("Carousel");
        spl.removeModule("Chances");
        spl.removeModule("ClockDivider");
        spl.removeModule("ClockedRandomGateExpanderCV");
        spl.removeModule("ClockedRandomGateExpanderLog");
        spl.removeModule("ClockedRandomGates");
        spl.removeModule("Comparator");
        spl.removeModule("Euclid");
        spl.removeModule("EuclidExpanderCV");
        spl.removeModule("EventArranger");
        spl.removeModule("EventTimer");
        spl.removeModule("EventTimer2");
        spl.removeModule("Fade");
        spl.removeModule("FadeExpander");
        spl.removeModule("G2T");
        spl.removeModule("GateDelay");
        spl.removeModule("GateDelayMT");
        spl.removeModule("GateModifier");
        spl.removeModule("GateSequencer16");
        spl.removeModule("GateSequencer16b");
        spl.removeModule("GateSequencer8");
        spl.removeModule("GatedComparator");
        spl.removeModule("HyperManiacalLFO");
        spl.removeModule("HyperManiacalLFOExpander");
        spl.removeModule("Mangler");
        spl.removeModule("Manifold");
        spl.removeModule("ManualCV");
        spl.removeModule("ManualCV2");
        spl.removeModule("ManualGate");
        spl.removeModule("MasterReset");
        spl.removeModule("MatrixCombiner");
        spl.removeModule("MatrixMixer");
        spl.removeModule("Megalomaniac");
        spl.removeModule("MinimusMaximus");
        spl.removeModule("Mixer");
        spl.removeModule("MorphShaper");
        spl.removeModule("Mult");
        spl.removeModule("MultiStepSequencer");
        spl.removeModule("Multiplexer");
        spl.removeModule("Mute");
        spl.removeModule("Mute-iple");
        spl.removeModule("NibbleTriggerSequencer");
        spl.removeModule("OctetTriggerSequencer");
        spl.removeModule("OctetTriggerSequencerCVExpander");
        spl.removeModule("OctetTriggerSequencerGateExpander");
        spl.removeModule("OffsetGenerator");
        spl.removeModule("Oscilloscope");
        spl.removeModule("Palette");
        spl.removeModule("PolyG2T");
        spl.removeModule("PolyGateModifier");
        spl.removeModule("PolyLogic");
        spl.removeModule("PolyMinMax");
        spl.removeModule("PolyMute");
        spl.removeModule("PolyVCPolarizer");
        spl.removeModule("PolyVCSwitch");
        spl.removeModule("PolyrhythmicGenerator");
        spl.removeModule("PolyrhythmicGeneratorMkII");
        spl.removeModule("RackEarLeft");
        spl.removeModule("RackEarRight");
        spl.removeModule("RandomAccessSwitch18");
        spl.removeModule("RandomAccessSwitch81");
        spl.removeModule("Rectifier");
        spl.removeModule("SRFlipFlop");
        spl.removeModule("SampleAndHold");
        spl.removeModule("SampleAndHold2");
        spl.removeModule("SequenceEncoder");
        spl.removeModule("Sequencer16");
        spl.removeModule("Sequencer64");
        spl.removeModule("Sequencer8");
        spl.removeModule("SequencerChannel16");
        spl.removeModule("SequencerChannel8");
        spl.removeModule("SequencerExpanderCV8");
        spl.removeModule("SequencerExpanderLOG8");
        spl.removeModule("SequencerExpanderOut8");
        spl.removeModule("SequencerExpanderRM8");
        spl.removeModule("SequencerExpanderTSG");
        spl.removeModule("SequencerExpanderTrig8");
        spl.removeModule("SequencerGates16");
        spl.removeModule("SequencerGates8");
        spl.removeModule("SequencerTriggers16");
        spl.removeModule("SequencerTriggers8");
        spl.removeModule("ShepardGenerator");
        spl.removeModule("ShiftRegister16");
        spl.removeModule("ShiftRegister32");
        spl.removeModule("SingleDFlipFlop");
        spl.removeModule("SingleSRFlipFlop");
        spl.removeModule("SingleTFlipFlop");
        spl.removeModule("SlopeDetector");
        spl.removeModule("Stack");
        spl.removeModule("StartupDelay");
        spl.removeModule("StepSequencer8");
        spl.removeModule("SubHarmonicGenerator");
        spl.removeModule("Switch16To1");
        spl.removeModule("Switch1To16");
        spl.removeModule("Switch1To8");
        spl.removeModule("Switch2");
        spl.removeModule("Switch3");
        spl.removeModule("Switch4");
        spl.removeModule("Switch8To1");
        spl.removeModule("TFlipFlop");
        spl.removeModule("TriggerSequencer16");
        spl.removeModule("TriggerSequencer8");
        spl.removeModule("VCFrequencyDivider");
        spl.removeModule("VCFrequencyDividerMkII");
        spl.removeModule("VCPolarizer");
        spl.removeModule("VCPulseDivider");
        spl.removeModule("VoltageControlledSwitch");
        spl.removeModule("VoltageInverter");
        spl.removeModule("VoltageScaler");
    }
}

static void initStatic__HetrickCV()
{
    Plugin* const p = new Plugin;
    pluginInstance__HetrickCV = p;

    const StaticPluginLoader spl(p, "HetrickCV");
    if (spl.ok())
    {
#define modelMidSide modelHetrickCVMidSide
#define InverterWidget HetrickCVInverterWidget
      p->addModel(modelDust);
      p->addModel(modelMidSide);
#undef modelMidSide
#undef InverterWidget
      spl.removeModule("2To4");
      spl.removeModule("ASR");
      spl.removeModule("AnalogToDigital");
      spl.removeModule("BinaryGate");
      spl.removeModule("BinaryNoise");
      spl.removeModule("Bitshift");
      spl.removeModule("BlankPanel");
      spl.removeModule("Boolean3");
      spl.removeModule("Chaos1Op");
      spl.removeModule("Chaos2Op");
      spl.removeModule("Chaos3Op");
      spl.removeModule("ChaoticAttractors");
      spl.removeModule("ClockedNoise");
      spl.removeModule("Comparator");
      spl.removeModule("Contrast");
      spl.removeModule("Crackle");
      spl.removeModule("DataCompander");
      spl.removeModule("Delta");
      spl.removeModule("DigitalToAnalog");
      spl.removeModule("Exponent");
      spl.removeModule("FBSineChaos");
      spl.removeModule("FlipFlop");
      spl.removeModule("FlipPan");
      spl.removeModule("GateDelay");
      spl.removeModule("GateJunction");
      spl.removeModule("GateJunctionExp");
      spl.removeModule("Gingerbread");
      spl.removeModule("LogicCombine");
      spl.removeModule("MinMax");
      spl.removeModule("PhaseDrivenSequencer");
      spl.removeModule("PhaseDrivenSequencer32");
      spl.removeModule("PhasorAnalyzer");
      spl.removeModule("PhasorBurstGen");
      spl.removeModule("PhasorDivMult");
      spl.removeModule("PhasorEuclidean");
      spl.removeModule("PhasorFreezer");
      spl.removeModule("PhasorGates");
      spl.removeModule("PhasorGates32");
      spl.removeModule("PhasorGates64");
      spl.removeModule("PhasorGen");
      spl.removeModule("PhasorGeometry");
      spl.removeModule("PhasorHumanizer");
      spl.removeModule("PhasorMixer");
      spl.removeModule("PhasorOctature");
      spl.removeModule("PhasorProbability");
      spl.removeModule("PhasorQuadrature");
      spl.removeModule("PhasorRandom");
      spl.removeModule("PhasorRanger");
      spl.removeModule("PhasorReset");
      spl.removeModule("PhasorRhythmGroup");
      spl.removeModule("PhasorShape");
      spl.removeModule("PhasorShift");
      spl.removeModule("PhasorSplitter");
      spl.removeModule("PhasorStutter");
      spl.removeModule("PhasorSubstepShape");
      spl.removeModule("PhasorSwing");
      spl.removeModule("PhasorTimetable");
      spl.removeModule("PhasorToClock");
      spl.removeModule("PhasorToLFO");
      spl.removeModule("PhasorToWaveforms");
      spl.removeModule("Probability");
      spl.removeModule("RandomGates");
      spl.removeModule("Rotator");
      spl.removeModule("Rungler");
      spl.removeModule("Scanner");
      spl.removeModule("TrigShaper");
      spl.removeModule("VectorMix");
      spl.removeModule("Waveshaper");
      spl.removeModule("XYToPolar");
    }
}

 static void initStatic__dbRackModules()
{
    Plugin* const p = new Plugin;
    pluginInstance__dbRackModules = p;

    const StaticPluginLoader spl(p, "dbRackModules");
    if (spl.ok())
    {
      p->addModel(modelOFS);
      spl.removeModule("AP");
      spl.removeModule("AUX");
      spl.removeModule("AddSynth");
      spl.removeModule("BWF");
      spl.removeModule("CHBY");
      spl.removeModule("CLP");
      spl.removeModule("CSOSC");
      spl.removeModule("CVS");
      spl.removeModule("CWS");
      spl.removeModule("DCBlock");
      spl.removeModule("DRM");
      spl.removeModule("DTrig");
      spl.removeModule("Drums");
      spl.removeModule("EVA");
      spl.removeModule("FLA");
      spl.removeModule("FLL");
      spl.removeModule("FS6");
      spl.removeModule("Faders");
      spl.removeModule("Frac");
      spl.removeModule("GenScale");
      spl.removeModule("Gendy");
      spl.removeModule("GeneticSuperTerrain");
      spl.removeModule("GeneticTerrain");
      spl.removeModule("HexSeq");
      spl.removeModule("HexSeqExp");
      spl.removeModule("HexSeqP");
      spl.removeModule("Hopa");
      spl.removeModule("Interface");
      spl.removeModule("JTKeys");
      spl.removeModule("JTScaler");
      spl.removeModule("JWS");
      spl.removeModule("L4P");
      spl.removeModule("LWF");
      spl.removeModule("MPad2");
      spl.removeModule("MVerb");
      spl.removeModule("OFS3");
      spl.removeModule("Osc1");
      spl.removeModule("Osc2");
      spl.removeModule("Osc3");
      spl.removeModule("Osc4");
      spl.removeModule("Osc5");
      spl.removeModule("Osc6");
      spl.removeModule("Osc7");
      spl.removeModule("Osc8");
      spl.removeModule("Osc9");
      spl.removeModule("OscA1");
      spl.removeModule("OscP");
      spl.removeModule("OscS");
      spl.removeModule("PHSR");
      spl.removeModule("PHSR2");
      spl.removeModule("PLC");
      spl.removeModule("PPD");
      spl.removeModule("PRB");
      spl.removeModule("PShift");
      spl.removeModule("Pad");
      spl.removeModule("Pad2");
      spl.removeModule("PhO");
      spl.removeModule("PhS");
      spl.removeModule("Plotter");
      spl.removeModule("Pulsar");
      spl.removeModule("RSC");
      spl.removeModule("RTrig");
      spl.removeModule("Ratio");
      spl.removeModule("RndC");
      spl.removeModule("RndG");
      spl.removeModule("RndH");
      spl.removeModule("RndH2");
      spl.removeModule("RndHvs3");
      spl.removeModule("SKF");
      spl.removeModule("SPF");
      spl.removeModule("SPL");
      spl.removeModule("STrig");
      spl.removeModule("SWF");
      spl.removeModule("SuperLFO");
      spl.removeModule("USVF");
      spl.removeModule("X16");
      spl.removeModule("X4");
      spl.removeModule("X6");
      spl.removeModule("X8");
      spl.removeModule("YAC");
    }
}


static void initStatic__MockbaModular()
{
    Plugin* const p = new Plugin;
    pluginInstance__MockbaModular = p;

    const StaticPluginLoader spl(p, "MockbaModular");
    if (spl.ok())
    {
        p->addModel(modelCZOsc);
        p->addModel(modelFiltah);
        p->addModel(modelMaugOsc);
        p->addModel(modelMixah);
        p->addModel(modelPannah);
        p->addModel(modelReVoltah);
        p->addModel(modelShapah);

        spl.removeModule("Blank");
        spl.removeModule("Comparator");
        spl.removeModule("Countah");
        spl.removeModule("CZDblSine");
        spl.removeModule("CZPulse");
        spl.removeModule("CZReso1");
        spl.removeModule("CZReso2");
        spl.removeModule("CZReso3");
        spl.removeModule("CZSaw");
        spl.removeModule("CZSawPulse");
        spl.removeModule("CZSquare");
        spl.removeModule("Dividah");
        spl.removeModule("DualBUFFER");
        spl.removeModule("DualNOT");
        spl.removeModule("DualOR");
        spl.removeModule("DualNOR");
        spl.removeModule("DualAND");
        spl.removeModule("DualNAND");
        spl.removeModule("DualXOR");
        spl.removeModule("DualXNOR");
        spl.removeModule("Feidah");
        spl.removeModule("FeidahS");
        spl.removeModule("Holdah");
        spl.removeModule("MaugSaw");
        spl.removeModule("MaugSaw2");
        spl.removeModule("MaugShark");
        spl.removeModule("MaugSquare");
        spl.removeModule("MaugSquare2");
        spl.removeModule("MaugSquare3");
        spl.removeModule("MaugTriangle");
        spl.removeModule("Mixah3");
        spl.removeModule("PSelectah");
        spl.removeModule("Selectah");
        spl.removeModule("UDPClockMaster");
        spl.removeModule("UDPClockSlave");
    }
}

static void initStatic__MindMeld()
{
    Plugin* const p = new Plugin;
    pluginInstance__MindMeld = p;

    const StaticPluginLoader spl(p, "MindMeldModular");
    if (spl.ok())
    {
      p->addModel(modelPatchMaster);
      p->addModel(modelPatchMasterBlank);

      spl.removeModule("AuxExpander");
      spl.removeModule("AuxExpanderJr");
      spl.removeModule("BassMaster");
      spl.removeModule("BassMasterJr");
      spl.removeModule("EqExpander");
      spl.removeModule("EqMaster");
      spl.removeModule("MSMelder");
      spl.removeModule("MasterChannel");
      spl.removeModule("Meld");
      spl.removeModule("MixMaster");
      spl.removeModule("MixMasterJr");
      spl.removeModule("RouteMasterMono1to5");
      spl.removeModule("RouteMasterMono5to1");
      spl.removeModule("RouteMasterStereo1to5");
      spl.removeModule("RouteMasterStereo5to1");
      spl.removeModule("ShapeMaster");
      spl.removeModule("Unmeld");
    }
}


static void initStatic__MUS_X()
{
    Plugin* const p = new Plugin;
    pluginInstance__MUS_X = p;

    const StaticPluginLoader spl(p, "MUS-X");
    if (spl.ok())
    {
    	p->addModel(musx::modelFilter);
    	p->addModel(musx::modelOnePoleLP);
      spl.removeModule("ADSR");
      spl.removeModule("Delay");
      spl.removeModule("Drift");
      spl.removeModule("LFO");
      spl.removeModule("Last");
      spl.removeModule("ModMatrix");
      spl.removeModule("OnePole");
      spl.removeModule("Oscillators");
      spl.removeModule("SplitStack");
      spl.removeModule("Synth");
      spl.removeModule("Tuner");
    }
}


static void initStatic__Submarine()
{
    Plugin* const p = new Plugin;
    pluginInstance__Submarine = p;

    const StaticPluginLoader spl(p, "Submarine");
    if (spl.ok())
    {
      p->addModel(modelTD202);
      p->addModel(modelTD410);

      spl.removeModule("A0-101");
      spl.removeModule("A0-106");
      spl.removeModule("A0-112");
      spl.removeModule("A0-118");
      spl.removeModule("A0-124");
      spl.removeModule("A0-136");
      spl.removeModule("AG-104");
      spl.removeModule("AG-106");
      spl.removeModule("AG-202");
      spl.removeModule("BB-120");
      spl.removeModule("BP-101");
      spl.removeModule("BP-102");
      spl.removeModule("BP-104");
      spl.removeModule("BP-108");
      spl.removeModule("BP-110");
      spl.removeModule("BP-112");
      spl.removeModule("BP-116");
      spl.removeModule("BP-120");
      spl.removeModule("BP-124");
      spl.removeModule("BP-132");
      spl.removeModule("DN-112");
      spl.removeModule("DO-105");
      spl.removeModule("DO-110");
      spl.removeModule("DO-115");
      spl.removeModule("DO-120");
      spl.removeModule("EN-104");
      spl.removeModule("EO-102");
      spl.removeModule("FF-110");
      spl.removeModule("FF-120");
      spl.removeModule("FF-206");
      spl.removeModule("FF-212");
      spl.removeModule("HS-101");
      spl.removeModule("LA-108");
      spl.removeModule("LA-216");
      spl.removeModule("LD-103");
      spl.removeModule("LD-106");
      spl.removeModule("LT-116");
      spl.removeModule("MZ-909");
      spl.removeModule("NG-106");
      spl.removeModule("NG-112");
      spl.removeModule("NG-206");
      spl.removeModule("OA-103");
      spl.removeModule("OA-105");
      spl.removeModule("OG-104");
      spl.removeModule("OG-106");
      spl.removeModule("OG-202");
      spl.removeModule("PG-104");
      spl.removeModule("PG-112");
      spl.removeModule("PO-101");
      spl.removeModule("PO-102");
      spl.removeModule("PO-204");
      spl.removeModule("SN-101");
      spl.removeModule("SS-112");
      spl.removeModule("SS-208");
      spl.removeModule("SS-212");
      spl.removeModule("SS-220");
      spl.removeModule("SS-221");
      spl.removeModule("TD-116");
      spl.removeModule("TD-316");
      spl.removeModule("TD-510");
      spl.removeModule("TF-101");
      spl.removeModule("TF-102");
      spl.removeModule("TM-105");
      spl.removeModule("VM-101");
      spl.removeModule("VM-102");
      spl.removeModule("VM-104");
      spl.removeModule("VM-201");
      spl.removeModule("VM-202");
      spl.removeModule("VM-204");
      spl.removeModule("WK-101");
      spl.removeModule("WK-205");
      spl.removeModule("WM-101");
      spl.removeModule("WM-102");
      spl.removeModule("XF-101");
      spl.removeModule("XF-102");
      spl.removeModule("XF-104");
      spl.removeModule("XF-201");
      spl.removeModule("XF-202");
      spl.removeModule("XF-301");
      spl.removeModule("XG-104");
      spl.removeModule("XG-106");
      spl.removeModule("XG-202");
      spl.removeModule("XX-219");
    }
}


static void initStatic__surgext()
{
    Plugin* const p = new Plugin;
    pluginInstance__surgext = p;

    const StaticPluginLoader spl(p, "surgext");
    if (spl.ok())
    {
        p->addModel(modelVCOModern);
        p->addModel(modelVCOSine);
        /*
        p->addModel(modelVCOAlias);
        p->addModel(modelVCOClassic);
        p->addModel(modelVCOFM2);
        p->addModel(modelVCOFM3);
        p->addModel(modelVCOSHNoise);
        p->addModel(modelVCOString);
        p->addModel(modelVCOTwist);
        p->addModel(modelVCOWavetable);
        p->addModel(modelVCOWindow);
        */
        spl.removeModule("SurgeXTOSCAlias");
        spl.removeModule("SurgeXTOSCClassic");
        spl.removeModule("SurgeXTOSCFM2");
        spl.removeModule("SurgeXTOSCFM3");
        spl.removeModule("SurgeXTOSCSHNoise");
        spl.removeModule("SurgeXTOSCString");
        spl.removeModule("SurgeXTOSCTwist");
        spl.removeModule("SurgeXTOSCWavetable");
        spl.removeModule("SurgeXTOSCWindow");

        // Add the ported ones
        p->addModel(modelSurgeLFO);
        p->addModel(modelSurgeMixer);
        p->addModel(modelSurgeMixerSlider);
        p->addModel(modelSurgeModMatrix);
        p->addModel(modelSurgeWaveshaper);
        /*
        p->addModel(modelSurgeDelay);
        p->addModel(modelSurgeDelayLineByFreq);
        p->addModel(modelSurgeDelayLineByFreqExpanded);
        p->addModel(modelSurgeDigitalRingMods);
        p->addModel(modelSurgeVCF);
        */
        spl.removeModule("SurgeXTDelay");
        spl.removeModule("SurgeXTDelayLineByFreq");
        spl.removeModule("SurgeXTDelayLineByFreqExpanded");
        spl.removeModule("SurgeXTDigitalRingMod");
        spl.removeModule("SurgeXTVCF");

        p->addModel(modelFXNimbus);
        p->addModel(modelFXBonsai);
        spl.removeModule("SurgeXTFXChorus");
        spl.removeModule("SurgeXTFXChow");
        spl.removeModule("SurgeXTFXCombulator");
        spl.removeModule("SurgeXTDigitalRingMod");
        spl.removeModule("SurgeXTFXDistortion");
        spl.removeModule("SurgeXTFXExciter");
        spl.removeModule("SurgeXTFXEnsemble");
        spl.removeModule("SurgeXTFXFlanger");
        spl.removeModule("SurgeXTFXFrequencyShifter");
        spl.removeModule("SurgeXTFXNeuron");
        spl.removeModule("SurgeXTFXPhaser");
        spl.removeModule("SurgeXTFXResonator");
        spl.removeModule("SurgeXTFXReverb");
        spl.removeModule("SurgeXTFXReverb2");
        spl.removeModule("SurgeXTFXRingMod");
        spl.removeModule("SurgeXTFXRotarySpeaker");
        spl.removeModule("SurgeXTFXSpringReverb");
        spl.removeModule("SurgeXTFXTreeMonster");
        spl.removeModule("SurgeXTFXVocoder");

        /*
        p->addModel(modelEGxVCA);
        p->addModel(modelQuadAD);
        p->addModel(modelUnisonHelper);
        p->addModel(modelUnisonHelperCVExpander);
        */
        p->addModel(modelQuadLFO);
        spl.removeModule("SurgeXTEGxVCA");
        spl.removeModule("SurgeXTQuadAD");
        spl.removeModule("SurgeXTUnisonHelper");
        spl.removeModule("SurgeXTUnisonHelperCVExpander");

        surgext_rack_initialize();
    }
}

static void initStatic__ValleyAudio()
{
    Plugin* const p = new Plugin;
    pluginInstance__ValleyAudio = p;

    const StaticPluginLoader spl(p, "ValleyAudio");
    if (spl.ok())
    {
        p->addModel(modelPlateau);

        spl.removeModule("Amalgam");
        spl.removeModule("Dexter");
        spl.removeModule("Feline");
        spl.removeModule("Interzone");
        spl.removeModule("Terrorform");
        spl.removeModule("Topograph");
        spl.removeModule("uGraph");
    }
}

static void initStatic__Venom()
{
    Plugin* p = new Plugin;
    pluginInstance__Venom = p;

    const StaticPluginLoader spl(p, "Venom");
    if (spl.ok())
    {
#define modelBypass modelVenomBypass
#define modelLogic modelVenomLogic
        p->addModel(modelVCAMix4);
        p->addModel(modelShapedVCA);

        // Defined in manifest but not in plugin 
        spl.removeModule("AuxClone");
        spl.removeModule("BayInput");
        spl.removeModule("BayNorm");
        spl.removeModule("BayOutput");
        spl.removeModule("BenjolinGatesExpander");
        spl.removeModule("BenjolinOsc");
        spl.removeModule("BenjolinVoltsExpander");
        spl.removeModule("BernoulliSwitch");
        spl.removeModule("BernoulliSwitchExpander");
        spl.removeModule("Blocker");
        spl.removeModule("Bypass");
        spl.removeModule("CloneMerge");
        spl.removeModule("HQ");
        spl.removeModule("Knob5");
        spl.removeModule("LinearBeats");
        spl.removeModule("LinearBeatsExpander");
        spl.removeModule("Logic");
        spl.removeModule("Mix4");
        spl.removeModule("Mix4Stereo");
        spl.removeModule("MixFade");
        spl.removeModule("MixFade2");
        spl.removeModule("MixMute");
        spl.removeModule("MixOffset");
        spl.removeModule("MixPan");
        spl.removeModule("MixSend");
        spl.removeModule("MixSolo");
        spl.removeModule("MousePad");
        spl.removeModule("MultiMerge");
        spl.removeModule("MultiSplit");
        spl.removeModule("NORSIQChord2Scale");
        spl.removeModule("NORS_IQ");
        spl.removeModule("Oscillator");
        spl.removeModule("PolyClone");
        spl.removeModule("PolyFade");
        spl.removeModule("PolyOffset");
        spl.removeModule("PolySHASR");
        spl.removeModule("PolyScale");
        spl.removeModule("PolyUnison");
        spl.removeModule("Push5");
        spl.removeModule("QuadVCPolarizer");
        spl.removeModule("Recurse");
        spl.removeModule("RecurseStereo");
        spl.removeModule("Reformation");
        spl.removeModule("RhythmExplorer");
        spl.removeModule("Thru");
        spl.removeModule("VCAMix4Stereo");
        spl.removeModule("VCOUnit");
        spl.removeModule("VenomBlank");
        spl.removeModule("WaveFolder");
        spl.removeModule("WidgetMenuExtender");
        spl.removeModule("WinComp");
#undef modelBypass
#undef modelLogic
    }
}


void initStaticPlugins()
{
    initStatic__Cardinal();
    initStatic__Fundamental();
    initStatic__AnimatedCircuits();
    initStatic__Aria();
    initStatic__AS();
    initStatic__AudibleInstruments();
    initStatic__Bidoo();
    initStatic__BogaudioModules();
    initStatic__countmodula();
    initStatic__HetrickCV();
    initStatic__dbRackModules();
    initStatic__MockbaModular();
    initStatic__MindMeld();
    initStatic__MUS_X();
    initStatic__Submarine();
    initStatic__surgext();
    initStatic__ValleyAudio();
    initStatic__Venom();
}

void destroyStaticPlugins()
{
    for (Plugin* p : plugins)
        delete p;
    plugins.clear();
}

void updateStaticPluginsDarkMode()
{
    const bool darkMode = settings::preferDarkPanels;
    // bogaudio
    {
        Skins& skins(Skins::skins());
        skins._default = darkMode ? "dark" : "light";

        std::lock_guard<std::mutex> lock(skins._defaultSkinListenersLock);
        for (auto listener : skins._defaultSkinListeners) {
            listener->defaultSkinChanged(skins._default);
        }
    }
    // surgext
    {
        surgext_rack_update_theme();
    }
}

}
}
