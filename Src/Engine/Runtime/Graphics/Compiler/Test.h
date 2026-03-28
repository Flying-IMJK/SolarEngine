

#include "slang.h"
#include "slang-com-ptr.h"


void Compiler()
{
	Slang::ComPtr<slang::IGlobalSession> globalSession;
	slang::createGlobalSession(globalSession.writeRef());

	slang::SessionDesc sessionDesc;
	Slang::ComPtr<slang::ISession> session;
	globalSession->createSession(sessionDesc, session.writeRef());

	Slang::ComPtr<slang::IBlob> diagnostics;
	slang::IModule* module = session->loadModule("", diagnostics.writeRef());

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	module->findEntryPointByName("vertex", entryPoint.writeRef());

	slang::IComponentType* components[] = { module, entryPoint };
	Slang::ComPtr<slang::IComponentType> program;
	session->createCompositeComponentType(components, 2, program.writeRef());

	Slang::ComPtr<slang::IComponentType> linkedProgram;
	Slang::ComPtr<ISlangBlob> diagnosticBlob;
	program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());

	slang::ProgramLayout* layout = program->getLayout();
//	layout->findTypeByName();
//	entryPoint->
//	linkedProgram->getEntryPointCode()

}
