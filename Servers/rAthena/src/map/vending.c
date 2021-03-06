// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/nullpo.h"
#include "../common/malloc.h" // aMalloc, aFree
#include "../common/showmsg.h" // ShowInfo
#include "../common/strlib.h"
#include "../common/utils.h"
#include "channel.h"
#include "clif.h"
#include "itemdb.h"
#include "atcommand.h"
#include "path.h"
#include "chrif.h"
#include "vending.h"
#include "pc.h"
#include "achievement.h"

#include <stdlib.h> // atoi

/// Struct for vending entry of autotrader
struct s_autotrade_entry {
	int cartinventory_id;
	uint16 amount;
	int price;
	uint16 index;
};

/// Struct of autotrader
struct s_autotrade {
	uint32 account_id;
	uint32 char_id;
	int vendor_id;
	int m;
	uint16 x,
		y;
	unsigned char sex, dir, head_dir, sit;
	char title[MESSAGE_SIZE];
	uint16 count;
	struct s_autotrade_entry **entries;
	struct map_session_data *sd;
};

static int vending_nextid = 0; ///Vending_id counter
static DBMap *vending_db; ///Db holder the vender : charid -> map_session_data

//Autotrader
static struct s_autotrade **autotraders; ///Autotraders Storage
static uint16 autotrader_count, autotrader_loaded_count; ///Autotrader count
static void do_final_vending_autotrade(void);

/**
 * Lookup to get the vending_db outside module
 * @return the vending_db
 */
DBMap * vending_getdb()
{
	return vending_db;
}

/**
 * Create an unique vending shop id.
 * @return the next vending_id
 */
static int vending_getuid(void)
{
	return ++vending_nextid;
}

/**
 * Make a player close his shop
 * @param sd : player session
 */
void vending_closevending(struct map_session_data* sd)
{
	nullpo_retv(sd);

	if( sd->state.vending ) {
		if( Sql_Query( mmysql_handle, "DELETE FROM `%s` WHERE vending_id = %d;", vending_items_db, sd->vender_id ) != SQL_SUCCESS ||
			Sql_Query( mmysql_handle, "DELETE FROM `%s` WHERE `id` = %d;", vendings_db, sd->vender_id ) != SQL_SUCCESS ) {
				Sql_ShowDebug(mmysql_handle);
		}
		
		sd->state.vending = false;
		sd->vender_id = 0;
		sd->vend_coin = battle_config.vending_zeny_id;
		clif_closevendingboard(&sd->bl, 0);
		idb_remove(vending_db, sd->status.char_id);

		if( map[sd->bl.m].flag.vending_cell ) // Cell becomes available again.
			map_setcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_NOVENDING, true);
	}
}

/**
 * Player request a shop's item list (a player shop)
 * @param sd : player requestion the list
 * @param id : vender account id (gid)
 */
void vending_vendinglistreq(struct map_session_data* sd, int id)
{
	struct map_session_data* vsd;
	nullpo_retv(sd);

	if( (vsd = map_id2sd(id)) == NULL )
		return;
	if( !vsd->state.vending )
		return; // not vending
	if( !battle_config.faction_allow_vending && vsd->status.faction_id != sd->status.faction_id )
	{
		clif_displaymessage(sd->fd,"You cannot purchase from other faction members.");
		return;
	}

	if (!pc_can_give_items(sd) || !pc_can_give_items(vsd)) { //check if both GMs are allowed to trade
		clif_displaymessage(sd->fd, msg_txt(sd,246));
		return;
	}

	sd->vended_id = vsd->vender_id;  // register vending uid
	if( battle_config.vending_zeny_id && vsd->vend_coin )
	{ // Extended Vending System
		char output[256];
		sprintf(output,msg_txt(sd,764),itemdb_jname(vsd->vend_coin));
		clif_displaymessage(sd->fd,output);
	}

	clif_vendinglist(sd, id, vsd->vending);
}

