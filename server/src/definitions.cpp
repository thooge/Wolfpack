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

// Library Includes
#include <QtXml>
#include <QFile>
#include <QStack>
#include <QRegExp>
#include <QStringList>
#include <QByteArray>

// Reloading
#include "ai/ai.h"
#include "spawnregions.h"
#include "territories.h"
#include "skills.h"
#include "items.h"
#include "world.h"
#include "skills.h"
#include "serverconfig.h"
#include "basechar.h"
#include "network/network.h"
#include "quests.h"

#include "definitions.h"

#include "basics.h"
#include "inlines.h"
#include "console.h"
#include "commands.h"
#include "network/encryption.h"
#include "basedef.h"
#include "log.h"

#include <stdlib.h>

struct stCategory
{
	const char* name;
	eDefCategory key;
};

stCategory categories[] =
{
{ "item", WPDT_ITEM },
{ "script", WPDT_SCRIPT },
{ "npc", WPDT_NPC },
{ "list", WPDT_LIST },
{ "menu", WPDT_MENU },
{ "spell", WPDT_SPELL },
{ "acl", WPDT_PRIVLEVEL },
{ "spawnregion", WPDT_SPAWNREGION },
{ "region", WPDT_REGION },
{ "multi", WPDT_MULTI },
{ "text", WPDT_TEXT },
{ "startitems", WPDT_STARTITEMS },
{ "location", WPDT_LOCATION },
{ "skill", WPDT_SKILL },
{ "action", WPDT_ACTION },
{ "make", WPDT_MAKESECTION },
{ "makeitem", WPDT_MAKEITEM },
{ "useitem", WPDT_USEITEM },
{ "skillcheck", WPDT_SKILLCHECK },
{ "define", WPDT_DEFINE },
{ "resource", WPDT_RESOURCE },
{ "contextmenu", WPDT_CONTEXTMENU },
{ "ai", WPDT_AI },
{ "teleporter", WPDT_TELEPORTER },
{ "quest", WPDT_QUEST },
{ 0, WPDT_COUNT },
};

class cDefManagerPrivate
{
public:
	QMap<QString, cElement*> unique[WPDT_COUNT];
	QMap<unsigned int, QList<cElement*> > nonunique;
	QStringList imports;
};

cDefinitions::Iterator cDefinitions::begin(eDefCategory type) {
	return impl->unique[type].begin();
}

cDefinitions::Iterator cDefinitions::end(eDefCategory type) {
	return impl->unique[type].end();
}

class cXmlHandler : public QXmlDefaultHandler
{
private:
	cDefManagerPrivate* impl;
	// Element level within the current file
	QList<int> levels;
	// Holds all read elements.
	QStack<cElement*> elements;
	// Files stack (each <include> pushes a file)
	QList<QString> filenames;
	// Locators associated to each document
	QStack<QXmlLocator*> locators;

public:
	cXmlHandler( cDefManagerPrivate* impl )
	{
		this->impl = impl;
	}

	virtual ~cXmlHandler()
	{
		while ( !elements.isEmpty() )
		{
			cElement *parent = 0;
			while ( !elements.isEmpty() && elements.top() != NULL )
			{
				parent = elements.pop();
			}
			if ( parent ) delete parent;

			while ( !elements.isEmpty() && elements.top() == NULL )
			{
				elements.pop();
			}
		}
	}

	void load( const QString& filename )
	{
		QFile file( filename );

		if ( !file.open( QIODevice::ReadOnly ) )
		{
			Console::instance()->send( tr( "Unable to open %1!\n" ).arg( filename ) );
			return;
		}

		filenames.push_back( filename );

		QXmlInputSource input( &file );
		QXmlSimpleReader reader;
		reader.setFeature( "http://trolltech.com/xml/features/report-whitespace-only-CharData", false );

		reader.setContentHandler( this );
		reader.setErrorHandler( this );
		reader.parse( &input, false );

		filenames.pop_back();
	}

	void setDocumentLocator( QXmlLocator* locator )
	{
		locators.push( locator );
	}

	bool startDocument()
	{
		levels.push_back( 0 );
		return true;
	}

	bool endDocument()
	{
		levels.pop_back();
		locators.pop();
		return true;
	}

