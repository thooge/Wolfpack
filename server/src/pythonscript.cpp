/*
 *     Wolfpack Emu (WP)
 * UO Server Emulation Program
 *
 * Copyright 2001-2016 by holders identified in AUTHORS.txt
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Palace - Suite 330, Boston, MA 02111-1307, USA.
 *
 * In addition to that license, if you are running this program or modified
 * versions of it on a public system you HAVE TO make the complete source of
 * the version used by you available or provide people with a location to
 * download it.
 *
 * Wolfpack Homepage: http://developer.berlios.de/projects/wolfpack/
 */

#include "pythonscript.h"

#include "muls/maps.h"
#include "network/network.h"
#include "console.h"

// Library Includes
#include <QFile>
#include <QtGlobal>
#include <QByteArray>

// Extension includes
#include "python/utilities.h"
#include "python/target.h"

// Keep this in Synch with the Enum on the header file
static char* eventNames[] =
{
/*
\event onUse
\param player The player who used the item.
\param item The item that was used.
\return Return 1 if your function should handle the event. If you return 0,
the server will try to process the event internally or call other scripts.
\condition Triggered when a player doubleclicks an item.
\notes This event is called for the character first and then for the item.
*/
"onUse",

/*
\event onSingleClick
\param item The item that was singleclicked.
\param viewer The player who singleclicked on the item.
\return Return 1 if your function handles the event. If you return 0,
the server will try to process the event internally or call other scripts.
\condition Triggered when the client requests the name of an immovable item.
\notes This event is only called for the item clicked on and not for the character
who clicked.
*/
"onSingleClick",

/*
\event onCollide
\param char The character who stepped on the item.
\param item The item that was stepped on.
\return Return 1 if your function handles the event. If you return 0, the core will
call the remaining scipts in the chain.
\condition Triggered when a character walks onto the item. This event is only triggered
if the character was not previously standing on the item.
\notes This event is only called for the item and not for the character.
*/
"onCollide",

/*
\event onWalk
\param char The character who requested to walk.
\param dir The direction the character tries to walk to.
\param sequence The packet sequence number.
\return Return 1 if your function handles the event. If you return 0, the core will
call the remaining scipts in the chain and try to handle the event itself.
\condition Triggered when any character tries to walk. The event is called before
any other checks are made.
\note Please be careful when using this event. It's called very often and therefor
could slow your server down if used extensively. Please note that you are
responsible for denying and accepting walk requests if you return 1 in this event
handler.
*/
"onWalk",

/*
\event onCreate
\param object The object that was just created.
\param id The definition id used to create the object.
\condition Triggered when either a character or an item is created from a
XML definition. Objects that are not created from a XML definition don't
trigger this event. They wouldn't have a script to call anyway.
*/
"onCreate",

/*
\event onTalk
\param player The player who talked.
\param type The speech type. Valid values are:
0x00 - Regular.
0x01 - Broadcast.
0x02 - Emote.
0x08 - Whisper.
0x09 - Yell.
\param color The color or the text sent by the client.
\param font The font for the speech. The default is 3.
\param text The text of the speech request.
\param lang The three letter code of the used language.
\return Return 1 if you want to ignore the speech request or handle it otherwise.
\condition Triggered when a player talks ingame. This event is not triggered for npcs.
*/
"onTalk",

/*
\event onWarModeToggle
\param player The player who changed his warmode status.
\param mode The new warmode status. Possible values are:
0 - Peace
1 - War
\condition Triggered when a player changes his warmode status.
*/
"onWarModeToggle",

/*
\event onLogin
\param player The player that is entering the world.
\condition Triggered when a player enters the world.
\notes onLogin isn't called if the character was lingering (as the character didn't
really leave the world). See onConnect.
*/
"onLogin",

/*
\event onLogout
\param player The player that is leaving the world.
\condition Triggered when a player leaves the world.
\notes onLogout takes the char timeout into consideration. See onDisconnect.
*/
"onLogout",

/*
\event onHelp
\param player The player who pressed the button.
\condition Triggered when a player pressed the help button on his paperdoll.
\return Return 1 to override the internal server behaviour.
*/
"onHelp",

/*
\event onChat
\param player The player who pressed the button.
\condition Triggered when a player pressed the chat button on his paperdoll.
\return Return 1 to override the internal server behaviour.
*/
"onChat",

/*
\event onSkillUse
\param player The player who used the skill.
\param skill The id of the skill that was used.
\condition Triggered when a player tries to actively use a skill by selecting
its button on the skill gump or activating a macro.
\return Return 1 to override the internal server behaviour.
*/
"onSkillUse",

/*
\event onSkillGain
\param player The player who has a chance to gain.
\param skill The skill id that was used.
\param min The lower difficulty range.
\param max The upper difficulty range.
\param success If the try to use the skill was successful.
\condition Triggered when a player used any skill (passive and active)
and has a chance to gain in that skill.
\notes Please note that the server does not determine whether the
skillgain was successful or not.
\return Return 1 to override the remaining scripts in the script chain.
*/
"onSkillGain",

/*
\event onShowPaperdoll
\param char The character whose paperdoll is requested.
\param player The player who is requesting the paperdoll.
\condition Triggered when a player requests the paperdoll of
another character by doubleclicking on him.
\return Return 1 to override the internal server behaviour.
*/
"onShowPaperdoll",

/*
\event onShowSkillGump
\param player The player who requested a list of his skills.
\condition Triggered when the complete list of skills is requested
by the client.
\return Return 1 to override the internal server behaviour.
*/
"onShowSkillGump",

/*
\event onDeath
\param char The character who died.
\param source The source of the lethal blow. This may be None.
\param corpse The corpse of the dead character. This may be None.
\condition Triggered when a character dies.
\notes This event is called before the death shroud for players has been
created.
*/
"onDeath",

/*
\event onShowPaperdollName
\param char The character whose paperdoll is being requested.
\param player The player who is requesting the paperdoll.
\condition Triggered when the paperdoll for a character is about
to be shown.
\return Return the text you want to have shown on the paperdoll or
otherwise None. Please note that the text may only be 59 characters
long and may only contain 7-bit ASCII characters.
*/
"onShowPaperdollName",

/*
\event onContextEntry
\param player The player who selected the context menu entry.
\param object The object the context menu was assigned to.
\param entry The id of the entry that was selected.
\condition Triggered when a character selects an entry from a
context menu.
\notes This event is only called for context menus but neither for the
object it was attached to nor for the player who selected the entry.
*/
"onContextEntry",

/*
\event onShowTooltip
\param player The player who requested the tooltip.
\param object The object the tooltip was requested for.
\param tooltip The <object id="tooltip">tooltip</object> object that is about to be sent.
\condition Triggered just before a tooltip is sent for an object.
\notes Please note that you cannot stop the tooltip from being sent.
You can only modify the tooltip. This event is only called for the
object and not for the player.
*/
"onShowTooltip",

/*
\event onCHLevelChange
\param player The player changing the level of his house.
\param level The level the player wants to change to.
\condition Triggered when a player changes the current
floor level while modifying his custom house.
*/
"onCHLevelChange",

/*
\event onSpeech
\param object The npc or item who heard the text.
\param player The player the text is coming from.
\param text The text.
\param keywords A list of numeric keywords. This is passed from the
client. See speech.mul for the meaning of keywords.
\condition Triggered when a npc or item hears text said by a player.
\return Return 1 if your npc or item understood what the player said,
no other npc scripts will be called then.
\notes If you Return 1 for your item, so no process for speech for npcs. So, be carefull.
*/
"onSpeech",

/*
\event onWearItem
\param player The player who is trying to equip an item.
\param char The character who is supposed to wear the item.
\param item The item.
\param layer The requested drop layer.
\condition Triggered when a player tries to equip an item.
\return Return 1 if the server should reject the equip request and
should bounce the item back.
\notes The event is called first for the item and then for the
character.
*/
"onWearItem",

/*
\event onEquip
\param char The character who is equipping the item.
\param item The item being equipped.
\param layer The layer the item is being equipped on.
\condition Triggered when a character equips an item.
*/
"onEquip",

/*
\event onUnequip
\param char The previous wearer of the item.
\param item The item being unequipped.
\param layer The layer the item was equipped on.
\condition Triggered when a character takes an item off.
*/
"onUnequip",

/*
\event onDropOnChar
\param char The character the item is being dropped on.
\param item The item.
\condition Triggered when an item is dropped on another character.
\return Return 1 to override the internal server behaviour. If you didn't
delete or move the item and still return 1, the server will automatically
bounce the item.
\notes You can find the player who is dropping the item in item.container.
*/
"onDropOnChar",

/*
\event onDropOnItem
\param target The item the player is dropping his item on.
\param item The dropped item.
\condition Triggered when an item is dropped on another item.
\return Return 1 to override the internal server behaviour.
\notes You can find the player who is dropping the item in item.container.
*/
"onDropOnItem",

/*
\event onDropOnGround
\param item The item being dropped.
\param pos The position the item is being dropped to.
\condition Triggered when an item is dropped to the ground.
\return Return 1 to override the internal server behaviour.
\notes You can find the player who is dropping the item in item.container.
*/
"onDropOnGround",

/*
\event onPickup
\param player The player trying to pick up an item.
\param item The requested item.
\condition Triggered when a player tries to pick up an item.
\return Return 1 to prevent the item from being picked up.
\notes The event is called first for the item and then for the
player.
*/
"onPickup",

/*
\event onDamage
\param char The character taking the damage.
\param type The damage type.
\param amount The amount of damage.
\param source The source the damage comes from. May be an item,
a character or None.
\condition Triggered when a character takes damage.
\return Return the new amount of damage as an integer.
\notes This event is only called for the victim. Please be careful
not to call the damage method of the character in this function to
prevent an endless loop.
*/
"onDamage",

/*
\event onCastSpell
\param player The player who requested to cast a spell.
\param spell The id of the spell the player wants to cast.
\condition Triggered when a player tries to cast a spell using his spellbook.
\return Return 1 to override the remaining scripts in the scriptchain.
*/
"onCastSpell",

/*
\event onTrade
\param player The player requesting to change the trade status.
\param type The request type.
\param buttonstate The accept button state.
\param itemserial The trade container serial.
\condition Triggered when a player requests to change the trade window state.
*/
"onTrade",

/*
\event onTradeStart
\param player The player initiating the trade.
\param partner The player the item was dropped on.
\param item The first item.
\condition Triggered when a player drops an item onto another player.
\notes This event is only called once for the initiating player.
*/
"onTradeStart",

/*
\event onDelete
\param object The object being deleted.
\condition Called when an item or character is deleted.
*/
"onDelete",

/*
\event onSwing
\param attacker The character swinging his weapon at another.
\param defender The attack target of the attacker.
\param time The servertime in miliseconds.
\condition Called when a character swings his weapon at another one.
\notes This is used to implement the combat system. Please note that this
event is not called for characters but only called as a global hook.
*/
"onSwing",

/*
\event onShowStatus
\param player The player who is requesting his status.
\param packet The status packet.
\condition Triggered just before the status packet is sent to a player.
\notes This is used to implement the combat system. Please note that this
event is not called for characters but only called as a global hook.
*/
"onShowStatus",

/*
\event onChangeRegion
\param char The character changing regions.
\param oldregion The last region the character was in.
\param newregion The new region the character is entering.
\condition Triggered when a character moves and the region he is in changed.
*/
"onChangeRegion",

/*
\event onAttach
\param object The object.
\condition Triggered when a script is attached to an object.
\notes This is even triggered when the item is loaded from a worldfile, but not
for scripts in the objects basescripts list.
*/
"onAttach",

/*
\event onDetach
\param object The object.
\condition Triggered when a script is removed from an object, but not
for scripts in the objects basescripts list.
*/
"onDetach",

/*
\event onTimeChange
\param object The object.
\condition This event is called for every connected client (or npcs) once an ingame hour has elapsed.
\notes If enabled by wolfpack.xml, it can be called for items too
*/
"onTimeChange",

/*
\event onDispel
\param player The player that is affected.
\param source The source character of the dispel effect. May be None.
\param silent Indicates that the effects should be silently dispelled.
This is a boolean value.
\param force Is the dispel effect dispelling even undispellable effects?
This is a boolean value.
\param dispelid Is the dispel effect limited to one kind of effect?
This is a string. If it's empty, all effects are affected.
\param dispelargs A tuple of arguments passed to the dispel function of the effects.
This may be an empty tuple.
\condition This event is called before a dispel effect affects the effects on this
character.
\return Return 1 to ignore the dispel effect.
*/
"onDispel",

/*
\event onTelekinesis
\param char The character trying to use telekinesis on this object.
\param item The object being used.
\condition This event is called when someone tries to use telekinesis on this
item. It is only called for the item.
\return Return behaviour is the same as for onUse.
*/
"onTelekinesis",

/*
\event onContextCheckVisible
\param player The player who is requesting the contextmenu entry.
\param object The object the contextmenu is requested for.
\param tag The tag of the contextmenu entry.
\return Return 1 if the context menu entry should be visible. 0 otherwise.
\condition Triggered when a contextmenu entry with the "checkvisible" flag
is requested.
*/
"onContextCheckVisible",

/*
\event onContextCheckEnabled
\param player The player who is requesting the contextmenu entry.
\param object The object the contextmenu is requested for.
\param tag The tag of the contextmenu entry.
\return Return 1 if the context menu entry should be enabled. 0 otherwise.
\condition Triggered when a contextmenu entry with the "checkenabled" flag
is requested.
*/
"onContextCheckEnabled",

/*
\event onSpawn
\param region The spawn region that generated the object.
\param object The object just spawned.
\condition Triggered when a spawnregion creates a new object, either NPC or item.
*/
"onSpawn",

/*
\event onUpdateDatabase
\param current The current database version.
\param version The version of the loaded database.
\condition Triggered when the version of the loaded database doesn't match the version the server expects.
*/
"onUpdateDatabase",

/*
\event onGetSellPrice
\param item The item being checked.
\param vendor The vendor buying the item. This could be None.
\param player The player selling the item.
\return None if your event does not know the price. An integer
value otherwise.
\condition This event is triggered to get the sellprice for an item.
It's triggered for the item, then for the npc and then for the global hook.
*/
"onGetSellPrice",

/*
\event onShowVirtueGump
\param player The player who pressed the button.
\param target The owner of the paperdoll the button was on.
\condition Triggered when a player presses the virtue gump button on his or another characters paperdoll.
*/
"onShowVirtueGump",

/*
\event onResurrect
\param char The character being resurrected.
\param source The source of the resurrection. This may be None.
\condition Triggered when a character is resurrected.
\notes Return True to cancel the resurrection.
*/
"onResurrect",

/*
\event onCheckSecurity
\param player The player who is seeking access to an item.
\param multi The multi or the object the object is in.
\param item The item the player is seeking access to.
\condition Triggered when a player tries to use an item that is within a multi, tries to drop an item into a container that is within a multi or tries to grab
an item that is in a container that is in a multi.
\notes Return True to deny access to the item.
*/
"onCheckSecurity",

/*
\event onCheckVictim
\param npc The npc that is thinking about a new target.
\param victim The victim that is being thought about.
\param dist The distance in tiles to the victm.
\return True if the target is a valid victim, False otherwise.
\condition This is triggered when the NPC is looking for a better target.
It is triggered for every character that could be attacked by the NPC.
*/
"onCheckVictim",

/*
\event onDoDamage
\param char The character dealing the damage.
\param type The damage type.
\param amount The amount of damage.
\param victim The victim taking the damage.
\condition Triggered when a character deals damage to another character.
\return Return the new amount of damage dealt to the victim.
*/
"onDoDamage",

/*
\event onSnooping
\param owner The owner of the container that is trying to be looked into.
\param item The item that is being snooped into.
\param player The player trying to snoop into the container.
\return True if you want to handle this skill use.
\condition Triggered for the player trying to snoop first, then for the owner of the
container that is being snooped into.
*/
"onSnooping",

/*
\event onRemoteUse
\param player The player who used the item.
\param item The item that was used.
\return Return True if the item may be used, False otherwise.
\condition Triggered when a player tries to use an item that is within the belongings of another character.
\notes This even is called for the using player, then for the current owner of the item.
*/
"onRemoteUse",

/*
\event onConnect
\param player The player that connected.
\param reconnecting True if the player is reconnecting to an online character.
\condition Triggered when a player logs in with a character (even if the character was still online).
\notes If the character wasn't online, onLogin is called before onConnect.
*/
"onConnect",

/*
\event onDisconnect
\param player The player that disconnected.
\condition Triggered when a player disconnects.
\notes If the character was in a safe-logout zone, onLogout is called immediately after onDisconnect.
*/
"onDisconnect",

/*
\event onGuildButton
\param player The player who pressed the button.
\condition Triggered when a player presses the guild button on his client.
*/
"onGuildButton",

/*
\event onSelectAbility
\param player The player who is trying to use the ability.
\param ability The id of the weapon ability that is being activated.
\condition Triggered when a player tries to use the primary or secondary ability of his weapon.
*/
"onSelectAbility",

/*
\event onLog
\param loglevel The loglevel this log event was issued with.
\param source If the event originates from a player, this is the socket the event was associated with.
\param text The text that was associated with this log event.
\param target This is a target object associated with this event. This could be a charcter, item, socket or None.
\return True if you don't want the string to be logged.
\condition This event is only triggered globally. If an logging event occurs, this event will be called.
*/
"onLog",

/*
\event onRegenHitpoints
\param char The character who will recover Hitpoints
\param points The points for timer calculation to next Hitpoints recover.
\return Return how many points have to be added to calculations for the next Hitpoints recover
\condition Triggered when a character Recover Hitpoints from Regen time.
*/
"onRegenHitpoints",

/*
\event onRegenMana
\param char The character who will recover Mana
\param points The points for timer calculation to next Mana recover.
\return Return how many points have to be added to calculations for the next Mana recover
\condition Triggered when a character Recover Mana from Regen time.
*/
"onRegenMana",

/*
\event onRegenStamina
\param char The character who will recover Stamina
\param points The points for timer calculation to next Stamina recover.
\return Return how many points have to be added to calculations for the next Stamina recover
\condition Triggered when a character Recover Stamina from Regen time.
*/
"onRegenStamina",

/*
\event onBecomeCriminal
\param char The character who will becomes Criminal
\param reason The number of the Reason (See the Indexed table in notes)
\param sourcechar The Character source (or target) for criminal act (may be none)
\param sourceitem The Item source (or target) for criminal act (may be none)
\return True if you want to player becomes criminal or False to block player becomes criminal
\condition Triggered when a character will be flagged as a Criminal.
\notes The List of reason is: 0 when called by scripts (.criminal), 1 when called because player killed someone, 2 when called because player begin a fight against a Innocent target, 3 for Looting and related things
*/
"onBecomeCriminal",

/*
\event onQuestButton
\param player The player who pressed the button.
\condition Triggered when a player presses the quest button on his client.
*/
"onQuestButton",

/*
\event onRealDayChange
\condition Called when server detects the day on machine changed.
\notes Please note that this event is only called as a global hook. Its called when server restarts too.
*/
"onRealDayChange",

/*
\event onWorldSave
\condition Called in the beggining of World Save Process, before save things.
\notes Please note that this event is only called as a global hook.
*/
"onWorldSave",

/*
\event onServerHour
\condition Called when a In-Game hour has elapsed.
\notes Please note that this event is only called as a global hook, while onTimeChange is called for chars and items.
*/
"onServerHour",

/*
\event onSeeChar
\param npc The NPC who saw the Char.
\param char The Char that NPC saw.
\return Return 1 to break the looping for other chars. If returned 0 or nothing (no return) it will just check all remaining chars calling event more times.
\condition Triggered when a npc see other chars around him.
Npc must have an ai.
"Default AI NPCs Check Time" in wolfpack.xml has to be greater than 0.
\notes This event will be called for every character around the Main Char of Event. So, use carefully.
*/
"onSeeChar",

/*
\event onSeeItem
\param npc The NPC who saw the Item.
\param item The Item that NPC saw.
\return Return 1 to break the looping for other items. If returned 0 or nothing (no return) it will just check all remain items calling event more times.
\condition Triggered when a npc see items around him.
Npc must have an ai.
"Default AI Items Check Time" in wolfpack.xml has to be greater than 0.
\notes This event will be called for every item around the Main Char of Event. So, use carefully.
*/
"onSeeItem",

/*
\event onTimerRegenHitpoints
\param char The character who will recover Hitpoints.
\param timer The timer for next Hitpoints recover.
\return Return The timer value for next Hitpoints recover.
\condition Triggered when a character Will recover HitPoints and set next recover timer (Its called after OnRegenHitPoints).
*/
"onTimerRegenHitpoints",

/*
\event onTimerRegenMana
\param char The character who will recover Mana.
\param timer The timer for next Mana recover.
\return Return The timer value for next Mana recover.
\condition Triggered when a character Will recover Mana and set next recover timer (Its called after OnRegenMana).
*/
"onTimerRegenMana",

/*
\event onTimerRegenStamina
\param char The character who will recover Stamina
\param timer The timer for next Stamina recover.
\return Return The timer value for next Stamina recover.
\condition Triggered when a character Will recover Stamina and set next recover timer (Its called after OnRegenStamina).
*/
"onTimerRegenStamina",

/*
\event onPickupFromContainer
\param player The player trying to pick up an item.
\param item The Item player is trying to pick up from Container.
\param container The Container where item is.
\condition Triggered when a player tries to pick up an item from a Container.
\return Return 1 to prevent the item from being picked up.
\notes The event is called always for the Container.
*/
"onPickupFromContainer",

/*
\event onGetBuyPrice
\param item The item being checked.
\param vendor The vendor selling the item. This could be None.
\param player The player buying the item.
\return None if your event does not know the price. An integer
value otherwise.
\condition This event is triggered to get the buyprice for an item.
It's triggered for the item, then for the npc and then for the global hook.
*/
"onGetBuyPrice",

/*
\event onBuy
\param item The item a player is trying to Buy.
\param vendor The vendor selling the item.
\param player The player buying the item.
\param amount The amount of that item player wants to buy from Vendor
\return Returns the amount of items to be bought. If 0, item will be skipped. So, Return 0 to prevent players to buy items. If you return a negative value, the Event will stop the buy action for all items.
\condition This item is triggered when a player try to buy a item from a NPC.
It's triggered for the item, then for the npc and then for the player.
*/
"onBuy",

/*
\event onUpdateAcctDatabase
\param current The current account database version.
\param version The version of the loaded account database.
\condition Triggered when the version of the loaded account database doesn't match the version the server expects.
*/
"onUpdateAcctDatabase",

/*
\event onStepChar
\param player The Player who stepped a character.
\param char The Character stepped by the player.
\return Return 1 if your function handles the event and you want to permit player to step.
Return 2 if your function handles the event and you want to deny the step to player.
If you return 0, the core will call the remaining scripts in the chain.
\condition Triggered when a character walks onto another character. This event is only triggered
if the character was not previously standing on the stepped character.
*/
"onStepChar",

/*
\event onStepWeightPercent
\param player The Player who stepped under certain Weight Percent.
\param percent The percent of MaxWeight this Char is carrying.
\param mounted If player is mounted or not (Bool)
\param running If player is running or not (Bool)
\return Return 1 if your function handles the event and you want to permit player to step.
Return 2 if your function handles the event and you want to deny the step.
If you return 0, the core will call the remaining scripts in the chain.
\condition Triggered when a character walks under certain Weight percent (Configurable by xml)
\notes This event will not be called for dead people or gms.
*/
"onStepWeightPercent",

/*
\event onReachDestination
\param npc The NPC who reached his destination.
\condition Triggered when a NPC reaches his given destination.
*/
"onReachDestination",


0
};

