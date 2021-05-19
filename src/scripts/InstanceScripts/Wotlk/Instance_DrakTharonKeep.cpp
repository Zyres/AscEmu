/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

#include "Setup.h"
#include "Instance_DrakTharonKeep.h"

class DrakTharonKeepInstanceScript : public InstanceScript
{
public:

    explicit DrakTharonKeepInstanceScript(MapMgr* pMapMgr) : InstanceScript(pMapMgr)
    {
    }

    static InstanceScript* Create(MapMgr* pMapMgr) { return new DrakTharonKeepInstanceScript(pMapMgr); }
};

// \todo Whole corpses/consume thingo is wrong

const uint32_t INVASION_INTERVAL = 20000;
const uint32_t INVADERS_PER_INVASION = 1;
//two mobs per 10s

class TrollgoreAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TrollgoreAI)
    explicit TrollgoreAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        invastion_timer = 0;

        if (_isHeroic())
        {
            addAISpell(49639, 20.0f, TARGET_ATTACKING, 0, 1);  //crush
            addAISpell(59805, 100.0f, TARGET_SELF, 0, 100);    //Consume
        }
        else
        {
            addAISpell(49639, 20.0f, TARGET_ATTACKING, 0, 1);  //crush
            addAISpell(49381, 100.0f, TARGET_SELF, 0, 100);    //Consume
        }

        addAISpell(49637, 50.0f, TARGET_ATTACKING, 0, 8);      //Infected_Wound
    }

    void OnCombatStart(Unit* /*mTarget*/) override
    {
        invastion_timer = _addTimer(INVASION_INTERVAL);
    }

    void OnCombatStop(Unit* /*mTarget*/) override
    {
        _removeTimer(invastion_timer);
    }

    void AIUpdate() override
    {
        if (_isTimerFinished(invastion_timer))
        {
            _resetTimer(invastion_timer, INVASION_INTERVAL);

            //spawn invaders ;)
            for (uint8_t i = 0; i < INVADERS_PER_INVASION; i++)
            {
                CreatureProperties const* cp = sMySQLStore.getCreatureProperties(CN_DRAKKARI_INVADER);
                if (cp != nullptr)
                {
                    Creature* c = getCreature()->GetMapMgr()->CreateCreature(CN_DRAKKARI_INVADER);
                    if (c)
                    {
                        //position is guessed
                        c->Load(cp, -259.532f, -618.976f, 26.669f, 0.0f);
                        c->PushToWorld(getCreature()->GetMapMgr());
                        //path finding would be usefull :)
                        //c->GetAIInterface()->SetRun();
                        c->GetAIInterface()->moveTo(getCreature()->GetPositionX(), getCreature()->GetPositionY(), getCreature()->GetPositionZ());
                    }
                }
            }
        }
    }

protected:

    uint32_t invastion_timer;
};

/*
 Novos the Summoner
 \todo
 - Crystal should be actived/deactived instead of created/deleted, minor
 - Create waypoints for summons, we need them coz Core doesn't not have path finding
 */

// NovosTheSummonerAI
const uint32_t INVADE_INTERVAL = 30000; //4 mobs per 30s
const uint32_t INVADERS_COUNT = 3;
const uint32_t HANDLER_INTERVAL = 60000; //one handler per 60s
const uint32_t ELITE_CHANCE = 20; //how much chance for elite we've got each invasion?

class NovosTheSummonerAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(NovosTheSummonerAI)
    explicit NovosTheSummonerAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        invasion_timer = 0;
        handler_timer = 0;

        if (_isHeroic())
        {
            addAISpell(59909, 70.0f, TARGET_ATTACKING, 0, 4);        //ArcaneBlast
            addAISpell(59854, 50.0f, TARGET_RANDOM_SINGLE, 0, 6);    //Blizzard
            addAISpell(59855, 40.0f, TARGET_RANDOM_SINGLE, 0, 2);    //Frostbolt
            addAISpell(59856, 10.0f, TARGET_RANDOM_SINGLE, 0, 5);    //WrathOfMisery
        }
        else
        {
            addAISpell(49198, 70.0f, TARGET_ATTACKING, 0, 4);        //ArcaneBlast
            addAISpell(49034, 50.0f, TARGET_RANDOM_SINGLE, 0, 6);    //Blizzard
            addAISpell(49037, 40.0f, TARGET_RANDOM_SINGLE, 0, 2);    //Frostbolt
            addAISpell(50089, 10.0f, TARGET_RANDOM_SINGLE, 0, 5);    //WrathOfMisery
        }

        addEmoteForEvent(Event_OnCombatStart, SAY_NOVOS_SUMMONER_01);
        addEmoteForEvent(Event_OnTargetDied, SAY_NOVOS_SUMMONER_02);
        addEmoteForEvent(Event_OnDied, SAY_NOVOS_SUMMONER_03);
    }

    void OnCombatStart(Unit* /*mTarget*/) override
    {
        getCreature()->castSpell(getCreature(), 47346, false);
        //spawn 4 Ritual Crystal
        for (uint8_t i = 0; i < 4; i++)
            SpawnCrystal(i);

        handler_timer = Util::getMSTime() + HANDLER_INTERVAL;
        _setMeleeDisabled(true);

        for (uint8_t i = 0; i < 7; i++)
            getCreature()->SchoolImmunityList[i] = 1;
        sendDBChatMessage(SAY_NOVOS_SUMMONER_05);
        sendDBChatMessage(SAY_NOVOS_SUMMONER_06);
    }

    void OnLoad() override
    {
        //root him and disable melee for him ;)
        _setMeleeDisabled(true);
        getCreature()->setMoveRoot(true);
    }

    void OnCombatStop(Unit* /*mTarget*/) override
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            if (getCreature()->m_ObjectSlots[i])
            {
                GameObject* Crystal = getCreature()->GetMapMgr()->GetGameObject(getCreature()->m_ObjectSlots[i]);
                if (Crystal && Crystal->IsInWorld())
                    Crystal->Despawn(0, 0);
            }
        }
        getCreature()->setMoveRoot(true);
        getCreature()->interruptSpell();
        getCreature()->RemoveAllAuras();
    }

    void AIUpdate() override
    {
        if (getScriptPhase() == 1)
        {
            if (invasion_timer <Util::getMSTime())
            {
                invasion_timer = Util::getMSTime() + INVADE_INTERVAL;
                SpawnInvader(0);
            }

            if (handler_timer <Util::getMSTime())
            {
                handler_timer = Util::getMSTime() + HANDLER_INTERVAL;
                SpawnInvader(1);
            }

            bool new_phase = true;
            for (uint8_t i = 0; i < 4; i++)
            {
                if (getCreature()->m_ObjectSlots[i])
                {
                    GameObject* Crystal = getCreature()->GetMapMgr()->GetGameObject(getCreature()->m_ObjectSlots[i]);
                    if (Crystal && Crystal->IsInWorld())
                        new_phase = false;
                }
            }

            if (new_phase)
            {
                getCreature()->interruptSpell();
                getCreature()->RemoveAllAuras();
                getCreature()->setMoveRoot(false);
                _setMeleeDisabled(false);
                setScriptPhase(2);

                for (uint8_t i = 0; i < 7; i++)
                    getCreature()->SchoolImmunityList[i] = 0;
            }
        }
    }

    void OnScriptPhaseChange(uint32_t scriptPhase) override
    {
        if (scriptPhase == 2)
            getCreature()->setMoveRoot(false);
    }
    
    Player* GetRandomPlayerTarget()
    {
        std::vector< uint32_t > possible_targets;
        for (const auto& iter : getCreature()->getInRangePlayersSet())
        {
            if (iter && static_cast<Player*>(iter)->isAlive())
                possible_targets.push_back(static_cast<uint32_t>(iter->getGuid()));
        }
        if (possible_targets.size() > 0)
        {
            uint32_t random_player = possible_targets[Util::checkChance(uint32_t(possible_targets.size() - 1))];
            return getCreature()->GetMapMgr()->GetPlayer(random_player);
        }
        return nullptr;
    }

    // scriptPhase
    //type: 1 - normal, 0 - handler
    void SpawnInvader(uint32_t type)
    {
        sendDBChatMessage(SAY_NOVOS_SUMMONER_04);
        //x                y                z
        //-379.101227f    -824.835449f    60.0f
        uint32_t mob_entry = 0;
        if (type)
        {
            mob_entry = 26627;
            CreatureProperties const* cp = sMySQLStore.getCreatureProperties(mob_entry);
            if (cp != nullptr)
            {
                Creature* c = getCreature()->GetMapMgr()->CreateCreature(mob_entry);
                if (c)
                {
                    //position is guessed
                    c->Load(cp, -379.101227f, -824.835449f, 60.0f, 0.0f);
                    c->PushToWorld(getCreature()->GetMapMgr());
                    c->setSummonedByGuid(getCreature()->getGuid());
                    //path finding would be usefull :)
                    Player* p_target = GetRandomPlayerTarget();
                    if (p_target)
                    {
                        c->GetAIInterface()->moveTo(p_target->GetPositionX(), p_target->GetPositionY(), p_target->GetPositionZ(), 0.0f, true);
                    }
                }
            }
        }
        else
        {
            for (uint8_t i = 0; i < INVADERS_COUNT; i++)
            {
                mob_entry = 0;
                if (Util::checkChance(ELITE_CHANCE))
                    mob_entry = 27597;
                else
                {
                    uint32_t mobs[2] = { 27598, 27600 };
                    mob_entry = mobs[Util::getRandomUInt(1)];
                }
                CreatureProperties const* cp = sMySQLStore.getCreatureProperties(mob_entry);
                if (cp != nullptr)
                {
                    Creature* c = getCreature()->GetMapMgr()->CreateCreature(mob_entry);
                    if (c)
                    {
                        //position is guessed
                        c->Load(cp, -379.101227f, -824.835449f, 60.0f, 0.0f);
                        c->PushToWorld(getCreature()->GetMapMgr());
                        //path finding would be usefull :)
                        Player* p_target = GetRandomPlayerTarget();
                        if (p_target)
                        {
                            c->GetAIInterface()->moveTo(p_target->GetPositionX(), p_target->GetPositionY(), p_target->GetPositionZ(), 0.0f, true);
                        }
                    }
                }
            }
        }
    }
    void SpawnCrystal(uint32_t id)
    {
        uint32_t entry = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, o = 0.0f;
        switch (id)
        {
            case 0:
            {
                entry = GO_RITUAL_CRYSTAL_ENTRY_1;
                x = -392.416f;
                y = -724.865f;
                z = 29.4156f;
                o = M_PI_FLOAT;
            }
            break;
            case 1:
            {
                entry = GO_RITUAL_CRYSTAL_ENTRY_2;
                x = -365.279f;
                y = -751.087f;
                z = 29.4156f;
                o = M_PI_FLOAT;
            }
            break;
            case 2:
            {
                entry = GO_RITUAL_CRYSTAL_ENTRY_3;
                x = -365.41f;
                y = -724.865f;
                z = 29.4156f;
                o = M_PI_FLOAT;
            }
            break;
            case 3:
            {
                entry = GO_RITUAL_CRYSTAL_ENTRY_4;
                x = -392.286f;
                y = -751.087f;
                z = 29.4156f;
                o = M_PI_FLOAT;
            }
            break;
        }
        GameObject* go = getCreature()->GetMapMgr()->CreateGameObject(entry);
        go->CreateFromProto(entry, getCreature()->GetMapMgr()->GetMapId(), x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f);
        go->PushToWorld(getCreature()->GetMapMgr());
        getCreature()->m_ObjectSlots[id] = go->GetUIdFromGUID();
    }

