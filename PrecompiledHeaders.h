#pragma once

// undefined CommonLibSSE dependency
namespace RE {
	class QueuedFile {
	public:
		int IncRef() {
			return 1;
		}
		int DecRef() {
			return 1;
		}
	};
}

#include "RecursiveLock.h"

#include "CommonLibSSE/include/ForceInclude.h"

#include "CommonLibSSE/include/SKSE/API.h"
#include "CommonLibSSE/include/SKSE/Interfaces.h"
#include "CommonLibSSE/include/SKSE/Logger.h"
#include "CommonLibSSE/include/SKSE/RegistrationSet.h"

#include "CommonLibSSE/include/RE/Skyrim.h"

#include "version.h"
#include "ObjectType.h"
#include "iniSettings.h"
#include "IHasValueWeight.h"
#include "FormHelper.h"
#include "utils.h"
