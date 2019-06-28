#include "Enemy.h"

#include "Doom.h"
#include "MathUtils.h"
#include "Random.h"
#include <cstdlib>

//--------------------------------------------------------------------------------------------------
// Return true if the target mobj_t is in melee range
//--------------------------------------------------------------------------------------------------
static bool CheckMeleeRange(const mobj_t* const pActor) noexcept {
    const mobj_t* const pTarget = pActor->target;   // Get the mobj_t of the target

    if (!pTarget)   // No target?
        return false;
    
    if ((pActor->flags & MF_SEETARGET) == 0)    // Can't see target?
        return false;
    
    const Fixed dist = GetApproxDistance(pTarget->x - pActor->x, pTarget->y - pActor->y);
    return (dist < MELEERANGE);
}

//--------------------------------------------------------------------------------------------------
// Return true if the actor is in missile range to the target
//--------------------------------------------------------------------------------------------------
static uint32_t CheckMissileRange(mobj_t* actor) noexcept {
    if ((actor->flags & MF_SEETARGET) == 0)     // Are you seen?
        return false;                           // Nope, don't fire!
    
    // If the target just hit the enemy, fight back!
    if (actor->flags & MF_JUSTHIT) {
        actor->flags &= ~MF_JUSTHIT;    // Clear the 'just hit' flag
        return true;                    // Fire back
    }

    if (actor->reactiontime)    // Still waking up?
        return false;           // Don't attack yet
    
    const mobj_t* const pTarget = actor->target;
    Fixed dist = (GetApproxDistance(actor->x - pTarget->x, actor->y - pTarget->y) >> FRACBITS) - 64;

    if (!actor->InfoPtr->meleestate) {  // No hand-to-hand combat mode?
        dist -= 128;                    // No melee attack, so fire more often
    }

    if (actor->InfoPtr == &mobjinfo[MT_SKULL]) {    // Is it a skull?
        dist >>= 1;                                 // Halve the distance for attack
    }

    if (dist >= 201) {      // 200 units?
        dist = 200;         // Set the maximum
    }
    
    if (Random::nextU8() < dist) {      // Random chance for attack
        return false;                   // No attack
    }

    return true;    // Attack!
}

//--------------------------------------------------------------------------------------------------
// Move in the current direction.
// Returns false if the move is blocked.
// Called for ACTORS, not the player or missiles
//--------------------------------------------------------------------------------------------------
static constexpr Fixed MOVE_X_SPEEDS[8] = {
    FRACUNIT,
    47000,
    0,
    -47000,
    -FRACUNIT,
    -47000,
    0,
    47000
};

static constexpr Fixed MOVE_Y_SPEEDS[8] = {
    0,
    47000,
    FRACUNIT,
    47000,
    0,
    -47000,
    -FRACUNIT,
    -47000
};

static bool P_Move(mobj_t& actor) noexcept {
    const dirtype_t moveDir = (dirtype_t) actor.movedir;

    if (moveDir == DI_NODIR)    // No direction of travel?
        return false;           // Can't move
    
    const uint32_t speed = actor.InfoPtr->Speed;
    const Fixed tryX = actor.x + ((Fixed) speed * MOVE_X_SPEEDS[moveDir]);    // New x and y
    const Fixed tryY = actor.y + ((Fixed) speed * MOVE_Y_SPEEDS[moveDir]);

    // Try to open any specials if we can't move to a location
    if (!P_TryMove(&actor, tryX, tryY) ) {
        if (actor.flags & MF_FLOAT && floatok) {
            // Must adjust height
            if (actor.z < tmfloorz) {
                actor.z += FLOATSPEED;      // Jump up
            } else {
                actor.z -= FLOATSPEED;      // Jump down
            }

            actor.flags |= MF_INFLOAT;      // I am floating (Or falling)
            return true;                    // I can move!!
        }

        line_t* const pBlockLine = blockline;       // What line blocked me?

        if (!pBlockLine || !pBlockLine->special)    // Am I blocked?
            return false;                           // Can't move
        
        actor.movedir = DI_NODIR;   // Force no direction
        
        // If the special isn't a door that can be opened, return 'false'.
        // Otherwise if the door can be used then return 'true' to indicate we can move.
        return P_UseSpecialLine(&actor, pBlockLine);
    }

    actor.flags &= ~MF_INFLOAT;     // I am not floating anymore

    if ((actor.flags & MF_FLOAT) == 0) {    // Can I float at all?
        actor.z = actor.floorz;             // Attach to the floor
    }

    return true;    // Yes, I can move there...
}

