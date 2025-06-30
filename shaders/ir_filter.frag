#version 330 core

/*
 * Fragment shader for realistic IR camera processing
 * Implements proper luminance-based contrast curves, edge enhancement, and sensor simulation
 *
 * MIT License
 * 
 * Copyright (c) 2025 sebastian <sebastian@eingabeausgabe.io>
 */

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec2 screenSize;
uniform int mode;           // 0=standard, 1=mono, 2=thermal, 3=IR, 4=enhanced_IR
uniform float contrast;
uniform float brightness;
uniform float gamma;
uniform float gain;
uniform float bias;
uniform float edgeIntensity;
uniform float time;

// Convert RGB to luminance using proper weights
float getLuminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// Apply contrast curve with gamma correction
float applyContrastCurve(float value, float gamma, float gain, float bias) {
    value = clamp(value, 0.0, 1.0);
    value = pow(value, gamma);
    value = value * gain + bias;
    return clamp(value, 0.0, 1.0);
}

// Sobel edge detection
float detectEdges(vec2 texCoord) {
    vec2 texelSize = 1.0 / screenSize;
    
    // Sample surrounding pixels
    float tl = getLuminance(texture(screenTexture, texCoord + vec2(-texelSize.x, -texelSize.y)).rgb);
    float tm = getLuminance(texture(screenTexture, texCoord + vec2(0.0, -texelSize.y)).rgb);
    float tr = getLuminance(texture(screenTexture, texCoord + vec2(texelSize.x, -texelSize.y)).rgb);
    float ml = getLuminance(texture(screenTexture, texCoord + vec2(-texelSize.x, 0.0)).rgb);
    float mr = getLuminance(texture(screenTexture, texCoord + vec2(texelSize.x, 0.0)).rgb);
    float bl = getLuminance(texture(screenTexture, texCoord + vec2(-texelSize.x, texelSize.y)).rgb);
    float bm = getLuminance(texture(screenTexture, texCoord + vec2(0.0, texelSize.y)).rgb);
    float br = getLuminance(texture(screenTexture, texCoord + vec2(texelSize.x, texelSize.y)).rgb);
    
    // Sobel operators
    float sobelX = (tr + 2.0*mr + br) - (tl + 2.0*ml + bl);
    float sobelY = (bl + 2.0*bm + br) - (tl + 2.0*tm + tr);
    
    return sqrt(sobelX*sobelX + sobelY*sobelY);
}

// Simulate sensor noise
float addSensorNoise(vec2 texCoord, float time) {
    vec2 noise = fract(sin(dot(texCoord + time * 0.1, vec2(12.9898, 78.233))) * 43758.5453);
    return (noise.x - 0.5) * 0.05;
}

// Apply IR-specific processing
vec3 processIR(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Aggressive contrast curve for IR
    luma = applyContrastCurve(luma, 0.4, 3.5, -0.2);
    
    // Edge enhancement
    float edge = detectEdges(texCoord);
    luma += edge * edgeIntensity * 0.5;
    
    // IR inversion - hot objects appear white
    luma = 1.0 - luma;
    
    // Quantize to simulate 8-bit sensor
    luma = floor(luma * 255.0) / 255.0;
    
    // Add sensor noise
    luma += addSensorNoise(texCoord, time) * 0.3;
    
    return vec3(luma);
}

// Apply enhanced IR processing
vec3 processEnhancedIR(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // More aggressive processing
    luma = applyContrastCurve(luma, 0.3, 4.0, -0.3);
    
    // Strong edge enhancement
    float edge = detectEdges(texCoord);
    luma += edge * edgeIntensity;
    
    // Harsh quantization
    luma = floor(luma * 64.0) / 64.0;
    
    // IR inversion
    luma = 1.0 - luma;
    
    return vec3(luma);
}

// Apply monochrome with enhancement
vec3 processMonochrome(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Apply contrast curve
    luma = applyContrastCurve(luma, gamma, gain, bias);
    
    // Subtle edge enhancement
    float edge = detectEdges(texCoord);
    luma += edge * edgeIntensity * 0.3;
    
    // Green tint for night vision effect
    return vec3(luma * 0.1, luma, luma * 0.1);
}

// Apply thermal simulation
vec3 processThermal(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Thermal contrast curve
    luma = applyContrastCurve(luma, 1.2, 1.8, 0.05);
    
    // Thermal color mapping (fake, but looks convincing)
    vec3 thermal;
    if (luma < 0.25) {
        thermal = mix(vec3(0.0, 0.0, 0.3), vec3(0.0, 0.0, 1.0), luma * 4.0);
    } else if (luma < 0.5) {
        thermal = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), (luma - 0.25) * 4.0);
    } else if (luma < 0.75) {
        thermal = mix(vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 0.0), (luma - 0.5) * 4.0);
    } else {
        thermal = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (luma - 0.75) * 4.0);
    }
    
    return thermal;
}

void main()
{
    vec3 color = texture(screenTexture, TexCoord).rgb;
    vec3 result;
    
    switch(mode) {
        case 0: // Standard
            result = color;
            break;
        case 1: // Monochrome
            result = processMonochrome(color, TexCoord);
            break;
        case 2: // Thermal
            result = processThermal(color, TexCoord);
            break;
        case 3: // IR
            result = processIR(color, TexCoord);
            break;
        case 4: // Enhanced IR
            result = processEnhancedIR(color, TexCoord);
            break;
        default:
            result = color;
            break;
    }
    
    // Apply global brightness/contrast adjustments
    result = result * brightness;
    result = pow(result, vec3(1.0 / contrast));
    
    FragColor = vec4(result, 1.0);
}