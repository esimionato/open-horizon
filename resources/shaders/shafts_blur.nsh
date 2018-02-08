@predefined camera_pos "nya camera position"
@predefined vp "nya viewport"

@uniform light_dir "light dir"

@all

varying vec2 tc;
varying vec2 dir;

@vertex

uniform vec4 camera_pos;
uniform vec4 light_dir;

uniform vec4 vp;

void main()
{
    vec4 sun_pos = gl_ModelViewProjectionMatrix * vec4(camera_pos.xyz + light_dir.xyz * 100.0, 1.0);
    sun_pos.xy /= sun_pos.w;

    dir = (sun_pos.xy - gl_Vertex.xy) * 0.5;
    tc = gl_Vertex.xy * 0.5 + 0.5;

    gl_Position = vec4(gl_Vertex.xy,1.0,1.0);
}

@sampler base_map "color"

@fragment

uniform sampler2D base_map;

void main()
{
vec4 color = vec4(0.0,0.0,0.0,0.0);

const int Samples = 16;
for (int i = 0; i < Samples; ++i)
    color += texture2D(base_map, tc+float(i)/float(Samples)*dir)  * (1.0 - float(i)/float(Samples));

    gl_FragColor = color/float(Samples)*4.0;
}
