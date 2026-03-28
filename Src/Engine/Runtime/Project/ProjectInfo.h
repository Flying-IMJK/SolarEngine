#pragma once

#include "Core/Types/Strings/String.h"
#include "Core/Types/UID.h"
//#include "Engine/Core/Math/Ray.h"
#include "Core/Types/Collections/List.h"
#include "Core/Types/Collections/HashSet.h"

#include "Runtime/API.h"
#include "Version.h"

namespace SE
{
	/// Contains information about Flax project.
	/// </summary>
	class SE_API_RUNTIME ProjectInfo
	{
	public:

		/// <summary>
		/// The loaded projects cache.
		/// </summary>
		static List<ProjectInfo*> ProjectsCache;

	public:

		/// <summary>
		/// The project reference.
		/// </summary>
		struct Reference
		{
			/// <summary>
			/// The referenced project name.
			/// </summary>
			String Name;

			/// <summary>
			/// The referenced project.
			/// </summary>
			ProjectInfo* Project;
		};

	public:

		/// <summary>
		/// The project name.
		/// </summary>
		String Name = String::Empty;

		/// <summary>
		/// The absolute path to the project file.
		/// </summary>
		String ProjectPath = String::Empty;

		/// <summary>
		/// The project root folder path.
		/// </summary>
		String ProjectFolderPath = String::Empty;

		/// <summary>
		/// The project version.
		/// </summary>
		VersionInfo Version;

		/// <summary>
		/// The project publisher company.
		/// </summary>
		String Company = String::Empty;

		/// <summary>
		/// The project copyright note.
		/// </summary>
		String Copyright = String::Empty;

		/// <summary>
		/// The name of the build target to use for the game building (final, cooked game code).
		/// </summary>
		String GameTarget = String::Empty;

		/// <summary>
		/// The name of the build target to use for the game in editor building (editor game code).
		/// </summary>
		String EditorTarget = String::Empty;

		/// <summary>
		/// The project references.
		/// </summary>
		List<Reference> References;

		/// <summary>
		/// The default scene asset identifier to open on project startup.
		/// </summary>
		UID DefaultScene = UID::Empty;

		/// <summary>
		/// The default scene spawn point (position and view direction).
		/// </summary>
//			Ray DefaultSceneSpawn;

		/// <summary>
		/// The minimum version supported by this project.
		/// </summary>
		VersionInfo MinEngineVersion;

		/// <summary>
		/// The user-friendly nickname of the engine installation to use when opening the project. Can be used to open game project with a custom engine distributed for team members. This value must be the same in engine and game projects to be paired.
		/// </summary>
		String EngineNickname = String::Empty;

	public:

		ProjectInfo()
		{
			Version = VersionInfo(1, 0);
//			DefaultSceneSpawn = Ray(Float3::Zero, Float3::Forward);
			DefaultScene = UID::Empty;
		}

		/// <summary>
		/// Saves the project file (*.flaxproj).
		/// </summary>
		/// <returns>True if cannot save it, otherwise false.</returns>
		bool SaveProject();

		/// <summary>
		/// Loads the project file (*.flaxproj).
		/// </summary>
		/// <param name="projectPath">The absolute path to the file with a project.</param>
		/// <returns>True if cannot load it, otherwise false.</returns>
		bool LoadProject(const String& projectPath);

		/// <summary>
		/// Loads the old project file (Project.xml).
		/// [Deprecated: 16.04.2020, expires 16.04.2021]
		/// </summary>
		/// <param name="projectPath">The absolute path to the file with a project.</param>
		/// <returns>True if cannot load it, otherwise false.</returns>
		bool LoadOldProject(const String& projectPath);

		/// <summary>
		/// Gets all projects including this project, it's references and their references (any deep level of references).
		/// </summary>
		/// <param name="result">The result list of projects (this and all references).</param>
		void GetAllProjects(HashSet<ProjectInfo*>& result)
		{
			result.Add(this);
			for (auto& reference : References)
				if (reference.Project)
					reference.Project->GetAllProjects(result);
		}

		/// <summary>
		/// Loads the project from the specified file.
		/// </summary>
		/// <param name="path">The path.</param>
		/// <returns>The loaded project.</returns>
		static ProjectInfo* Load(const String& path);
	};

	inline uint32 GetHash(const ProjectInfo& key)
	{
		return GetHash(&key);
	}


} // SE
