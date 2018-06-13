//
// Header for standard system include files.
//

#pragma once

#define VERIFY(expression) if (!(expression)) {throw "";}

#include <sstream>

#include <collection.h>
#include <ppltasks.h>
#include <shared_mutex>
#include <mfidl.h>
#include <rpcndr.h>

#include "MFPropertyGuids.h"
#include "App.xaml.h"