protected:

    uint32_t invasion_timer;
    uint32_t handler_timer;
};


class CrystalHandlerAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(CrystalHandlerAI)
    explicit CrystalHandlerAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        if (_isHeroic())
            addAISpell(59004, 50.0f, TARGET_ATTACKING, 0, 4);    //FlashofDarkness
        else
            addAISpell(49668, 50.0f, TARGET_ATTACKING, 0, 4);    //FlashofDarkness
    }

    void OnDied(Unit* /*mKiller*/) override
    {
        Unit* Novos = getCreature()->GetMapMgr()->GetUnit(getCreature()->getSummonedByGuid());
        if (Novos)
        {
            for (uint8_t i = 0; i < 4; i++)
            {
                if (Novos->m_ObjectSlots[i])
                {
                    GameObject* Crystal = Novos->GetMapMgr()->GetGameObject(Novos->m_ObjectSlots[i]);
                    if (Crystal && Crystal->IsInWorld())
                    {
                        Crystal->Despawn(0, 0);
                        return;
                    }
                }
            }
        }
    }
};


// \todo King Dred Call nearby friends
class KingDreadAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(KingDreadAI)
    explicit KingDreadAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        if (_isHeroic())
        {
            addAISpell(59422, 80.0f, TARGET_ATTACKING, 0, 3);  //GrievousBite
            addAISpell(48873, 40.0f, TARGET_ATTACKING, 0, 2);  //ManglingSlash
        }
        else
        {
            addAISpell(48849, 80.0f, TARGET_ATTACKING, 0, 3);  //GrievousBite
            addAISpell(48873, 40.0f, TARGET_ATTACKING, 0, 2);  //ManglingSlas
        }

        addAISpell(22686, 30.0f, TARGET_ATTACKING, 0, 5); //BellowingRoar
        addAISpell(48878, 40.0f, TARGET_ATTACKING, 0, 5); //PiercingSlash
    }
};

/*
 The Prophet Tharon'ja
 \todo
 - Skeleton/Normal phases should be based on boss health instead of time
 - Figure out why players are not always changed to skeletons while chaning phases
 */

