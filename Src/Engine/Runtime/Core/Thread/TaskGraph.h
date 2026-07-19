
#pragma once

#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE::Threading
{
	class TaskGraph;

	/// <summary>
	/// System that can generate work into Task Graph for asynchronous execution.
	/// </summary>
	class SE_API_RUNTIME TaskGraphSystem /*: public ScriptingObject*/
	{
		friend TaskGraph;
	private:
		List<TaskGraphSystem*, InlinedAllocation<16>> _dependencies;
		List<TaskGraphSystem*, InlinedAllocation<16>> _reverseDependencies;

	public:
		/// <summary>
		/// The execution order of the system (systems with higher order are executed later, lower first).
		/// </summary>
		int32 Order = 0;

	public:
		virtual ~TaskGraphSystem();

		/// <summary>
		/// Adds the dependency on the system execution. Before this system can be executed the given dependant system has to be executed first.
		/// </summary>
		/// <param name="system">The system to depend on.</param>
		void AddDependency(TaskGraphSystem* system);

		/// <summary>
		/// Removes the dependency on the system execution.
		/// </summary>
		/// <param name="system">The system to not depend on anymore.</param>
		void RemoveDependency(TaskGraphSystem* system);

		/// <summary>
		/// Called before executing any systems of the graph. Can be used to initialize data (synchronous).
		/// </summary>
		/// <param name="graph">The graph executing the system.</param>
		virtual void PreExecute(TaskGraph* graph);

		/// <summary>
		/// Executes the system logic and schedules the asynchronous work.
		/// </summary>
		/// <param name="graph">The graph executing the system.</param>
		virtual void Execute(TaskGraph* graph);

		/// <summary>
		/// Called after executing all systems of the graph. Can be used to cleanup data (synchronous).
		/// </summary>
		/// <param name="graph">The graph executing the system.</param>
		virtual void PostExecute(TaskGraph* graph);
	};

	/// <summary>
	/// Graph-based asynchronous tasks scheduler for high-performance computing and processing.
	/// </summary>
	class SE_API_RUNTIME TaskGraph /*: public ScriptingObject*/
	{
	private:
		List<TaskGraphSystem*, InlinedAllocation<64>> _systems;
		List<TaskGraphSystem*, InlinedAllocation<64>> _remaining;
		List<TaskGraphSystem*, InlinedAllocation<64>> _queue;
		List<int64, InlinedAllocation<64>> _labels;
		TaskGraphSystem* _currentSystem = nullptr;

	public:
		/// <summary>
		/// Gets the list of systems.
		/// </summary>
		const List<TaskGraphSystem*, InlinedAllocation<64>>& GetSystems() const;

		/// <summary>
		/// Adds the system to the graph for the execution.
		/// </summary>
		/// <param name="system">The system to add.</param>
		void AddSystem(TaskGraphSystem* system);

		/// <summary>
		/// Removes the system from the graph.
		/// </summary>
		/// <param name="system">The system to add.</param>
		void RemoveSystem(TaskGraphSystem* system);

		/// <summary>
		/// Schedules the asynchronous systems execution including ordering and dependencies handling.
		/// </summary>
		void Execute();

		/// <summary>
		/// Dispatches the job for the execution.
		/// </summary>
		/// <remarks>Call only from system's Execute method to properly schedule job.</remarks>
		/// <param name="job">The job. Argument is an index of the job execution.</param>
		/// <param name="jobCount">The job executions count.</param>
		void DispatchJob(const Function<void(int32)>& job, int32 jobCount = 1);
	};
}
