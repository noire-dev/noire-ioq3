attribute vec3 attr_Position;
uniform mat4 u_ModelViewProjectionMatrix;
varying vec3 var_SkyDirection;
void main() {
    var_SkyDirection = normalize(attr_Position);
    gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
}