const uint32_t WINDSERPENT_PHASE_INTERVAL = 60000; //change phase each 60s
const uint32_t WINDSERPENT_PHASE_LENGTH = 30000; //30s
const uint32_t PHASES_COUNT = 2;

class TheProphetTaronjaAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TheProphetTaronjaAI)
    explicit TheProphetTaronjaAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        phase_timer = 0;
        phase_length = 0;

        if (_isHeroic())
        {
            addAISpell(59972, 80.0f, TARGET_RANDOM_SINGLE, 0, 7);   //CurseOfLife
            addAISpell(59963, 60.0f, TARGET_RANDOM_SINGLE, 0, 4);   //LightningBreath
            addAISpell(59969, 30.0f, TARGET_RANDOM_SINGLE, 0, 6);   //PoisonCloud
            addAISpell(59971, 70.0f, TARGET_RANDOM_SINGLE, 0, 10);   //PoisonCloud
            addAISpell(59973, 60.0f, TARGET_RANDOM_SINGLE, 0, 5);   //ShadowVolley
            auto ShadowVolley = addAISpell(59965, 50.0f, TARGET_RANDOM_SINGLE, 0, 3);
            ShadowVolley->setAvailableForScriptPhase({ 2 });
        }
        else
        {
            addAISpell(49527, 80.0f, TARGET_RANDOM_SINGLE, 0, 7);   //CurseOfLife
            addAISpell(49537, 60.0f, TARGET_RANDOM_SINGLE, 0, 4);   //LightningBreath
            addAISpell(49548, 30.0f, TARGET_RANDOM_SINGLE, 0, 6);   //PoisonCloud
            addAISpell(49518, 70.0f, TARGET_RANDOM_SINGLE, 0, 10);   //PoisonCloud
            addAISpell(49528, 60.0f, TARGET_RANDOM_SINGLE, 0, 5);   //ShadowVolley
            auto ShadowVolley = addAISpell(49544, 50.0f, TARGET_RANDOM_SINGLE, 0, 3);
            ShadowVolley->setAvailableForScriptPhase({ 2 });
        }
    }

    void OnCombatStart(Unit* /*mTarget*/) override
    {
        phase_length = 0;
        phase_timer = Util::getMSTime() + WINDSERPENT_PHASE_INTERVAL;
    }

    void OnCombatStop(Unit* /*mTarget*/) override
    {
        phase_timer = 0;
        phase_length = 0;
        getCreature()->setDisplayId(getCreature()->getNativeDisplayId());
    }

    void OnDamageTaken(Unit* /*mAttacker*/, uint32_t /*fAmount*/) override
    {
        if (getCreature()->getHealthPct() < 2 && getScriptPhase() == 2)
        {
            phase_timer = Util::getMSTime() + WINDSERPENT_PHASE_INTERVAL;
            getCreature()->setDisplayId(getCreature()->getNativeDisplayId());
            getCreature()->castSpell(getCreature(), 53463, false);
        }
    }

    void AIUpdate() override
    {
        if (getScriptPhase() == 1 && phase_timer < Util::getMSTime())
        {
            setScriptPhase(2);
            phase_length = Util::getMSTime() + WINDSERPENT_PHASE_LENGTH;
            getCreature()->setDisplayId(27073);
            getCreature()->RemoveAllAuras();
            getCreature()->castSpell(getCreature(), 49356, false);
        }
        if (getScriptPhase() == 2 && phase_length <Util::getMSTime())
        {
            setScriptPhase(1);
            phase_timer = Util::getMSTime() + WINDSERPENT_PHASE_INTERVAL;
            getCreature()->setDisplayId(getCreature()->getNativeDisplayId());
            getCreature()->RemoveAllAuras();
            getCreature()->castSpell(getCreature(), 53463, false);
        }
    }

    uint32_t phase_timer;
    uint32_t phase_length;
};

void SetupDrakTharonKeep(ScriptMgr* mgr)
{
    mgr->register_instance_script(MAP_DRAK_THARON_KEEP, &DrakTharonKeepInstanceScript::Create);

    //Bosses
    mgr->register_creature_script(CN_TROLLGORE, &TrollgoreAI::Create);
    mgr->register_creature_script(CN_NOVOS_THE_SUMMONER, &NovosTheSummonerAI::Create);
    mgr->register_creature_script(CN_CRYSTAL_HANDLER, &CrystalHandlerAI::Create);
    mgr->register_creature_script(CN_KING_DRED, &KingDreadAI::Create);
    mgr->register_creature_script(CN_THE_PROPHET_THARONJA, &TheProphetTaronjaAI::Create);
}
