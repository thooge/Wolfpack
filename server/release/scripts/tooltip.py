#################################################################
#   )      (\_     # WOLFPACK 13.0.0 Scripts                    #
#  ((    _/{  "-;  # Created by: codex                          #
#   )).-' {{ ;'`   # Revised by:                                #
#  ( (  ;._ \\ ctr # Last Modification: Created                 #
#################################################################

from wolfpack.consts import *
import wolfpack
from wolfpack.utilities import *
import re

# Register as a global script
def onLoad():
   wolfpack.registerglobal( EVENT_SHOWTOOLTIP, "tooltip" )

def onShowTooltip( sender, target, tooltip ):  

	if target.isitem():
		name = target.getname()
            
		if isarmor ( target ):
			armor( target, tooltip )
	
		elif ishat( target ):
			hat( target, tooltip )
	
		elif isshield( target ):
			shield( target, tooltip )
	         
		elif isweapon( target ):
			weapon( target, tooltip )
	                 
		else:
	#	   if target.name == '' or target.name == '#':
   	#params = str( target.amount ) + "\t" + '#' + str( 0xF9060 + target.id )
   	#print params
   	#tooltip.add( 1050039, params )
   #else:
   	#tooltip.add( 1050039, str( target.amount ) + "\t" + target.getname() ) #$amount $name
		
			if target.name == '#' or target.name == '':
				if target.amount > 1:
					tooltip.add( 1050045, " \t#" + str( 0xF9060 + target.id ) + "\t: " + str( target.amount ) )
				else:			
					tooltip.add( 0xF9060 + target.id, '' )
				
			else:
				if target.amount > 1:
					tooltip.add( 1050045, " \t" + name + "\t: " + str( target.amount ) )
				else:
					tooltip.add( 1050045, " \t" + name + "\t " )
	
			if target.hastag( "blessed" ):
				tooltip.add( 1038021, "" ) # Blessed
		
			if isspellbook( target ):
				tooltip.add( 1042886, str( target.spellscount() ) )
	else:
		name = target.name
		tooltip.add( 1050045, " \t" + name + "\t " )

	tooltip.send( sender )

	return 1

def armor( target, tooltip ):
   name = target.getname()
   tooltip.add( 1050045, " \t" + name + "\t " ) #prefix name suffix
   if (target.hastag( "blessed")):
      tooltip.add( 1038021, "" ) # Blessed
   tooltip.add( 1060448, "3" )                #Physical Resist 
   tooltip.add( 1060447, "1" )                #Fire Resist
   tooltip.add( 1060445, "1" )                #Cold Resist
   tooltip.add( 1060449, "5" )                #Poison Resist
   tooltip.add( 1060446, "3" )                #Energy Resist
   if target.hastag( 'req_str' ):
      tooltip.add( 1061170, str( target.gettag( 'req_str' ) ) )               #Strength Requirement
   tooltip.add( 1060639, str( target.health ) + "\t" + str( target.maxhealth ) )           #Durability

def container( target, tooltip ):
   tooltip.add( 1050045, "Bag" )
   tooltip.add( 1050044, "1000\t10000" )     #$count items, $weight stones

def shield( target, tooltip ):
   name = target.getname()
   tooltip.add( 1050045, " \t" + name + "\t " )
   if (target.hastag( "blessed")):
      tooltip.add( 1038021, "" ) # Blessed
   tooltip.add( 1060448, "1" )         #Physical resist
   if target.hastag( 'req_str' ):
      tooltip.add( 1061170, str( target.gettag( 'req_str' ) ) )               #Strength Requirement
   tooltip.add( 1060639, str( target.health ) + "\t" + str( target.maxhealth ) )  #Durability

def hat( target, tooltip ):
   name = target.getname()
   tooltip.add( 1050045, " \t" + name + "\t " )
   if (target.hastag( "blessed" )):
      tooltip.add( 1038021, "" ) # Blessed
   tooltip.add( 1060447, "5" )         #Fire Resist
   tooltip.add( 1060445, "9" )         #Cold Resist
   tooltip.add( 1060449, "5" )         #Poison Resist
   tooltip.add( 1060446, "3" )         #Energy Resist
   if target.hastag( 'req_str' ):
      tooltip.add( 1061170, str( target.gettag( 'req_str' ) ) )               #Strength Requirement
   tooltip.add( 1060639, str( target.health ) + "\t"  + str( target.maxhealth) )    #Durability

def weapon( target, tooltip ):
   name = target.getname()
   tooltip.add( 1050045, " \t" + name + "\t " )
   if (target.hastag( "blessed" )):
      tooltip.add( 1038021, "" ) # Blessed
   tooltip.add( 1060403, "100" ) # Physical Damage
   tooltip.add( 1061168, str( target.lodamage ) + " \t" + str(target.hidamage))# Weapon Damage
   tooltip.add( 1061167, str( target.speed ))
   if target.hastag( 'req_str' ):
      tooltip.add( 1061170, str( target.gettag( 'req_str' ) ) )               #Strength Requirement
   if ( target.twohanded ):
      tooltip.add( 1061171, "" ) # two-handed
   else:
      tooltip.add( 1061824, "" ) # One-handed
   if ( target.type ) == 1001 or ( target.type ) == 1002:
      tooltip.add( 1061172, "" ) # Weapon Skill: Swordsmanship
   if ( target.type ) == 1003 or ( target.type ) == 1004:
      tooltip.add( 1061173, "" ) # Weapon Skill: Mace Fighting
   if ( target.type ) == 1005:
      tooltip.add( 1061174, "" ) #Weapon Skill: Fencing
   if ( target.type ) == 1006 or ( target.type ) == 1007:
      tooltip.add( 1061175, "" ) # Weapon Skill: Archery
   tooltip.add( 1060639, str( target.health ) + "\t" + str( target.maxhealth ))