/**
 * Purchase item(s) from a shop
 * @param sd : buyer player session
 * @param aid : account id of vender
 * @param uid : shop unique id
 * @param data : items data who would like to purchase \n
 *	data := {<index>.w <amount>.w }[count]
 * @param count : number of different items he's trying to buy
 */
void vending_purchasereq(struct map_session_data* sd, int aid, int uid, const uint8* data, int count)
{
	int i, j, cursor, w, new_ = 0, blank, vend_list[MAX_VENDING];
	double z;
	struct s_vending vending[MAX_VENDING]; // against duplicate packets
	struct map_session_data* vsd = map_id2sd(aid);
	char output[256];

	nullpo_retv(sd);
	if( vsd == NULL || !vsd->state.vending || vsd->bl.id == sd->bl.id )
		return; // invalid shop

	if( vsd->vender_id != uid ) { // shop has changed
		clif_buyvending(sd, 0, 0, 6);  // store information was incorrect
		return;
	}

	if( !searchstore_queryremote(sd, aid) && ( sd->bl.m != vsd->bl.m || !check_distance_bl(&sd->bl, &vsd->bl, AREA_SIZE) ) )
		return; // shop too far away

	searchstore_clearremote(sd);

	if( count < 1 || count > MAX_VENDING || count > vsd->vend_num )
		return; // invalid amount of purchased items

	blank = pc_inventoryblank(sd); //number of free cells in the buyer's inventory

	// duplicate item in vending to check hacker with multiple packets
	memcpy(&vending, &vsd->vending, sizeof(vsd->vending)); // copy vending list

	// some checks
	z = 0.; // zeny counter
	w = 0;  // weight counter
	for( i = 0; i < count; i++ ) {
		short amount = *(uint16*)(data + 4*i + 0);
		short idx    = *(uint16*)(data + 4*i + 2);
		idx -= 2;

		if( amount <= 0 )
			return;

		// check of item index in the cart
		if( idx < 0 || idx >= MAX_CART )
			return;

		ARR_FIND( 0, vsd->vend_num, j, vsd->vending[j].index == idx );
		if( j == vsd->vend_num )
			return; //picked non-existing item
		else
			vend_list[i] = j;

		z += ((double)vsd->vending[j].value * (double)amount);
		if( !vsd->vend_coin || vsd->vend_coin == battle_config.vending_zeny_id )
		{ // Normal Vending - Zeny Option
			if( z > (double)sd->status.zeny || z < 0. || z > (double)MAX_ZENY )
			{
				clif_buyvending(sd, idx, amount, 1); // you don't have enough zeny
				return;
			}
			if( z + (double)vsd->status.zeny > (double)MAX_ZENY && !battle_config.vending_over_max )
			{
				clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // too much zeny = overflow
				return;
			}
		}
		else if( battle_config.vending_cash_id && vsd->vend_coin == battle_config.vending_cash_id )
		{ // Cash Shop
			if( z > (double)sd->cashPoints || z < 0. || z > (double)MAX_ZENY )
			{
				sprintf(output,msg_txt(sd,765),itemdb_jname(vsd->vend_coin));
				clif_displaymessage(sd->fd,output);
				return;
			}
			if( z + (double)vsd->cashPoints > (double)MAX_ZENY && !battle_config.vending_over_max )
			{
				sprintf(output,msg_txt(sd,766),itemdb_jname(vsd->vend_coin));
				clif_displaymessage(sd->fd,output);
				return;
			}
		}

		w += itemdb_weight(vsd->status.cart[idx].nameid) * amount;
		if( w + sd->weight > sd->max_weight ) {
			clif_buyvending(sd, idx, amount, 2); // you can not buy, because overweight
			return;
		}

		//Check to see if cart/vend info is in sync.
		if( vending[j].amount > vsd->status.cart[idx].amount )
			vending[j].amount = vsd->status.cart[idx].amount;

		// if they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// here, we check cumulative amounts
		if( vending[j].amount < amount ) {
			// send more quantity is not a hack (an other player can have buy items just before)
			clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // not enough quantity
			return;
		}

		vending[j].amount -= amount;

		switch( pc_checkadditem(sd, vsd->status.cart[idx].nameid, amount) ) {
		case CHKADDITEM_EXIST:
			break;	//We'd add this item to the existing one (in buyers inventory)
		case CHKADDITEM_NEW:
			new_++;
			if (new_ > blank)
				return; //Buyer has no space in his inventory
			break;
		case CHKADDITEM_OVERAMOUNT:
			return; //too many items
		}
	}

	// Payments
	if( !vsd->vend_coin || vsd->vend_coin == battle_config.vending_zeny_id )
	{
		pc_payzeny(sd, (int)z, LOG_TYPE_VENDING, vsd);
		achievement_validate_zeny(sd,ATZ_USE_VENDING,(int)z);

		if( battle_config.vending_tax || (vsd->state.autotrade && battle_config.at_tax) )
			z -= z * ((battle_config.vending_tax + ((vsd->state.autotrade) ? battle_config.at_tax : 0)) / 10000.);

		pc_getzeny(vsd, (int)z, LOG_TYPE_VENDING, sd);
		achievement_validate_zeny(vsd,ATZ_GET_VENDING,(int)z);
	}
	else if( battle_config.vending_cash_id && vsd->vend_coin == battle_config.vending_cash_id )
	{
		pc_paycash(sd,(int)z,0,LOG_TYPE_VENDING);
		pc_getcash(vsd,(int)z,0,LOG_TYPE_VENDING);
	}
	else
	{
		if( z < 0. || (i = pc_search_inventory(sd,vsd->vend_coin)) < 0 || z > (double)sd->status.inventory[i].amount )
		{
			sprintf(output,msg_txt(sd,715),itemdb_jname(vsd->vend_coin));
			clif_displaymessage(sd->fd,output);
			return;
		}

		switch( pc_checkadditem(vsd,vsd->vend_coin,(int)z) )
		{
		case CHKADDITEM_NEW:
			if( pc_inventoryblank(vsd) > 0 )
				break;
		case CHKADDITEM_OVERAMOUNT:
			sprintf(output,msg_txt(sd,716),itemdb_jname(vsd->vend_coin));
			clif_displaymessage(sd->fd,output);
			return;
		}

		pc_additem(vsd,&sd->status.inventory[i],(int)z,LOG_TYPE_VENDING);
		pc_delitem(sd,i,(int)z,0,6,LOG_TYPE_VENDING);
	}

	for( i = 0; i < count; i++ ) {
		short amount = *(uint16*)(data + 4*i + 0);
		short idx    = *(uint16*)(data + 4*i + 2);
		idx -= 2;

		// vending item
		pc_additem(sd, &vsd->status.cart[idx], amount, LOG_TYPE_VENDING);
		vsd->vending[vend_list[i]].amount -= amount;

		if( vsd->vending[vend_list[i]].amount ) {
			if( Sql_Query( mmysql_handle, "UPDATE `%s` SET `amount` = %d WHERE `vending_id` = %d and `cartinventory_id` = %d", vending_items_db, vsd->vending[vend_list[i]].amount, vsd->vender_id, vsd->status.cart[idx].id ) != SQL_SUCCESS ) {
				Sql_ShowDebug( mmysql_handle );
			}
		} else {
			if( Sql_Query( mmysql_handle, "DELETE FROM `%s` WHERE `vending_id` = %d and `cartinventory_id` = %d", vending_items_db, vsd->vender_id, vsd->status.cart[idx].id ) != SQL_SUCCESS ) {
				Sql_ShowDebug( mmysql_handle );
			}
		}

		pc_cart_delitem(vsd, idx, amount, 0, LOG_TYPE_VENDING);
		clif_vendingreport(vsd, idx, amount);

		//print buyer's name
		if( battle_config.buyer_name ) {
			char temp[256];
			sprintf(temp, msg_txt(sd,265), sd->status.name);
			clif_disp_onlyself(vsd,temp,strlen(temp));
		}
	}

	// compact the vending list
	for( i = 0, cursor = 0; i < vsd->vend_num; i++ ) {
		if( vsd->vending[i].amount == 0 )
			continue;

		if( cursor != i ) { // speedup
			vsd->vending[cursor].index = vsd->vending[i].index;
			vsd->vending[cursor].amount = vsd->vending[i].amount;
			vsd->vending[cursor].value = vsd->vending[i].value;
		}

		cursor++;
	}

	vsd->vend_num = cursor;

	//Always save BOTH: customer (buyer) and vender
	if( save_settings&CHARSAVE_VENDING ) {
		chrif_save(sd,0);
		chrif_save(vsd,0);
	}

	//check for @AUTOTRADE users [durf]
	if( vsd->state.autotrade ) {
		//see if there is anything left in the shop
		ARR_FIND( 0, vsd->vend_num, i, vsd->vending[i].amount > 0 );
		if( i == vsd->vend_num ) {
			//Close Vending (this was automatically done by the client, we have to do it manually for autovenders) [Skotlex]
			vending_closevending(vsd);
			map_quit(vsd);	//They have no reason to stay around anymore, do they?
		}
	}
}