cPythonScript::cPythonScript() : loaded( false )
{
	codeModule = 0;
	for ( unsigned int i = 0; i < EVENT_COUNT; ++i )
	{
		events[i] = 0;
	}
}

cPythonScript::~cPythonScript()
{
	if ( loaded )
		unload();
}

/*
	\event onUnload
	\return None
	\condition Triggered when the script is unloaded.
*/
void cPythonScript::unload( void )
{
	loaded = false;

	// Free Cached Events
	for ( unsigned int i = 0; i < EVENT_COUNT; ++i )
	{
		if ( events[i] )
		{
			events[i] = 0;
		}
	}

	callEventHandler( "onUnload" );

	Py_XDECREF( codeModule );
	codeModule = 0;
}

/*
	\event onLoad
	\return None
	\condition Triggered when the script is loaded.
	\notes Wolfpack loads scripts before loading the worldfile, creating items in this event handler
	might result in corrupt worldfiles due to serial id clashes.
*/
// Find our module name
bool cPythonScript::load( const QByteArray& name )
{
	if ( name.isEmpty() )
	{
		return false;
	}

	setName( name );

	codeModule = PyImport_ImportModule( const_cast<char*>( name.data() ) );

	if ( !codeModule )
	{
		reportPythonError( name );
		return false;
	}

	// Call the onLoad event
	callEventHandler( "onLoad", 0, true );

	if ( PyErr_Occurred() )
	{
		reportPythonError( name_ );
	}

	// Get and cache the module's dictionary
	PyObject *pDict = PyModule_GetDict(codeModule); // BORROWED REFERENCE

	// Cache Event Functions
	for (unsigned int i = 0; i < EVENT_COUNT; ++i) {
		events[i] = PyDict_GetItemString( pDict, eventNames[i] );

		if (events[i] && !PyCallable_Check(events[i])) {
			Console::instance()->log( LOG_ERROR, tr( "Script %1 has non callable event: %1.\n" ).arg(QString(name_)).arg( eventNames[i] ) );
			Py_DECREF(events[i]);
			events[i] = 0;
		}
	}

	loaded = true;
	return true;
}

