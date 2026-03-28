#include "ShaderIncluder.h"
#include "Core/Types/Strings/StringBuilder.h"

#include <algorithm>

namespace SE
{

    glslang::TShader::Includer::IncludeResult *ShaderIncluder::includeLocal(const char *headerName, const char *includerName, uint64 inclusionDepth)
    {
        return ReadLocalPath(headerName, includerName, (int)inclusionDepth);
    }

    glslang::TShader::Includer::IncludeResult *ShaderIncluder::includeSystem(const char *headerName, const char *, uint64)
    {
        return ReadSystemPath(headerName);
    }

    void ShaderIncluder::pushExternalLocalDirectory(const String &dir)
    {
        directoryStack.Add(dir);
        externalLocalDirectoryCount = (int)directoryStack.Count();
    }

    void ShaderIncluder::releaseInclude(IncludeResult *result)
    {
        if (result != nullptr)
        {
            delete[] static_cast<char*>(result->userData);
            delete result;
        }
    }

    HashSet<String> ShaderIncluder::getIncludedFiles()
    {
        return includedFiles;
    }

    glslang::TShader::Includer::IncludeResult *ShaderIncluder::ReadLocalPath(const char *headerName, const char *includerName, int depth)
    {
        // Discard popped include directories, and
        // initialize when at parse-time first level.
        directoryStack.Resize(depth + externalLocalDirectoryCount);
        if (depth == 1)
            directoryStack.Last() = GetDirectory(includerName);

		StringBuilder stringBuilder;
        // Find a directory that works, using a reverse search of the include stack.
        for (int index = directoryStack.Count() - 1; index >= 0; --index)
        {
			stringBuilder.Clear();
			stringBuilder.Append(directoryStack[index]);
			stringBuilder.Append('/');
			stringBuilder.Append(headerName);

			String path = stringBuilder.ToString();
			path.Replace('\\', '/');

            std::ifstream file(path.Get(), std::ios_base::binary | std::ios_base::ate);
            if (file)
            {
                directoryStack.Add(GetDirectory(path));
                includedFiles.Add(path);
                return NewIncludeResult(path, file, (int)file.tellg());
            }
        }

        return nullptr;
    }

    glslang::TShader::Includer::IncludeResult *ShaderIncluder::NewIncludeResult(const String &path, std::ifstream &file, int length) const
    {
        char *content = new char[length];
        file.seekg(0, file.beg);
        file.read(content, length);
        return new IncludeResult(path.Get(), content, length, content);
    }

    String ShaderIncluder::GetDirectory(const String path) const
    {
        int last = path.FindLast("/\\");
        return last == INVALID_INDEX ? "." : path.Substring(0, last);
    }
}