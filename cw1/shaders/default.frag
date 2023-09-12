#version 450

layout( set = 1, binding = 0 ) uniform sampler2D uTexColor;
layout( location = 0) in vec2 g2fTexCoords;
layout( location = 1) in vec3 g2fColor;
layout( location = 2) in float gsTriangleDensity;

layout( location = 0 ) out vec4 oColor;


vec3 interpolateColorHSL(float lod, float lodMax);
vec3 colormap(float value);

void main()
{
    
	oColor = vec4( texture( uTexColor, g2fTexCoords ).rgb, 1.f );
    
    // to visualize mipmaps uncomment this
    //----------------------------------------------------
    
    vec2 texSize =  vec2(textureSize(uTexColor, 0));
    float lod = textureQueryLod(uTexColor,g2fTexCoords).x;
    float maxLod = log2(max(texSize.x, texSize.y));
    vec3 color = interpolateColorHSL(lod, maxLod);
    oColor = vec4(color,1.0);
    
    //---------------------------------------------------

    if(g2fTexCoords.x == 0 && g2fTexCoords.y == 0)
    {
        oColor = vec4(g2fColor,1.0f);
    } 
    
    //to visualize mesh density uncomment this
    //---------------------------------------------------------------
    /*
    float normalizedDensity = clamp(gsTriangleDensity, 0.0, 1.0);
    vec3 densityColor = colormap(normalizedDensity);
    oColor = vec4(densityColor,1.0f);
    */
    //-------------------------------------------------------------

   
}

//To achieve an enhanced visual appearance, I employed the conversion from HSL color space to RGB color space 
//using the hsl2rgb function, which is inspired by the color space conversion code available on ShaderToy. 
//Subsequently, the ratio of the current texture level to the maximum texture level was utilized as the 
//hue value in the HSL color space for interpolation, enabling a smooth color transition.
//https://www.shadertoy.com/view/XljGzV

vec3 hsl2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

vec3 interpolateColorHSL(float lod, float lodMax) {
    float hue = lod / lodMax;
    vec3 hsl = vec3(hue, 1.0, 0.5);
    return hsl2rgb(hsl);
}


vec3 colormap(float value) {

    vec3 color1 = vec3(1.0, 48.0/255.0, 48.0/255.0); //Firebrick1
    vec3 color2 = vec3(1.0);  //white
    vec3 color3 = vec3(152.0/255.0, 245.0/255.0, 1.0); //CadetBlue1

    float minValue = 0.0;
    float maxValue = 1.0;
    float interpolationPoint = 0.5;
    //25 is the experimental value to get a better visual effect
    float normalizedValue = clamp((value * 25 - minValue) / (maxValue - minValue), 0.0, 1.0);

    if (normalizedValue < interpolationPoint) {
        normalizedValue = normalizedValue / interpolationPoint;
        return mix(color1, color2, normalizedValue);
    } else {
        normalizedValue = (normalizedValue - interpolationPoint) / (1.0 - interpolationPoint);
        return mix(color2, color3, normalizedValue);
    }
}