/**
 * Player setup a new shop
 * @param sd : player opening the shop
 * @param message : shop title
 * @param data : itemlist data
 *	data := {<index>.w <amount>.w <value>.l}[count]
 * @param count : number of different items
 * @return 0 If success, 1 - Cannot open (die, not state.prevend, trading), 2 - No cart, 3 - Count issue, 4 - Cart data isn't saved yet, 5 - No valid item found
 */
char vending_openvending(struct map_session_data* sd, const char* message, const uint8* data, int count)
{
	int i, j, char_id;
	int vending_skill_lvl;
	char message_sql[MESSAGE_SIZE*2];
	
	nullpo_retr(false,sd);

	if ( pc_isdead(sd) || !sd->state.prevend || pc_istrading(sd)) {
		return 1; // can't open vendings lying dead || didn't use via the skill (wpe/hack) || can't have 2 shops at once
	}

	vending_skill_lvl = pc_checkskill(sd, MC_VENDING);
	
	// skill level and cart check
	if( !vending_skill_lvl || !pc_iscarton(sd) ) {
		clif_skill_fail(sd, MC_VENDING, USESKILL_FAIL_LEVEL, 0);
		return 2;
	}

	// check number of items in shop
	if( count < 1 || count > MAX_VENDING || count > 2 + vending_skill_lvl ) { // invalid item count
		clif_skill_fail(sd, MC_VENDING, USESKILL_FAIL_LEVEL, 0);
		return 3;
	}

	if (save_settings&CHARSAVE_VENDING) // Avoid invalid data from saving
		chrif_save(sd, 0);

	// filter out invalid items
	i = 0;
	for( j = 0; j < count; j++ ) {
		short index        = *(uint16*)(data + 8*j + 0);
		short amount       = *(uint16*)(data + 8*j + 2);
		unsigned int value = *(uint32*)(data + 8*j + 4);

		index -= 2; // offset adjustment (client says that the first cart position is 2)

		if( index < 0 || index >= MAX_CART // invalid position
		||  pc_cartitem_amount(sd, index, amount) < 0 // invalid item or insufficient quantity
		//NOTE: official server does not do any of the following checks!
		||  !sd->status.cart[index].identify // unidentified item
		||  sd->status.cart[index].attribute == 1 // broken item
		||  sd->status.cart[index].expire_time // It should not be in the cart but just in case
		||  (sd->status.cart[index].bound && !pc_can_give_bounded_items(sd)) // can't trade account bound items and has no permission
		||  ( sd->status.cart[index].card[0] == CARD0_CREATE && (char_id = MakeDWord(sd->status.cart[index].card[2],sd->status.cart[index].card[3])) > 0 && ((battle_config.bg_reserved_char_id && char_id == battle_config.bg_reserved_char_id) || (battle_config.ancient_reserved_char_id && char_id == battle_config.ancient_reserved_char_id)) )
		||  !itemdb_cantrade(&sd->status.cart[index], pc_get_group_level(sd), pc_get_group_level(sd)) ) // untradeable item
			continue;

		sd->vending[i].index = index;
		sd->vending[i].amount = amount;
		sd->vending[i].value = min(value, (unsigned int)battle_config.vending_max_value);

		// Player just moved item to cart and we don't have the correct cart ID yet.
		if (sd->status.cart[sd->vending[i].index].id == 0) {
			struct item_data *idb = itemdb_search(sd->status.cart[index].nameid);
			char msg[256];

			sprintf(msg, msg_txt(sd, 733), idb->jname);
			clif_displaymessage(sd->fd, msg);
			clif_skill_fail(sd, MC_VENDING, USESKILL_FAIL_LEVEL, 0);
			return 4;
		}

		i++; // item successfully added
	}

	if( i != j )
		clif_displaymessage (sd->fd, msg_txt(sd,266)); //"Some of your items cannot be vended and were removed from the shop."

	if( i == 0 ) { // no valid item found
		clif_skill_fail(sd, MC_VENDING, USESKILL_FAIL_LEVEL, 0); // custom reply packet
		return 5;
	}

	sd->state.prevend = 0;
	sd->state.vending = true;
	sd->vender_id = vending_getuid();
	sd->vend_num = i;
	safestrncpy(sd->message, message, MESSAGE_SIZE);
	
	Sql_EscapeString( mmysql_handle, message_sql, sd->message );

	if( Sql_Query( mmysql_handle, "INSERT INTO `%s`(`id`,`account_id`,`char_id`,`sex`,`map`,`x`,`y`,`title`,`autotrade`, `body_direction`, `head_direction`, `sit`) "
		"VALUES( %d, %d, %d, '%c', '%s', %d, %d, '%s', %d, '%d', '%d', '%d' );",
		vendings_db, sd->vender_id, sd->status.account_id, sd->status.char_id, sd->status.sex == 0 ? 'F' : 'M', map[sd->bl.m].name, sd->bl.x, sd->bl.y, message_sql, sd->state.autotrade, sd->ud.dir, sd->head_dir, pc_issit(sd) ) != SQL_SUCCESS ) {
		Sql_ShowDebug(mmysql_handle);
	}

	for( i = 0; i < count; i++ ) {
		if( Sql_Query( mmysql_handle, "INSERT INTO `%s`(`vending_id`,`index`,`cartinventory_id`,`amount`,`price`) VALUES( %d, %d, %d, %d, %d );", vending_items_db, sd->vender_id, i, sd->status.cart[sd->vending[i].index].id, sd->vending[i].amount, sd->vending[i].value ) != SQL_SUCCESS ) {
			Sql_ShowDebug(mmysql_handle);
		}
	}

	clif_openvending(sd,sd->bl.id,sd->vending);
	clif_showvendingboard(&sd->bl,message,0);

	idb_put(vending_db, sd->status.char_id, sd);

	if( battle_config.channel_announces&16 && server_channel[CHN_VENDING] )
	{
		char chat_message[256];
		sprintf(chat_message, msg_txt(NULL,820), server_channel[CHN_VENDING]->name, sd->status.name, sd->message, map[sd->bl.m].name, sd->bl.x, sd->bl.y);
		clif_channel_message(server_channel[CHN_VENDING], chat_message, 27);
	}

	if( map[sd->bl.m].flag.vending_cell )
		map_setcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_NOVENDING, false);

	return 0;
}