//--------------------------------------------------------------------------------------------------
// Attempts to move actor in its current (ob->moveangle) direction.
//
// If blocked by either a wall or an actor returns 'false'.
// If move is either clear or blocked only by a door, returns 'true' and
// if a door is in the way, an OpenDoor call is made to start it opening.
//--------------------------------------------------------------------------------------------------
static bool P_TryWalk(mobj_t& actor) noexcept {
    if (!P_Move(actor))         // Try to move in this direction
        return false;           // Can't move there!
    
    actor.movecount = Random::nextU8(15);       // Get distance to travel
    return true;                                // I'm moving
}

//--------------------------------------------------------------------------------------------------
// Pick a direction to travel to chase the actor's target
//--------------------------------------------------------------------------------------------------
static constexpr dirtype_t OPPOSITE_DIR[] = {
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_NODIR
};

static constexpr dirtype_t DIAGONAL_DIRS[] = {
    DI_NORTHWEST,
    DI_NORTHEAST,
    DI_SOUTHWEST,
    DI_SOUTHEAST
};

static void P_NewChaseDir(mobj_t& actor) noexcept {
    const dirtype_t oldDir = (dirtype_t) actor.movedir;         // Get current direction
    const dirtype_t turnaroundDir = OPPOSITE_DIR[oldDir];       // Get opposite direction

    ASSERT(actor.target);
    mobj_t& target = *actor.target;
    const Fixed deltax = target.x - actor.x;    // Get the x offset
    const Fixed deltay = target.y - actor.y;    // Get the y offset

    dirtype_t d1;
    
    if (deltax >= (10 * FRACUNIT)) {    // Towards the east?
        d1 = DI_EAST;
    } else if (deltax < (-10 * FRACUNIT)) {     // Towards the west?
        d1 = DI_WEST;
    } else {
        d1 = DI_NODIR;  // Go straight
    }

    dirtype_t d2;

    if (deltay < (-10 * FRACUNIT)) {            // Towards the south?
        d2 = DI_SOUTH;
    } else if (deltay >= (10 * FRACUNIT)) {     // Towards the north?
        d2 = DI_NORTH;
    } else {
        d2 = DI_NODIR;  // Go straight
    }

    // Try direct route diagonally
    if (d1 != DI_NODIR && d2 != DI_NODIR) {
        uint32_t index = 0;

        if (deltay < 0) {
            index = 2;
        }

        if (deltax >= 0) {
            index |= 1;
        }

        actor.movedir = DIAGONAL_DIRS[index];

        if (actor.movedir != turnaroundDir && P_TryWalk(actor))
            return;     // It's ok!
    }

    // Try other directions
    if (Random::nextU8() >= 201 || (abs(deltax) < abs(deltay))) {
        const dirtype_t temp = d1;      // Reverse the direction priorities
        d1 = d2;
        d2 = temp;
    }

    // Invalidate reverse course
    if (d1 == turnaroundDir) {   
        d1 = DI_NODIR;
    }

    if (d2 == turnaroundDir) {
        d2 = DI_NODIR;
    }

    // Move in the highest priority direction first and then the lower priority direction
    if (d1 != DI_NODIR) {
        actor.movedir = d1;

        if (P_TryWalk(actor))
            return;     // Either moved forward or attacked
    }

    if (d2 != DI_NODIR) {
        actor.movedir = d2;     // Low priority direction

        if (P_TryWalk(actor))
            return;
    }

    // There is no direct path to the player, so pick another direction
    if (oldDir != DI_NODIR) {
        actor.movedir = oldDir;     // Try the old direction

        if (P_TryWalk(actor))
            return;
    }

    // Pick a direction by turning in a random order.
    // Don't choose the reverse course!
    if (Random::nextBool()) {   // Which way to go duh George?
        int tdir = DI_EAST;

        do {
            if (tdir != (int) turnaroundDir) {      // Not backwards?
                actor.movedir = (dirtype_t) tdir;
                if (P_TryWalk(actor))               // Can I go this way?
                    return;
            }
        } while (++tdir < (DI_SOUTHEAST + 1));
    } else {
        int tdir = DI_SOUTHEAST;

        do {
            if (tdir != (int) turnaroundDir) {      // Not backwards?
                actor.movedir = (dirtype_t) tdir;
                if (P_TryWalk(actor))               // Can I go this way?
                    return;
            }
        } while (--tdir >= (int) DI_EAST);  // Next step
    }

    // Hmmm, the only choice left is to turn around (assuming we have a valid dir)
    if (turnaroundDir != DI_NODIR) {
        actor.movedir = turnaroundDir;  // Try backwards

        if (P_TryWalk(actor))
            return;     // Ok, let's go!
    }

    actor.movedir = DI_NODIR;   // Can't move
}