	bool startElement( const QString& /*namespaceURI*/, const QString& localName, const QString& qName, const QXmlAttributes& atts )
	{
		levels.back()++;

		// Ignore document root
		if ( levels.back() == 1 )
		{
			if ( levels.isEmpty() )
			{
				// Top level
				elements.push( NULL );
			}
			else
			{
				// Within an include
				if ( elements.isEmpty() )
					elements.push( 0 );
				else
					elements.push( elements.top() );
			}
			return true;
		}

		// Include another file
		if ( qName == "include" )
		{
			QString value = atts.value( "file" );
			load( value );

			elements.push( elements.top() );
			return true;
		}

		cElement* element = new cElement;
		element->setName( localName.toLatin1() );
		element->copyAttributes( atts );

		// Child Element?
		if ( elements.top() != NULL )
		{
			cElement* parent = elements.top(); // Pop the potential parent
			parent->addChild( element ); // Add the child to it's parent
			element->setParent( parent );
		} else {
			Definitions::instance()->addElement(element);
		}

		elements.push( element ); // Push our element (there may be children)

		return true;
	}

	bool endElement( const QString& /*namespaceURI*/, const QString& /*localName*/, const QString& /*qName*/ )
	{
		cElement* element = elements.pop();
		if ( --( levels.back() ) == 0 )
		{
			// Ignore root
			return true;
		}

		if ( element == elements.top() )
		{
			// Ignore include
			return true;
		}

		// Did we complete a parent node?
		if ( elements.top() == NULL )
		{
			// Find a category node
			unsigned int i = 0;

			// Sort it into a category.
			while ( categories[i].name != 0 )
			{
				if ( element->name() == categories[i].name )
				{
					QString tagId = element->getAttribute( "id" );

					// If the element has an id,
					if ( !tagId.isEmpty() )
					{
						if ( impl->unique[categories[i].key].contains( tagId ) && !Config::instance()->overwriteDefinitions() )
						{
							Console::instance()->log( LOG_WARNING, tr( "Duplicate %1: %2\n[File: %3, Line: %4]\n" )
								.arg( QString( element->name() ) ).arg( tagId ).arg( filenames.back() ).arg( locators.top()->lineNumber() ) );
							delete element;
						}
						else
						{
							impl->unique[categories[i].key].insert( tagId, element );
						}
					}
					else
					{
						impl->nonunique[categories[i].key].push_back( element );
					}

					return true;
				}
				++i;
			}

			Console::instance()->log( LOG_WARNING, tr( "Unknown element: %1\n[File: %2, Line: %3]\n" )
				.arg( QString( element->name() ) ).arg( filenames.back() ).arg( locators.top()->lineNumber() ) );
			delete element;
		}

		return true;
	}

	bool characters( const QString& ch )
	{
		if ( !elements.isEmpty() )
		{
			cElement *element = elements.top();
			if ( element )
				element->setText( element->text() + ch );
		}

		return true;
	}

	// error handling
	bool warning( const QXmlParseException& exception )
	{
		Console::instance()->log( LOG_WARNING, tr( "%1\n[File: %2, Line: %3, Column: %4]" )
			.arg( exception.message(), filenames.back() ).arg( exception.lineNumber() ).arg( exception.columnNumber() ) );
		return true; // continue
	}
	bool error( const QXmlParseException& exception )
	{
		Console::instance()->log( LOG_ERROR, tr( "%1\n[File: %2, Line: %3, Column: %4]" )
			.arg( exception.message(), filenames.back() ).arg( exception.lineNumber() ).arg( exception.columnNumber() ) );
		return true; // continue
	}
	bool fatalError( const QXmlParseException& exception )
	{
		Console::instance()->log( LOG_ERROR, tr( "%1\n[File: %2, Line: %3, Column: %4]" )
			.arg( exception.message(), filenames.back() ).arg( exception.lineNumber() ).arg( exception.columnNumber() ) );
		return true; // continue
	}
};

