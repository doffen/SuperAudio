// SuperAudio.cpp for iOS, Mac and Android (compile as Objective-C++ in XCode)

// audio output using Superpowered

/****************************************************************************
 Copyright (c) 2018 David T. Offen
 
 http://www.doffen.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

// Requires full compliance with Superpowered licence agreement if released in a product.
// You should NEVER include any Cocos2d-x include files here.  If you must use
//   Cocos2d-x resources, move them into SuperAudioUtils.

#include <cstring>
#include <cstdint>
#include "SuperAudio.h"
#include "SuperAudioUtils.h"
#include "SuperpoweredSimple.h"
#include "SuperpoweredAdvancedAudioPlayer.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "SuperpoweredIOSAudioIO.h"
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
#include "SuperpoweredOSXAudioIO.h"
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
#include "SuperpoweredAndroidAudioIO.h"
#include "SuperpoweredCPU.h"
#include "JniHelper.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <string>
#endif

#define MAX_AUDIOINSTANCES 24

static float *outputBuffer = nullptr;
static unsigned int lastSamplerate = 44100; // default

struct PlayerInfo {
    SuperpoweredAdvancedAudioPlayer *player;
    bool nowLoading;
    std::function<void(int id, bool isSuccess)> callbackWhenloaded;
    float volume;
    bool closeWhenDone;
    std::function<void()> callbackWhenDone;
    int id; // same as playerInfo's index
};
static PlayerInfo playerInfo[MAX_AUDIOINSTANCES];

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
static SuperpoweredIOSAudioIO *audioSystem = nullptr;
@interface OutDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation OutDelegate
    - (void)interruptionStarted {} //The audio session may be interrupted by a phone call, etc. This method is called on the main thread when this happens.
    - (void)interruptionEnded {
        //The audio session may be interrupted by a phone call, etc. This method is called on the main thread when audio resumes.
        for (auto info=playerInfo; info < &playerInfo[MAX_AUDIOINSTANCES]; info++) {
            // If a player plays Apple Lossless audio files, then we need this. Otherwise unnecessary.
            if (info->player) info->player->onMediaserverInterrupt();
        }
    }
    - (void)recordPermissionRefused {} //Called if the user did not grant a recording permission for the app.
    - (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {} //This method is called on the main thread, when a multi-channel audio device is connected or disconnected.
@end
static OutDelegate *outDelegate = nil;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
static SuperpoweredOSXAudioIO *audioSystem = nil;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
static SuperpoweredAndroidAudioIO *audioSystem = nullptr;
static std::string APKPath;
#endif

// MARK: - locally-scoped functions
// A few locally-scoped functions follow, which require Superpowered-specific data types
//  (& therefore shouldn't go in SuperAudio.h and its class):

static PlayerInfo *getInfoForId(int id) {
    return (id<0 || id>=MAX_AUDIOINSTANCES) ? nullptr : &playerInfo[id];
}

static void closePlayer(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        auto ip = info->player; // null before deleting so interrupt by outputProcessing is safe
        info->player = nullptr;
        delete ip; // info->player
        if (info->callbackWhenloaded) {
            auto cb = info->callbackWhenloaded;
            info->callbackWhenloaded = nullptr;
            cb(-1, false); // signal error if premature close
        }
        if (info->callbackWhenDone) {
            auto cb = info->callbackWhenDone;
            info->callbackWhenDone = nullptr;
            cb();
        }
    }
}

static void playerEventCallback(void *clientdata, SuperpoweredAdvancedAudioPlayerEvent event, void *value) {
    if (event == SuperpoweredAdvancedAudioPlayerEvent_EOF ||
        event == SuperpoweredAdvancedAudioPlayerEvent_LoadError ||
        event == SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess) {
        auto info = (PlayerInfo *)clientdata;
        
        // make sure user is called back from main thread

        SuperAudioUtils::useCocosThread([event,info,value]() { // lambda executes in main thread! :
            auto id = info->id;
            char *val1 = (char *)value;
            std::string val2 = "";
            if (val1) val2 = val1; // error code

            switch (event) {
                case SuperpoweredAdvancedAudioPlayerEvent_EOF:
                    if (info->player && !info->player->looping) { // done playing
                        info->player->pause();
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
                        SuperpoweredCPU::setSustainedPerformanceMode(false);
#endif
                        if (info->callbackWhenDone) {
                            auto cb = info->callbackWhenDone;
                            info->callbackWhenDone = nullptr;
                            if (info->closeWhenDone) closePlayer(id);
                            cb();
                        } else {
                            if (info->closeWhenDone) closePlayer(id);
                        }
                    }
                    break;
                case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
                    CCLOG("SuperAudio file id: %d now loaded", id);
                    info->nowLoading = false;
                    if (info->callbackWhenloaded) {
                        auto cb = info->callbackWhenloaded;
                        info->callbackWhenloaded = nullptr;
                        cb(id, true);
                    }
                    break;
                case SuperpoweredAdvancedAudioPlayerEvent_LoadError:
                    CCLOG("SuperAudio LoadError: %s", val1);
                    // NOTE: if you get this error on an MP3 with "Unknown file format" for value,
                    // it's because this file is NOT encoded at MPEG Level 3, as required by the
                    // Superpowered library for Android.
                    info->nowLoading = false;
                    closePlayer(id);
                    break;
                default:
                    break;
            }
        });
    }
}

// MARK: - private class methods:

/*static*/ bool SuperAudio::outputProcessing(void *clientdata, float **buffers, short int *buffer, unsigned int numberOfSamples, unsigned int samplerate) {

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    if (samplerate != lastSamplerate) {
        lastSamplerate = samplerate;
        for (auto info=playerInfo; info < &playerInfo[MAX_AUDIOINSTANCES]; info++) {
            if (info->player) info->player->setSamplerate(samplerate);
        }
    }
#endif
    
    auto haveData = false;
    for (auto info=playerInfo; info < &playerInfo[MAX_AUDIOINSTANCES]; info++) { // merge all playing sounds
         if (info->player && info->player->process(outputBuffer, haveData, numberOfSamples, info->volume))
            haveData = true;
    }

    if (haveData) {
        if (buffer != nullptr)
            SuperpoweredFloatToShortInt(outputBuffer, buffer, numberOfSamples);
        else // use buffers[]
            SuperpoweredDeInterleave(outputBuffer, buffers[0], buffers[1], numberOfSamples);
    }

    return haveData;
}

