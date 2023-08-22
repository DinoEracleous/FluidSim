#version 450 core
in vec2 uv;
out vec4 FragColor;

//uniform sampler2D texture0;
//uniform sampler2D texture1;
uniform vec3 color;

float circleSDF(vec2 pos){
    return length(pos)-0.5;
}

void main()
{
    if(circleSDF(uv)>0) discard;
    FragColor = vec4(color,1.0f);
}

