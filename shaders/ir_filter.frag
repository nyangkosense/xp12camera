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

// Enhanced sensor noise simulation
float addSensorNoise(vec2 texCoord, float time) {
    // Temporal noise
    vec2 temporalNoise = fract(sin(dot(texCoord + time * 0.1, vec2(12.9898, 78.233))) * 43758.5453);
    
    // Spatial noise (fixed pattern)
    vec2 spatialNoise = fract(sin(dot(texCoord * 100.0, vec2(41.2345, 67.5432))) * 23857.1134);
    
    // Dead pixels simulation
    float deadPixel = step(0.999, spatialNoise.x) * step(0.999, spatialNoise.y);
    
    // Combine different noise sources
    float noise = (temporalNoise.x - 0.5) * 0.03 + 
                  (spatialNoise.x - 0.5) * 0.01 + 
                  deadPixel * -0.8;
    
    return noise;
}

// Apply IR-specific processing
vec3 processIR(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Simulate IR sensor response curve
    // IR cameras are sensitive to different wavelengths
    float irResponse = pow(luma, 0.6) * 1.4;
    
    // Aggressive contrast curve for IR
    irResponse = applyContrastCurve(irResponse, 0.4, 3.5, -0.2);
    
    // Edge enhancement for better object definition
    float edge = detectEdges(texCoord);
    irResponse += edge * edgeIntensity * 0.5;
    
    // IR inversion - hot objects appear white
    irResponse = 1.0 - irResponse;
    
    // Simulate digital gain and quantization
    irResponse *= 1.8; // Digital gain
    irResponse = floor(irResponse * 64.0) / 64.0; // Harsh quantization
    
    // Add realistic sensor noise
    irResponse += addSensorNoise(texCoord, time) * 0.4;
    
    // Clamp to valid range
    irResponse = clamp(irResponse, 0.0, 1.0);
    
    return vec3(irResponse);
}

// Apply enhanced IR processing (military grade)
vec3 processEnhancedIR(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Simulate high-end IR sensor with multiple processing stages
    float irSignal = pow(luma, 0.5) * 1.6;
    
    // Aggressive contrast curve with multiple stages
    irSignal = applyContrastCurve(irSignal, 0.3, 4.0, -0.3);
    
    // Multi-pass edge enhancement
    float edge = detectEdges(texCoord);
    irSignal += edge * edgeIntensity * 1.2;
    
    // Simulate automatic gain control (AGC)
    float avgBrightness = 0.5; // Could be calculated from surrounding area
    float agcGain = clamp(1.0 / (avgBrightness + 0.1), 0.5, 3.0);
    irSignal *= agcGain;
    
    // IR inversion - hot objects appear white
    irSignal = 1.0 - irSignal;
    
    // Harsh quantization to simulate digital processing
    irSignal = floor(irSignal * 32.0) / 32.0;
    
    // Add characteristic IR noise pattern
    irSignal += addSensorNoise(texCoord, time) * 0.6;
    
    // Digital sharpening filter
    float sharpening = detectEdges(texCoord) * 0.8;
    irSignal += sharpening;
    
    // Final clamp
    irSignal = clamp(irSignal, 0.0, 1.0);
    
    return vec3(irSignal);
}

// Apply monochrome with enhancement (night vision style)
vec3 processMonochrome(vec3 color, vec2 texCoord) {
    float luma = getLuminance(color);
    
    // Simulate low-light amplification
    luma = pow(luma, 0.7) * 2.2;
    
    // Apply contrast curve
    luma = applyContrastCurve(luma, gamma, gain, bias);
    
    // Strong edge enhancement for night vision
    float edge = detectEdges(texCoord);
    luma += edge * edgeIntensity * 0.6;
    
    // Add slight bloom effect for bright areas
    if (luma > 0.8) {
        luma += (luma - 0.8) * 0.5;
    }
    
    // Add subtle sensor noise
    luma += addSensorNoise(texCoord, time) * 0.15;
    
    // Clamp to prevent overexposure
    luma = clamp(luma, 0.0, 1.0);
    
    // Realistic night vision green tint with slight vignette
    float vignette = 1.0 - length(texCoord - 0.5) * 0.3;
    vec3 nightVision = vec3(luma * 0.2, luma * 0.9, luma * 0.15);
    nightVision *= vignette;
    
    return nightVision;
}