/*static*/ bool SuperAudio::lazyInit() {
    if (outputBuffer != nullptr) return true; // init static vars only once
    
    for (auto i=0; i < MAX_AUDIOINSTANCES; i++) {
        auto info = getInfoForId(i);
        info->player = nullptr;
        info->nowLoading = false;
        info->callbackWhenloaded = nullptr;
        info->volume = 0.5f;
        info->closeWhenDone = true;
        info->callbackWhenDone = nullptr;
        info->id = i;
    }

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    posix_memalign((void **)&outputBuffer, 16, 4096+128);
    outDelegate = [[OutDelegate alloc] init];
    audioSystem = [[SuperpoweredIOSAudioIO alloc] initWithDelegate: (id<SuperpoweredIOSAudioIODelegate>)outDelegate preferredBufferSize:12 preferredSamplerate:lastSamplerate audioSessionCategory:AVAudioSessionCategoryPlayback channels:2 audioProcessingCallback:SuperAudio::audioProcessing clientdata:nil];
    [audioSystem start];
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    posix_memalign((void **)&outputBuffer, 16, 4096+128);
    audioSystem = [[SuperpoweredOSXAudioIO alloc] initWithDelegate:nil preferredBufferSizeMs:12 numberOfChannels:2 enableInput:false enableOutput:true];
    [audioSystem setProcessingCallback_C:SuperAudio::audioProcessing clientdata:nullptr];
    [audioSystem start];
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    // call Java to get output samplerate, buffersize and APKPath
    lastSamplerate = cocos2d::JniHelper::callStaticIntMethod("org.cocos2dx.cpp/AppActivity", "getSampleRate");
    auto buffersize = cocos2d::JniHelper::callStaticIntMethod("org.cocos2dx.cpp/AppActivity", "getBuffersize");
    APKPath = cocos2d::JniHelper::callStaticStringMethod("org.cocos2dx.cpp/AppActivity", "getAPKPath");
    outputBuffer = (float *)memalign(16, (buffersize+16)*sizeof(float)*2);
    audioSystem = new SuperpoweredAndroidAudioIO(lastSamplerate, buffersize, false, true, SuperAudio::audioProcessing, nullptr, -1, SL_ANDROID_STREAM_MEDIA); //, buffersize*2);
#endif
    return true;
}

// MARK: - public class methods:

/*static*/ void SuperAudio::end() {
    if (outputBuffer == nullptr) return; // already ended

    stopAndCloseAll();

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    [audioSystem stop];
    [audioSystem release];
    audioSystem = nil;
    [outDelegate release];
    outDelegate = nil;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    [audioSystem stop];
    [audioSystem release];
    audioSystem = nil;
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    if (audioSystem)  {
        audioSystem->stop();
        delete audioSystem;
        audioSystem = nullptr;
    }
#endif
    
    free(outputBuffer);
    outputBuffer = nullptr;
}

