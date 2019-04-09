#include "hsd_memory.h"

#include <stdint.h>

static HSD_MemoryEntry** entrybysz;
static u32 sz_entries = 0;

static HSD_ObjDef hsd_iddata; //804C23C0
static u32* hsd_idtable[101]; //804C23EC

//8037CD80
u32* HSD_IDGetAllocData(){
	return &hsd_iddata;
}

//8037CD8C
void HSD_IDInitAllocData(){
	HSD_ObjAllocInit(&hsd_iddata, 12, 4);
}

//8037CDBC
void* HSD_IDSetup(){
	return memset(&hsd_idtable, 0, 404);
}

//8037CDEC
void HSD_IDInsertToTable(u32** id_table, u32* id, u8** j){
	if(id_table == NULL){
		id_table = &hsd_idtable;
	}
	u32* unk1 = (id_table + ((uintptr_t)id % 101) * 4); 
	u32* unk2 = *unk1; 
	while(unk2 != NULL && unk2[1] == id){
		unk2 = (u32*)*unk2;
	}
	if(unk2 == NULL){
		unk2 = HSD_ObjAlloc(&hsd_iddata);
		assert(unk2 != NULL);
		memset(unk2, 0, 12);
		unk2[1] = id;
		unk2[2] = j;
		unk2[0] = unk1[0];
		unk1[0] = unk2;

	}else{
		unk2[1] = id;
		unk2[2] = j;
	}
}

//8037CEE8
void HSD_IDRemoveByIDFromTable(u32* id_table, u32* id){
	if(id_table == NULL){
		id_table = &hsd_idtable;
	}
	u32* unk1 = (id_table + ((uintptr_t)id % 101) * 4);
	u32* unk2 = NULL;
	u32* unk3 = NULL;
	while(true) {
		unk2 = unk1;
		if (unk2 == NULL) {
			return;
		}
		if (unk2[1] == id) 
			break;
		unk1 = (u32*)*unk2;
		unk3 = unk2;
	}
	if (unk3 == NULL) {
		*(u32*)(unk1 + ((uintptr_t)id % 101) * 4) = *unk2;
	}else {
		*unk3 = *unk2;
	}
	HSD_ObjFree(&hsd_iddata, unk2);
}

//8037CF98
void HSD_IDGetDataFromTable(u32* id_table, u32* id, u8* val){
	if(id_table == NULL){
		id_table = &hsd_idtable;
	}
	u32* unk = (id_table + ((uintptr_t)id % 101) * 4);
	while(true){
		if(unk == NULL){
			if(val != NULL){
				*val = 0;
			}
			return 0;
		}
		if(unk[1] = id)
			break;
		unk = *unk;
	}
	if(val != NULL){
		*val = 1;
	}
	return unk[2];
}

//8037D020
void _HSD_IDForgetMemory(){
	return memset(&hsd_idtable, 0, 101*sizeof(u32));
}

//8037F1B0
void HSD_Free(void* ptr){
	//OSFreeToHeap(HSD_GetHeap(), ptr);
	free(ptr);
}

//8037F1E4
void* HSD_MemAlloc(u32 size){
	void* result = NULL;
	
	if(size > 0){	
		//result = OSAllocFromHeap(HSD_GetHeap(), size);
		result = malloc(size);
		assert(result != NULL);
	}
	return result;
}

