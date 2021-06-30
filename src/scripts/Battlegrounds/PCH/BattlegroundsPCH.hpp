/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Setup.h"
#include "Server/Script/ScriptSetup.h"
#include "Server/Script/ScriptMgr.h"

#include "Management/Arenas.h"
#include "Management/Battleground/Battleground.h"
#include "Server/Master.h"
#include "Objects/GameObject.h"
#include "Storage/MySQLDataStore.hpp"
#include "Management/QuestLogEntry.hpp"
#include "Management/WorldStates.h"
#include "Server/MainServerDefines.h"
#include "Management/HonorHandler.h"
#include "Map/MapMgr.h"
#include "Spell/SpellMgr.hpp"
#include "Server/Packets/SmsgMessageChat.h"
#include "Chat/ChatDefines.hpp"
#include "Objects/ObjectMgr.h"
#include "Management/TransporterHandler.h"
#include "Objects/Transporter.h"
#include "Spell/SpellCastTargets.hpp"
