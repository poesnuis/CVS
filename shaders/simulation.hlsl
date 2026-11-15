// Linear-light sRGB simulation. CPU reference: src/core/color.cpp.
Texture2D<float4> sourceTexture : register(t0);
SamplerState sourceSampler : register(s0);
cbuffer Simulation : register(b0) {
    float4 first[3]; float4 second[3]; float4 separator;
    float4 artistic[3]; float4 adjustment;
};
struct Vertex { float4 position : SV_Position; float2 uv : TEXCOORD0; };
Vertex VS(uint id : SV_VertexID) {
    Vertex v; v.uv=float2((id << 1) & 2, id & 2);
    v.position=float4(v.uv * float2(2,-2)+float2(-1,1),0,1); return v;
}
float linearChannel(float x) { return x <= 0.04045 ? x / 12.92 : pow((x + 0.055)/1.055, 2.4); }
float encodedChannel(float x) { x=saturate(x); return x <= 0.0031308 ? x*12.92 : 1.055*pow(x,1.0/2.4)-0.055; }
float4 PS(Vertex v) : SV_Target {
    float3 c=saturate(sourceTexture.Sample(sourceSampler,v.uv).rgb);
    c=float3(linearChannel(c.r),linearChannel(c.g),linearChannel(c.b));
    if(dot(c,separator.xyz)<0) c=float3(dot(second[0].xyz,c),dot(second[1].xyz,c),dot(second[2].xyz,c));
    else c=float3(dot(first[0].xyz,c),dot(first[1].xyz,c),dot(first[2].xyz,c));
    c=float3(dot(artistic[0].xyz,c),dot(artistic[1].xyz,c),dot(artistic[2].xyz,c));
    c=(c*adjustment.x-adjustment.z)*adjustment.y+adjustment.z;
    return float4(encodedChannel(c.r),encodedChannel(c.g),encodedChannel(c.b),1);
}
