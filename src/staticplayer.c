#include "staticplayer.h"

struct StaticPlayer players[MAX_PLAYERS];

//800322C0
u32 StaticPlayer_GetState(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].state;
}

//80032330
u32 StaticPlayer_GetExternalID(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].character_id;
}

//800323A0
struct StaticPlayer* StaticPlayer_SetExternalID(u32 slot, u32 id){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].character_id = id;
	return &players[slot];
}

//8003241C
u32 StaticPlayer_GetSlotType(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].slot_type;
}

//8003254C
struct StaticPlayer* StaticPlayer_SetSlotType(u32 slot, u32 type){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].slot_type = type;
	return &players[slot];
}

//800326CC
f32* StaticPlayer_GetNametagCoords(u32 slot, f32 *pos){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	if(players[slot].transformed == 1){
		pos[0] = players[slot].nametag_pos[0];
		pos[1] = players[slot].nametag_pos[1];
		pos[2] = players[slot].nametag_pos[2];
	} else if(players[slot].transformed == 256){
		pos[0] = players[slot].transformed_pos[0];
		pos[1] = players[slot].transformed_pos[1];
		pos[2] = players[slot].transformed_pos[2];
	}
	return pos;
}

//80033024
f32 StaticPlayer_GetFacing(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].facedir;
}

//80033094
struct StaticPlayer* StaticPlayer_SetFacing(u32 slot, f32 facing){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].facedir = facing;
	return &players[slot];
}

//80033548
u8 StaticPlayer_GetCPULevel(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].cpu_level;
}

//80033798
u8 StaticPlayer_GetHandicap(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].handicap;
}

//80033808
struct StaticPlayer* StaticPlayer_SetHandicap(u32 slot, u8 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].handicap = amt;
	return &players[slot];
}

//80033884
f32 StaticPlayer_Get50(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].unk50;
}

//800338F4
f32 StaticPlayer_GetAttackRatio(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].attack_ratio;
}

//80033964
struct StaticPlayer* StaticPlayer_SetAttackRatio(u32 slot, f32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].attack_ratio = amt;
	return &players[slot];
}

//800339E0
f32 StaticPlayer_GetDefenseRatio(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].dmgdef_ratio;
}

//80033A50
struct StaticPlayer* StaticPlayer_SetDefenseRatio(u32 slot, f32 ratio){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].dmgdef_ratio = ratio;
	return &players[slot];
}

//80033ACC
f32 StaticPlayer_GetModelScale(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].model_scale;
}

//80033B3C
struct StaticPlayer* StaticPlayer_SetModelScale(u32 slot, f32 scale){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].model_scale = scale;
	return &players[slot];
}

//80034C04
u16 StaticPlayer_GetStaminaHP(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	u16 hp = players[slot].stamina_hp;
	if(hp < 0)
		return 0;
	return hp;
}

//80034524
struct StaticPlayer* StaticPlayer_SetStaminaHP(u32 slot, u16 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].stamina_hp = amt;
	return &players[slot];
}

//80034D78
u32 StaticPlayer_GetFalls(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].falls;
}

//80034E04
struct StaticPlayer* StaticPlayer_SetFalls(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].falls = amt;
	return &players[slot];
}

//80034F24
u32 StaticPlayer_GetKillCount(u32 slot, u32 tslot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].ko_counter[tslot];
}

//80035114
s32 StaticPlayer_GetMatchFrames(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].match_frames;
}

//80035184
s32 StaticPlayer_SetMatchFrames(u32 slot, bool r4){
	s32 result = 0;
	
	assert(slot >= 0 && slot < MAX_PLAYERS);
	if(!r4){
		result = players[slot].match_frames;
		if(result == -1){
			result = MatchInfo_GetFrameCount();
			players[slot].match_frames = result;
		}
	}
	return result;
}

//80033BD8
u8 StaticPlayer_GetStocks(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].stocks;
}

//80033C4C
u8 StaticPlayer_GetPlayer1Stocks(){
	return players[0].stocks;
}

//80033C60
struct StaticPlayer* StaticPlayer_SetStocks(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].stocks = amt;
	return &players[slot];
}

//80033CE0
u32 StaticPlayer_DecrementStocks(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	if(players[slot].stocks > 0)
		players[slot].stocks = players[slot].stocks - 1;
	return players[slot].stocks;
}

//80033D60
u32 StaticPlayer_GetCurrentCoins(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].curr_coins;
}

//80033DD0
struct StaticPlayer* StaticPlayer_SetCurrentCoins(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].curr_coins = amt;
	return &players[slot];
}

//80033E4C
u32 StaticPlayer_GetTotalCoins(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].total_coins;
}

//80033EBC
struct StaticPlayer* StaticPlayer_SetTotalCoins(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].total_coins = amt;
	return &players[slot];
}

//80033F38
u32 StaticPlayer_Get98(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].unk98;
}

//80033FA8
struct StaticPlayer* StaticPlayer_Set98(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].unk98 = amt;
	return &players[slot];
}

//80034024
u32 StaticPlayer_Get9C(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return players[slot].unk9C;
}

//80034094
struct StaticPlayer* StaticPlayer_Set9C(u32 slot, u32 amt){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	players[slot].unk9C = amt;
	return &players[slot];
}

//8003418C
struct Entity* StaticPlayer_GetCharacterEntity(u32 slot, bool subchar){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	if(subchar){
		return players[slot].subchar_ent;
	}
	return players[slot].player_ent;
}

//80034110
struct Player* StaticPlayer_GetPlayerStruct(u32 slot, bool subchar){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	if(subchar){
		return (struct Player*)players[slot].subchar_ent->entity_data;
	}
	return (struct Player*)players[slot].player_ent->entity_data;
}

//80036244
//80036324
u32* StaticPlayer_GetStaleMoveTable(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return &players[slot].stale_writeidx;
}

//800361D4
u32* StaticPlayer_GetStatsPtr(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return &players[slot].stats[0];
}

//800362B4
u32* StaticPlayer_Get6A8(u32 slot){
	assert(slot >= 0 && slot < MAX_PLAYERS);
	return &players[slot].unk6A8;
}