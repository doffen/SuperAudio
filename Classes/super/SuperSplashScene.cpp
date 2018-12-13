// SuperSplashScene.cpp (compile as objective-C++ in XCode for Mac)
//
//  Show Superpowered splash screen in Cocos2d-x, as required by
//    Superpowered licensing agreement.
//  If your first scene is not HelloWorld, edit the two HelloWorld lines below
//    with your first scene.
//
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

#include "HelloWorldScene.h" // edit this line with scene to start after this splash
#include "SuperSplashScene.h"

// for PC and Mac, see: https://discuss.cocos2d-x.org/t/playing-a-video-on-mac-and-win32/43101/18

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS
#include "ui/CocosGUI.h"
using namespace cocos2d::experimental::ui; // for video player
#endif

#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
#include <AVKit/AVKit.h>
#endif

Scene* SuperSplashScene::createScene() {
    CCLOG("SuperSplashScene");
    auto scene = Scene::create();
    scene->addChild(SuperSplashScene::create());
    return scene;
}

bool SuperSplashScene::init() {
    if ( !Layer::init() ) return false;
    doSplashAndWait();
    return true;
}

Scene *SuperSplashScene::myFirstScene() {
    return HelloWorld::createScene(); // edit this line with for scene to start after this splash
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS
void SuperSplashScene::doSplashAndWait() {
    auto videoPlayer = VideoPlayer::create();
    auto visibleSize = Director::getInstance()->getVisibleSize();
    videoPlayer->setContentSize(visibleSize);
    videoPlayer->setPosition(visibleSize/2);
    videoPlayer->setKeepAspectRatioEnabled(true);
    this->addChild(videoPlayer);
    videoPlayer->setFileName("superpowered.mp4");
    videoPlayer->play();
    auto node = DrawNode::create();
    addChild(node);
    node->runAction(Sequence::create(DelayTime::create(3), CallFunc::create([=]() {
        Director::getInstance()->replaceScene(myFirstScene());
    }), nullptr));
}
#endif // Android & iOS

#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC
void SuperSplashScene::doSplashAndWait() {
    //necessary?: _fileName = videoPath;
    NSView* view = ((NSWindow *)Director::getInstance()->getOpenGLView()->getCocoaWindow()).contentView;
    auto avPlayer = [[AVPlayer alloc] initWithURL:[NSURL fileURLWithPath:[NSString stringWithCString:"superpowered.mp4" encoding:[NSString defaultCStringEncoding]]]];
    AVPlayerLayer *playerLayer = [AVPlayerLayer playerLayerWithPlayer: avPlayer];
    auto rect = Director::getInstance()->getSafeAreaRect();
    auto containerView = [[NSView alloc] initWithFrame: NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height)];
    [containerView setWantsLayer:YES];
    playerLayer.frame = view.frame;
    [containerView.layer addSublayer: playerLayer];
    [playerLayer setVideoGravity: AVLayerVideoGravityResizeAspect];
    [playerLayer setNeedsDisplay];
    [containerView needsDisplay];
    [view addSubview:containerView];
    [avPlayer play];
    auto node = DrawNode::create();
    addChild(node);
    node->runAction(Sequence::create(DelayTime::create(3), CallFunc::create([=]() {
        [playerLayer release];
        Director::getInstance()->replaceScene(myFirstScene());
    }), nullptr));
}
#endif //Mac

