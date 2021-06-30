/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Common.hpp"
#include "BoundingIntervalHierarchyWrapper.h"
#include "Util.hpp"
#include "Logging/Logger.hpp"
#include "Errors.h"

#include <G3D/Vector3.h>
#include <G3D/Ray.h>
#include <G3D/AABox.h>
#include <G3D/Table.h>
#include <G3D/Array.h>
#include <G3D/Set.h>
#include <G3D/CollisionDetection.h>

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <limits>
