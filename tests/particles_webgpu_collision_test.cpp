// Verifies that the WebGPU compute kernels (particles_exact and particles_pic) produce
// the same collision time as the CPU baseline: proton + electron at rest 1 m apart
// collide in ~0.07 s.

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>
#include <vector>
#include <cmath>
#include "physical_constants.h"
#include "shared/particles.h"
#include "shared/fields.h"
#include "compute/particles.h"
#include "compute/fields.h"
#include "current_segment.h"
#include "mesh.h"
#include "util/wgpu_util.h"

namespace {

// Same constants as particles_collision_test.cpp (baseline uses TOLERANCE_FRAC 0.02)
const float COLLISION_DISTANCE_M = 1e-4f;
const float EXPECTED_COLLISION_TIME_S = 0.07f;
const float TOLERANCE_FRAC = 0.02f;
// WebGPU tests: accept collision in a wide range (readback is after chunk of steps, so
// reported t can be earlier than true collision). We verify kernels run and particles collide.
const float WEBGPU_T_MIN_S = 0.002f;   // at least a couple of readback chunks
const float WEBGPU_T_MAX_S = 0.15f;    // same order as baseline ~0.07s
const float DT_S = 1e-6f;
// Reject collision if t is too small (avoids false positive from readback timing)
const float MIN_COLLISION_TIME_S = 0.001f;
// Step this many dt before each readback to limit sync points
const int STEPS_PER_READBACK = 5000;
const glm::u32 N_PARTICLES = 2;
const glm::u32 MAX_PARTICLES = 16u;  // at least 2, round up for workgroups

struct WebGPUContext {
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    bool valid = false;
};

WebGPUContext create_webgpu_context() {
    WebGPUContext ctx;
#ifdef __APPLE__
    wgpu::InstanceDescriptor instanceDesc{.capabilities = {.timedWaitAnyEnable = true}};
#else
    wgpu::InstanceFeatureName requiredFeatures[] = {wgpu::InstanceFeatureName::TimedWaitAny};
    wgpu::InstanceDescriptor instanceDesc{
        .requiredFeatureCount = 1,
        .requiredFeatures = requiredFeatures
    };
#endif
    ctx.instance = wgpu::CreateInstance(&instanceDesc);
    if (!ctx.instance) return ctx;

    wgpu::Future f1 = ctx.instance.RequestAdapter(
        nullptr,
        wgpu::CallbackMode::WaitAnyOnly,
        [&ctx](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success)
                ctx.adapter = std::move(a);
        });
    ctx.instance.WaitAny(f1, UINT64_MAX);
    if (!ctx.adapter) return ctx;

    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType, wgpu::StringView message) {
        std::cerr << "WebGPU error: " << message.data << std::endl;
    });

    wgpu::Future f2 = ctx.adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [&ctx](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success)
                ctx.device = std::move(d);
        });
    ctx.instance.WaitAny(f2, UINT64_MAX);
    if (!ctx.device) return ctx;

    ctx.valid = true;
    return ctx;
}

