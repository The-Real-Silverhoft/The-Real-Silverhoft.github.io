/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/flash/media/videodecoder.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void MediaVideoDecoder::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("H264AVC",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"H264Avc"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SORENSON",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"Sorenson"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VP6",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"VP6"),CONSTANT_TRAIT);
}