#pragma once

#include "../Database/ReflectionDatabase.h"
#include "../mustache.hpp"
#include <sstream>

using namespace kainjow;

namespace SE::BuildTool
{
    class Generator
    {
    public:
        Generator() : m_pDatabase(nullptr) {}
        ~Generator() {}
        bool Generate(ReflectionDatabase const &database, SolutionInfo const &solution);
        char const *GetErrorMessage() const { return m_errorMessage.c_str(); }

        template<typename... Params>
        bool LogError(const char * pErrorFormat, Params... params) const
        {
            m_errorMessage = Utils::String::Format(pErrorFormat, params...);
            return false;
        }

    private:
        void LoadTemplateFile(SolutionInfo const &solution);

        // File specific functions
        void GenerateTypeInfoFileHeader(HeaderInfo const &hdr, std::string_view solutionPath);
        void AppendBindingIncludesIfNeeded();
        void GenerateModuleCodeFile(ReflectionDatabase const& database, ProjectInfo const &prj, std::vector<TypeData> const &typesInModule);

        // Utils
        static bool SaveStreamToFile(std::string const &filePath, std::stringstream &stream);
    private:
        ReflectionDatabase const *m_pDatabase;
        std::stringstream m_typeInfoFile;
        std::stringstream m_moduleFile;
        std::stringstream m_engineTypeRegistrationFile;
        std::stringstream m_toolsTypeRegistrationFile;
        bool m_typeInfoFileHasBinding = false;
        mutable std::string m_errorMessage;

        std::string m_CodeModuleTemplate;
        std::string m_CodeCppMetaTemplate;
		std::string m_CodeCppEnumTemplate;
		std::string m_CodeCppClassTemplate;
    };
}