// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PresetManager.h"
#include "GeneratedParameters.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace lr608
{
namespace
{
constexpr auto presetHeader="LR-608 Patch Snapshot";
constexpr auto presetExtension=".LR608";
constexpr auto presetWildcard="*.LR608";
const juce::Identifier currentPresetProperty{"currentPresetRelativePath"};

std::vector<juce::String> tokeniseReaperState(const juce::String&text)
{
    std::vector<juce::String> result;juce::String current;bool quoted=false;
    for(const auto c:text){if(c=='"'){quoted=!quoted;continue;}if(!quoted&&juce::CharacterFunctions::isWhitespace(c)){if(current.isNotEmpty()){result.push_back(current);current.clear();}continue;}current+=c;}
    if(current.isNotEmpty())result.push_back(current);return result;
}
juce::String embeddedRplText(){int size=0;if(const auto*data=BinaryData::getNamedResource("LR608_jsfx_rpl",size))return juce::String::fromUTF8(data,size);return {};}
int descriptorIndex(juce::String key)
{
    key=key.trim();if(key.startsWith("[PARAM] "))key=key.substring(8).trim();
    for(int i=0;i<int(std::size(generated::parameters));++i)if(key==generated::parameters[i].id||key==generated::parameters[i].name)return i;return-1;
}
}

PresetManager::PresetManager(juce::AudioProcessorValueTreeState&s,juce::File suppliedRoot):state(s),root(suppliedRoot==juce::File()?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("LR-608"):std::move(suppliedRoot)){}

void PresetManager::setStateCallbacks(std::function<juce::ValueTree()>capture,std::function<void(const juce::ValueTree&)>restore,std::function<void()>legacy)
{captureState=std::move(capture);restoreState=std::move(restore);importLegacy=std::move(legacy);}

juce::Result PresetManager::ensureLibraryExists()
{
    if(const auto r=root.createDirectory();r.failed())return juce::Result::fail("Cannot create the LR-608 preset folder");
    const auto factory=root.getChildFile("Factory");if(const auto r=factory.createDirectory();r.failed())return juce::Result::fail("Cannot create the Factory preset folder");
    const auto marker=root.getChildFile(".factory-rpl-4-lr608-installed");if(marker.existsAsFile())return juce::Result::ok();
    const auto presets=parseReaperLibrary(embeddedRplText());if(presets.empty())return juce::Result::fail("The embedded LR-608 preset library is invalid");
    std::vector<juce::File> installed;
    for(std::size_t i=0;i<presets.size();++i){const auto installedName=i==0?juce::String("Init"):presets[i].name;auto legal=juce::File::createLegalFileName(installedName.trim());if(legal.isEmpty())legal="Factory preset";const auto file=factory.getChildFile(juce::String(int(i)+1).paddedLeft('0',3)+" - "+legal+presetExtension);if(!file.replaceWithText(serialiseLegacyPreset(installedName,presets[i].values)))return juce::Result::fail("Cannot write an LR-608 factory preset");installed.push_back(file);}
    const auto old=factory.findChildFiles(juce::File::findFiles,false,presetWildcard,juce::File::FollowSymlinks::no);for(const auto&file:old){const auto n=file.getFileName();const auto managed=n.length()>6&&n.substring(0,3).containsOnly("0123456789")&&n.substring(3,6)==" - ";if(managed&&std::find(installed.begin(),installed.end(),file)==installed.end()&&!file.deleteFile())return juce::Result::fail("An obsolete Factory preset could not be removed");}
    if(!marker.replaceWithText("LR-608 Factory bank synchronised with Init-first migration version 3.\n"))return juce::Result::fail("Factory presets were written but the marker failed");return juce::Result::ok();
}

juce::Result PresetManager::loadFactoryInit()
{
    if(const auto result=ensureLibraryExists();result.failed())return result;
    const auto file=root.getChildFile("Factory").getChildFile("001 - Init"+juce::String(presetExtension));
    juce::String name;int number=0;
    return loadPreset(file,name,number);
}

bool PresetManager::naturalFileLess(const juce::File&a,const juce::File&b){return a.getFileName().compareNatural(b.getFileName(),false)<0;}
bool PresetManager::isInsideLibrary(const juce::File&file)const{return file==root||file.isAChildOf(root);}
std::vector<PresetManager::BrowserEntry> PresetManager::listDirectory(const juce::File&directory)const
{
    std::vector<BrowserEntry> out;if(!isInsideLibrary(directory)||!directory.isDirectory())return out;
    auto dirs=directory.findChildFiles(juce::File::findDirectories|juce::File::ignoreHiddenFiles,false,"*",juce::File::FollowSymlinks::no);std::sort(dirs.begin(),dirs.end(),naturalFileLess);for(const auto&d:dirs)out.push_back({d,d.getFileName(),true});
    auto files=directory.findChildFiles(juce::File::findFiles|juce::File::ignoreHiddenFiles,false,presetWildcard,juce::File::FollowSymlinks::no);std::sort(files.begin(),files.end(),naturalFileLess);for(const auto&f:files)if(isValidPresetFile(f))out.push_back({f,nameWithoutExtension(f),false});return out;
}
std::vector<juce::File> PresetManager::allPresetFiles()const
{
    std::vector<juce::File> out;if(!root.isDirectory())return out;for(const auto&f:root.findChildFiles(juce::File::findFiles|juce::File::ignoreHiddenFiles,true,presetWildcard,juce::File::FollowSymlinks::no))if(isValidPresetFile(f))out.push_back(f);const auto factory=root.getChildFile("Factory");std::sort(out.begin(),out.end(),[this,factory](const auto&a,const auto&b){const auto af=a.isAChildOf(factory),bf=b.isAChildOf(factory);if(af!=bf)return af;return a.getRelativePathFrom(root).compareNatural(b.getRelativePathFrom(root),false)<0;});return out;
}
bool PresetManager::isValidPresetFile(const juce::File&file)const{if(!file.existsAsFile()||!file.hasFileExtension("LR608")||!isInsideLibrary(file))return false;ParsedPreset parsed;return parseTextPreset(file.loadFileAsString(),parsed);}

juce::Result PresetManager::savePreset(const juce::String&requested,const juce::File&directory,juce::File&saved,bool overwrite)
{
    if(const auto r=ensureLibraryExists();r.failed())return r;if(!captureState)return juce::Result::fail("Complete preset state is unavailable");
    const auto targetDirectory=isInsideLibrary(directory)&&directory.isDirectory()?directory:root;auto name=requested.trim();if(name.endsWithIgnoreCase(presetExtension))name=name.dropLastCharacters(6).trim();else if(name.endsWithIgnoreCase(".txt"))name=name.dropLastCharacters(4).trim();name=juce::File::createLegalFileName(name);if(name.isEmpty()||name=="."||name=="..")return juce::Result::fail("Enter a valid preset name");
    saved=targetDirectory.getChildFile(name+presetExtension);if(!isInsideLibrary(saved))return juce::Result::fail("The preset must remain inside the LR-608 folder");if(saved.existsAsFile()&&!overwrite)return juce::Result::fail("A preset with this name already exists");const auto complete=captureState();if(!complete.isValid())return juce::Result::fail("Complete preset state is unavailable");if(!saved.replaceWithText(serialiseCompletePreset(name,complete)))return juce::Result::fail("The preset could not be written");rememberCurrentPreset(saved);return juce::Result::ok();
}

juce::Result PresetManager::deletePreset(const juce::File&file)
{
    if(!isValidPresetFile(file))return juce::Result::fail("Select a valid LR-608 preset file");
    if(file.isAChildOf(root.getChildFile("Factory")))return juce::Result::fail("Factory presets cannot be deleted");
    const auto wasCurrent=recalledCurrentPreset()==file;
    if(!file.deleteFile())return juce::Result::fail("The preset could not be deleted");
    if(wasCurrent)state.state.removeProperty(currentPresetProperty,nullptr);
    return juce::Result::ok();
}

juce::Result PresetManager::applyParsed(const ParsedPreset&parsed)
{
    if(parsed.completeState.isValid()){if(!restoreState)return juce::Result::fail("Complete preset restore is unavailable");restoreState(parsed.completeState);return juce::Result::ok();}
    if(parsed.values.size()!=std::size(generated::parameters))return juce::Result::fail("This is not a valid LR-608 preset");applyValues(parsed.values);if(importLegacy)importLegacy();return juce::Result::ok();
}
juce::Result PresetManager::loadPreset(const juce::File&file,juce::String&name,int&number)
{
    if(const auto r=ensureLibraryExists();r.failed())return r;if(!isValidPresetFile(file))return juce::Result::fail("This is not a valid LR-608 preset");ParsedPreset parsed;if(!parseTextPreset(file.loadFileAsString(),parsed))return juce::Result::fail("This is not a valid LR-608 preset");if(const auto r=applyParsed(parsed);r.failed())return r;rememberCurrentPreset(file);name=nameWithoutExtension(file);const auto files=allPresetFiles();const auto found=std::find(files.begin(),files.end(),file);number=found==files.end()?0:int(std::distance(files.begin(),found))+1;return juce::Result::ok();
}
juce::Result PresetManager::loadRelativePreset(int direction,juce::String&name,int&number)
{
    if(const auto r=ensureLibraryExists();r.failed())return r;const auto files=allPresetFiles();if(files.empty())return juce::Result::fail("No LR-608 presets found");const auto found=std::find(files.begin(),files.end(),recalledCurrentPreset());int index=found==files.end()?(direction>=0?0:int(files.size())-1):int(std::distance(files.begin(),found));if(found!=files.end())index=(index+(direction>=0?1:-1)+int(files.size()))%int(files.size());return loadPreset(files[std::size_t(index)],name,number);
}
PresetManager::PatchSnapshot PresetManager::capturePatchSnapshot()const{PatchSnapshot s;if(captureState)s.completeState=captureState();s.hadCurrentPreset=state.state.hasProperty(currentPresetProperty);s.currentPresetRelativePath=state.state.getProperty(currentPresetProperty).toString();return s;}
void PresetManager::restorePatchSnapshot(const PatchSnapshot&s){if(!s.isValid()||!restoreState)return;restoreState(s.completeState);if(s.hadCurrentPreset)state.state.setProperty(currentPresetProperty,s.currentPresetRelativePath,nullptr);else state.state.removeProperty(currentPresetProperty,nullptr);}
juce::Result PresetManager::previewPreset(const juce::File&file){if(!isValidPresetFile(file))return juce::Result::fail("This is not a valid LR-608 preset");ParsedPreset p;if(!parseTextPreset(file.loadFileAsString(),p))return juce::Result::fail("This is not a valid LR-608 preset");return applyParsed(p);}
juce::Result PresetManager::commitPresetPreview(const juce::File&file){if(!isValidPresetFile(file))return juce::Result::fail("This is not a valid LR-608 preset");rememberCurrentPreset(file);return juce::Result::ok();}

std::vector<PresetManager::ParsedPreset> PresetManager::parseReaperLibrary(const juce::String&text)
{
    std::vector<ParsedPreset> out;const auto lines=juce::StringArray::fromLines(text);juce::String name,b64;bool inside=false;for(const auto&raw:lines){const auto line=raw.trim();if(!inside&&line.startsWith("<PRESET `")){const auto close=line.indexOfChar(9,'`');if(close>9){name=line.substring(9,close).trim();inside=true;}continue;}if(!inside)continue;if(line!=">"){b64+=line;continue;}juce::MemoryOutputStream decoded;if(juce::Base64::convertFromBase64(decoded,b64)){const auto source=juce::String::fromUTF8(static_cast<const char*>(decoded.getData()),int(decoded.getDataSize()));const auto tokens=tokeniseReaperState(source);if(tokens.size()>=257){ParsedPreset p;p.name=name;for(const auto&d:generated::parameters){auto value=float(juce::jlimit(d.minimum,d.maximum,d.defaultValue));if(d.sliderNumber>0&&d.sliderNumber<=256){const auto ti=d.sliderNumber<=64?d.sliderNumber-1:d.sliderNumber;if(juce::isPositiveAndBelow(ti,int(tokens.size()))){const auto&t=tokens[std::size_t(ti)];if(t!="-"&&t.containsOnly("0123456789+-.eE")){const auto v=t.getDoubleValue();if(std::isfinite(v))value=float(juce::jlimit(d.minimum,d.maximum,v));}}}p.values.push_back(value);}out.push_back(std::move(p));}}name.clear();b64.clear();inside=false;}return out;
}
bool PresetManager::parseTextPreset(const juce::String&text,ParsedPreset&parsed)
{
    parsed.values.clear();for(const auto&d:generated::parameters)parsed.values.push_back(float(juce::jlimit(d.minimum,d.maximum,d.defaultValue)));bool header=false;int mapped=0;bool sawDelayTime=false,sawDelayResonance=false;double legacyDelayTime=0.0;
    for(const auto&raw:juce::StringArray::fromLines(text)){const auto line=raw.trim();if(line.isEmpty())continue;if(!header){if(line!=presetHeader)return false;header=true;continue;}const auto sep=line.indexOfChar(':');if(sep<=0)continue;const auto key=line.substring(0,sep).trim(),valueText=line.substring(sep+1).trim();if(key=="[STATE] Preset Name"){parsed.name=valueText;continue;}if(key=="[STATE] Complete State"){juce::MemoryOutputStream decoded;if(juce::Base64::convertFromBase64(decoded,valueText)){const auto xmlText=juce::String::fromUTF8(static_cast<const char*>(decoded.getData()),int(decoded.getDataSize()));if(auto xml=juce::XmlDocument::parse(xmlText))parsed.completeState=juce::ValueTree::fromXml(*xml);}continue;}const auto i=descriptorIndex(key);if(i<0||!valueText.containsOnly("0123456789+-.eE"))continue;const auto v=valueText.getDoubleValue();if(!std::isfinite(v))continue;if(key=="slotDelayDivision"){sawDelayTime=true;legacyDelayTime=v;}if(key=="slotDelayFilterResonance")sawDelayResonance=true;const auto&d=generated::parameters[i];parsed.values[std::size_t(i)]=float(juce::jlimit(d.minimum,d.maximum,v));++mapped;}
    if(sawDelayTime&&!sawDelayResonance&&legacyDelayTime>=0.0&&legacyDelayTime<=11.0)
    {
        static constexpr int oldToNew[]{1035,1031,1027,1023,1016,1008,992,960,896,768,512,0};
        const auto i=descriptorIndex("slotDelayDivision");if(i>=0)parsed.values[std::size_t(i)]=float(oldToNew[juce::jlimit(0,11,juce::roundToInt(legacyDelayTime))]);
    }
    return header&&(parsed.completeState.isValid()||mapped>0);
}
juce::String PresetManager::serialiseLegacyPreset(const juce::String&name,const std::vector<float>&values){juce::String text=juce::String(presetHeader)+"\n\n[STATE] Preset Name: "+name+"\n";for(std::size_t i=0;i<std::min(values.size(),std::size(generated::parameters));++i)text+="[PARAM] "+juce::String(generated::parameters[i].id)+": "+juce::String(values[i],9)+"\n";return text;}
juce::String PresetManager::serialiseCompletePreset(const juce::String&name,const juce::ValueTree&tree){auto xml=tree.createXml();if(!xml)return {};const auto raw=xml->toString();return juce::String(presetHeader)+"\n\n[STATE] Preset Name: "+name+"\n[STATE] Saved At: "+juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S")+"\n[STATE] Complete State: "+juce::Base64::toBase64(raw.toRawUTF8(),std::size_t(raw.getNumBytesAsUTF8()))+"\n";}
juce::String PresetManager::nameWithoutExtension(const juce::File&file){auto name=file.getFileNameWithoutExtension().trim();if(name.length()>6&&name.substring(0,3).containsOnly("0123456789")&&name.substring(3,6)==" - ")name=name.substring(6).trim();return name;}
void PresetManager::applyValues(const std::vector<float>&values){if(values.size()!=std::size(generated::parameters))return;for(std::size_t i=0;i<values.size();++i)if(auto*p=state.getParameter(generated::parameters[i].id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(values[i]));p->endChangeGesture();}}
void PresetManager::rememberCurrentPreset(const juce::File&file){state.state.setProperty(currentPresetProperty,file.getRelativePathFrom(root),nullptr);}
juce::File PresetManager::recalledCurrentPreset()const{const auto relative=state.state.getProperty(currentPresetProperty).toString();return relative.isEmpty()?juce::File():root.getChildFile(relative);}
int PresetManager::getEmbeddedFactoryPresetCount(){return int(parseReaperLibrary(embeddedRplText()).size());}
}
