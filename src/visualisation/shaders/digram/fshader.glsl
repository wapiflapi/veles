/*
 * Copyright 2016 CodiLime
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#version 330
in vec2 v_coord;
layout (location = 0, index = 0) out vec4 o_color;
uniform sampler2D tx;
uniform float c_sqr, c_cir;
const float TAU = 3.1415926535897932384626433832795 * 2;
const float PI = 3.1415926535897932384626433832795;

void main() {

  vec2 z = v_coord * 2 - vec2(1, 1);

  vec2 cpos = vec2(length(z), (atan(z.y, z.x) + PI) / TAU);
  vec2 xpos = v_coord * (1 - c_cir) + cpos * c_cir;

  // YES the transitiion is uglier than for trigrams.
  // that's because it's freaking hard to compute the paths
  // in this direction. For trigrams we do it the oposite
  // way in the vshader. Maybe we should do the same.
  // But that would require some refactoring, do that "tomorrow".

  vec4 t = texture(tx, xpos);

  float clr = t.x;
  float ch = t.y;
  ch /= clr;
  clr *= 4096.0;

  if (length(z) > 1) {
    // not in circle.
    clr = (1 - c_cir) * clr;
  }

  // color indicates where the data comes from in the selection.
  o_color = vec4(clr * (1.0 - ch), clr/2.0, clr * ch, 0);

}