void cDefinitions::unload()
{
	QStringList oldAISections = getSections( WPDT_AI );
	QStringList::iterator it;
	for ( it = oldAISections.begin(); it != oldAISections.end(); ++it )
	{
		AIFactory::instance()->unregisterType( *it );
	}

	// Clear the nodes
	unsigned int i;
	for ( i = 0; i < WPDT_COUNT; ++i )
	{
		impl->unique[i].clear();
		impl->nonunique[i].clear();
	}

	impl->imports.clear();
	CharBaseDefs::instance()->reset();
	ItemBaseDefs::instance()->reset();
	listcache_.clear();

	// Free the memory allocated by our nodes.
	QList<cElement*>::iterator eit;
	for (eit = elements.begin(); eit != elements.end(); ++eit) {
		delete *eit;
	}
	elements.clear();

	cComponent::unload();
}

void cDefinitions::reload( void )
{
	QStringList oldAISections = Definitions::instance()->getSections( WPDT_AI );

	unload();

	load();

	// Update all SubSystems associated with this Definition Manager
	AIFactory::instance()->checkScriptAI( oldAISections, Definitions::instance()->getSections( WPDT_AI ) );
}

// Load the Definitions
void cDefinitions::load( void )
{
	impl->imports = Config::instance()->getString( "General", "Definitions", "definitions/index.xml", true ).split( ";" );

	for ( int i = 0; i < impl->imports.size(); ++i )
	{
		cXmlHandler handler( impl );
		handler.load( impl->imports[i] );
	}

	// create a list cache, because reading all the lists on the fly
	// means wasting time
	QMap<QString, cElement*>::iterator it = impl->unique[WPDT_LIST].begin();

	while ( it != impl->unique[WPDT_LIST].end() )
	{
		cElement* DefSection = it.value();

		QStringList list;
		QString data;
		for ( unsigned int i = 0; i < DefSection->childCount(); ++i )
		{
			const cElement* childTag = DefSection->getChild( i );

			// Using the nodename is a very very bad habit
			// if the name of the node is "item" then
			// use the node value instead

			if ( childTag->name() == "item" )
				data = childTag->text();
			else
				data = childTag->name();

			int mult = childTag->getAttribute( "mult" ).toInt();
			if ( mult <= 0 )
				mult = 1;

			int j = 0;
			while ( j < mult )
			{
				list.push_back( data );
				++j;
			}
		}

		listcache_.insert( it.key(), list );

		++it;
	}

	Skills::instance()->load();
	Commands::instance()->loadACLs();
	KeyManager::instance()->load();
	Quests::instance()->load();

	// Script NPC AI types
	QStringList aiSections = Definitions::instance()->getSections( WPDT_AI );
	QStringList::const_iterator aiit = aiSections.begin();
	while ( aiit != aiSections.end() )
	{
		ScriptAI::registerInFactory( *aiit );
		++aiit;
	}

	cComponent::load();
}

// Returns a list of section-names found
QStringList cDefinitions::getSections( eDefCategory Type ) const
{
	// This is a VERY VERY slow function!
	QStringList result;

	QMap<QString, cElement*>::iterator it = impl->unique[Type].begin();

	while ( it != impl->unique[Type].end() )
	{
		result.push_back( it.key() );
		it++;
	}

	return result;
}

QString cDefinitions::getRandomListEntry( const QString& ListSection )
{
	QStringList* list = NULL;

	QMap<QString, QStringList>::iterator it = listcache_.find( ListSection );
	if ( it != listcache_.end() )
		list = &( it.value() );

	if ( !list || list->isEmpty() )
		return QString();
	else
		return ( *list )[RandomNum( 0, list->size() - 1 )];
}

QStringList cDefinitions::getList( const QString& ListSection )
{
	QStringList list;

	QMap<QString, QStringList>::iterator it = listcache_.find( ListSection );
	if ( it != listcache_.end() )
		list = it.value();

	return list;
}

QString cDefinitions::getText( const QString& /*TextSection*/ ) const
{
	/*const QDomElement* DefSection = this->getSection( WPDT_TEXT, TextSection );
	if( DefSection->isNull() )
		return QString();
	else
	{
		QString text = DefSection->text();
		if( text.left( 1 ) == "\n" || text.left( 0 ) == "\r" )
			text = text.right( text.length()-1 );
		text = text.replace( QRegExp( "\\t" ), "" );
		if( text.right( 1 ) == "\n" || text.right( 1 ) == "\r" )
			text = text.left( text.length()-1 );
		return text;
	}*/
	return QString();
}

