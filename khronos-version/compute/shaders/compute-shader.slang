struct UniformBuffer {
    float deltaTime;
};
ConstantBuffer<UniformBuffer> ubo;

struct Particle {
    float2 position;
    float2 velocity;
    float3 color;
};

// SSBO = shader storage buffer object
/*
struct ParticleSSBO {
    Particle particle;
};

StructuredBuffer<ParticleSSBO> particlesIn;
RWStructuredBuffer<ParticleSSBO> particlesOut;
*/

StructuredBuffer<Particle> particlesIn;
RWStructuredBuffer<Particle> particlesOut;

[shader("compute")]
[numthreads(256, 1, 1)]
void compMain(uint3 threadId: SV_DispatchThreadId) {
    uint index = threadId.x;
    /*
    particlesOut[index].particle.position = particlesIn[index].particle.position + ubo.deltaTime * particlesIn[index].particle.velocity;

    particlesOut[index].particle.velocity = particlesIn[index].particle.velocity;

    // Flip velocity if particle went out of window border
    if (particlesOut[index].particle.position.x < -1.0 || particlesOut[index].particle.position.x > 1.0) {
        particlesOut[index].particle.velocity.x *= -1;
    }
    if (particlesOut[index].particle.position.y < -1.0 || particlesOut[index].particle.position.y > 1.0) {
        particlesOut[index].particle.velocity.y *= -1;
    }
    */

    particlesOut[index].position = particlesIn[index].position + ubo.deltaTime * particlesIn[index].velocity;

    particlesOut[index].velocity = particlesIn[index].velocity;

    // Flip velocity if particle went out of window border
    if (particlesOut[index].position.x < -1.0 || particlesOut[index].position.x > 1.0) {
        particlesOut[index].velocity.x *= -1;
    }
    if (particlesOut[index].position.y < -1.0 || particlesOut[index].position.y > 1.0) {
        particlesOut[index].velocity.y *= -1;
    }
}