/**
 * Checks if an item is being sold in given player's vending.
 * @param sd : vender session (player)
 * @param nameid : item id
 * @return 0:not selling it, 1: yes
 */
bool vending_search(struct map_session_data* sd, unsigned short nameid)
{
	int i;

	if( !sd->state.vending ) { // not vending
		return false;
	}

	ARR_FIND( 0, sd->vend_num, i, sd->status.cart[sd->vending[i].index].nameid == (short)nameid );
	if( i == sd->vend_num ) { // not found
		return false;
	}

	return true;
}

/**
 * Searches for all items in a vending, that match given ids, price and possible cards.
 * @param sd : The vender session to search into
 * @param s : parameter of the search (see s_search_store_search)
 * @return Whether or not the search should be continued.
 */
bool vending_searchall(struct map_session_data* sd, const struct s_search_store_search* s)
{
	int i, c, slot;
	unsigned int idx, cidx;
	struct item* it;

	if( !sd->state.vending ) // not vending
		return true;

	for( idx = 0; idx < s->item_count; idx++ ) {
		ARR_FIND( 0, sd->vend_num, i, sd->status.cart[sd->vending[i].index].nameid == (short)s->itemlist[idx] );
		if( i == sd->vend_num ) { // not found
			continue;
		}
		it = &sd->status.cart[sd->vending[i].index];

		if( s->min_price && s->min_price > sd->vending[i].value ) { // too low price
			continue;
		}

		if( s->max_price && s->max_price < sd->vending[i].value ) { // too high price
			continue;
		}

		if( s->card_count ) { // check cards
			if( itemdb_isspecial(it->card[0]) ) { // something, that is not a carded
				continue;
			}
			slot = itemdb_slot(it->nameid);

			for( c = 0; c < slot && it->card[c]; c ++ ) {
				ARR_FIND( 0, s->card_count, cidx, s->cardlist[cidx] == it->card[c] );
				if( cidx != s->card_count ) { // found
					break;
				}
			}

			if( c == slot || !it->card[c] ) { // no card match
				continue;
			}
		}

		if( !searchstore_result(s->search_sd, sd->vender_id, sd->status.account_id, sd->message, it->nameid, sd->vending[i].amount, sd->vending[i].value, it->card, it->refine) ) { // result set full
			return false;
		}
	}

	return true;
}