cDefinitions::cDefinitions()
{
	impl = new cDefManagerPrivate;
}

cDefinitions::~cDefinitions()
{
	unload();
	delete impl;
}


const cElement* cDefinitions::getDefinition( eDefCategory type, const QString& id ) const
{
	QMap<QString, cElement*>::const_iterator it = impl->unique[type].find( id );

	if ( it == impl->unique[type].end() )
		return 0;
	else
		return it.value();
}

const QList<cElement*>& cDefinitions::getDefinitions( eDefCategory type ) const
{
	return impl->nonunique[type];
}

/*
 *	Implementation of cElement
 */

cElement::cElement() : nameHashKey( 0 ), name_( QString::null ), text_( QString::null )
{
	childCount_ = 0;
	attrCount_ = 0;
	children = 0;
	attributes = 0;
	parent_ = 0;

	//instances.append(this);
};

cElement::~cElement()
{
	freeAttributes();
	freeChildren();

	if ( parent_ )
		parent_->removeChild( this );

	//instances.remove(this);
}

void cElement::freeAttributes()
{
	if ( attributes != 0 )
	{
		for ( unsigned int i = 0; i < attrCount_; ++i )
			delete attributes[i];

		attrCount_ = 0;
		delete[] attributes;
		attributes = 0;
	}
}

void cElement::freeChildren()
{
	if ( children != 0 )
	{
		cElement **copy = children;
		unsigned int copyCount = childCount_;

		// This prevents removeChild from doing any work
		children = 0;
		childCount_ = 0;

		for ( unsigned int i = 0; i < copyCount; ++i )
			delete copy[i];

		delete[] copy;
	}
}

void cElement::copyAttributes( const QXmlAttributes& attributes )
{
	freeAttributes();
	attrCount_ = attributes.count();

	if ( attrCount_ > 0 )
	{
		this->attributes = new stAttribute * [attrCount_];

		for ( unsigned int i = 0; i < attrCount_; ++i )
		{
			this->attributes[i] = new stAttribute;
			this->attributes[i]->name = attributes.localName( i ).toLocal8Bit();
			this->attributes[i]->value = attributes.value( i );
		}
	}
}

void cElement::addChild( cElement* element )
{
	if ( children == 0 )
	{
		childCount_ = 1;
		children = new cElement * [1];
		children[0] = element;
	}
	else
	{
		cElement** newChildren = new cElement* [childCount_ + 1];

		// Copy over the old list
		memcpy( newChildren, children, childCount_ * sizeof( cElement * ) );
		newChildren[childCount_++] = element;

		delete[] children;
		children = newChildren;
	}
}

void cElement::removeChild( cElement* element )
{
	for ( unsigned int i = 0; i < childCount_; ++i )
	{
		if ( children[i] == element )
		{
			// Found the element we want to delete
			unsigned int offset = 0;

			cElement** newChildren = new cElement* [childCount_ - 1];

			for ( unsigned int j = 0; j < childCount_; ++j )
				if ( children[j] != element )
					newChildren[offset++] = children[j];

			childCount_--;
			delete[] children;
			children = newChildren;
			return;
		}
	}
}

bool cElement::hasAttribute( const QByteArray& name ) const
{
	for ( unsigned int i = 0; i < attrCount_; ++i )
		if ( attributes[i]->name == name )
			return true;

	return false;
}

const QString& cElement::getAttribute( const QByteArray& name, const QString& def ) const
{
	for ( unsigned int i = 0; i < attrCount_; ++i )
		if ( attributes[i]->name == name )
			return attributes[i]->value;

	return def;
}

void cElement::setName( const QByteArray& data )
{
	name_ = data;
}

const QString& cElement::name() const
{
	return name_;
}

uint cElement::nameHash() const
{
	if ( !nameHashKey )
		nameHashKey = elfHash( name_.toLatin1() );
	return nameHashKey;
}

void cElement::setText( const QString& data )
{
	text_ = data;
}

const QString& cElement::text() const
{
	return text_;
}

void cElement::setParent( cElement* parent )
{
	parent_ = parent;
}

const cElement* cElement::parent() const
{
	return parent_;
}