//--------------------------------------------------------------------------------------------------
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//--------------------------------------------------------------------------------------------------
static bool P_LookForPlayers(mobj_t& actor, bool bAllAround) noexcept {
    // Pick another player as target if possible
    if (!(actor.flags & MF_SEETARGET)) {    // Can I see the player?
    newtarget:
        actor.target = players.mo;          // Force player #0 tracking
        return false;                       // No one is targeted
    }

    const mobj_t* const pTarget = actor.target;     // Get the target

    if (!pTarget || pTarget->MObjHealth <= 0) {     // Is it alive?
        goto newtarget;                             // Pick a target...
    }

    // Ambush guys will turn around on a shot
    if (actor.subsector->sector->soundtarget == actor.target) {
        bAllAround = true;
    }

    if (!bAllAround) {  // Only 180 degrees?
        const angle_t angle = PointToAngle(actor.x, actor.y, pTarget->x, pTarget->y) - actor.angle;

        if (angle > ANG90 && angle < ANG270) {  // In the span?
            const Fixed dist = GetApproxDistance(pTarget->x - actor.x, pTarget->y - actor.y);

            // If real close, react anyway
            if (dist > MELEERANGE) {
                return false;   // Behind back
            }
        }
    }

    actor.threshold = TICKSPERSEC * 4;  // Follow for 4 seconds
    return true;                        // I have a target!
}

//--------------------------------------------------------------------------------------------------
// Stay in state until a player is sighted
//--------------------------------------------------------------------------------------------------
void A_Look(mobj_t* const pActor) noexcept {
    // If current target is visible, start attacking
    if (!P_LookForPlayers(*pActor, false)) {
        return;
    }

    // Go into chase state
    uint32_t sound = pActor->InfoPtr->seesound;      

    if (sound) {                // Any sound to play?
        switch (sound) {        // Special case for the sound?
            case sfx_posit1:
            case sfx_posit2:
            case sfx_posit3:
                sound = sfx_posit1 + Random::nextU32(1);
                break;
            
            case sfx_bgsit1:
            case sfx_bgsit2:
                sound = sfx_bgsit1 + Random::nextU32(1);
        }

        S_StartSound(&pActor->x, sound);    // Begin a sound
    }

    SetMObjState(pActor, pActor->InfoPtr->seestate);    // Set the chase state
}

