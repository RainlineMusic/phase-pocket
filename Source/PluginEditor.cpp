#include "PluginEditor.h"
namespace {
juce::Font uiFont(float size){
    static const juce::String family=[] {const auto fonts=juce::Font::findAllTypefaceNames();
#if JUCE_WINDOWS
        for(auto name:{"Segoe UI Variable Text","Segoe UI Variable","Segoe UI"})if(fonts.contains(name))return juce::String(name);
#elif JUCE_MAC
        for(auto name:{"SF Pro Text","SF Pro",".AppleSystemUIFont"})if(fonts.contains(name))return juce::String(name);
#else
        if(fonts.contains("Liberation Sans"))return juce::String("Liberation Sans");
#endif
        return juce::Font::getDefaultSansSerifFontName();}();
    juce::Font font(juce::FontOptions(family,size,juce::Font::plain));
#if JUCE_WINDOWS
    static const juce::String style=[] {auto styles=juce::Font::findAllTypefaceStyles(family);for(auto s:{"Semilight Text","Semilight","SemiLight"})if(styles.contains(s))return juce::String(s);return juce::String();}();if(style.isNotEmpty())font.setTypefaceStyle(style);
#endif
    return font;
}
// Written as raw UTF-8 bytes on purpose: a \u escape in a narrow literal gets
// transcoded to the compiler's execution charset (MSVC without /utf-8 turns it
// into '?'), which is exactly how the dials lost their infinity sign.
constexpr const char* kInfinity="\xe2\x88\x9e";
constexpr const char* kMinusInfinity="-\xe2\x88\x9e";
void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<float> r,float size,juce::Colour c,int align=juce::Justification::centredLeft,float glow=0.f){
    g.setFont(uiFont(size));
    if(glow>0.f){g.setColour(c.withAlpha(glow));const float o[8][2]={{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{1,1},{-1,1},{1,-1}};for(auto& d:o)g.drawText(s,r.translated(d[0],d[1]),align);}
    g.setColour(c);g.drawText(s,r,align);
}
void stroke(juce::Graphics& g,const juce::Path& p,juce::Colour c,float width){g.setColour(c);g.strokePath(p,juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
juce::String hz(double v){return v>=1000?juce::String(v/1000,1)+" kHz":juce::String(juce::roundToInt(v))+" Hz";}
juce::String timeLabel(double seconds){const int ms=juce::roundToInt(seconds*1000);return ms<1000?juce::String(ms)+" ms":juce::String(seconds,seconds==std::floor(seconds)?0:2)+" s";}
constexpr std::array<double,6> windows{{.1,.25,.5,1.,2.,5.}};
void glowStroke(juce::Graphics& g,const juce::Path& p,juce::Colour c,float width,bool glow){
    if(glow)stroke(g,p,c.withAlpha(.13f),width*2.8f);
    stroke(g,p,c,width);
}
juce::Path smoothPath(const std::vector<juce::Point<float>>& points){
    juce::Path p;if(points.empty())return p;p.startNewSubPath(points.front());
    for(size_t i=1;i+1<points.size();++i){auto mid=(points[i]+points[i+1])*.5f;p.quadraticTo(points[i],mid);}
    if(points.size()>1)p.lineTo(points.back());return p;
}
}
juce::Colour PocketLook::pick(juce::uint32 neon,juce::uint32 dark,juce::uint32 white) const {
    if(theme==PocketTheme::Neon)return juce::Colour(neon);
    if(theme==PocketTheme::SolidDark)return juce::Colour(dark);
    if(theme==PocketTheme::SolidWhite)return juce::Colour(white);
    // Amber: preserve the semantic brightness/alpha of the dark palette while
    // moving it onto the warm brown/orange ramp from the reference UI.
    const auto source=juce::Colour(dark?dark:neon);
    const float b=source.getPerceivedBrightness();
    const float sat=juce::jmap(b,0.f,1.f,.72f,.20f);
    const float value=juce::jlimit(.025f,1.f,b*.93f+.018f);
    return juce::Colour::fromHSV(.078f,sat,value,source.getFloatAlpha());
}
juce::Colour PocketLook::ink() const{return theme==PocketTheme::Amber?juce::Colour(0xfff1d6c8):pick(0xffdce5fc,0xfff0f0f0,0xff242527);}
juce::Colour PocketLook::muted() const{return theme==PocketTheme::Amber?juce::Colour(0xffbd9c87):pick(0xff97a6c2,0xffa7a7a7,0xff6b6c70);}
juce::Colour PocketLook::accent() const{return theme==PocketTheme::Amber?juce::Colour(0xffff7126):pick(0xff35d6dc,0xffe8e8e8,0xff2f74d0);}
juce::Colour PocketLook::accent2() const{return theme==PocketTheme::Amber?juce::Colour(0xffffd164):(isNeon()?juce::Colour(0xff35d6dc):accent());}
juce::Colour PocketLook::themedAccent(juce::uint32 neon) const {return isAmber()?(juce::Colour(neon).getHue()>.3f?juce::Colour(0xffffd164):juce::Colour(0xffff7126)):juce::Colour(neon);}
juce::Font PocketLook::getTextButtonFont(juce::TextButton&,int){return uiFont(15);}
void PocketLook::drawButtonBackground(juce::Graphics& g,juce::Button&,const juce::Colour&,bool hover,bool down){auto r=g.getClipBounds().toFloat().reduced(2);if(hasGlow()){g.setColour(juce::Colours::black.withAlpha(.35f));g.fillRoundedRectangle(r.translated(0,2),9);g.setGradientFill(juce::ColourGradient(pick(hover?0xff26354c:0xff172130,0,0),0,r.getY(),pick(0xff070d16,0,0),0,r.getBottom(),false));g.fillRoundedRectangle(r,9);}else{auto fill=pick(0,hover?0xff3b3b3b:0xff292929,hover?0xffffffff:0xfff4f4f4);if(down)fill=fill.contrasting(.08f);g.setColour(fill);g.fillRoundedRectangle(r,8);}g.setColour(pick(0xff34445b,0xff555555,0xffc2c3c6));g.drawRoundedRectangle(r,8,.8f);}
void PocketLook::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool){
    auto r=b.getLocalBounds().toFloat();auto name=b.getButtonText();
    auto c=r.getCentre();float s=juce::jmin(r.getWidth(),r.getHeight())/40;juce::Path p;
    if(name=="power"){p.addCentredArc(0,1,8,8,0,.65f,juce::MathConstants<float>::twoPi-.65f,true);p.startNewSubPath(0,-10);p.lineTo(0,-1);p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,ink(),1.7f*s);return;}
    if(name=="panel"){float dir=b.getToggleState()?-1.f:1.f;for(float y:{-4.f,3.f}){p.startNewSubPath(-5,y-2*dir);p.lineTo(0,y+2*dir);p.lineTo(5,y-2*dir);}p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,ink().withAlpha(b.isEnabled()?1.f:.35f),1.7f*s);return;}
    if(name=="freeze"){
        // small snowflake: three crossed arms with barbs, lit up while frozen
        for(int arm=0;arm<3;++arm){
            const float a=juce::MathConstants<float>::halfPi+float(arm)*juce::MathConstants<float>::pi/3.f;
            const float dx=std::cos(a),dy=std::sin(a);
            p.startNewSubPath(-dx*10,-dy*10);p.lineTo(dx*10,dy*10);
            for(float sign:{-1.f,1.f})for(float at:{5.5f,9.f}){
                const float bx=dx*at*sign,by=dy*at*sign;
                for(float spread:{.62f,-.62f}){
                    const float ax=std::cos(a+spread),ay=std::sin(a+spread);
                    p.startNewSubPath(bx,by);p.lineTo(bx+ax*3.2f*sign,by+ay*3.2f*sign);
                }
            }
        }
        p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));
        stroke(g,p,(b.getToggleState()?accent():ink()).withAlpha(b.isEnabled()?1.f:.35f),1.35f*s);return;
    }
    constexpr int teeth=10;for(int i=0;i<teeth*4;++i){float a=float(i)*juce::MathConstants<float>::twoPi/float(teeth*4)-juce::MathConstants<float>::halfPi;float radius=(i%4==1||i%4==2)?10.f:7.7f;auto pt=juce::Point<float>(std::cos(a)*radius,std::sin(a)*radius);if(i==0)p.startNewSubPath(pt);else p.lineTo(pt);}p.closeSubPath();p.applyTransform(juce::AffineTransform::scale(s).translated(c.x,c.y));stroke(g,p,ink(),1.65f*s);g.setColour(ink());g.drawEllipse(c.x-3.2f*s,c.y-3.2f*s,6.4f*s,6.4f*s,1.65f*s);
}
void PocketLook::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float minPos,float maxPos,juce::Slider::SliderStyle style,juce::Slider&){float cy=float(y)+float(h)*.5f,left=style==juce::Slider::TwoValueHorizontal?minPos:float(x),right=style==juce::Slider::TwoValueHorizontal?maxPos:pos;g.setColour(pick(0xff172334,0xff343434,0xffc9cacc));g.fillRoundedRectangle(float(x),cy-3,float(w),6,3);if(hasGlow())g.setGradientFill(juce::ColourGradient(isAmber()?juce::Colour(0xffff7126):juce::Colour(0xff4e78ff),left,cy,isAmber()?juce::Colour(0xffffd164):juce::Colour(0xff35d6dc),right+1,cy,false));else g.setColour(pick(0,0xffd7d7d7,0xff303236));g.fillRoundedRectangle(left,cy-3,juce::jmax(.1f,right-left),6,3);auto thumb=[&](float px){g.setColour(pick(0xffdfebff,0xffeeeeee,0xfff2f2f3));g.fillEllipse(px-6,cy-6,12,12);g.setColour(pick(0xff638bda,0xff777777,0xff85878b));g.drawEllipse(px-6,cy-6,12,12,1);};if(style==juce::Slider::TwoValueHorizontal){thumb(minPos);thumb(maxPos);}else thumb(pos);}
ModernDial::ModernDial(PocketLook& l,juce::String t,juce::String sub,juce::String u,juce::uint32 a,bool inf,bool infMin,bool compactDial):look(l),title(t),subtitle(sub),unit(u),accent(a),infinity(inf),infinityAtMin(infMin),compact(compactDial){setSliderStyle(juce::Slider::RotaryVerticalDrag);setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);setName(t);setWantsKeyboardFocus(true);}
void ModernDial::paint(juce::Graphics& g){
    const float design=compact?100.f:200.f,s=float(getWidth())/design,r=(compact?34.f:77.f)*s,ring=r+(compact?5.f:8.f)*s;juce::Point<float> c(float(getWidth())*.5f,float(getHeight())*.5f);auto face=juce::Rectangle<float>(2*r,2*r).withCentre(c);
    if(look.hasGlow())for(int i=compact?3:5;i>0;--i){g.setColour(look.themedAccent(accent).withAlpha(.035f));g.fillEllipse(face.expanded(float(i)*2*s));}
    if(look.hasGlow())g.setGradientFill(juce::ColourGradient(look.pick(0xff233349,0,0),c.x-r,c.y-r,look.pick(0xff060b12,0,0),c.x+r,c.y+r,false));else g.setColour(look.pick(0,0xff252525,0xfff8f8f8));g.fillEllipse(face);g.setColour(look.pick(0xff344963,0xff555555,0xffbabcc0));g.drawEllipse(face,s);
    float proportion=float(valueToProportionOfLength(getValue())),start=juce::MathConstants<float>::pi*1.25f,end=start+juce::MathConstants<float>::pi*1.5f*proportion;juce::Path track,arc;track.addCentredArc(c.x,c.y,ring,ring,0,start,juce::MathConstants<float>::pi*2.75f,true);stroke(g,track,look.pick(0xff050a11,0xff101010,0xffc7c8ca),(compact?4.f:6.f)*s);
    if(proportion>0){arc.addCentredArc(c.x,c.y,ring,ring,0,start,end,true);auto a=look.hasGlow()?look.themedAccent(accent):look.pick(0,0xffe8e8e8,0xff303235);if(look.hasGlow()){stroke(g,arc,a.withAlpha(.10f),(compact?8.f:14.f)*s);stroke(g,arc,a.withAlpha(.18f),(compact?6.f:9.f)*s);g.setGradientFill(juce::ColourGradient(a.brighter(.2f),c.x-r,c.y+r,look.isAmber()?look.accent2():juce::Colour(0xff8c86ed),c.x+r,c.y-r,false));}else g.setColour(a);g.strokePath(arc,juce::PathStrokeType((compact?3.5f:5.f)*s,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
    auto marker=juce::Point<float>(c.x+ring*std::sin(end),c.y-ring*std::cos(end));g.setColour(look.pick(0xffdeebff,0xfff4f4f4,0xff303030));g.fillEllipse(marker.x-(compact?3.5f:5.5f)*s,marker.y-(compact?3.5f:5.5f)*s,(compact?7.f:11.f)*s,(compact?7.f:11.f)*s);
    const bool showInfinity=infinity&&((infinityAtMin&&proportion<.0005f)||(!infinityAtMin&&proportion>.9995f));juce::String value;if(showInfinity)value=juce::String::fromUTF8(infinityAtMin?kMinusInfinity:kInfinity);else if(unit=="dB")value=juce::String(double(juce::roundToInt(getValue()*100.))/100.,2);else value=juce::String(getValue(),unit=="%"?1:0)+(unit=="%"?"%":" ms");
    if(compact){text(g,title,{c.x-r,c.y-22*s,2*r,15*s},12*s,look.ink(),juce::Justification::centred);text(g,value,{c.x-r,c.y-7*s,2*r,23*s},19*s,look.ink(),juce::Justification::centred);text(g,subtitle,{c.x-r,c.y+16*s,2*r,13*s},10*s,look.muted(),juce::Justification::centred);}else{text(g,title,{c.x-r,c.y-46*s,2*r,25*s},18*s,look.ink(),juce::Justification::centred);text(g,value,{c.x-r,c.y-19*s,2*r,43*s},unit=="%"?33*s:30*s,look.ink(),juce::Justification::centred);text(g,subtitle,{c.x-r,c.y+28*s,2*r,23*s},13*s,look.muted(),juce::Justification::centred);}
}
DuckPocketAudioProcessorEditor::DuckPocketAudioProcessorEditor(DuckPocketAudioProcessor& p):AudioProcessorEditor(&p),audioProcessor(p){
    juce::PropertiesFile::Options o;o.applicationName="DuckPocket";o.filenameSuffix="settings";o.folderName="RainlineMusic";o.osxLibrarySubFolder="Application Support";preferences=std::make_unique<juce::PropertiesFile>(o);
    // Solid Dark is the default theme; a stored preference still wins.
    auto saved=preferences->getValue("duckPocket.ui.theme",preferences->getValue("phasePocket.ui.theme","solidDark"));setTheme(saved=="neon"?PocketTheme::Neon:(saved=="amber"?PocketTheme::Amber:(saved=="solidWhite"?PocketTheme::SolidWhite:PocketTheme::SolidDark)),false);
    gainWindow=preferences->getDoubleValue("duckPocket.ui.graphWindow",preferences->getDoubleValue("duckPocket.ui.gainWindow",preferences->getDoubleValue("phasePocket.ui.gainWindow",1.)));scopeWindow=gainWindow;
    setLookAndFeel(&look);setOpaque(true);setResizable(true,true);
    for(auto* c:std::initializer_list<juce::Component*>{&influence,&duration,&outputGain,&sidechainRange,&processingRange,&midSide,&settingsButton,&bypassButton,&panelButton,&freezeButton})addAndMakeVisible(c);
    influenceAttach=std::make_unique<SliderAttachment>(p.parameters,"amount",influence);durationAttach=std::make_unique<SliderAttachment>(p.parameters,"duration",duration);outputAttach=std::make_unique<SliderAttachment>(p.parameters,"outputGain",outputGain);outputGain.setDoubleClickReturnValue(true,0);midSide.setSliderStyle(juce::Slider::LinearHorizontal);midSide.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);midSide.setDoubleClickReturnValue(true,0);msAttach=std::make_unique<SliderAttachment>(p.parameters,"msBalance",midSide);midSide.onValueChange=[this]{repaint(scaled(90,808,780,52));};
    bypassButton.setClickingTogglesState(true);bypassAttach=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bypass",bypassButton);settingsButton.onClick=[this]{showSettingsMenu();};panelButton.onClick=[this]{if(!bypassTarget)setPanelExpanded(!expanded);};
    // One button freezes and resumes both graphs at once.
    freezeButton.setClickingTogglesState(true);freezeButton.setTooltip("Freeze both graphs");
    freezeButton.onClick=[this]{setFrozen(freezeButton.getToggleState());};
    auto setupRange=[](juce::Slider& s){s.setSliderStyle(juce::Slider::TwoValueHorizontal);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setRange(20,20000,0.);};setupRange(sidechainRange);setupRange(processingRange);
    lowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scLow"),[this](float){syncRange();});highAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("scHigh"),[this](float){syncRange();});lowAttach->sendInitialUpdate();highAttach->sendInitialUpdate();
    sidechainRange.onDragStart=[this]{rangeGesture=true;lowAttach->beginGesture();highAttach->beginGesture();};sidechainRange.onValueChange=[this]{float a=float(sidechainRange.getMinValue()),b=float(sidechainRange.getMaxValue());repaint(scaled(24,640,912,82));if(rangeGesture){lowAttach->setValueAsPartOfGesture(a);highAttach->setValueAsPartOfGesture(b);}else{lowAttach->setValueAsCompleteGesture(a);highAttach->setValueAsCompleteGesture(b);}};sidechainRange.onDragEnd=[this]{lowAttach->endGesture();highAttach->endGesture();rangeGesture=false;syncRange();};sidechainRange.onReset=[this]{lowAttach->setValueAsCompleteGesture(20);highAttach->setValueAsCompleteGesture(20000);syncRange();};
    processLowAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processLow"),[this](float){syncProcessingRange();});processHighAttach=std::make_unique<juce::ParameterAttachment>(*p.parameters.getParameter("processHigh"),[this](float){syncProcessingRange();});processLowAttach->sendInitialUpdate();processHighAttach->sendInitialUpdate();
    processingRange.onDragStart=[this]{processRangeGesture=true;processLowAttach->beginGesture();processHighAttach->beginGesture();};processingRange.onValueChange=[this]{float a=float(processingRange.getMinValue()),b=float(processingRange.getMaxValue());repaint(scaled(24,720,912,82));if(processRangeGesture){processLowAttach->setValueAsPartOfGesture(a);processHighAttach->setValueAsPartOfGesture(b);}else{processLowAttach->setValueAsCompleteGesture(a);processHighAttach->setValueAsCompleteGesture(b);}};processingRange.onDragEnd=[this]{processLowAttach->endGesture();processHighAttach->endGesture();processRangeGesture=false;syncProcessingRange();};processingRange.onReset=[this]{processLowAttach->setValueAsCompleteGesture(20);processHighAttach->setValueAsCompleteGesture(20000);syncProcessingRange();};
    duration.setTooltip("Total key duration: 1 ms to infinity; last 20% fades to zero");outputGain.setTooltip("Output gain: minus infinity to +6 dB; double-click resets to 0 dB");influence.setTooltip("Ducking depth");bypassButton.setTooltip("Enable / bypass processing");settingsButton.setTooltip("Settings");panelButton.setTooltip("Show sidechain, processing range and M/S balance");processingRange.setTooltip("Frequency range affected by ducking; 20 Hz to 20 kHz keeps the plain wideband duck");
    expanded=p.editorExpanded.load();
    if(!expanded)expanded=preferences->getBoolValue("duckPocket.ui.expanded",false);
    int width=p.editorWidth.load();if(width<800||width>1500)width=preferences->getIntValue("duckPocket.ui.width",preferences->getIntValue("phasePocket.ui.width",960));width=juce::jlimit(800,1500,width);
    const double designHeight=expanded?890.:636.;
    setResizeLimits(800,530,1500,1400);getConstrainer()->setFixedAspectRatio(960./designHeight);setSize(width,juce::roundToInt(width*designHeight/960.));
    panelButton.setToggleState(expanded,juce::dontSendNotification);
    for(auto* c:{static_cast<juce::Component*>(&sidechainRange),static_cast<juce::Component*>(&processingRange),static_cast<juce::Component*>(&midSide)})c->setVisible(expanded);
    pathPoints.reserve(4096);pathTop.reserve(4096);pathBottom.reserve(4096);bucketLo.reserve(4096);bucketHi.reserve(4096);bucketScratch.reserve(4096);
    ready=true;p.editorWidth.store(width);p.editorExpanded.store(expanded);PocketTrace discard;while(p.popTrace(discard)){}p.editorOpen.store(true);frameTick();
    // VBlank-driven rendering, capped to 60 fps. This avoids timer jitter and
    // never queues frames faster than the monitor can present them.
    vblank=std::make_unique<juce::VBlankAttachment>(this,[this]{
        const double now=juce::Time::getMillisecondCounterHiRes();
        if(nextFrameMs==0)nextFrameMs=now;
        if(now+0.25<nextFrameMs)return;
        do nextFrameMs+=1000.0/60.0; while(nextFrameMs<now-1000.0/60.0);
        frameTick();
    });
}
DuckPocketAudioProcessorEditor::~DuckPocketAudioProcessorEditor(){vblank.reset();saveSize();audioProcessor.editorOpen.store(false);if(rangeGesture){lowAttach->endGesture();highAttach->endGesture();}if(processRangeGesture){processLowAttach->endGesture();processHighAttach->endGesture();}setLookAndFeel(nullptr);}
void DuckPocketAudioProcessorEditor::saveSize(){if(!ready||!preferences)return;audioProcessor.editorWidth.store(getWidth());audioProcessor.editorExpanded.store(expanded);preferences->setValue("duckPocket.ui.width",getWidth());preferences->setValue("duckPocket.ui.expanded",expanded);preferences->saveIfNeeded();resizeStamp=0;}
void DuckPocketAudioProcessorEditor::invalidateChrome(){chromeValid=false;repaint();}
void DuckPocketAudioProcessorEditor::setTheme(PocketTheme t,bool persist){look.theme=t;if(persist&&preferences){preferences->setValue("duckPocket.ui.theme",t==PocketTheme::Neon?"neon":(t==PocketTheme::Amber?"amber":(t==PocketTheme::SolidDark?"solidDark":"solidWhite")));preferences->saveIfNeeded();}chromeValid=false;repaint();for(auto* c:getChildren())c->repaint();if(bypassMix>0)juce::MessageManager::callAsync([safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this)]{if(safe)safe->captureBlurSnapshot();});}
void DuckPocketAudioProcessorEditor::setHistoryWindow(bool,double seconds){gainWindow=scopeWindow=seconds;preferences->setValue("duckPocket.ui.graphWindow",seconds);preferences->setValue("duckPocket.ui.gainWindow",seconds);preferences->setValue("duckPocket.ui.scopeWindow",seconds);preferences->saveIfNeeded();invalidateChrome();}
// A single snowflake button freezes and resumes both graphs together.
void DuckPocketAudioProcessorEditor::setFrozen(bool frozen){
    gainFrozen=scopeFrozen=frozen;frozenGain.clear();frozenScope.clear();
    if(frozen){
        frozenGain.reserve(size_t(filled));
        for(int i=0;i<filled;++i)frozenGain.push_back(history[size_t((cursor-filled+i+historyCapacity)%historyCapacity)]);
        frozenScope=frozenGain;
    }else{
        const double resume=filled?history[size_t((cursor+historyCapacity-1)%historyCapacity)].time:0.;
        gainResume=scopeResume=resume;
    }
    freezeButton.setToggleState(frozen,juce::dontSendNotification);freezeButton.repaint();
    repaint(gainArea);repaint(scopeArea);
}
void DuckPocketAudioProcessorEditor::showSettingsMenu(){juce::PopupMenu root,window,theme;for(size_t i=0;i<windows.size();++i)window.addItem(int(i)+1,timeLabel(windows[i]),true,std::abs(gainWindow-windows[i])<1e-6);theme.addItem(201,"Neon",true,look.theme==PocketTheme::Neon);theme.addItem(204,"Amber",true,look.theme==PocketTheme::Amber);theme.addItem(202,"Solid Dark",true,look.theme==PocketTheme::SolidDark);theme.addItem(203,"Solid White",true,look.theme==PocketTheme::SolidWhite);root.addSubMenu("Graph window",window);root.addSeparator();root.addSubMenu("Theme",theme);auto safe=juce::Component::SafePointer<DuckPocketAudioProcessorEditor>(this);root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsButton),[safe](int id){if(!safe||id==0)return;if(id>=1&&id<=6)safe->setHistoryWindow(true,windows[size_t(id-1)]);else if(id==201)safe->setTheme(PocketTheme::Neon);else if(id==204)safe->setTheme(PocketTheme::Amber);else if(id==202)safe->setTheme(PocketTheme::SolidDark);else if(id==203)safe->setTheme(PocketTheme::SolidWhite);});}
void DuckPocketAudioProcessorEditor::setPanelExpanded(bool open){expanded=open;audioProcessor.editorExpanded.store(open);if(preferences){preferences->setValue("duckPocket.ui.expanded",open);preferences->saveIfNeeded();}panelButton.setToggleState(open,juce::dontSendNotification);panelButton.setTooltip(open?"Hide advanced controls":"Show sidechain, processing range and M/S balance");for(auto* c:{static_cast<juce::Component*>(&sidechainRange),static_cast<juce::Component*>(&processingRange),static_cast<juce::Component*>(&midSide)})c->setVisible(open);double height=open?890.:636.;getConstrainer()->setFixedAspectRatio(960./height);setSize(getWidth(),juce::roundToInt(getWidth()*height/960.));resized();invalidateChrome();}
void DuckPocketAudioProcessorEditor::syncRange(){if(rangeGesture)return;float a=audioProcessor.parameters.getRawParameterValue("scLow")->load(),b=audioProcessor.parameters.getRawParameterValue("scHigh")->load();sidechainRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(24,640,912,82));}
void DuckPocketAudioProcessorEditor::syncProcessingRange(){if(processRangeGesture)return;float a=audioProcessor.parameters.getRawParameterValue("processLow")->load(),b=audioProcessor.parameters.getRawParameterValue("processHigh")->load();processingRange.setMinAndMaxValues(juce::jmin(a,b),juce::jmax(a,b),juce::dontSendNotification);repaint(scaled(24,720,912,82));}
juce::Rectangle<int> DuckPocketAudioProcessorEditor::scaled(float x,float y,float w,float h) const {float s=float(getWidth())/960;return{juce::roundToInt(x*s),juce::roundToInt(y*s),juce::roundToInt(w*s),juce::roundToInt(h*s)};}
void DuckPocketAudioProcessorEditor::resized(){settingsButton.setBounds(scaled(838,22,40,40));bypassButton.setBounds(scaled(888,22,40,40));influence.setBounds(scaled(725,104,200,220));duration.setBounds(scaled(725,350,200,220));outputGain.setBounds(scaled(848,293,84,84));outputGain.toFront(false);panelButton.setBounds(scaled(24,590,36,30));freezeButton.setBounds(scaled(650,541,24,24));sidechainRange.setBounds(scaled(52,684,856,36));processingRange.setBounds(scaled(52,764,856,36));midSide.setBounds(scaled(340,824,280,30));blurArea=scaled(12,82,936,expanded?795:542);gainArea=scaled(24,94,674,230);scopeArea=scaled(24,344,674,230);blurredSnapshot={};chromeValid=false;if(ready){audioProcessor.editorWidth.store(getWidth());resizeStamp=juce::Time::getMillisecondCounterHiRes();}}
void DuckPocketAudioProcessorEditor::panel(juce::Graphics& g,juce::Rectangle<float> r){if(look.hasGlow()){for(int i=4;i>0;--i){g.setColour(juce::Colours::black.withAlpha(.05f));g.fillRoundedRectangle(r.expanded(float(i)).translated(0,float(i)*2),16+float(i));}g.setGradientFill(juce::ColourGradient(look.pick(0xff182538,0,0),r.getX(),r.getY(),look.pick(0xff060c14,0,0),r.getRight(),r.getBottom(),false));}else g.setColour(look.pick(0,0xff202020,0xffffffff));g.fillRoundedRectangle(r,16);g.setColour(look.pick(0xff304259,0xff505050,0xffbfc0c2));g.drawRoundedRectangle(r,16,1);}
/*
    Only the moving traces are painted here. Everything static (panels, grids,
    labels, gradients and their glow passes) lives in the cached chrome image, so
    a timer tick no longer re-rasterises the whole window. That was the real cost
    of running several open editors in one host: each one repainted every pixel,
    with glow, 60 times a second on the shared message thread.
*/
void DuckPocketAudioProcessorEditor::graph(juce::Graphics& g,juce::Rectangle<float> box,bool gain){
    const bool glow=look.hasGlow();
    const juce::Rectangle<float> plot(box.getX()+18,box.getY()+47,box.getWidth()-70,box.getHeight()-87);
    const bool frozen=gain?gainFrozen:scopeFrozen;
    const auto& snap=gain?frozenGain:frozenScope;
    const int count=frozen?int(snap.size()):filled;
    if(count<2)return;
    auto at=[&](int i)->const PocketTrace& {return frozen?snap[size_t(i)]:history[size_t((cursor-count+i+historyCapacity)%historyCapacity)];};
    const double window=gain?gainWindow:scopeWindow,now=at(count-1).time,resume=gain?gainResume:scopeResume;
    const auto label=look.ink();
    juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(plot.toNearestInt());
    const int columns=juce::jlimit(2,4096,juce::roundToInt(plot.getWidth()));
    const float span=plot.getWidth()/float(columns-1);
    auto column=[&](double age){return juce::jlimit(0,columns-1,columns-1-juce::roundToInt(age/window*(columns-1)));};

    // Fill holes between real trace buckets. At the 100 ms view there are fewer
    // audio packets than physical pixels; interpolation prevents long stair-step
    // diagonals without inventing peaks (the real bucket extrema remain anchors).
    auto interpolate=[&](bool pair){
        int previous=-1;
        for(int c=0;c<columns;++c){
            const bool valid=pair?bucketHi[size_t(c)]>=bucketLo[size_t(c)]:bucketLo[size_t(c)]<=1.f;
            if(!valid)continue;
            if(previous>=0&&c>previous+1){
                const float lo0=bucketLo[size_t(previous)],lo1=bucketLo[size_t(c)];
                const float hi0=pair?bucketHi[size_t(previous)]:0.f,hi1=pair?bucketHi[size_t(c)]:0.f;
                for(int x=previous+1;x<c;++x){const float t=float(x-previous)/float(c-previous);bucketLo[size_t(x)]=lo0+t*(lo1-lo0);if(pair)bucketHi[size_t(x)]=hi0+t*(hi1-hi0);}
            }
            previous=c;
        }
    };

    if(gain){
        bucketLo.assign(size_t(columns),2.f);
        for(int i=count-1;i>=0;--i){const auto& v=at(i);const double age=now-v.time;if(age>window)break;if(!frozen&&v.time<resume)continue;if(age<0||!std::isfinite(v.gain))continue;const size_t c=size_t(column(age));bucketLo[c]=juce::jmin(bucketLo[c],juce::jlimit(0.f,1.f,v.gain));}
        interpolate(false);
        // Display-only 3-tap reconstruction filter. It removes the one-pixel
        // bucket phase changes that shimmer while the trace scrolls; audio and
        // stored extrema are untouched.
        bucketScratch=bucketLo;
        for(int c=1;c+1<columns;++c)if(bucketLo[size_t(c-1)]<=1.f&&bucketLo[size_t(c)]<=1.f&&bucketLo[size_t(c+1)]<=1.f)bucketScratch[size_t(c)]=.25f*bucketLo[size_t(c-1)]+.5f*bucketLo[size_t(c)]+.25f*bucketLo[size_t(c+1)];
        bucketLo.swap(bucketScratch);pathPoints.clear();
        for(int c=0;c<columns;++c)if(bucketLo[size_t(c)]<=1.f)pathPoints.push_back({plot.getX()+float(c)*span,plot.getBottom()-bucketLo[size_t(c)]*plot.getHeight()});
        if(pathPoints.size()>1)glowStroke(g,smoothPath(pathPoints),label,1.65f,glow);
        return;
    }

    for(int kind=1;kind>=0;--kind){
        bucketLo.assign(size_t(columns),1.f);bucketHi.assign(size_t(columns),-1.f);
        bool any=false;
        for(int i=count-1;i>=0;--i){const auto& v=at(i);const double age=now-v.time;if(age>window)break;if(!frozen&&v.time<resume)continue;if(age<0)continue;const float lo=kind?v.keyLo:v.outLo,hi=kind?v.keyHi:v.outHi;if(!std::isfinite(lo)||!std::isfinite(hi))continue;const size_t c=size_t(column(age));bucketLo[c]=juce::jmin(bucketLo[c],juce::jlimit(-1.f,1.f,lo));bucketHi[c]=juce::jmax(bucketHi[c],juce::jlimit(-1.f,1.f,hi));any=true;}
        if(!any)continue;
        interpolate(true);
        bucketScratch=bucketLo;
        for(int c=1;c+1<columns;++c)if(bucketHi[size_t(c-1)]>=bucketLo[size_t(c-1)]&&bucketHi[size_t(c)]>=bucketLo[size_t(c)]&&bucketHi[size_t(c+1)]>=bucketLo[size_t(c+1)])bucketScratch[size_t(c)]=.25f*bucketLo[size_t(c-1)]+.5f*bucketLo[size_t(c)]+.25f*bucketLo[size_t(c+1)];
        bucketLo.swap(bucketScratch);bucketScratch=bucketHi;
        for(int c=1;c+1<columns;++c)if(bucketHi[size_t(c-1)]>=bucketLo[size_t(c-1)]&&bucketHi[size_t(c)]>=bucketLo[size_t(c)]&&bucketHi[size_t(c+1)]>=bucketLo[size_t(c+1)])bucketScratch[size_t(c)]=.25f*bucketHi[size_t(c-1)]+.5f*bucketHi[size_t(c)]+.25f*bucketHi[size_t(c+1)];
        bucketHi.swap(bucketScratch);pathTop.clear();pathBottom.clear();
        // No centre trace in silence: only columns whose envelope is visibly above
        // the noise floor are drawn. Each audible run becomes its own shape that
        // tapers to the centre line at both ends, so nothing is drawn across gaps.
        constexpr float silenceThreshold=.004f;
        const float midY=plot.getCentreY();
        auto active=[&](int c){const float lo=bucketLo[size_t(c)],hi=bucketHi[size_t(c)];return hi>=lo&&juce::jmax(std::abs(lo),std::abs(hi))>silenceThreshold;};
        const auto colour=kind?(look.isAmber()?look.accent2():look.pick(0xff3794d4,0xff8e8e8e,0xff59616b)):label;
        for(int c=0;c<columns;){
            if(!active(c)){++c;continue;}
            int e=c;while(e+1<columns&&active(e+1))++e;
            pathTop.clear();pathBottom.clear();
            const float x0=plot.getX()+float(c-1)*span,x1=plot.getX()+float(e+1)*span;
            pathTop.push_back({x0,midY});pathBottom.push_back({x0,midY});
            for(int i=c;i<=e;++i){const float x=plot.getX()+float(i)*span;pathTop.push_back({x,midY-bucketHi[size_t(i)]*plot.getHeight()*.5f});pathBottom.push_back({x,midY-bucketLo[size_t(i)]*plot.getHeight()*.5f});}
            pathTop.push_back({x1,midY});pathBottom.push_back({x1,midY});
            c=e+1;
            juce::Path body;
            body.startNewSubPath(pathTop.front());
            for(size_t i=1;i+1<pathTop.size();++i)body.quadraticTo(pathTop[i],(pathTop[i]+pathTop[i+1])*.5f);
            body.lineTo(pathTop.back());
            body.lineTo(pathBottom.back());
            for(size_t i=pathBottom.size()-1;i>1;--i)body.quadraticTo(pathBottom[i-1],(pathBottom[i-1]+pathBottom[i-2])*.5f);
            body.lineTo(pathBottom.front());body.closeSubPath();
            if(glow){g.setColour(colour.withAlpha(.12f));g.strokePath(body,juce::PathStrokeType(3.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
            g.setColour(colour.withAlpha(kind?.28f:.36f));g.fillPath(body);
            g.setColour(colour.withAlpha(kind?.86f:.96f));g.strokePath(body,juce::PathStrokeType(1.25f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
    }
}
void DuckPocketAudioProcessorEditor::paintChrome(juce::Graphics& g){
    const float physicalScale=juce::jlimit(.75f,4.f,g.getInternalContext().getPhysicalPixelScaleFactor());
    const int w=juce::jmax(1,juce::roundToInt(float(getWidth())*physicalScale));
    const int h=juce::jmax(1,juce::roundToInt(float(getHeight())*physicalScale));
    if(!chromeValid||!chrome.isValid()||chrome.getWidth()!=w||chrome.getHeight()!=h||std::abs(chromeScale-physicalScale)>.001f){
        chrome=juce::Image(juce::Image::ARGB,w,h,true);chromeScale=physicalScale;
        juce::Graphics cg(chrome);cg.addTransform(juce::AffineTransform::scale(physicalScale));
        cg.fillAll(look.pick(0xff060b12,0xff171717,0xfff1f1f1));
        juce::Graphics::ScopedSaveState save(cg);cg.addTransform(juce::AffineTransform::scale(float(getWidth())/960));
        if(look.hasGlow()){
            const auto top=look.isAmber()?juce::Colour(0xff2b1907):juce::Colour(0xff1d2b3e);
            const auto bottom=look.isAmber()?juce::Colour(0xff0d0803):juce::Colour(0xff04080e);
            cg.setGradientFill(juce::ColourGradient(top,480,0,bottom,480,890,true));cg.fillRect(0,0,960,expanded?890:636);
            const auto ray=look.isAmber()?juce::Colour(0xffad6a31):juce::Colour(0xff52617e);
            for(int i=0;i<7;++i){juce::Path p;p.startNewSubPath(340+float(i)*23,0);p.cubicTo(430+float(i)*20,70,470+float(i)*20,80,670+float(i)*25,95);stroke(cg,p,ray.withAlpha(.14f),.8f);}
        }
        text(cg,"DUCK POCKET",{180,14,600,54},39,look.ink(),juce::Justification::centred);
        cg.setColour(look.pick(0xff263347,0xff444444,0xffc5c6c8));cg.drawLine(24,78,936,78);
        const bool glow=look.hasGlow();const float textGlow=glow?.075f:0.f;
        auto frame=[&](juce::Rectangle<float> box,bool gain){
            cg.setColour(look.pick(0xff050b13,0xff111111,0xffffffff));cg.fillRoundedRectangle(box,12);
            const auto label=look.ink(),secondary=look.muted(),grid=look.pick(0xff223349,0xff363636,0xffdddddd);
            text(cg,gain?"GAIN HISTORY":"OSCILLOSCOPE",{box.getX()+18,box.getY()+12,220,24},13,label,juce::Justification::centredLeft,textGlow);
            text(cg,gain?"WAVEFORM ENVELOPE":"OUT     KEY",{box.getRight()-240,box.getY()+12,216,24},11,secondary,juce::Justification::centredRight,textGlow);
            const juce::Rectangle<float> plot(box.getX()+18,box.getY()+47,box.getWidth()-70,box.getHeight()-87);
            cg.setColour(grid);
                const float top=plot.getY(),bottom=plot.getBottom(),midY=plot.getCentreY(),height=plot.getHeight();
                const float left=plot.getX(),right=plot.getRight(),endS=height*.10f,ringS=height*.04f;
                auto arc=[&](float cx,float s){juce::Path p;p.startNewSubPath(cx,top);p.quadraticTo(cx+2.f*s,midY,cx,bottom);stroke(cg,p,grid,.85f);};
                // End lenses: outer and inner parabolas meet at both corners.
                arc(left,-endS);arc(left,endS);arc(right,endS);arc(right,-endS);
                // Quarter rings bend towards the graph centre; the middle stays straight.
                arc(left+plot.getWidth()*.25f,ringS);arc(left+plot.getWidth()*.75f,-ringS);
                cg.drawLine(plot.getCentreX(),top,plot.getCentreX(),bottom,.85f);
                // Full top/bottom generators.
                cg.drawLine(left,top,right,top,.85f);cg.drawLine(left,bottom,right,bottom,.85f);
                // Interior generators terminate exactly on the inner lens parabolas.
                for(float u:{.25f,.5f,.75f}){const float y=top+u*height,offset=4.f*endS*u*(1.f-u);cg.drawLine(left+offset,y,right-offset,y,.85f);}

            if(gain){text(cg,"100%",{plot.getRight()+7,plot.getY()-9,43,20},11,label,juce::Justification::centredLeft,textGlow);text(cg,"0%",{plot.getRight()+7,plot.getBottom()-10,43,20},11,label,juce::Justification::centredLeft,textGlow);}
            else{text(cg,"+1",{plot.getRight()+7,plot.getY()-9,40,20},11,secondary,juce::Justification::centredLeft,textGlow);text(cg,"-1",{plot.getRight()+7,plot.getBottom()-10,40,20},11,secondary,juce::Justification::centredLeft,textGlow);}
            const double window=gain?gainWindow:scopeWindow;
            text(cg,"-"+timeLabel(window),{plot.getX(),box.getBottom()-29,100,20},11,secondary,juce::Justification::centredLeft,textGlow);
            text(cg,"NOW",{plot.getRight()-70,box.getBottom()-29,70,20},11,secondary,juce::Justification::centredRight,textGlow);
        };
        frame({24,94,674,230},true);frame({24,344,674,230},false);panel(cg,{714,94,222,480});text(cg,"Sidechain",{69,592,170,26},13,look.muted());
        if(expanded)panel(cg,{24,637,912,229});
        chromeValid=true;
    }
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImageTransformed(chrome,juce::AffineTransform::scale(1.f/chromeScale));
}

void DuckPocketAudioProcessorEditor::paintDynamicLabels(juce::Graphics& g){
    if(!expanded)return;
    const auto primary=look.ink(),secondary=look.muted();
    text(g,"Sidechain filter",{330,648,300,28},16,primary,juce::Justification::centred);
    text(g,hz(sidechainRange.getMinValue()),{52,648,150,28},15,primary,juce::Justification::centredLeft);
    text(g,hz(sidechainRange.getMaxValue()),{758,648,150,28},15,primary,juce::Justification::centredRight);
    text(g,"Processing range",{330,728,300,28},16,primary,juce::Justification::centred);
    text(g,hz(processingRange.getMinValue()),{52,728,150,28},15,primary,juce::Justification::centredLeft);
    text(g,hz(processingRange.getMaxValue()),{758,728,150,28},15,primary,juce::Justification::centredRight);
    const float v=float(midSide.getValue());
    const int mid=juce::roundToInt((1.f-juce::jmax(0.f,v))*100.f);
    const int side=juce::roundToInt((1.f+juce::jmin(0.f,v))*100.f);
    text(g,"Mid: "+juce::String(mid)+"%",{120,820,200,30},14,secondary,juce::Justification::centredRight);
    text(g,"Side: "+juce::String(side)+"%",{640,820,200,30},14,secondary,juce::Justification::centredLeft);
}

void DuckPocketAudioProcessorEditor::paint(juce::Graphics& g){
    paintChrome(g);juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(float(getWidth())/960));
    graph(g,{24,94,674,230},true);graph(g,{24,344,674,230},false);paintDynamicLabels(g);
}
void DuckPocketAudioProcessorEditor::captureBlurSnapshot(){
    if(capturingBlur||blurArea.isEmpty())return;
    capturingBlur=true;const float old=bypassMix;bypassMix=0;
    auto source=createComponentSnapshot(blurArea,true,1.f);
    bypassMix=old;capturingBlur=false;
    if(!source.isValid()||source.getWidth()<8||source.getHeight()<8)return;
    const int w=juce::jmax(16,source.getWidth()/6),h=juce::jmax(16,source.getHeight()/6);
    juce::Image small(juce::Image::ARGB,w,h,true);
    {juce::Graphics sg(small);sg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);sg.drawImage(source,juce::Rectangle<float>(0,0,float(w),float(h)),juce::RectanglePlacement::stretchToFit);}
    juce::Image soft(juce::Image::ARGB,w,h,true);
    juce::ImageConvolutionKernel kernel(9);kernel.createGaussianBlur(2.2f);kernel.applyToImage(soft,small,small.getBounds());
    blurredSnapshot=soft;
}
void DuckPocketAudioProcessorEditor::paintOverChildren(juce::Graphics& g){
    if(capturingBlur||bypassMix<.5f||!blurredSnapshot.isValid())return;
    juce::Graphics::ScopedSaveState save(g);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(blurredSnapshot,blurArea.toFloat(),juce::RectanglePlacement::stretchToFit);
    g.setColour((look.isDark()?juce::Colours::black:juce::Colours::white).withAlpha(.18f));g.fillRoundedRectangle(blurArea.toFloat(),12);
    const auto centre=blurArea.toFloat().translated(0,-float(blurArea.getHeight())*.05f);
    const float size=juce::jmax(34.f,float(getWidth())/18.f);
    const auto ink=look.isDark()?juce::Colours::white:juce::Colour(0xff202124);
    g.setColour((look.isDark()?juce::Colours::black:juce::Colours::white).withAlpha(.55f));g.setFont(uiFont(size));g.drawText("BYPASSED",centre.translated(0,2),juce::Justification::centred);
    text(g,"BYPASSED",centre,size,ink,juce::Justification::centred,look.isDark()?.1f:0.f);
}
void DuckPocketAudioProcessorEditor::frameTick(){
    if(resizeStamp>0&&juce::Time::getMillisecondCounterHiRes()-resizeStamp>400)saveSize();
    PocketTrace v;bool fresh=false;
    while(audioProcessor.popTrace(v)){if(!std::isfinite(v.time))continue;history[size_t(cursor)]=v;cursor=(cursor+1)%historyCapacity;filled=juce::jmin(filled+1,historyCapacity);fresh=true;}
    const bool target=audioProcessor.parameters.getRawParameterValue("bypass")->load()>.5f||audioProcessor.displayBypass.load();
    if(target!=bypassTarget){
        bypassTarget=target;
        for(auto* c:{static_cast<juce::Component*>(&panelButton),static_cast<juce::Component*>(&sidechainRange),static_cast<juce::Component*>(&processingRange),static_cast<juce::Component*>(&midSide)})c->setEnabled(!target);
        if(target){captureBlurSnapshot();bypassMix=1;}else{bypassMix=0;blurredSnapshot=juce::Image();}
        repaint();return;
    }
    // Repaint the two plot rectangles only, and skip a frozen graph entirely.
    if(!fresh||bypassMix>0)return;
    if(!gainFrozen)repaint(gainArea);
    if(!scopeFrozen)repaint(scopeArea);
}
