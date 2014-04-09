#version 120

//
//  model_shadow.vert
//  vertex shader
//
//  Created by Andrzej Kapolka on 3/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

void main(void) {
    // just use standard pipeline transform
    gl_Position = ftransform();
}