stError* cPythonScriptable::setProperty( const QString& name, const cVariant& /*value*/ )
{
	// No settable properties are available for this class
	PROPERTY_ERROR( -1, QString( "Property not found: '%1'" ).arg( name ) )
}

bool cPythonScript::isLoaded() const
{
	return loaded;
}

PyObject* cPythonScript::callEvent( ePythonEvent event, PyObject* args, bool ignoreErrors )
{
	PyObject* result = 0;

	if ( event < EVENT_COUNT && events[event] )
	{
		result = PyEval_CallObject( events[event], args ); // Using the DEFINE here should give a minor speed improvement

		if ( !ignoreErrors )
			reportPythonError( name_ );
	}

	return result;
}

PyObject* cPythonScript::callEvent( const QString& name, PyObject* args, bool ignoreErrors )
{
	PyObject* result = 0;

	if ( codeModule && !name.isEmpty() && PyObject_HasAttrString( codeModule, const_cast<char*>( name.toLatin1().data() ) ) )
	{
		PyObject* event = PyObject_GetAttrString( codeModule, const_cast<char*>( name.toLatin1().data() ) );

		if ( event && PyCallable_Check( event ) )
		{
			result = PyEval_CallObject( event, args );

			if ( !ignoreErrors )
				reportPythonError( name_ );
		}

		Py_XDECREF( event );
	}

	return result;
}

