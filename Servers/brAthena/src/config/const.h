/****************************************************************************!
*                _           _   _   _                                       *    
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *  
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *   
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *    
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *    
*                                                                            *
*                                                                            *
* \file src/config/const.h                                                   *
* Descri��o Prim�ria.                                                        *
* Descri��o mais elaborada sobre o arquivo.                                  *
* \author brAthena, rAthena                                                  *
* \date ?                                                                    *
* \todo ?                                                                    *  
*****************************************************************************/

#ifndef _RRCONFIGS_CONST_
#define _RRCONFIGS_CONST_

#include "../config/secure.h"

/**
 * Checagens de seguran�a para tempo de intervalo nos di�logos com npcs.
 **/
#if SECURE_NPCTIMEOUT_INTERVAL <= 0
	#error SECURE_NPCTIMEOUT_INTERVAL should be at least 1 (1s)
#endif
#if SECURE_NPCTIMEOUT < 0
	#error SECURE_NPCTIMEOUT cannot be lower than 0
#endif

/**
 * Sufixo para arquivos no banco de dados da renova��o/pr�-renova��o
 **/
#ifdef RENEWAL
	#define DBPATH "_re.txt"
#else
	#define DBPATH "_pre-re.txt"
#endif

/**
 * DefType
 **
#ifdef RENEWAL
 **/
	typedef short defType;
	#define DEFTYPE_MIN SHRT_MIN
	#define DEFTYPE_MAX SHRT_MAX
/*
#else
	typedef signed char defType;
	#define DEFTYPE_MIN CHAR_MIN
	#define DEFTYPE_MAX CHAR_MAX
#endif
*/

/* Defini��es para corrigir v�rios gcc warnings */
#ifdef __64BIT__
	#define __64BPRTSIZE(y) (intptr)y
#else
	#define __64BPRTSIZE(y) y
#endif

/* C�lculos de ATCMD_FUNC(mobinfo) HIT e FLEE */
#ifdef RENEWAL
	#define MOB_FLEE(mob) ( mob->lv + mob->status.agi + mob->status.luk/5 + 100 )
	#define MOB_HIT(mob)  ( mob->lv + mob->status.dex + mob->status.luk/3 + 175 )
#else
	#define MOB_FLEE(mob) ( mob->lv + mob->status.agi )
	#define MOB_HIT(mob)  ( mob->lv + mob->status.dex )
#endif

/* Modo de renova��o para modifica��es de dano conforme o n�vel */
#ifdef RENEWAL_LVDMG
	#define RE_LVL_DMOD(val) \
		if( status_get_lv(src) > 100 && val > 0 ) \
			skillratio = skillratio * status_get_lv(src) / val;
	#define RE_LVL_MDMOD(val) \
		if( status_get_lv(src) > 100 && val > 0) \
			md.damage = md.damage * status_get_lv(src) / val;
	#define RE_LVL_TMDMOD() \
		if( status_get_lv(src) > 100 ) \
			md.damage = md.damage * 150 / 100 + md.damage * status_get_lv(src) / 100;
#else
	#define RE_LVL_DMOD(val) 
	#define RE_LVL_MDMOD(val)
	#define RE_LVL_TMDMOD()
#endif

/* N�mero m�ximo de carrinhos para clientes de 2012 */
#if PACKETVER >= 20120201
	#define NEW_CARTS
	#define MAX_CARTS 9
#else
	#define MAX_CARTS 5
#endif

// Redu��o do tempo de conjura��o vari�vel da renova��o.
#ifdef RENEWAL_CAST
	#define VARCAST_REDUCTION(val){ \
		if( (varcast_r += val) != 0 && varcast_r >= 0 ) \
			time = time * (1 - (float)min(val, 100) / 100); \
	}
#endif
#endif