// Simulate heat shimmer effect
vec2 addHeatShimmer(vec2 texCoord, float time) {
    vec2 shimmer = vec2(
        sin(texCoord.y * 50.0 + time * 3.0) * 0.001,
        cos(texCoord.x * 30.0 + time * 2.0) * 0.0008
    );
    return texCoord + shimmer;
}

// Enhanced thermal color mapping with better temperature zones
vec3 getThermalColor(float temperature) {
    // Clamp temperature to valid range
    temperature = clamp(temperature, 0.0, 1.0);
    
    // More realistic thermal palette
    if (temperature < 0.15) {
        // Very cold - deep purple/black
        return mix(vec3(0.0, 0.0, 0.0), vec3(0.2, 0.0, 0.4), temperature / 0.15);
    } else if (temperature < 0.3) {
        // Cold - purple to blue
        float t = (temperature - 0.15) / 0.15;
        return mix(vec3(0.2, 0.0, 0.4), vec3(0.0, 0.2, 0.8), t);
    } else if (temperature < 0.45) {
        // Cool - blue to cyan
        float t = (temperature - 0.3) / 0.15;
        return mix(vec3(0.0, 0.2, 0.8), vec3(0.0, 0.6, 1.0), t);
    } else if (temperature < 0.6) {
        // Moderate - cyan to green
        float t = (temperature - 0.45) / 0.15;
        return mix(vec3(0.0, 0.6, 1.0), vec3(0.0, 1.0, 0.4), t);
    } else if (temperature < 0.75) {
        // Warm - green to yellow
        float t = (temperature - 0.6) / 0.15;
        return mix(vec3(0.0, 1.0, 0.4), vec3(1.0, 1.0, 0.0), t);
    } else if (temperature < 0.9) {
        // Hot - yellow to orange
        float t = (temperature - 0.75) / 0.15;
        return mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.5, 0.0), t);
    } else {
        // Very hot - orange to white
        float t = (temperature - 0.9) / 0.1;
        return mix(vec3(1.0, 0.5, 0.0), vec3(1.0, 1.0, 1.0), t);
    }
}

// Apply thermal simulation with enhanced processing
vec3 processThermal(vec3 color, vec2 texCoord) {
    // Add heat shimmer to texture coordinates
    vec2 shimmerCoord = addHeatShimmer(texCoord, time);
    vec3 shimmerColor = texture(screenTexture, shimmerCoord).rgb;
    
    // Use multiple factors to simulate thermal signatures
    float luma = getLuminance(shimmerColor);
    
    // Fake thermal signature based on color analysis
    // Sky/bright areas = cold, dark areas = potentially warm
    // Green vegetation = cooler, concrete/metal = warmer
    float thermalSignature = luma;
    
    // Enhance contrast for thermal effect
    thermalSignature = applyContrastCurve(thermalSignature, 0.8, 2.2, 0.1);
    
    // Simulate atmospheric effects - distant objects cooler
    float distanceFactor = 1.0 - (texCoord.y * 0.3); // Sky is further = cooler
    thermalSignature *= distanceFactor;
    
    // Add subtle noise to simulate sensor imperfections
    float thermalNoise = addSensorNoise(texCoord, time) * 0.1;
    thermalSignature += thermalNoise;
    
    // Apply thermal color mapping
    vec3 thermal = getThermalColor(thermalSignature);
    
    // Add subtle edge enhancement for object definition
    float edge = detectEdges(texCoord);
    thermal += edge * 0.2;
    
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