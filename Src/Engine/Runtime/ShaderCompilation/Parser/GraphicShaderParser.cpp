// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "GraphicShaderParser.h"

#include "Core/Types/Collections/List.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Core/Logging/Logging.h"
#include "TextProcessing.h"
#include "ShaderFunctionReader.CB.h"
#include "ShaderFunctionReader.VS.h"
#include "ShaderFunctionReader.HS.h"
#include "ShaderFunctionReader.DS.h"
#include "ShaderFunctionReader.GS.h"
#include "ShaderFunctionReader.PS.h"
#include "ShaderFunctionReader.CS.h"
#include "Config.h"

namespace SE::ShaderParser
{
	Parser::Parser(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel)
		: failed(false), targetName(targetName), text(source, sourceLength), _macros(macros), _featureLevel(featureLevel)
	{
	}

	Parser::~Parser()
	{
	}

	bool Parser::Process(const String& targetName, const char* source, int32 sourceLength, ParserMacros macros, FeatureLevel featureLevel, ShaderMeta* result)
	{
		PROFILE_CPU_NAMED("Shader.Parse");
		Parser parser(targetName, source, sourceLength, macros, featureLevel);
		parser.Process(result);
		return !parser.Failed();
	}

	void Parser::Process(ShaderMeta* result)
	{
		init();

		failed = process();

		if (!failed)
			failed = collectResults(result);
	}

	void Parser::init()
	{
		// Init text processing tokens for hlsl language
		text.Setup_HLSL();

		// Init shader functions readers
		_childReaders.Add(New<ConstantBufferReader>());
		_childReaders.Add(New<VertexShaderFunctionReader>());
		_childReaders.Add(New<HullShaderFunctionReader>());
		_childReaders.Add(New<DomainShaderFunctionReader>());
		_childReaders.Add(New<GeometryShaderFunctionReader>());
		_childReaders.Add(New<PixelShaderFunctionReader>());
		_childReaders.Add(New<ComputeShaderFunctionReader>());
	}

	bool Parser::process()
	{
		const Token defineToken("#define");
		const Separator singleLineCommentSeparator('/', '/');
		const Separator multiLineCommentSeparator('/', '*');

		// TODO: split parsing into two phrases: comments preprocessing and parsing

		// Read whole source code
		Token token;
		while (text.CanRead())
		{
			text.ReadToken(&token);

			// Single line comment
			if (token.Separator == singleLineCommentSeparator)
			{
				// Read whole line
				text.ReadLine();
			}
				// Multi line comment
			else if (token.Separator == multiLineCommentSeparator)
			{
				// Read tokens until end sequence
				char prev = ' ';
				char c;
				while (text.CanRead())
				{
					c = text.ReadChar();
					if (prev == '*' && c == '/')
					{
						break;
					}
					prev = c;
				}

				// Check if comment is valid (has end before file end)
				if (!text.CanRead())
				{
					OnWarning(SE_TEXT("Missing multiline comment ending"));
				}
			}
				// Preprocessor definition
			else if (token == defineToken)
			{
				// Skip
				text.ReadLine();
			}
			else
			{
				// Call children
				ProcessChildren(token, this);
			}
		}

		return false;
	}

	bool Parser::collectResults(ShaderMeta* result)
	{
		// Collect results from all the readers
		for (int32 i = 0; i < _childReaders.Count(); i++)
			_childReaders[i]->CollectResults(this, result);

		return false;
	}

	void Parser::OnError(const String& message)
	{
		// Set flag
		failed = true;

		// Send event
		LOG_ERROR("Shader", "ShaderParser Processing shader '{0}' error at line {1}. {2}", targetName, text.GetLine(), message);
	}

	void Parser::OnWarning(const String& message)
	{
		// Send event
		LOG_WARNING("Shader", "ShaderParser Processing shader '{0}' warning at line {1}. {2}", targetName, text.GetLine(), message);
	}

}