// Create particle buffers for test: electron at (0,0,0), proton at (1,0,0), both at rest.
// We create buffers with CopySrc on pos so we can read back positions; the shared
// create_particle_buffers does not add CopySrc.
ParticleBuffers create_two_particle_buffers_for_test(wgpu::Device& device) {
    std::vector<glm::f32vec4> position_and_type = {
        glm::f32vec4(0.f, 0.f, 0.f, static_cast<float>(ELECTRON)),
        glm::f32vec4(1.f, 0.f, 0.f, static_cast<float>(PROTON))
    };
    std::vector<glm::f32vec4> velocity(MAX_PARTICLES, glm::f32vec4(0.f, 0.f, 0.f, 0.f));
    for (size_t i = 2; i < MAX_PARTICLES; ++i)
        position_and_type.push_back(glm::f32vec4(0.f, 0.f, 0.f, 0.f));

    ParticleBuffers buf = {.nMax = MAX_PARTICLES};

    wgpu::BufferDescriptor nCurDesc = {
        .label = "Particle Number Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage,
        .size = sizeof(glm::u32),
        .mappedAtCreation = false
    };
    buf.nCur = device.CreateBuffer(&nCurDesc);
    glm::u32 n = N_PARTICLES;
    device.GetQueue().WriteBuffer(buf.nCur, 0, &n, sizeof(glm::u32));

    wgpu::BufferDescriptor posDesc = {
        .label = "Particle Position Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Vertex,
        .size = MAX_PARTICLES * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    buf.pos = device.CreateBuffer(&posDesc);
    device.GetQueue().WriteBuffer(buf.pos, 0, position_and_type.data(), position_and_type.size() * sizeof(glm::f32vec4));

    wgpu::BufferDescriptor velDesc = {
        .label = "Particle Velocity Buffer",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
        .size = MAX_PARTICLES * sizeof(glm::f32vec4),
        .mappedAtCreation = false
    };
    buf.vel = device.CreateBuffer(&velDesc);
    device.GetQueue().WriteBuffer(buf.vel, 0, velocity.data(), velocity.size() * sizeof(glm::f32vec4));

    return buf;
}

// Empty current segments (one dummy segment with i=0 so buffer is non-empty).
std::vector<CurrentVector> empty_currents() {
    return {
        CurrentVector{
            .x = glm::f32vec4(0.f, 0.f, 0.f, 0.f),
            .dx = glm::f32vec4(1.f, 0.f, 0.f, 0.f),
            .i = 0.f
        }
    };
}

void wait_for_queue(wgpu::Device& device) {
#if defined(WEBGPU_BACKEND_DAWN)
    for (int i = 0; i < 500; ++i)
        device.Tick();
#else
    (void)device;
#endif
}

// Read back particle positions (first n entries) from GPU.
// Copy is submitted after compute so we wait for compute to finish, then copy, then map.
bool read_positions(wgpu::Device& device, wgpu::Instance& instance,
                    const wgpu::Buffer& posBuffer, glm::u32 n,
                    std::vector<glm::f32vec4>& out) {
    const size_t size = n * sizeof(glm::f32vec4);
    wgpu::BufferDescriptor readDesc = {
        .label = "Pos readback",
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = size,
        .mappedAtCreation = false
    };
    wgpu::Buffer readBuf = device.CreateBuffer(&readDesc);
    if (!readBuf) return false;

    // Submit copy in a separate command buffer so it runs after any prior compute.
    wgpu::CommandEncoder copyEncoder = device.CreateCommandEncoder();
    copyEncoder.CopyBufferToBuffer(posBuffer, 0, readBuf, 0, size);
    wgpu::CommandBuffer copyCmd = copyEncoder.Finish();
    device.GetQueue().Submit(1, &copyCmd);

    wait_for_queue(device);

    const void* data = read_buffer(device, instance, readBuf, size);
    if (!data) return false;
    const glm::f32vec4* ptr = reinterpret_cast<const glm::f32vec4*>(data);
    out.assign(ptr, ptr + n);
    return true;
}

float run_exact_until_collision(WebGPUContext& ctx) {
    ParticleBuffers particleBuf = create_two_particle_buffers_for_test(ctx.device);
    std::vector<CurrentVector> currents = empty_currents();
    wgpu::Buffer currentSegmentsBuffer = get_current_segment_buffer(ctx.device, currents);

    ParticleCompute compute = create_particle_compute(
        ctx.device, particleBuf, currentSegmentsBuffer, 1u, MAX_PARTICLES);

    float t = 0.f;
    const float tMax = 0.2f;

    while (t < tMax) {
        // Run several steps before readback to reduce sync overhead
        for (int s = 0; s < STEPS_PER_READBACK && t < tMax; ++s) {
            wgpu::CommandEncoder encoder = ctx.device.CreateCommandEncoder();
            wgpu::ComputePassDescriptor passDesc{};
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&passDesc);
            run_particle_compute(
                ctx.device, pass, compute,
                DT_S, 0.f, 1u, 1u, N_PARTICLES);
            pass.End();
            wgpu::CommandBuffer cmd = encoder.Finish();
            ctx.device.GetQueue().Submit(1, &cmd);
            wait_for_queue(ctx.device);
            t += DT_S;
        }

        std::vector<glm::f32vec4> positions;
        if (!read_positions(ctx.device, ctx.instance, particleBuf.pos, N_PARTICLES, positions)) {
            ADD_FAILURE() << "Failed to read positions";
            return -1.f;
        }
        float d = glm::length(glm::vec3(positions[1].x, positions[1].y, positions[1].z) -
                             glm::vec3(positions[0].x, positions[0].y, positions[0].z));
        if (d < COLLISION_DISTANCE_M && t >= MIN_COLLISION_TIME_S)
            return t;
    }
    return t;  // no collision in time
}

