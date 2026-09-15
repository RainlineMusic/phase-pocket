#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
enum class PocketTheme { Neon, SolidDark, SolidWhite };
class PocketLook final:public juce::LookAndFeel_V4 {
public:
    PocketTheme theme=PocketTheme::Neon;
    bool isDark() const{return theme!=PocketTheme::SolidWhite;}bool isNeon() const{return theme==PocketTheme::Neon;}
    juce::Colour pick(juce::uint32 neon,juce::uint32 dark,juce::uint32 white) const;
    juce::Colour ink() const;juce::Colour muted() const;juce::Colour accent() const;
    juce::Font getTextButtonFont(juce::TextButton&,int) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
};
class ResettableRangeSlider final:public juce::Slider {
public:
    std::function<void()> onReset;
    void mouseDoubleClick(const juce::MouseEvent&) override {if(onReset)onReset();}
    double valueToProportionOfLength(double value) override {return std::log(juce::jlimit(20.,20000.,value)/20.)/std::log(1000.);}
    double proportionOfLengthToValue(double proportion) override {return 20.*std::pow(1000.,juce::jlimit(0.,1.,proportion));}
};
class ModernDial final:public juce::Slider {
public:
    ModernDial(PocketLook&,juce::String,juce::String,juce::String,juce::uint32,bool=false,bool=false,bool=false);void paint(juce::Graphics&) override;
private:
    PocketLook& look;juce::String title,subtitle,unit;juce::uint32 accent;bool infinity,infinityAtMin,compact;
};
class DuckPocketAudioProcessorEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit DuckPocketAudioProcessorEditor(DuckPocketAudioProcessor&);~DuckPocketAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;void paintOverChildren(juce::Graphics&) override;void resized() override;
    void setPanelExpanded(bool);
private:
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    DuckPocketAudioProcessor& audioProcessor;PocketLook look;
    ModernDial influence{look,"Influence","Depth","%",0xff5987ff};
    ModernDial duration{look,"Duration","Sidechain length","ms",0xff32d4cb,true};
    ModernDial outputGain{look,"Output","dB","dB",0xfff1e84b,true,true,true};
    ResettableRangeSlider sidechainRange,processingRange;juce::Slider midSide;
    juce::TextButton settingsButton{"settings"},bypassButton{"power"},panelButton{"panel"};
    juce::TextButton freezeGainButton{"freeze"},freezeScopeButton{"freeze"};
    std::unique_ptr<SliderAttachment> influenceAttach,durationAttach,outputAttach,msAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttach;
    std::unique_ptr<juce::ParameterAttachment> lowAttach,highAttach,processLowAttach,processHighAttach;
    std::unique_ptr<juce::PropertiesFile> preferences;
    std::array<PocketTrace,8192> history{};int cursor=0,filled=0;
    std::vector<PocketTrace> frozenGain,frozenScope;bool gainFrozen=false,scopeFrozen=false;double gainResume=0,scopeResume=0;
    bool expanded=false,ready=false,rangeGesture=false,processRangeGesture=false,capturingBlur=false,bypassTarget=false;
    double resizeStamp=0;float bypassMix=0;double gainWindow=1.,scopeWindow=1.;
    juce::Image blurredSnapshot,chrome;bool chromeValid=false;float chromeScale=1.f;
    juce::Rectangle<int> blurArea,gainArea,scopeArea;
    std::vector<float> bucketLo,bucketHi;
    void timerCallback() override;void syncRange();void syncProcessingRange();void saveSize();
    void setTheme(PocketTheme,bool persist=true);void showSettingsMenu();void setHistoryWindow(bool gain,double seconds);void captureBlurSnapshot();
    void setFrozen(bool gain,bool frozen);void invalidateChrome();void paintChrome(juce::Graphics&);
    void panel(juce::Graphics&,juce::Rectangle<float>);void graph(juce::Graphics&,juce::Rectangle<float>,bool);
    juce::Rectangle<int> scaled(float,float,float,float) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DuckPocketAudioProcessorEditor)
};
