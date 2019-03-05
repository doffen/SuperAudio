// SuperAudio.h, for iOS, Mac and Android

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

/****************************************************************************
 This is an interface to Superpowered Audio from Cocos2d-x, loosely based on
 the Cocos2-x AudioEngine interface.  This include file must contain no include
 files from either Superpowered or Cocos2d-x (except CCPlatformCobnfig.h),
 because resulting symbol conflicts can cause linker errors.
 Requires full compliance with Superpowered licence agreement if released in a product.
 ****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#define USE_SUPERPOWERED 1 // this is where the Superpowered Audio Engine gets enabled

#ifndef __SUPERAUDIO_H_
#define __SUPERAUDIO_H_

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <map> // for std:: definitions
#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

class SuperAudio {
public:
    /**
     * Release objects relating to SuperAudio.
     */
    static void end();
    
    /**
     * Open an audio instance.
     *
     * @param filePath The path of a 2d audio file of at least 1/10 second in length.
     * @param loop Whether or not the audio instance loops to the beginning.
     * @param volume Volume value (range from 0.0 to 1.0).
     * @param closeAtFinish Whether or not to automatically close when it's done playing.
     * @param callback Tells whether the file was able to open successfully.  This callback is needed if you
     *        are going to call getDuration() or setCurrentTime(), since they fail until open() has succeeded.
     * @return An audio ID (or 0 if bad filePath). It allows you to affect the behavior of an audio instance.
     */
    static int open(const std::string &filePath, bool loop=false, float volume=0.5f, bool closeAtFinish=true, const std::function<void(int id, bool isSuccess)> &callback = nullptr);
    
    /**
     * Start playing the opened audio instance from beginning.
     *
     * @param audioID An audioID returned from open.
     * @param callback Invoked when the audio instance has completed playing.
     */
    static void playFromStart(int audioID, const std::function<void()> &callback = nullptr);
    
    /**
     * Sets whether an audio instance loops or not.
     *
     * @param audioID An audioID returned from open.
     * @param loop Whether audio instance loops or not.
     */
    static void setLoop(int audioID, bool loop);
    
    /**
     * Checks whether an audio instance is looping.
     *
     * @param audioID An audioID returned from open.
     * @return Whether or not an audio instance is looping.
     */
    static bool isLoop(int audioID);
    
    /**
     * Sets volume for an audio instance.
     *
     * @param audioID An audioID returned from open.
     * @param volume Volume value (range from 0.0 to 1.0).
     */
    static void setVolume(int audioID, float volume);
    
    /**
     * Gets the volume value of an audio instance.
     *
     * @param audioID An audioID returned from open.
     * @return Volume value (range from 0.0 to 1.0).
     */
    static float getVolume(int audioID);
    
    /**
     * Pause an audio instance.
     *
     * @param audioID An audioID returned from open.
     */
    static void pause(int audioID);
    
    /** Pause all playing audio instances. */
    static void pauseAll();
    
    /**
     * Resume an audio instance.
     *
     * @param audioID An audioID returned from open.
     */
    static void resume(int audioID);
    
    /** Resume all paused audio instances. */
    static void resumeAll();
    
    /**
     * Stop an audio instance (if necessary) and close it.
     *
     * @param audioID An audioID returned from open.
     *        It will no longer be valid.
     */
    static void stopAndClose(int audioID);
    
    /** Stop and close all audio instances (invalidates all audioIDs). */
    static void stopAndCloseAll();
    
    /**
     * Gets the duration of an audio instance.
     *
     * @param audioID An audioID returned from open.
     * @return The duration of an audio instance in seconds,
     *         or -1 if invalid audioID or not currently available
     *         (unavailable briefly after opened).
     */
    static float getDuration(int audioID);

    /**
     * Gets the current playback position of an audio instance.
     *
     * @param audioID An audioID returned from open.
     * @return The current playback position of an audio instance in seconds,
     *         or -1 if invalid audioID.
     */
    static float getCurrentTime(int audioID);

    /**
     * Sets the current playback position of an audio instance and pauses it if playing.
     *
     * @param audioID   An audioID returned from open.
     * @param sec       The offset in seconds from the start to seek to.
     * @return true if current time was able to be set (unavailable briefly after opened)
     */
    static bool setCurrentTime(int audioID, float sec);

    /**
     * Returns whether or not an audio instance is currently playing.
     *
     * @param audioID An audioID returned from open.
     * @return The playing status of an audio instance.
     */
    static bool isPlaying(int audioID);
    
    /**
     * Register a callback to be invoked when an audio instance has completed playing.
     *
     * @param audioID An audioID returned from open.
     * @param callback
     */
    static void setFinishCallback(int audioID, const std::function<void()> &callback);
    
    /**
     * Gets the maximum number of simultaneous audio instances of SuperAudio.
     */
    static int getMaxAudioInstances();

    /**
     * Gets playing audio count.
     */
    static int getPlayingAudioCount();

private:
    SuperAudio() {};
    ~SuperAudio() {};

    static bool lazyInit();
    static bool outputProcessing(void *clientdata, float **buffers, short int *buffer, unsigned int numberOfSamples, unsigned int samplerate); // for all platforms
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
// for Superpowered v1.2.4x
    static bool audioProcessing(void *clientdata, float **buffers, unsigned int unusedInputChannels, unsigned int unusedOutputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t unusedHostTime) {
        return outputProcessing(clientdata, buffers, nullptr, numberOfSamples, samplerate);
    };
// for Superpowered v1.3.1
    static bool audioProcessing(void *clientdata, float **inputBuffers, unsigned int unusedInputChannels, float **outputBuffers, unsigned int unusedOutputChannels, unsigned int numberOfSamples, unsigned int samplerate, unsigned long long unusedHostTime) {
        return outputProcessing(clientdata, outputBuffers, nullptr, numberOfSamples, samplerate);
}
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    static bool audioProcessing (void *clientdata, float **unusedInputBuffers, unsigned int unusedInputChannels, float **buffers, unsigned int unusedOutputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t unusedHostTime) {
        return outputProcessing(clientdata, buffers, nullptr, numberOfSamples, samplerate);
    };
#endif
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
    static bool audioProcessing(void *clientdata, short int *buffer, int numberOfSamples, int samplerate) {
        return outputProcessing(clientdata, nullptr, buffer, numberOfSamples, samplerate);
    };
#endif
};
#endif // __SUPERAUDIO_H_
#endif