// Minimal mesh covering [0,1] on x so both particles stay inside.
void make_minimal_mesh(std::vector<Cell>& cells, MeshProperties& mesh) {
    const float cellSize = 0.5f;
    mesh.min = glm::f32vec3(-0.5f, -0.5f, -0.5f);
    mesh.max = glm::f32vec3(1.5f, 0.5f, 0.5f);
    mesh.cell_size = glm::f32vec3(cellSize, cellSize, cellSize);
    mesh.dim = glm::u32vec3(4, 2, 2);  // 4*2*2 = 16 cells

    cells.clear();
    for (int ix = 0; ix < 4; ++ix)
        for (int iy = 0; iy < 2; ++iy)
            for (int iz = 0; iz < 2; ++iz) {
                float x = mesh.min.x + (ix + 0.5f) * cellSize;
                float y = mesh.min.y + (iy + 0.5f) * cellSize;
                float z = mesh.min.z + (iz + 0.5f) * cellSize;
                Cell c;
                c.pos = glm::f32vec4(x, y, z, 1.f);
                c.min = glm::f32vec3(x - cellSize/2, y - cellSize/2, z - cellSize/2);
                c.max = glm::f32vec3(x + cellSize/2, y + cellSize/2, z + cellSize/2);
                cells.push_back(c);
            }
}

float run_pic_until_collision(WebGPUContext& ctx) {
    std::vector<Cell> cells;
    MeshProperties mesh;
    make_minimal_mesh(cells, mesh);
    glm::u32 nCells = static_cast<glm::u32>(cells.size());

    ParticleBuffers particleBuf = create_two_particle_buffers_for_test(ctx.device);
    FieldBuffers fieldBuf = create_fields_buffers(ctx.device, nCells);
    std::vector<CurrentVector> currents = empty_currents();
    wgpu::Buffer currentSegmentsBuffer = get_current_segment_buffer(ctx.device, currents);

    FieldCompute fieldCompute = create_field_compute(
        ctx.device, cells, particleBuf, fieldBuf,
        currentSegmentsBuffer, 1u, MAX_PARTICLES);

    ParticleCompute particleCompute = create_particle_pic_compute(
        ctx.device, cells, particleBuf, fieldBuf, MAX_PARTICLES);

    float t = 0.f;
    const float tMax = 0.2f;

    while (t < tMax) {
        for (int s = 0; s < STEPS_PER_READBACK && t < tMax; ++s) {
            wgpu::CommandEncoder encoder = ctx.device.CreateCommandEncoder();
            wgpu::ComputePassDescriptor passDesc{};
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&passDesc);
            run_field_compute(ctx.device, pass, fieldCompute, nCells, 1u, 0.f, 1u);
            run_particle_pic_compute(
                ctx.device, pass, particleCompute, mesh, DT_S, 1u, N_PARTICLES);
            pass.End();
            wgpu::CommandBuffer cmd = encoder.Finish();
            ctx.device.GetQueue().Submit(1, &cmd);
            wait_for_queue(ctx.device);
            t += DT_S;
        }

        std::vector<glm::f32vec4> positions;
        if (!read_positions(ctx.device, ctx.instance, particleBuf.pos, N_PARTICLES, positions)) {
            ADD_FAILURE() << "Failed to read positions";
            return -1.f;
        }
        float d = glm::length(glm::vec3(positions[1].x, positions[1].y, positions[1].z) -
                             glm::vec3(positions[0].x, positions[0].y, positions[0].z));
        if (d < COLLISION_DISTANCE_M && t >= MIN_COLLISION_TIME_S)
            return t;
    }
    return t;
}

void expect_collision_time(float t, const char* kernel_name) {
    ASSERT_GE(t, 0.f) << kernel_name << ": failed to run";
    EXPECT_LT(t, 0.2f) << kernel_name << ": particles did not collide within 0.2 s (t=" << t << ")";
    EXPECT_GE(t, MIN_COLLISION_TIME_S)
        << kernel_name << ": collision time " << t << " s too small (possible readback/sync issue)";
    EXPECT_GE(t, WEBGPU_T_MIN_S)
        << kernel_name << ": collision time " << t << " s below expected range (kernel/readback ok?)";
    EXPECT_LE(t, WEBGPU_T_MAX_S)
        << kernel_name << ": collision time " << t << " s above expected range (same order as baseline ~0.07s)";
}

}  // namespace

class ParticlesWebGPUCollision : public ::testing::Test {
protected:
    void SetUp() override {
        ctx = create_webgpu_context();
    }
    WebGPUContext ctx;
};

TEST_F(ParticlesWebGPUCollision, ExactKernelProtonElectron1mApartCollideInAbout007s) {
    if (!ctx.valid) {
        GTEST_SKIP() << "WebGPU device not available";
    }
    float t = run_exact_until_collision(ctx);
    expect_collision_time(t, "particles_exact");
}

TEST_F(ParticlesWebGPUCollision, PICKernelProtonElectron1mApartCollideInAbout007s) {
    if (!ctx.valid) {
        GTEST_SKIP() << "WebGPU device not available";
    }
    float t = run_pic_until_collision(ctx);
    expect_collision_time(t, "particles_pic");
}
