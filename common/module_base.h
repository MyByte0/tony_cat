#ifndef COMMON_MODULE_BASE_H_
#define COMMON_MODULE_BASE_H_

#include "core_define.h"

SER_NAME_SPACE_BEGIN

class ModuleManager;

class ModuleBase {
public:
	ModuleBase(ModuleManager* pModuleManager);
	virtual ~ModuleBase();

	virtual void BeforeInit() {}
	virtual void OnInit() {}
	virtual void AfterInit() {}

	virtual void BeforeStop() {}
	virtual void OnStop() {}
	virtual void AfterStop() {}

	virtual void BeforeUpdate() {}
	virtual void OnUpdate() {}

	virtual void OnConfigChange() {}
    virtual void AfterConfigChange() {}

protected:
	ModuleManager* m_pModuleManager;
};

SER_NAME_SPACE_END

#endif // COMMON_MODULE_BASE_H_
