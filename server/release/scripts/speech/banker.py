#===============================================================#
#   )      (\_     | WOLFPACK 13.0.0 Scripts                    #
#  ((    _/{  "-;  | Created by: DarkStorm                      #
#   )).-' {{ ;'`   | Revised by:                                #
#  ( (  ;._ \\ ctr | Last Modification: Created                 #
#===============================================================#
# Speech Script for Bankers                                     #
#===============================================================#

from wolfpack import tr
from wolfpack.consts import *
import wolfpack.utilities
import wolfpack
import re

#0x0000 "*withdraw*"
#0x0000 "*withdrawl*"
#0x0001 "*balance*"
#0x0001 "*statement*"
#0x0002 "*bank*"
#0x0003 "*check*"

MIN_CHECK_AMOUNT = 5000

amountre = re.compile( '(\d+)' )

def onSpeech( listener, speaker, text, keywords ):
	if not speaker.canreach(listener, 3):
		return False

	for keyword in keywords:
		# withdraw
		if keyword == 0x0:
			# Search for the amount we want to withdraw (should be the only digits in there)
			result = amountre.search( text )
			amount = 0

			if result:
				amount = int( result.group( 1 ) )

			# Invalid Withdraw Amount
			if not amount:
				# Thou must tell me how much thou wishest to withdraw
				speaker.socket.clilocmessage( 500379, "", 0x3b2, 3, listener )
				return

			# Withraw
			else:
				# Check if the player has enough gold in his bank
				bank = speaker.getbankbox()
				gold = 0

				if bank:
					gold = bank.countresource( 0xEED, 0x0 )

				if amount > gold:
					# Thou dost not have sufficient funds in thy account to withdraw that much.
					speaker.socket.clilocmessage( 500382, "", 0x3b2, 3, listener )
					return

				else:
					# We have enough money, so let's withdraw it
					# Thou hast withdrawn gold from thy account.
					speaker.socket.clilocmessage( 1010005, "", 0x3b2, 3, listener )
					bank.useresource( amount, 0xEED, 0x0 )
					backpack = speaker.getbackpack()

					while amount > 0:
						item = wolfpack.additem("eed")
						item.amount = min( [ amount, 65535 ] )
						amount -= item.amount
            
						if not wolfpack.utilities.tocontainer(item, backpack):
							item.update()            
            
					speaker.soundeffect( 0x37, 0 )
          
			break

		# balance (count all gold)
		elif keyword == 0x1:
			listener.turnto(speaker)
			bank = speaker.getbankbox()
			amount = bank.countresource(0xeed, 0x0)
			if not amount:
				listener.say(tr("Alas you don't have any money in your bank."))
			else:
				listener.say(tr("You have %i gold in your bank.") % amount)
			return True

		# bank
		elif keyword == 0x2:
			if speaker.dead:
				listener.say(500895) # That sounded spooky.
				return True

			if listener.gettag('allowcriminals') != 1 and speaker.iscriminal():
				listener.say(500378) # Thou art a criminal and cannot access thy bank box.
				return True

			bank = speaker.getbankbox()
			listener.turnto(speaker)
			listener.say(tr("Here is your bank box, %s.") % speaker.name)
			speaker.socket.sendcontainer(bank)
			return True

		# check
		elif keyword == 0x3:
			result = amountre.search( text )
			amount = 0

			if result:
				amount = int( result.group( 1 ) )

			# Invalid Withdraw Amount
			if amount < MIN_CHECK_AMOUNT:
				speaker.socket.clilocmessage( 500383, "", 0x3b2, 3, listener ) # Thou must request a minimum of 5000 gold to draw up a check.
				return

			# Withraw
			else:
				# Check if the player has enough gold in his bank
				bank = speaker.getbankbox()
				gold = 0

				if bank:
					gold = bank.countresource( 0xEED, 0x0 )

				if amount > gold:
					speaker.socket.clilocmessage( 500380, "", 0x3b2, 3, listener ) # Ah, art thou trying to fool me? Thou hast not so much gold!
					return
				else:
					# We have enough money, so let's withdraw it
					# Into your bank box I have placed a check in the amount of:
					total = str(amount)
					speaker.socket.clilocmessage( 1042673, "", 0x3b2, 3, listener, " %s" %total ) # Into your bank box I have placed a check in the amount of:
					bank.useresource( amount, 0xEED, 0x0 )
					check = wolfpack.additem( "bank_check" )
					check.settag( 'value', amount )
					bank.additem(check)
					check.update()
          
					# Resend Status (Money changed)
					speaker.socket.resendstatus()
			return True

	return False

# An item has been dropped on us
def onDropOnChar( char, item ):
	player = item.container
	
	if not player or not player.ischar() or not player.socket:
		return False
	
	# Move to bankbox if gold
	if item.id == 0xeed:
		bankbox = player.getbankbox()
		if not wolfpack.utilities.tocontainer(item, bankbox):
			item.update()
		return True
	else:	
		char.say( 500388, "", "", False, 0x3b2, player.socket ) # I only accept gold coins.

	return 0