/*static*/ int SuperAudio::open(const std::string &filePath, bool loop, float volume, bool closeAtFinish, const std::function<void(int id, bool isSuccess)> &callback) {
    int id = -1; // default error return
    
    if (filePath != "" && lazyInit()) {
        
      // look for empty slot to play from
      for (auto info=playerInfo; info < &playerInfo[MAX_AUDIOINSTANCES]; info++) {
        if (info->player == nullptr) { // found empty slot
            int fileOffset = 0, fileLength = 0;
            std::string fullPath;
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
            fullPath = SuperAudioUtils::fullPathForFilename(filePath);
            if (fullPath == "") break; // no such filename, id=-1 still
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
            // Path is ignored.  Instead, uses proj.android/app/src/main/res/raw, which uses APKPath
            // Get 2 int values for file offset and length, using packed string returned from java
            fullPath = APKPath;
            int pos = filePath.rfind("/");
            int start = (pos == std::string::npos) ? 0 : pos+1;
            int len = filePath.rfind(".")-start;
            std::string packedStr = cocos2d::JniHelper::callStaticStringMethod("org.cocos2dx.cpp/AppActivity", "getPackedString", filePath.substr(start, len));
            if (packedStr == "") break; // couldn't find match, id=-1 still
            std::string::size_type sz;
            fileOffset = std::stoi(packedStr, &sz);
            fileLength = std::stoi(packedStr.substr(sz+1));
#endif
            info->nowLoading = true;
            info->callbackWhenloaded = callback;
            info->player = new SuperpoweredAdvancedAudioPlayer(info, playerEventCallback, lastSamplerate, 0);
            if (fileLength)
                info->player->open(fullPath.c_str(), fileOffset, fileLength);
            else
                info->player->open(fullPath.c_str());
            info->closeWhenDone = closeAtFinish;
            id = info->id;
            setVolume(id, volume);
            setLoop(id, loop);
            break;
        }
      }
    }

    if (id == -1 && callback) callback(-1, false); // error
    return id;
}

/*static*/ void SuperAudio::setVolume(int audioID, float volume) {
    auto info = getInfoForId(audioID);
    if (info) info->volume = fminf(1, fmaxf(0, volume));
}

/*static*/ float SuperAudio::getVolume(int audioID) {
    auto info = getInfoForId(audioID);
    if (info) return info->volume;
    return 0.5f;
}

/*static*/ void SuperAudio::setLoop(int audioID, bool loop) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        if (loop) 
            info->player->loop(0.0, (double)info->player->durationMs, false, 255, false);
        else
            info->player->exitLoop();
    }
}

/*static*/ bool SuperAudio::isLoop(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) return info->player->looping;
    return false;
}

/*static*/ void SuperAudio::playFromStart(int audioID, const std::function<void()> &callback) {
    setCurrentTime(audioID, 0);
    setFinishCallback(audioID, callback);
    resume(audioID);
}

/*static*/ void SuperAudio::pause(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        info->player->pause();
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        SuperpoweredCPU::setSustainedPerformanceMode(false);
#endif
    }
}

/*static*/ void SuperAudio::pauseAll() {
    if (outputBuffer == nullptr) return; // not initial;izeds
    for (auto i=0; i < MAX_AUDIOINSTANCES; i++)
        pause(i);
}

/*static*/ void SuperAudio::resume(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        info->player->play(false);
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        SuperpoweredCPU::setSustainedPerformanceMode(true);
#endif
     }
}

/*static*/ void SuperAudio::resumeAll() {
    if (outputBuffer == nullptr) return; // not initialized
    for (auto i=0; i < MAX_AUDIOINSTANCES; i++)
        resume(i);
}

/*static*/ void SuperAudio::stopAndClose(int audioID) {
    pause(audioID);
    closePlayer(audioID);
}

/*static*/ void SuperAudio::stopAndCloseAll() {
    if (outputBuffer == nullptr) return; // not initialized
    for (auto i=0; i < MAX_AUDIOINSTANCES; i++)
        stopAndClose(i);
}

/*static*/ float SuperAudio::getDuration(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        if (info->nowLoading) return -1;
        return (float)((double)info->player->durationMs / 1000.0);
    }
    return -1; // nothing to return
}

/*static*/ float SuperAudio::getCurrentTime(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player)
        return (float)(info->player->displayPositionMs / 1000.0);
    return -1; // nothing to return
}

/*static*/ bool SuperAudio::setCurrentTime(int audioID, float sec) {
    auto info = getInfoForId(audioID);
    if (info && info->player) {
        if (info->nowLoading) return false; // can't seek yet
        info->player->setPosition(sec*1000.0, true, false);
        return true;
    }
    return false;
}

/*static*/ bool SuperAudio::isPlaying(int audioID) {
    auto info = getInfoForId(audioID);
    if (info && info->player) return info->player->playing;
    return false;
}

/*static*/ void SuperAudio::setFinishCallback(int audioID, const std::function<void()> &callback) {
    auto info = getInfoForId(audioID);
    if (info) info->callbackWhenDone = callback;
}

/*static*/ int SuperAudio::getMaxAudioInstances() {
    return MAX_AUDIOINSTANCES;
}

/*static*/ int SuperAudio::getPlayingAudioCount() {
    int count = 0;
    if (outputBuffer == nullptr) return 0; // not initialized
    for (auto info=playerInfo; info < &playerInfo[MAX_AUDIOINSTANCES]; info++) {
        if (info->player && info->player->playing)
            count++;
    }
    return count;
}
