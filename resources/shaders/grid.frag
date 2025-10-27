#version 330 core

out vec4 FragColor;

// Color can be driven by the renderer; default to mid-gray if unset
uniform vec3 gridColor;

void main()
{
    vec3 color = gridColor;
    // If the uniform wasn't provided by the app, fall back to a neutral gray
    if (color == vec3(0.0)) {
        color = vec3(0.4);
    }
    FragColor = vec4(color, 1.0);
}
