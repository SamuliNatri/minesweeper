cbuffer constants : register(b0)
{
    row_major float4x4 model;
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 color;
	float u_offset;
	float v_offset;
};

struct VS_Input
{
	float3 position: POSITION;
	float2 uv: UV;
};

struct VS_Output
{
	float4 position: SV_POSITION;
	float4 color: COLOR;
	float2 uv: UV;
};

VS_Output vs_main(VS_Input input)
{
	VS_Output output;
	output.position = mul(float4(input.position, 1.0f), mul(mul(model, view), projection));
	output.color = color;
	input.uv.x += u_offset;
	input.uv.y += v_offset;
	output.uv = input.uv;
	return output;
};

Texture2D my_texture;
SamplerState my_sampler;

float4 ps_main(VS_Output input): SV_TARGET
{
	return my_texture.Sample(my_sampler, input.uv) * input.color;
	// return input.color;
};