//80381D58
HSD_MemoryEntry* GetMemoryEntry(u32 size){
	assert(size >= 0);
	if ( size >= sz_entries )
	{
		if ( sz_entries > 0 ){ //Basically amounts to resizing the array by doubling the size and copying entries to a new array
			u32 i = 2 * sz_entries;
			while(size >= i){
				i *= 2;
			}
			HSD_MemoryEntry* entry = (HSD_MemoryEntry*)HSD_MemAlloc(4 * i);
			if ( entry == NULL ){
				return NULL;
			}
			memcpy(entry, entrybysz, 4 * sz_entries);
			memset(&entry[sz_entries], 0, 4 * (i - sz_entries)); //You start *after* existing ptrs and make sure memory is zero'd
			
			HSD_MemoryEntry** org_entrybysz = entrybysz;
			u32 tmp = sz_entries;
			u32 offset = 4 * tmp & 0xFFFFFFE0;
			sz_entries = i;
			entrybysz = &entry;
			hsdFreeMemPiece(org_entrybysz, sz_entries);
			HSD_MemoryEntry* unk_entry = (HSD_MemoryEntry*)(*entrybysz + (((u32)(offset + 31) >> 3) & 0x1FFFFFFC) - 4);
			unk_entry->x4_unk += 1;
		} else { //In this case, it's uninitialized and allocs the array
			u32 j = 32;
			while(size >= j){
				j *= 2;
			}
			HSD_MemoryEntry* entry = (HSD_MemoryEntry*)HSD_MemAlloc(4 * j);
			entrybysz = &entry;
			if ( entry == NULL ){
				return NULL;
			}
			memset(entry, 0, 4 * j);
			sz_entries = j;
		}
	}
	if ( entrybysz[size] == NULL ) {
		HSD_MemoryEntry* entry = (HSD_MemoryEntry*)HSD_MemAlloc(0x14u);
		if ( entry == NULL ){
			return NULL;
		}
		memset(entry, 0, 0x14u);
		entry->total_bits = 32 * (size + 1);
		entrybysz[size] = entry;
		
		s32 idx = size - 1;
		BOOL not_first = FALSE;
		u32 tmp_size = size;
		HSD_MemoryEntry* t_entry;
		if ( idx >= 0 ){
			for(t_entry = entrybysz[idx]; t_entry != NULL; t_entry[idx]){
				--idx;
				if ( !--tmp_size )
					goto JUMP_OUT;
			}
			not_first = TRUE;
			entry->next = entrybysz[idx]->next;
			entrybysz[idx]->next = entry;
		}
JUMP_OUT:
		if ( not_first == FALSE ){
			idx = size + 1;
			tmp_size = sz_entries - (size + 1);
			if ( idx < sz_entries ){
				for(t_entry = entrybysz[idx]; t_entry != NULL; t_entry[idx]){
					++idx;
					if ( !--tmp_size )
						return entrybysz[size];
				}
				entry->next = entrybysz[idx];
			}
		}
	}
	return entrybysz[size];
}

//80381FA8
HSD_MemPiece* hsdAllocMemPiece(u32 size){
	HSD_MemoryEntry* entry;
	HSD_MemoryEntry* entry_2;
	HSD_MemPiece* piece;

	entry = GetMemoryEntry(((size + 31) >> 5) - 1);
	if(entry == NULL){
		return NULL;
	}
	piece = entry->data;
	if(piece != NULL){
		entry->data = piece->next;
		entry->free_pieces -= 1;
	}else{
		for(HSD_MemoryEntry* i = entry->next; i != NULL; i = entry->next){
			if(i->data != NULL){
				entry_2 = GetMemoryEntry((i->total_bits - entry->total_bits + 31) >> 5) + size - 1;
				if(entry_2 == NULL){
					return NULL;
				}
				piece = i->data;
				i->data = i->data->next;
				i->free_pieces -= 1;
				i->x4_unk -= 1;
				
				HSD_MemPiece* np = (HSD_MemPiece*)((u8*)piece + entry->total_bits);
				np->next = entry_2->data;
				entry_2->data = np;
				np->x4_unk += 1;
				np->x8_unk += 1;
				entry->x4_unk += 1;
				return piece;
			}
		}
		u32 tmp_size = sz_entries - ((size + 31) >> 5) - 2;
		if(tmp_size < 0 || (entry_2 = GetMemoryEntry(tmp_size)) != NULL){
			piece = (HSD_MemPiece*)HSD_MemAlloc(sz_entries * 32);
			if(piece != NULL){
				if(tmp_size >= 0){
					HSD_MemPiece* np = (HSD_MemPiece*)((u8*)piece + entry->total_bits);
					np->next = entry->data;
					entry->data = np;
					np->x4_unk += 1;
					np->x8_unk += 1;
				}
				entry->x4_unk += 1;
			}else{
				return NULL;
			}
		}else{
			return NULL;
		}
	}
	return piece;
}

//8038216C
void hsdFreeMemPiece(void* mem, u32 size){
	HSD_MemoryEntry* entry;
	HSD_MemPiece* piece = (HSD_MemPiece*)mem;

	if ( mem != NULL ){
		entry = GetMemoryEntry(((size + 31) >> 5) - 1);
		piece->next = entry->data;
		entry->data = piece;
		entry->free_pieces += 1;
	}
	return entry;
}