//--------------------------------------------------------------------------------------------------
// Actor has a melee attack, so it tries to close in as fast as possible
//--------------------------------------------------------------------------------------------------
void A_Chase(mobj_t* const pActor) noexcept {
    // Adjust the reaction time
    if (pActor->reactiontime) {
        --pActor->reactiontime;
    }
    
    // Modify target threshold
    if (pActor->threshold) {
        --pActor->threshold;
    }

    // Turn towards movement direction if not there yet
    if (pActor->movedir < 8) {
        pActor->angle &= (angle_t)(7UL<<29);
        const int32_t delta = pActor->angle - (pActor->movedir << 29);

        if (delta > 0) {
            pActor->angle -= ANG45;
        } else if (delta < 0) {
            pActor->angle += ANG45;
        }
    }

    // Look for a new target if required
    const mobjinfo_t* const pInfo = pActor->InfoPtr;

    if ((!pActor->target) || ((pActor->target->flags & MF_SHOOTABLE) == 0)) {        
        if (P_LookForPlayers(*pActor, true)) {
            return;     // Got a new target
        }

        SetMObjState(pActor, pInfo->spawnstate);    // Reset the state
        return;
    }

    // Don't attack twice in a row
    if (pActor->flags & MF_JUSTATTACKED) {      // Attacked?
        pActor->flags &= ~MF_JUSTATTACKED;      // Clear the flag
        P_NewChaseDir(*pActor);                 // Chase the player
        return;
    }

    // Check for melee attack
    if (pInfo->meleestate && CheckMeleeRange(pActor)) {
        S_StartSound(&pActor->x, pInfo->attacksound);   // Start attack sound
        SetMObjState(pActor, pInfo->meleestate);
        return;
    }

    // Check for missile attack
    if ((gameskill == sk_nightmare || pActor->movecount == 0) &&
        pInfo->missilestate &&
        CheckMissileRange(pActor)
    ) {
        SetMObjState(pActor, pInfo->missilestate);      // Shoot missile
        if (gameskill != sk_nightmare) {                // Ruthless!!
            pActor->flags |= MF_JUSTATTACKED;           // Don't attack next round
        }
        return;
    }

    // Chase towards player and move the critter
    if ((pActor->movecount == 0) || !P_Move(*pActor)) {
        P_NewChaseDir(*pActor);
    }

    // Make active sound - only 1 in 80 chance of gurgle
    if (Random::nextU8() < 3) {
        S_StartSound(&pActor->x, pInfo->activesound);
    }

    // Decrease time until turn
    if (pActor->movecount > 0) {
        --pActor->movecount;
    }
}