const cElement* cElement::getTopmostParent() const
{
	if ( parent_ )
	{
		return parent_->getTopmostParent();
	}

	return this;
}

const cElement* cElement::findChild( const QString& name ) const
{
	for ( unsigned int i = 0; i < childCount_; ++i )
	{
		if ( children[i]->name() == name.toLatin1() )
			return children[i];
	}

	return 0;
}

const cElement* cElement::getChild( unsigned int index ) const
{
	if ( index >= childCount_ )
		return 0;

	return children[index];
}

unsigned int cElement::childCount() const
{
	return childCount_;
}

QString cElement::value() const
{
	QString Value = text_;

	for ( unsigned int i = 0; i < childCount(); ++i )
	{
		const cElement* childTag = getChild( i );
		// <random />
		if ( childTag->name() == "random" )
		{
			// <random min="" max="" />
			if ( childTag->hasAttribute( "min" ) && childTag->hasAttribute( "max" ) )
			{
				QString min = childTag->getAttribute( "min" );
				QString max = childTag->getAttribute( "max" );

				if ( min.contains( "." ) || max.contains( "." ) )
					Value += QString::number( RandomNum( ( int ) min.toFloat(), ( int ) max.toFloat() ) );
				else
					Value += QString::number( RandomNum( min.toInt(), max.toInt() ) );
			}
			// <random valuelist="value1,value2,value3" />
			else if ( childTag->hasAttribute( "valuelist" ) )
			{
				QStringList RandValues = childTag->getAttribute( "valuelist" ).split( "," );
				Value += RandValues[RandomNum( 0, RandValues.size() - 1 )];
			}
			// <random list="listname" />
			else if ( childTag->hasAttribute( "list" ) )
			{
				Value += Definitions::instance()->getRandomListEntry( childTag->getAttribute( "list" ) );
			}
			// <random randomlist="listname1,listname2,listname3" />
			else if ( childTag->hasAttribute( "randomlist" ) )
			{
				QStringList RandValues = childTag->getAttribute( "randomlist" ).split( "," );
				Value += Definitions::instance()->getRandomListEntry( RandValues[RandomNum( 0, RandValues.size() - 1 )] );
			}
			// <random dice="1d6+2" />
			else if ( childTag->hasAttribute( "dice" ) )
			{
				Value += QString::number( rollDice( childTag->getAttribute( "dice" ) ) );
			}
			// <random value="10-20" />
			else if ( childTag->hasAttribute( "value" ) )
			{
				QStringList parts = childTag->getAttribute( "value", "0-0" ).split( "-" );

				if ( parts.count() >= 2 )
				{
					QString min = parts[0];
					QString max = parts[1];

					if ( max.contains( "." ) || min.contains( "." ) )
						Value += QString::number( RandomNum( ( int ) min.toFloat(), ( int ) max.toFloat() ) );
					else
						Value += QString::number( RandomNum( min.toInt(), max.toInt() ) );
				}
			}
			else
				Value += QString( "0" );
		}
	}

	return hex2dec( Value );
}

/*
	\object element
	\description This object type represents an element in the definitions.
*/
struct wpElement
{
	PyObject_HEAD;
	cElement* element;
};

static PyObject* wpElement_getAttr( wpElement* self, char* name );

