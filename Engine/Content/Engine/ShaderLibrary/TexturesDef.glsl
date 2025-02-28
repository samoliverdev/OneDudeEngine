#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

TextureCube(0, 1, _IrradianceMap, _IrradianceMapSampler)
TextureCube(0, 2, _PrefilterMap, _PrefilterMapSampler)
Texture2D(0, 3, _BrdfLUT, _BrdfLUTSampler)
Texture2DArray(0, 4, _DirectionalShadowAtlas, _DirectionalShadowAtlasSampler)
Texture2DArray(0, 5, _OtherShadowAtlas, _OtherShadowAtlasSampler)

//Uniform mediump sampler2DArray _DirectionalShadowAtlas;
//Uniform mediump sampler2DArray _OtherShadowAtlas;

#endif