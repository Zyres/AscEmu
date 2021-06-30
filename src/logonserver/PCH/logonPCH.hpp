/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Common.hpp"
#include "Database/DatabaseCommon.hpp"
#include "Database/Database.h"
#include "Util.hpp"
#include "Logging/Logger.hpp"
#include "Log.hpp"
#include "Util/Strings.hpp"
#include "Threading/AEThread.h"
#include "ByteBuffer.h"
#include "Config/Config.h"
#include "Network/Network.h"
#include "WorldPacket.h"

#include "Auth/BigNumber.h"
#include "Auth/Sha1.h"
#include "Auth/MD5.h"
#include "Auth/WowCrypt.hpp"
#include "Auth/AuthSocket.h"
#include "Auth/AutoPatcher.h"

#include "Network/Network.h"
#include "Realm/RealmManager.hpp"
#include "Server/Master.hpp"
#include "Server/LogonServerDefines.hpp"

#include "LogonCommServer/LogonCommServer.h"
#include "Console/LogonConsole.h"

#include <openssl/md5.h>

#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <cstdint>
