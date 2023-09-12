#version 450 core

layout( location = 0 ) in vec2 v2gTexCoords[];
layout( location = 1 ) in vec3 v2gColor[];
layout( location = 2 ) in vec3 v2gPos[];

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout( location = 0) out vec2 g2fTexCoords;
layout( location = 1) out vec3 g2fColor;
layout( location = 2) out float gsTriangleDensity;


float calculateTriangleArea(vec3 p1, vec3 p2, vec3 p3) {
    vec3 edge1 = p2 - p1;
    vec3 edge2 = p3 - p1;
    return length(cross(edge1, edge2)) * 0.5;
}

void main() {
    float triangleArea = calculateTriangleArea(v2gPos[0], v2gPos[1], v2gPos[2]);

    //25 is the experimental value to get a better visual effect
   // float normalizedDensity = triangleArea*25;

    for (int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;
        g2fTexCoords = v2gTexCoords[i];
        g2fColor = v2gColor[i];
        gsTriangleDensity = triangleArea;
        EmitVertex();
    }
    EndPrimitive();

}
