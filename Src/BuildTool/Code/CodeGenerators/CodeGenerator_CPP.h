#pragma once

#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"
#include <sstream>

using namespace kainjow;

namespace SE::ReflectTool
{
    class Generator
    {
    public:
        Generator() : m_pDatabase(nullptr) {}
        ~Generator() {}
        bool Generate(ReflectionDatabase const &database, SolutionInfo const &solution);
        char const *GetErrorMessage() const { return m_errorMessage.Get(); }

        template<typename... Params>
        bool LogError(const char * pErrorFormat, Params... params) const
        {
            m_errorMessage = StringAnsi::Format(pErrorFormat, params...);
            return false;
        }

    private:
        void LoadTemplateFile(SolutionInfo const &solution);

        // File specific functions
        void GenerateTypeInfoFileHeader(HeaderInfo const &hdr, StringView solutionPath);
        void AppendBindingIncludesIfNeeded();
        void GenerateModuleCodeFile(ReflectionDatabase const& database, ProjectInfo const &prj, List<DataType> const &typesInModule);

        // Utils
        static bool SaveStreamToFile(String const &filePath, std::stringstream &stream);
    private:
        ReflectionDatabase const *m_pDatabase;
        std::stringstream m_typeInfoFile;
        std::stringstream m_moduleFile;
        std::stringstream m_engineTypeRegistrationFile;
        std::stringstream m_toolsTypeRegistrationFile;
        bool m_typeInfoFileHasBinding = false;
        mutable StringAnsi m_errorMessage;

        StringAnsi m_CodeModuleTemplate;
        StringAnsi m_CodeCppMetaTemplate;
		StringAnsi m_CodeCppEnumTemplate;
		StringAnsi m_CodeCppClassTemplate;
    };
}