bool cPythonScript::canHandleEvent( const QString& event )
{
	if ( codeModule && !event.isEmpty() && PyObject_HasAttrString( codeModule, const_cast<char*>( event.toLatin1().data() ) ) )
	{
		PyObject* object = PyObject_GetAttrString( codeModule, const_cast<char*>( event.toLatin1().data() ) );

		if ( object )
		{
			if ( PyCallable_Check( object ) )
			{
				Py_DECREF( object );
				return true;
			}

			Py_DECREF( object );
		}
	}
	return false;
}

bool cPythonScript::callEventHandler( ePythonEvent event, PyObject* args, bool ignoreErrors )
{
	PyObject* result = callEvent( event, args, ignoreErrors );
	bool handled = false;

	if ( result )
	{
		handled = PyObject_IsTrue( result ) == 0 ? false : true;
		Py_DECREF( result );
	}

	return handled;
}

bool cPythonScript::callEventHandler( const QString& name, PyObject* args, bool ignoreErrors )
{
	PyObject* result = callEvent( name, args, ignoreErrors );
	bool handled = false;

	if ( result )
	{
		handled = PyObject_IsTrue( result ) == 0 ? false : true;
		Py_DECREF( result );
	}

	return handled;
}

// Standard Handler for Python ScriptChains assigned to objects
bool cPythonScript::callChainedEventHandler( ePythonEvent event, cPythonScript** chain, PyObject* args )
{
	bool handled = false;

	if ( chain )
	{
		// Measure
		size_t count = reinterpret_cast<size_t>( chain[0] );
		cPythonScript** copy = new cPythonScript*[count];
		for ( size_t j = 0; j < count; ++j )
			copy[j] = chain[j + 1];

		// Find a valid handler function
		for ( size_t i = 0; i < count; ++i )
		{
			PyObject* result = copy[i]->callEvent( event, args );

			if ( result )
			{
				if ( PyObject_IsTrue( result ) )
				{
					handled = true;
					Py_DECREF( result );
					break;
				}

				Py_DECREF( result );
			}
		}

		delete[] copy;
	}

	return handled;
}

