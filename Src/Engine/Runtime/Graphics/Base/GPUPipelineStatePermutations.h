#pragma once

#include "GPUPipelineState.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"

namespace SE
{
    template <int Size>
    class GPUPipelineStatePermutations
    {
    public:
        GPUPipelineState *States[Size];

    public:
        GPUPipelineStatePermutations()
        {
            Platform::MemoryClear(States, sizeof(States));
        }

        ~GPUPipelineStatePermutations()
        {
            Delete();
        }

    public:
        bool IsValid() const
        {
            for (int i = 0; i < Size; i++)
            {
                if (States[i] == nullptr || !States[i]->IsValid())
                {
                    return false;
                }
            }

            return true;
        }

        FORCE_INLINE GPUPipelineState *Get(int index) const
        {
            return States[index];
        }

        FORCE_INLINE GPUPipelineState *&operator[](int32 index)
        {
            return States[index];
        }

    public:
        void CreatePipelineStates()
        {
            for (int i = 0; i < Size; i++)
            {
                if (States[i] == nullptr)
                {
                    States[i] = GPUDevice::instance->CreatePipelineState();
                }
            }
        }

        void Release()
        {
            for (int i = 0; i < Size; i++)
            {
                if (States[i])
                {
                    States[i]->ReleaseGPU();
                }
            }
        }

        void Delete()
        {
            for (int i = 0; i < Size; i++)
            {
                if (States[i])
                {
                    States[i]->DeleteObjectNow();
                    States[i] = nullptr;
                }
            }
        }
    };

    template <int Size>
    class GPUPipelineStatePermutationsPs : public GPUPipelineStatePermutations<Size>
    {
    public:
        typedef GPUPipelineStatePermutations<Size> Base;

    public:
        GPUPipelineStatePermutationsPs()
        {
        }

        ~GPUPipelineStatePermutationsPs()
        {
        }

    public:
        bool Create(GPUPipelineState::Description &desc, GPUShader *shader, const StringView &psName)
        {
            for (int i = 0; i < Size; i++)
            {
                ENGINE_ASSERT(Base::States[i]);

                desc.PS = shader->GetPS(psName, i);
                if (!Base::States[i]->Init(desc))
                {
                    return false;
                }
            }

            return true;
        }
    };

    template <int Size>
    class ComputeShaderPermutation
    {
    public:
        GPUShaderProgramCS *Shaders[Size];

    public:
        ComputeShaderPermutation()
        {
            Platform::MemoryClear(Shaders, sizeof(Shaders));
        }

        ~ComputeShaderPermutation()
        {
        }

    public:
        FORCE_INLINE GPUShaderProgramCS *Get(const int index) const
        {
            return Shaders[index];
        }

    public:
        void Clear()
        {
            Platform::MemoryClear(Shaders, sizeof(Shaders));
        }

        void Get(GPUShader *shader, const StringView &name)
        {
            for (int i = 0; i < Size; i++)
            {
                Shaders[i] = shader->GetCS(name, i);
            }
        }
    };
}
