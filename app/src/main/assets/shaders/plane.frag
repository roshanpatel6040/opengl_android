precision highp float;
precision highp int;
uniform sampler2D texture;
varying vec2 v_textureCoords;
varying float v_alpha;

void main() {
  float r = texture2D(texture, v_textureCoords).r;
  gl_FragColor = vec4(v_alpha);
}
