// SuyperAudioUtils.cpp

// for Cocos2d-x methods that are compiled without reference to any Superpowered includes
//
//  SuperAudioUtils.cpp

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

//  Never include any Superpowered include files here.  This module is strictly
//    for using Cocos2d-x resources.  Otherwise, symbol conflicts may cause
//    linker errors.

#include "base/CCRef.h"
#include "base/CCDirector.h"
#include "base/CCScheduler.h"
#include "platform/CCFileUtils.h"
#include "SuperAudioUtils.h"

/*static*/ std::string SuperAudioUtils::fullPathForFilename(const std::string &filename) {
    return cocos2d::FileUtils::getInstance()->fullPathForFilename(filename);
}

/*static*/ void SuperAudioUtils::useCocosThread(std::function<void()> function) {
    cocos2d::Scheduler *scheduler = cocos2d::Director::getInstance()->getScheduler();
    scheduler->performFunctionInCocosThread(function);
}