/**
* Open vending for Autotrader
* @param sd Player as autotrader
*/
void vending_reopen( struct map_session_data* sd )
{
	nullpo_retv(sd);

	// Ready to open vending for this char
	if ( autotrader_count > 0 && autotraders) {
		uint16 i;
		uint8 *data, *p, fail = 0;
		uint16 j, count;

		ARR_FIND(0,autotrader_count,i,autotraders[i] && autotraders[i]->char_id == sd->status.char_id);
		if (i >= autotrader_count) {
			return;
		}
		
		// Init vending data for autotrader
		CREATE(data, uint8, autotraders[i]->count * 8);

		for (j = 0, p = data, count = autotraders[i]->count; j < autotraders[i]->count; j++) {
			struct s_autotrade_entry *entry = autotraders[i]->entries[j];
			uint16 *index = (uint16*)(p + 0);
			uint16 *amount = (uint16*)(p + 2);
			uint32 *value = (uint32*)(p + 4);

			// Find item position in cart
			ARR_FIND(0, MAX_CART, entry->index, sd->status.cart[entry->index].id == entry->cartinventory_id);

			if (entry->index == MAX_CART) {
				count--;
				continue;
			}

			*index = entry->index + 2;
			*amount = itemdb_isstackable(sd->status.cart[entry->index].nameid) ? entry->amount : 1;
			*value = entry->price;

			p += 8;
		}

		// Set him into a hacked prevend state
		sd->state.prevend = 1;

		// Make sure abort all NPCs
		npc_event_dequeue(sd);
		pc_cleareventtimer(sd);

		// Open the vending again
		if( (fail = vending_openvending(sd, autotraders[i]->title, data, count)) == 0 ) {
			// Set him to autotrade
			if (Sql_Query( mmysql_handle, "UPDATE `%s` SET `autotrade` = 1, `body_direction` = '%d', `head_direction` = '%d', `sit` = '%d' "
				"WHERE `id` = %d;",
				vendings_db, autotraders[i]->dir, autotraders[i]->head_dir, autotraders[i]->sit, sd->vender_id ) != SQL_SUCCESS ) {
				Sql_ShowDebug( mmysql_handle );
			}

			// Make vendor look perfect
			pc_setdir(sd, autotraders[i]->dir, autotraders[i]->head_dir);
			clif_changed_dir(&sd->bl, AREA_WOS);
			if( autotraders[i]->sit ) {
				pc_setsit(sd);
				skill_sit(sd, 1);
				clif_sitting(&sd->bl,true);
			}

			// Immediate save
			chrif_save(sd, 3);

			ShowInfo("Loaded vending for '"CL_WHITE"%s"CL_RESET"' with '"CL_WHITE"%d"CL_RESET"' items at "CL_WHITE"%s (%d,%d)"CL_RESET"\n",
				sd->status.name, count, mapindex_id2name(sd->mapindex), sd->bl.x, sd->bl.y);
		} else {
			// Failed to open the vending, set him offline
			ShowError("Failed (%d) to load autotrade vending data for '"CL_WHITE"%s"CL_RESET"' with '"CL_WHITE"%d"CL_RESET"' items\n", fail, sd->status.name, count );

			map_quit( sd );
		}

		aFree(data);

		//If the last autotrade is loaded, clear autotraders [Cydh]
		if (++autotrader_loaded_count >= autotrader_count)
			do_final_vending_autotrade();
	}
}