PyTypeObject wpElementType =
{
PyObject_HEAD_INIT( NULL )
0,
"element",
sizeof( wpElementType ),
0,
wpDealloc,
0,
( getattrfunc ) wpElement_getAttr,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/*
	\method element.hasattribute
	\param name The name of the attribute
	\return True or False.
	\description This method checks if the element has an attribute with the
	given name and returns True if so, False otherwise.
*/
static PyObject* wpElement_hasattribute( wpElement* self, PyObject* args )
{
	char* name;

	if ( !PyArg_ParseTuple( args, "s:element.hasattribute(name)", &name ) )
	{
		return 0;
	}

	if ( self->element->hasAttribute( name ) )
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

/*
	\method element.getattribute
	\param name The name of the attribute.
	\param default The return value if the attribute doesn't exist. Defaults to an empty string.
	\return A unicode string containing the value of the attribute or the default value if the
	attribute doesn't exist.
	\description This method retrieves the string value of an attribute.
*/
static PyObject* wpElement_getattribute( wpElement* self, PyObject* args )
{
	char* name;
	char* value = 0;

	if ( !PyArg_ParseTuple( args, "s|es:element.getattribute(name, [default])", &name, "utf-8", &value ) )
	{
		return 0;
	}

	QString defvalue = QString::null;

	if ( value != 0 )
	{
		defvalue = QString::fromUtf8( value );
		PyMem_Free( value );
	}

	QString result = self->element->getAttribute( name, defvalue );

	return QString2Python( result );
}

/*
	\method element.findchild
	\param name The name of the child element.
	\return An <object id="element">element</object> object for the child or None.
	\description This method tries to find a child element with the given name. If
	none could be found, None is returned. Otherwise the child element is returned.
*/
static PyObject* wpElement_findchild( wpElement* self, PyObject* args )
{
	char* name;

	if ( !PyArg_ParseTuple( args, "s:element.findchild(name)", &name ) )
	{
		return 0;
	}

	cElement* result = const_cast<cElement*>( self->element->findChild( name ) );

	if ( result )
	{
		return result->getPyObject();
	}
	else
	{
		Py_RETURN_NONE;
	}
}

/*
	\method element.getchild
	\param index The index of the child element.
	\return An <object id="element">element</object> object for the child or None.
	\description This method returns a child element at the given index or None if
	the index exceeds the number of child elements - 1.
*/
static PyObject* wpElement_getchild( wpElement* self, PyObject* args )
{
	unsigned int pos;

	if ( !PyArg_ParseTuple( args, "I:element.getchild(pos)", &pos ) )
	{
		return 0;
	}

	cElement* result = const_cast<cElement*>( self->element->getChild( pos ) );

	if ( result )
	{
		return result->getPyObject();
	}
	else
	{
		Py_RETURN_NONE;
	}
}

static PyMethodDef methods[] =
{
	{"hasattribute", ( getattrofunc ) wpElement_hasattribute, METH_VARARGS, 0},
	{"getattribute", ( getattrofunc ) wpElement_getattribute, METH_VARARGS, 0},
	{"findchild", ( getattrofunc ) wpElement_findchild, METH_VARARGS, 0},
	{"getchild", ( getattrofunc ) wpElement_getchild, METH_VARARGS, 0},
	{0, 0, 0, 0}
};

static PyObject* wpElement_getAttr( wpElement* self, char* name )
{
	cElement* element = self->element;

	// \rproperty element.name A string containing the name of the element.
	if ( !strcmp( name, "name" ) )
	{
		return QString2Python( element->name() );
	}
	/*
		\rproperty element.parent An <object id="element">element</object> object for the parent of this element.
		If there is no parent this property is None.
	*/
	else if ( !strcmp( name, "parent" ) )
	{
		if ( element->parent() )
		{
			cElement* parent = const_cast<cElement*>( element->parent() );
			return parent->getPyObject();
		}
		else
		{
			Py_RETURN_NONE;
		}
	}
	/*
		\rproperty element.value The parsed value of this node.
		This converts hexadecimal numbers to decimal numers and
		computes random values. The result still is a unicode string.
	*/
	else if ( !strcmp( name, "value" ) )
	{
		return QString2Python( element->value() );
	}
	/*
		\rproperty element.text This is the text enclosed by this element.
	*/
	else if ( !strcmp( name, "text" ) )
	{
		return QString2Python( element->text() );
	}
	/*
		\rproperty element.childcount This is the number of children this
		element has.
	*/
	else if ( !strcmp( name, "childcount" ) )
	{
		return PyInt_FromLong( self->element->childCount() );
	}

	return Py_FindMethod( methods, ( PyObject * ) self, name );
}

// Python Scripting Interface for elements
PyObject* cElement::getPyObject()
{
	wpElement* returnVal = PyObject_New( wpElement, &wpElementType );
	returnVal->element = this;
	return ( PyObject * ) returnVal;
}

const char* cElement::className() const
{
	return "element";
}

bool cElement::implements( const QString& name ) const
{
	if ( name == "element" )
	{
		return true;
	}
	else
	{
		return cPythonScriptable::implements( name );
	}
}

