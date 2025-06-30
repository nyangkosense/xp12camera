#version 330 core

/*
 * Vertex shader for IR camera post-processing effects
 *
 * MIT License
 * 
 * Copyright (c) 2025 sebastian <sebastian@eingabeausgabe.io>
 */

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}