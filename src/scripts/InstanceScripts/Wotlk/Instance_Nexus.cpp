/*
Copyright (c) 2014-2021 AscEmu Team <http://www.ascemu.org>
This file is released under the MIT license. See README-MIT for more information.
*/

// \todo Last boss needs to be finished
#include "Setup.h"
#include "Instance_Nexus.h"

#include "Server/Script/CreatureAIScript.h"

class AnomalusAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(AnomalusAI)
    explicit AnomalusAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        mInstance = getInstanceScript();

        if (_isHeroic())
            addAISpell(SPARK_HC, 80.0f, TARGET_RANDOM_SINGLE, 0, 3);
        else
            addAISpell(SPARK, 80.0f, TARGET_RANDOM_SINGLE, 0, 3);

        mSummon = 0;
        mRift = false;
        mSummonTimer = 0;

        addEmoteForEvent(Event_OnCombatStart, 4317);    // Chaos beckons.
        addEmoteForEvent(Event_OnDied, 4318);           // Of course.
    }

    void OnCombatStart(Unit* /*mTarget*/) override
    {
        mSummon = 0;
        mRift = false;
        mSummonTimer = _addTimer(_isHeroic() ? 14000 : 18000);   // check heroic
    }

    void AIUpdate() override
    {
        if ((_getHealthPercent() <= 50 && mSummon == 0))
            mSummon += 1;

        if (mSummon == 1)
            ChargeRift();

        if (_isTimerFinished(mSummonTimer) && mRift == false)
        {
            SummonRift(false);
            _resetTimer(mSummonTimer, _isHeroic() ? 14000 : 18000);
        }

        if (mRift == true && (getLinkedCreatureAIScript() == NULL || !getLinkedCreatureAIScript()->isAlive()))
        {
            _removeAura(47748);
            mRift = false;
        }
    }

    void SummonRift(bool bToCharge)
    {
        if (!bToCharge)
            sendDBChatMessage(4319);     // Reality... unwoven.

        sendAnnouncement("Anomalus opens a Chaotic Rift!");
        //we are linked with CN_CHAOTIC_RIFT.
        CreatureAIScript* chaoticRift = spawnCreatureAndGetAIScript(CN_CHAOTIC_RIFT, getCreature()->GetPositionX() + 13.5f, getCreature()->GetPositionY(), getCreature()->GetPositionZ(), getCreature()->GetOrientation());
        if (chaoticRift != nullptr)
        {
            setLinkedCreatureAIScript(chaoticRift);
            chaoticRift->setLinkedCreatureAIScript(this);
        }
    }

    void ChargeRift()
    {
        SummonRift(true);
        sendDBChatMessage(4320);     // Indestructible.
        sendAnnouncement("Anomalus shields himself and diverts his power to the rifts!");
        _applyAura(47748);   // me immune
        setRooted(true);

        mRift = true;
        mSummon += 1;
    }

    void OnDied(Unit* /*pKiller*/) override
    {
        if (mInstance)
        {
            GameObjectSet sphereSet = mInstance->getGameObjectsSetForEntry(ANOMALUS_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }
    }

private:

    int32_t mSummonTimer;
    uint8_t mSummon;
    bool mRift;
    InstanceScript* mInstance;
};

class ChaoticRiftAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(ChaoticRiftAI)
    explicit ChaoticRiftAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        getCreature()->GetAIInterface()->setAllowedToEnterCombat(false);
        auto spell_mana_wrath = sSpellMgr.getSpellInfo(SUMMON_MANA_WRAITH);
        if (spell_mana_wrath != nullptr)
            addAISpell(SUMMON_MANA_WRAITH, 30.0f, TARGET_SELF, 0, spell_mana_wrath->getRecoveryTime());

        auto spell_energy_burst = sSpellMgr.getSpellInfo(CHAOTIC_ENERGY_BURST);
        if (spell_energy_burst != nullptr)
            addAISpell(CHAOTIC_ENERGY_BURST, 30.0f, TARGET_RANDOM_SINGLE, 0, spell_energy_burst->getRecoveryTime());
    }

    void OnLoad() override
    {
        _applyAura(CHAOTIC_RIFT_AURA);
        despawn(40000, 0);
    }

    void OnDied(Unit* /*mKiller*/) override
    {
        despawn(2000, 0);
    }

    void OnCombatStop(Unit* /*pTarget*/) override
    {
        despawn(2000, 0);
    }
};

class CraziedManaWrathAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(CraziedManaWrathAI)
    explicit CraziedManaWrathAI(Creature* pCreature) : CreatureAIScript(pCreature) {}

    void OnCombatStop(Unit* /*pTarget*/) override
    {
        despawn(2000, 0);
    }

    void OnDied(Unit* /*mKiller*/) override
    {
        despawn(2000, 0);
    }
};

static LocationVector FormSpawns[] =
{
    { 494.726990f, 89.128799f, -15.941300f, 6.021390f },
    { 495.006012f, 89.328102f, -16.124609f, 0.027486f },
    { 504.798431f, 102.248375f, -16.124609f, 4.629921f }
};

class TelestraBossAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TelestraBossAI)
    explicit TelestraBossAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        mInstance = getInstanceScript();

        if (_isHeroic())
        {
            addAISpell(ICE_NOVA_HC, 25.0f, TARGET_SELF, 2, 15);
            addAISpell(FIREBOMB_HC, 35.0f, TARGET_RANDOM_SINGLE, 2, 5);
            addAISpell(GRAVITY_WELL, 15.0f, TARGET_SELF, 1, 20);
        }
        else
        {
            addAISpell(ICE_NOVA, 25.0f, TARGET_SELF, 2, 15);
            addAISpell(FIREBOMB, 35.0f, TARGET_RANDOM_SINGLE, 2, 5);
            addAISpell(GRAVITY_WELL, 15.0f, TARGET_SELF, 1, 20);
        }

        mAddCount = 0;
        mPhaseRepeat = 2;

        mAddArray[0] = mAddArray[1] = mAddArray[2] = NULL;

        addEmoteForEvent(Event_OnCombatStart, 4326);    // You know what they say about curiosity....
        addEmoteForEvent(Event_OnTargetDied, 4327);     // Death becomes you.
        addEmoteForEvent(Event_OnDied, 4328);           // Damn the... luck.
    }

    void AIUpdate() override
    {
        if (isScriptPhase(1) && _getHealthPercent() <= (mPhaseRepeat * 25))
        {
            switch (Util::getRandomUInt(1))
            {
                case 0:
                    sendDBChatMessage(4330);      // There's plenty of me to go around.
                    break;
                case 1:
                    sendDBChatMessage(4331);      // I'll give you more than you can handle!
                    break;
            }

            setScriptPhase(2);
            setRooted(true);
            _setRangedDisabled(true);
            _setCastDisabled(true);
            _setTargetingDisabled(true);
            _applyAura(60191);

            for (uint8_t i = 0; i < 3; ++i)
            {
                mAddArray[i] = spawnCreature(CN_TELESTRA_FIRE + i, FormSpawns[i].x, FormSpawns[i].y, FormSpawns[i].z, FormSpawns[i].o);
                if (mAddArray[i] != NULL)
                    ++mAddCount;
            }
        }

        if (isScriptPhase(2))
        {
            for (uint8_t i = 0; i < 3; ++i)
            {
                if (mAddArray[i] != NULL)
                {
                    mAddArray[i]->Despawn(1000, 0);
                    mAddArray[i] = NULL;
                    --mAddCount;
                }
            }

            if (mAddCount != 0)
                return;

            sendChatMessage(CHAT_MSG_MONSTER_YELL, 13323, "Now to finish the job!");

            _removeAura(60191);
            setRooted(false);
            mPhaseRepeat = 1;
            setScriptPhase(_isHeroic() ? 1 : 3);   //3 disables p2
        }
    }

    void OnCombatStop(Unit* /*pTarget*/) override
    {
        for (uint8_t i = 0; i < 3; ++i)
        {
            if (mAddArray[i] != NULL)
            {
                mAddArray[i]->Despawn(1000, 0);
                mAddArray[i] = NULL;
            }
        }
    }

    void OnDied(Unit* /*pKiller*/) override
    {
        for (uint8_t i = 0; i < 3; ++i)
        {
            if (mAddArray[i] != NULL)
            {
                mAddArray[i]->Despawn(1000, 0);
                mAddArray[i] = NULL;
            }
        }

        if (mInstance)
        {
            mInstance->setData(NEXUS_TELESTRA, Finished);
            GameObjectSet sphereSet = mInstance->getGameObjectsSetForEntry(TELESTRA_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }
    }

private:

    Creature* mAddArray[3];

    int32_t mPhaseRepeat;
    int32_t mAddCount;

    InstanceScript* mInstance;
};

class TelestraFireAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TelestraFireAI)
    explicit TelestraFireAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        if (_isHeroic())
        {
            addAISpell(FIRE_BLAST_HC, 30.0f, TARGET_RANDOM_SINGLE, 0, 14);
            addAISpell(SCORCH_HC, 100.0f, TARGET_ATTACKING, 1, 3);
        }
        else
        {
            addAISpell(FIRE_BLAST, 30.0f, TARGET_RANDOM_SINGLE, 0, 14);
            addAISpell(SCORCH, 100.0f, TARGET_ATTACKING, 1, 3);
        }
    }
};

class TelestraFrostAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TelestraFrostAI)
    explicit TelestraFrostAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        if (_isHeroic())
        {
            addAISpell(BLIZZARD_HC, 20.0f, TARGET_RANDOM_DESTINATION, 0, 20);
            addAISpell(ICE_BARB_HC, 25.0f, TARGET_RANDOM_SINGLE, 1, 6);
        }
        else
        {
            addAISpell(BLIZZARD, 20.0f, TARGET_RANDOM_DESTINATION, 0, 20);
            addAISpell(ICE_BARB, 25.0f, TARGET_RANDOM_SINGLE, 1, 6);
        }
    }
};

class TelestraArcaneAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(TelestraArcaneAI)
    explicit TelestraArcaneAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        addAISpell(TIME_STOP, 30.0f, TARGET_SELF, 2, 30);
        addAISpell(CRITTER, 25.0f, TARGET_RANDOM_SINGLE, 0, 20);
    }
};

class OrmorokAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(OrmorokAI)
    explicit OrmorokAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        mInstance = getInstanceScript();

        if (_isHeroic())
            addAISpell(TRAMPLE_H, 30.0f, TARGET_ATTACKING, 0, 9);
        else
            addAISpell(TRAMPLE, 30.0f, TARGET_ATTACKING, 0, 9);

        addAISpell(SPELL_REFLECTION, 35.0f, TARGET_SELF, 2, 15);

        auto crystalSpikes = addAISpell(CRYSTAL_SPIKES, 25.0f, TARGET_SELF, 0, 12);
        crystalSpikes->addEmote("Bleed!", CHAT_MSG_MONSTER_YELL, 13332);

        mEnraged = false;

        addEmoteForEvent(Event_OnCombatStart, 1943);    // Noo!
        addEmoteForEvent(Event_OnDied, 1944);           // Aaggh!
    }

    void OnCombatStart(Unit* /*pTarget*/) override
    {
        mEnraged = false;
    }

    void AIUpdate() override
    {
        if (_getHealthPercent() <= 25 && mEnraged == false)
        {
            _applyAura(FRENZY);
            sendAnnouncement("Ormorok the Tree-Shaper goes into a frenzy!");
            mEnraged = true;
        }
    }

    void OnDied(Unit* /*pKiller*/) override
    {
        if (mInstance)
        {
            GameObjectSet sphereSet = mInstance->getGameObjectsSetForEntry(ORMOROK_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }
    }

private:

    bool mEnraged;
    InstanceScript* mInstance;
};

class CrystalSpikeAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(CrystalSpikeAI)
    explicit CrystalSpikeAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        m_part = 0;
    }

    void OnLoad() override
    {
        setCanEnterCombat(false);
        setRooted(true);
        getCreature()->addUnitFlags(UNIT_FLAG_NOT_SELECTABLE);

        despawn(4500, 0);
        RegisterAIUpdateEvent(500);
    }

    void AIUpdate() override
    {
        m_part += 1;

        if (m_part == 1)
        {
            getCreature()->castSpell(getCreature(), SPELL_CRYSTAL_SPIKE_VISUAL, true);
        }
        else if (m_part == 5)
        {
            if (_isHeroic())
            {
                getCreature()->castSpell(getCreature(), sSpellMgr.getSpellInfo(SPELL_CRYSTAL_SPIKE_H), true);
            }
            else
            {
                getCreature()->castSpell(getCreature(), sSpellMgr.getSpellInfo(SPELL_CRYSTAL_SPIKE), true);
            }
        }
    }