/**
* Initializing autotraders from table
*/
void do_init_vending_autotrade(void)
{
	if (battle_config.feature_autotrade) {
		uint16 i, items = 0;
		autotrader_count = autotrader_loaded_count = 0;

		// Get autotrader from table. `map`, `x`, and `y`, aren't used here
		// Just read player that has data at vending_items [Cydh]
		if (Sql_Query(mmysql_handle,
			"SELECT `id`, `account_id`, `char_id`, `sex`, `title`, `body_direction`, `head_direction`, `sit` "
			"FROM `%s` "
			"WHERE `autotrade` = 1 AND (SELECT COUNT(`vending_id`) FROM `%s` WHERE `vending_id` = `id`) > 0 "
			"ORDER BY `id`;",
			vendings_db, vending_items_db ) != SQL_SUCCESS ) {
			Sql_ShowDebug(mmysql_handle);
			return;
		}

		if( (autotrader_count = (uint16)Sql_NumRows(mmysql_handle)) > 0 ) {
			// Init autotraders
			CREATE(autotraders, struct s_autotrade *, autotrader_count);

			if (autotraders == NULL) { //This is shouldn't happen [Cydh]
				ShowError("Failed to initialize vending autotraders!\n");
				Sql_FreeResult(mmysql_handle);
				return;
			}

			// Init each autotrader data
			i = 0;
			while (SQL_SUCCESS == Sql_NextRow(mmysql_handle) && i < autotrader_count) {
				size_t len;
				char* data;
				struct s_autotrade *at = NULL;

				CREATE(autotraders[i], struct s_autotrade, 1);
				at = autotraders[i];

				Sql_GetData(mmysql_handle, 0, &data, NULL); at->vendor_id = atoi(data);
				Sql_GetData(mmysql_handle, 1, &data, NULL); at->account_id = atoi(data);
				Sql_GetData(mmysql_handle, 2, &data, NULL); at->char_id = atoi(data);
				Sql_GetData(mmysql_handle, 3, &data, NULL); at->sex = (data[0] == 'F') ? 0 : 1;
				Sql_GetData(mmysql_handle, 4, &data, &len); safestrncpy(at->title, data, min(len + 1, MESSAGE_SIZE));
				Sql_GetData(mmysql_handle, 5, &data, NULL); at->dir = atoi(data);
				Sql_GetData(mmysql_handle, 6, &data, NULL); at->head_dir = atoi(data);
				Sql_GetData(mmysql_handle, 7, &data, NULL); at->sit = atoi(data);
				at->count = 0;

				if (battle_config.feature_autotrade_direction >= 0)
					at->dir = battle_config.feature_autotrade_direction;
				if (battle_config.feature_autotrade_head_direction >= 0)
					at->head_dir = battle_config.feature_autotrade_head_direction;
				if (battle_config.feature_autotrade_sit >= 0)
					at->sit = battle_config.feature_autotrade_sit;

				// initialize player
				CREATE(at->sd, struct map_session_data, 1);
				pc_setnewpc(at->sd, at->account_id, at->char_id, 0, gettick(), at->sex, 0);
				at->sd->state.autotrade = 1;
				at->sd->state.monster_ignore = (battle_config.autotrade_monsterignore);
				chrif_authreq(at->sd, true);
				i++;
			}
			Sql_FreeResult(mmysql_handle);

			//Init items on vending list each autotrader
			for (i = 0; i < autotrader_count; i++) {
				struct s_autotrade *at = NULL;
				uint16 j;

				if ((at = autotraders[i]) == NULL)
					continue;

				if (SQL_ERROR == Sql_Query(mmysql_handle,
					"SELECT `cartinventory_id`, `amount`, `price` "
					"FROM `%s` "
					"WHERE `vending_id` = %d "
					"ORDER BY `index` ASC;",
					vending_items_db, at->vendor_id ) ) {
					Sql_ShowDebug(mmysql_handle);
					continue;
				}

				if (!(at->count = (uint16)Sql_NumRows(mmysql_handle))) {
					map_quit(at->sd);
					continue;
				}
			
				//Init the list
				CREATE(at->entries, struct s_autotrade_entry *,at->count);

				//Add the item into list
				j = 0;
				while (SQL_SUCCESS == Sql_NextRow(mmysql_handle) && j < at->count) {
					char* data;
					CREATE(at->entries[j], struct s_autotrade_entry, 1);

					Sql_GetData(mmysql_handle, 0, &data, NULL); at->entries[j]->cartinventory_id = atoi(data);
					Sql_GetData(mmysql_handle, 1, &data, NULL); at->entries[j]->amount = atoi(data);
					Sql_GetData(mmysql_handle, 2, &data, NULL); at->entries[j]->price = atoi(data);
					j++;
				}
				items += j;
				Sql_FreeResult(mmysql_handle);
			}

			ShowStatus("Done loading '"CL_WHITE"%d"CL_RESET"' vending autotraders with '"CL_WHITE"%d"CL_RESET"' items.\n", autotrader_count, items);
		}
	}

	// Everything is loaded fine, their entries will be reinserted once they are loaded
	if (Sql_Query( mmysql_handle, "DELETE FROM `%s`;", vendings_db ) != SQL_SUCCESS ||
		Sql_Query( mmysql_handle, "DELETE FROM `%s`;", vending_items_db ) != SQL_SUCCESS) {
		Sql_ShowDebug(mmysql_handle);
	}
}

/**
* Clear all autotraders
* @author [Cydh]
*/
void do_final_vending_autotrade(void)
{
	if (autotrader_count && autotraders) {
		uint16 i = 0;
		while (i < autotrader_count) { //Free the autotrader
			if (autotraders[i] == NULL)
				continue;
			if (autotraders[i]->count) {
				uint16 j = 0;
				while (j < autotraders[i]->count) { //Free the autotrade entries
					if (autotraders[i]->entries == NULL)
						continue;
					if (autotraders[i]->entries[j])
						aFree(autotraders[i]->entries[j]);
					j++;
				}
				aFree(autotraders[i]->entries);
			}
			aFree(autotraders[i]);
			i++;
		}
		aFree(autotraders);
		autotrader_count = 0;
	}
}

/**
 * Initialise the vending module
 * called in map::do_init
 */
void do_final_vending(void)
{
	db_destroy(vending_db);
	do_final_vending_autotrade(); //Make sure everything is cleared [Cydh]
}

/**
 * Destory the vending module
 * called in map::do_final
 */
void do_init_vending(void)
{
	vending_db = idb_alloc(DB_OPT_BASE);
	vending_nextid = 0;
}
