#pragma once

#include "Core/Types/Collections/List.h"
#include "ITokenReader.h"

namespace SE::ShaderParser
{
    /// <summary>
    /// Interface for objects that can have child token readers
    /// </summary>
    template<typename Type>
    class ITokenReadersContainerBase
    {
    protected:

        List<Type*> _childReaders;

    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~ITokenReadersContainerBase()
        {
            // Cleanup
            _childReaders.ClearDelete();
        }

    protected:

        /// <summary>
        /// Try to process given token by any child reader
        /// </summary>
        /// <param name="token">Starting token to check</param>
        /// <param name="parser">Parser object</param>
        /// <returnsFalse if no token processing has been done, otherwise true</returns>
        virtual bool ProcessChildren(const Token& token, IShaderParser* parser)
        {
            for (int32 i = 0; i < _childReaders.Count(); i++)
            {
                if (_childReaders[i]->CheckStartToken(token))
                {
                    _childReaders[i]->Process(parser, parser->GetReader());
                    return true;
                }
            }

            return false;
        }
    };

    /// <summary>
    /// Interface for objects that can have child ITokenReader objects
    /// </summary>
    class ITokenReadersContainer : public ITokenReadersContainerBase<ITokenReader>
    {
    public:

        /// <summary>
        /// Virtual destructor
        /// </summary>
        virtual ~ITokenReadersContainer()
        {
            // Cleanup
            _childReaders.ClearDelete();
        }
    };
}