private:

    int m_part;
};

// \todo currently unfinished
class KeristraszaAI : public CreatureAIScript
{
    ADD_CREATURE_FACTORY_FUNCTION(KeristraszaAI)
    explicit KeristraszaAI(Creature* pCreature) : CreatureAIScript(pCreature)
    {
        if (_isHeroic())
            addAISpell(CRYSTALFIRE_BREATH_HC, 30.0f, TARGET_SELF, 1, 14);
        else
            addAISpell(CRYSTALFIRE_BREATH, 30.0f, TARGET_SELF, 1, 14);

        addAISpell(CRYSTAL_CHAINS, 30.0f, TARGET_RANDOM_SINGLE, 0, 12);
        addAISpell(TAIL_SWEEP, 40.0f, TARGET_SELF, 0, 8);

        auto crystalize = addAISpell(CRYSTALLIZE, 25.0f, TARGET_SELF, 0, 22);
        crystalize->addEmote("Stay. Enjoy your final moments.", CHAT_MSG_MONSTER_YELL, 13451);

        mEnraged = false;
        setCanEnterCombat(false);

        addEmoteForEvent(Event_OnCombatStart, 4321);    // Preserve? Why? There's no truth in it. No no no... only in the taking! I see that now!
        addEmoteForEvent(Event_OnTargetDied, 4322);     // Now we've come to the truth! )
        addEmoteForEvent(Event_OnDied, 4324);           // Dragonqueen... Life-Binder... preserve... me.
    }

    void OnLoad() override
    {
        _applyAura(47543);   // frozen prison
    }

    void AIUpdate() override
    {
        if (mEnraged == false && _getHealthPercent() <= 25)
        {
            _applyAura(ENRAGE);
            mEnraged = true;
        }
    }

    // called from instance script o.O
    void Release()
    {
        setCanEnterCombat(true);
        _removeAura(47543);
        _applyAura(INTENSE_COLD);
    }

private:

    bool mEnraged;
};

// Nexus Instance script
class NexusScript : public InstanceScript
{
public:

    uint32_t mAnomalusGUID;
    uint32_t mTelestraGUID;
    uint32_t mOrmorokGUID;
    uint32_t mKeristraszaGUID;

    uint8_t mCSCount;

    uint32_t m_uiEncounters[NEXUS_END];

    explicit NexusScript(MapMgr* pMapMgr) : InstanceScript(pMapMgr)
    {
        mAnomalusGUID = 0;
        mTelestraGUID = 0;
        mOrmorokGUID = 0;
        mKeristraszaGUID = 0;

        mCSCount = 0;

        for (uint8_t i = 0; i < NEXUS_END; ++i)
            m_uiEncounters[i] = NotStarted;

        PrepareGameObjectsForState();
    }

    static InstanceScript* Create(MapMgr* pMapMgr) { return new NexusScript(pMapMgr); }

