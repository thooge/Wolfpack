#===============================================================#
#   )      (\_     | WOLFPACK 13.0.0 Scripts                    #
#  ((    _/{  "-;  | Created by: DarkStorm                      #
#   )).-' {{ ;'`   | Revised by: Radiant                        #
#  ( (  ;._ \\ ctr | Last Modification: Implented necromancy    #
#===============================================================#
# Script for Spellbooks                                         #
#===============================================================#

"""
	\command addspell
	\description Add a spell to a spellbok.
	\usage - <code>addspell id</code>
	- <code>addspell all</code>
	Id is the id of the spell you want to add.
	If you use 'addspell all', all available spells
	will be added to the book.
"""

"""
	\command removespell
	\description Remove a spell from a spellbook.
	\usage - <code>removespell id</code>
	- <code>removespell all</code>
	Id is the id of the spell that should be removed
	from the book. If you use 'removespell all', all
	spells will be removed from the book.
"""

import wolfpack
from math import floor,ceil
from wolfpack import tr

def commandAddSpell( socket, command, arguments ):
	if arguments != 'all':
		try:
			if len( arguments ) < 1:
				raise Exception

			spell = int( arguments )
		except:
			socket.sysmessage( tr('Usage: addspell <spell> or addspell all') )
			return

		if spell >= 64:
			socket.sysmessage( tr('Values between 0 and 63 are valid.') )
			return
	else:
		spell = 'all'

	socket.sysmessage( tr('Select the spellbook you want to add the spell to.') )
	socket.attachtarget( "magic.spellbook.addspelltarget", [ spell ] )

def commandRemoveSpell( socket, command, arguments ):
	if arguments != 'all':
		try:
			if len( arguments ) < 1:
				raise Exception

			spell = int( arguments )
		except:
			socket.sysmessage( tr('Usage: removespell <spell> or removespell all') )
			return

		if spell >= 64:
			socket.sysmessage( tr('Values between 0 and 63 are valid.') )
			return
	else:
		spell = 'all'

	socket.sysmessage( tr('Select the spellbook you want to add the spell to.') )
	socket.attachtarget( "magic.spellbook.removespelltarget", [ spell ] )

def onLoad():
	# Register .addspell, .removespell, .editspells (?)
	wolfpack.registercommand( "addspell", commandAddSpell )
	wolfpack.registercommand( "removespell", commandRemoveSpell )

# Does the Spellbook have a specific spell?
def hasspell( item, spell ):
	if item and item.hasscript( 'magic.spellbook' ):
		circle = int( floor( spell / 8 ) ) + 1 # 0 for first circle
		spell = spell % 8

		if item.hastag( 'circle' + str( circle ) ):
			spells = int( item.gettag( 'circle' + str( circle ) ) )

			return spells & ( 0x01 << spell )

	return False

# spellcount
def countspells(item):
	count = 0

	for i in range( 1, 9 ):
		if item.hastag('circle' + str(i)):
			spells = int(item.gettag('circle' + str(i)))
			for j in range(0, 9):
				if (spells >> j) & 0x01:
					count += 1
	return count

# Adds the specified spell to the specified spellbook
def addspell( item, spell ):
	if not item or not item.hasscript( 'magic.spellbook' ):
		return False

	circle = int( floor( spell / 8 ) ) + 1 # 0 for first circle
	spell = spell % 8
	spells = 0
	if item.hastag( 'circle' + str( circle ) ):
		spells = int( item.gettag( 'circle' + str( circle ) ) )

	spells |= 0x01 << spell
	item.settag( 'circle' + str( circle ), spells )
	item.resendtooltip()

	return True

# Removes the specified spell from the specified spellbook
def removespell( item, spell ):
	if not item or not item.hasscript( 'magic.spellbook' ):
		return False

	# Circle
	circle = int( floor( spell / 8 ) ) + 1 # 0 for first circle
	spell = spell % 8

	if item.hastag( 'circle' + str( circle ) ):
		spells = int( item.gettag( 'circle' + str( circle ) ) )
		spells &= ~( 0x01 << spell )
		item.settag( 'circle' + str( circle ), spells )

	item.resendtooltip()

	return True

"""
	Add a spell to a spellbook
"""
def addspelltarget( char, args, target ):
	spell = args[0]

	item = target.item

	if not item or not item.hasscript( 'magic.spellbook' ):
		char.socket.sysmessage( tr('You need to target a spellbook.') )
		return


	if spell == 'all':
		for i in range( 1, 9 ):
			item.settag( 'circle' + str( i ), 0xFF )

		item.resendtooltip()
		char.socket.sysmessage( tr('Added all spells to the spellbook.') )
		return

	addspell( item, spell )

	circle = int( floor( spell / 8 ) ) + 1
	char.socket.sysmessage( tr('Added spell %d of circle %d (Id %d) to the spellbook.') % ( spell % 8 + 1, circle, spell ) )

"""
	Remove a spell from a spellbook
"""
def removespelltarget( char, args, target ):
	spell = args[0]

	item = target.item

	if not item or not item.hasscript( 'magic.spellbook' ):
		char.socket.sysmessage( tr('You need to target a spellbook.') )
		return

	if spell == 'all':
		for i in range( 1, 9 ):
			item.deltag( 'circle' + str( i ) )
		item.resendtooltip()

		char.socket.sysmessage( tr('Removed all spells from the spellbook.') )
		return

	removespell( item, spell )

	circle = int( floor( spell / 8 ) ) + 1
	char.socket.sysmessage( tr('Removed spell %d of circle %d (Id %d)from the spellbook.') % ( spell % 8 + 1, circle, spell ) )

"""
	open up a spellbook
"""
def onUse( char, item ):
	if item.getoutmostchar() != char:
		char.socket.clilocmessage( 500207 ) # The spellbook must be in your backpack (and not in a container within) to open.
		return True

	# This is annoying and eats bandwith but its the only way to "reopen" the spellbook
	# once its already open.
	#char.socket.removeobject(item)
	if item.container and item.container.isitem():
		char.socket.sendobject(item.container)
	char.socket.sendobject(item)

	packet = wolfpack.packet( 0x24, 7 )
	packet.setint( 1, item.serial )
	packet.setshort( 5, 0xffff )
	packet.send( char.socket )

	packet = wolfpack.packet( 0xbf, 23 )
	packet.setshort( 1, 23 )	 # Packet length
	packet.setshort( 3, 0x1b )	 # 0xbf subcommand
	packet.setshort( 5, 1 )	 # Unknown. Maybe it's a subsubcommand ?
	packet.setint( 7, item.serial ) # Spellbook serial
	packet.setshort( 11, item.id ) # Item id
	packet.setshort( 13, 1 ) # Scroll offset (1 = regular, 101 = necro, 201 = paladin)

	for i in range( 0, 8 ):
		if not item.hastag( 'circle' + str( i + 1 ) ):
			packet.setbyte( 15 + i, 0 ) # Should be unneccesary
		else:
			packet.setbyte( 15 + i, int( item.gettag( 'circle' + str( i + 1 ) ) ) ) # Should be unneccesary

	packet.send( char.socket )

	return True

#
# Create the special tooltip
#
def onShowTooltip(viewer, object, tooltip):
	tooltip.add(1042886, str(countspells(object))) # ~1_NUMBERS_OF_SPELLS~ Spells
