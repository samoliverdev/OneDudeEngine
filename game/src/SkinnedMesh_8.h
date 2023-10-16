#pragma once

#include <OD/OD.h>

/*#include <ozz/base/io/stream.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/log.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>*/

#include <stdlib.h>

using namespace OD;

struct SkinnedMesh_8: public OD::Module {

    const char* filename = "res/Ozz/ruby_skeleton.ozz";

    void OnInit() override {
        /*
        ozz::io::File file(filename, "rb");

        if(!file.opened()){
            LogError("Cannot open file");
            Application::Quit();
            return;
        }

        ozz::io::IArchive archive(&file);

        if(!archive.TestTag<ozz::animation::Skeleton>()){
            LogError("Archive doesn't contain the expected object type.");
            Application::Quit();
            return;
        }

        ozz::animation::Skeleton skeleton;
        archive >> skeleton;

        ozz::animation::SamplingJob sampling_job;
        ozz::vector<ozz::math::SoaTransform> locals_;
        sampling_job.output = make_span(locals_);
        */
    }

    void OnUpdate(float deltaTime) override {
        
    }

    void OnRender(float deltaTime) override {

    }

    void OnGUI() override {

    }

    void OnResize(int width, int height) override {}
};