    void PrepareGameObjectsForState()
    {
        if (getData(NEXUS_ANOMALUS) == Finished)
        {
            GameObjectSet sphereSet = getGameObjectsSetForEntry(ANOMALUS_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }

        if (getData(NEXUS_TELESTRA) == Finished)
        {
            GameObjectSet sphereSet = getGameObjectsSetForEntry(TELESTRA_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }

        if (getData(NEXUS_ORMOROK) == Finished)
        {
            GameObjectSet sphereSet = getGameObjectsSetForEntry(ORMOROK_CS);
            for (auto goSphere : sphereSet)
            {
                if (goSphere != nullptr)
                    goSphere->removeFlags(GO_FLAG_NOT_SELECTABLE);
            }
        }
    }

    void OnCreaturePushToWorld(Creature* pCreature) override
    {
        switch (pCreature->getEntry())
        {
        case CN_KERISTRASZA:
            mKeristraszaGUID = pCreature->getGuidLow();
            break;
        case CN_ANOMALUS:
            mAnomalusGUID = pCreature->getGuidLow();
            break;
        case CN_TELESTRA:
            mTelestraGUID = pCreature->getGuidLow();
            break;
        case CN_ORMOROK:
            mOrmorokGUID = pCreature->getGuidLow();
            break;
        }
    }

    void OnGameObjectPushToWorld(GameObject* pGameObject) override
    {
        switch (pGameObject->getEntry())
        {
        case ANOMALUS_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            break;
        case TELESTRA_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            break;
        case ORMOROK_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            break;
        }
    }

    void OnGameObjectActivate(GameObject* pGameObject, Player* /*pPlayer*/) override
    {
        switch (pGameObject->getEntry())
        {
        case ANOMALUS_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            ++mCSCount;
            break;
        case TELESTRA_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            ++mCSCount;
            break;
        case ORMOROK_CS:
            pGameObject->setFlags(GO_FLAG_NOT_SELECTABLE);
            ++mCSCount;
            break;
        default:
            return;
        }

        if (mCSCount == 3)   // release last boss
        {
            Creature* pKeristrasza = GetCreatureByGuid(mKeristraszaGUID);
            if (pKeristrasza == NULL)
                return;

            KeristraszaAI* pKeristraszaAI = static_cast<KeristraszaAI*>(pKeristrasza->GetScript());
            if (pKeristraszaAI == NULL)
                return;

            pKeristraszaAI->Release();
        }
    }

    void OnPlayerEnter(Player* player) override
    {
        if (!spawnsCreated())
        {
            // team spawns
            if (player->getTeam() == TEAM_ALLIANCE)
            {
                for (uint8_t i = 0; i < 18; i++)
                    spawnCreature(TrashHordeSpawns[i].entry, TrashHordeSpawns[i].x, TrashHordeSpawns[i].y, TrashHordeSpawns[i].z, TrashHordeSpawns[i].o, TrashHordeSpawns[i].faction);
            }
            else
            {
                for (uint8_t i = 0; i < 18; i++)
                    spawnCreature(TrashAllySpawns[i].entry, TrashAllySpawns[i].x, TrashAllySpawns[i].y, TrashAllySpawns[i].z, TrashAllySpawns[i].o, TrashAllySpawns[i].faction);
            }

            // difficulty spawns
            if (player->getDungeonDifficulty() == InstanceDifficulty::DUNGEON_NORMAL)
            {
                switch (player->getTeam())
                {
                case TEAM_ALLIANCE:
                    spawnCreature(CN_HORDE_COMMANDER, 425.39f, 185.82f, -35.01f, -1.57f, 14);
                    break;
                case TEAM_HORDE:
                    spawnCreature(CN_ALLIANCE_COMMANDER, 425.39f, 185.82f, -35.01f, -1.57f, 14);
                    break;
                }
            }
            else    // MODE_HEROIC
            {
                switch (player->getTeam())
                {
                case TEAM_ALLIANCE:
                    spawnCreature(H_CN_HORDE_COMMANDER, 425.39f, 185.82f, -35.01f, -1.57f, 14);
                    break;
                case TEAM_HORDE:
                    spawnCreature(H_CN_ALLIANCE_COMMANDER, 425.39f, 185.82f, -35.01f, -1.57f, 14);
                    break;
                }
            }

            setSpawnsCreated();
        }
    }
};

void SetupNexus(ScriptMgr* mgr)
{
    // Anomalus Encounter
    mgr->register_creature_script(CN_ANOMALUS, &AnomalusAI::Create);
    mgr->register_creature_script(CN_CHAOTIC_RIFT, &ChaoticRiftAI::Create);
    mgr->register_creature_script(CN_CRAZED_MANA_WRAITH, &CraziedManaWrathAI::Create);

    // Grand Magus Telestra Encounter
    mgr->register_creature_script(CN_TELESTRA, &TelestraBossAI::Create);
    mgr->register_creature_script(CN_TELESTRA_ARCANE, &TelestraArcaneAI::Create);
    mgr->register_creature_script(CN_TELESTRA_FROST, &TelestraFrostAI::Create);
    mgr->register_creature_script(CN_TELESTRA_FIRE, &TelestraFireAI::Create);

    // Ormorok the Tree-Shaper Encounter
    mgr->register_creature_script(CN_ORMOROK, &OrmorokAI::Create);
    mgr->register_creature_script(CN_CRYSTAL_SPIKE, &CrystalSpikeAI::Create);

    // Keristrasza Encounter
    mgr->register_creature_script(CN_KERISTRASZA, &KeristraszaAI::Create);

    mgr->register_instance_script(MAP_NEXUS, &NexusScript::Create);
}