//--------------------------------------------------------------------------------------------------
// Turn to face your target
//--------------------------------------------------------------------------------------------------
void A_FaceTarget(mobj_t* const pActor) noexcept {
    const mobj_t* const pTarget = pActor->target;

    if (pTarget) {                      // Is there a target?
        pActor->flags &= ~MF_AMBUSH;    // Not ambushing anymore
        pActor->angle = PointToAngle(pActor->x, pActor->y, pTarget->x, pTarget->y);

        if (pActor->target->flags & MF_SHADOW) {    // Hard to see?
            pActor->angle += (255 - Random::nextU32(511)) << 21;
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Shoot the player with a pistol (Used by Zombiemen)
//--------------------------------------------------------------------------------------------------
void A_PosAttack(mobj_t* const pActor) noexcept {   
    if (pActor->target) {                       // Is there a target?
        A_FaceTarget(pActor);                   // Face the target
        S_StartSound(&pActor->x, sfx_pistol);   // Shoot the pistol
        
        angle_t angle = pActor->angle;                          // Get the angle
        angle += (255 - Random::nextU32(511)) << 20;            // Angle of error
        const uint32_t damage = (Random::nextU32(7) + 1) * 3;   // 1D8 * 3

        LineAttack(pActor, angle, MISSILERANGE, MAXINT, damage);
    }
}

//--------------------------------------------------------------------------------------------------
// Shoot the player with a shotgun (Used by Shotgun man)
//--------------------------------------------------------------------------------------------------
void A_SPosAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {
        S_StartSound(&pActor->x, sfx_shotgn);
        A_FaceTarget(pActor);
        const angle_t bAngle = pActor->angle;   // Base angle
        
        for (uint32_t i = 0; i < 3u; ++i) {
            const angle_t angle = bAngle + ((255 - Random::nextU32(511)) << 20);
            const uint32_t damage = (Random::nextU32(7) + 1 ) * 3;
            LineAttack(pActor, angle, MISSILERANGE, MAXINT, damage);
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Spider demon firing machine gun
//--------------------------------------------------------------------------------------------------
void A_SpidRefire(mobj_t* const pActor) noexcept {    
    A_FaceTarget(pActor);

    // Keep firing unless target got out of sight
    if (Random::nextU8(255) >= 10) {
        const bool bNoTarget = (
            (!pActor->target) ||
            (pActor->target->MObjHealth <= 0) ||
            ((pActor->flags & MF_SEETARGET) == 0)
        );

        if (bNoTarget) {
            SetMObjState(pActor, pActor->InfoPtr->seestate);
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Imp attack
//--------------------------------------------------------------------------------------------------
void A_TroopAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {
        A_FaceTarget(pActor);   // Face your victim

        if (CheckMeleeRange(pActor)) {
            S_StartSound(&pActor->x, sfx_claw);                     // Claw sound
            const uint32_t damage = (Random::nextU32(7) + 1) * 3;   // 1D8 * 3
            DamageMObj(pActor->target, pActor, pActor, damage);
            return;                                                 // End attack
        }
        
        // Launch a imp's missile
        P_SpawnMissile(pActor, pActor->target, &mobjinfo[MT_TROOPSHOT]);
    }
}

//--------------------------------------------------------------------------------------------------
// Demon or Spectre attack
//--------------------------------------------------------------------------------------------------
void A_SargAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {
        A_FaceTarget(pActor);                                       // Face the player
        const uint32_t damage = (Random::nextU32(7) + 1) * 4;       // 1D8 * 4
        LineAttack(pActor, pActor->angle, MELEERANGE, 0, damage);   // Attack
    }
}

//--------------------------------------------------------------------------------------------------
// Evil eye attack
//--------------------------------------------------------------------------------------------------
void A_HeadAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {       // Anyone targeted?
        A_FaceTarget(pActor);   // Face the target

        if (CheckMeleeRange(pActor)) {                              // In bite range?
            const uint32_t damage = (Random::nextU32(7) + 1) * 8;   // 1D8 * 8
            DamageMObj(pActor->target, pActor, pActor, damage);
            return;
        }
        
        // Launch a missile - shoot eye missile
        P_SpawnMissile(pActor, pActor->target, &mobjinfo[MT_HEADSHOT]);
    }
}

//--------------------------------------------------------------------------------------------------
// Cyberdemon firing missile
//--------------------------------------------------------------------------------------------------
void A_CyberAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {
        A_FaceTarget(pActor);                                           // Face the enemy
        P_SpawnMissile(pActor, pActor->target, &mobjinfo[MT_ROCKET]);   // Launch missile
    }
}

//--------------------------------------------------------------------------------------------------
// Baron of hell attack
//--------------------------------------------------------------------------------------------------
void A_BruisAttack(mobj_t* const pActor) noexcept {
    if (pActor->target) {                                               // Target aquired?
        if (CheckMeleeRange(pActor)) {                                  // Claw range?
            S_StartSound(&pActor->x, sfx_claw);                         // Ouch!
            const uint32_t damage = (Random::nextU32(7) + 1) * 11;      // 1D8 * 11
            DamageMObj(pActor->target, pActor, pActor, damage);
            return;
        }

        // Launch a missile
        P_SpawnMissile(pActor, pActor->target, &mobjinfo[MT_BRUISERSHOT]);
    }
}

//--------------------------------------------------------------------------------------------------
// Fly at the player like a missile
//--------------------------------------------------------------------------------------------------
void A_SkullAttack(mobj_t* const pActor) noexcept {
    const mobj_t* const pDest = pActor->target;

    if (pDest) {                                                    // Target aquired?
        pActor->flags |= MF_SKULLFLY;                               // High speed mode
        S_StartSound(&pActor->x, pActor->InfoPtr->attacksound);
        A_FaceTarget(pActor);                                       // Face the target

        // Speed for distance
        const angle_t angle = pActor->angle >> ANGLETOFINESHIFT;
        pActor->momx = sfixedMul16_16(SKULLSPEED, finecosine[angle]);
        pActor->momy = sfixedMul16_16(SKULLSPEED, finesine[angle]);

        uint32_t dist = GetApproxDistance(pDest->x - pActor->x, pDest->y - pActor->y);
        dist = dist / SKULLSPEED;   // Speed to hit target
        if (dist == 0) {            // Prevent divide by 0
            dist = 1;
        }

        pActor->momz = (pDest->z + (pDest->height >> 1) - pActor->z) / dist;
    }
}

//--------------------------------------------------------------------------------------------------
// Play a normal death sound
//--------------------------------------------------------------------------------------------------
void A_Scream(mobj_t* const pActor) noexcept {
    uint32_t sound = pActor->InfoPtr->deathsound;

    switch (sound) {
        case 0:         // No sound at all?
            return;
        
        case sfx_podth1:
        case sfx_podth2:
        case sfx_podth3:
            sound = sfx_podth1 + Random::nextU32(1);
            break;

        case sfx_bgdth1:
        case sfx_bgdth2:
            sound = sfx_bgdth1 + Random::nextU32(1);
            break;
    }

    S_StartSound(&pActor->x, sound);
}

//--------------------------------------------------------------------------------------------------
// Play the gory squish sound for a gruesome death
//--------------------------------------------------------------------------------------------------
void A_XScream(mobj_t* const pActor) noexcept {
    S_StartSound(&pActor->x, sfx_slop);
}

//--------------------------------------------------------------------------------------------------
// Play the pain sound if any
//--------------------------------------------------------------------------------------------------
void A_Pain(mobj_t* const actor) noexcept {
    S_StartSound(&actor->x, actor->InfoPtr->painsound);
}

//--------------------------------------------------------------------------------------------------
// Actor fell to the ground dead, mark so you can walk over it
//--------------------------------------------------------------------------------------------------
void A_Fall(mobj_t* const pActor) noexcept {
    pActor->flags &= ~MF_SOLID;     // Not solid anymore
}

//--------------------------------------------------------------------------------------------------
// Process damage from an explosion
//--------------------------------------------------------------------------------------------------
void A_Explode(mobj_t* const pActor) noexcept {
    RadiusAttack(pActor, pActor->target, 128);    // BOOM!
}

//--------------------------------------------------------------------------------------------------
// For level #8 of Original DOOM, I will trigger event #666
// to allow you to access the exit portal when you kill the
// Barons of Hell.
//--------------------------------------------------------------------------------------------------
void A_BossDeath(mobj_t* const pActor) noexcept {    
    line_t junk;

    if (gamemap != 8) {     // Level #8?
        return;             // Kill all you want, we'll make more!
    }

    // Scan the remaining thinkers to see if all bosses are dead.
    // This is a brute force method, but it works!
    const mobj_t* pActor2 = mobjhead.next;

    do {
        if (pActor2 != pActor && pActor2->InfoPtr == pActor->InfoPtr && pActor2->MObjHealth) {
            return;     // Other boss not dead
        }

        // Keep scanning the list
        pActor2 = pActor2->next;

    } while (pActor2 != &mobjhead);     // Stop when wrapped around back to the start

    // Victory!
    junk.tag = 666;                         // Floor's must be tagged with 666
    EV_DoFloor(&junk,lowerFloorToLowest);   // Open the level
}

//--------------------------------------------------------------------------------------------------
// Play the Cyberdemon's metal hoof sound
//--------------------------------------------------------------------------------------------------
void A_Hoof(mobj_t* const pActor) noexcept {
    S_StartSound(&pActor->x, sfx_hoof);     // Play the sound
    A_Chase(pActor);                        // Chase the player
}

//--------------------------------------------------------------------------------------------------
// Make the spider demon's metal leg sound
//--------------------------------------------------------------------------------------------------
void A_Metal(mobj_t* const pActor) noexcept {
    S_StartSound(&pActor->x, sfx_metal);    // Make the sound
    A_Chase(pActor);                        // Handle the standard chase code
}

//--------------------------------------------------------------------------------------------------
// A move in Base.cpp caused a missile to hit another thing or wall
//--------------------------------------------------------------------------------------------------
void L_MissileHit(mobj_t* const mapObj, mobj_t* const pMissile) noexcept {
    if (pMissile) {
        const uint32_t damage = (Random::nextU32(7) +1 ) * mapObj->InfoPtr->damage;     // Calc the damage
        DamageMObj(pMissile, mapObj, mapObj->target, damage);                           // Inflict damage
    }

    ExplodeMissile(mapObj);     // Detonate the missile
}

//--------------------------------------------------------------------------------------------------
// A move in Base.cpp caused a flying skull to hit another thing or a wall
//--------------------------------------------------------------------------------------------------
void L_SkullBash(mobj_t* const pMapObj, mobj_t* const pSkull) noexcept {
    if (pSkull) {
        const uint32_t damage = (Random::nextU32(7) + 1) * pMapObj->InfoPtr->damage;
        DamageMObj(pSkull, pMapObj, pMapObj, damage);
    }

    pMapObj->flags &= ~MF_SKULLFLY;                         // The skull isn't flying fast anymore
    pMapObj->momx = pMapObj->momy = pMapObj->momz = 0;      // Zap the momentum
    SetMObjState(pMapObj, pMapObj->InfoPtr->spawnstate);    // Normal mode
}
