#pragma once

#include "Runtime/Core/Types/Variable.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Types/Collections/HashSet.h"

#include <glslang/Public/ShaderLang.h>

#include <fstream>

namespace SE
{
    class ShaderIncluder : public glslang::TShader::Includer
    {
    public:
        ShaderIncluder() : externalLocalDirectoryCount(0), includedFiles() {}

        IncludeResult *includeLocal(const char *headerName, const char *includerName, uint64 inclusionDepth) override;

        IncludeResult *includeSystem(const char *headerName, const char * /*includerName*/,  uint64 /*inclusionDepth*/) override;

        //外部设置目录。
        // -最先检查最近推送的。
        // -所有这些都在本地目录的解析时堆栈之后进行检查
        // -这只适用于#include的“局部”形式。
        // -创建自己的路径副本。
        void pushExternalLocalDirectory(const String &dir);

        void releaseInclude(IncludeResult *result) override;

        HashSet<String> getIncludedFiles();

        virtual ~ShaderIncluder() override {}

    protected:
        List<String> directoryStack;
        int externalLocalDirectoryCount;
        HashSet<String> includedFiles;

        /*!
         * @brief 根据include的堆栈组合搜索有效的“本地”路径，目录和头文件的名义名称。
         * @param headerName 
         * @param includerName 
         * @param depth 
         * @return 
         */
        IncludeResult *ReadLocalPath(const char *headerName, const char *includerName, int depth);

        // Search for a valid <system> path.
        // Not implemented yet; returning nullptr signals failure to find.
        virtual IncludeResult *ReadSystemPath(const char * headerName) const
        {
            return nullptr;
        }

        
        /*!
         * @brief 实际读取文件，填充新的include。
         * @param path 
         * @param file 
         * @param length 
         * @return 
         */
        IncludeResult *NewIncludeResult(const String &path, std::ifstream &file, int length) const;


        /*!
         * @brief 如果没有路径标记，则返回当前工作目录，否则，删除文件名并返回指向该文件的路径。
         * @param path 
         * @return 
         */
        String GetDirectory(const String path) const;
    };

} // namespace SE