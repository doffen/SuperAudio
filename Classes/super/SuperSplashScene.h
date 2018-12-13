//
//  SuperSplashScene.h
//  Show Superpowered splash screen in Cocos2d-x, as required by
//    Superpowered licensing agreement.
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

#ifndef SuperSplashScene_h
#define SuperSplashScene_h
#include "cocos2d.h"
USING_NS_CC;

class SuperSplashScene: public Layer {
public: \
    static Scene* createScene();
    CREATE_FUNC(SuperSplashScene);
private:
    DISALLOW_COPY_AND_ASSIGN(SuperSplashScene);
    SuperSplashScene() {}
    ~SuperSplashScene() {}
    virtual bool init();
    void doSplashAndWait();
    Scene *myFirstScene();
};


#endif /* SuperSplashScene_h */