PyObject* cPythonScript::callChainedEvent( ePythonEvent event, cPythonScript** chain, PyObject* args )
{
	PyObject* result = 0;

	if ( chain )
	{
		// Measure
		size_t count = reinterpret_cast<size_t>( chain[0] );
		cPythonScript** copy = new cPythonScript*[count];
		for ( size_t j = 0; j < count; ++j )
			copy[j] = chain[j + 1];

		// Find a valid handler function
		for ( size_t i = 0; i < count; ++i )
		{
			result = copy[i]->callEvent( event, args );

			if ( result && PyObject_IsTrue( result ) )
				break;
		}

		delete[] copy;
	}

	return result;
}

bool cPythonScript::canChainHandleEvent( ePythonEvent event, cPythonScript** chain )
{
	if ( !chain )
	{
		return false;
	}

	bool result = false;

	if ( event < EVENT_COUNT )
	{
		size_t count = reinterpret_cast<size_t>( *( chain++ ) );

		for ( size_t i = 0; i < count; ++i )
		{
			if ( chain[i]->canHandleEvent( event ) )
			{
				result = true;
				break;
			}
		}
	}
	return result;
}

PyObject* cPythonScriptable::getProperty( const QString& name, uint /*hash*/ )
{
	PY_PROPERTY( "classname", className() );

	// Apparently the property hasn't been found
	return 0;
}

bool cPythonScriptable::implements( const QString& name ) const
{
	if ( name == cPythonScriptable::className() )
	{
		return true;
	}
	else
	{
		return false;
	}
}

const char* cPythonScriptable::className() const
{
	return "pythonscriptable";
}

// Conversion Routines
bool cPythonScriptable::convertPyObject( PyObject* object, P_CHAR& pChar )
{
	if ( checkWpChar( object ) )
	{
		pChar = getWpChar( object );
		return true;
	}
	else if ( object == Py_None )
	{
		pChar = 0;
		return true;
	}
	else if ( PyInt_Check( object ) )
	{
		pChar = World::instance()->findChar( PyInt_AsLong( object ) );
		return true;
	}
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, P_ITEM& /*pItem*/ )
{
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, Coord& /*pos*/ )
{
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, QString& /*string*/ )
{
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, QByteArray& /*string*/ )
{
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, unsigned int& /*data*/ )
{
	return false;
}

bool cPythonScriptable::convertPyObject( PyObject* /*object*/, int& /*data*/ )
{
	return false;
}
